/**
 * @file drv_sd_hal.c
 * @brief Implémentation bas niveau HAL SDMMC pour STM32H743.
 */

#include "drv_sd_hal.h"
#include "drv_sd.h"

static SDCDriver *sdcd = &SDCD1;
static const SDCConfig sd_cfg = {
    .bus_width = SDC_MODE_4BIT,
    .sdclk_freq_max_hz = 50000000U,
};

void drv_sd_hal_init(void) {
    sdcObjectInit(sdcd);
    sdcStart(sdcd, &sd_cfg);
}

void drv_sd_hal_deinit(void) {
    sdcStop(sdcd);
}

bool drv_sd_hal_is_card_present(void) {
    return sdcIsCardInserted(sdcd) == true;
}

static sd_hal_status_t drv_sd_hal_translate(eventflags_t flags) {
    if (flags & SDC_CARD_REMOVED_EVENT) {
        return SD_HAL_NO_CARD;
    }
    if (flags & (SDC_EVENT_CRCFAIL | SDC_EVENT_RXOVERRUN | SDC_EVENT_TXUNDERRUN)) {
        return SD_HAL_CRC;
    }
    if (flags & SDC_EVENT_TIMEOUT) {
        return SD_HAL_TIMEOUT;
    }
    return SD_HAL_ERROR;
}

sd_hal_status_t drv_sd_hal_read_blocks(uint8_t *buffer, uint32_t sector, uint32_t count) {
    if (!drv_sd_hal_is_card_present()) {
        return SD_HAL_NO_CARD;
    }
    if (sdcConnect(sdcd) != HAL_SUCCESS) {
        return SD_HAL_ERROR;
    }
    msg_t res = sdcRead(sdcd, sector, buffer, count);
    sd_hal_status_t status = SD_HAL_OK;
    if (res != HAL_SUCCESS) {
        status = drv_sd_hal_translate(sdcd->errors);
    }
    sdcDisconnect(sdcd);
    return status;
}

sd_hal_status_t drv_sd_hal_write_blocks(const uint8_t *buffer, uint32_t sector, uint32_t count) {
    if (!drv_sd_hal_is_card_present()) {
        return SD_HAL_NO_CARD;
    }
    if (sdcConnect(sdcd) != HAL_SUCCESS) {
        return SD_HAL_ERROR;
    }
    msg_t res = sdcWrite(sdcd, sector, buffer, count);
    sd_hal_status_t status = SD_HAL_OK;
    if (res != HAL_SUCCESS) {
        status = drv_sd_hal_translate(sdcd->errors);
    }
    sdcDisconnect(sdcd);
    return status;
}

void drv_sd_hal_sync(void) {
    (void)sdcSync(sdcd);
}
