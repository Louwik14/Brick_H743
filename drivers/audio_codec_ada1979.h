/**
 * @file audio_codec_ada1979.h
 * @brief Driver I2C pour la configuration des ADC ADAU1979 en mode TDM 8 canaux.
 */

#ifndef AUDIO_CODEC_ADA1979_H
#define AUDIO_CODEC_ADA1979_H

#include "ch.h"
#include "hal.h"
#include "audio_conf.h"

/**
 * @brief Initialise l'interface de contrôle I2C et les deux codecs ADAU1979.
 */
void adau1979_init(void);

/**
 * @brief Configuration par défaut : TDM 8 canaux, 24 bits, 48 kHz, codecs esclaves.
 */
void adau1979_set_default_config(void);

/**
 * @brief Met les deux codecs en mute numérique (ou réactive).
 */
void adau1979_mute(bool en);

#endif /* AUDIO_CODEC_ADA1979_H */
