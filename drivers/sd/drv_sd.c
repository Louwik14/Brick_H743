/**
 * @file drv_sd.c
 * @brief API publique du driver SD orchestrant HAL, FatFS et thread dédié.
 */

#include "drv_sd.h"
#include "drv_sd_thread.h"
#include "drv_sd_hal.h"
#include "drv_sd_fs.h"
#include "drv_sd_project.h"
#include <string.h>

sd_state_t g_sd_state = SD_STATE_INITIALIZING;
sd_error_t g_sd_last_error = SD_OK;
sd_stats_t g_sd_stats = {0};

static bool sd_initialized = false;
static thread_t *sd_audio_thread = NULL;

static void sd_ensure_thread_started(void) {
    if (!sd_initialized) {
        drv_sd_thread_start();
        sd_initialized = true;
        g_sd_state = SD_STATE_UNMOUNTED;
    }
}

static bool sd_context_forbidden(void) {
    if (__get_IPSR() != 0U) {
        return true;
    }
    const char *name = chRegGetThreadNameX();
    if (name != NULL && strcmp(name, "audioProcess") == 0) {
        return true;
    }
    if (sd_audio_thread != NULL && chThdGetSelfX() == sd_audio_thread) {
        return true;
    }
    return false;
}

static void sd_record_rejection(sd_error_t err) {
    g_sd_last_error = err;
    g_sd_stats.ops_total++;
    g_sd_stats.ops_error++;
    switch (err) {
    case SD_ERR_CONTEXT: g_sd_stats.err_context++; break;
    case SD_ERR_BUSY: g_sd_stats.err_busy++; break;
    default: break;
    }
}

static sd_error_t sd_submit_request(sd_request_t *req) {
    if (!req) {
        sd_record_rejection(SD_ERR_BUSY);
        return SD_ERR_BUSY;
    }
    if (!drv_sd_thread_post(req)) {
        drv_sd_thread_release(req);
        sd_record_rejection(SD_ERR_BUSY);
        return SD_ERR_BUSY;
    }
    msg_t wait = chBSemWaitTimeout(&req->done, TIME_MS2I(2000));
    if (wait == MSG_TIMEOUT) {
        req->auto_release = true;
        g_sd_last_error = SD_ERR_FAULT;
        return SD_ERR_FAULT;
    }
    sd_error_t res = req->result;
    drv_sd_thread_release(req);
    return res;
}

sd_error_t drv_sd_init(void) {
    if (sd_context_forbidden()) {
        sd_record_rejection(SD_ERR_CONTEXT);
        return SD_ERR_CONTEXT;
    }
    sd_ensure_thread_started();
    sd_request_t *req = drv_sd_thread_alloc();
    if (req == NULL) {
        return SD_ERR_BUSY;
    }
    req->type = SD_REQ_INIT;
    return sd_submit_request(req);
}

sd_error_t drv_sd_mount(bool read_only) {
    if (sd_context_forbidden()) {
        sd_record_rejection(SD_ERR_CONTEXT);
        return SD_ERR_CONTEXT;
    }
    sd_ensure_thread_started();
    sd_request_t *req = drv_sd_thread_alloc();
    if (req == NULL) {
        return SD_ERR_BUSY;
    }
    req->type = SD_REQ_MOUNT;
    req->params.mount_ro = read_only;
    return sd_submit_request(req);
}

sd_error_t drv_sd_unmount(void) {
    if (sd_context_forbidden()) {
        sd_record_rejection(SD_ERR_CONTEXT);
        return SD_ERR_CONTEXT;
    }
    sd_ensure_thread_started();
    sd_request_t *req = drv_sd_thread_alloc();
    if (req == NULL) {
        return SD_ERR_BUSY;
    }
    req->type = SD_REQ_UNMOUNT;
    return sd_submit_request(req);
}

bool drv_sd_is_present(void) {
    return drv_sd_hal_is_card_present();
}

sd_state_t drv_sd_get_state(void) {
    return g_sd_state;
}

sd_error_t drv_sd_get_last_error(void) {
    return g_sd_last_error;
}

static bool sd_validate_name(const char *name) {
    if (name == NULL) {
        return false;
    }
    return strlen(name) < SD_MAX_NAME_LEN;
}

sd_error_t drv_sd_load_pattern(const char *project_name,
                               const char *pattern_name,
                               uint8_t    *buffer,
                               size_t      buffer_size,
                               size_t     *loaded_size,
                               uint32_t   *generation) {
    if (sd_context_forbidden()) {
        sd_record_rejection(SD_ERR_CONTEXT);
        return SD_ERR_CONTEXT;
    }
    sd_ensure_thread_started();
    if (!sd_validate_name(project_name) || !sd_validate_name(pattern_name) || buffer == NULL) {
        sd_record_rejection(SD_ERR_PARAM);
        return SD_ERR_PARAM;
    }
    sd_request_t *req = drv_sd_thread_alloc();
    if (req == NULL) {
        return SD_ERR_BUSY;
    }
    req->type = SD_REQ_LOAD_PATTERN;
    req->params.pattern.project_name = project_name;
    req->params.pattern.pattern_name = pattern_name;
    req->params.pattern.buffer = buffer;
    req->params.pattern.buffer_size = buffer_size;
    req->params.pattern.loaded_size = loaded_size;
    req->params.pattern.generation = generation;
    return sd_submit_request(req);
}

sd_error_t drv_sd_save_pattern(const char *project_name,
                               const char *pattern_name,
                               const uint8_t *data,
                               size_t data_size,
                               uint32_t generation) {
    if (sd_context_forbidden()) {
        sd_record_rejection(SD_ERR_CONTEXT);
        return SD_ERR_CONTEXT;
    }
    sd_ensure_thread_started();
    if (!sd_validate_name(project_name) || !sd_validate_name(pattern_name) || data == NULL || data_size == 0U || data_size > SD_MAX_PATTERN_SIZE) {
        sd_record_rejection(SD_ERR_PARAM);
        return SD_ERR_PARAM;
    }
    sd_request_t *req = drv_sd_thread_alloc();
    if (req == NULL) {
        return SD_ERR_BUSY;
    }
    req->type = SD_REQ_SAVE_PATTERN;
    req->params.pattern.project_name = project_name;
    req->params.pattern.pattern_name = pattern_name;
    req->params.pattern.input_data = data;
    req->params.pattern.input_size = data_size;
    req->params.pattern.generation = &generation;
    return sd_submit_request(req);
}

sd_error_t drv_sd_load_sample(const char *sample_name,
                              uint8_t    *buffer,
                              size_t      buffer_size,
                              size_t     *loaded_size) {
    if (sd_context_forbidden()) {
        sd_record_rejection(SD_ERR_CONTEXT);
        return SD_ERR_CONTEXT;
    }
    sd_ensure_thread_started();
    if (!sd_validate_name(sample_name) || buffer == NULL || buffer_size == 0U) {
        sd_record_rejection(SD_ERR_PARAM);
        return SD_ERR_PARAM;
    }
    sd_request_t *req = drv_sd_thread_alloc();
    if (req == NULL) {
        return SD_ERR_BUSY;
    }
    req->type = SD_REQ_LOAD_SAMPLE;
    req->params.sample.sample_name = sample_name;
    req->params.sample.buffer = buffer;
    req->params.sample.buffer_size = buffer_size;
    req->params.sample.loaded_size = loaded_size;
    return sd_submit_request(req);
}

sd_error_t drv_sd_list_projects(sd_project_info_t *projects,
                                size_t max_projects,
                                size_t *listed) {
    if (sd_context_forbidden()) {
        sd_record_rejection(SD_ERR_CONTEXT);
        return SD_ERR_CONTEXT;
    }
    sd_ensure_thread_started();
    if (projects == NULL || listed == NULL || max_projects == 0U) {
        sd_record_rejection(SD_ERR_PARAM);
        return SD_ERR_PARAM;
    }
    sd_request_t *req = drv_sd_thread_alloc();
    if (req == NULL) {
        return SD_ERR_BUSY;
    }
    req->type = SD_REQ_LIST_PROJECTS;
    req->params.list.projects = projects;
    req->params.list.max_projects = max_projects;
    req->params.list.listed = listed;
    return sd_submit_request(req);
}

sd_error_t drv_sd_get_stats(sd_stats_t *out_stats) {
    if (sd_context_forbidden()) {
        sd_record_rejection(SD_ERR_CONTEXT);
        return SD_ERR_CONTEXT;
    }
    sd_ensure_thread_started();
    if (out_stats == NULL) {
        sd_record_rejection(SD_ERR_PARAM);
        return SD_ERR_PARAM;
    }
    sd_request_t *req = drv_sd_thread_alloc();
    if (req == NULL) {
        return SD_ERR_BUSY;
    }
    req->type = SD_REQ_GET_STATS;
    req->params.stats.stats = out_stats;
    return sd_submit_request(req);
}

void drv_sd_clear_stats(void) {
    if (sd_context_forbidden()) {
        sd_record_rejection(SD_ERR_CONTEXT);
        return;
    }
    sd_ensure_thread_started();
    sd_request_t *req = drv_sd_thread_alloc();
    if (req == NULL) {
        sd_record_rejection(SD_ERR_BUSY);
        return;
    }
    req->type = SD_REQ_CLEAR_STATS;
    (void)sd_submit_request(req);
}
