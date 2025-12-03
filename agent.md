Tu es un expert en développement firmware embarqué spécialisé en :
- Microcontrôleurs STM32 (y compris STM32H7)
- ChibiOS / RT
- Piles USB Host, en particulier la librairie officielle récente de ST :
  "stm32-mw-usb-host-master"
- MIDI 1.0, périphériques USB MIDI class-compliant, SysEx et messages temps réel

Tu travailles sur un projet avec les contraintes suivantes :

- RTOS : ChibiOS / RT (version récente)
- MCU : STM32H743 (Cortex-M7, D-Cache activé)
- La partie USB Device est déjà entièrement implémentée avec le driver USB Device de ChibiOS dans les fichiers :
  - `usbcfg.c`, `usbcfg.h`
  - `usb_device.c`, `usb_device.h`
  - `midi.c`, `midi.h`
- Ces fichiers implémentent déjà :
  - Un périphérique USB MIDI Class-Compliant
  - Une sortie MIDI DIN via UART
  - Un thread dédié, des mailboxes, des sémaphores (`tx_sem`, `sof_sem`)
  - La gestion du D-Cache pour STM32H7
- Le but est maintenant d’ajouter une implémentation **USB MIDI Host**.

Tu disposes aussi de :

1) Un dossier **`stm32-mw-usb-host-master/`**
   - Il correspond à la version **récente officielle** de la librairie USB Host de ST.
   - Cette librairie est la **référence absolue** pour l’API USB Host.
   - Tout le code Host doit être **strictement compatible avec cette librairie**.

2) Un dossier **`inspiration_host/`** contenant uniquement du code ancien servant
   d’**inspiration fonctionnelle**, mais **PAS comme base d’implémentation** :
   - Un ancien dossier `STM32_USB_Host_Library/` obsolète :
     - `Core/Inc/*.h`, `Core/Src/*.c`
     - `Class/HID/...`
     - `Class/CDC/...`
   - Du vieux code du projet Ksoloti :
     - `usbh_midi_core.c`, `usbh_midi_core.h`
     - `usbh_vendor.c`
     - `usbh_conf.c`, `usbh_conf.h`

**TRÈS IMPORTANT :**
- Le code contenu dans `inspiration_host/` est **uniquement une source d’idées**.
- Il ne doit **jamais être copié-collé tel quel**.
- L’implémentation finale doit être :
  - Propre
  - Moderne
  - Robuste
  - Adaptée au STM32H743
  - Compatible exclusivement avec `stm32-mw-usb-host-master`
- Il est strictement interdit de coder contre l’ancienne lib obsolète.

-------------------------------------------------------------------------------
OBJECTIF HAUT NIVEAU
-------------------------------------------------------------------------------

Implémenter une pile **USB MIDI Host robuste et professionnelle** qui :

1. Utilise exclusivement la librairie **`stm32-mw-usb-host-master`** comme cœur USB Host.
2. Fournit un **driver de classe MIDI** intégré proprement dans cette librairie.
3. Fournit une **API wrapper compatible ChibiOS** pour l’application :
   - Initialisation de l’USB Host
   - Démarrage / arrêt du host
   - Détection des connexions / déconnexions
   - Réception des messages MIDI depuis les périphériques USB
   - Envoi de messages MIDI vers ces périphériques
4. Supporte le maximum de périphériques **USB-MIDI class-compliant** :
   - Descripteurs AudioControl + MIDIStreaming standards
   - Multiples endpoints IN/OUT
   - Multiples jacks embeddés / externes
   - Multiples câbles virtuels
   - Gestion complète des SysEx (longueur arbitraire)
   - Messages System RealTime (Clock, Start, Stop, Continue, Active Sensing, Reset)
5. Soit écrit en C clair, structuré, documenté, avec des commentaires de type Doxygen.

-------------------------------------------------------------------------------
EXIGENCES D’ARCHITECTURE
-------------------------------------------------------------------------------

### 1) Driver de classe USB Host MIDI (niveau librairie ST)

Créer un nouveau driver de classe dans l’arborescence de
`stm32-mw-usb-host-master` :

- `STM32_USB_Host_Library/Class/MIDI/Inc/usbh_midi.h`
- `STM32_USB_Host_Library/Class/MIDI/Src/usbh_midi.c`

Ce driver doit :

- Suivre strictement le style des autres drivers de classe ST (`CDC`, `HID`, etc.).
- Exposer une structure `USBH_ClassTypeDef`, par exemple `USBH_MIDI_Class`.
- Implémenter les points d’entrée classiques :
  - `USBH_MIDI_InterfaceInit(USBH_HandleTypeDef *phost)`
  - `USBH_MIDI_InterfaceDeInit(USBH_HandleTypeDef *phost)`
  - `USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost)`
  - `USBH_MIDI_Process(USBH_HandleTypeDef *phost)`
  - `USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost)` si nécessaire

- Lors de l’énumération USB, parser le descripteur de configuration :
  - Trouver les interfaces AudioControl et MIDIStreaming
  - Trouver tous les jacks MIDI IN / MIDI OUT
  - Trouver tous les endpoints Bulk IN et Bulk OUT de l’interface MIDIStreaming
  - Gérer le cas de plusieurs endpoints

- Maintenir une structure `MIDI_HandleTypeDef` contenant :
  - Les adresses d’endpoints, tailles de paquets, numéros de pipes IN/OUT
  - Des buffers circulaires pour :
    - Les paquets entrants USB-MIDI (4 octets)
    - Les paquets sortants USB-MIDI
  - Des machines d’état pour les transferts IN et OUT (`IDLE`, `SEND`, `WAIT`, etc.)
  - Des flags indiquant si les endpoints sont valides / prêts
  - Des compteurs statistiques

- Gérer proprement les URB :
  - Re-soumettre en continu les URB IN tant que le périphérique est connecté
  - Pour les OUT :
    - Envoyer quand des données sont disponibles
    - Gérer `URB_NOTREADY`, `URB_ERROR`, NAK, etc.
    - Ne jamais boucler en attente active
  - Toute la logique doit passer par `USBH_MIDI_Process()`

- Format MIDI :
  - Utiliser strictement le format USB-MIDI Event Packet de 4 octets
  - Fournir des helpers pour :
    - Convertir des octets MIDI bruts en paquets USB-MIDI
    - Gérer le morcellement des SysEx (CIN 0x4 / 0x5 / 0x6 / 0x7)
    - Gérer correctement les messages temps réel

- L’API publique de `usbh_midi.h` doit exposer au minimum :
  - Une fonction pour savoir si un périphérique MIDI est prêt
  - Une fonction pour lire un paquet USB-MIDI reçu
  - Une fonction pour envoyer un paquet USB-MIDI
  - Optionnellement : envoi de notes, CC, SysEx, etc. en octets bruts

- Les tailles de buffers doivent être configurables par des macros.

- Le tout doit être robuste face aux débordements et erreurs USB.

### 2) Wrapper ChibiOS côté application

Créer un wrapper dans un dossier séparé `usb_host/` :

- `usb_host/usb_host_midi.h`
- `usb_host/usb_host_midi.c`

Responsabilités :

- Initialiser le cœur USB Host + le driver MIDI
- Créer un thread ChibiOS dédié qui appelle périodiquement :
  - `USBH_Process(&hUsbHostFS)` (ou HS selon la configuration)
- Fournir une API simple pour l’application :
  - Détecter connexion / déconnexion
  - Recevoir les messages MIDI
  - Envoyer des messages MIDI

- Utiliser exclusivement les primitives ChibiOS :
  - `chThdCreateStatic`
  - `chMBObjectInit`
  - `chMBPostTimeout`
  - `chMBFetchTimeout`
  - Sémaphores si besoin

- Pas d’allocation dynamique à l’exécution
- Aucun appel bloquant en ISR
- Timeouts pour tous les waits

- Le wrapper doit pouvoir s’intégrer naturellement avec ton `midi.c` existant
  (même philosophie de stats, queues, etc.).

### 3) Couche bas niveau / portage STM32H7

Le fichier `usbh_conf.c` de Ksoloti est obsolète pour STM32H7.

Créer une couche propre spécifique STM32H743 + ChibiOS :

- `usb_host/usbh_platform_chibios_h7.h`
- `usb_host/usbh_platform_chibios_h7.c`

Cette couche doit :
- Initialiser le contrôleur USB OTG utilisé en mode Host
- Implémenter toutes les fonctions requises par la librairie ST :
  - `USBH_LL_Init`, `USBH_LL_DeInit`, `USBH_LL_Start`, `USBH_LL_Stop`
  - `USBH_LL_GetSpeed`, `USBH_LL_ResetPort`, `USBH_LL_GetLastXferSize`, etc.
- Être clairement commentée avec :
  - Les parties dépendantes du hardware
  - Les pins à adapter (VBUS, ID, overcurrent, etc.)
  - Les TODO liés à la carte réelle

Une première version “template” est acceptable si le matériel exact n’est pas connu.

-------------------------------------------------------------------------------
STYLE DE CODE & QUALITÉ
-------------------------------------------------------------------------------

- C moderne (C99/C11)
- Nommage clair et cohérent
- Commentaires Doxygen sur toutes les API publiques
- Gestion explicite des erreurs
- Statistiques détaillées (drops, erreurs URB, buffers pleins, etc.)
- Aucune valeur magique non documentée

-------------------------------------------------------------------------------
API ATTENDUE
-------------------------------------------------------------------------------

Dans `usbh_midi.h` :

- `bool USBH_MIDI_IsReady(USBH_HandleTypeDef *phost);`
- `bool USBH_MIDI_ReadEvent(USBH_HandleTypeDef *phost, uint8_t *packet4);`
- `bool USBH_MIDI_WriteEvent(USBH_HandleTypeDef *phost, const uint8_t *packet4);`
- Optionnel : `USBH_MIDI_SendShortMessage`, `USBH_MIDI_SendSysEx`, etc.

Dans `usb_host_midi.h` :

- `void usb_host_midi_init(void);`
- `bool usb_host_midi_is_device_attached(void);`
- `bool usb_host_midi_is_ready(void);`

- Fonctions RX non bloquantes
- Fonctions TX niveau MIDI (NoteOn, CC, SysEx, etc.)
- Callbacks optionnels pour connexion / déconnexion

L’API doit être simple pour un développeur qui n’est pas expert USB.

-------------------------------------------------------------------------------
SORTIES ATTENDUES
-------------------------------------------------------------------------------

1) Le contenu complet des fichiers suivants, chacun dans un bloc de code séparé,
avec un commentaire en tête indiquant le chemin :

   - `STM32_USB_Host_Library/Class/MIDI/Inc/usbh_midi.h`
   - `STM32_USB_Host_Library/Class/MIDI/Src/usbh_midi.c`
   - `usb_host/usb_host_midi.h`
   - `usb_host/usb_host_midi.c`
   - `usb_host/usbh_platform_chibios_h7.h`
   - `usb_host/usbh_platform_chibios_h7.c` (si nécessaire)

2) À la fin, un fichier récapitulatif en Markdown :

`HOST_MIDI_RECAP.md`

dans un bloc de code Markdown, contenant :

- Vue d’ensemble de l’architecture
- Interaction entre :
  - la librairie ST USB Host récente (`stm32-mw-usb-host-master`)
  - le driver de classe MIDI
  - le wrapper ChibiOS
- Ce que l’application doit appeler pour :
  - Initialiser
  - Démarrer
  - Envoyer / Recevoir des messages MIDI
- Paramètres configurables
- Hypothèses matérielles
- Limitations actuelles
- TODO pour les itérations suivantes

-------------------------------------------------------------------------------
DÉMARRER L’IMPLÉMENTATION
-------------------------------------------------------------------------------

Commence par un court paragraphe décrivant le plan d’implémentation,
puis génère immédiatement tous les fichiers C/H demandés,
et termine par `HOST_MIDI_RECAP.md`.

Le dernier prompt est prioritaire sur agent.md