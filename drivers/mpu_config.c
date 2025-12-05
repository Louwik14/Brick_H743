/**
 * @file mpu_config.c
 * @brief Initialisation minimale et idempotente du MPU pour la section .ram_d2.
 */

#include "mpu_config.h"

#include <stddef.h>
#include <stdint.h>

#include "stm32h743xx.h"
#include "core_cm7.h"
#include "mpu_armv7.h"

#define MPU_STATIC_ASSERT(cond, name) typedef char static_assert_##name[(cond) ? 1 : -1]

MPU_STATIC_ASSERT((((uintptr_t)&__ram_d2_start__) & 0x1FU) == 0U, ram_d2_start_alignment_32_bytes);

static bool mpu_compute_region_size(uintptr_t base,
                                    uintptr_t size,
                                    uint32_t *encoded_size,
                                    uintptr_t *region_bytes) {
    uint32_t rasr_size = ARM_MPU_REGION_SIZE_32B;
    uintptr_t candidate_bytes = 32U;

    if ((size == 0U) || (encoded_size == NULL) || (region_bytes == NULL)) {
        return false;
    }

    while ((candidate_bytes < size) && (rasr_size < ARM_MPU_REGION_SIZE_4GB)) {
        candidate_bytes <<= 1;
        rasr_size++;
    }

    if ((candidate_bytes & (candidate_bytes - 1U)) != 0U) {
        return false;
    }

    if ((base & (candidate_bytes - 1U)) != 0U) {
        return false;
    }

    *encoded_size = rasr_size;
    *region_bytes = candidate_bytes;
    return true;
}

bool mpu_config_init_once(void) {
    static bool initialized = false;
    const uintptr_t ram_d2_base = (uintptr_t)&__ram_d2_start__;
    const uintptr_t ram_d2_end  = (uintptr_t)&__ram_d2_end__;
    const uintptr_t ram_d2_size = ram_d2_end - ram_d2_base;
    uint32_t region_size_encoding = 0U;
    uintptr_t region_size_bytes = 0U;

    if (initialized) {
        return true;
    }

    if (!mpu_compute_region_size(ram_d2_base, ram_d2_size,
                                 &region_size_encoding, &region_size_bytes)) {
        return false;
    }

    if (ram_d2_size > region_size_bytes) {
        return false;
    }

    ARM_MPU_Disable();
    __DSB();
    __ISB();

    /*
     * Région D2 : mémoire normale, non-cacheable, shareable pour les buffers
     * DMA audio/SD (.ram_d2).
     */
    ARM_MPU_SetRegion(ARM_MPU_RBAR(MPU_REGION_D2_NOCACHE, ram_d2_base),
                      ARM_MPU_RASR(0u,              /* XN */
                                   ARM_MPU_AP_FULL, /* RW */
                                   1u,              /* TEX: Normal memory */
                                   1u,              /* Shareable */
                                   0u,              /* Non-cacheable */
                                   0u,              /* Non-bufferable */
                                   0u,              /* Subregions enabled */
                                   region_size_encoding));

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

