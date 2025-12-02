#include "drv_leds_addr.h"
#include "ch.h"
#include "hal.h"
#include "stm32h7xx.h"

/* ================= CONFIG ================= */

#define TIM_WS              TIM8
#define TIM_WS_CH           2

#define WS_BDMA             BDMA_Channel0

#define TIMER_CLOCK         200000000U
#define WS_FREQ             800000U
#define PERIOD_TICKS        (TIMER_CLOCK / WS_FREQ)

#define DUTY_0              (PERIOD_TICKS * 3 / 10)
#define DUTY_1              (PERIOD_TICKS * 7 / 10)

#define LED_BITS_PER_LED    24
#define RESET_SLOTS         80

/* ================= BUFFERS ================= */

static uint16_t pwm_buffer[NUM_ADRESS_LEDS * LED_BITS_PER_LED + RESET_SLOTS];
static led_color_t led_buffer[NUM_ADRESS_LEDS];
led_state_t drv_leds_addr_state[NUM_ADRESS_LEDS];

/* ================= TIM8 INIT ================= */

static void ws_tim_init(void) {
    rccEnableTIM8(true);

    TIM_WS->PSC = 0;
    TIM_WS->ARR = PERIOD_TICKS - 1;
    TIM_WS->CCR2 = 0;

    TIM_WS->CCMR1 |= (6 << 12);      /* PWM mode 1 CH2 */
    TIM_WS->CCER  |= TIM_CCER_CC2E;
    TIM_WS->BDTR  |= TIM_BDTR_MOE;

    TIM_WS->DIER |= TIM_DIER_CC2DE; /* DMA request on CH2 */

    TIM_WS->CR1 |= TIM_CR1_CEN;
}

/* ================= BDMA INIT ================= */

static void ws_bdma_init(void) {
    rccEnableBDMA1(true);

    WS_BDMA->CCR = 0;
    WS_BDMA->CPAR  = (uint32_t)&TIM_WS->CCR2;
    WS_BDMA->CM0AR = (uint32_t)pwm_buffer;
    WS_BDMA->CNDTR = 0;

    WS_BDMA->CCR =
        BDMA_CCR_MINC  |      /* incr mémoire */
        BDMA_CCR_DIR   |      /* mem -> periph */
        BDMA_CCR_PSIZE_0 |    /* 16 bits */
        BDMA_CCR_MSIZE_0 |    /* 16 bits */
        BDMA_CCR_TCIE;
}

/* ================= WS2812 ENCODAGE ================= */

static void ws_prepare_buffer(void) {
    uint32_t idx = 0;

    for (uint32_t i = 0; i < NUM_ADRESS_LEDS; i++) {
        uint32_t grb =
            (led_buffer[i].g << 16) |
            (led_buffer[i].r << 8)  |
            (led_buffer[i].b);

        for (int bit = 23; bit >= 0; bit--) {
            pwm_buffer[idx++] =
                (grb & (1U << bit)) ? DUTY_1 : DUTY_0;
        }
    }

    for (uint32_t i = 0; i < RESET_SLOTS; i++) {
        pwm_buffer[idx++] = 0;
    }

    WS_BDMA->CNDTR = idx;
}

/* ================= API ================= */

void drv_leds_addr_init(void) {
    ws_tim_init();
    ws_bdma_init();
    drv_leds_addr_clear();
}

void drv_leds_addr_update(void) {
    ws_prepare_buffer();

    WS_BDMA->CCR &= ~BDMA_CCR_EN;
    WS_BDMA->CNDTR = sizeof(pwm_buffer) / sizeof(pwm_buffer[0]);
    WS_BDMA->CCR |= BDMA_CCR_EN;
}

void drv_leds_addr_set_rgb(int index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < 0 || index >= NUM_ADRESS_LEDS) return;
    led_buffer[index].r = r;
    led_buffer[index].g = g;
    led_buffer[index].b = b;
}

void drv_leds_addr_set_color(int index, led_color_t color) {
    if (index < 0 || index >= NUM_ADRESS_LEDS) return;
    led_buffer[index] = color;
}

void drv_leds_addr_clear(void) {
    for (int i = 0; i < NUM_ADRESS_LEDS; i++)
        led_buffer[i] = (led_color_t){0, 0, 0};
}

void drv_leds_addr_set(int index, led_color_t color, led_mode_t mode) {
    if (index < 0 || index >= NUM_ADRESS_LEDS) return;
    drv_leds_addr_state[index].color = color;
    drv_leds_addr_state[index].mode  = mode;
}

void drv_leds_addr_render(void) {
    static uint32_t tick = 0;
    tick++;

    drv_leds_addr_clear();

    for (int i = 0; i < NUM_ADRESS_LEDS; i++) {
        if (drv_leds_addr_state[i].mode == LED_MODE_ON)
            drv_leds_addr_set_color(i, drv_leds_addr_state[i].color);
    }

    drv_leds_addr_update();
}
