#ifndef DRV_HALL_H
#define DRV_HALL_H

#include <stdbool.h>
#include <stdint.h>

#include "brick_config.h"

typedef struct {
    uint16_t raw;
    int32_t  filtered;
    uint16_t min;
    uint16_t max;
    uint16_t threshold;
    uint16_t hysteresis;

    uint32_t last_time;
    uint16_t last_raw;

    uint8_t  value;
    uint8_t  velocity;
    bool     pressed;
} hall_state_t;

void drv_hall_init(void);
void drv_hall_task(void);

uint16_t drv_hall_get_raw(uint8_t i);
uint8_t  drv_hall_get_value(uint8_t i);
uint8_t  drv_hall_get_velocity(uint8_t i);
bool     drv_hall_is_pressed(uint8_t i);

#endif /* DRV_HALL_H */
