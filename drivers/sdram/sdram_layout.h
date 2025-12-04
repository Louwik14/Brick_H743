#ifndef SDRAM_LAYOUT_H
#define SDRAM_LAYOUT_H

/**
 * @file sdram_layout.h
 * @brief Static SDRAM region layout and configuration constants.
 */

#include <stdint.h>
#include "sdram_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Base physical address and size of the SDRAM device.
 */
#define SDRAM_BASE_ADDRESS      (0xC0000000U)
#define SDRAM_TOTAL_SIZE_BYTES  (32u * 1024u * 1024u)

/**
 * @brief Minimum alignment guaranteed for audio buffers placed in SDRAM.
 */
#define SDRAM_AUDIO_ALIGNMENT_BYTES  (64u)

/**
 * @brief Build-time toggle for the optional cacheable residual region.
 *
 * When set to 0, the SDRAM_CACHE_RESIDUAL region is absent and any request for it
 * via sdram_get_region will fail with base/size = 0. When set to 1, the region
 * is exposed as CPU-only (cacheable) and remains forbidden for DMA.
 */
#define SDRAM_ENABLE_CACHE_RESIDUAL  (0u)

/**
 * @brief Static descriptor for a memory region.
 */
typedef struct {
  sdram_region_id_t id;   /**< Logical identifier. */
  uintptr_t base;         /**< Physical base address. */
  uint32_t size_bytes;    /**< Size in bytes. */
  uint32_t flags;         /**< Attribute flags (SDRAM_REGION_FLAG_*). */
  uint32_t alignment;     /**< Alignment enforced for internal allocations. */
} sdram_region_descriptor_t;

/**
 * @brief Table of region descriptors defined at link time.
 *
 * Implementations must provide definitions for this array and count in the
 * corresponding .c file. Audio regions shall always be marked with
 * SDRAM_REGION_FLAG_DMA_AUDIO_SAFE and non-cacheable attributes, while the
 * optional CPU-only region is marked SDRAM_REGION_FLAG_CACHEABLE |
 * SDRAM_REGION_FLAG_CPU_ONLY.
 */
extern const sdram_region_descriptor_t sdram_region_descriptors[];
extern const uint32_t sdram_region_descriptor_count;

#ifdef __cplusplus
}
#endif

#endif /* SDRAM_LAYOUT_H */
