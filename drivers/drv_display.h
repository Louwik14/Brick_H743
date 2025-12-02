#ifndef DRV_DISPLAY_H
#define DRV_DISPLAY_H

#include "ch.h"
#include "hal.h"
#include "brick_config.h"
#include <stdint.h>
#include "font.h"

/* Taille de l'écran issue de brick_config */
#define OLED_WIDTH   BRICK_OLED_WIDTH
#define OLED_HEIGHT  BRICK_OLED_HEIGHT

/* ====================================================================== */
/*                              API PUBLIQUE                              */
/* ====================================================================== */

void drv_display_init(void);
void drv_display_clear(void);
void drv_display_update(void);
uint8_t* drv_display_get_buffer(void);
void drv_display_start(void);

/* ====================================================================== */
/*                           GESTION DES POLICES                          */
/* ====================================================================== */

void drv_display_set_font(const font_t *font);

/* ====================================================================== */
/*                           DESSIN ET TEXTE                              */
/* ====================================================================== */

void drv_display_draw_char(uint8_t x, uint8_t y, char c);
void drv_display_draw_text(uint8_t x, uint8_t y, const char *txt);
void drv_display_draw_number(uint8_t x, uint8_t y, int num);

void drv_display_draw_text_with_font(const font_t *font,
                                     uint8_t x, uint8_t y,
                                     const char *txt);

void drv_display_draw_text_at_baseline(const font_t *font,
                                       uint8_t x, uint8_t baseline_y,
                                       const char *txt);

void drv_display_draw_char_in_box(const font_t *font,
                                  uint8_t x, uint8_t y,
                                  uint8_t box_w, uint8_t box_h,
                                  char c);

#endif /* DRV_DISPLAY_H */
