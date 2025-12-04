/**
 * @file drv_sd_hal.h
 * @brief Interface bas niveau HAL SDMMC pour le driver SD.
 */

#ifndef DRV_SD_HAL_H
#define DRV_SD_HAL_H

#include "ch.h"
#include "hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SD_HAL_OK = 0,
    SD_HAL_NO_CARD,
    SD_HAL_ERROR,
    SD_HAL_CRC,
    SD_HAL_TIMEOUT
} sd_hal_status_t;

void            drv_sd_hal_init(void);
void            drv_sd_hal_deinit(void);
bool            drv_sd_hal_is_card_present(void);
sd_hal_status_t drv_sd_hal_connect(void);
void            drv_sd_hal_disconnect(void);
sd_hal_status_t drv_sd_hal_read_blocks(uint8_t *buffer, uint32_t sector, uint32_t count);
sd_hal_status_t drv_sd_hal_write_blocks(const uint8_t *buffer, uint32_t sector, uint32_t count);
void            drv_sd_hal_sync(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SD_HAL_H */
