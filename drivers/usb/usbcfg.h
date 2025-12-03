/**
 * @file usbcfg.h
 * @brief Déclarations publiques pour la configuration USB MIDI (ChibiOS).
 *
 * Ce module définit les constantes, variables globales et structures
 * nécessaires à la configuration de l’interface **USB MIDI Class-Compliant**.
 *
 * Il est associé au fichier `usbcfg.c`, qui implémente :
 * - les descripteurs USB (device/config/string),
 * - les callbacks d’événements USB,
 * - la configuration des endpoints IN/OUT pour le flux MIDI.
 *
 * Contraintes :
 * - À inclure uniquement dans les modules nécessitant l’accès aux définitions
 *   d’endpoints et à la configuration `USBConfig`.
 * - Pas d’appels bloquants dans les callbacks ISR définis dans `usbcfg.c`.
 *
 * @ingroup drivers
 */

#ifndef BRICK_USB_USBCFG_H
#define BRICK_USB_USBCFG_H

#include "hal.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================== */
/*                        DÉFINITIONS DES ENDPOINTS                       */
/* ====================================================================== */

/**
 * @name Endpoints USB MIDI
 * Définitions des numéros et taille de paquet pour l’interface MIDI FS.
 * @{
 */

/** @brief Endpoint de réception (Bulk OUT) — numéro logique 1. */
#define MIDI_EP_OUT   1U

/** @brief Endpoint d’émission (Bulk IN) — numéro logique 2. */
#define MIDI_EP_IN    2U

/** @brief Taille maximale de paquet pour les endpoints MIDI (Full Speed). */
#define MIDI_EP_SIZE  64U
/** @} */

/* ====================================================================== */
/*                            VARIABLES GLOBALES                          */
/* ====================================================================== */

/**
 * @brief Indique si la couche USB-MIDI est prête à transmettre des données.
 *
 * @details
 * - Mise à `true` lors de l’événement `USB_EVENT_CONFIGURED`.
 * - Repassée à `false` lors d’un `RESET`, `SUSPEND` ou `UNCONFIGURED`.
 * - Cette variable est modifiée dans les callbacks de `usbcfg.c`.
 */
extern volatile bool usb_midi_tx_ready;

/* ====================================================================== */
/*                         CONFIGURATION DU DRIVER                        */
/* ====================================================================== */

/**
 * @brief Structure de configuration du driver USB (ChibiOS).
 *
 * @details
 * Utilisée lors de l’initialisation :
 * @code
 *   usbStart(&USBD1, &usbcfg);
 * @endcode
 * Elle relie les callbacks définis dans `usbcfg.c` :
 * - `usb_event()` : gestion des événements du bus,
 * - `get_descriptor()` : sélection des descripteurs,
 * - `sof_handler()` : gestion du Start Of Frame (facultatif selon besoin).
 */
extern const USBConfig usbcfg;

#ifdef __cplusplus
}
#endif

#endif /* BRICK_USB_USBCFG_H */
