#include "ch.h"
#include "hal.h"

#include "sdram_driver_priv.h"
#include "sdram_layout.h"

#define SDRAM_FULL_BYTES   (32u * 1024u * 1024u)
#define SDRAM_QUICK_BYTES  (1u * 1024u * 1024u)
#define STRESS_BLOCK_BYTES (256u * 1024u)

static void bist_init_result(sdram_bist_result_t *res) {
  res->status = SDRAM_BIST_PASS;
  res->words_tested = 0u;
  res->error_count = 0u;
  res->first_error_address = 0u;
  res->first_error = SDRAM_BIST_ERR_NONE;
  res->first_error_pattern = 0u;
  res->timestamp_start = chVTGetSystemTimeX();
  res->timestamp_end = res->timestamp_start;
}

static void bist_record_error(sdram_bist_result_t *res, uintptr_t addr, uint16_t expected, uint16_t observed) {
  res->error_count++;
  if (res->first_error == SDRAM_BIST_ERR_NONE) {
    res->first_error_address = addr;
    res->first_error_pattern = expected;
    res->first_error = (expected == observed) ? SDRAM_BIST_ERR_ADDRESS_ALIAS : SDRAM_BIST_ERR_DATA_MISMATCH;
    res->status = SDRAM_BIST_FAIL;
  }
}

static void bist_finalize_result(sdram_bist_result_t *res) {
  if (res->error_count > 0u && res->status == SDRAM_BIST_PASS) {
    res->status = SDRAM_BIST_FAIL;
  }
  res->timestamp_end = chVTGetSystemTimeX();
}

static void bist_write_constant(volatile uint16_t *base, uint32_t words, uint16_t pattern) {
  for (uint32_t i = 0; i < words; ++i) {
    base[i] = pattern;
  }
}

static void bist_verify_constant(volatile uint16_t *base, uint32_t words, uint16_t pattern, sdram_bist_result_t *res) {
  for (uint32_t i = 0; i < words; ++i) {
    uint16_t read_back = base[i];
    if (read_back != pattern) {
      bist_record_error(res, (uintptr_t)(&base[i]), pattern, read_back);
    }
  }
}

static void bist_run_constant_pattern(volatile uint16_t *base, uint32_t words, uint16_t pattern, sdram_bist_context_t *ctx) {
  bist_write_constant(base, words, pattern);
  bist_verify_constant(base, words, pattern, &ctx->result);
  ctx->patterns_executed++;
  ctx->result.words_tested += words;
}

static void bist_run_walking(volatile uint16_t *base, uint32_t words, bool walking_one, sdram_bist_context_t *ctx) {
  for (uint32_t bit = 0; bit < 16u; ++bit) {
    uint16_t pattern = walking_one ? ((uint16_t)1u << bit) : (uint16_t)(~((uint16_t)1u << bit));
    bist_write_constant(base, words, pattern);
    bist_verify_constant(base, words, pattern, &ctx->result);
    ctx->patterns_executed++;
    ctx->result.words_tested += words;
  }
}

static void bist_run_lfsr(volatile uint16_t *base, uint32_t words, sdram_bist_context_t *ctx) {
  uint16_t lfsr = 0xACE1u;
  for (uint32_t i = 0; i < words; ++i) {
    uint16_t bit = (uint16_t)(((lfsr >> 0u) ^ (lfsr >> 2u) ^ (lfsr >> 3u) ^ (lfsr >> 5u)) & 1u);
    lfsr = (uint16_t)((lfsr >> 1u) | (bit << 15));
    base[i] = lfsr;
  }

  lfsr = 0xACE1u;
  for (uint32_t i = 0; i < words; ++i) {
    uint16_t bit = (uint16_t)(((lfsr >> 0u) ^ (lfsr >> 2u) ^ (lfsr >> 3u) ^ (lfsr >> 5u)) & 1u);
    lfsr = (uint16_t)((lfsr >> 1u) | (bit << 15));
    uint16_t expected = lfsr;
    uint16_t read_back = base[i];
    if (read_back != expected) {
      bist_record_error(&ctx->result, (uintptr_t)(&base[i]), expected, read_back);
    }
  }

  ctx->patterns_executed++;
  ctx->result.words_tested += words;
}

static void bist_run_stress_sequential(volatile uint16_t *base, uint32_t words, sdram_bist_context_t *ctx) {
  for (uint32_t i = 0; i < words; ++i) {
    base[i] = (uint16_t)(i & 0xFFFFu);
  }

  for (uint32_t i = 0; i < words; ++i) {
    uint16_t expected = (uint16_t)(i & 0xFFFFu);
    uint16_t read_back = base[i];
    if (read_back != expected) {
      bist_record_error(&ctx->result, (uintptr_t)(&base[i]), expected, read_back);
    }
  }

  ctx->patterns_executed++;
  ctx->result.words_tested += words;
}

bool sdram_bist_start(sdram_bist_context_t *ctx) {
  if (ctx == NULL) {
    return false;
  }

  bist_init_result(&ctx->result);

  const uint32_t coverage_bytes = (ctx->mode == SDRAM_BIST_MODE_FULL) ? SDRAM_FULL_BYTES : SDRAM_QUICK_BYTES;
  const uint32_t coverage_words = coverage_bytes / sizeof(uint16_t);
  volatile uint16_t *const base = (volatile uint16_t *)SDRAM_BASE_ADDRESS;

  /* Static patterns */
  bist_run_constant_pattern(base, coverage_words, 0x0000u, ctx);
  bist_run_constant_pattern(base, coverage_words, 0xFFFFu, ctx);
  bist_run_constant_pattern(base, coverage_words, 0xAAAAu, ctx);
  bist_run_constant_pattern(base, coverage_words, 0x5555u, ctx);

  /* Walking patterns */
  bist_run_walking(base, coverage_words, true, ctx);
  bist_run_walking(base, coverage_words, false, ctx);

  /* Pseudo-random coverage */
  bist_run_lfsr(base, coverage_words, ctx);

  if (ctx->mode == SDRAM_BIST_MODE_FULL) {
    const uint32_t stress_words = (STRESS_BLOCK_BYTES / sizeof(uint16_t));
    bist_run_stress_sequential(base, stress_words, ctx);
    chThdYield();
  }

  bist_finalize_result(&ctx->result);
  return true;
}

