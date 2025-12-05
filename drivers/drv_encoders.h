#ifndef DRV_ENCODERS_H
#define DRV_ENCODERS_H

#include <stdint.h>
#include "brick_config.h"

#define ENCODER_COUNT BRICK_NUM_ENCODERS

typedef enum {
    ENC1 = 0,
    ENC2,
    ENC3,
    ENC4
} encoder_id_t;

/*
 * API prévue pour un contexte mono-thread (UI / polling).
 * Les fonctions get_delta* mettent à jour un état interne (last_val[]).
 */
void    drv_encoders_start(void);
int16_t drv_encoder_get(encoder_id_t id);
void    drv_encoder_reset(encoder_id_t id);
int16_t drv_encoder_get_delta(encoder_id_t id);
int16_t drv_encoder_get_delta_accel(encoder_id_t id);

#endif
