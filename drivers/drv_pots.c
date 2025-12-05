#include "drv_pots.h"
#include "ch.h"
#include "hal.h"
#include "brick_config.h"
#include <stdbool.h>

/* ====================================================================== */
/*                        CONFIGURATION ADC (ADCv4 H743)                  */
/* ====================================================================== */

static adcsample_t adc_sample;

/* Compteur d'erreurs ADC (diagnostic interne) */
static uint32_t adc_error_count;

typedef enum {
    POTS_STOPPED = 0,
    POTS_RUNNING,
    POTS_FAULT
} pots_state_t;

static volatile pots_state_t pots_state = POTS_STOPPED;
static thread_t            *pots_thread;
static volatile bool        stop_requested;

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

static volatile uint16_t pots_raw[BRICK_POT_MUX_COUNT];
static uint16_t         pots_filt[BRICK_POT_MUX_COUNT];

/* ====================================================================== */
/*                         THREAD DE LECTURE                              */
/* ====================================================================== */

static THD_WORKING_AREA(waPotReader, 512);

static THD_FUNCTION(potReaderThread, arg) {
    (void)arg;

    adcStart(&ADCD1, NULL);

    while (!stop_requested) {

        for (uint8_t i = 0; i < BRICK_POT_MUX_COUNT; i++) {

            mux_select(i);
            chThdSleepMicroseconds(8);   /* Stabilisation MUX */

            msg_t ret = adcConvert(&ADCD1, &adcgrpcfg, &adc_sample, 1);
            if (ret != MSG_OK) {
                adc_error_count++;
                pots_state = POTS_FAULT;
                continue;
            }

            int32_t delta     = (int32_t)adc_sample - (int32_t)pots_filt[i];
            pots_filt[i]      = (uint16_t)(pots_filt[i] + (delta >> 3));
            pots_raw[i]       = pots_filt[i];
        }

        palClearLine(LINE_MUX_POT_S0);
        palClearLine(LINE_MUX_POT_S1);
        palClearLine(LINE_MUX_POT_S2);

        chThdSleepMilliseconds(5);
    }

    palClearLine(LINE_MUX_POT_S0);
    palClearLine(LINE_MUX_POT_S1);
    palClearLine(LINE_MUX_POT_S2);

    adcStop(&ADCD1);
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
    if (pots_state == POTS_RUNNING ||
        (pots_thread != NULL && chThdTerminatedX(pots_thread) == false)) {
        return;
    }

    stop_requested = false;
    drv_pots_init();

    pots_thread = chThdCreateStatic(waPotReader, sizeof(waPotReader),
                                    NORMALPRIO, potReaderThread, NULL);
    if (pots_thread != NULL) {
        pots_state = POTS_RUNNING;
    }
}

void drv_pots_stop(void) {
    if (pots_thread == NULL) {
        pots_state = POTS_STOPPED;
        return;
    }

    stop_requested = true;
    chThdWait(pots_thread);
    pots_thread   = NULL;
    pots_state    = POTS_STOPPED;
}

/* ====================================================================== */
/*                        ACCÈS AUX VALEURS                                */
/* ====================================================================== */

uint16_t drv_pots_get_raw(uint8_t index) {
    if (index >= BRICK_POT_MUX_COUNT)
        return 0;

    return pots_raw[index];
}

uint32_t drv_pots_get_error_count(void) {
    return adc_error_count;
}
