/**
 * @file usbh_platform_chibios_h7.c
 * @brief Real USB OTG FS host binding for STM32H743 using ChibiOS.
 */

#include "usbh_platform_chibios_h7.h"
#include "usbh_ioreq.h"
#include <stddef.h>
#include <stdint.h>

#ifndef assert_param
#define assert_param(expr) ((void)0U)
#endif

static HCD_HandleTypeDef hhcd_USB_OTG_FS;
static volatile uint32_t port_reset_count = 0U;
static const uint32_t cache_line_size = 32U;

static void usbh_dcache_clean(const uint8_t *address, uint32_t length)
{
  if ((address == NULL) || (length == 0U))
  {
    return;
  }

  uintptr_t aligned_addr = ((uintptr_t)address) & ~(cache_line_size - 1U);
  uint32_t aligned_length = (uint32_t)((((uintptr_t)address + length + cache_line_size - 1U) &
                                       ~(cache_line_size - 1U)) - aligned_addr);

  SCB_CleanDCache_by_Addr((uint32_t *)aligned_addr, (int32_t)aligned_length);
}

static void usbh_dcache_invalidate(uint8_t *address, uint32_t length)
{
  if ((address == NULL) || (length == 0U))
  {
    return;
  }

  uintptr_t aligned_addr = ((uintptr_t)address) & ~(cache_line_size - 1U);
  uint32_t aligned_length = (uint32_t)((((uintptr_t)address + length + cache_line_size - 1U) &
                                       ~(cache_line_size - 1U)) - aligned_addr);

  SCB_InvalidateDCache_by_Addr((uint32_t *)aligned_addr, (int32_t)aligned_length);
}

static void MX_USB_OTG_FS_HCD_Init(HCD_HandleTypeDef *hhcd)
{
  hhcd->Instance = USB_OTG_FS;
  hhcd->Init.dev_endpoints = 8U;
  hhcd->Init.Host_channels = 8U;
  hhcd->Init.dma_enable = DISABLE;
  hhcd->Init.speed = HCD_SPEED_FULL;
  hhcd->Init.phy_itface = HCD_PHY_EMBEDDED;
  hhcd->Init.Sof_enable = ENABLE;
  hhcd->Init.low_power_enable = DISABLE;
  hhcd->Init.lpm_enable = DISABLE;
  hhcd->Init.battery_charging_enable = DISABLE;
  hhcd->Init.vbus_sensing_enable = DISABLE;
  hhcd->Init.use_dedicated_ep1 = DISABLE;
  hhcd->Init.use_external_vbus = DISABLE;
  hhcd->Init.ep0_mps = 64U;
}

void HAL_HCD_MspInit(HCD_HandleTypeDef *hhcd)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (hhcd->Instance == USB_OTG_FS)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    /* PA11 -> USB_DM, PA12 -> USB_DP */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(OTG_FS_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);

    HAL_PWREx_EnableUSBVoltageDetector();
  }
}

void HAL_HCD_MspDeInit(HCD_HandleTypeDef *hhcd)
{
  if (hhcd->Instance == USB_OTG_FS)
  {
    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
  }
}

USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost)
{
  phost->pData = &hhcd_USB_OTG_FS;
  hhcd_USB_OTG_FS.pData = phost;

  MX_USB_OTG_FS_HCD_Init(&hhcd_USB_OTG_FS);

  if (HAL_HCD_Init(&hhcd_USB_OTG_FS) != HAL_OK)
  {
    return USBH_FAIL;
  }

  /* ---- USB OTG FS FIFO CONFIGURATION (Direct register access, HAL H7 safe) ---- */

  USB_OTG_GlobalTypeDef *USBx = hhcd_USB_OTG_FS.Instance;

  /* Rx FIFO size (in 32-bit words) */
  USBx->GRXFSIZ = 0x80U;

  /* Non-periodic Tx FIFO (EP0 / Control) */
  USBx->DIEPTXF0_HNPTXFSIZ = (0x40U << 16) | 0U;

  /* Host periodic Tx FIFO */
  USBx->HPTXFSIZ = (0x80U << 16) | 0x40U;

  /* Flush all Tx FIFOs */
  USBx->GRSTCTL = USB_OTG_GRSTCTL_TXFFLSH | (0x10U << 6);
  while (USBx->GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH) {}

  /* Flush Rx FIFO */
  USBx->GRSTCTL = USB_OTG_GRSTCTL_RXFFLSH;
  while (USBx->GRSTCTL & USB_OTG_GRSTCTL_RXFFLSH) {}



  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost)
{
  (void)phost;
  if (HAL_HCD_DeInit(&hhcd_USB_OTG_FS) != HAL_OK)
  {
    return USBH_FAIL;
  }
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost)
{
  (void)phost;
  if (HAL_HCD_Start(&hhcd_USB_OTG_FS) != HAL_OK)
  {
    return USBH_FAIL;
  }
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost)
{
  (void)phost;
  if (HAL_HCD_Stop(&hhcd_USB_OTG_FS) != HAL_OK)
  {
    return USBH_FAIL;
  }
  return USBH_OK;
}

USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost)
{
  uint32_t speed = HAL_HCD_GetCurrentSpeed((HCD_HandleTypeDef *)phost->pData);
  if (speed == HCD_DEVICE_SPEED_LOW)
  {
    return USBH_SPEED_LOW;
  }
  return USBH_SPEED_FULL;
}

USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost)
{
  port_reset_count++;
  if (HAL_HCD_ResetPort((HCD_HandleTypeDef *)phost->pData) != HAL_OK)
  {
    return USBH_FAIL;
  }
  return USBH_OK;
}

uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  return HAL_HCD_HC_GetXferCount((HCD_HandleTypeDef *)phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state)
{
  (void)phost;

  if (state == 0U)
  {
    /* Nominally "turn off" VBUS.
     * On this board, VBUS is always powered and there is no control GPIO,
     * so we only give some time for the bus to settle.
     */
    chThdSleepMilliseconds(5);
  }
  else
  {
    /* Nominally "turn on" VBUS.
     * VBUS is always present here, but the USB Host stack expects a delay
     * before starting the port reset and enumeration sequence.
     */
    chThdSleepMilliseconds(100);
  }

  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost, uint8_t pipe,
                                    uint8_t epnum, uint8_t dev_address,
                                    uint8_t speed, uint8_t ep_type,
                                    uint16_t mps)
{
  if (HAL_HCD_HC_Init((HCD_HandleTypeDef *)phost->pData, pipe, epnum,
                      dev_address, speed, ep_type, mps) != HAL_OK)
  {
    return USBH_FAIL;
  }
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  if (HAL_HCD_HC_Halt((HCD_HandleTypeDef *)phost->pData, pipe) != HAL_OK)
  {
    return USBH_FAIL;
  }
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost, uint8_t pipe,
                                     uint8_t direction, uint8_t ep_type,
                                     uint8_t token, uint8_t *pbuff,
                                     uint16_t length, uint8_t do_ping)
{
  if (direction == 0U)
  {
    usbh_dcache_clean(pbuff, length);
  }
  else
  {
    usbh_dcache_invalidate(pbuff, length);
  }

  if (HAL_HCD_HC_SubmitRequest((HCD_HandleTypeDef *)phost->pData, pipe,
                               direction, ep_type, token, pbuff, length,
                               do_ping) != HAL_OK)
  {
    return USBH_FAIL;
  }
  return USBH_OK;
}

USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  HCD_HandleTypeDef *hhcd = (HCD_HandleTypeDef *)phost->pData;
  HCD_URBStateTypeDef urb_state = HAL_HCD_HC_GetURBState(hhcd, pipe);

  if ((urb_state == URB_DONE) && (hhcd->hc[pipe].ep_is_in != 0U))
  {
    usbh_dcache_invalidate((uint8_t *)hhcd->hc[pipe].xfer_buff, hhcd->hc[pipe].xfer_len);
  }

  switch (urb_state)
  {
  case URB_IDLE:
    return USBH_URB_IDLE;
  case URB_DONE:
    return USBH_URB_DONE;
  case URB_NOTREADY:
    return USBH_URB_NOTREADY;
  case URB_NYET:
    return USBH_URB_NYET;
  case URB_ERROR:
    return USBH_URB_ERROR;
  case URB_STALL:
    return USBH_URB_STALL;
  default:
    return USBH_URB_ERROR;
  }
}

USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle)
{
  HCD_HandleTypeDef *hhcd = (HCD_HandleTypeDef *)phost->pData;
  if ((phost->Pipes[pipe] & 0x80U) == 0U)
  {
    hhcd->hc[pipe].toggle_out = toggle;
  }
  else
  {
    hhcd->hc[pipe].toggle_in = toggle;
  }
  return USBH_OK;
}

uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  HCD_HandleTypeDef *hhcd = (HCD_HandleTypeDef *)phost->pData;
  if ((phost->Pipes[pipe] & 0x80U) == 0U)
  {
    return hhcd->hc[pipe].toggle_out;
  }
  else
  {
    return hhcd->hc[pipe].toggle_in;
  }
}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_HandleTypeDef *phost = (USBH_HandleTypeDef *)hhcd->pData;
  USBH_LL_IncTimer(phost);
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_HandleTypeDef *phost = (USBH_HandleTypeDef *)hhcd->pData;
  USBH_LL_Connect(phost);
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_HandleTypeDef *phost = (USBH_HandleTypeDef *)hhcd->pData;
  USBH_LL_Disconnect(phost);
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_HandleTypeDef *phost = (USBH_HandleTypeDef *)hhcd->pData;
  USBH_LL_PortEnabled(phost);
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_HandleTypeDef *phost = (USBH_HandleTypeDef *)hhcd->pData;
  USBH_LL_PortDisabled(phost);
}

void OTG_FS_IRQHandler(void)
{
  HAL_HCD_IRQHandler(&hhcd_USB_OTG_FS);
}

void USBH_Delay(uint32_t Delay)
{
  chThdSleepMilliseconds(Delay);
}

uint32_t usbh_platform_get_reset_count(void)
{
  return port_reset_count;
}
