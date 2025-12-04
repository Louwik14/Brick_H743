# 🤖 agent.md — Contexte & Règles du projet Groovebox STM32H743

Ce dépôt contient le firmware principal d’une **groovebox matérielle temps réel** basée sur **STM32H743 + ChibiOS RT**.
L’objectif est de construire **une machine musicale déterministe, robuste et modulaire**, inspirée des architectures type Elektron / Octatrack.

⚠️ **Ce fichier est L’UNIQUE source de vérité pour l’IA (Codex / ChatGPT) entre les passes.
Il doit être relu AVANT toute modification de code, et mis à jour À LA FIN de chaque passe.**

🛑 **Si une instruction d’un prompt contredit ce fichier `agent.md`, ce fichier a TOUJOURS priorité.**

---

## ✅ 1. Plateforme matérielle

### MCU principal

* STM32H743 (Cortex-M7, FPU, D-Cache)
* Horloge audio maître générée par le MCU
* D-Cache **ACTIF**
* MPU utilisé pour définir des régions **non-cacheables**

---

## ✅ 2. Audio (socle validé industriellement)

### Chaîne audio

* **ADC** : 2× ADAU1979 en TDM → 8 canaux d’entrée
* **DAC** : PCM4104 en TDM → 4 canaux de sortie

  * 2 canaux = sortie ligne
  * 2 canaux = sortie casque (via ampli analogique)
* Format : **48 kHz / 24 bits** (stockés dans `int32`)
* TDM :

  * 8×32 bits en entrée
  * 4×32 bits en sortie
* Le STM32 est **MAÎTRE des horloges audio**

### Transport temps réel

* SAI + DMA double buffer **ping/pong**
* Traitement audio **exclusivement dans un thread ChibiOS**
* IRQ DMA = **signalisation uniquement**

### Sécurité cache / DMA (VALIDÉE)

* Buffers audio placés en **SRAM D2 non-cacheable via MPU**
* Section dédiée `.ram_d2` mappée à `0x30040000`
* Alignement minimum : **32 bytes**
* **AUCUNE opération SCB_Clean/Invalidate** dans le pipeline audio
* Coordination RX/TX :

  * Synchronisation stricte par demi-buffer
  * Aucun croisement ping/pong possible
* ISR DMA :

  * Détection `TEIF / DMEIF / FEIF`
  * `chSysHalt("AUDIO DMA ERROR")` en cas de faute

✅ Le socle audio est **mathématiquement sûr** sur STM32H743.

---

## ✅ 3. Cartouches sonores (SPI-Link)

* Jusqu’à **4 cartouches** basées sur **STM32F429**
* Chaque cartouche :

  * Produit son propre audio
  * Est **esclave SPI** du H743
  * Ne connaît :

    * ni tempo
    * ni patterns
    * ni séquenceur
* Elle ne fait **qu’appliquer des EVENTS**

---

## ✅ 4. Architecture SPI — Event-Driven V2

Chaque trame SPI fixe contient :

```
HEADER
FRAME_COUNTER
AUDIO_BLOCK (4 ch × 16 samples × int32)
EVENT_COUNT
EVENTS_AREA
FOOTER
```

Types d’events autorisés (V1) :

* NOTE_ON
* NOTE_OFF
* PARAM_SET
* CART_ID
* FW_VERSION

Contraintes absolues :

* ❌ Aucun `track_id` dans les events
* ✅ Le **SPI utilisé définit implicitement la cartouche cible**
* ✅ Architecture **100 % event-driven**
* ❌ Aucun PLAY / STOP global

---

## ✅ 5. Séquenceur (côté H7)

* 16 pistes
* 64 steps par piste
* Steps = TRIGS + P-LOCKS
* Le moteur :

  1. lit le pattern actif
  2. génère une **liste temporaire d’events**
  3. l’envoie immédiatement sur les SPI
  4. **ne stocke jamais d’events persistants**

---

## ✅ 6. Stockage patterns / samples (Carte SD)

* Tous les patterns sont stockés sur **carte SD**
* **Un seul pattern en RAM à la fois**
* Changement de pattern :

  1. Sauvegarde sur SD
  2. Chargement depuis la SD
  3. Reprise lecture
* La carte SD est :

  * retirée manuellement par l’utilisateur pour transfert sur PC
  * **pas exposée en USB Mass Storage**
* **Aucune dépendance temps réel à la SD**
* ❌ La SD n’est **PAS utilisée pour le streaming audio pour l’instant**

### Mise à jour firmware

* ❌ Aucune mise à jour firmware dans l’application
* ✅ La mise à jour firmware se fait **exclusivement via un BOOTLOADER externe**
* Ce dépôt **ne contient PAS de code de bootloader**
* Le driver SD applicatif :

  * ❌ ne lit jamais de `.bin`
  * ❌ ne touche jamais à la flash interne

---

## ✅ 7. Architecture logicielle en couches (héritée de Brick)

Architecture **STRICTEMENT hiérarchique, sans dépendance circulaire** :

```
Application / Modes
↓
UI
↓
Backend neutre
↓
Link / Registry
↓
Bus
↓
Drivers
↓
HAL ChibiOS
```

Règles absolues :

* L’UI **n’accède jamais directement au bus**
* L’UI **n’inclut aucun driver**
* Les drivers **ne connaissent aucune logique applicative**
* Toute communication passe par un **backend neutre**
* Chaque couche dépend **uniquement de celle du dessous**
* Les headers publics sont **minimaux**
* **Forward-declaration obligatoire** entre couches
* ❌ **Aucune dépendance circulaire autorisée**

---

## ✅ 8. Séparation des responsabilités

* UI : rendu + interaction
* Backend : routage neutre
* Link : shadow, filtres, anti-redondance, sérialisation logique
* Bus : SPI / DMA brut
* Drivers : hardware pur uniquement

---

## ✅ 9. Contraintes de développement (STRICTES)

* C99 uniquement
* ❌ Aucun malloc / calloc / free
* ❌ Aucune allocation dynamique
* ❌ Aucune dépendance à CubeMX
* ✅ ChibiOS RT + LLD uniquement
* ✅ Code déterministe temps réel
* ✅ Tout traitement audio hors IRQ
* ✅ Aucun accès SD depuis les IRQ
* ✅ Style **production**, pas pédagogique

---

## ✅ 10. Dossier `/docs` & ChibiOS

Dans `/docs/ChibiOS` se trouve **la copie de ChibiOS EFFECTIVEMENT utilisée par le projet**.
Le Makefile pointe dessus via :

```make
CHIBIOS := ./docs/ChibiOS
```

⚠️ Règles importantes pour Codex / ChatGPT :

* ✅ `docs/ChibiOS` est la **racine ChibiOS réelle du build**
* ❌ Ne pas déplacer / renommer ce dossier
* ❌ Ne pas changer la variable `CHIBIOS` dans le Makefile
* ❌ Ne pas “mettre à jour” ChibiOS tout seul
* ✅ Les fichiers de config projet (`mcuconf.h`, `halconf.h`, etc.) se trouvent dans `cfg/` et sont **la source de vérité**, même si ChibiOS fournit des templates ailleurs.

Autres fichiers dans `/docs` :

* Datasheets audio
* Board files (`board.h`, `board.c`)
* Matériel de référence

---

## ✅ 11. Règles de travail pour l’IA (Codex)

Avant chaque passe :

1. Lire **obligatoirement** :

   * `agent.md`

2. Modifier **UNIQUEMENT** les fichiers explicitement listés par l’utilisateur.

3. Interdictions formelles :

   * Introduire une dépendance circulaire
   * Ajouter de l’UI dans les drivers
   * Ajouter de la logique applicative dans les drivers
   * Ajouter de la logique SPI lourde dans les IRQ
   * Ajouter de la logique audio dans les IRQ
   * Ajouter des fonctionnalités non demandées
   * Toucher à l’organisation ChibiOS / Makefile sans instruction explicite

4. Après chaque passe :

   * lister précisément les fichiers modifiés
   * expliquer ce qui a changé
   * mentionner les limites connues restantes

---

## ✅ 12. Philosophie du projet

Ce projet vise :

* une **machine musicale matérielle sérieuse**
* temps réel strict
* aucun comportement non déterministe
* architecture inspirée de :

  * consoles de mixage
  * groovebox Elektron
  * systèmes modulaires numériques

Ordre de priorité immuable :

> **Stabilité → Déterminisme → Qualité audio → Fonctionnalités**

---

