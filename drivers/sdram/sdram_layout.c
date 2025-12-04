#include "ch.h"

#include "sdram_driver_priv.h"
#include "sdram_layout.h"

#define SDRAM_TOTAL_BYTES   (32u * 1024u * 1024u)

#if (SDRAM_ENABLE_CACHE_RESIDUAL == 1)
#define SDRAM_RESIDUAL_BYTES  (1u * 1024u * 1024u)
#endif

static const sdram_region_descriptor_t sdram_region_descriptors[] = {
  {
    .id = SDRAM_AUDIO_LOOP,
    .base = SDRAM_BASE_ADDRESS,
    .size_bytes = 16u * 1024u * 1024u,
    .flags = SDRAM_REGION_FLAG_DMA_AUDIO_SAFE,
    .alignment = SDRAM_AUDIO_ALIGNMENT_BYTES,
  },
  {
    .id = SDRAM_AUDIO_DELAY,
    .base = SDRAM_BASE_ADDRESS + (16u * 1024u * 1024u),
    .size_bytes = 12u * 1024u * 1024u,
    .flags = SDRAM_REGION_FLAG_DMA_AUDIO_SAFE,
    .alignment = SDRAM_AUDIO_ALIGNMENT_BYTES,
  },
  {
    .id = SDRAM_AUDIO_FX,
    .base = SDRAM_BASE_ADDRESS + (28u * 1024u * 1024u),
    .size_bytes = 3u * 1024u * 1024u,
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

  out_info->id = id;
  out_info->base = match->base;
  out_info->size_bytes = match->size_bytes;
  out_info->flags = match->flags;
  out_info->alignment_bytes = match->alignment;

  return true;
}

