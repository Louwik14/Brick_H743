#include "drv_pots.h"
#include "ch.h"
#include "hal.h"
#include "brick_config.h"

/* ====================================================================== */
/*                        CONFIGURATION ADC (ADCv4 H743)                  */
/* ====================================================================== */

static adcsample_t adc_sample;

/*
 * Groupe de conversion ADC — STM32H743 / ChibiOS ADCv4
 * 1 seul canal lu à la fois via le MUX
 * Canal utilisé : ADC1_IN15  (PH5)
 */
static const ADCConversionGroup adcgrpcfg = {
    .circular     = FALSE,
    .num_channels = 1,
    .end_cb       = NULL,
    .error_cb     = NULL,

    /* CFGR : on laisse par défaut (trigger logiciel, right aligned) */
    .cfgr  = 0,
    .cfgr2 = 0,

    /* Temps d'échantillonnage : canal 15 → SMPR2 */
    .smpr = {
        0,
        (ADC_SMPR_SMP_64P5 << ADC_SMPR2_SMP15_Pos)
    },

    /* Séquence régulière : SQ1 = canal 15 */
    .sqr = {
        ADC_SQR1_SQ1_N(15),
        0,
        0,
        0
    }
};

/* ====================================================================== */
/*                           MUX DE SÉLECTION                             */
/* ====================================================================== */

static inline void mux_select(uint8_t index) {
    palWriteLine(LINE_MUX_POT_S0, (index >> 0) & 1U);
    palWriteLine(LINE_MUX_POT_S1, (index >> 1) & 1U);
    palWriteLine(LINE_MUX_POT_S2, (index >> 2) & 1U);
}

/* ====================================================================== */
/*                         STOCKAGE INTERNE                               */
/* ====================================================================== */

static uint16_t pots_raw[BRICK_POT_MUX_COUNT];

/* ====================================================================== */
/*                         THREAD DE LECTURE                              */
/* ====================================================================== */

static THD_WORKING_AREA(waPotReader, 512);

static THD_FUNCTION(potReaderThread, arg) {
    (void)arg;

    adcStart(&ADCD1, NULL);

    while (true) {

        for (uint8_t i = 0; i < BRICK_POT_MUX_COUNT; i++) {

            mux_select(i);
            chThdSleepMicroseconds(8);   /* Stabilisation MUX */

            adcConvert(&ADCD1, &adcgrpcfg, &adc_sample, 1);
            pots_raw[i] = adc_sample;
        }

        chThdSleepMilliseconds(5);
    }
}

/* ====================================================================== */
/*                           INITIALISATION                               */
/* ====================================================================== */

void drv_pots_init(void) {

    /* Entrée ADC */
    palSetLineMode(LINE_MUX_POT_ADC, PAL_MODE_INPUT_ANALOG);

    /* Lignes de sélection MUX */
    palSetLineMode(LINE_MUX_POT_S0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(LINE_MUX_POT_S1, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(LINE_MUX_POT_S2, PAL_MODE_OUTPUT_PUSHPULL);

    palClearLine(LINE_MUX_POT_S0);
    palClearLine(LINE_MUX_POT_S1);
    palClearLine(LINE_MUX_POT_S2);
}

void drv_pots_start(void) {
    drv_pots_init();
    chThdCreateStatic(waPotReader, sizeof(waPotReader),
                      NORMALPRIO, potReaderThread, NULL);
}

/* ====================================================================== */
/*                        ACCÈS AUX VALEURS                                */
/* ====================================================================== */

uint16_t drv_pots_get_raw(uint8_t index) {
    if (index >= BRICK_POT_MUX_COUNT)
        return 0;

    return pots_raw[index];
}
