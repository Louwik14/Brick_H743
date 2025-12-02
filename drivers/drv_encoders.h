#ifndef DRV_ENCODERS_H
#define DRV_ENCODERS_H

#include <stdint.h>

typedef enum {
    ENC1 = 0,
    ENC2,
    ENC3,
    ENC4
} encoder_id_t;

void    drv_encoders_start(void);
int16_t drv_encoder_get(encoder_id_t id);
void    drv_encoder_reset(encoder_id_t id);
int16_t drv_encoder_get_delta(encoder_id_t id);
int16_t drv_encoder_get_delta_accel(encoder_id_t id);

#endif
