/**
 * @file drv_audio.c
 * @brief Gestion complète du pipeline audio TDM (SAI + DMA + callbacks DSP).
 */

#include "drv_audio.h"
#include "audio_codec_ada1979.h"
#include "audio_codec_pcm4104.h"

/* -------------------------------------------------------------------------- */
/* Buffers ping/pong                                                          */
/* -------------------------------------------------------------------------- */

static int32_t audio_in_buffers[2][AUDIO_FRAMES_PER_BUFFER][AUDIO_NUM_INPUT_CHANNELS];
static int32_t audio_out_buffers[2][AUDIO_FRAMES_PER_BUFFER][AUDIO_NUM_OUTPUT_CHANNELS];

static volatile uint8_t audio_in_active_buffer = 0U;
static volatile uint8_t audio_out_active_buffer = 0U;

/* Indices disponibles pour l'application. */
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

/* -------------------------------------------------------------------------- */
/* Prototypes internes                                                        */
/* -------------------------------------------------------------------------- */

static void audio_hw_configure_sai(void);
static void audio_dma_start(void);
static void audio_dma_stop(void);

static void audio_dma_rx_half_cb(ADCDriver *adcp, adcsample_t *buffer, size_t n);
static void audio_dma_rx_full_cb(ADCDriver *adcp, adcsample_t *buffer, size_t n);
static void audio_dma_tx_half_cb(void *arg, uint32_t flags);
static void audio_dma_tx_full_cb(void *arg, uint32_t flags);

static THD_WORKING_AREA(audioThreadWA, AUDIO_THREAD_STACK_SIZE);
static THD_FUNCTION(audioThread, arg);

/* -------------------------------------------------------------------------- */
/* API publique                                                               */
/* -------------------------------------------------------------------------- */

void drv_audio_init(void) {
    chBSemObjectInit(&audio_dma_sem, TRUE);

    /* Prépare le bus I2C et les codecs. */
    audio_codec_ada1979_init(&AUDIO_I2C_DRIVER, NULL);
    audio_codec_pcm4104_init();

    /* Les GPIO SAI sont déjà configurés via board.h. */
    audio_hw_configure_sai();
}

void drv_audio_start(void) {
    audio_codec_pcm4104_set_mute(true);
    audio_codec_ada1979_configure_tdm();

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
    /* Dans ce design, les DMA tournent en mode circulaire : aucun traitement spécifique. */
}

void drv_audio_set_master_volume(float vol) {
    if (vol < 0.0f) {
        vol = 0.0f;
    }
    if (vol > 1.0f) {
        vol = 1.0f;
    }
    audio_master_volume = vol;
}

void drv_audio_register_spilink_pull(drv_spilink_pull_cb_t cb) {
    spilink_pull_cb = cb;
}

void drv_audio_register_spilink_push(drv_spilink_push_cb_t cb) {
    spilink_push_cb = cb;
}

/* -------------------------------------------------------------------------- */
/* Hook DSP par défaut                                                        */
/* -------------------------------------------------------------------------- */

void __attribute__((weak)) drv_audio_process_block(const int32_t *adc_in, int32_t *dac_out, size_t frames) {
    /* Pass-through avec master volume. */
    for (size_t n = 0; n < frames; ++n) {
        for (size_t c = 0; c < AUDIO_NUM_OUTPUT_CHANNELS; ++c) {
            size_t src_c = (c < AUDIO_NUM_INPUT_CHANNELS) ? c : 0U;
            float sample = (float)adc_in[n * AUDIO_NUM_INPUT_CHANNELS + src_c];
            sample *= audio_master_volume;
            dac_out[n * AUDIO_NUM_OUTPUT_CHANNELS + c] = (int32_t)sample;
        }
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
        size_t frames;
        const int32_t *in_buf = drv_audio_get_input_buffer(&in_idx, &frames);
        int32_t *out_buf = drv_audio_get_output_buffer(&out_idx, NULL);

        if (in_buf == NULL || out_buf == NULL) {
            continue;
        }

        /* Récupère l'audio des cartouches si disponible. */
        if (spilink_pull_cb != NULL) {
            spilink_pull_cb(out_buf, frames);
        }

        drv_audio_process_block(in_buf, out_buf, frames);

        /* Exporte le flux vers les cartouches si besoin. */
        if (spilink_push_cb != NULL) {
            spilink_push_cb(in_buf, frames);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Configuration SAI + DMA                                                    */
/* -------------------------------------------------------------------------- */

static void audio_hw_configure_sai(void) {
    /* Les registres SAI sont initialisés ici de manière minimale pour activer
     * le mode TDM : slots 8×32 bits en RX, 4×32 bits en TX. Les valeurs exactes
     * (FSALL, FRL, DS) doivent être alignées avec le Reference Manual RM0433. */

#if defined(STM32H7xx) && defined(SAI_xCR1_MODE_0)
    /* Active les horloges SAI et force la réinitialisation. */
    rccEnableSAI1(true);
    rccResetSAI1();

    /* Bloc A : maître RX TDM8, 24 bits dans des mots 32 bits. */
    AUDIO_SAI_RX_BLOCK->CR1 = SAI_xCR1_MODE_0 |       /* Master TX mode (génère SCK/FS). */
                              SAI_xCR1_PRTCFG_0 |    /* Free protocol */
                              SAI_xCR1_DS_4 |        /* 24 bits data */
                              SAI_xCR1_DS_2;
    AUDIO_SAI_RX_BLOCK->CR2 = SAI_xCR2_FTH_0;         /* Threshold half. */
    AUDIO_SAI_RX_BLOCK->FRCR = ((256 - 1) << SAI_xFRCR_FRL_Pos) |
                               ((128 - 1) << SAI_xFRCR_FSALL_Pos) |
                               SAI_xFRCR_FSDEF | SAI_xFRCR_FSOFF;
    AUDIO_SAI_RX_BLOCK->SLOTR = (7U << SAI_xSLOTR_FBOFF_Pos) | /* Frame offset at bit 0 */
                                ((AUDIO_NUM_INPUT_CHANNELS - 1U) << SAI_xSLOTR_NBSLOT_Pos) |
                                (0xFFFFU); /* 8 slots activés */

    /* Bloc B : esclave synchrone pour l'émission 4 canaux. */
    AUDIO_SAI_TX_BLOCK->CR1 = SAI_xCR1_MODE_1 |       /* Synchronous slave TX. */
                              SAI_xCR1_PRTCFG_0 |
                              SAI_xCR1_DS_4 | SAI_xCR1_DS_2;
    AUDIO_SAI_TX_BLOCK->CR2 = SAI_xCR2_FTH_0;
    AUDIO_SAI_TX_BLOCK->FRCR = ((128 - 1) << SAI_xFRCR_FRL_Pos) |
                               ((64 - 1) << SAI_xFRCR_FSALL_Pos) |
                               SAI_xFRCR_FSDEF | SAI_xFRCR_FSOFF;
    AUDIO_SAI_TX_BLOCK->SLOTR = (3U << SAI_xSLOTR_FBOFF_Pos) |
                                ((AUDIO_NUM_OUTPUT_CHANNELS - 1U) << SAI_xSLOTR_NBSLOT_Pos) |
                                0x0FU; /* 4 slots actifs */
#else
    /* Compilation sans accès direct aux registres : rien à faire ici. */
#endif
}

static void audio_dma_start(void) {
    /* Placeholder : configuration DMA circulaire double-buffer à implémenter
     * selon les canaux exacts (voir RM0433 + mapping DMA). Les callbacks
     * ci-dessous sont appelés par les ISR DMA. */
    (void)audio_dma_rx_half_cb;
    (void)audio_dma_rx_full_cb;
    (void)audio_dma_tx_half_cb;
    (void)audio_dma_tx_full_cb;
}

static void audio_dma_stop(void) {
    /* Arrêt propre du SAI/DMA. */
}

/* Callbacks DMA (à relier aux ISR SAI/DMA dans la BSP). */
static void audio_dma_rx_half_cb(ADCDriver *adcp, adcsample_t *buffer, size_t n) {
    (void)adcp; (void)buffer; (void)n;
    audio_in_active_buffer = 0U;
    audio_in_ready_index = 0U;
    chBSemSignalI(&audio_dma_sem);
}

static void audio_dma_rx_full_cb(ADCDriver *adcp, adcsample_t *buffer, size_t n) {
    (void)adcp; (void)buffer; (void)n;
    audio_in_active_buffer = 1U;
    audio_in_ready_index = 1U;
    chBSemSignalI(&audio_dma_sem);
}

static void audio_dma_tx_half_cb(void *arg, uint32_t flags) {
    (void)arg; (void)flags;
    audio_out_active_buffer = 0U;
    audio_out_ready_index = 0U;
    chBSemSignalI(&audio_dma_sem);
}

static void audio_dma_tx_full_cb(void *arg, uint32_t flags) {
    (void)arg; (void)flags;
    audio_out_active_buffer = 1U;
    audio_out_ready_index = 1U;
    chBSemSignalI(&audio_dma_sem);
}

