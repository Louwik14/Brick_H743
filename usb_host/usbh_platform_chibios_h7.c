/**
 * @file usbh_platform_chibios_h7.c
 * @brief Low-level driver bridging STM32H7 OTG FS to the USB Host library on ChibiOS.
 */

#include "usbh_platform_chibios_h7.h"
#include "stm32h7xx_hal.h"
#include <stdbool.h>
#include <string.h>

#define USBH_H7_OTG_FS_IRQn      OTG_FS_IRQn
#define USBH_H7_OTG_FS_HANDLER   OTG_FS_IRQHandler
#define USBH_H7_OTG_FS_INSTANCE  USB_OTG_FS
#define USBH_H7_HOST_CHANNELS    12U
#define USBH_H7_RX_FIFO_SIZE     0x80U
#define USBH_H7_NP_TX_FIFO_SIZE  0x40U
#define USBH_H7_P_TX_FIFO_SIZE   0x10U

static HCD_HandleTypeDef hhcd;
static USBH_HandleTypeDef *registered_host = NULL;
static volatile USBH_URBStateTypeDef urb_state[USBH_MAX_PIPES];
static volatile uint32_t last_xfer_size[USBH_MAX_PIPES];
static uint8_t toggle_states[USBH_MAX_PIPES];
static bool vbus_gpio_ready = false;

static void USBH_H7_GPIOInit(void);
static void USBH_H7_GPIODeInit(void);
static void USBH_H7_EnableClocks(void);
static void USBH_H7_DisableClocks(void);
static void USBH_H7_ConfigVBUSGPIO(void);

USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost)
{
  if (phost == NULL)
  {
    return USBH_FAIL;
  }

  registered_host = phost;
  (void)memset((void *)urb_state, 0, sizeof(urb_state));
  (void)memset((void *)last_xfer_size, 0, sizeof(last_xfer_size));
  (void)memset(toggle_states, 0, sizeof(toggle_states));

  USBH_H7_EnableClocks();
  USBH_H7_GPIOInit();

  hhcd.Instance = USBH_H7_OTG_FS_INSTANCE;
  hhcd.Init.Host_channels = USBH_H7_HOST_CHANNELS;
  hhcd.Init.speed = HCD_SPEED_FULL;
  hhcd.Init.dma_enable = DISABLE;
  hhcd.Init.phy_itface = HCD_PHY_EMBEDDED;
  hhcd.Init.Sof_enable = ENABLE;
  hhcd.Init.low_power_enable = DISABLE;
  hhcd.Init.lpm_enable = DISABLE;
  hhcd.Init.vbus_sensing_enable = DISABLE;
  hhcd.Init.use_external_vbus = ENABLE;

  hhcd.pData = phost;
  phost->pData = &hhcd;

  if (HAL_HCD_Init(&hhcd) != HAL_OK)
  {
    return USBH_FAIL;
  }

  (void)HAL_HCDEx_SetRxFiFo(&hhcd, USBH_H7_RX_FIFO_SIZE);
  (void)HAL_HCDEx_SetTxFiFo(&hhcd, 0, USBH_H7_NP_TX_FIFO_SIZE);
  (void)HAL_HCDEx_SetTxFiFo(&hhcd, 1, USBH_H7_P_TX_FIFO_SIZE);

  USBH_LL_SetTimer(phost, HAL_HCD_GetCurrentFrame(&hhcd));

  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost)
{
  if ((phost == NULL) || (phost->pData != &hhcd))
  {
    return USBH_FAIL;
  }

  (void)HAL_HCD_DeInit(&hhcd);
  USBH_H7_GPIODeInit();
  USBH_H7_DisableClocks();
  registered_host = NULL;
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost)
{
  if ((phost == NULL) || (phost->pData != &hhcd))
  {
    return USBH_FAIL;
  }

  (void)USBH_LL_DriverVBUS(phost, 1U);
  (void)HAL_HCD_Start(&hhcd);
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost)
{
  if ((phost == NULL) || (phost->pData != &hhcd))
  {
    return USBH_FAIL;
  }

  (void)HAL_HCD_Stop(&hhcd);
  (void)USBH_LL_DriverVBUS(phost, 0U);
  return USBH_OK;
}

USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost)
{
  (void)phost;
  switch (HAL_HCD_GetCurrentSpeed(&hhcd))
  {
    case HCD_SPEED_HIGH:
      return USBH_SPEED_HIGH;
    case HCD_SPEED_LOW:
      return USBH_SPEED_LOW;
    case HCD_SPEED_FULL:
    default:
      return USBH_SPEED_FULL;
  }
}

USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost)
{
  if ((phost == NULL) || (phost->pData != &hhcd))
  {
    return USBH_FAIL;
  }

  (void)HAL_HCD_ResetPort(&hhcd);
  return USBH_OK;
}

uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  (void)phost;
  if (pipe < USBH_MAX_PIPES)
  {
    return last_xfer_size[pipe];
  }
  return 0U;
}

USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t epnum,
                                    uint8_t dev_address, uint8_t speed, uint8_t ep_type,
                                    uint16_t mps)
{
  if ((phost == NULL) || (phost->pData != &hhcd) || (pipe >= USBH_MAX_PIPES))
  {
    return USBH_FAIL;
  }

  if (HAL_HCD_HC_Init(&hhcd, pipe, epnum, dev_address, speed, ep_type, mps) != HAL_OK)
  {
    return USBH_FAIL;
  }

  urb_state[pipe] = USBH_URB_IDLE;
  toggle_states[pipe] = 0U;
  last_xfer_size[pipe] = 0U;
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  if ((phost == NULL) || (phost->pData != &hhcd) || (pipe >= USBH_MAX_PIPES))
  {
    return USBH_FAIL;
  }

  (void)HAL_HCD_HC_Halt(&hhcd, pipe);
  urb_state[pipe] = USBH_URB_IDLE;
  last_xfer_size[pipe] = 0U;
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_ActivatePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  if ((phost == NULL) || (phost->pData != &hhcd) || (pipe >= USBH_MAX_PIPES))
  {
    return USBH_FAIL;
  }

  urb_state[pipe] = USBH_URB_IDLE;
  last_xfer_size[pipe] = 0U;
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t direction,
                                     uint8_t ep_type, uint8_t token, uint8_t *pbuff,
                                     uint16_t length, uint8_t do_ping)
{
  (void)do_ping;

  if ((phost == NULL) || (phost->pData != &hhcd) || (pipe >= USBH_MAX_PIPES))
  {
    return USBH_FAIL;
  }

  urb_state[pipe] = USBH_URB_IDLE;
  last_xfer_size[pipe] = 0U;

  if (HAL_HCD_HC_SubmitRequest(&hhcd, pipe, direction, ep_type, token, pbuff, length, do_ping) != HAL_OK)
  {
    urb_state[pipe] = USBH_URB_ERROR;
    return USBH_FAIL;
  }

  return USBH_OK;
}

USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  (void)phost;
  if (pipe < USBH_MAX_PIPES)
  {
    return urb_state[pipe];
  }
  return USBH_URB_ERROR;
}

USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state)
{
  (void)phost;
  if (!vbus_gpio_ready)
  {
    USBH_H7_ConfigVBUSGPIO();
  }
  if (state != 0U)
  {
    HAL_GPIO_WritePin(BOARD_USB_VBUS_PORT, BOARD_USB_VBUS_PIN, BOARD_USB_VBUS_ACTIVE_STATE);
  }
  else
  {
    HAL_GPIO_WritePin(BOARD_USB_VBUS_PORT, BOARD_USB_VBUS_PIN, BOARD_USB_VBUS_INACTIVE_STATE);
  }
  return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle)
{
  (void)phost;
  if (pipe < USBH_MAX_PIPES)
  {
    toggle_states[pipe] = toggle;
    return USBH_OK;
  }
  return USBH_FAIL;
}

uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  (void)phost;
  if (pipe < USBH_MAX_PIPES)
  {
    return toggle_states[pipe];
  }
  return 0U;
}

static void USBH_H7_ConfigVBUSGPIO(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin = BOARD_USB_VBUS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BOARD_USB_VBUS_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(BOARD_USB_VBUS_PORT, BOARD_USB_VBUS_PIN, BOARD_USB_VBUS_INACTIVE_STATE);
  vbus_gpio_ready = true;
}

static void USBH_H7_EnableClocks(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  HAL_PWREx_EnableUSBVoltageDetector();
#ifdef USBH_USE_FS_PORT
  __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
#endif
}

static void USBH_H7_DisableClocks(void)
{
#ifdef USBH_USE_FS_PORT
  __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
#endif
}

static void USBH_H7_GPIOInit(void)
{
#ifdef USBH_USE_FS_PORT
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BOARD_USB_VBUS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BOARD_USB_VBUS_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(BOARD_USB_VBUS_PORT, BOARD_USB_VBUS_PIN, BOARD_USB_VBUS_INACTIVE_STATE);
  vbus_gpio_ready = true;

  HAL_NVIC_SetPriority(USBH_H7_OTG_FS_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USBH_H7_OTG_FS_IRQn);
#endif
}

static void USBH_H7_GPIODeInit(void)
{
#ifdef USBH_USE_FS_PORT
  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
  HAL_GPIO_DeInit(BOARD_USB_VBUS_PORT, BOARD_USB_VBUS_PIN);
  vbus_gpio_ready = false;
  HAL_NVIC_DisableIRQ(USBH_H7_OTG_FS_IRQn);
#endif
}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd_local)
{
  if (registered_host != NULL)
  {
    USBH_LL_IncTimer(registered_host);
  }
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd_local)
{
  if (registered_host != NULL)
  {
    USBH_LL_Connect(registered_host);
  }
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd_local)
{
  if (registered_host != NULL)
  {
    USBH_LL_Disconnect(registered_host);
  }
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd_local, uint8_t chnum, HCD_URBStateTypeDef state)
{
  (void)hhcd_local;
  if (chnum < USBH_MAX_PIPES)
  {
    last_xfer_size[chnum] = HAL_HCD_HC_GetXferCount(&hhcd, chnum);
    switch (state)
    {
      case URB_IDLE:      urb_state[chnum] = USBH_URB_IDLE; break;
      case URB_DONE:      urb_state[chnum] = USBH_URB_DONE; break;
      case URB_NOTREADY:  urb_state[chnum] = USBH_URB_NOTREADY; break;
      case URB_NYET:      urb_state[chnum] = USBH_URB_NYET; break;
      case URB_ERROR:     urb_state[chnum] = USBH_URB_ERROR; break;
      case URB_STALL:     urb_state[chnum] = USBH_URB_STALL; break;
      default:            urb_state[chnum] = USBH_URB_ERROR; break;
    }
  }
}

void HAL_HCD_MspInit(HCD_HandleTypeDef *hcd)
{
  (void)hcd;
}

void HAL_HCD_MspDeInit(HCD_HandleTypeDef *hcd)
{
  (void)hcd;
}

void USBH_H7_OTG_FS_HANDLER(void)
{
  HAL_HCD_IRQHandler(&hhcd);
}

