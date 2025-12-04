/**
 * @file drv_sd_fs.c
 * @brief Couche FatFS statique (montage, accès restreint aux répertoires dédiés).
 */

#include "drv_sd_fs.h"
#include "drv_sd_hal.h"
#include <string.h>

static FATFS sd_fs;
static bool fs_mounted = false;

static sd_error_t drv_sd_fs_map_result(FRESULT res) {
    switch (res) {
    case FR_OK:
        return SD_OK;
    case FR_NO_FILE:
    case FR_NO_PATH:
        return SD_ERR_CORRUPTED;
    case FR_DISK_ERR:
    case FR_INT_ERR:
    case FR_NOT_READY:
    case FR_INVALID_DRIVE:
        return SD_ERR_IO;
    case FR_DENIED:
        return SD_ERR_FULL;
    case FR_EXIST:
        return SD_ERR_PARAM;
    case FR_WRITE_PROTECTED:
        return SD_ERR_FS;
    default:
        return SD_ERR_FS;
    }
}

sd_error_t drv_sd_fs_mount(sd_fs_mode_t mode) {
    (void)mode;
    if (!drv_sd_hal_is_card_present()) {
        fs_mounted = false;
        return SD_ERR_NO_CARD;
    }
    FRESULT res = f_mount(&sd_fs, "", 1);
    if (res != FR_OK) {
        fs_mounted = false;
        return drv_sd_fs_map_result(res);
    }
    fs_mounted = true;
    return SD_OK;
}

void drv_sd_fs_unmount(void) {
    if (fs_mounted) {
        f_unmount("");
    }
    fs_mounted = false;
}

bool drv_sd_fs_is_mounted(void) {
    return fs_mounted;
}

sd_error_t drv_sd_fs_open(sd_fs_file_t *handle, const char *path, BYTE mode) {
    if (!fs_mounted || handle == NULL || path == NULL) {
        return SD_ERR_PARAM;
    }
    FRESULT res = f_open(&handle->file, path, mode);
    handle->open = (res == FR_OK);
    return drv_sd_fs_map_result(res);
}

void drv_sd_fs_close(sd_fs_file_t *handle) {
    if (handle != NULL && handle->open) {
        (void)f_close(&handle->file);
        handle->open = false;
    }
}

sd_error_t drv_sd_fs_read(sd_fs_file_t *handle, void *buffer, UINT btr, UINT *br) {
    if (!handle || !handle->open) {
        return SD_ERR_PARAM;
    }
    FRESULT res = f_read(&handle->file, buffer, btr, br);
    return drv_sd_fs_map_result(res);
}

sd_error_t drv_sd_fs_write(sd_fs_file_t *handle, const void *buffer, UINT btw, UINT *bw) {
    if (!handle || !handle->open) {
        return SD_ERR_PARAM;
    }
    FRESULT res = f_write(&handle->file, buffer, btw, bw);
    return drv_sd_fs_map_result(res);
}

sd_error_t drv_sd_fs_sync(sd_fs_file_t *handle) {
    if (!handle || !handle->open) {
        return SD_ERR_PARAM;
    }
    return drv_sd_fs_map_result(f_sync(&handle->file));
}

sd_error_t drv_sd_fs_stat(const char *path, FILINFO *info) {
    if (!fs_mounted || path == NULL || info == NULL) {
        return SD_ERR_PARAM;
    }
    return drv_sd_fs_map_result(f_stat(path, info));
}

sd_error_t drv_sd_fs_rename(const char *oldp, const char *newp) {
    if (!fs_mounted || oldp == NULL || newp == NULL) {
        return SD_ERR_PARAM;
    }
    return drv_sd_fs_map_result(f_rename(oldp, newp));
}

sd_error_t drv_sd_fs_delete(const char *path) {
    if (!fs_mounted || path == NULL) {
        return SD_ERR_PARAM;
    }
    return drv_sd_fs_map_result(f_unlink(path));
}

sd_error_t drv_sd_fs_mkdir(const char *path) {
    if (!fs_mounted || path == NULL) {
        return SD_ERR_PARAM;
    }
    return drv_sd_fs_map_result(f_mkdir(path));
}

sd_error_t drv_sd_fs_list_dir(const char *path, FRESULT (*cb)(FILINFO *info, void *arg), void *arg) {
    if (!fs_mounted || path == NULL || cb == NULL) {
        return SD_ERR_PARAM;
    }
    DIR dir;
    FRESULT res = f_opendir(&dir, path);
    if (res != FR_OK) {
        return drv_sd_fs_map_result(res);
    }
    FILINFO fno;
    for (;;) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) {
            break;
        }
        res = cb(&fno, arg);
        if (res != FR_OK) {
            break;
        }
    }
    f_closedir(&dir);
    return drv_sd_fs_map_result(res);
}
