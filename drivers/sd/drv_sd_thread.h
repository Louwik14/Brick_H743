/**
 * @file drv_sd_thread.h
 * @brief Thread dédié SD et gestion de la file de requêtes.
 */

#ifndef DRV_SD_THREAD_H
#define DRV_SD_THREAD_H

#include "drv_sd.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SD_REQ_INIT = 0,
    SD_REQ_MOUNT,
    SD_REQ_UNMOUNT,
    SD_REQ_LOAD_PATTERN,
    SD_REQ_SAVE_PATTERN,
    SD_REQ_LOAD_SAMPLE,
    SD_REQ_LIST_PROJECTS,
    SD_REQ_GET_STATS,
    SD_REQ_CLEAR_STATS
} sd_request_type_t;

typedef struct sd_request_s sd_request_t;

typedef struct {
    const char *project_name;
    const char *pattern_name;
    uint8_t    *buffer;
    size_t      buffer_size;
    size_t     *loaded_size;
    uint32_t   *generation;
    const uint8_t *input_data;
    size_t      input_size;
} sd_pattern_params_t;

typedef struct {
    const char *sample_name;
    uint8_t    *buffer;
    size_t      buffer_size;
    size_t     *loaded_size;
} sd_sample_params_t;

typedef struct {
    sd_project_info_t *projects;
    size_t             max_projects;
    size_t            *listed;
} sd_list_params_t;

typedef struct {
    sd_stats_t *stats;
} sd_stats_params_t;

struct sd_request_s {
    sd_request_type_t type;
    sd_error_t        result;
    binary_semaphore_t done;
    union {
        bool                mount_ro;
        sd_pattern_params_t pattern;
        sd_sample_params_t  sample;
        sd_list_params_t    list;
        sd_stats_params_t   stats;
    } params;
};

void       drv_sd_thread_start(void);
bool       drv_sd_thread_post(sd_request_t *req);
sd_request_t* drv_sd_thread_alloc(void);
void       drv_sd_thread_release(sd_request_t *req);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SD_THREAD_H */
