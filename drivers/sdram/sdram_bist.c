#include "ch.h"
#include "hal.h"

#include "sdram_driver_priv.h"
#include "sdram_layout.h"

#define SDRAM_FULL_BYTES   SDRAM_TOTAL_SIZE_BYTES
#define SDRAM_QUICK_BYTES  (1u * 1024u * 1024u)
#define STRESS_BLOCK_BYTES (256u * 1024u)
#define ALIAS_PATTERN_A    (0xA55Au)
#define ALIAS_PATTERN_B    (0x5AA5u)

static void bist_init_result(sdram_bist_result_t *res) {
  res->status = SDRAM_BIST_PASS;
  res->words_tested = 0u;
  res->words_covered_unique = 0u;
  res->error_count = 0u;
  res->first_error_address = 0u;
  res->first_error = SDRAM_BIST_ERR_NONE;
  res->first_error_pattern = 0u;
  res->timestamp_start = chVTGetSystemTimeX();
  res->timestamp_end = res->timestamp_start;
}

static void bist_record_error(sdram_bist_result_t *res,
                              uintptr_t addr,
                              uint16_t expected,
                              uint16_t observed,
                              sdram_bist_error_t classification) {
  res->error_count++;
  if (res->first_error == SDRAM_BIST_ERR_NONE) {
    res->first_error_address = addr;
    res->first_error_pattern = expected;
    res->first_error = classification;
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
      bist_record_error(res, (uintptr_t)(&base[i]), pattern, read_back, SDRAM_BIST_ERR_DATA_MISMATCH);
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
      bist_record_error(&ctx->result, (uintptr_t)(&base[i]), expected, read_back, SDRAM_BIST_ERR_DATA_MISMATCH);
    }
  }

  ctx->patterns_executed++;
  ctx->result.words_tested += words;
}

static void bist_run_alias_probe(volatile uint16_t *base, uint32_t words, sdram_bist_context_t *ctx) {
  if (words < 2u) {
    return;
  }

  volatile uint16_t *addr_a = base;
  volatile uint16_t *addr_b = base + (words / 2u);

  addr_a[0] = ALIAS_PATTERN_A;
  addr_b[0] = ALIAS_PATTERN_B;

  const uint16_t observed_a = addr_a[0];
  const uint16_t observed_b = addr_b[0];

  if ((observed_a == observed_b) && (ALIAS_PATTERN_A != ALIAS_PATTERN_B)) {
    bist_record_error(&ctx->result,
                      (uintptr_t)addr_a,
                      ALIAS_PATTERN_A,
                      observed_a,
                      SDRAM_BIST_ERR_ADDRESS_ALIAS);
  } else {
    if (observed_a != ALIAS_PATTERN_A) {
      bist_record_error(&ctx->result,
                        (uintptr_t)addr_a,
                        ALIAS_PATTERN_A,
                        observed_a,
                        SDRAM_BIST_ERR_DATA_MISMATCH);
    }
    if (observed_b != ALIAS_PATTERN_B) {
      bist_record_error(&ctx->result,
                        (uintptr_t)addr_b,
                        ALIAS_PATTERN_B,
                        observed_b,
                        SDRAM_BIST_ERR_DATA_MISMATCH);
    }
  }

  ctx->patterns_executed++;
  ctx->result.words_tested += 2u;
}

static void bist_run_stress_sequential_offset(volatile uint16_t *base,
                                              uint32_t offset_words,
                                              uint32_t words,
                                              sdram_bist_context_t *ctx) {
  volatile uint16_t *const target = base + offset_words;
  for (uint32_t i = 0; i < words; ++i) {
    target[i] = (uint16_t)(i & 0xFFFFu);
  }

  for (uint32_t i = 0; i < words; ++i) {
    const uint16_t expected = (uint16_t)(i & 0xFFFFu);
    const uint16_t read_back = target[i];
    if (read_back != expected) {
      bist_record_error(&ctx->result, (uintptr_t)(&target[i]), expected, read_back, SDRAM_BIST_ERR_DATA_MISMATCH);
    }
  }

  ctx->patterns_executed++;
  ctx->result.words_tested += words;
}

bool sdram_bist_start(sdram_bist_context_t *ctx) {
  if (ctx == NULL) {
    return false;
  }

  if ((ctx->mode != SDRAM_BIST_MODE_QUICK) && (ctx->mode != SDRAM_BIST_MODE_FULL)) {
    return false;
  }

  bist_init_result(&ctx->result);
  ctx->patterns_executed = 0u;

  const uint32_t coverage_bytes = (ctx->mode == SDRAM_BIST_MODE_FULL) ? SDRAM_FULL_BYTES : SDRAM_QUICK_BYTES;
  const uint32_t coverage_words = coverage_bytes / sizeof(uint16_t);
  ctx->words_target = coverage_words;
  ctx->result.words_covered_unique = coverage_words;
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
    const uint32_t max_start = (coverage_words > stress_words) ? (coverage_words - stress_words) : 0u;
    const uint32_t mid_offset = (max_start > stress_words) ? ((coverage_words / 2u) - (stress_words / 2u)) : 0u;
    const uint32_t end_offset = max_start;

    bist_run_alias_probe(base, coverage_words, ctx);
    bist_run_stress_sequential_offset(base, 0u, stress_words, ctx);
    bist_run_stress_sequential_offset(base, mid_offset, stress_words, ctx);
    bist_run_stress_sequential_offset(base, end_offset, stress_words, ctx);
    chThdYield();
  }

  bist_finalize_result(&ctx->result);
  return true;
}

