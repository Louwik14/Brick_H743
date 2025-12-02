/**
 * @file audio_conf.h
 * @brief Configuration centrale du sous-système audio (TDM SAI + buffers).
 */

#ifndef AUDIO_CONF_H
#define AUDIO_CONF_H

#include "ch.h"
#include "hal.h"

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
 * @brief Canal SAI utilisé pour la capture TDM 8 canaux (bloc A).
 * @note Le routage exact (SD, FS, SCK, MCLK) est défini dans docs/board.h.
 */
#define AUDIO_SAI_RX                  SAI1
#define AUDIO_SAI_RX_BLOCK            SAI1_Block_A

/**
 * @brief Canal SAI utilisé pour l'émission TDM 4 canaux (bloc B synchro maître).
 */
#define AUDIO_SAI_TX                  SAI1
#define AUDIO_SAI_TX_BLOCK            SAI1_Block_B

/** Broches SAI issues du fichier board.h (mode Alternate 6). */
#define AUDIO_LINE_SAI_MCLK           LINE_SAI1_MCLK_A
#define AUDIO_LINE_SAI_FS             LINE_SAI1_FS_A
#define AUDIO_LINE_SAI_SCK            LINE_SAI1_SCK_A
#define AUDIO_LINE_SAI_SD_TX          LINE_SAI1_SD_B

/* -------------------------------------------------------------------------- */
/* I2C et périphériques externes                                              */
/* -------------------------------------------------------------------------- */

/** Bus I2C dédié aux codecs ADAU1979 (voir board.h pour le câblage exact). */
#define AUDIO_I2C_DRIVER              I2CD3

/** Adresse 7 bits par défaut des ADAU1979 (cf. datasheet, section TWI Address). */
#define ADAU1979_I2C_ADDRESS          0x11U

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

#endif /* AUDIO_CONF_H */
