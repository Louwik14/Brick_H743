#include "drv_encoders.h"
#include <stdbool.h>
#include "ch.h"
#include "hal.h"

BRICK_STATIC_ASSERT(BRICK_NUM_ENCODERS == 4, encoder_timer_mapping_assumes_four_encoders);

/* Dernières valeurs pour calcul de delta */
static int16_t last_val[ENCODER_COUNT] = {0};

/* Pointeurs sur les registres timers (H7 = ok) */
static TIM_TypeDef *timers[ENCODER_COUNT] = {
    TIM3,  /* ENC1 */
    TIM2,  /* ENC2 */
    TIM5,  /* ENC3 */
    TIM4   /* ENC4 */
};

/* La configuration GPIO/Alternate Function des pins encodeurs est gérée
 * ailleurs (board.c / code d'initialisation GPIO dédié), ce driver
 * manipule uniquement les périphériques timers. */

static bool encoder_id_valid(encoder_id_t id) {
    return ((unsigned)id < ENCODER_COUNT);
}

static bool encoders_started = false;

/* --------------------------------------------------------------------- */
/*              Activation clocks RCC – STM32H743                        */
/* --------------------------------------------------------------------- */

static void enable_rcc(void) {
    rccEnableAPB1L(RCC_APB1LENR_TIM2EN, true);
    rccEnableAPB1L(RCC_APB1LENR_TIM3EN, true);
    rccEnableAPB1L(RCC_APB1LENR_TIM4EN, true);
    rccEnableAPB1L(RCC_APB1LENR_TIM5EN, true);
}

/* --------------------------------------------------------------------- */
/*              Initialisation encodeur STM32H743                         */
/* --------------------------------------------------------------------- */

static void encoder_tim_init(TIM_TypeDef *tim) {

    /* Arrêt timer */
    tim->CR1 = 0;
    tim->CNT = 0;
    tim->PSC = 0;
    tim->ARR = 0xFFFF;

    /* CC1/CC2 en entrée sur TI1/TI2 */
    tim->CCMR1 =
        (1U << 0) |   /* CC1S = 01 -> TI1 */
        (1U << 8);    /* CC2S = 01 -> TI2 */

    /* Aucun filtre, polarité normale */
    tim->CCER = 0;

    /* Mode encodeur 3 (x4) */
    tim->SMCR = (3U << TIM_SMCR_SMS_Pos);

    /* Reset flags */
    tim->SR = 0;

    /* Enable */
    tim->CR1 |= TIM_CR1_CEN;
}

/* --------------------------------------------------------------------- */
/*                          API PUBLIQUE                                 */
/* --------------------------------------------------------------------- */

void drv_encoders_start(void) {

    if (encoders_started)
        return;

    enable_rcc();

    encoder_tim_init(TIM3);
    encoder_tim_init(TIM2);
    encoder_tim_init(TIM5);
    encoder_tim_init(TIM4);

    for (int i = 0; i < ENCODER_COUNT; i++)
        last_val[i] = 0;

    encoders_started = true;
}

int16_t drv_encoder_get(encoder_id_t id) {
    if (!encoder_id_valid(id))
        return 0;

    return (int16_t)timers[id]->CNT;
}

void drv_encoder_reset(encoder_id_t id) {
    if (!encoder_id_valid(id))
        return;

    timers[id]->CNT = 0;
    last_val[id] = 0;
}

int16_t drv_encoder_get_delta(encoder_id_t id) {
    if (!encoder_id_valid(id))
        return 0;

    int16_t now   = (int16_t)timers[id]->CNT;
    int16_t delta = now - last_val[id];
    last_val[id]  = now;
    return delta;
}

/* Accélération simple */
int16_t drv_encoder_get_delta_accel(encoder_id_t id) {
    int16_t d = drv_encoder_get_delta(id);

    if (d > 5)       return d * 4;
    else if (d > 2)  return d * 2;
    else if (d < -5) return d * 4;
    else if (d < -2) return d * 2;
    else             return d;
}
