/**
 * @file usbh_platform_chibios_h7.c
 * @brief Template low-level driver to bridge STM32H7 OTG to the USB Host library on ChibiOS.
 *
 * This file intentionally keeps the hardware layer minimal. The glue must be
 * completed with the actual board USB OTG configuration (pins, VBUS drive,
 * over-current monitoring and the ChibiOS USBH driver instance). The current
 * implementation provides a software placeholder so that upper layers can be
 * integrated and tested without crashing, while clearly marking the sections
 * that must be replaced on real hardware.
 */

#include "usbh_platform_chibios_h7.h"
#include <string.h>

/* Local placeholders ---------------------------------------------------------*/
typedef struct
{
  USBH_HandleTypeDef *phost;
  USBH_SpeedTypeDef   speed;
  USBH_URBStateTypeDef urb_state[USBH_MAX_PIPES];
  uint32_t             xfer_size[USBH_MAX_PIPES];
  uint8_t              toggle[USBH_MAX_PIPES];
} USBH_ChibiosH7_LLContext;

static USBH_ChibiosH7_LLContext ll_ctx;

/* Low-level hooks ------------------------------------------------------------*/
USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost)
{
  (void)memset(&ll_ctx, 0, sizeof(ll_ctx));
  ll_ctx.phost = phost;
  ll_ctx.speed = USBH_SPEED_FULL;

  phost->pData = &ll_ctx;

  /* TODO: configure USBHDriver instance, GPIOs for VBUS, ID, OC and clocks. */
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost)
{
  (void)phost;
  (void)memset(&ll_ctx, 0, sizeof(ll_ctx));
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost)
{
  (void)phost;
  /* TODO: start host controller through ChibiOS USBH driver. */
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost)
{
  (void)phost;
  /* TODO: stop host controller safely. */
  return USBH_OK;
}

USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost)
{
  (void)phost;
  return ll_ctx.speed;
}

USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost)
{
  (void)phost;
  /* TODO: drive port reset using OTG registers. */
  return USBH_OK;
}

uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  (void)phost;
  if (pipe < USBH_MAX_PIPES)
  {
    return ll_ctx.xfer_size[pipe];
  }
  return 0U;
}

USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t epnum,
                                    uint8_t dev_address, uint8_t speed, uint8_t ep_type,
                                    uint16_t mps)
{
  (void)phost;
  (void)epnum;
  (void)dev_address;
  (void)speed;
  (void)ep_type;
  (void)mps;

  if (pipe < USBH_MAX_PIPES)
  {
    ll_ctx.urb_state[pipe] = USBH_URB_IDLE;
    ll_ctx.xfer_size[pipe] = 0U;
    ll_ctx.toggle[pipe] = 0U;
  }

  /* TODO: configure host channel using the ChibiOS HCD wrapper. */
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  (void)phost;
  if (pipe < USBH_MAX_PIPES)
  {
    ll_ctx.urb_state[pipe] = USBH_URB_IDLE;
  }
  /* TODO: disable host channel. */
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_ActivatePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  (void)phost;
  if (pipe < USBH_MAX_PIPES)
  {
    ll_ctx.urb_state[pipe] = USBH_URB_IDLE;
  }
  /* TODO: enable host channel and trigger transfer. */
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t direction,
                                     uint8_t ep_type, uint8_t token, uint8_t *pbuff,
                                     uint16_t length, uint8_t do_ping)
{
  (void)phost;
  (void)direction;
  (void)ep_type;
  (void)token;
  (void)pbuff;
  (void)length;
  (void)do_ping;

  if (pipe < USBH_MAX_PIPES)
  {
    /* Placeholder: mark transfer as done without real hardware I/O. */
    ll_ctx.urb_state[pipe] = USBH_URB_DONE;
    ll_ctx.xfer_size[pipe] = length;
  }

  /* TODO: use USBHDriver API to queue URB to the OTG controller. */
  return USBH_OK;
}

USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  (void)phost;
  if (pipe < USBH_MAX_PIPES)
  {
    return ll_ctx.urb_state[pipe];
  }
  return USBH_URB_ERROR;
}

USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state)
{
  (void)phost;
  (void)state;
  /* TODO: drive VBUS switch GPIO. */
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle)
{
  (void)phost;
  if (pipe < USBH_MAX_PIPES)
  {
    ll_ctx.toggle[pipe] = toggle;
  }
  return USBH_OK;
}

uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  (void)phost;
  if (pipe < USBH_MAX_PIPES)
  {
    return ll_ctx.toggle[pipe];
  }
  return 0U;
}

