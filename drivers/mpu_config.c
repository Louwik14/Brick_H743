/**
 * @file mpu_config.c
 * @brief Initialisation minimale et idempotente du MPU pour la section .ram_d2.
 */

#include "mpu_config.h"

#include "stm32h743xx.h"
#include "core_cm7.h"
#include "mpu_armv7.h"

bool mpu_config_init_once(void) {
    static bool initialized = false;

    if (initialized) {
        return true;
    }

    ARM_MPU_Disable();
    __DSB();
    __ISB();

    /*
     * Région D2 : mémoire normale, non-cacheable, shareable pour les buffers
     * DMA audio/SD (.ram_d2).
     */
    ARM_MPU_SetRegion(ARM_MPU_RBAR(MPU_REGION_D2_NOCACHE, MPU_D2_BASE_ADDRESS),
                      ARM_MPU_RASR(0u,              /* XN */
                                   ARM_MPU_AP_FULL, /* RW */
                                   1u,              /* TEX: Normal memory */
                                   1u,              /* Shareable */
                                   0u,              /* Non-cacheable */
                                   0u,              /* Non-bufferable */
                                   0u,              /* Subregions enabled */
                                   MPU_D2_REGION_SIZE));

    __DSB();
    __ISB();

    SCB_InvalidateDCache();
    __DSB();
    __ISB();
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
    __DSB();
    __ISB();

    initialized = true;
    return true;
}

