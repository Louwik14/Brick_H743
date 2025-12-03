/**
 * @file usb_device.h
 * @brief Interface de gestion du périphérique USB (OTG FS) — mode Device.
 *
 * Ce module fournit une interface minimale pour contrôler la couche USB :
 * - Démarrage et ré-énumération forcée du périphérique USB.
 * - Vérification de l’état de disponibilité du lien **USB MIDI**.
 *
 * Contraintes :
 * - À appeler **après** `halInit()` / `chSysInit()`.
 * - Les callbacks (configuration endpoints, événements bus) sont définis dans `usbcfg.c`.
 *
 * @ingroup drivers
 */

#ifndef BRICK_USB_DEVICE_H
#define BRICK_USB_DEVICE_H

#include "hal.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Démarre la pile USB Device et force la ré-énumération.
 *
 * Séquence typique implémentée côté `usb_device.c` :
 * - `usbDisconnectBus()` pour forcer la reconnexion hôte,
 * - temporisation courte,
 * - `usbStart(&USBD1, &usbcfg)`,
 * - configuration de l’option `NOVBUSSENS` si nécessaire,
 * - `usbConnectBus()` pour activer le pull-up DP.
 *
 * @note Indispensable sur certaines Nucleo où **VBUS** n’est pas câblé.
 */
void usb_device_start(void);

/**
 * @brief Indique si le périphérique **USB MIDI** est actif et prêt à transmettre.
 *
 * @return `true` si la couche USB MIDI est configurée et l’endpoint IN prêt, sinon `false`.
 */
bool usb_device_active(void);

#ifdef __cplusplus
}
#endif

#endif /* BRICK_USB_DEVICE_H */
