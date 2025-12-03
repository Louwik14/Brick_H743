# 🤖 agent.md — Contexte & Règles du projet Groovebox STM32H743

Ce dépôt contient le firmware principal d’une **groovebox matérielle temps réel** basée sur **STM32H743 + ChibiOS RT**.
L’objectif est de construire **une machine musicale déterministe, robuste et modulaire**, inspirée des architectures type Elektron / Octatrack.

⚠️ **Ce fichier est L’UNIQUE source de vérité pour l’IA (Codex / ChatGPT) entre les passes.  
Il doit être relu AVANT toute modification de code, et mis à jour À LA FIN de chaque passe.**

---

## ✅ 1. Plateforme matérielle

### MCU principal
- STM32H743 (Cortex-M7, FPU, D-Cache)
- Horloge audio maître générée par le MCU

### Audio
- **ADC** : 2× ADAU1979 en TDM → 8 canaux d’entrée
- **DAC** : PCM4104 en TDM → 4 canaux de sortie
  - 2 canaux = sortie ligne
  - 2 canaux = sortie casque (via ampli analogique)
- Format : 48 kHz / 24 bits (stockés dans int32)
- TDM 8×32 bits en entrée, 4×32 bits en sortie
- Le STM32 est **MAÎTRE des horloges audio**

### Bus temps réel
- SAI + DMA double buffer ping/pong
- Traitement audio **exclusivement dans un thread ChibiOS**, jamais en IRQ

---

## ✅ 2. Cartouches sonores (SPI-Link)

- Jusqu’à 4 cartouches basées sur STM32F429
- Chaque cartouche :
  - Génère son propre audio
  - Est esclave SPI du H743
  - Ne connaît ni tempo, ni patterns, ni séquenceur
- Communication via **trames SPI fixes (DMA)** avec :
  - Bloc audio
  - Zone d’events sérialisés

---

## ✅ 3. Architecture logicielle audio

Fichiers clés :

- `audio_conf.h`  
  Paramètres globaux : Fs, nombre de canaux, tailles de buffers, DMA.

- `audio_codec_ada1979.c/h`  
  Initialisation I2C des ADC (PLL, TDM, slots, mute, volumes).

- `audio_codec_pcm4104.c/h`  
  DAC en mode matériel autonome (pas de SPI).

- `drv_audio.c/h`  
  Cœur audio :
  - DMA RX/TX
  - Ping/pong buffers
  - Routing MAIN / CUE
  - Volume maître
  - Mix pistes
  - Architecture SEND FX
  - Hook DSP faible

- `recap_audio.txt`  
  **Document technique officiel du pipeline audio.  
  Il doit être systématiquement mis à jour à chaque passe.**

---

## ✅ 4. Contraintes de développement (STRICTES)

- C99 uniquement
- ❌ AUCUN malloc / calloc / free
- ❌ AUCUNE allocation dynamique
- ❌ Aucune dépendance à CubeMX
- ✅ ChibiOS RT + LLD uniquement
- ✅ Code déterministe temps réel
- ✅ Tout traitement audio hors IRQ
- ✅ Style production, pas pédagogique

---

## ✅ 5. Cache, DMA & Robustesse H743

Le STM32H743 possède un **D-Cache actif**.
Les buffers DMA audio doivent être :

- Placés dans une **RAM non cacheable** (ex: `.ram_d2`)
- Alignés sur 32 bytes minimum
- Jamais manipulés avec des incohérences cache

Les callbacks DMA doivent :

- Gérer HT / TC
- **Détecter TEIF / DMEIF / FEIF**
- Appeler `chSysHalt()` en cas d’erreur critique (priorité : silence sûr)

---

## ✅ 6. Modèle de mix actuel

- 4 pistes stéréo (ADC → pistes 0 à 3)
- Bus MAIN → sortie ligne
- Bus CUE → sortie casque
- Gains par piste :
  - `gain_main`
  - `gain_cue`
- **Architecture SEND FX globale en place (bypass pour l’instant)** :
  - `gain_send` par piste
  - Bus SEND → FX → RETURN → MAIN

Aucun effet réel n’est encore implémenté (structure seulement).

---

## ✅ 7. Évolution future prévue

- Effets en SEND globaux :
  - Reverb
  - Delay
  - Granular (inspiré de Mutable Instruments / Clouds)
- Séquenceur piloté par le compteur d’échantillons audio
- Mixage audio interne + audio en provenance des cartouches SPI
- UI pilotant uniquement des **paramètres**, jamais le temps audio

---

## ✅ 8. Dossier /docs

Dans le dossier **`/docs`**, sont présents :

- Les **datasheets des codecs audio**
- Les **fichiers de configuration board** (`board.h`, `board.c`)
- La **copie exacte de ChibiOS utilisée dans ce projet**

⚠️ Important :
- Cette copie de ChibiOS est **uniquement une référence documentaire**
- ❌ Le chemin du ChibiOS **ne doit JAMAIS être modifié dans les Makefiles**
- ❌ Codex ne doit pas s’en servir pour changer l’architecture de build
- Les `mcuconf.h` restent **ceux du projet**, pas ceux de `/docs`

---

## ✅ 9. Règles de travail pour l’IA (Codex)

Avant chaque passe :

1. Lire **impérativement** :
   - `agent.md`
   - `recap_audio.txt`

2. Ne modifier **QUE** les fichiers explicitement demandés.

3. Ne jamais :
   - Introduire de dépendance circulaire
   - Ajouter de logique UI dans les drivers bas niveau
   - Modifier les IRQ sauf instruction explicite
   - Ajouter de fonctionnalités non demandées

4. Après chaque passe :
   - Mettre à jour `recap_audio.txt`
   - Décrire ce qui a réellement changé
   - Mentionner les limites connues

---

## ✅ 10. Philosophie du projet

Ce projet vise :

- Une **machine musicale matérielle sérieuse**
- Temps réel strict
- Aucun comportement non déterministe
- Architecture inspirée de :
  - consoles de mixage
  - groovebox Elektron
  - systèmes modulaires numériques

La priorité est :
> **Stabilité → Déterminisme → Qualité audio → Fonctionnalités**

---

🛑 **Si une instruction du prompt contredit ce fichier `agent.md`, ce fichier a TOUJOURS priorité.**
