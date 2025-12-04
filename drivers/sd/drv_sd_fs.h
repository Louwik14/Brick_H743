/**
 * @file drv_sd_fs.h
 * @brief Couche FatFS pour le driver SD.
 */

#ifndef DRV_SD_FS_H
#define DRV_SD_FS_H

#include "ff.h"
#include "drv_sd.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    FIL     file;
    bool    open;
} sd_fs_file_t;

typedef enum {
    SD_FS_RW = 0,
    SD_FS_RO
} sd_fs_mode_t;

sd_error_t drv_sd_fs_mount(sd_fs_mode_t mode);
void       drv_sd_fs_unmount(void);
bool       drv_sd_fs_is_mounted(void);
bool       drv_sd_fs_is_read_only(void);
sd_error_t drv_sd_fs_open(sd_fs_file_t *handle, const char *path, BYTE mode);
void       drv_sd_fs_close(sd_fs_file_t *handle);
sd_error_t drv_sd_fs_read(sd_fs_file_t *handle, void *buffer, UINT btr, UINT *br);
sd_error_t drv_sd_fs_write(sd_fs_file_t *handle, const void *buffer, UINT btw, UINT *bw);
sd_error_t drv_sd_fs_sync(sd_fs_file_t *handle);
sd_error_t drv_sd_fs_stat(const char *path, FILINFO *info);
sd_error_t drv_sd_fs_rename(const char *oldp, const char *newp);
sd_error_t drv_sd_fs_delete(const char *path);
sd_error_t drv_sd_fs_mkdir(const char *path);
sd_error_t drv_sd_fs_list_dir(const char *path, FRESULT (*cb)(FILINFO *info, void *arg), void *arg);
bool       drv_sd_fs_consume_write_protect_event(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SD_FS_H */
