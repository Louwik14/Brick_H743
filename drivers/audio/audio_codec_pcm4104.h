/**
 * @file audio_codec_pcm4104.h
 * @brief Gestion bas niveau du DAC PCM4104 (TDM 4 canaux, mode autonome).
 */

#ifndef AUDIO_CODEC_PCM4104_H
#define AUDIO_CODEC_PCM4104_H

#include "ch.h"
#include "hal.h"
#include "audio_conf.h"

/**
 * @brief Initialise les GPIO associés (mute casque/ligne) et prépare la sortie TDM.
 */
void audio_codec_pcm4104_init(void);

/**
 * @brief Force le mute matériel (broche MUTE TPA6138A2 ou similaire).
 */
void audio_codec_pcm4104_set_mute(bool mute);

#endif /* AUDIO_CODEC_PCM4104_H */
