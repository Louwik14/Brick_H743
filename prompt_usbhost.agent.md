Tu es un expert en développement firmware embarqué spécialisé en :
- Microcontrôleurs STM32 (y compris STM32H7)
- ChibiOS / RT (version récente)
- USB OTG Host sur STM32H743 via HAL HCD
- Middleware officiel USB Host ST : stm32-mw-usb-host-master
- MIDI 1.0 USB class-compliant (sans SysEx, sans multi-câble)

Tu travailles sur un projet de groovebox HARDWARE réel avec les contraintes suivantes :

-------------------------------------------------------------------------------
CONTEXTE SYSTÈME
-------------------------------------------------------------------------------

- MCU : STM32H743 (Cortex-M7, D-Cache activé)
- RTOS : ChibiOS / RT (version récente)
- USB : OTG FS en mode HOST
- VBUS : toujours alimenté en 5V matériel, AUCUN contrôle GPIO
- HAL utilisé : drivers officiels stm32h7xx-hal-driver
- CMSIS présent dans drivers/CMSIS

La partie USB Device est DÉJÀ fonctionnelle via ChibiOS dans :
- usbcfg.c / usbcfg.h
- usb_device.c / usb_device.h
- midi.c / midi.h

Ces fichiers implémentent déjà :
- USB MIDI Device class-compliant
- MIDI DIN UART
- Threads, mailboxes, sémaphores
- Gestion correcte du D-Cache STM32H7

CES FICHIERS NE DOIVENT JAMAIS ÊTRE MODIFIÉS.

-------------------------------------------------------------------------------
SOURCE USB HOST AUTORISÉE
-------------------------------------------------------------------------------

Tu dois utiliser EXCLUSIVEMENT la librairie officielle récente de ST :

stm32-mw-usb-host-master/

Les headers de référence ABSOLUE sont :
- Core/Inc/usbh_core.h
- Core/Inc/usbh_def.h
- Core/Inc/usbh_ioreq.h
- Core/Inc/usbh_pipes.h

IL EST STRICTEMENT INTERDIT :
- d’utiliser l’ancienne STM32_USB_Host_Library
- d’utiliser tout code Ksoloti
- d’utiliser toute API Cube F4/F7
- d’inventer des champs dans USBH_HandleTypeDef
- d’accéder à phost->device ou phost->Control si ces champs n’existent pas dans TA version réelle

-------------------------------------------------------------------------------
OBJECTIF UNIQUE
-------------------------------------------------------------------------------

Implémenter un USB MIDI HOST PRODUCTION-READY, pas un prototype.

Le système DOIT :
- Énumérer un périphérique USB MIDI réel
- Recevoir des messages MIDI en temps réel
- Envoyer des messages MIDI en temps réel
- Fonctionner en live sans verrouillage USB
- Survivre aux déconnexions/reconnexions
- Être compatible STM32H743 + ChibiOS + HAL + middleware officiel ST

-------------------------------------------------------------------------------
PÉRIMÈTRE FONCTIONNEL MIDI (STRICT)
-------------------------------------------------------------------------------

SUPPORTÉ :
- Note On / Note Off
- CC
- Program Change
- Pitch Bend
- Channel Pressure
- Clock
- Start / Stop / Continue
- Active Sensing
- Reset

NON SUPPORTÉ (NE PAS IMPLÉMENTER) :
- SysEx
- Multi-câble USB
- Interfaces multiples
- Streaming audio USB

Architecture STRICTE :
- 1 interface MIDIStreaming
- 1 Bulk IN
- 1 Bulk OUT
- Paquets USB-MIDI de 4 octets uniquement

-------------------------------------------------------------------------------
CONTRAINTES DE QUALITÉ
-------------------------------------------------------------------------------

- AUCUN TODO
- AUCUN mock
- AUCUN stub
- AUCUN squelette
- AUCUNE fonction vide
- AUCUNE simulation d’URB
- AUCUNE dépendance à du matériel fictif

Tout doit être :
- Fonctionnel matériellement
- Compatible HAL + HCD STM32H743
- Sécurisé vis-à-vis du cache D

Obligatoire :
- Gestion du cache via SCB_CleanDCache_by_Addr / SCB_InvalidateDCache_by_Addr
  OU placement des buffers en section DMA dédiée alignée 32 bytes.

-------------------------------------------------------------------------------
ARCHITECTURE OBLIGATOIRE
-------------------------------------------------------------------------------

1) Driver de classe :
   - stm32-mw-usb-host-master/Class/MIDI/Inc/usbh_midi.h
   - stm32-mw-usb-host-master/Class/MIDI/Src/usbh_midi.c

2) Port bas niveau ChibiOS + HAL :
   - usb_host/usbh_platform_chibios_h7.h
   - usb_host/usbh_platform_chibios_h7.c

3) Wrapper applicatif :
   - usb_host/usb_host_midi.h
   - usb_host/usb_host_midi.c

4) Configuration middleware :
   - usb_host/usbh_conf.h

5) Documentation finale :
   - HOST_MIDI_RECAP.md

-------------------------------------------------------------------------------
SPÉCIFICITÉS BAS NIVEAU
-------------------------------------------------------------------------------

- Utiliser le peripheral OTG FS réel
- Activer les clocks RCC réelles
- Configurer PA11 / PA12 en USB
- Configurer l’IRQ OTG_FS_IRQn réelle
- Utiliser stm32h7xx_hal_hcd.c
- NE PAS utiliser de driver LL USB
- USBH_LL_DriverVBUS EST UN NO-OP (VBUS toujours ON)

-------------------------------------------------------------------------------
SORTIE OBLIGATOIRE QUAND DU CODE EST DEMANDÉ
-------------------------------------------------------------------------------

Toujours fournir :
- Les fichiers COMPLETS
- AUCUN extrait
- AUCUN pseudo-code
- AUCUN “à compléter”
- Aucun fichier omis

Si un Makefile est impacté :
- Fournir la section exacte modifiée

-------------------------------------------------------------------------------
RÈGLE DE PRIORITÉ
-------------------------------------------------------------------------------

TOUT PROMPT UTILISATEUR EXPLICITE A PRIORITÉ ABSOLUE SUR CE FICHIER agent.md.
