/**
 * @file mpu_config.h
 * @brief Configuration minimale commune du MPU pour les buffers non-cacheables.
 */

#ifndef MPU_CONFIG_H
#define MPU_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include "mpu_map.h"

/*
 * Limites exportées par le script LD pour la section .ram_d2 (SRAM D2).
 * Utilisées pour dériver la fenêtre MPU non-cacheable compatible DMA.
 */
extern uint8_t __ram_d2_start__;
extern uint8_t __ram_d2_end__;

bool mpu_config_init_once(void);

#endif /* MPU_CONFIG_H */
