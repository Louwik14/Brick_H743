/**
 * @file drivers.h
 * @brief Gestion centralisée et abstraction de l’ensemble des drivers matériels du firmware Brick.
 * @details Ce module agit comme point d’entrée unique pour l’initialisation et la mise à jour
 * des différents sous-systèmes matériels : affichage OLED, LEDs adressables, boutons, encodeurs
 * et potentiomètres.
 *
 * Il permet aux couches supérieures (UI, moteur, etc.) d’accéder aux périphériques
 * sans dépendre directement de chaque driver individuel.
 *
 * @ingroup drivers
 */

#ifndef DRIVERS_H
#define DRIVERS_H

#include "ch.h"
#include "brick_config.h"
#include "hal.h"

/* === Sous-modules inclus === */
/**
 * @brief Drivers matériels composant le système Brick.
 */
#include "drv_display.h"
#include "drv_leds_addr.h"
#include "drv_buttons.h"
#include "drv_encoders.h"
#include "drv_pots.h"

/* ====================================================================== */
/*                              INTERFACE API                             */
/* ====================================================================== */

/**
 * @brief Initialise l’ensemble des drivers matériels.
 *
 * Appelée une seule fois au démarrage du firmware.
 * Configure et démarre tous les périphériques requis.
 */
void drivers_init_all(void);

/**
 * @brief Met à jour les drivers nécessitant un rafraîchissement périodique.
 *
 * Appelée dans la boucle principale ou un thread système
 * pour maintenir l’état des périphériques actifs (LEDs, écran, etc.).
 */
void drivers_update_all(void);

#endif /* DRIVERS_H */
