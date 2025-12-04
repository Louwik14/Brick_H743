/**
 * @file drv_sd_hal.c
 * @brief Implémentation bas niveau HAL SDMMC pour STM32H743.
 */

#include "drv_sd_hal.h"
#include "drv_sd.h"

static SDCDriver *sdcd = &SDCD1;
static const SDCConfig sd_cfg = {
    .bus_width = SDC_MODE_4BIT,
    .slowdown = 0U,
};
static bool sd_hal_initialized = false;
static bool sd_connected = false;

void drv_sd_hal_init(void) {
    if (!sd_hal_initialized) {
        sdcObjectInit(sdcd);
        sdcStart(sdcd, &sd_cfg);
        sd_hal_initialized = true;
    }
}

void drv_sd_hal_deinit(void) {
    drv_sd_hal_sync();
    sdcStop(sdcd);
}

bool drv_sd_hal_is_card_present(void) {
    return sdcIsCardInserted(sdcd) == true;
}

sd_hal_status_t drv_sd_hal_connect(void) {
    if (sd_connected) {
        return SD_HAL_OK;
    }
    if (!drv_sd_hal_is_card_present()) {
        return SD_HAL_NO_CARD;
    }
    if (sdcConnect(sdcd) != HAL_SUCCESS) {
        return SD_HAL_ERROR;
    }
    sd_connected = true;
    return SD_HAL_OK;
}

void drv_sd_hal_disconnect(void) {
    if (!sd_connected) {
        return;
    }
    sdcDisconnect(sdcd);
    sd_connected = false;
}

static sd_hal_status_t drv_sd_hal_translate(sdcflags_t flags) {
    if (flags == SDC_NO_ERROR) {
        return SD_HAL_OK;
    }
    if ((flags & (SDC_CMD_CRC_ERROR | SDC_DATA_CRC_ERROR)) != 0U) {
        return SD_HAL_CRC;
    }
    if ((flags & (SDC_DATA_TIMEOUT | SDC_COMMAND_TIMEOUT)) != 0U) {
        return SD_HAL_TIMEOUT;
    }
    if ((flags & (SDC_RX_OVERRUN | SDC_TX_UNDERRUN | SDC_OVERFLOW_ERROR | SDC_STARTBIT_ERROR | SDC_UNHANDLED_ERROR)) != 0U) {
        return SD_HAL_ERROR;
    }
    return SD_HAL_ERROR;
}

sd_hal_status_t drv_sd_hal_read_blocks(uint8_t *buffer, uint32_t sector, uint32_t count) {
    if (!drv_sd_hal_is_card_present() || !sd_connected) {
        return SD_HAL_NO_CARD;
    }
    msg_t res = sdcRead(sdcd, sector, buffer, count);
    sdcflags_t errors = sdcGetAndClearErrors(sdcd);
    if ((res == HAL_SUCCESS) && (errors == SDC_NO_ERROR)) {
        return SD_HAL_OK;
    }
    return drv_sd_hal_translate(errors);
}

sd_hal_status_t drv_sd_hal_write_blocks(const uint8_t *buffer, uint32_t sector, uint32_t count) {
    if (!drv_sd_hal_is_card_present() || !sd_connected) {
        return SD_HAL_NO_CARD;
    }
    msg_t res = sdcWrite(sdcd, sector, buffer, count);
    sdcflags_t errors = sdcGetAndClearErrors(sdcd);
    if ((res == HAL_SUCCESS) && (errors == SDC_NO_ERROR)) {
        return SD_HAL_OK;
    }
    return drv_sd_hal_translate(errors);
}

void drv_sd_hal_sync(void) {
    (void)sdcSync(sdcd);
}

sd_hal_status_t drv_sd_hal_get_info(BlockDeviceInfo *info) {
    if (info == NULL) {
        return SD_HAL_ERROR;
    }
    if (!drv_sd_hal_is_card_present() || !sd_connected) {
        return SD_HAL_NO_CARD;
    }
    if (sdcGetInfo(sdcd, info) != HAL_SUCCESS) {
        return SD_HAL_ERROR;
    }
    return SD_HAL_OK;
}
