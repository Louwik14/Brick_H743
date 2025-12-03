# USB MIDI Host Recap

## Vue d'ensemble
- **Librairie cœur** : `stm32-mw-usb-host-master` fournit l'USB Host core et les callbacks utilisateur.
- **Classe MIDI** : `STM32_USB_Host_Library/Class/MIDI` implémente un driver USB-MIDI class-compliant (bulk IN/OUT, parsing descriptor minimal, machines d'état RX/TX, buffers circulaires configurables).
- **Wrapper ChibiOS** : `usb_host/usb_host_midi.{c,h}` gère l'initialisation du core Host, l'enregistrement de la classe MIDI, la boucle `USBH_Process()` dans un thread dédié, ainsi que des mailboxes RX/TX pour échanger des paquets MIDI (4 octets) avec l'application sans allocation dynamique.
- **Portage bas niveau** : `usb_host/usbh_platform_chibios_h7.{c,h}` fournit un gabarit pour connecter l'OTG STM32H7 à la librairie Host via ChibiOS (pipes, URB, VBUS). Les sections matérielles sont balisées `TODO`.

## Flux d'initialisation
1. `usb_host_midi_init()` :
   - Initialise les mailboxes RX/TX.
   - `USBH_Init()` avec le callback `USBH_UserProcess` pour suivre les événements (connexion, classe active, déconnexion).
   - `USBH_RegisterClass()` avec `USBH_MIDI_Class` puis `USBH_Start()`.
   - Lance le thread `usb_host_midi_thread` qui appelle périodiquement `USBH_Process()` et relaie RX/TX.
2. `USBH_MIDI_InterfaceInit()` (côté classe) :
   - Sélection de l'interface Audio/MIDIStreaming.
   - Découverte des endpoints bulk IN/OUT, ouverture des pipes et initialisation des buffers circulaires.

## API côté application
- **État** :
  - `usb_host_midi_is_device_attached()` : détection connexion.
  - `usb_host_midi_is_ready()` : classe active et endpoints prêts.
- **Réception** :
  - `usb_host_midi_fetch_event(uint8_t packet[4])` : lecture non bloquante d'un paquet USB-MIDI depuis la mailbox RX.
- **Transmission** :
  - `usb_host_midi_send_event(const uint8_t packet[4])` : poste un paquet à envoyer (mailbox TX) ; le thread le transmet via `USBH_MIDI_WriteEvent()`.
- **Callbacks optionnels** : `usb_host_midi_register_attach_callback()` / `usb_host_midi_register_detach_callback()` pour réagir aux connexions.
- **Helpers classe** : `USBH_MIDI_EncodeShortMessage()` pour formater un message court MIDI en paquet USB-MIDI.

## Paramètres configurables
- Tailles de buffers d'événements MIDI (RX/TX) : macros `USBH_MIDI_RX_EVENT_BUFFER`, `USBH_MIDI_TX_EVENT_BUFFER` dans `usbh_midi.h`.
- Taille des mailboxes du wrapper : macros en tête de `usb_host_midi.c`.
- Taille maximale de paquet bulk : `USBH_MIDI_MAX_PACKET_SIZE` (64 par défaut).

## Hypothèses matérielles
- MCU STM32H743 en mode USB Host Full-Speed/High-Speed via OTG.
- ChibiOS fournit les primitives temps réel et un driver USBH adapté au contrôleur OTG.
- Les signaux VBUS/ID/OC et la configuration d'horloge OTG doivent être renseignés dans `usbh_platform_chibios_h7.c`.

## Limitations actuelles
- La couche bas niveau OTG est un gabarit : aucun accès réel aux registres ni au driver ChibiOS n'est implémenté.
- Le parsing de descripteur MIDIStreaming est volontairement minimal (1 interface bulk IN/OUT) et devra être étendu pour plusieurs câbles/jacks.
- Pas encore de gestion avancée des SysEx longue durée côté wrapper (traité paquet par paquet).

## TODO / itérations futures
- Compléter `usbh_platform_chibios_h7.c` avec l'initialisation réelle du contrôleur OTG (clocks, GPIO, VBUS, interruptions) et l'appel au driver USBH ChibiOS.
- Étendre le parseur pour plusieurs endpoints et jacks externes/embeddés, avec mapping des câbles virtuels.
- Ajouter des statistiques détaillées et des hooks de monitoring (profiling des URB, taux de drop mailboxes).
- Gérer des APIs MIDI de plus haut niveau (NoteOn/Off, CC, SysEx streaming avec segmentation).
- Tester avec plusieurs périphériques USB-MIDI class-compliant et vérifier le comportement D-Cache STM32H7.
