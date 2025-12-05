/**
 * @file mpu_config.h
 * @brief Configuration minimale commune du MPU pour les buffers non-cacheables.
 */

#ifndef MPU_CONFIG_H
#define MPU_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include "mpu_map.h"
#include "mpu_armv7.h"

/*
 * Configuration par défaut : fenêtre .ram_d2 alignée sur 32 KiB à 0x30040000
 * (voir script LD STM32H743xI.ld). La taille est exprimée avec les constantes
 * ARM_MPU_REGION_SIZE_* attendues par ARM_MPU_RASR().
 */
#define MPU_D2_BASE_ADDRESS  0x30040000UL
#define MPU_D2_REGION_SIZE   ARM_MPU_REGION_SIZE_32KB

/* Vérification d'alignement compile-time : adresse multiple de la taille MPU. */
#define MPU_D2_REGION_BYTES (1UL << (MPU_D2_REGION_SIZE + 1U))
#if ((MPU_D2_BASE_ADDRESS & (MPU_D2_REGION_BYTES - 1UL)) != 0U)
#error "MPU_D2_BASE_ADDRESS must be aligned on MPU_D2_REGION_BYTES"
#endif

bool mpu_config_init_once(void);

#endif /* MPU_CONFIG_H */
