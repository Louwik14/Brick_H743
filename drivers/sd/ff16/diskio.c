/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs (C)ChaN, 2025                     */
/*-----------------------------------------------------------------------*/
/* Glue functions to attach the SDMMC driver to FatFS using ChibiOS HAL. */
/*-----------------------------------------------------------------------*/

#include "ff.h"                 /* Basic definitions of FatFs */
#include "diskio.h"             /* Declarations FatFs API */
#include "ch.h"
#include "hal.h"
#include "drv_sd_hal.h"

#define DEV_MMC 0U

static DSTATUS sd_status = STA_NOINIT;

static DRESULT sd_map_status(sd_hal_status_t status) {
    switch (status) {
    case SD_HAL_OK:
        return RES_OK;
    case SD_HAL_NO_CARD:
        return RES_NOTRDY;
    case SD_HAL_CRC:
    case SD_HAL_TIMEOUT:
    case SD_HAL_ERROR:
    default:
        return RES_ERROR;
    }
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != DEV_MMC) {
        return STA_NOINIT;
    }

    if (!drv_sd_hal_is_card_present()) {
        sd_status = STA_NOINIT | STA_NODISK;
    }

    return sd_status;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != DEV_MMC) {
        return STA_NOINIT;
    }

    drv_sd_hal_init();

    if (!drv_sd_hal_is_card_present()) {
        sd_status = STA_NOINIT | STA_NODISK;
        return sd_status;
    }

    sd_hal_status_t status = drv_sd_hal_connect();
    if (status != SD_HAL_OK) {
        sd_status = STA_NOINIT;
        return sd_status;
    }

    sd_status = 0U;
    return sd_status;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != DEV_MMC || buff == NULL || count == 0U) {
        return RES_PARERR;
    }

    if (!drv_sd_hal_is_card_present()) {
        sd_status = STA_NOINIT | STA_NODISK;
        return RES_NOTRDY;
    }

    (void)drv_sd_hal_connect();

    sd_hal_status_t status = drv_sd_hal_read_blocks(buff, (uint32_t)sector, (uint32_t)count);
    return sd_map_status(status);
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != DEV_MMC || buff == NULL || count == 0U) {
        return RES_PARERR;
    }

    if (!drv_sd_hal_is_card_present()) {
        sd_status = STA_NOINIT | STA_NODISK;
        return RES_NOTRDY;
    }

    (void)drv_sd_hal_connect();

    sd_hal_status_t status = drv_sd_hal_write_blocks(buff, (uint32_t)sector, (uint32_t)count);
    return sd_map_status(status);
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    if (pdrv != DEV_MMC) {
        return RES_PARERR;
    }

    if (!drv_sd_hal_is_card_present()) {
        return RES_NOTRDY;
    }

    if ((buff == NULL) && (cmd != CTRL_SYNC)) {
        return RES_PARERR;
    }

    switch (cmd) {
    case CTRL_SYNC:
        drv_sd_hal_sync();
        return RES_OK;
    case GET_SECTOR_COUNT: {
        BlockDeviceInfo info;
        if (drv_sd_hal_get_info(&info) != SD_HAL_OK) {
            return RES_ERROR;
        }
        *((LBA_t *)buff) = (LBA_t)info.blk_num;
        return RES_OK;
    }
    case GET_SECTOR_SIZE:
        *((WORD *)buff) = (WORD)MMCSD_BLOCK_SIZE;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *((DWORD *)buff) = 1U;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}

