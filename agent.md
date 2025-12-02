### 🧠 PROMPT POUR CODEX

Tu es **Codex**, un assistant développeur embarqué expert en :

* STM32H7 (Cortex-M7 + FPU)
* Temps réel dur avec **ChibiOS RT**
* DMA, SAI (TDM), SPI full-duplex
* Audio numérique multicanal, drivers de codecs audio
* Architecture dengines audio “block-based” (type DAW / groovebox / modular / Mutable Instruments)

Ta mission est de produire un **driver audio bas niveau complet et “prod-ready”** pour un projet de groovebox modulaire, qui :

* Capture de l’audio :

  * depuis l’extérieur via **2× ADC ADAU1979** en TDM (8 canaux)
  * depuis des **cartouches audio externes (STM32F429 type Ksoloti)** via un **lien SPI-LINK V2** (trames audio full-duplex)
* Permet de **router/mixer ces signaux** (infrastructure prête pour du mix + FX, mais sans implémenter les FX avancés)
* Renvoie de l’audio :

  * vers les cartouches via SPI-LINK (retour audio)
  * vers l’extérieur via **DAC PCM4104** (sortie ligne + casque)

Le résultat doit ressembler à la base d’une **table de mixage / Octatrack** : une architecture claire de transport audio, avec points d’insertion où on pourra plus tard plugger des effets (par ex. issus du code open source **Mutable Instruments** type Clouds, etc.).

---

## 1. CONTEXTE MATÉRIEL

### 1.1 Unité centrale (Master)

* MCU principal : **STM32H743** (Cortex-M7, FPU)
* OS : **ChibiOS RT** (dernière version)
* Le STM32H7 est :

  * **Maître audio** (génère MCLK/BCLK/LRCLK)
  * **Maître SPI** de toutes les cartouches

### 1.2 Audio matériel

**ADCs – 2 × ADAU1979 (Analog Devices)**

* Chaque ADAU1979 : 4 canaux ADC
* Les deux sont chaînés pour produire un flux **TDM commun 8 canaux**
* Format : **TDM, 24 bits, 48 kHz**
* 1 seule ligne SDATA (TDM 8 canaux) vers le STM32
* Contrôle via I2C (init, mode TDM, volume/mute numérique, etc.)
* Le H7 doit être **master clock** (MCLK/BCLK/LRCLK fournis par le SAI du H7)

**DAC – PCM4104 (TI)**

* 4 canaux DAC 24 bits, TDM/I2S
* Utilisé en mode **standalone / auto** (pas de SPI)
* Configuré en hardware (pins MODE/FMT/FS) pour :

  * accepter un flux TDM à **48 kHz, 24 bits**
  * 4 canaux, dont :

    * 2 pour la sortie ligne
    * 2 pour la sortie casque (via ampli)

**Ampli casque – TPA6138A2 (TI)**

* Driver casque analogique relié au PCM4104
* Juste une broche MUTE via GPIO éventuel
* Aucun flux numérique à gérer

**STM32H743 – SAI & DMA**

* Utilisation de **2 blocs SAI** :

  * `SAIx_A` : **RX TDM 8 canaux**, relié au flux commun des 2×ADAU1979
  * `SAIy_B` : **TX TDM 4 canaux**, relié au PCM4104
* Le STM32 est **MAÎTRE** pour MCLK/BCLK/LRCLK
* Utilisation de **DMA double-buffer (ping/pong)** pour RX et TX
* Format interne : **int32_t**, 24 bits significatifs dans les bits de poids fort

---

## 2. CONTEXTE SPI-LINK AUDIO (CARTOUCHES)

Il existe jusqu’à **4 cartouches audio externes** (STM32F429 type Ksoloti), reliées par **SPI** séparés, une cartouche par bus SPI.

Chaque lien SPI est basé sur une **TRAME V2** full-duplex **fixe en taille**, transmise en continu via DMA :

```
TRAME V2 :

HEADER          (mot de sync, ex : 0xABCD1234)
FRAME_COUNTER   (incrémental)
AUDIO_BLOCK     (4 canaux, 16 samples/channel, int32_t)
EVENT_COUNT     (nombre d’events)
EVENTS_AREA     (zone fixe, events sérialisés)
FOOTER          (mot de fin, ex : 0xDEADBEEF)
```

Seule la partie **AUDIO_BLOCK** est pertinente pour ce prompt (transport d’audio).
Les events SPI (NOTE_ON, NOTE_OFF, PARAM_SET, etc.) existent, mais **tu n’as pas à implémenter tout le séquenceur ici**. Tu dois uniquement :

* Fournir une **API propre pour pousser/récupérer des blocs audio** de/vers chaque cartouche
* Gérer la partie **AUDIO_BLOCK** des trames côté H7 (buffers int32_t[16][4])

**Important :**
Pour rester cohérent avec SPI-LINK, on considère comme **taille de bloc audio de base** :
`AUDIO_FRAMES_PER_BUFFER = 16` (c’est-à-dire 16 samples par bloc pour toutes les sources / sinks).

---

## 3. OBJECTIF DU CODE À PRODUIRE

Tu dois produire un **ensemble de drivers et d’APIs C** (C99) pour le STM32H743 + ChibiOS qui gèrent :

1. **Transport audio ADC (ADAU1979) → H7**

   * SAI RX en TDM 8 canaux, 24 bits dans des mots 32 bits
   * DMA double buffer (ping/pong)
   * Buffer interne statique :

     * `int32_t audio_in_buffers[2][AUDIO_FRAMES_PER_BUFFER][AUDIO_NUM_INPUT_CHANNELS];`
   * `AUDIO_NUM_INPUT_CHANNELS = 8`

2. **Transport audio H7 → DAC (PCM4104)**

   * SAI TX en TDM 4 canaux, 24 bits dans des mots 32 bits
   * DMA double buffer (ping/pong)
   * Buffer interne statique :

     * `int32_t audio_out_buffers[2][AUDIO_FRAMES_PER_BUFFER][AUDIO_NUM_OUTPUT_CHANNELS];`
   * `AUDIO_NUM_OUTPUT_CHANNELS = 4`

3. **Points d’intégration pour SPI-LINK audio (cartouches)**

   * API claire pour **échanger des blocs audio** int32_t[16][4] vers/depuis chaque cartouche
   * Ces blocs pourront ensuite être mixés avec les ADC et renvoyés vers :

     * le DAC (vers l’extérieur)
     * les cartouches (retours audio)
   * Ne PAS implémenter la totalité du protocole SPI-LINK (events, header/footer, etc.), mais prévoir :

     * Une structure de buffer pour AUDIO_BLOCK
     * Des fonctions type :

       * `bool spi_audio_get_input_block(uint8_t cart_id, const int32_t **buf, size_t *frames, size_t *channels);`
       * `bool spi_audio_get_output_block(uint8_t cart_id, int32_t **buf, size_t *frames, size_t *channels);`
     * Le détail exact des fonctions est à concevoir, mais elles doivent s’intégrer naturellement à l’API audio globale.

4. **Thread audio de traitement (block-based)**

   * Un thread ChibiOS dédié, par exemple `audioProcessThread`, qui :

     * Attend des événements (semaphores / flags) indiquant qu’un **half-buffer** ou **full-buffer** est prêt (RX + TX)

     * Récupère des pointeurs vers :

       * bloc ADC (in)
       * bloc DAC (out)
       * blocs SPI des cartouches (in/out)

     * Appelle une fonction de traitement haut niveau :

       ```c
       void drv_audio_process_block(
           const int32_t *adc_in,      /* [frames][AUDIO_NUM_INPUT_CHANNELS]    */
           const int32_t *spi_ins[],   /* tableau de pointeurs par cartouche    */
           int32_t       *dac_out,     /* [frames][AUDIO_NUM_OUTPUT_CHANNELS]   */
           int32_t       *spi_outs[],  /* tableau de pointeurs par cartouche    */
           size_t         frames);
       ```

     * Puis marque les buffers comme “libérés / traités”.

   * **Important :** tu peux simplifier la signature si nécessaire, mais garde l’idée :

     * Un bloc ADC
     * Un bloc DAC
     * Zéro à N blocs SPI (cartouches) en entrée/sortie

5. **Implémentation par défaut du traitement**

   * Par défaut, ne fais que :

     * copier les 2 premiers canaux ADC vers les 2 premiers canaux DAC (thru)
     * copier éventuellement un canal d’une cartouche vers un canal DAC (ex : simple mix 50/50)

   * Appliquer un **volume master software** simple :

     ```c
     void drv_audio_set_master_volume(float vol); // clamp 0.0 .. 1.0
     ```

   * **Aucun effet complexe** (pas de reverb, granular, etc.)
     → l’architecture doit cependant être prête à recevoir plus tard du DSP lourd (Mutable Instruments, etc.) dans `drv_audio_process_block`.

---

## 4. CONTRAINTES GÉNÉRALES

### 4.1 Style & langage

* **C99 uniquement**
* **AUCUN malloc/calloc/free**, aucune allocation dynamique
* Tout en **statique** ou sur la stack
* Pas de C++, pas de STL, pas de new/delete
* Pas de dépendance autre que :

  * ChibiOS RT
  * HAL/LL STM32H7 (SAI, DMA, I2C, SPI, GPIO)
* Code **prod-ready**, pas de pseudo-code, pas de “TODO: implement” sur les chemins critiques

### 4.2 Structure des buffers

* `AUDIO_SAMPLE_RATE = 48000`
* `AUDIO_BITS_PER_SAMPLE = 24`
* `AUDIO_NUM_INPUT_CHANNELS = 8`  (ADCs)
* `AUDIO_NUM_OUTPUT_CHANNELS = 4` (DAC)
* `AUDIO_FRAMES_PER_BUFFER = 16`  (pour matcher un AUDIO_BLOCK SPI-LINK)
* Format interne des samples : `int32_t`, 24 bits utiles dans les bits de poids fort (signés)

### 4.3 Temps réel & IRQ

* Les **IRQ DMA SAI / SPI** ne font :

  * que la gestion des ping/pong
  * la mise à jour d’index de buffers
  * la signalisation d’un événement à un thread (semaphore, eventflags, etc.)
* **AUCUN DSP lourd dans les IRQ**
* Tout le traitement audio (mix, FX, routage) doit être fait dans **le thread audio**.

### 4.4 Gestion des erreurs

* Vérifier les retours d’init (SAI, DMA, I2C pour ADAU1979, etc.)
* Si erreur critique : assert ou retour d’erreur explicite
* Ne jamais “fail silently”

---

## 5. ARCHITECTURE DE FICHIERS À PRODUIRE

Tu dois produire les fichiers **COMPLETS** suivants (header + source) :

1. `audio_conf.h`

   * Définitions de configuration globale :

     ```c
     #define AUDIO_SAMPLE_RATE          48000
     #define AUDIO_BITS_PER_SAMPLE      24
     #define AUDIO_NUM_INPUT_CHANNELS   8
     #define AUDIO_NUM_OUTPUT_CHANNELS  4
     #define AUDIO_FRAMES_PER_BUFFER    16
     ```

   * Mapping matériel :

     * Quel SAI en RX (pour ADAU1979)
     * Quel SAI en TX (pour PCM4104)
     * Configuration TDM (slots, word size, etc.)
     * Périphériques DMA utilisés
     * Pins MCLK, BCLK, LRCLK, SD_IN, SD_OUT

   * Adresses I2C des deux ADAU1979

2. `audio_codec_ada1979.h` / `audio_codec_ada1979.c`

   * API minimale :

     ```c
     void adau1979_init(void);
     void adau1979_set_default_config(void);
     void adau1979_mute(bool en);
     ```

   * Implémentation de l’I2C et des registres pour :

     * mode TDM, 8 canaux, 24 bits, 48 kHz
     * configuration en esclave (horloges fournies par H7)
     * gestion mute/volume numérique de base

3. `audio_codec_pcm4104.h` / `audio_codec_pcm4104.c`

   * API minimale :

     ```c
     void pcm4104_init(void);
     void pcm4104_mute(bool en);
     ```

   * Documentation en commentaires des pins MODE/FMT/FSx et mode standalone

   * Pilotage éventuel de la broche MUTE via GPIO

4. `drv_audio.h` / `drv_audio.c`  (**driver audio principal SAI + DMA + thread**)

   * Buffers internes statiques :

     ```c
     static int32_t audio_in_buffers[2][AUDIO_FRAMES_PER_BUFFER][AUDIO_NUM_INPUT_CHANNELS];
     static int32_t audio_out_buffers[2][AUDIO_FRAMES_PER_BUFFER][AUDIO_NUM_OUTPUT_CHANNELS];
     ```

   * Index/flags :

     ```c
     static volatile uint8_t audio_in_active_buffer;
     static volatile uint8_t audio_out_active_buffer;
     ```

   * API publique :

     ```c
     void drv_audio_init(void);
     void drv_audio_start(void);
     void drv_audio_stop(void);

     const int32_t* drv_audio_get_input_buffer(uint8_t *index, size_t *frames);
     int32_t*       drv_audio_get_output_buffer(uint8_t *index, size_t *frames);
     void           drv_audio_release_buffers(uint8_t in_index, uint8_t out_index);

     void drv_audio_set_master_volume(float vol);

     /* Hook de traitement – version par défaut fournie, l’utilisateur pourra la surcharger */
     void drv_audio_process_block(
         const int32_t *adc_in,      /* [frames][AUDIO_NUM_INPUT_CHANNELS]   */
         int32_t       *dac_out,     /* [frames][AUDIO_NUM_OUTPUT_CHANNELS]  */
         size_t         frames);
     ```

   * Thread audio :

     * Attente d’événements half/full buffer ready RX/TX
     * Appel à `drv_audio_process_block`
     * Gestion propre du ping/pong

   * Intégration SPI-LINK :

     * Prévoir (au minimum) des **stubs/API de base** pour que plus tard, un module `drv_spilink.c` puisse :

       * fournir des blocs audio provenant des cartouches
       * récupérer des blocs audio à renvoyer aux cartouches
     * Tu peux par exemple prévoir des fonctions faibles (weak) ou des callbacks que `drv_spilink` pourra enregistrer.

5. `recap_audio.txt`

   * Description du pipeline audio :

     ```
     ADAU1979 (TDM 8ch) → SAI RX → audio_in_buffers → audioProcessThread
                         ↘ SPI-LINK audio_in (cartouches)
     audioProcessThread → mix/FX (future) → audio_out_buffers → SAI TX → PCM4104 → TPA6138A2 (casque/ligne)
                         ↘ SPI-LINK audio_out (cartouches)
     ```

   * Format exact des buffers (ordre des canaux, 24 bits dans int32_t)

   * Latence théorique en fonction de AUDIO_FRAMES_PER_BUFFER = 16

   * Endroit où plugger le DSP temps réel (dans `drv_audio_process_block`)

   * Résumé du rôle de chaque fichier

   * Liste de points à tuner plus tard :

     * taille des buffers
     * organisation du mix
     * gestion plus fine des volumes/pan
     * ajout d’effets (Mutable Instruments, etc.)

---

## 6. RÈGLES FINALES POUR LA SORTIE

* **Pas de pseudo-code** : tous les fichiers `.c/.h` doivent être **compilables** (même si certaines parties hardware sont `#ifdef`ées ou légères).
* Commente clairement les sections critiques :

  * config TDM SAI (slots, bit clock, etc.)
  * mapping canaux ADC → indices de buffer
  * mapping canaux DAC → indices de buffer
* Tu peux utiliser des noms de périphériques SAI/DMA génériques (SAI1, SAI2, etc.), mais le code doit être structuré de façon réaliste pour STM32H743 + ChibiOS.
* Si des détails hardware précis manquent (pins exactes, numéros de DMA stream), utilise des `#define` dans `audio_conf.h` avec des noms explicites, et commente-les comme des points à ajuster.

**Maintenant, génère tous les fichiers demandés :**

* `audio_conf.h`
* `audio_codec_ada1979.h`
* `audio_codec_ada1979.c`
* `audio_codec_pcm4104.h`
* `audio_codec_pcm4104.c`
* `drv_audio.h`
* `drv_audio.c`
* `recap_audio.txt`

Avec du code C99 complet et cohérent, sans malloc, prêt à être adapté sur STM32H743 + ChibiOS.

---

### 📚 Références matérielles et logicielles disponibles dans le dépôt

Le dossier `/docs` du dépôt contient **toutes les références officielles nécessaires** à la configuration correcte du matériel et des drivers bas niveau. Tu dois t’y référer systématiquement au lieu de faire des suppositions. Ce dossier inclut notamment :

* Les **datasheets complètes des codecs audio** (ADAU1979, PCM4104, TPA6138A2), à utiliser comme **source de vérité** pour toute configuration de registres, de PLL, de formats TDM/I²S, de séquences d’initialisation et de timings.
* Les **fichiers de configuration de la carte** (`board.ch` et `board.mk`) correspondant exactement au matériel réel.
* Une **copie locale complète de la version de ChibiOS utilisée dans le projet**, fournie uniquement **à titre de référence** pour l’API, les structures internes et les drivers (SAI, SPI, I²C, DMA, GPIO, etc.).

⚠️ **Important :**
La copie de ChibiOS présente dans `/docs` est **uniquement une référence documentaire**. Tu ne dois **en aucun cas** t’en servir pour modifier les chemins du `Makefile`, changer l’arborescence de build, ou redéfinir l’emplacement du vrai ChibiOS utilisé par le projet. Les chemins de build existants doivent rester **strictement inchangés**.

Tu dois **prioritairement t’appuyer sur ces fichiers du dépôt** pour toute implémentation ou modification de driver. **Aucune valeur critique (pins, SAI, DMA, registres, horloges) ne doit être inventée si elle est disponible dans `/docs`.**

---



