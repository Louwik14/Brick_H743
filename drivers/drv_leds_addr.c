#include "drv_leds_addr.h"
#include "ch.h"
#include "hal.h"
#include "stm32h7xx.h"

/* ================= CONFIG ================= */

#define TIM_WS              TIM8
#define TIM_WS_CH           2

#define WS_BDMA             BDMA1_Channel0

#ifndef STM32_BDMA1_CH0_PRIORITY
#define STM32_BDMA1_CH0_PRIORITY STM32_IRQ_EXTI0_PRIORITY
#endif

#ifndef BDMA_Channel0_IRQn
#define BDMA_Channel0_IRQn BDMA1_Channel0_IRQn
#endif

#define TIMER_CLOCK         200000000U
#define WS_FREQ             800000U
#define PERIOD_TICKS        (TIMER_CLOCK / WS_FREQ)

#define DUTY_0              (PERIOD_TICKS * 3 / 10)
#define DUTY_1              (PERIOD_TICKS * 7 / 10)

#define LED_BITS_PER_LED    24
#define RESET_SLOTS         80    /* 80 slots @1.25us = 100 µs de reset WS2812 */
#define LED_TOTAL_SLOTS     (NUM_ADRESS_LEDS * LED_BITS_PER_LED + RESET_SLOTS)
#define LED_DMA_BUFFER_ATTR __attribute__((section(".ram_d2"), aligned(32)))
#define LED_PWM_BUFFER_SIZE LED_TOTAL_SLOTS

/* ================= BUFFERS ================= */

static uint16_t LED_DMA_BUFFER_ATTR pwm_buffer[LED_PWM_BUFFER_SIZE];
static led_color_t led_buffer[NUM_ADRESS_LEDS];
led_state_t drv_leds_addr_state[NUM_ADRESS_LEDS];

static mutex_t leds_mutex;
static volatile bool led_dma_busy = false;
static volatile uint32_t led_dma_errors = 0;
static volatile uint32_t last_frame_time_us = 0;
static volatile systime_t last_frame_start = 0;

#if LED_TOTAL_SLOTS > (sizeof(pwm_buffer) / sizeof(pwm_buffer[0]))
#error "pwm_buffer too small for configured LED payload"
#endif

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

static inline void ws_tim_resync(void) {
    TIM_WS->EGR = TIM_EGR_UG;
    TIM_WS->CNT = 0;
}

/* ================= BDMA INIT ================= */

static void ws_bdma_init(void) {
    rccEnableBDMA1(true);

    BDMA1->IFCR = BDMA_ISR_GIF0;

    WS_BDMA->CCR = 0;
    WS_BDMA->CPAR  = (uint32_t)&TIM_WS->CCR2;
    WS_BDMA->CM0AR = (uint32_t)pwm_buffer;
    WS_BDMA->CNDTR = 0;

    WS_BDMA->CCR =
        BDMA_CCR_MINC  |      /* incr mémoire */
        BDMA_CCR_DIR   |      /* mem -> periph */
        BDMA_CCR_PSIZE_0 |    /* 16 bits */
        BDMA_CCR_MSIZE_0 |    /* 16 bits */
        BDMA_CCR_TCIE |
        BDMA_CCR_TEIE;

    nvicEnableVector(BDMA_Channel0_IRQn, STM32_BDMA1_CH0_PRIORITY);
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

    chDbgAssert(idx == LED_TOTAL_SLOTS, "PWM buffer length mismatch");
}

static inline void ws_dma_start_locked(void) {
    ws_tim_resync();

    BDMA1->IFCR = BDMA_ISR_GIF0 | BDMA_ISR_TEIF0 | BDMA_ISR_TCIF0;
    WS_BDMA->CCR &= ~BDMA_CCR_EN;

    WS_BDMA->CM0AR = (uint32_t)pwm_buffer;
    WS_BDMA->CNDTR = LED_TOTAL_SLOTS;

    const size_t pwm_bytes = LED_PWM_BUFFER_SIZE * sizeof(uint16_t);
    SCB_CleanDCache_by_Addr((uint32_t *)pwm_buffer, (int32_t)((pwm_bytes + 31U) & ~31U));

    led_dma_busy = true;
    last_frame_start = chVTGetSystemTimeX();
    WS_BDMA->CCR |= BDMA_CCR_EN;
}

static inline void ws_dma_restart_on_error_i(void) {
    BDMA1->IFCR = BDMA_ISR_GIF0 | BDMA_ISR_TEIF0 | BDMA_ISR_TCIF0;
    WS_BDMA->CCR &= ~BDMA_CCR_EN;

    ws_tim_resync();

    WS_BDMA->CM0AR = (uint32_t)pwm_buffer;
    WS_BDMA->CNDTR = LED_TOTAL_SLOTS;
    led_dma_busy = true;
    last_frame_start = chVTGetSystemTimeX();
    WS_BDMA->CCR |= BDMA_CCR_EN;
}

/* ================= API ================= */

void drv_leds_addr_init(void) {
    chMtxObjectInit(&leds_mutex);
    led_dma_busy = false;
    led_dma_errors = 0;
    last_frame_time_us = 0;
    last_frame_start = 0;

    ws_tim_init();
    ws_bdma_init();
    drv_leds_addr_clear();
}

void drv_leds_addr_update(void) {
    chMtxLock(&leds_mutex);
    if (led_dma_busy) {
        chMtxUnlock(&leds_mutex);
        return;
    }

    ws_prepare_buffer();
    ws_dma_start_locked();

    chMtxUnlock(&leds_mutex);
}

void drv_leds_addr_set_rgb(int index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < 0 || index >= NUM_ADRESS_LEDS) return;
    chMtxLock(&leds_mutex);
    led_buffer[index].r = r;
    led_buffer[index].g = g;
    led_buffer[index].b = b;
    chMtxUnlock(&leds_mutex);
}

void drv_leds_addr_set_color(int index, led_color_t color) {
    if (index < 0 || index >= NUM_ADRESS_LEDS) return;
    chMtxLock(&leds_mutex);
    led_buffer[index] = color;
    chMtxUnlock(&leds_mutex);
}

void drv_leds_addr_clear(void) {
    chMtxLock(&leds_mutex);
    for (int i = 0; i < NUM_ADRESS_LEDS; i++)
        led_buffer[i] = (led_color_t){0, 0, 0};
    chMtxUnlock(&leds_mutex);
}

void drv_leds_addr_set(int index, led_color_t color, led_mode_t mode) {
    if (index < 0 || index >= NUM_ADRESS_LEDS) return;
    chMtxLock(&leds_mutex);
    drv_leds_addr_state[index].color = color;
    drv_leds_addr_state[index].mode  = mode;
    chMtxUnlock(&leds_mutex);
}

void drv_leds_addr_render(void) {
    static uint32_t tick = 0;
    tick++;

    chMtxLock(&leds_mutex);

    if (led_dma_busy) {
        chMtxUnlock(&leds_mutex);
        return;
    }

    for (int i = 0; i < NUM_ADRESS_LEDS; i++)
        led_buffer[i] = (led_color_t){0, 0, 0};

    for (int i = 0; i < NUM_ADRESS_LEDS; i++) {
        bool on = false;
        switch (drv_leds_addr_state[i].mode) {
        case LED_MODE_ON:
            on = true;
            break;
        case LED_MODE_BLINK:
            on = ((tick >> 7) & 0x1U) != 0U;
            break;
        case LED_MODE_PLAYHEAD:
            on = ((tick & 0x1FU) == (uint32_t)i);
            break;
        case LED_MODE_OFF:
        default:
            on = false;
            break;
        }

        if (on) {
            led_buffer[i] = drv_leds_addr_state[i].color;
        }
    }

    ws_prepare_buffer();
    ws_dma_start_locked();

    chMtxUnlock(&leds_mutex);
}

OSAL_IRQ_HANDLER(BDMA_Channel0_IRQHandler) {
    OSAL_IRQ_PROLOGUE();

    uint32_t isr = BDMA1->ISR;

    if ((isr & BDMA_ISR_TEIF0) != 0U) {
        BDMA1->IFCR = BDMA_ISR_GIF0 | BDMA_ISR_TEIF0;
        chSysLockFromISR();
        led_dma_errors++;
        chSysUnlockFromISR();
        ws_dma_restart_on_error_i();
        OSAL_IRQ_EPILOGUE();
        return;
    }

    if ((isr & BDMA_ISR_TCIF0) != 0U) {
        BDMA1->IFCR = BDMA_ISR_GIF0 | BDMA_ISR_TCIF0;
        chSysLockFromISR();
        led_dma_busy = false;
        last_frame_time_us = (uint32_t)ST2US(chVTTimeElapsedSinceX(last_frame_start));
        chSysUnlockFromISR();
    }

    OSAL_IRQ_EPILOGUE();
}

bool drv_leds_addr_is_busy(void) {
    return led_dma_busy;
}

uint32_t drv_leds_addr_error_count(void) {
    return led_dma_errors;
}

uint32_t drv_leds_addr_last_frame_time_us(void) {
    return last_frame_time_us;
}
