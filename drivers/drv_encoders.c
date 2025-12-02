#include "drv_encoders.h"
#include "ch.h"
#include "hal.h"

/* Dernières valeurs pour calcul de delta */
static int16_t last_val[4] = {0};

/* Pointeurs sur les registres timers (H7 = ok) */
static TIM_TypeDef *timers[4] = {
    TIM3,  /* ENC1 */
    TIM2,  /* ENC2 */
    TIM5,  /* ENC3 */
    TIM4   /* ENC4 */
};

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

    enable_rcc();

    encoder_tim_init(TIM3);
    encoder_tim_init(TIM2);
    encoder_tim_init(TIM5);
    encoder_tim_init(TIM4);

    for (int i = 0; i < 4; i++)
        last_val[i] = 0;
}

int16_t drv_encoder_get(encoder_id_t id) {
    return (int16_t)timers[id]->CNT;
}

void drv_encoder_reset(encoder_id_t id) {
    timers[id]->CNT = 0;
    last_val[id] = 0;
}

int16_t drv_encoder_get_delta(encoder_id_t id) {
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
