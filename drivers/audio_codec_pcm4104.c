/**
 * @file audio_codec_pcm4104.c
 * @brief Driver minimal pour le DAC PCM4104 (mode matériel, aucune interface série).
 */

#include "audio_codec_pcm4104.h"

/* La PCM4104 est câblée en mode autonome : aucune configuration série n'est requise.
 * Ce fichier gère simplement le mute matériel (TPA6138A2) et prépare les lignes SAI. */

/* GPIO à ajuster selon le schéma (broche MUTE de l'ampli casque). */
#ifndef AUDIO_HP_MUTE_LINE
#define AUDIO_HP_MUTE_LINE    LINE_GPIOB_LED2  /* Placeholder : à ajuster via board.h */
#endif

void audio_codec_pcm4104_init(void) {
    /* Configure la ligne de mute en sortie, état actif haut. */
    palSetLineMode(AUDIO_HP_MUTE_LINE, PAL_MODE_OUTPUT_PUSHPULL);
    palClearLine(AUDIO_HP_MUTE_LINE); /* Mute par défaut au boot. */
}

void audio_codec_pcm4104_set_mute(bool mute) {
    if (mute) {
        palClearLine(AUDIO_HP_MUTE_LINE);
    } else {
        palSetLine(AUDIO_HP_MUTE_LINE);
    }
}

