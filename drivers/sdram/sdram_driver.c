#include "ch.h"
#include "hal.h"

#include "sdram_driver.h"
#include "sdram_driver_priv.h"
#include "sdram_layout.h"

// Global driver context and mutex protecting state/error/BIST fields.
sdram_driver_ctx_t sdram_ctx = {
  .state = SDRAM_NOT_INITIALIZED,
  .last_error = SDRAM_ERR_NONE,
  .bist_running = false,
  .last_bist_result = {0}
};

MUTEX_DECL(sdram_ctx_mtx);

static void sdram_set_fault_locked(sdram_error_t error) {
  sdram_ctx.state = SDRAM_FAULT;
  sdram_ctx.last_error = error;
}

static void sdram_clear_region_info(sdram_region_info_t *info) {
  if (info == NULL) {
    return;
  }

  info->id = SDRAM_REGION_INVALID;
  info->base = 0u;
  info->size_bytes = 0u;
  info->flags = 0u;
  info->alignment_bytes = 0u;
}

void sdram_init(bool run_quick_bist) {
  // Ensure single initialization sequence.
  chMtxLock(&sdram_ctx_mtx);
  if (sdram_ctx.state != SDRAM_NOT_INITIALIZED) {
    chMtxUnlock(&sdram_ctx_mtx);
    return;
  }

  sdram_ctx.state = SDRAM_INITIALIZING;
  sdram_ctx.last_error = SDRAM_ERR_NONE;
  sdram_ctx.bist_running = false;
  chMtxUnlock(&sdram_ctx_mtx);

  // Hardware/FMC initialization sequence.
  if (!sdram_hw_init_sequence()) {
    chMtxLock(&sdram_ctx_mtx);
    sdram_set_fault_locked(SDRAM_ERR_FMC_TIMEOUT);
    chMtxUnlock(&sdram_ctx_mtx);
    return;
  }

  // MPU region configuration for cache attributes.
  if (!sdram_configure_mpu_regions()) {
    chMtxLock(&sdram_ctx_mtx);
    sdram_set_fault_locked(SDRAM_ERR_PARAM);
    chMtxUnlock(&sdram_ctx_mtx);
    return;
  }

  if (!run_quick_bist) {
    chMtxLock(&sdram_ctx_mtx);
    sdram_ctx.state = SDRAM_READY;
    chMtxUnlock(&sdram_ctx_mtx);
    return;
  }

  // Quick BIST at boot (synchronous or quasi-synchronous).
  sdram_bist_context_t bist_ctx = {0};
  bist_ctx.mode = SDRAM_BIST_MODE_QUICK;

  chMtxLock(&sdram_ctx_mtx);
  sdram_ctx.bist_running = true;
  chMtxUnlock(&sdram_ctx_mtx);

  bool bist_ok = sdram_bist_start(&bist_ctx);

  chMtxLock(&sdram_ctx_mtx);
  sdram_ctx.bist_running = false;
  sdram_ctx.last_bist_result = bist_ctx.result;

  if (!bist_ok || bist_ctx.result.status != SDRAM_BIST_PASS) {
    sdram_ctx.state = SDRAM_DEGRADED;
    sdram_ctx.last_error = SDRAM_ERR_BIST_FAIL;
  } else {
    sdram_ctx.state = SDRAM_READY;
    sdram_ctx.last_error = SDRAM_ERR_NONE;
  }

  chMtxUnlock(&sdram_ctx_mtx);
}

sdram_state_t sdram_status(void) {
  chMtxLock(&sdram_ctx_mtx);
  sdram_state_t state = sdram_ctx.state;
  chMtxUnlock(&sdram_ctx_mtx);
  return state;
}

sdram_error_t sdram_get_error(void) {
  chMtxLock(&sdram_ctx_mtx);
  sdram_error_t error = sdram_ctx.last_error;
  chMtxUnlock(&sdram_ctx_mtx);
  return error;
}

bool sdram_run_bist(sdram_bist_mode_t mode, sdram_bist_result_t *out_result) {
  sdram_state_t entry_state;

  chMtxLock(&sdram_ctx_mtx);
  entry_state = sdram_ctx.state;

  if ((entry_state == SDRAM_NOT_INITIALIZED) || (entry_state == SDRAM_INITIALIZING) || sdram_ctx.bist_running) {
    chMtxUnlock(&sdram_ctx_mtx);
    return false;
  }

  sdram_ctx.bist_running = true;
  chMtxUnlock(&sdram_ctx_mtx);

  sdram_bist_context_t ctx = {0};
  ctx.mode = mode;

  bool started = sdram_bist_start(&ctx);

  chMtxLock(&sdram_ctx_mtx);
  sdram_ctx.bist_running = false;

  if (started) {
    sdram_ctx.last_bist_result = ctx.result;

    if (ctx.result.status == SDRAM_BIST_PASS) {
      if ((entry_state == SDRAM_DEGRADED) && (mode == SDRAM_BIST_MODE_FULL)) {
        sdram_ctx.state = SDRAM_READY;
        sdram_ctx.last_error = SDRAM_ERR_NONE;
      }
    } else {
      sdram_ctx.last_error = SDRAM_ERR_BIST_FAIL;

      if (mode == SDRAM_BIST_MODE_FULL) {
        sdram_ctx.state = SDRAM_FAULT;
      } else {
        sdram_ctx.state = SDRAM_DEGRADED;
      }
    }
  }

  if (out_result != NULL) {
    *out_result = sdram_ctx.last_bist_result;
  }

  chMtxUnlock(&sdram_ctx_mtx);
  return started;
}

bool sdram_get_region(sdram_region_id_t region_id, sdram_region_info_t *out_info) {
  if (out_info == NULL) {
    return false;
  }

  chMtxLock(&sdram_ctx_mtx);
  sdram_state_t state = sdram_ctx.state;
  bool bist_running = sdram_ctx.bist_running;
  chMtxUnlock(&sdram_ctx_mtx);

  if ((state == SDRAM_FAULT) || bist_running) {
    sdram_clear_region_info(out_info);
    return false;
  }

  if ((region_id == SDRAM_CACHE_RESIDUAL) && (SDRAM_ENABLE_CACHE_RESIDUAL == 0u)) {
    sdram_clear_region_info(out_info);
    return false;
  }

  if (!sdram_query_region_descriptor(region_id, out_info)) {
    sdram_clear_region_info(out_info);
    return false;
  }

  return true;
}

