#ifndef BRICK_CAL_H
#define BRICK_CAL_H

#include <stdbool.h>
#include <stdint.h>

struct brick_cal_pot {
  uint8_t resolution;
  uint8_t length;
  uint16_t maximum;
  uint16_t* value;
  uint16_t* center;
  uint16_t* detentlo;
  uint16_t* detenthi;
  uint8_t* enable;
};

int brick_cal_pot_init(struct brick_cal_pot* cal, uint8_t resolution, uint8_t length);
int brick_cal_pot_enable_range(struct brick_cal_pot* cal, uint8_t start, uint8_t length);
int brick_cal_pot_enable_get(struct brick_cal_pot* cal, uint8_t channel, uint8_t* enable);
int brick_cal_pot_center_get(struct brick_cal_pot* cal, uint8_t channel, uint16_t* center);
int brick_cal_pot_center_set(struct brick_cal_pot* cal, uint8_t channel, uint16_t center);
int brick_cal_pot_detent_get(struct brick_cal_pot* cal, uint8_t channel, uint16_t* detent, bool high);
int brick_cal_pot_detent_set(struct brick_cal_pot* cal, uint8_t channel, uint16_t detent, bool high);
int brick_cal_pot_value_get(struct brick_cal_pot* cal, uint8_t channel, uint16_t* value);
int brick_cal_pot_next(struct brick_cal_pot* cal, uint8_t channel, uint16_t in, uint16_t* out);

struct brick_ui_button_state;
uint16_t brick_ui_button_state_get_min(struct brick_ui_button_state* state);
uint16_t brick_ui_button_state_get_max(struct brick_ui_button_state* state);
void brick_ui_button_state_value_update(struct brick_ui_button_state* state, uint16_t value, uint64_t now);

struct brick_cal_but {
  uint8_t length;
  uint8_t* enable;
  struct brick_ui_button_state** states;
};

int brick_cal_but_init(struct brick_cal_but* cal, uint8_t length);
int brick_cal_but_enable_get(struct brick_cal_but* cal, uint8_t channel, uint8_t* enable);
int brick_cal_but_enable_set(struct brick_cal_but* cal, uint8_t channel, struct brick_ui_button_state* state);
int brick_cal_but_minmax_get(struct brick_cal_but* cal, uint8_t channel, uint16_t* min, uint16_t* max);
int brick_cal_but_min_set(struct brick_cal_but* cal, uint8_t channel, uint16_t min);
int brick_cal_but_max_set(struct brick_cal_but* cal, uint8_t channel, uint16_t max);
struct brick_ui_button_state* brick_cal_but_state_get(struct brick_cal_but* cal, uint8_t channel);

struct brick_cal_model {
  struct brick_cal_pot potmeter;
  struct brick_cal_but button;
};

extern struct brick_cal_model brick_cal_state;

#endif /* BRICK_CAL_H */
