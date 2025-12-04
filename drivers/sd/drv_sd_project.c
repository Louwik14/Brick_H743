/**
 * @file drv_sd_project.c
 * @brief Couche données projet : lecture/écriture patterns et samples.
 */

#include "drv_sd_project.h"
#include "drv_sd_fs.h"
#include <string.h>
#include <stdio.h>

#define SD_PATH_MAX 160U

static uint8_t SD_DMA_BUFFER_ATTR sd_io_buffer[SD_MAX_SAMPLE_CHUNK];

typedef struct {
    sd_project_info_t *projects;
    size_t             max_projects;
    size_t            *listed;
} project_list_ctx_t;

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8U; ++b) {
            uint32_t mask = (uint32_t)(-(int32_t)(crc & 1U));
            crc = (crc >> 1) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}

static sd_error_t ensure_project_dirs(const char *project_name) {
    char path[SD_PATH_MAX];
    snprintf(path, sizeof(path), "/projects/%s", project_name);
    (void)drv_sd_fs_mkdir(path);
    snprintf(path, sizeof(path), "/projects/%s/patterns", project_name);
    (void)drv_sd_fs_mkdir(path);
    return SD_OK;
}

sd_error_t drv_sd_project_load_pattern(const char *project_name,
                                       const char *pattern_name,
                                       uint8_t    *buffer,
                                       size_t      buffer_size,
                                       size_t     *loaded_size,
                                       uint32_t   *generation) {
    if (project_name == NULL || pattern_name == NULL || buffer == NULL || buffer_size == 0U) {
        return SD_ERR_PARAM;
    }
    if (!drv_sd_fs_is_mounted()) {
        return SD_ERR_NOT_MOUNTED;
    }
    char path[SD_PATH_MAX];
    snprintf(path, sizeof(path), "/projects/%s/patterns/%s.pat", project_name, pattern_name);
    sd_fs_file_t file;
    sd_error_t err = drv_sd_fs_open(&file, path, FA_READ);
    if (err != SD_OK) {
        return err;
    }
    sd_file_header_t header;
    UINT br = 0;
    err = drv_sd_fs_read(&file, sd_io_buffer, sizeof(header), &br);
    if (err != SD_OK || br != sizeof(header)) {
        drv_sd_fs_close(&file);
        return SD_ERR_CORRUPTED;
    }
    memcpy(&header, sd_io_buffer, sizeof(header));
    if (header.magic != SD_PATTERN_MAGIC || header.version != SD_PATTERN_VERSION || header.size_bytes > buffer_size) {
        drv_sd_fs_close(&file);
        return SD_ERR_CORRUPTED;
    }
    if (header.size_bytes > SD_MAX_SAMPLE_CHUNK) {
        drv_sd_fs_close(&file);
        return SD_ERR_PARAM;
    }
    err = drv_sd_fs_read(&file, sd_io_buffer, (UINT)header.size_bytes, &br);
    drv_sd_fs_close(&file);
    if (err != SD_OK || br != header.size_bytes) {
        return SD_ERR_IO;
    }
    memcpy(buffer, sd_io_buffer, header.size_bytes);
    uint32_t crc = crc32_update(0U, sd_io_buffer, header.size_bytes);
    if (crc != header.crc32) {
        return SD_ERR_CRC;
    }
    if (loaded_size != NULL) {
        *loaded_size = header.size_bytes;
    }
    if (generation != NULL) {
        *generation = header.generation;
    }
    return SD_OK;
}

sd_error_t drv_sd_project_save_pattern(const char *project_name,
                                       const char *pattern_name,
                                       const uint8_t *data,
                                       size_t data_size,
                                       uint32_t generation) {
    if (project_name == NULL || pattern_name == NULL || data == NULL || data_size == 0U || data_size > SD_MAX_PATTERN_SIZE) {
        return SD_ERR_PARAM;
    }
    if (!drv_sd_fs_is_mounted()) {
        return SD_ERR_NOT_MOUNTED;
    }
    (void)ensure_project_dirs(project_name);

    char path_final[SD_PATH_MAX];
    char path_tmp[SD_PATH_MAX];
    snprintf(path_final, sizeof(path_final), "/projects/%s/patterns/%s.pat", project_name, pattern_name);
    snprintf(path_tmp, sizeof(path_tmp), "%s.tmp", path_final);

    sd_fs_file_t file;
    sd_error_t err = drv_sd_fs_open(&file, path_tmp, FA_WRITE | FA_CREATE_ALWAYS);
    if (err != SD_OK) {
        return err;
    }
    sd_file_header_t header;
    header.magic = SD_PATTERN_MAGIC;
    header.version = SD_PATTERN_VERSION;
    header.size_bytes = (uint32_t)data_size;
    header.generation = generation;
    header.crc32 = crc32_update(0U, data, data_size);

    UINT bw = 0;
    memcpy(sd_io_buffer, &header, sizeof(header));
    err = drv_sd_fs_write(&file, sd_io_buffer, sizeof(header), &bw);
    if (err != SD_OK || bw != sizeof(header)) {
        drv_sd_fs_close(&file);
        drv_sd_fs_delete(path_tmp);
        return SD_ERR_IO;
    }
    memcpy(sd_io_buffer, data, data_size);
    bw = 0;
    err = drv_sd_fs_write(&file, sd_io_buffer, (UINT)data_size, &bw);
    if (err != SD_OK || bw != data_size) {
        drv_sd_fs_close(&file);
        drv_sd_fs_delete(path_tmp);
        return SD_ERR_IO;
    }
    err = drv_sd_fs_sync(&file);
    drv_sd_fs_close(&file);
    if (err != SD_OK) {
        drv_sd_fs_delete(path_tmp);
        return err;
    }
    err = drv_sd_fs_rename(path_tmp, path_final);
    if (err != SD_OK) {
        drv_sd_fs_delete(path_tmp);
        return err;
    }
    return SD_OK;
}

sd_error_t drv_sd_project_load_sample(const char *sample_name,
                                      uint8_t    *buffer,
                                      size_t      buffer_size,
                                      size_t     *loaded_size) {
    if (sample_name == NULL || buffer == NULL || buffer_size == 0U) {
        return SD_ERR_PARAM;
    }
    if (!drv_sd_fs_is_mounted()) {
        return SD_ERR_NOT_MOUNTED;
    }
    char path[SD_PATH_MAX];
    snprintf(path, sizeof(path), "/samples/%s", sample_name);

    sd_fs_file_t file;
    sd_error_t err = drv_sd_fs_open(&file, path, FA_READ);
    if (err != SD_OK) {
        return err;
    }
    sd_file_header_t header;
    UINT br = 0;
    err = drv_sd_fs_read(&file, sd_io_buffer, sizeof(header), &br);
    if (err != SD_OK || br != sizeof(header)) {
        drv_sd_fs_close(&file);
        return SD_ERR_CORRUPTED;
    }
    memcpy(&header, sd_io_buffer, sizeof(header));
    if (header.magic != SD_SAMPLE_MAGIC || header.version != SD_SAMPLE_VERSION || header.size_bytes > buffer_size || header.size_bytes > (64U * 1024U * 1024U)) {
        drv_sd_fs_close(&file);
        return SD_ERR_CORRUPTED;
    }
    uint32_t remaining = header.size_bytes;
    uint32_t offset = 0U;
    while (remaining > 0U) {
        UINT chunk = (remaining > SD_MAX_SAMPLE_CHUNK) ? SD_MAX_SAMPLE_CHUNK : remaining;
        UINT read = 0;
        err = drv_sd_fs_read(&file, sd_io_buffer, chunk, &read);
        if (err != SD_OK || read != chunk) {
            drv_sd_fs_close(&file);
            return SD_ERR_IO;
        }
        memcpy(&buffer[offset], sd_io_buffer, chunk);
        offset += chunk;
        remaining -= chunk;
    }
    drv_sd_fs_close(&file);
    uint32_t crc = crc32_update(0U, buffer, header.size_bytes);
    if (crc != header.crc32) {
        return SD_ERR_CRC;
    }
    if (loaded_size != NULL) {
        *loaded_size = header.size_bytes;
    }
    return SD_OK;
}

static FRESULT list_projects_cb(FILINFO *info, void *arg) {
    project_list_ctx_t *params = (project_list_ctx_t *)arg;
    if (!(info->fattrib & AM_DIR)) {
        return FR_OK;
    }
    if (info->fname[0] == '.') {
        return FR_OK;
    }
    if (*(params->listed) >= params->max_projects) {
        return FR_EXIST; /* stop iteration */
    }
    sd_project_info_t *dst = &params->projects[*(params->listed)];
    strncpy(dst->name, info->fname, SD_MAX_NAME_LEN - 1U);
    dst->name[SD_MAX_NAME_LEN - 1U] = '\0';
    dst->generation = 0U;
    (*(params->listed))++;
    return FR_OK;
}

sd_error_t drv_sd_project_list_projects(sd_project_info_t *projects,
                                        size_t max_projects,
                                        size_t *listed) {
    if (projects == NULL || listed == NULL || max_projects == 0U) {
        return SD_ERR_PARAM;
    }
    if (!drv_sd_fs_is_mounted()) {
        return SD_ERR_NOT_MOUNTED;
    }
    *listed = 0U;
    project_list_ctx_t params = {
        .projects = projects,
        .max_projects = max_projects,
        .listed = listed,
    };
    (void)drv_sd_fs_mkdir("/projects");
    sd_error_t res = drv_sd_fs_list_dir("/projects", list_projects_cb, &params);
    if (res == SD_ERR_FS && *listed > 0U) {
        return SD_OK;
    }
    return res;
}
