# USB MIDI Host Recap

## Vue d'ensemble
- **Librairie cœur** : `stm32-mw-usb-host-master` fournit le cœur USB Host et les callbacks utilisateur.
- **Classe MIDI** : `STM32_USB_Host_Library/Class/MIDI` implémente un driver USB-MIDI prêt production : une interface MIDIStreaming unique avec un bulk IN et un bulk OUT, buffers circulaires statiques, machines d'état RX/TX sans allocation dynamique.
- **Wrapper ChibiOS** : `usb_host/usb_host_midi.{c,h}` crée un thread dédié qui appelle en continu `USBH_Process()` toutes les ~1 ms, pousse la RX vers une mailbox, dépile la TX depuis une autre, et publie les états device/class via `device_attached` et `midi_ready`.
- **Portage bas niveau** : `usb_host/usbh_platform_chibios_h7.{c,h}` pilote réellement l'OTG FS du STM32H743 avec le driver HAL HCD : clocks RCC, GPIO DM/DP, IRQ OTG_FS, FIFO RX/TX, requêtes URB DMA/FIFO et GPIO VBUS physique.

## Flux d'initialisation
1. `usb_host_midi_init()` :
   - Initialise les mailboxes RX/TX.
   - Appelle `USBH_Init()` avec `USBH_UserProcess`, enregistre `USBH_MIDI_Class`, démarre le Host (`USBH_Start`).
   - Lance le thread `usb_host_midi_thread` (prio `NORMALPRIO+2`).
2. `USBH_LL_Init()` (portage STM32H7) :
   - Active les clocks OTG FS, SYSCFG et GPIO nécessaires.
   - Configure PA11/PA12 en AF10 OTG_FS, prépare la broche VBUS (macro `BOARD_USB_VBUS_PORT/PIN`).
   - Initialise `HCD_HandleTypeDef` (12 channels, SOF actif, PHY embarqué, vbus sensing désactivé car piloté par GPIO), dimensionne les FIFO RX/TX, assigne le handle au Host.
3. `USBH_LL_Start()` :
   - Alimente le connecteur via `USBH_LL_DriverVBUS()` (GPIO à l'état actif) puis démarre le contrôleur OTG.
4. Enumeration :
   - Les callbacks HAL (`HAL_HCD_Connect_Callback`, `HAL_HCD_HC_NotifyURBChange_Callback`, etc.) réinjectent les événements dans la stack Host, mettent à jour l'état URB et la taille réelle transférée.
5. `USBH_MIDI_InterfaceInit()` :
   - Sélectionne l'unique interface Audio/MIDIStreaming (classe 0x01 / sous-classe 0x03, alt 0).
   - Trouve un bulk IN et un bulk OUT, limite la taille de paquet à `USBH_MIDI_MAX_PACKET_SIZE`, ouvre les pipes et marque l'interface prête.

## Flux en fonctionnement
- **Thread wrapper** : boucle infinie avec `USBH_Process()`, puis `pump_rx_events()` et `pump_tx_events()`, sommeil 1 ms.
- **RX** : `pump_rx_events()` lit autant de paquets 4 octets que disponibles via `USBH_MIDI_ReadEvent()` et tente de les poster dans la mailbox RX. En cas de saturation, `rx_overflow` est incrémenté et les paquets supplémentaires sont abandonnés.
- **TX** : `usb_host_midi_send_event()` poste les paquets dans la mailbox TX (sinon `tx_overflow++`). `pump_tx_events()` dépile et injecte dans la ring TX de la classe ; un échec d'injection incrémente également `tx_overflow` pour tracer le drop.
- **URB** : `USBH_LL_SubmitURB()` programme réellement les transferts via `HAL_HCD_HC_SubmitRequest()`. Les états URB et tailles transférées sont mis à jour par `HAL_HCD_HC_NotifyURBChange_Callback()`.
- **VBUS** : `USBH_LL_DriverVBUS()` pilote une broche GPIO matérielle (macros `BOARD_USB_VBUS_*`) pour alimenter ou couper le périphérique. VBUS est activé au `Start`, coupé au `Stop` et lors des déconnexions.
- **Synchronisation état** : `USBH_UserProcess()` met à jour `device_attached` (connexion) et `midi_ready` (classe active). En déconnexion, les mailboxes sont réinitialisées et `reset_events` est incrémenté.

## API côté application
- `usb_host_midi_init()` : démarre le host et le thread wrapper.
- `usb_host_midi_is_device_attached()` : vrai quand un périphérique est détecté.
- `usb_host_midi_is_ready()` : vrai quand la classe est active et les pipes configurés.
- `usb_host_midi_fetch_event(uint8_t packet[4])` : lecture non bloquante d'un paquet USB-MIDI depuis la mailbox RX.
- `usb_host_midi_send_event(const uint8_t packet[4])` : poste un paquet vers la mailbox TX (drop + compteur si pleine).
- `usb_host_midi_register_attach_callback()` / `usb_host_midi_register_detach_callback()` : callbacks app.
- Helpers classe : `USBH_MIDI_EncodeShortMessage()` pour formatter Note/CC/Clock/Start/Stop/Program/PitchBend/Aftertouch en paquet 4 octets.

## Paramètres configurables
- Tailles des ring buffers MIDI : macros `USBH_MIDI_RX_EVENT_BUFFER`, `USBH_MIDI_TX_EVENT_BUFFER`, `USBH_MIDI_MAX_PACKET_SIZE` dans `usbh_midi.h`.
- Tailles des mailboxes wrapper : constantes en tête de `usb_host_midi.c` (`USB_HOST_MIDI_RX_MAILBOX_SIZE`, `USB_HOST_MIDI_TX_MAILBOX_SIZE`).
- Matériel VBUS : `BOARD_USB_VBUS_PORT`, `BOARD_USB_VBUS_PIN`, `BOARD_USB_VBUS_ACTIVE_STATE` / `BOARD_USB_VBUS_INACTIVE_STATE` dans `usbh_platform_chibios_h7.h`.

## Séquence de test terrain
1. Alimenter la carte STM32H743 (D-Cache actif) et vérifier que le firmware démarre ChibiOS.
2. `usb_host_midi_init()` est appelé au boot ; vérifier que VBUS reste coupé tant que le Host n'est pas lancé.
3. Brancher un clavier USB-MIDI class-compliant sur le port OTG FS.
4. Observer l'énumération : la broche VBUS passe à l'état actif, l'IRQ OTG_FS génère `HOST_USER_CLASS_ACTIVE`, `usb_host_midi_is_ready()` devient vrai.
5. Appuyer sur quelques notes : `usb_host_midi_fetch_event()` retourne des paquets 4 octets (CIN/status/données), `rx_overflow` reste à zéro sur une consommation régulière.
6. Envoyer des notes depuis l'application via `usb_host_midi_send_event()` ; vérifier qu'elles partent réellement sur le bus (LED clavier ou capture MIDI) et que `tx_overflow` ne monte pas en conditions nominales.
7. Débrancher/rebrancher à chaud : `reset_events` s'incrémente, les mailboxes sont vidées, `midi_ready` repasse à vrai après re-énumération.

## Hypothèses matérielles
- OTG FS utilisé par défaut (PHY embarqué, 12 channels). Pour OTG HS, définir `USBH_USE_HS_PORT` et adapter l'horloge/pinout.
- La broche VBUS du connecteur est pilotée par un interrupteur 5V commandé via `BOARD_USB_VBUS_PORT/PIN` (niveau actif configurable).
- ChibiOS fournit `hal.h` et le support du HAL STM32H7 ; `HAL_PWREx_EnableUSBVoltageDetector()` est disponible.

## Limitations actuelles
- Pas de support SysEx longue durée ni multi-câbles : une seule interface MIDIStreaming avec un bulk IN/OUT.
- L'horloge et le pinout OTG HS ne sont pas configurés par défaut ; le port FS est utilisé pour la production actuelle.
- Les compteurs de débordement (mailboxes) ne sont pas exposés par API publique mais peuvent être instrumentés si nécessaire.
