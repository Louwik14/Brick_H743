# USB MIDI Host Recap

## Vue d'ensemble
- **Librairie cœur** : `stm32-mw-usb-host-master` fournit l'USB Host core et les callbacks utilisateur.
- **Classe MIDI** : `STM32_USB_Host_Library/Class/MIDI` implémente maintenant un driver USB-MIDI complet (bulk IN/OUT, parsing descripteur robuste pour une interface MIDIStreaming, machines d'état RX/TX fonctionnelles, buffers circulaires configurables).
- **Wrapper ChibiOS** : `usb_host/usb_host_midi.{c,h}` crée un thread qui appelle en continu `USBH_Process()`, pompe les files RX/TX via des mailboxes ChibiOS et expose des callbacks d'attach/detach.
- **Portage bas niveau** : `usb_host/usbh_platform_chibios_h7.{c,h}` reste un gabarit compilable : le comportement URB est simulé, et chaque étape dépendante du hardware (clocks OTG, GPIO VBUS/ID/OC, driver ChibiOS USBH) est balisée par des `TODO` détaillés.

## Flux d'initialisation
1. `usb_host_midi_init()` :
   - Initialise les mailboxes RX/TX.
   - `USBH_Init()` + `USBH_RegisterClass(&USBH_MIDI_Class)` + `USBH_Start()`.
   - Lance le thread `usb_host_midi_thread` qui appelle `USBH_Process()` toutes les ~1 ms puis `pump_rx_events()` et `pump_tx_events()`.
2. `USBH_MIDI_InterfaceInit()` (côté classe) :
   - Sélectionne l'interface Audio/MIDIStreaming (classe 0x01 / sous-classe 0x03, alt 0).
   - Détecte un bulk IN et un bulk OUT, limite la taille de paquet à `USBH_MIDI_MAX_PACKET_SIZE`, ouvre les pipes et remet à zéro les ring buffers + compteurs.

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

## Stratégie RX/TX
- **Classe** :
  - RX : si pipe IN idle, `USBH_BulkReceiveData()` soumet un URB. À `URB_DONE`, chaque paquet 4 octets est poussé dans la ring buffer `rx_events` (drops comptés dans `rx_dropped`).
  - TX : les paquets de la ring buffer `tx_events` sont concaténés jusqu'à la taille de paquet bulk puis envoyés via `USBH_BulkSendData()`. Les échecs `NOTREADY/STALL` réarment l'état, les erreurs incrémentent `tx_dropped`.
- **Wrapper ChibiOS** :
  - `pump_rx_events()` lit la classe tant que possible et pousse dans la mailbox RX ; si pleine, les nouveaux paquets sont simplement ignorés.
  - `pump_tx_events()` dépile la mailbox TX et tente de les injecter dans la ring TX de la classe ; en cas de saturation, le paquet est réinséré pour une tentative ultérieure.

## Hypothèses matérielles
- MCU STM32H743 en mode USB Host Full-Speed/High-Speed via OTG.
- ChibiOS fournit les primitives temps réel et un driver USBH adapté au contrôleur OTG.
- Les signaux VBUS/ID/OC et la configuration d'horloge OTG doivent être renseignés dans `usbh_platform_chibios_h7.c` (sections `TODO`).

## Limitations actuelles
- La couche bas niveau OTG est un gabarit compilable : aucun accès réel aux registres ni au driver ChibiOS n'est implémenté (URB simulées immédiatement DONE).
- Le parsing de descripteur MIDIStreaming est volontairement minimal (1 interface bulk IN/OUT) et devra être étendu pour plusieurs câbles/jacks.
- Pas encore de gestion avancée des SysEx longue durée côté wrapper (traité paquet par paquet).

## TODO / itérations futures
- Compléter `usbh_platform_chibios_h7.c` avec l'initialisation réelle du contrôleur OTG (clocks, GPIO, VBUS, interruptions) et l'appel au driver USBH ChibiOS.
- Étendre le parseur pour plusieurs endpoints et jacks externes/embeddés, avec mapping des câbles virtuels.
- Ajouter des statistiques détaillées et des hooks de monitoring (profiling des URB, taux de drop mailboxes).
- Gérer des APIs MIDI de plus haut niveau (NoteOn/Off, CC, SysEx streaming avec segmentation).
- Tester avec plusieurs périphériques USB-MIDI class-compliant et vérifier le comportement D-Cache STM32H7.
