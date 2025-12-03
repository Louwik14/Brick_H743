#ifndef DRV_LEDS_ADDR_H
#define DRV_LEDS_ADDR_H

#include "ch.h"
#include "brick_config.h"
#include "hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Nombre total de LEDs adressables */
#define NUM_ADRESS_LEDS   BRICK_NUM_WS2812_LEDS

/* =======================================================================
 *                              TYPES & COULEURS
 * ======================================================================= */

typedef struct {
    uint8_t g;  /* vert */
    uint8_t r;  /* rouge */
    uint8_t b;  /* bleu  */
} led_color_t;

/* Couleurs prédéfinies (GRB) */
#define COLOR_RED       (led_color_t){0, 255, 0}
#define COLOR_GREEN     (led_color_t){255, 0, 0}
#define COLOR_BLUE      (led_color_t){0, 0, 255}
#define COLOR_YELLOW    (led_color_t){255, 255, 0}
#define COLOR_CYAN      (led_color_t){255, 0, 255}
#define COLOR_MAGENTA   (led_color_t){0, 255, 255}
#define COLOR_WHITE     (led_color_t){255, 255, 255}

#define COLOR_ORANGE    (led_color_t){128, 255, 0}
#define COLOR_PINK      (led_color_t){20, 255, 127}
#define COLOR_PURPLE    (led_color_t){0, 128, 255}
#define COLOR_TURQUOISE (led_color_t){255, 64, 128}
#define COLOR_OFF       (led_color_t){0, 0, 0}

/* =======================================================================
 *                              MAPPAGE LOGIQUE
 * ======================================================================= */

/* Mapping LEDs → boutons physiques Brick (déjà existants) */
#define LED_REC        0
#define LED_SEQ8       1
#define LED_SEQ7       2
#define LED_SEQ6       3
#define LED_SEQ5       4
#define LED_SEQ4       5
#define LED_SEQ3       6
#define LED_SEQ2       7
#define LED_SEQ1       8
#define LED_SEQ9       9
#define LED_SEQ10      10
#define LED_SEQ11      11
#define LED_SEQ12      12
#define LED_SEQ13      13
#define LED_SEQ14      14
#define LED_SEQ15      15
#define LED_SEQ16      16

/* Nouvelles LEDs (17 à 24) */
#define LED_MENU1      17
#define LED_MENU2      18
#define LED_MENU3      19
#define LED_MENU4      20
#define LED_MENU5      21
#define LED_MENU6      22
#define LED_MENU7      23
#define LED_MENU8      24

/* =======================================================================
 *                              MODES LOGIQUES
 * ======================================================================= */

typedef enum {
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK,
    LED_MODE_PLAYHEAD
} led_mode_t;

typedef struct {
    led_color_t color;
    led_mode_t  mode;
} led_state_t;

/* État logique global */
extern led_state_t drv_leds_addr_state[NUM_ADRESS_LEDS];

/* =======================================================================
 *                              API
 * ======================================================================= */

/** Driver init (mutex, timer, BDMA). */
void drv_leds_addr_init(void);

/**
 * Encode le buffer LED et lance un transfert DMA non bloquant.
 * Protégé par mutex + flag busy pour éviter les re-entrées.
 */
void drv_leds_addr_update(void);

/** Accès thread-safe au buffer GRB logique. */
void drv_leds_addr_set_rgb(int index, uint8_t r, uint8_t g, uint8_t b);
void drv_leds_addr_set_color(int index, led_color_t color);
void drv_leds_addr_clear(void);

/**
 * Met à jour l'état logique (mode + couleur) avec protection mutex.
 */
void drv_leds_addr_set(int index, led_color_t color, led_mode_t mode);

/**
 * Rendu complet (modes, blink) + déclenchement DMA si libre.
 */
void drv_leds_addr_render(void);

/* Diagnostic temps réel, toutes thread-safe / non bloquantes. */
bool drv_leds_addr_is_busy(void);
uint32_t drv_leds_addr_error_count(void);
uint32_t drv_leds_addr_last_frame_time_us(void);

#endif /* DRV_LEDS_ADDR_H */
