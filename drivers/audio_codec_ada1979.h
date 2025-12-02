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
 * @brief Initialise l'interface de contrôle I2C des ADAU1979.
 *
 * @param[in] i2cp     Pointeur vers le driver I2C configuré (horloge, pins).
 * @param[in] i2cfg    Configuration du bus (peut être NULL pour utiliser la config par défaut).
 */
void audio_codec_ada1979_init(I2CDriver *i2cp, const I2CConfig *i2cfg);

/**
 * @brief Place les deux ADAU1979 en mode TDM 8 canaux 24 bits / 48 kHz.
 *
 * La fonction configure l'horloge, active la PLL interne synchronisée sur MCLK,
 * positionne les slots TDM et active les entrées.
 *
 * @return HAL status du dernier échange I2C (HAL_RET_SUCCESS en cas de succès).
 */
msg_t audio_codec_ada1979_configure_tdm(void);

/**
 * @brief Ajuste le volume numérique global des ADC (atténuateur intégré).
 *
 * @param[in] volume_db Atténuation en dB (0 dB = unity). Les valeurs sont
 *                      limitées aux bornes supportées par le codec.
 */
msg_t audio_codec_ada1979_set_volume(float volume_db);

/**
 * @brief Met les ADAU1979 en mute ou les réactive.
 */
msg_t audio_codec_ada1979_set_mute(bool mute);

#endif /* AUDIO_CODEC_ADA1979_H */
