#include "ch.h"

#include "sdram_driver_priv.h"
#include "sdram_layout.h"

#define SDRAM_TOTAL_BYTES   SDRAM_TOTAL_SIZE_BYTES
#define SDRAM_LOOP_BYTES    (16u * 1024u * 1024u)
#define SDRAM_DELAY_BYTES   (12u * 1024u * 1024u)
#define SDRAM_FX_BYTES      (3u * 1024u * 1024u)

#if (SDRAM_ENABLE_CACHE_RESIDUAL == 1)
#define SDRAM_RESIDUAL_BYTES  (1u * 1024u * 1024u)
#if (SDRAM_RESIDUAL_BYTES >= SDRAM_TOTAL_BYTES)
#error "Residual region exceeds SDRAM size"
#endif
#endif

#if ((SDRAM_LOOP_BYTES + SDRAM_DELAY_BYTES + SDRAM_FX_BYTES) > SDRAM_TOTAL_BYTES)
#error "SDRAM region layout exceeds total SDRAM capacity"
#endif

#if ((SDRAM_BASE_ADDRESS % SDRAM_AUDIO_ALIGNMENT_BYTES) != 0)
#error "SDRAM base address is not aligned to audio alignment"
#endif

#if (((SDRAM_LOOP_BYTES % SDRAM_AUDIO_ALIGNMENT_BYTES) != 0) || \
     ((SDRAM_DELAY_BYTES % SDRAM_AUDIO_ALIGNMENT_BYTES) != 0) || \
     ((SDRAM_FX_BYTES % SDRAM_AUDIO_ALIGNMENT_BYTES) != 0))
#error "SDRAM region sizes must align to audio alignment boundary"
#endif

#if ((SDRAM_ENABLE_CACHE_RESIDUAL == 1) && ((SDRAM_RESIDUAL_BYTES % SDRAM_AUDIO_ALIGNMENT_BYTES) != 0))
#error "Residual region size must align to audio alignment boundary"
#endif

const sdram_region_descriptor_t sdram_region_descriptors[] = {
  {
    .id = SDRAM_AUDIO_LOOP,
    .base = SDRAM_BASE_ADDRESS,
    .size_bytes = SDRAM_LOOP_BYTES,
    .flags = SDRAM_REGION_FLAG_DMA_AUDIO_SAFE,
    .alignment = SDRAM_AUDIO_ALIGNMENT_BYTES,
  },
  {
    .id = SDRAM_AUDIO_DELAY,
    .base = SDRAM_BASE_ADDRESS + SDRAM_LOOP_BYTES,
    .size_bytes = SDRAM_DELAY_BYTES,
    .flags = SDRAM_REGION_FLAG_DMA_AUDIO_SAFE,
    .alignment = SDRAM_AUDIO_ALIGNMENT_BYTES,
  },
  {
    .id = SDRAM_AUDIO_FX,
    .base = SDRAM_BASE_ADDRESS + SDRAM_LOOP_BYTES + SDRAM_DELAY_BYTES,
    .size_bytes = SDRAM_FX_BYTES,
    .flags = SDRAM_REGION_FLAG_DMA_AUDIO_SAFE,
    .alignment = SDRAM_AUDIO_ALIGNMENT_BYTES,
  },
#if (SDRAM_ENABLE_CACHE_RESIDUAL == 1)
  {
    .id = SDRAM_CACHE_RESIDUAL,
    .base = SDRAM_BASE_ADDRESS + (SDRAM_TOTAL_BYTES - SDRAM_RESIDUAL_BYTES),
    .size_bytes = SDRAM_RESIDUAL_BYTES,
    .flags = SDRAM_REGION_FLAG_CACHEABLE | SDRAM_REGION_FLAG_CPU_ONLY | SDRAM_REGION_FLAG_OPTIONAL,
    .alignment = SDRAM_AUDIO_ALIGNMENT_BYTES,
  },
#endif
};

const uint32_t sdram_region_descriptor_count = (uint32_t)(sizeof(sdram_region_descriptors) / sizeof(sdram_region_descriptors[0]));

static void sdram_clear_info(sdram_region_info_t *info, sdram_region_id_t id) {
  info->id = id;
  info->base = 0u;
  info->size_bytes = 0u;
  info->flags = 0u;
  info->alignment_bytes = 0u;
}

bool sdram_query_region_descriptor(sdram_region_id_t id, sdram_region_info_t *out_info) {
  if (out_info == NULL) {
    return false;
  }

  const sdram_state_t state = sdram_status();

  const sdram_region_descriptor_t *match = NULL;
  for (uint32_t i = 0u; i < sdram_region_descriptor_count; ++i) {
    if (sdram_region_descriptors[i].id == id) {
      match = &sdram_region_descriptors[i];
      break;
    }
  }

  if (match == NULL) {
    sdram_clear_info(out_info, id);
    return false;
  }

  if ((id == SDRAM_CACHE_RESIDUAL) && (SDRAM_ENABLE_CACHE_RESIDUAL == 0u)) {
    sdram_clear_info(out_info, id);
    return false;
  }

  if ((state == SDRAM_DEGRADED) && ((id == SDRAM_AUDIO_LOOP) || (id == SDRAM_AUDIO_DELAY) || (id == SDRAM_AUDIO_FX))) {
    sdram_clear_info(out_info, id);
    return false;
  }

  const uintptr_t region_end = match->base + (uintptr_t)match->size_bytes;
  const uintptr_t sdram_end = SDRAM_BASE_ADDRESS + (uintptr_t)SDRAM_TOTAL_BYTES;

  /* Guard against misconfigured layout or overflow. */
  if ((match->size_bytes == 0u) ||
      (match->base < SDRAM_BASE_ADDRESS) ||
      (region_end > sdram_end) ||
      ((match->alignment != 0u) && ((match->base % match->alignment) != 0u)) ||
      (match->alignment < SDRAM_AUDIO_ALIGNMENT_BYTES)) {
    sdram_clear_info(out_info, id);
    return false;
  }

  out_info->id = id;
  out_info->base = match->base;
  out_info->size_bytes = match->size_bytes;
  out_info->flags = match->flags;
  out_info->alignment_bytes = match->alignment;

  return true;
}

