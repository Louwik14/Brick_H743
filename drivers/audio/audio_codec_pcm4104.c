/**
 * @file audio_codec_pcm4104.c
 * @brief Driver minimal pour le DAC PCM4104 (mode matériel, aucune interface série).
 *
 * Le PCM4104 est configuré en mode autonome (pins hardware).
 * Aucun contrôle numérique n'est requis côté MCU.
 * Le volume et le mute sont gérés 100 % en software dans le moteur audio.
 */

#include "audio_codec_pcm4104.h"

/* Aucun MUTE matériel utilisé dans ce projet.
 * Le mute est géré uniquement en software via le volume global.
 */
#ifndef AUDIO_HP_MUTE_LINE
#define AUDIO_HP_MUTE_LINE    PAL_NOLINE
#endif

void audio_codec_pcm4104_init(void) {
#if (AUDIO_HP_MUTE_LINE != PAL_NOLINE)
    /* Si un jour une broche MUTE est ajoutée en hardware,
       elle sera configurée ici automatiquement. */
    palSetLineMode(AUDIO_HP_MUTE_LINE, PAL_MODE_OUTPUT_PUSHPULL);
    palClearLine(AUDIO_HP_MUTE_LINE); /* Mute par défaut au boot. */
#endif
}

void audio_codec_pcm4104_set_mute(bool mute) {
#if (AUDIO_HP_MUTE_LINE != PAL_NOLINE)
    if (mute) {
        palClearLine(AUDIO_HP_MUTE_LINE);
    } else {
        palSetLine(AUDIO_HP_MUTE_LINE);
    }
#else
    /* Mute purement logiciel : aucune action hardware */
    (void)mute;
#endif
}
