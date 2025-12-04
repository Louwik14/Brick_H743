#ifndef SDRAM_DRIVER_H
#define SDRAM_DRIVER_H

/**
 * @file sdram_driver.h
 * @brief Public API for the STM32H743 SDRAM driver (audio-first model A).
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "ch.h" /* for systime_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Global driver state machine.
 */
typedef enum {
  SDRAM_NOT_INITIALIZED = 0,
  SDRAM_INITIALIZING,
  SDRAM_READY,
  SDRAM_DEGRADED,
  SDRAM_FAULT
} sdram_state_t;

/**
 * @brief Last fatal or blocking error observed by the driver.
 */
typedef enum {
  SDRAM_ERR_NONE = 0,
  SDRAM_ERR_FMC_TIMEOUT,
  SDRAM_ERR_FMC_CMD,
  SDRAM_ERR_REFRESH,
  SDRAM_ERR_BIST_FAIL,
  SDRAM_ERR_PARAM
} sdram_error_t;

/**
 * @brief Logical regions exposed by the driver.
 *
 * All regions except SDRAM_CACHE_RESIDUAL are non-cacheable and DMA-safe for audio
 * by contract. SDRAM_CACHE_RESIDUAL is CPU-only and must never be used for DMA or
 * audio buffers.
 */
typedef enum {
  SDRAM_AUDIO_LOOP = 0,
  SDRAM_AUDIO_DELAY,
  SDRAM_AUDIO_FX,
  SDRAM_CACHE_RESIDUAL,
  SDRAM_REGION_INVALID
} sdram_region_id_t;

/**
 * @brief Attributes describing a logical region.
 */
typedef struct {
  sdram_region_id_t id;      /**< Region identifier. */
  uintptr_t base;            /**< Base address (NULL/0 if unavailable). */
  uint32_t size_bytes;       /**< Size of the region in bytes (0 if unavailable). */
  uint32_t flags;            /**< Attribute flags (see SDRAM_REGION_FLAG_*). */
  uint32_t alignment_bytes;  /**< Guaranteed alignment for buffers within the region. */
} sdram_region_info_t;

/** @name Region attribute flags */
/**@{*/
#define SDRAM_REGION_FLAG_CACHEABLE       (1u << 0) /**< Region is cacheable (CPU-only). */
#define SDRAM_REGION_FLAG_DMA_AUDIO_SAFE  (1u << 1) /**< Region is non-cacheable and safe for concurrent CPU + DMA audio. */
#define SDRAM_REGION_FLAG_CPU_ONLY        (1u << 2) /**< Region is intended for CPU-only usage, no DMA allowed. */
#define SDRAM_REGION_FLAG_OPTIONAL        (1u << 3) /**< Region may be absent in degraded mode or build-time disabled. */
/**@}*/

/**
 * @brief BIST status code.
 */
typedef enum {
  SDRAM_BIST_PASS = 0,
  SDRAM_BIST_FAIL,
  SDRAM_BIST_ABORT
} sdram_bist_status_t;

/**
 * @brief BIST error classification for the first detected fault.
 */
typedef enum {
  SDRAM_BIST_ERR_NONE = 0,
  SDRAM_BIST_ERR_DATA_MISMATCH,
  SDRAM_BIST_ERR_ADDRESS_ALIAS,
  SDRAM_BIST_ERR_STUCK_AT,
  SDRAM_BIST_ERR_TIMEOUT
} sdram_bist_error_t;

/**
 * @brief BIST execution mode.
 */
typedef enum {
  SDRAM_BIST_MODE_QUICK = 0, /**< Minimal coverage (e.g., 1 MiB, boot-time). */
  SDRAM_BIST_MODE_FULL       /**< Full coverage (32 MiB, maintenance). */
} sdram_bist_mode_t;

/**
 * @brief Normalized BIST result report (see design section 10.4.3).
 */
typedef struct {
  sdram_bist_status_t status;        /**< PASS/FAIL/ABORT. */
  uint32_t words_tested;             /**< Number of 16-bit words tested. */
  uint32_t error_count;              /**< Total detected errors. */
  uintptr_t first_error_address;     /**< Absolute SDRAM address of first failing word (0 if none). */
  sdram_bist_error_t first_error;    /**< Classification of the first error. */
  uint16_t first_error_pattern;      /**< Pattern in use when the first error occurred. */
  systime_t timestamp_start;         /**< ChibiOS tick at BIST start. */
  systime_t timestamp_end;           /**< ChibiOS tick at BIST end. */
} sdram_bist_result_t;

/**
 * @brief Initialize the SDRAM controller and memory map.
 *
 * - Must be invoked from a ChibiOS thread during boot, after clocks/MPU setup
 *   and before starting any audio processing.
 * - Optionally triggers a quick BIST (mode SDRAM_BIST_MODE_QUICK) depending on
 *   the implementation and parameter.
 * - Thread context only; never call from ISR.
 *
 * @param run_quick_bist When true, a quick BIST is started immediately after FMC init.
 */
void sdram_init(bool run_quick_bist);

/**
 * @brief Retrieve the current global state of the SDRAM driver.
 *
 * This function is safe to call from any thread. Audio and other modules must
 * ensure the state is SDRAM_READY before accessing SDRAM regions.
 *
 * @return Current state (READY, DEGRADED, FAULT, etc.).
 */
sdram_state_t sdram_status(void);

/**
 * @brief Retrieve the last recorded blocking error.
 *
 * @return Last error code (SDRAM_ERR_NONE if no error).
 */
sdram_error_t sdram_get_error(void);

/**
 * @brief Run a memory BIST in the requested mode.
 *
 * - Executes in thread context only; must not be called from ISR.
 * - Should be launched from a low-priority maintenance thread once SDRAM is
 *   initialized. Audio must be detached from SDRAM during a full BIST.
 *
 * @param mode Desired BIST mode (quick or full).
 * @param out_result Optional pointer to receive the normalized result report.
 * @return true if the BIST was scheduled/launched, false on invalid state/params.
 */
bool sdram_run_bist(sdram_bist_mode_t mode, sdram_bist_result_t *out_result);

/**
 * @brief Obtain information about a logical SDRAM region.
 *
 * - Returns false if the region is unavailable (invalid id, degraded/fault
 *   state, or optional region disabled). In that case, base = 0 and size = 0
 *   are returned.
 * - Audio/DMA code must request only audio regions (LOOP, DELAY, FX), which are
 *   guaranteed non-cacheable and DMA-safe by contract.
 * - CPU-only data must be confined to SDRAM_CACHE_RESIDUAL when present; DMA is
 *   forbidden on that region.
 *
 * @param region_id Logical region identifier.
 * @param out_info Pointer to the structure to fill.
 * @return true if the region is available and info populated, false otherwise.
 */
bool sdram_get_region(sdram_region_id_t region_id, sdram_region_info_t *out_info);

#ifdef __cplusplus
}
#endif

#endif /* SDRAM_DRIVER_H */
