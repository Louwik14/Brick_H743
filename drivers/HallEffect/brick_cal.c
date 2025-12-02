#include "brick_cal.h"

#include <string.h>

#define BRICK_CAL_DEFAULT_DEADZONE_DEN 20U

struct brick_cal_model brick_cal_state = {0};

static bool channel_valid(struct brick_cal_pot* cal, uint8_t channel) {
  return channel < cal->length && channel < BRICK_NUM_HALL_SENSORS;
}

int brick_cal_pot_init(struct brick_cal_pot* cal, uint8_t resolution, uint8_t length) {
  cal->resolution = resolution;
  cal->length = length;
  cal->maximum = (uint16_t)(1U << resolution);

  for (uint8_t i = 0; i < BRICK_NUM_HALL_SENSORS; ++i) {
    cal->min[i] = cal->maximum;
    cal->max[i] = 0;
    cal->enable[i] = 0;
    cal->detentlo[i] = 0;
    cal->detenthi[i] = cal->maximum;
  }

  return 0;
}

int brick_cal_pot_enable_range(struct brick_cal_pot* cal, uint8_t start, uint8_t length) {
  if (start >= cal->length) {
    return 1;
  }
  if ((uint16_t)start + length > cal->length) {
    return 1;
  }

  uint8_t end = (uint8_t)(start + length);
  for (uint8_t i = start; i < end; ++i) {
    cal->enable[i] = 1;
  }

  return 0;
}

int brick_cal_pot_enable_get(struct brick_cal_pot* cal, uint8_t channel, uint8_t* enable) {
  if (!channel_valid(cal, channel)) {
    return 1;
  }

  *enable = cal->enable[channel];
  return 0;
}

int brick_cal_pot_detent_get(struct brick_cal_pot* cal, uint8_t channel, uint16_t* detent, bool high) {
  if (!channel_valid(cal, channel)) {
    return 1;
  }

  *detent = high ? cal->detenthi[channel] : cal->detentlo[channel];
  return 0;
}

int brick_cal_pot_detent_set(struct brick_cal_pot* cal, uint8_t channel, uint16_t detent, bool high) {
  if (!channel_valid(cal, channel)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    return 1;
  }

  if (high) {
    cal->detenthi[channel] = detent;
  } else {
    cal->detentlo[channel] = detent;
  }

  return 0;
}

int brick_cal_pot_min_get(struct brick_cal_pot* cal, uint8_t channel, uint16_t* min_value) {
  if (!channel_valid(cal, channel)) {
    return 1;
  }

  *min_value = cal->min[channel];
  return 0;
}

int brick_cal_pot_max_get(struct brick_cal_pot* cal, uint8_t channel, uint16_t* max_value) {
  if (!channel_valid(cal, channel)) {
    return 1;
  }

  *max_value = cal->max[channel];
  return 0;
}

static void update_detent(struct brick_cal_pot* cal, uint8_t channel, uint16_t range, uint16_t center) {
  uint16_t half_deadzone = range / (BRICK_CAL_DEFAULT_DEADZONE_DEN * 2U);
  uint16_t lo = (center > half_deadzone) ? (uint16_t)(center - half_deadzone) : 0;
  uint16_t hi = center + half_deadzone;
  if (hi > cal->maximum) {
    hi = cal->maximum;
  }
  cal->detentlo[channel] = lo;
  cal->detenthi[channel] = hi;
}

int brick_cal_pot_next(struct brick_cal_pot* cal, uint8_t channel, uint16_t in, uint16_t* out) {
  if (!channel_valid(cal, channel)) {
    return 1;
  }

  if (!cal->enable[channel]) {
    *out = in;
    return 0;
  }

  if (in < cal->min[channel]) {
    cal->min[channel] = in;
  }
  if (in > cal->max[channel]) {
    cal->max[channel] = in;
  }

  uint16_t range = (uint16_t)(cal->max[channel] - cal->min[channel]);
  if (range == 0) {
    range = 1;
  }
  uint16_t center = (uint16_t)(cal->min[channel] + range / 2U);
  update_detent(cal, channel, range, center);

  *out = in;
  return 0;
}
