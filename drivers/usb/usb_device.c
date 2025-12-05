/**
 * @file usb_device.c
 * @brief Gestion du périphérique USB (démarrage, connexion, état).
 *
 * Ce module gère l’initialisation complète du périphérique USB OTG FS :
 * - Séquence de **reconnexion logicielle** pour forcer la ré-énumération par l’hôte.
 * - Démarrage du driver USB ChibiOS avec la configuration `usbcfg`.
 * - Désactivation de la détection VBUS (mode forcé “Device”) pour cartes Nucleo.
 * - Suivi de l’état de disponibilité de la couche MIDI USB.
 *
 * @ingroup drivers
 */

#include "ch.h"
#include "hal.h"
#include "usbcfg.h"          // Descripteurs et configuration USB
#include <stdbool.h>

#ifndef USB_FORCE_NOVBUS
#define USB_FORCE_NOVBUS  0
#endif

/* ====================================================================== */
/*                            VARIABLES EXTERNES                          */
/* ====================================================================== */

/** @brief Indique si la couche MIDI USB est prête à l’envoi.
 * Défini et mis à jour dans `usb/usbcfg.c`. */
extern volatile bool usb_midi_tx_ready;

/* ====================================================================== */
/*                              FONCTIONS                                 */
/* ====================================================================== */

/**
 * @brief Démarre et initialise le périphérique USB (mode Device).
 *
 * Effectue une séquence complète :
 * 1. **Déconnexion logicielle** du bus USB (simule un unplug).
 * 2. Délai de 1,5 s pour garantir la ré-énumération côté hôte.
 * 3. Démarrage du driver `USBD1` avec la configuration `usbcfg`.
 * 4. Forçage du **mode périphérique** (désactivation de la détection VBUS).
 * 5. Connexion du bus (activation du pull-up DP).
 *
 * @note Cette procédure est nécessaire sur certaines cartes Nucleo où
 *       la broche VBUS (PA9) n’est pas connectée au contrôleur OTG FS.
 */
void usb_device_start(void) {
    /* 1. Déconnexion logique pour forcer une ré-énumération */
    usbDisconnectBus(&USBD1);
    chThdSleepMilliseconds(1500);

    /* 2. Démarrage du driver avec la config USB du projet */
    usbStart(&USBD1, &usbcfg);

    /* 3. Forçage du mode "device" (désactivation VBUS sensing).
     *
     * Les bits GCCFG varient selon les familles STM32 / versions d’en-têtes :
     * - F4/F7 : NOVBUSSENS + VBUSBSEN/VBUSASEN
     * - H7    : VBDEN
     *
     * On ne dépend que des macros disponibles pour conserver la compatibilité.
     */
    if (USB_FORCE_NOVBUS) {
#if defined(USB_OTG_GCCFG_NOVBUSSENS)
        USBD1.otg->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
        USBD1.otg->GCCFG &= ~(USB_OTG_GCCFG_VBUSBSEN | USB_OTG_GCCFG_VBUSASEN);
#elif defined(USB_OTG_GCCFG_VBDEN)
        USBD1.otg->GCCFG &= ~USB_OTG_GCCFG_VBDEN;
#endif
    }

    /* 4. Connexion logique sur le bus */
    usbConnectBus(&USBD1);
}

/**
 * @brief Indique si le périphérique USB MIDI est actif et prêt à l’envoi.
 *
 * @return true si la couche USB MIDI est initialisée et disponible.
 */
bool usb_device_active(void) {
    return usb_midi_tx_ready;
}
