#include "drv_leds_addr.h"
#include "ch.h"
#include "hal.h"
#include "stm32h7xx.h"

/* ================= CONFIG ================= */

#define TIM_WS              TIM8
#define TIM_WS_CH           2

#define WS_DMA_STREAM_ID    STM32_DMA_STREAM_ID(2, 1)
#define WS_DMA_PRIORITY     13  /* Bas dans la hiérarchie : audio/SPI au-dessus */

#define WS_DMA_WORKER_STACK_SIZE THD_WORKING_AREA_SIZE(256)
#define LED_DMA_FORCE_DCACHE_CLEAN 0

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
static led_state_t drv_leds_addr_state[NUM_ADRESS_LEDS];

static mutex_t leds_mutex;
static volatile bool led_dma_busy = false;
static volatile uint32_t led_dma_errors = 0;
static volatile uint32_t last_frame_time_us = 0;
static volatile systime_t last_frame_start = 0;
static volatile bool led_dma_tc_pending = false;
static volatile bool led_dma_error_pending = false;

static binary_semaphore_t led_dma_sem;
static THD_WORKING_AREA(led_dma_thread_wa, WS_DMA_WORKER_STACK_SIZE);
static THD_FUNCTION(led_dma_thread, arg);

_Static_assert((LED_TOTAL_SLOTS <= (sizeof(pwm_buffer) / sizeof(pwm_buffer[0]))),
               "pwm_buffer too small for configured LED payload");

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

/* ================= DMA STATE ================= */

static const stm32_dma_stream_t *ws_dma_stream = NULL;
static uint32_t ws_dma_mode = 0U;
static void ws_dma_prepare_stream(void);
static void ws_dma_callback(void *p, uint32_t flags);

static void ws_dma_init(void) {
    ws_dma_stream = dmaStreamAlloc(WS_DMA_STREAM_ID,
                                   WS_DMA_PRIORITY,
                                   ws_dma_callback,
                                   NULL);
    chDbgAssert(ws_dma_stream != NULL, "WS DMA alloc failed");

#if STM32_DMA_SUPPORTS_DMAMUX == TRUE
    dmaSetRequestSource(ws_dma_stream, STM32_DMAMUX1_TIM8_CH2);
#endif

    ws_dma_prepare_stream();
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

static void ws_dma_prepare_stream(void) {
    ws_dma_mode = STM32_DMA_CR_MINC |
                  STM32_DMA_CR_DIR_M2P |
                  STM32_DMA_CR_PSIZE_HWORD |
                  STM32_DMA_CR_MSIZE_HWORD |
                  STM32_DMA_CR_TCIE |
                  STM32_DMA_CR_TEIE |
                  STM32_DMA_CR_DMEIE;

    dmaStreamSetPeripheral(ws_dma_stream, &TIM_WS->CCR2);
}

static inline void ws_dma_start_locked(void) {
    ws_tim_resync();

    dmaStreamDisable(ws_dma_stream);
    dmaStreamSetMemory0(ws_dma_stream, pwm_buffer);
    dmaStreamSetTransactionSize(ws_dma_stream, LED_TOTAL_SLOTS);
    dmaStreamSetMode(ws_dma_stream, ws_dma_mode);

#if LED_DMA_FORCE_DCACHE_CLEAN
    const size_t pwm_bytes = LED_PWM_BUFFER_SIZE * sizeof(uint16_t);
    /* pwm_buffer est en .ram_d2 non-cacheable via MPU, ce clean est généralement inutile. */
    SCB_CleanDCache_by_Addr((uint32_t *)pwm_buffer, (int32_t)((pwm_bytes + 31U) & ~31U));
#endif

    led_dma_busy = true;
    last_frame_start = chVTGetSystemTimeX();
    dmaStreamEnable(ws_dma_stream);
}

/* ================= API ================= */

void drv_leds_addr_init(void) {
    chMtxObjectInit(&leds_mutex);
    led_dma_busy = false;
    led_dma_errors = 0;
    last_frame_time_us = 0;
    last_frame_start = 0;
    led_dma_tc_pending = false;
    led_dma_error_pending = false;

    chBSemObjectInit(&led_dma_sem, true);

    ws_tim_init();
    ws_dma_init();
    drv_leds_addr_clear();

    chThdCreateStatic(led_dma_thread_wa,
                      WS_DMA_WORKER_STACK_SIZE,
                      NORMALPRIO,
                      led_dma_thread,
                      NULL);
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

static void led_dma_process_events(bool error_pending,
                                   bool tc_pending,
                                   systime_t frame_start_snapshot) {
    if (tc_pending) {
        last_frame_time_us = TIME_I2US(chVTTimeElapsedSinceX(frame_start_snapshot));
    }

    if (error_pending) {
        chMtxLock(&leds_mutex);
        if (!led_dma_busy) {
            ws_dma_start_locked();
        }
        chMtxUnlock(&leds_mutex);
    }
}

static THD_FUNCTION(led_dma_thread, arg) {
    (void)arg;
    chRegSetThreadName("led-dma");

    while (true) {
        chBSemWait(&led_dma_sem);

        bool error_pending;
        bool tc_pending;
        systime_t frame_start_snapshot;

        chSysLock();
        error_pending = led_dma_error_pending;
        tc_pending = led_dma_tc_pending;
        led_dma_error_pending = false;
        led_dma_tc_pending = false;
        frame_start_snapshot = last_frame_start;
        chSysUnlock();

        led_dma_process_events(error_pending, tc_pending, frame_start_snapshot);
    }
}

static void ws_dma_callback(void *p, uint32_t flags) {
    (void)p;

    const uint32_t error_flags = STM32_DMA_ISR_TEIF |
                                 STM32_DMA_ISR_DMEIF |
                                 STM32_DMA_ISR_FEIF;

    if ((flags & error_flags) != 0U) {
        chSysLockFromISR();
        led_dma_errors++;
        led_dma_busy = false;
        led_dma_error_pending = true;
        chBSemSignalI(&led_dma_sem);
        chSysUnlockFromISR();
        return;
    }

    if ((flags & STM32_DMA_ISR_TCIF) != 0U) {
        chSysLockFromISR();
        led_dma_busy = false;
        led_dma_tc_pending = true;
        chBSemSignalI(&led_dma_sem);
        chSysUnlockFromISR();
    }
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
