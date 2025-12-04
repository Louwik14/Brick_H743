#ifndef SDRAM_DRIVER_PRIV_H
#define SDRAM_DRIVER_PRIV_H

/**
 * @file sdram_driver_priv.h
 * @brief Internal types and helpers for the SDRAM driver implementation.
 */

#include <stdbool.h>
#include <stdint.h>
#include "sdram_driver.h"
#include "sdram_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Internal driver context (stateful, single instance).
 */
typedef struct {
  sdram_state_t state;
  sdram_error_t last_error;
  bool bist_running;
  sdram_bist_result_t last_bist_result;
} sdram_driver_ctx_t;

/**
 * @brief BIST execution context shared across patterns.
 */
typedef struct {
  sdram_bist_mode_t mode;
  uint32_t patterns_executed;
  uint32_t words_target;
  sdram_bist_result_t result;
} sdram_bist_context_t;

/**
 * @brief Initialize FMC + MPU mapping (internal helper).
 *
 * Executes the JEDEC-compliant SDRAM sequence and programs the refresh counter.
 * Returns false if the FMC reports a timeout or if refresh remains inactive.
 */
bool sdram_hw_init_sequence(void);

/**
 * @brief Configure/enable MPU regions for SDRAM logical layout.
 *
 * Should map audio regions as non-cacheable, shareable and the optional residual
 * region as cacheable CPU-only. Returns false on misconfiguration or parameter
 * mismatch.
 */
bool sdram_configure_mpu_regions(void);

/**
 * @brief Populate the public region info structure from the static descriptors.
 *
 * Returns false if the region is unavailable (disabled or outside READY state).
 */
bool sdram_query_region_descriptor(sdram_region_id_t id, sdram_region_info_t *out_info);

/**
 * @brief Internal helper to start a BIST run with the provided context.
 */
bool sdram_bist_start(sdram_bist_context_t *ctx);

/**
 * @brief Global singleton instance (definition in sdram_driver.c).
 */
extern sdram_driver_ctx_t sdram_ctx;

#ifdef __cplusplus
}
#endif

#endif /* SDRAM_DRIVER_PRIV_H */
