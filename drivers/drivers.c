/**
 * @file drivers.c
 * @brief Point d’entrée unique pour l’initialisation de tous les drivers matériels du firmware.
 * @ingroup drivers
 */

#include "drivers.h"

mutex_t spi5_mutex;

void drivers_init_all(void) {
    chMtxObjectInit(&spi5_mutex);

    drv_display_init();
    drv_leds_addr_init();
    drv_buttons_start();
    drv_encoders_start();
    drv_pots_start();
}

/* Mise à jour périodique : surtout pour l’écran (les LEDs sont rendues via ui_led_backend_refresh). */
void drivers_update_all(void) {
    /* ⚠️ Ne **pas** appeler drv_leds_addr_update() ici.
       Le pipeline LED passe par ui_led_backend_refresh() → drv_leds_addr_render(). */
    drv_display_update();
}
