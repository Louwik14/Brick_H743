/**
 * @file drv_sd.h
 * @brief API publique du driver SD : orchestration complète HAL/FatFS/thread dédié.
 */

#ifndef DRV_SD_H
#define DRV_SD_H

#include "ch.h"
#include "hal.h"
#include "ff.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Buffers marked with this attribute MUST live in an MPU non-cacheable region
 * (linker section .ram_d2). The linker exports __ram_d2_start__/__ram_d2_end__,
 * used by mpu_config_init_once() to program the matching MPU region.
 */
#define SD_DMA_BUFFER_ATTR __attribute__((section(".ram_d2"), aligned(32)))

#define SD_PATTERN_MAGIC   0x42525450UL /* 'PBRT' */
#define SD_PATTERN_VERSION 0x00010001UL

#define SD_SAMPLE_MAGIC    0x42525350UL /* 'PBSR' */
#define SD_SAMPLE_VERSION  0x00010001UL

#define SD_MAX_PATTERN_SIZE    (8U * 1024U)
#define SD_MAX_SAMPLE_CHUNK    (64U * 1024U)
#define SD_MAX_NAME_LEN        64U
#define SD_MAX_PROJECTS        64U
#define SD_MAX_PATTERNS        128U
#define SD_FIFO_DEPTH          8U

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SD_OK = 0,
    SD_ERR_NO_CARD,
    SD_ERR_NOT_MOUNTED,
    SD_ERR_BUSY,
    SD_ERR_IO,
    SD_ERR_CRC,
    SD_ERR_FS,
    SD_ERR_PARAM,
    SD_ERR_FULL,
    SD_ERR_CORRUPTED,
    SD_ERR_FAULT,
    SD_ERR_TIMEOUT,
    SD_ERR_CONTEXT
} sd_error_t;

typedef enum {
    SD_STATE_INITIALIZING = 0,
    SD_STATE_UNMOUNTED,
    SD_STATE_MOUNTED_RW,
    SD_STATE_MOUNTED_RO,
    SD_STATE_DEGRADED,
    SD_STATE_FAULT,
    SD_STATE_BUSY
} sd_state_t;

typedef struct {
    uint32_t ops_total;
    uint32_t ops_success;
    uint32_t ops_error;
    uint32_t err_no_card;
    uint32_t err_io;
    uint32_t err_crc;
    uint32_t err_fs;
    uint32_t err_param;
    uint32_t err_full;
    uint32_t err_corrupted;
    uint32_t err_fault;
    uint32_t err_timeout;
    uint32_t err_busy;
    uint32_t err_context;
    uint32_t busy_rejections;
    uint32_t latency_min_us;
    uint32_t latency_max_us;
    uint32_t latency_avg_us;
} sd_stats_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size_bytes;
    uint32_t generation;
    uint32_t crc32;
} sd_file_header_t;

typedef struct {
    uint32_t generation;
    char     name[SD_MAX_NAME_LEN];
} sd_project_info_t;

sd_error_t drv_sd_init(void);
sd_error_t drv_sd_mount(bool read_only);
sd_error_t drv_sd_unmount(void);
bool       drv_sd_is_present(void);
sd_state_t drv_sd_get_state(void);
sd_error_t drv_sd_get_last_error(void);

sd_error_t drv_sd_load_pattern(const char *project_name,
                               const char *pattern_name,
                               uint8_t    *buffer,
                               size_t      buffer_size,
                               size_t     *loaded_size,
                               uint32_t   *generation);

sd_error_t drv_sd_save_pattern(const char *project_name,
                               const char *pattern_name,
                               const uint8_t *data,
                               size_t data_size,
                               uint32_t generation);

sd_error_t drv_sd_load_sample(const char *sample_name,
                              uint8_t    *buffer,
                              size_t      buffer_size,
                              size_t     *loaded_size);

sd_error_t drv_sd_list_projects(sd_project_info_t *projects,
                                size_t max_projects,
                                size_t *listed);

sd_error_t drv_sd_get_stats(sd_stats_t *out_stats);
void       drv_sd_clear_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SD_H */
