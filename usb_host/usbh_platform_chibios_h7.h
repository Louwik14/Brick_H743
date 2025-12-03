/**
 * @file usbh_platform_chibios_h7.h
 * @brief Low-level USB Host glue for STM32H7 running ChibiOS.
 */

#ifndef USBH_PLATFORM_CHIBIOS_H7_H
#define USBH_PLATFORM_CHIBIOS_H7_H

#include "hal.h"
#include "usbh_core.h"

#ifdef __cplusplus
extern "C" {
#endif

USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost);
USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost);
USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost);
USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost);
USBH_SpeedTypeDef  USBH_LL_GetSpeed(USBH_HandleTypeDef *phost);
USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost);
uint32_t           USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe);
USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t epnum,
                                    uint8_t dev_address, uint8_t speed, uint8_t ep_type,
                                    uint16_t mps);
USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe);
USBH_StatusTypeDef USBH_LL_ActivatePipe(USBH_HandleTypeDef *phost, uint8_t pipe);
USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t direction,
                                     uint8_t ep_type, uint8_t token, uint8_t *pbuff,
                                     uint16_t length, uint8_t do_ping);
USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe);
USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state);
USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle);
uint8_t             USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe);

#ifdef __cplusplus
}
#endif

#endif /* USBH_PLATFORM_CHIBIOS_H7_H */
