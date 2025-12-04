/**
 * @file drv_sd_thread.c
 * @brief Thread dédié SD + FIFO statique.
 */

#include "drv_sd_thread.h"
#include "drv_sd_fs.h"
#include "drv_sd_hal.h"
#include "drv_sd_project.h"
#include <string.h>

/* États globaux déclarés dans drv_sd.c */
extern sd_state_t g_sd_state;
extern sd_error_t g_sd_last_error;
extern sd_stats_t g_sd_stats;

#define SD_THREAD_STACK_SIZE 2048U
#define SD_THREAD_PRIORITY   (NORMALPRIO - 2)

static THD_WORKING_AREA(sdThreadWA, SD_THREAD_STACK_SIZE);
static mailbox_t sd_mb;
static msg_t sd_mailbox_buffer[SD_FIFO_DEPTH];

static sd_request_t sd_requests[SD_FIFO_DEPTH];
static bool sd_request_in_use[SD_FIFO_DEPTH];
static mutex_t sd_alloc_mutex;

static bool sd_request_is_write(const sd_request_t *req) {
    if (req == NULL) {
        return false;
    }
    return (req->type == SD_REQ_SAVE_PATTERN);
}

static void sd_purge_write_requests(void) {
    sd_request_t *pending[SD_FIFO_DEPTH];
    size_t pending_count = 0U;
    msg_t msg;
    while (chMBFetchTimeout(&sd_mb, &msg, TIME_IMMEDIATE) == MSG_OK) {
        sd_request_t *req = (sd_request_t *)msg;
        if (sd_request_is_write(req)) {
            req->result = SD_ERR_FS;
            g_sd_last_error = SD_ERR_FS;
            chBSemSignal(&req->done);
            drv_sd_thread_release(req);
        } else {
            pending[pending_count++] = req;
        }
    }
    for (size_t i = 0; i < pending_count; ++i) {
        (void)chMBPostTimeout(&sd_mb, (msg_t)pending[i], TIME_IMMEDIATE);
    }
}

static void sd_stats_record(sd_error_t res, uint32_t latency_us) {
    g_sd_stats.ops_total++;
    if (res == SD_OK) {
        g_sd_stats.ops_success++;
    } else {
        g_sd_stats.ops_error++;
    }
    switch (res) {
    case SD_ERR_NO_CARD: g_sd_stats.err_no_card++; break;
    case SD_ERR_IO: g_sd_stats.err_io++; break;
    case SD_ERR_CRC: g_sd_stats.err_crc++; break;
    case SD_ERR_FS: g_sd_stats.err_fs++; break;
    case SD_ERR_PARAM: g_sd_stats.err_param++; break;
    case SD_ERR_FULL: g_sd_stats.err_full++; break;
    case SD_ERR_CORRUPTED: g_sd_stats.err_corrupted++; break;
    case SD_ERR_FAULT: g_sd_stats.err_fault++; break;
    case SD_ERR_BUSY: g_sd_stats.err_busy++; break;
    case SD_ERR_CONTEXT: g_sd_stats.err_context++; break;
    default: break;
    }
    if (latency_us < g_sd_stats.latency_min_us || g_sd_stats.latency_min_us == 0U) {
        g_sd_stats.latency_min_us = latency_us;
    }
    if (latency_us > g_sd_stats.latency_max_us) {
        g_sd_stats.latency_max_us = latency_us;
    }
    if (g_sd_stats.ops_total > 0U) {
        g_sd_stats.latency_avg_us = (uint32_t)(((uint64_t)g_sd_stats.latency_avg_us * (g_sd_stats.ops_total - 1U) + latency_us) / g_sd_stats.ops_total);
    }
}

static void sd_set_state(sd_state_t new_state) {
    g_sd_state = new_state;
}

static void sd_handle_write_protect_flag(void) {
    if (drv_sd_fs_consume_write_protect_event()) {
        sd_set_state(SD_STATE_MOUNTED_RO);
        sd_purge_write_requests();
    }
}

static sd_error_t sd_handle_init(void) {
    drv_sd_hal_init();
    drv_sd_fs_unmount();
    return SD_OK;
}

static sd_error_t sd_handle_mount(bool read_only) {
    if (!drv_sd_hal_is_card_present()) {
        sd_set_state(SD_STATE_UNMOUNTED);
        return SD_ERR_NO_CARD;
    }
    sd_error_t res = drv_sd_fs_mount(read_only ? SD_FS_RO : SD_FS_RW);
    if (res == SD_OK) {
        bool fs_ro = drv_sd_fs_is_read_only();
        sd_set_state((read_only || fs_ro) ? SD_STATE_MOUNTED_RO : SD_STATE_MOUNTED_RW);
    } else {
        sd_set_state(SD_STATE_UNMOUNTED);
    }
    return res;
}

static sd_error_t sd_handle_unmount(void) {
    drv_sd_fs_unmount();
    sd_set_state(SD_STATE_UNMOUNTED);
    return SD_OK;
}

static sd_error_t sd_handle_pattern_load(sd_request_t *req) {
    if (!drv_sd_fs_is_mounted()) {
        return SD_ERR_NOT_MOUNTED;
    }
    return drv_sd_project_load_pattern(req->params.pattern.project_name,
                                       req->params.pattern.pattern_name,
                                       req->params.pattern.buffer,
                                       req->params.pattern.buffer_size,
                                       req->params.pattern.loaded_size,
                                       req->params.pattern.generation);
}

static sd_error_t sd_handle_pattern_save(sd_request_t *req) {
    if (g_sd_state == SD_STATE_MOUNTED_RO || g_sd_state == SD_STATE_DEGRADED) {
        return SD_ERR_FS;
    }
    if (!drv_sd_fs_is_mounted()) {
        return SD_ERR_NOT_MOUNTED;
    }
    return drv_sd_project_save_pattern(req->params.pattern.project_name,
                                       req->params.pattern.pattern_name,
                                       req->params.pattern.input_data,
                                       req->params.pattern.input_size,
                                       req->params.pattern.generation == NULL ? 0U : *req->params.pattern.generation);
}

static sd_error_t sd_handle_sample_load(sd_request_t *req) {
    if (!drv_sd_fs_is_mounted()) {
        return SD_ERR_NOT_MOUNTED;
    }
    return drv_sd_project_load_sample(req->params.sample.sample_name,
                                      req->params.sample.buffer,
                                      req->params.sample.buffer_size,
                                      req->params.sample.loaded_size);
}

static sd_error_t sd_handle_list(sd_request_t *req) {
    if (!drv_sd_fs_is_mounted()) {
        return SD_ERR_NOT_MOUNTED;
    }
    return drv_sd_project_list_projects(req->params.list.projects,
                                        req->params.list.max_projects,
                                        req->params.list.listed);
}

static sd_error_t sd_handle_get_stats(sd_request_t *req) {
    if (req->params.stats.stats == NULL) {
        return SD_ERR_PARAM;
    }
    memcpy(req->params.stats.stats, &g_sd_stats, sizeof(g_sd_stats));
    return SD_OK;
}

static sd_error_t sd_handle_clear_stats(void) {
    memset(&g_sd_stats, 0, sizeof(g_sd_stats));
    return SD_OK;
}

static void sd_apply_error_state(sd_error_t res) {
    if (res == SD_OK) {
        return;
    }
    if (res == SD_ERR_CRC) {
        sd_set_state(SD_STATE_DEGRADED);
        return;
    }
    if (res == SD_ERR_IO || res == SD_ERR_FS || res == SD_ERR_FULL) {
        if (g_sd_state == SD_STATE_MOUNTED_RW) {
            sd_set_state(SD_STATE_DEGRADED);
        }
        return;
    }
    if (res == SD_ERR_NO_CARD) {
        sd_set_state(SD_STATE_UNMOUNTED);
        return;
    }
    if (res == SD_ERR_FAULT) {
        sd_set_state(SD_STATE_FAULT);
    }
}

static THD_FUNCTION(sdThread, arg) {
    (void)arg;
    chRegSetThreadName("sdThread");
    for (;;) {
        msg_t msg;
        if (chMBFetchTimeout(&sd_mb, &msg, TIME_INFINITE) != MSG_OK) {
            continue;
        }
        sd_request_t *req = (sd_request_t *)msg;
        systime_t start = chVTGetSystemTimeX();
        sd_state_t prev_state = g_sd_state;
        bool io_request = (req->type != SD_REQ_INIT && req->type != SD_REQ_GET_STATS && req->type != SD_REQ_CLEAR_STATS);
        if (io_request) {
            sd_set_state(SD_STATE_BUSY);
        }
        sd_error_t res = SD_ERR_PARAM;
        switch (req->type) {
        case SD_REQ_INIT:
            res = sd_handle_init();
            sd_set_state(SD_STATE_UNMOUNTED);
            break;
        case SD_REQ_MOUNT:
            res = sd_handle_mount(req->params.mount_ro);
            break;
        case SD_REQ_UNMOUNT:
            res = sd_handle_unmount();
            break;
        case SD_REQ_LOAD_PATTERN:
            res = sd_handle_pattern_load(req);
            sd_set_state(prev_state);
            break;
        case SD_REQ_SAVE_PATTERN:
            res = sd_handle_pattern_save(req);
            sd_set_state(prev_state);
            break;
        case SD_REQ_LOAD_SAMPLE:
            res = sd_handle_sample_load(req);
            sd_set_state(prev_state);
            break;
        case SD_REQ_LIST_PROJECTS:
            res = sd_handle_list(req);
            sd_set_state(prev_state);
            break;
        case SD_REQ_GET_STATS:
            res = sd_handle_get_stats(req);
            sd_set_state(prev_state);
            break;
        case SD_REQ_CLEAR_STATS:
            res = sd_handle_clear_stats();
            sd_set_state(prev_state);
            break;
        default:
            res = SD_ERR_PARAM;
            sd_set_state(prev_state);
            break;
        }
        req->result = res;
        g_sd_last_error = res;
        sd_apply_error_state(res);
        sd_handle_write_protect_flag();
        uint32_t latency_us = (uint32_t)TIME_I2US(chVTTimeElapsedSinceX(start));
        sd_stats_record(res, latency_us);
        chBSemSignal(&req->done);
        if (req->auto_release) {
            drv_sd_thread_release(req);
        }
    }
}

void drv_sd_thread_start(void) {
    chMBObjectInit(&sd_mb, sd_mailbox_buffer, SD_FIFO_DEPTH);
    chMtxObjectInit(&sd_alloc_mutex);
    memset(sd_request_in_use, 0, sizeof(sd_request_in_use));
    chThdCreateStatic(sdThreadWA, sizeof(sdThreadWA), SD_THREAD_PRIORITY, sdThread, NULL);
}

sd_request_t* drv_sd_thread_alloc(void) {
    sd_request_t *req = NULL;
    chMtxLock(&sd_alloc_mutex);
    for (size_t i = 0; i < SD_FIFO_DEPTH; ++i) {
        if (!sd_request_in_use[i]) {
            sd_request_in_use[i] = true;
            req = &sd_requests[i];
            memset(req, 0, sizeof(*req));
            chBSemObjectInit(&req->done, false);
            break;
        }
    }
    chMtxUnlock(&sd_alloc_mutex);
    return req;
}

void drv_sd_thread_release(sd_request_t *req) {
    if (req == NULL) {
        return;
    }
    chMtxLock(&sd_alloc_mutex);
    for (size_t i = 0; i < SD_FIFO_DEPTH; ++i) {
        if (&sd_requests[i] == req) {
            sd_request_in_use[i] = false;
            break;
        }
    }
    chMtxUnlock(&sd_alloc_mutex);
}

bool drv_sd_thread_post(sd_request_t *req) {
    if (req == NULL) {
        return false;
    }
    msg_t msg = (msg_t)req;
    msg_t status = chMBPostTimeout(&sd_mb, msg, TIME_IMMEDIATE);
    if (status != MSG_OK) {
        g_sd_stats.busy_rejections++;
        return false;
    }
    return true;
}
