/**
 * @file drv_audio.c
 * @brief Gestion complète du pipeline audio TDM (SAI + DMA + callbacks DSP).
 */

#include "drv_audio.h"
#include "audio_codec_ada1979.h"
#include "audio_codec_pcm4104.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Buffers ping/pong                                                          */
/* -------------------------------------------------------------------------- */

static int32_t audio_in_buffers[2][AUDIO_FRAMES_PER_BUFFER][AUDIO_NUM_INPUT_CHANNELS];
static int32_t audio_out_buffers[2][AUDIO_FRAMES_PER_BUFFER][AUDIO_NUM_OUTPUT_CHANNELS];

static spilink_audio_block_t spi_in_buffers;
static spilink_audio_block_t spi_out_buffers;

static volatile uint8_t audio_in_ready_index = 0xFFU;
static volatile uint8_t audio_out_ready_index = 0xFFU;

/* SPI-LINK callbacks. */
static drv_spilink_pull_cb_t spilink_pull_cb = NULL;
static drv_spilink_push_cb_t spilink_push_cb = NULL;

/* Volume maître flottant (0.0 -> silence, 1.0 -> unity). */
static float audio_master_volume = 1.0f;

/* -------------------------------------------------------------------------- */
/* Synchronisation des DMA                                                    */
/* -------------------------------------------------------------------------- */

static binary_semaphore_t audio_dma_sem;

static const stm32_dma_stream_t *sai_rx_dma = NULL;
static const stm32_dma_stream_t *sai_tx_dma = NULL;

/* Nombre d'échantillons transférés par transaction (ping + pong). */
#define AUDIO_DMA_IN_SAMPLES   (AUDIO_FRAMES_PER_BUFFER * AUDIO_NUM_INPUT_CHANNELS * 2U)
#define AUDIO_DMA_OUT_SAMPLES  (AUDIO_FRAMES_PER_BUFFER * AUDIO_NUM_OUTPUT_CHANNELS * 2U)
/*
 * Les tableaux [2][frames][channels] sont vus par le DMA comme un buffer
 * linéaire unique : interruption Half-Transfer => index 0 (ping),
 * interruption Transfer-Complete => index 1 (pong).
 */

/* -------------------------------------------------------------------------- */
/* Prototypes internes                                                        */
/* -------------------------------------------------------------------------- */

static void audio_hw_configure_sai(void);
static void audio_dma_start(void);
static void audio_dma_stop(void);

static void audio_dma_rx_cb(void *p, uint32_t flags);
static void audio_dma_tx_cb(void *p, uint32_t flags);

static THD_WORKING_AREA(audioThreadWA, AUDIO_THREAD_STACK_SIZE);
static THD_FUNCTION(audioThread, arg);

/* -------------------------------------------------------------------------- */
/* API publique                                                               */
/* -------------------------------------------------------------------------- */

void drv_audio_init(void) {
    chBSemObjectInit(&audio_dma_sem, FALSE);

    /* Prépare le bus I2C et les codecs. */
    adau1979_init();
    audio_codec_pcm4104_init();

    memset(audio_in_buffers, 0, sizeof(audio_in_buffers));
    memset(audio_out_buffers, 0, sizeof(audio_out_buffers));
    memset(spi_in_buffers, 0, sizeof(spi_in_buffers));
    memset(spi_out_buffers, 0, sizeof(spi_out_buffers));

    /* Les GPIO SAI sont déjà configurés via board.h. */
    audio_hw_configure_sai();
}

void drv_audio_start(void) {
    audio_codec_pcm4104_set_mute(true);
    adau1979_set_default_config();

    audio_dma_start();
    chThdCreateStatic(audioThreadWA, sizeof(audioThreadWA), AUDIO_THREAD_PRIORITY, audioThread, NULL);
    audio_codec_pcm4104_set_mute(false);
}

void drv_audio_stop(void) {
    audio_dma_stop();
    audio_codec_pcm4104_set_mute(true);
}

const int32_t* drv_audio_get_input_buffer(uint8_t *index, size_t *frames) {
    chSysLock();
    uint8_t ready = audio_in_ready_index;
    audio_in_ready_index = 0xFFU;
    chSysUnlock();

    if (ready == 0xFFU) {
        return NULL;
    }
    if (index != NULL) {
        *index = ready;
    }
    if (frames != NULL) {
        *frames = AUDIO_FRAMES_PER_BUFFER;
    }
    return (const int32_t*)audio_in_buffers[ready];
}

int32_t* drv_audio_get_output_buffer(uint8_t *index, size_t *frames) {
    chSysLock();
    uint8_t ready = audio_out_ready_index;
    audio_out_ready_index = 0xFFU;
    chSysUnlock();

    if (ready == 0xFFU) {
        return NULL;
    }
    if (index != NULL) {
        *index = ready;
    }
    if (frames != NULL) {
        *frames = AUDIO_FRAMES_PER_BUFFER;
    }
    return audio_out_buffers[ready];
}

void drv_audio_release_buffers(uint8_t in_index, uint8_t out_index) {
    (void)in_index;
    (void)out_index;
    /* DMA circulaire : rien à faire, les buffers seront réécrits au prochain tour. */
}

int32_t (*drv_audio_get_spi_in_buffers(void))[AUDIO_FRAMES_PER_BUFFER][4] {
    return spi_in_buffers;
}

int32_t (*drv_audio_get_spi_out_buffers(void))[AUDIO_FRAMES_PER_BUFFER][4] {
    return spi_out_buffers;
}

size_t drv_audio_get_spi_frames(void) {
    return AUDIO_FRAMES_PER_BUFFER;
}

void drv_audio_register_spilink_pull(drv_spilink_pull_cb_t cb) {
    spilink_pull_cb = cb;
}

void drv_audio_register_spilink_push(drv_spilink_push_cb_t cb) {
    spilink_push_cb = cb;
}

void drv_audio_set_master_volume(float vol) {
    if (vol < 0.0f) {
        vol = 0.0f;
    }
    audio_master_volume = vol;
}

/* -------------------------------------------------------------------------- */
/* Hook DSP faible                                                            */
/* -------------------------------------------------------------------------- */

void __attribute__((weak)) drv_audio_process_block(const int32_t              *adc_in,
                                                   const spilink_audio_block_t spi_in,
                                                   int32_t                    *dac_out,
                                                   spilink_audio_block_t       spi_out,
                                                   size_t                      frames) {
    /* Pass-through par défaut : copie les 4 premiers canaux ADC vers le DAC,
     * mute SPI out et ignore SPI in. */
    const int32_t *adc_ptr = adc_in;
    int32_t *dac_ptr = dac_out;
    for (size_t n = 0; n < frames; ++n) {
        for (size_t ch = 0; ch < AUDIO_NUM_OUTPUT_CHANNELS; ++ch) {
            size_t src_ch = (ch < AUDIO_NUM_INPUT_CHANNELS) ? ch : 0U;
            int32_t sample = adc_ptr[src_ch];
            dac_ptr[ch] = (int32_t)((float)sample * audio_master_volume);
        }
        adc_ptr += AUDIO_NUM_INPUT_CHANNELS;
        dac_ptr += AUDIO_NUM_OUTPUT_CHANNELS;
    }

    (void)spi_in;
    if (spi_out != NULL) {
        memset(spi_out, 0, sizeof(spi_out_buffers));
    }
}

/* -------------------------------------------------------------------------- */
/* Thread audio : déclenché par les callbacks DMA                             */
/* -------------------------------------------------------------------------- */

static THD_FUNCTION(audioThread, arg) {
    (void)arg;
    chRegSetThreadName("audioProcess");

    while (!chThdShouldTerminateX()) {
        chBSemWait(&audio_dma_sem);

        uint8_t in_idx, out_idx;
        size_t frames = AUDIO_FRAMES_PER_BUFFER;

        chSysLock();
        if (audio_in_ready_index == 0xFFU || audio_out_ready_index == 0xFFU) {
            chSysUnlock();
            continue;
        }
        in_idx = audio_in_ready_index;
        out_idx = audio_out_ready_index;
        audio_in_ready_index = 0xFFU;
        audio_out_ready_index = 0xFFU;
        chSysUnlock();

        const int32_t *in_buf = (const int32_t *)audio_in_buffers[in_idx];
        int32_t *out_buf = audio_out_buffers[out_idx];

        /* Récupère l'audio des cartouches si disponible. */
        if (spilink_pull_cb != NULL) {
            spilink_pull_cb(spi_in_buffers, frames);
        } else {
            memset(spi_in_buffers, 0, sizeof(spi_in_buffers));
        }

        drv_audio_process_block(in_buf,
                                 spi_in_buffers,
                                 out_buf,
                                 spi_out_buffers,
                                 frames);

        /* Exporte le flux vers les cartouches si besoin. */
        if (spilink_push_cb != NULL) {
            spilink_push_cb(spi_out_buffers, frames);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Configuration SAI + DMA                                                    */
/* -------------------------------------------------------------------------- */

static void audio_hw_configure_sai(void) {
#if defined(STM32H7xx) && defined(SAI_xCR1_MODE_0)
    /* Active les horloges SAI et force la réinitialisation. */
    rccEnableSAI1(true);
    rccResetSAI1();

    /* Bloc B = maître RX TDM 8x32 bits, données valides sur 24 bits MSB. */
    AUDIO_SAI_RX_BLOCK->CR1 = SAI_xCR1_MODE_0 |           /* Master Receiver (génère BCLK/FS). */
                              SAI_xCR1_PRTCFG_0 |        /* Free protocol. */
                              SAI_xCR1_DS_4 | SAI_xCR1_DS_2 | /* 24 bits data size (slot 32 bits). */
                              SAI_xCR1_CKSTR;             /* Données échantillonnées sur front montant. */
    AUDIO_SAI_RX_BLOCK->CR2 = SAI_xCR2_FTH_0;             /* Threshold half FIFO. */
    /* Frame 8 slots de 32 bits => 256 bits. FSALL = 128-1 (FS = 50% duty), FRL = 256-1. */
    AUDIO_SAI_RX_BLOCK->FRCR = ((256U - 1U) << SAI_xFRCR_FRL_Pos) |
                               ((128U - 1U) << SAI_xFRCR_FSALL_Pos) |
                               SAI_xFRCR_FSDEF | SAI_xFRCR_FSOFF;
    /* Slot: 8 slots, 32 bits (SLOTSZ_1). FBOFF=0 => premier bit juste après FS. */
    AUDIO_SAI_RX_BLOCK->SLOTR = (0U << SAI_xSLOTR_FBOFF_Pos) |
                                (SAI_xSLOTR_SLOTSZ_1) |
                                ((AUDIO_NUM_INPUT_CHANNELS - 1U) << SAI_xSLOTR_NBSLOT_Pos) |
                                0x00FFU; /* Slots 0..7 actifs */

    /* Bloc A = esclave TX TDM 4x32 bits, synchronisé sur bloc B. */
    AUDIO_SAI_TX_BLOCK->CR1 = SAI_xCR1_MODE_1 |           /* Slave Transmitter. */
                              SAI_xCR1_PRTCFG_0 |
                              SAI_xCR1_DS_4 | SAI_xCR1_DS_2 |
                              SAI_xCR1_SYNCEN_0;          /* Synchro interne sur bloc B. */
    AUDIO_SAI_TX_BLOCK->CR2 = SAI_xCR2_FTH_0;
    /* Frame 4 slots de 32 bits => 128 bits. FSALL = 64-1, FRL = 128-1. */
    AUDIO_SAI_TX_BLOCK->FRCR = ((128U - 1U) << SAI_xFRCR_FRL_Pos) |
                               ((64U - 1U) << SAI_xFRCR_FSALL_Pos) |
                               SAI_xFRCR_FSDEF | SAI_xFRCR_FSOFF;
    AUDIO_SAI_TX_BLOCK->SLOTR = (0U << SAI_xSLOTR_FBOFF_Pos) |
                                (SAI_xSLOTR_SLOTSZ_1) |
                                ((AUDIO_NUM_OUTPUT_CHANNELS - 1U) << SAI_xSLOTR_NBSLOT_Pos) |
                                0x000FU; /* Slots 0..3 actifs */

    /* Seul le bloc B (maître RX) génère les horloges MCLK/BCLK/FS pour éviter tout double pilotage. */
    AUDIO_SAI_RX_BLOCK->CR1 |= SAI_xCR1_OUTDRIV | SAI_xCR1_NODIV;
    /* Bloc A TX reste un esclave synchronisé : ne pas activer OUTDRIV/NODIV côté TX. */
#endif
}

static void audio_dma_start(void) {
#if STM32_DMA_SUPPORTS_DMAMUX == TRUE
    sai_rx_dma = dmaStreamAlloc(AUDIO_SAI_RX_DMA_STREAM,
                                AUDIO_SAI_RX_DMA_PRIORITY,
                                audio_dma_rx_cb,
                                NULL);
    sai_tx_dma = dmaStreamAlloc(AUDIO_SAI_TX_DMA_STREAM,
                                AUDIO_SAI_TX_DMA_PRIORITY,
                                audio_dma_tx_cb,
                                NULL);

    dmaSetRequestSource(sai_rx_dma, AUDIO_SAI_RX_DMA_REQUEST);
    dmaSetRequestSource(sai_tx_dma, AUDIO_SAI_TX_DMA_REQUEST);

    /* RX : P2M, 32 bits, circulaire, half/full interrupt. */
    uint32_t rx_mode = STM32_DMA_CR_PL(AUDIO_SAI_RX_DMA_PRIORITY) |
                       STM32_DMA_CR_DIR_P2M |
                       STM32_DMA_CR_PSIZE_WORD |
                       STM32_DMA_CR_MSIZE_WORD |
                       STM32_DMA_CR_MINC |
                       STM32_DMA_CR_CIRC |
                       STM32_DMA_CR_HTIE |
                       STM32_DMA_CR_TCIE;

    dmaStreamSetPeripheral(sai_rx_dma, &AUDIO_SAI_RX_BLOCK->DR);
    dmaStreamSetMemory0(sai_rx_dma, audio_in_buffers);
    dmaStreamSetTransactionSize(sai_rx_dma, AUDIO_DMA_IN_SAMPLES);
    dmaStreamSetMode(sai_rx_dma, rx_mode);

    /* TX : M2P, 32 bits, circulaire, half/full interrupt. */
    uint32_t tx_mode = STM32_DMA_CR_PL(AUDIO_SAI_TX_DMA_PRIORITY) |
                       STM32_DMA_CR_DIR_M2P |
                       STM32_DMA_CR_PSIZE_WORD |
                       STM32_DMA_CR_MSIZE_WORD |
                       STM32_DMA_CR_MINC |
                       STM32_DMA_CR_CIRC |
                       STM32_DMA_CR_HTIE |
                       STM32_DMA_CR_TCIE;

    dmaStreamSetPeripheral(sai_tx_dma, &AUDIO_SAI_TX_BLOCK->DR);
    dmaStreamSetMemory0(sai_tx_dma, audio_out_buffers);
    dmaStreamSetTransactionSize(sai_tx_dma, AUDIO_DMA_OUT_SAMPLES);
    dmaStreamSetMode(sai_tx_dma, tx_mode);

    dmaStreamEnable(sai_rx_dma);
    dmaStreamEnable(sai_tx_dma);

    /* Active DMA puis SAI. */
    AUDIO_SAI_RX_BLOCK->CR1 |= SAI_xCR1_DMAEN;
    AUDIO_SAI_TX_BLOCK->CR1 |= SAI_xCR1_DMAEN;
    AUDIO_SAI_RX_BLOCK->CR1 |= SAI_xCR1_SAIEN;
    AUDIO_SAI_TX_BLOCK->CR1 |= SAI_xCR1_SAIEN;
#else
    (void)audio_dma_rx_cb;
    (void)audio_dma_tx_cb;
#endif
}

static void audio_dma_stop(void) {
    if (sai_rx_dma != NULL) {
        dmaStreamDisable(sai_rx_dma);
        dmaStreamFree(sai_rx_dma);
        sai_rx_dma = NULL;
    }
    if (sai_tx_dma != NULL) {
        dmaStreamDisable(sai_tx_dma);
        dmaStreamFree(sai_tx_dma);
        sai_tx_dma = NULL;
    }

    AUDIO_SAI_RX_BLOCK->CR1 &= ~(SAI_xCR1_SAIEN | SAI_xCR1_DMAEN);
    AUDIO_SAI_TX_BLOCK->CR1 &= ~(SAI_xCR1_SAIEN | SAI_xCR1_DMAEN);
}

/* Callbacks DMA : half/full -> signale le thread. */
static void audio_dma_rx_cb(void *p, uint32_t flags) {
    (void)p;
    if ((flags & STM32_DMA_ISR_HTIF) != 0U) {
        audio_in_ready_index = 0U;
    } else if ((flags & STM32_DMA_ISR_TCIF) != 0U) {
        audio_in_ready_index = 1U;
    }
    chBSemSignalI(&audio_dma_sem);
}

static void audio_dma_tx_cb(void *p, uint32_t flags) {
    (void)p;
    if ((flags & STM32_DMA_ISR_HTIF) != 0U) {
        audio_out_ready_index = 0U;
    } else if ((flags & STM32_DMA_ISR_TCIF) != 0U) {
        audio_out_ready_index = 1U;
    }
    chBSemSignalI(&audio_dma_sem);
}
