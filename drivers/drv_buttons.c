#include "drv_buttons.h"
#include "drivers.h"
#include "ch.h"
#include "hal.h"
#include "brick_config.h"

/* ====================================================================== */
/*      CONFIG SPI5 (STM32H743 – ChibiOS HAL dernière génération)         */
/* ====================================================================== */

/*
   Le 74HC165 échantillonne sur front montant → CPOL=0, CPHA=0
   SPI 8 bits, MSB first, master mode.
*/

static const SPIConfig spicfg_buttons = {
    .circular = false,
    .slave    = false,
    .data_cb  = NULL,
    .error_cb = NULL,

    /* ------------------------------------------------------------------
     * CFG1 :
     *  - Baudrate prescaler = /4  (~10 MHz si SPI kernel clock ≈ 40 MHz)
     *  - DSIZE = 7 -> 8 bits (DSIZE = N-1)
     * ------------------------------------------------------------------ */
    .cfg1 = SPI_CFG1_MBR_2 |        /* Prescaler */
            (7U << SPI_CFG1_DSIZE_Pos),  /* 8 bits */

    /* ------------------------------------------------------------------
     * CFG2 :
     *  - Master mode
     *  - CPOL = 0
     *  - CPHA = 0
     *  - MSB first
     *  - NSS logiciel
     * ------------------------------------------------------------------ */
    .cfg2 = SPI_CFG2_MASTER |
            (0U << SPI_CFG2_CPOL_Pos) |
            (0U << SPI_CFG2_CPHA_Pos) |
            (0U << SPI_CFG2_LSBFRST_Pos) |
            SPI_CFG2_SSOE
};


/* ====================================================================== */
/*                        STOCKAGE INTERNE                                */
/* ====================================================================== */

static volatile uint8_t  buttons_raw[3];
static volatile uint32_t buttons_mask = 0;

/* ====================================================================== */
/*                       LECTURE SHIFT REGISTER                           */
/* ====================================================================== */

/*
   IMPORTANT :
   - 74HC165 : pas de DMA → spiReceive() bloquant = SAFE sur H7
   - Aucun problème D-Cache : données manipulées UNIQUEMENT par CPU
*/

static void buttons_read_shiftreg(void) {

    uint8_t rx[3] = {0};

    chMtxLock(&spi5_mutex);

    /* CS actif bas */
    palClearLine(LINE_SPI5_CS_SR);

    /* lecture 3 octets = 24 bits */
    spiReceive(&SPID5, 3, rx);

    /* CS inactif */
    palSetLine(LINE_SPI5_CS_SR);

    chMtxUnlock(&spi5_mutex);

    buttons_raw[0] = rx[0];
    buttons_raw[1] = rx[1];
    buttons_raw[2] = rx[2];

    /* assemblage en 32 bits */
    uint32_t m =
         ((uint32_t)rx[0] <<  0) |
         ((uint32_t)rx[1] <<  8) |
         ((uint32_t)rx[2] << 16);

    buttons_mask = (m & 0x000FFFFF);   /* 20 bits utiles */
}

/* ====================================================================== */
/*                             THREAD                                      */
/* ====================================================================== */

static THD_WORKING_AREA(waButtons, 256);

static THD_FUNCTION(buttonThread, arg) {
    (void)arg;
    chRegSetThreadName("buttons");

    spiStart(&SPID5, &spicfg_buttons);

    while (true) {
        buttons_read_shiftreg();
        chThdSleepMilliseconds(2);  /* 500 Hz */
    }
}

/* ====================================================================== */
/*                             API PUBLIQUE                               */
/* ====================================================================== */

void drv_buttons_init(void) {

    /* CS shift-register */
    palSetLineMode(LINE_SPI5_CS_SR, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLine(LINE_SPI5_CS_SR);   /* repos = haut */
}

void drv_buttons_start(void) {
    drv_buttons_init();

    chThdCreateStatic(waButtons, sizeof(waButtons),
                      NORMALPRIO, buttonThread, NULL);
}

bool drv_button_get(uint8_t index) {
    if (index >= DRV_BUTTONS_COUNT)
        return false;

    return (buttons_mask >> index) & 1;
}

uint32_t drv_buttons_get_mask(void) {
    return buttons_mask;
}
