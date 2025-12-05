/**
 * @file audio_conf.h
 * @brief Configuration centrale du sous-système audio (TDM SAI + buffers).
 */

#ifndef AUDIO_CONF_H
#define AUDIO_CONF_H

#include "ch.h"
#include "hal.h"

/*
 * Attribut pour placer les buffers DMA audio en RAM non cacheable (.ram_d2).
 * Section LD délimitée par __ram_d2_start__/__ram_d2_end__ et configurée en
 * région MPU non-cacheable par mpu_config_init_once().
 */
#define AUDIO_DMA_BUFFER_ATTR __attribute__((section(".ram_d2"), aligned(32)))

/* -------------------------------------------------------------------------- */
/* Paramètres généraux du flux audio                                          */
/* -------------------------------------------------------------------------- */

/** Fréquence d'échantillonnage commune à tous les périphériques. */
#define AUDIO_SAMPLE_RATE_HZ          48000U

/** Taille d'un bloc audio (nombre d'échantillons par canal). */
#define AUDIO_FRAMES_PER_BUFFER       16U

/** Nombre de canaux TDM en entrée (2× ADAU1979 = 8). */
#define AUDIO_NUM_INPUT_CHANNELS      8U

/** Nombre de canaux TDM en sortie (PCM4104 = 4). */
#define AUDIO_NUM_OUTPUT_CHANNELS     4U

/** Résolution logique des échantillons (24 bits significatifs dans int32_t). */
#define AUDIO_SAMPLE_BITS             24U

/**
 * @brief SAI1 Bloc B = ADC (RX maître TDM 8 canaux).
 */
#define AUDIO_SAI_RX                  SAI1
#define AUDIO_SAI_RX_BLOCK            SAI1_Block_B

/**
 * @brief SAI1 Bloc A = DAC (TX esclave TDM 4 canaux synchronisé sur le bloc B).
 */
#define AUDIO_SAI_TX                  SAI1
#define AUDIO_SAI_TX_BLOCK            SAI1_Block_A

/** Broches SAI issues du fichier board.h (mode Alternate 6). */
#define AUDIO_LINE_SAI_MCLK           LINE_SAI1_MCLK_A
#define AUDIO_LINE_SAI_FS             LINE_SAI1_FS_A
#define AUDIO_LINE_SAI_SCK            LINE_SAI1_SCK_A
#define AUDIO_LINE_SAI_SD_RX          LINE_SAI1_SD_B
#define AUDIO_LINE_SAI_SD_TX          LINE_SAI1_SD_A

/* -------------------------------------------------------------------------- */
/* I2C et périphériques externes                                              */
/* -------------------------------------------------------------------------- */

/** Bus I2C dédié aux codecs ADAU1979 (voir board.h pour le câblage exact). */
#define AUDIO_I2C_DRIVER              I2CD3

/** Adresses 7 bits des deux ADAU1979 (config strap ADDR0/ADDR1). */
#define ADAU1979_I2C_ADDRESS_0        0x11U
#define ADAU1979_I2C_ADDRESS_1        0x12U

/* -------------------------------------------------------------------------- */
/* DMA (DMAMUX)                                                               */
/* -------------------------------------------------------------------------- */

/* Streams DMA à ajuster selon le schéma exact (DMA1/2 + stream). */
#define AUDIO_SAI_RX_DMA_STREAM       STM32_DMA_STREAM_ID(1, 0)
#define AUDIO_SAI_TX_DMA_STREAM       STM32_DMA_STREAM_ID(1, 1)

/* Requests DMAMUX : valeurs issues du RM0433, section DMAMUX. */
#define AUDIO_SAI_RX_DMA_REQUEST      87U  /* SAI1_B */
#define AUDIO_SAI_TX_DMA_REQUEST      86U  /* SAI1_A */

/* Priorité DMA (0 = basse, 3 = très haute). */
#define AUDIO_SAI_RX_DMA_PRIORITY     2U
#define AUDIO_SAI_TX_DMA_PRIORITY     2U

/* -------------------------------------------------------------------------- */
/* Options de pile/threads                                                    */
/* -------------------------------------------------------------------------- */

/** Taille de pile du thread audio temps réel. */
#define AUDIO_THREAD_STACK_SIZE       THD_WORKING_AREA_SIZE(2048)

/** Priorité du thread audio : haut, juste sous le kernel. */
#define AUDIO_THREAD_PRIORITY         (HIGHPRIO - 1)

/* -------------------------------------------------------------------------- */
/* Types utilitaires                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Type de buffer audio interne : tableau [frames][channels].
 */
typedef int32_t audio_buffer_t[AUDIO_FRAMES_PER_BUFFER][AUDIO_NUM_INPUT_CHANNELS];

/**
 * @brief Buffer de sortie TDM 4 canaux.
 */
typedef int32_t audio_out_buffer_t[AUDIO_FRAMES_PER_BUFFER][AUDIO_NUM_OUTPUT_CHANNELS];

/**
 * @brief Bloc audio SPI-LINK : 4 cartouches, 4 canaux chacune.
 */
typedef int32_t spilink_audio_block_t[4][AUDIO_FRAMES_PER_BUFFER][4];

#endif /* AUDIO_CONF_H */
