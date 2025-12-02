#include "drv_hall.h"

#include "brick_asc.h"
#include "brick_cal.h"

#include "ch.h"
#include "hal.h"

#include <limits.h>
#include <string.h>

#define HALL_ADC_RESOLUTION       12U
#define HALL_FILTER_FACTOR        1U
#define HALL_THRESHOLD_RATIO      0.5f
#define HALL_HYSTERESIS_RATIO     0.2f
#define HALL_DETENT_RATIO         0.05f
#define HALL_SETTLE_US            8U
#define HALL_NORMALIZE_MAX        127U

#define HALL_LINE_ADC1            PAL_LINE(GPIOC, 4U)
#define HALL_LINE_ADC2            PAL_LINE(GPIOA, 7U)
#define HALL_LINE_MUX_S0          PAL_LINE(GPIOA, 5U)
#define HALL_LINE_MUX_S1          PAL_LINE(GPIOA, 4U)
#define HALL_LINE_MUX_S2          PAL_LINE(GPIOA, 6U)

static const ADCConversionGroup adcgrpcfg1 = {
    .circular     = FALSE,
    .num_channels = 1,
    .end_cb       = NULL,
    .error_cb     = NULL,
    .cfgr         = 0,
    .cfgr2        = 0,
    .smpr         = {
        (ADC_SMPR_SMP_64P5 << ADC_SMPR1_SMP4_Pos),
        0
    },
    .sqr          = {
        ADC_SQR1_SQ1_N(4),
        0,
        0,
        0
    }
};

static const ADCConversionGroup adcgrpcfg2 = {
    .circular     = FALSE,
    .num_channels = 1,
    .end_cb       = NULL,
    .error_cb     = NULL,
    .cfgr         = 0,
    .cfgr2        = 0,
    .smpr         = {
        (ADC_SMPR_SMP_64P5 << ADC_SMPR1_SMP7_Pos),
        0
    },
    .sqr          = {
        ADC_SQR1_SQ1_N(7),
        0,
        0,
        0
    }
};

static adcsample_t adc_sample1;
static adcsample_t adc_sample2;

static hall_state_t hall_state[BRICK_NUM_HALL_SENSORS];
static struct brick_asc asc_state[BRICK_NUM_HALL_SENSORS];
static struct brick_cal_pot cal_state;

static inline void mux_select(uint8_t index) {
    palWriteLine(HALL_LINE_MUX_S0, (index >> 0) & 1U);
    palWriteLine(HALL_LINE_MUX_S1, (index >> 1) & 1U);
    palWriteLine(HALL_LINE_MUX_S2, (index >> 2) & 1U);
}

static inline uint8_t clamp_uint8(int32_t v) {
    if (v < 0) {
        return 0;
    }
    if (v > HALL_NORMALIZE_MAX) {
        return HALL_NORMALIZE_MAX;
    }
    return (uint8_t)v;
}

static void hall_state_init(void) {
    memset(hall_state, 0, sizeof(hall_state));
    memset(asc_state, 0, sizeof(asc_state));

    for (uint8_t i = 0; i < BRICK_NUM_HALL_SENSORS; ++i) {
        hall_state[i].min = UINT16_MAX;
        hall_state[i].max = 0;
        hall_state[i].threshold = 0;
        hall_state[i].hysteresis = 0;
    }

    brick_asc_array_set_factors(asc_state, BRICK_NUM_HALL_SENSORS, 0, BRICK_NUM_HALL_SENSORS, HALL_FILTER_FACTOR);
    brick_cal_pot_init(&cal_state, HALL_ADC_RESOLUTION, BRICK_NUM_HALL_SENSORS);
    brick_cal_pot_enable_range(&cal_state, 0, BRICK_NUM_HALL_SENSORS);
}

static uint16_t apply_deadzone(uint16_t value, uint16_t min, uint16_t max) {
    uint16_t center = (uint16_t)((min + max) / 2);
    uint16_t range = (uint16_t)(max - min);
    uint16_t detent = (uint16_t)(range * HALL_DETENT_RATIO);

    uint16_t lo = (center > detent / 2) ? (center - detent / 2) : min;
    uint16_t hi = center + detent / 2;
    if (hi > max) {
        hi = max;
    }

    if (value >= lo && value <= hi) {
        return center;
    }

    return value;
}

static void update_state(uint8_t index, uint16_t raw_value) {
    hall_state_t* st = &hall_state[index];
    uint16_t calibrated = 0;
    brick_cal_pot_next(&cal_state, index, raw_value, &calibrated);

    st->raw = calibrated;
    st->filtered = calibrated;

    if (calibrated < st->min) {
        st->min = calibrated;
    }
    if (calibrated > st->max) {
        st->max = calibrated;
    }

    uint16_t range = st->max - st->min;
    if (range == 0) {
        range = 1;
    }

    uint16_t trig_lo = st->min + (uint32_t)(range * (HALL_THRESHOLD_RATIO - HALL_HYSTERESIS_RATIO / 2));
    uint16_t trig_hi = st->min + (uint32_t)(range * (HALL_THRESHOLD_RATIO + HALL_HYSTERESIS_RATIO / 2));

    uint16_t dz_value = apply_deadzone(calibrated, st->min, st->max);

    int32_t normalized = ((int32_t)(st->max - dz_value) * HALL_NORMALIZE_MAX) / range;
    st->value = clamp_uint8(normalized);

    systime_t now = chVTGetSystemTimeX();
    uint32_t dt = ST2US(chVTTimeElapsedSinceX(st->last_time));
    if (dt == 0) {
        dt = 1;
    }

    int32_t delta = (int32_t)st->last_raw - (int32_t)calibrated;
    int32_t speed = (delta * 1000) / (int32_t)dt;
    if (speed < 0) {
        speed = 0;
    }
    if (speed > 256) {
        speed = 256;
    }
    st->velocity = clamp_uint8(speed * HALL_NORMALIZE_MAX / 256);

    if (calibrated <= trig_lo) {
        st->pressed = true;
    } else if (calibrated >= trig_hi) {
        st->pressed = false;
    }

    st->threshold = trig_lo;
    st->hysteresis = trig_hi;
    st->last_raw = calibrated;
    st->last_time = now;
}

void drv_hall_init(void) {
    palSetLineMode(HALL_LINE_ADC1, PAL_MODE_INPUT_ANALOG);
    palSetLineMode(HALL_LINE_ADC2, PAL_MODE_INPUT_ANALOG);

    palSetLineMode(HALL_LINE_MUX_S0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(HALL_LINE_MUX_S1, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(HALL_LINE_MUX_S2, PAL_MODE_OUTPUT_PUSHPULL);

    palClearLine(HALL_LINE_MUX_S0);
    palClearLine(HALL_LINE_MUX_S1);
    palClearLine(HALL_LINE_MUX_S2);

    adcStart(&ADCD1, NULL);
    adcStart(&ADCD2, NULL);

    hall_state_init();
}

void drv_hall_task(void) {
    for (uint8_t mux = 0; mux < BRICK_HALL_MUX_CHANNELS; ++mux) {
        mux_select(mux);
        chThdSleepMicroseconds(HALL_SETTLE_US);

        adcConvert(&ADCD1, &adcgrpcfg1, &adc_sample1, 1);
        adcConvert(&ADCD2, &adcgrpcfg2, &adc_sample2, 1);

        uint16_t filtered1 = 0;
        uint16_t filtered2 = 0;

        if (brick_asc_process(&asc_state[mux], adc_sample1, &filtered1)) {
            update_state(mux, filtered1);
        }

        if (brick_asc_process(&asc_state[mux + BRICK_HALL_MUX_CHANNELS], adc_sample2, &filtered2)) {
            update_state(mux + BRICK_HALL_MUX_CHANNELS, filtered2);
        }
    }
}

uint16_t drv_hall_get_raw(uint8_t i) {
    if (i >= BRICK_NUM_HALL_SENSORS) {
        return 0;
    }
    return hall_state[i].raw;
}

uint8_t drv_hall_get_value(uint8_t i) {
    if (i >= BRICK_NUM_HALL_SENSORS) {
        return 0;
    }
    return hall_state[i].value;
}

uint8_t drv_hall_get_velocity(uint8_t i) {
    if (i >= BRICK_NUM_HALL_SENSORS) {
        return 0;
    }
    return hall_state[i].velocity;
}

bool drv_hall_is_pressed(uint8_t i) {
    if (i >= BRICK_NUM_HALL_SENSORS) {
        return false;
    }
    return hall_state[i].pressed;
}
