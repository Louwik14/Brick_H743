#include "drv_display.h"
#include "ch.h"
#include "hal.h"
#include "brick_config.h"
#include <string.h>
#include <stdio.h>

/* ====================================================================== */
/*                        CONFIGURATION MATÉRIELLE                        */
/* ====================================================================== */

/* SPI5 - Configuration compatible ChibiOS récent */
static const SPIConfig spicfg = {
    .circular = false,
    .slave    = false,
    .data_cb  = NULL,
    .error_cb = NULL,

    /* ~10–15 MHz selon ton clock SPI */
    .cfg1 = SPI_CFG1_MBR_2 |               /* Prescaler */
            SPI_CFG1_DSIZE_VALUE(7),      /* 8 bits */

    .cfg2 = SPI_CFG2_MASTER |
            SPI_CFG2_SSM    |
            SPI_CFG2_SSOE
};

/* ====================================================================== */
/*                             VARIABLES INTERNES                         */
/* ====================================================================== */

static uint8_t buffer[BRICK_OLED_WIDTH * BRICK_OLED_HEIGHT / 8];
static const font_t *current_font = NULL;

/* ====================================================================== */
/*                              UTILITAIRES GPIO                          */
/* ====================================================================== */

static inline void dc_cmd(void)  { palClearLine(LINE_SPI5_DC_OLED); }
static inline void dc_data(void) { palSetLine  (LINE_SPI5_DC_OLED); }

/* ====================================================================== */
/*                              UTILITAIRES SPI                           */
/* ====================================================================== */

static void send_cmd(uint8_t cmd) {
    dc_cmd();
    palClearLine(LINE_SPI5_CS_OLED);
    spiSend(&SPID5, 1, &cmd);
    palSetLine(LINE_SPI5_CS_OLED);
}

static void send_data(const uint8_t *data, size_t len) {
    dc_data();
    palClearLine(LINE_SPI5_CS_OLED);
    spiSend(&SPID5, len, data);
    palSetLine(LINE_SPI5_CS_OLED);
}

/* ====================================================================== */
/*                              FRAMEBUFFER                               */
/* ====================================================================== */

uint8_t* drv_display_get_buffer(void) {
    return buffer;
}

/* ====================================================================== */
/*                              PIXELS                                    */
/* ====================================================================== */

static inline void set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= BRICK_OLED_WIDTH || y < 0 || y >= BRICK_OLED_HEIGHT) return;

    const int index = x + (y >> 3) * BRICK_OLED_WIDTH;
    const uint8_t mask = (uint8_t)(1U << (y & 7));

    if (on) buffer[index] |=  mask;
    else    buffer[index] &= (uint8_t)~mask;
}

/* ====================================================================== */
/*                      INITIALISATION OLED                               */
/* ====================================================================== */

void drv_display_init(void) {

    /* Reset OLED */
    palClearLine(LINE_SPI5_RES_OLED);
    chThdSleepMilliseconds(10);
    palSetLine(LINE_SPI5_RES_OLED);
    chThdSleepMilliseconds(10);

    /* Démarrage SPI */
    spiStart(&SPID5, &spicfg);

    /* Séquence init SSD1309 / SSD1306 */
    send_cmd(0xAE);
    send_cmd(0xD5); send_cmd(0x80);
    send_cmd(0xA8); send_cmd(0x3F);
    send_cmd(0xD3); send_cmd(0x00);
    send_cmd(0x40);
    send_cmd(0x8D); send_cmd(0x14);
    send_cmd(0x20); send_cmd(0x02);
    send_cmd(0xA1);
    send_cmd(0xC8);
    send_cmd(0xDA); send_cmd(0x12);
    send_cmd(0x81); send_cmd(0x7F);
    send_cmd(0xD9); send_cmd(0xF1);
    send_cmd(0xDB); send_cmd(0x40);
    send_cmd(0xA4);
    send_cmd(0xA6);
    send_cmd(0x21); send_cmd(0x00); send_cmd(0x7F);
    send_cmd(0xAF);

    drv_display_clear();

    extern const font_t FONT_5X7;
    current_font = &FONT_5X7;
}

/* ====================================================================== */
/*                               CLEAR                                    */
/* ====================================================================== */

void drv_display_clear(void) {
    memset(buffer, 0x00, sizeof(buffer));
}

/* ====================================================================== */
/*                               UPDATE                                   */
/* ====================================================================== */

void drv_display_update(void) {

    for (uint8_t page = 0; page < 8; page++) {
        send_cmd(0xB0 + page);
        send_cmd(0x00);
        send_cmd(0x10);
        send_cmd(0x21); send_cmd(0x00); send_cmd(0x7F);
        send_data(&buffer[page * BRICK_OLED_WIDTH], BRICK_OLED_WIDTH);
    }
}

/* ====================================================================== */
/*                              TEXTE                                     */
/* ====================================================================== */

void drv_display_set_font(const font_t *font) {
    current_font = font;
}

static inline uint8_t font_advance(const font_t *f) {
    return (uint8_t)(f->width + f->spacing);
}

void drv_display_draw_char(uint8_t x, uint8_t y, char c) {

    if (!current_font) return;

    if ((uint8_t)c < current_font->first || (uint8_t)c > current_font->last)
        c = '?';

    for (uint8_t col = 0; col < current_font->width; col++) {
        uint8_t bits = current_font->get_col(c, col);
        for (uint8_t row = 0; row < current_font->height; row++) {
            if (bits & (1U << row))
                set_pixel(x + col, y + row, true);
        }
    }
}

void drv_display_draw_text(uint8_t x, uint8_t y, const char *txt) {

    if (!current_font || !txt) return;

    const uint8_t adv = font_advance(current_font);

    while (*txt && x < BRICK_OLED_WIDTH) {
        drv_display_draw_char(x, y, *txt++);
        x = (uint8_t)(x + adv);
    }
}

void drv_display_draw_text_with_font(const font_t *font,
                                     uint8_t x, uint8_t y,
                                     const char *txt) {

    if (!font || !txt) return;

    const font_t *save = current_font;
    current_font = font;
    drv_display_draw_text(x, y, txt);
    current_font = save;
}

void drv_display_draw_text_at_baseline(const font_t *font,
                                       uint8_t x, uint8_t baseline_y,
                                       const char *txt) {

    if (!font || !txt) return;

    const font_t *save = current_font;
    current_font = font;

    uint8_t y = (baseline_y >= font->height)
                ? (baseline_y - font->height)
                : 0;

    drv_display_draw_text(x, y, txt);
    current_font = save;
}

void drv_display_draw_number(uint8_t x, uint8_t y, int num) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d   ", num);
    drv_display_draw_text(x, y, buf);
}

/* ====================================================================== */
/*                         THREAD DE RAFRAÎCHISSEMENT                      */
/* ====================================================================== */

static THD_WORKING_AREA(waDisplay, 512);

static THD_FUNCTION(displayThread, arg) {
    (void)arg;
    chRegSetThreadName("Display");

    while (true) {
        drv_display_update();
        chThdSleepMilliseconds(33);
    }
}

void drv_display_start(void) {
    drv_display_init();
    chThdCreateStatic(waDisplay, sizeof(waDisplay),
                      NORMALPRIO, displayThread, NULL);
}
