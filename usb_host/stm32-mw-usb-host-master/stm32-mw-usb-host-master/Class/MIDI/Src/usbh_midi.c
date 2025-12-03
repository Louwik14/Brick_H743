/**
 * @file usbh_midi.c
 * @brief USB Host MIDI class driver.
 */

#include "usbh_midi.h"
#include <string.h>

static USBH_StatusTypeDef USBH_MIDI_InterfaceInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost);

static void MIDI_ProcessReception(USBH_HandleTypeDef *phost, MIDI_HandleTypeDef *MIDI_Handle);
static void MIDI_ProcessTransmission(USBH_HandleTypeDef *phost, MIDI_HandleTypeDef *MIDI_Handle);
static void MIDI_ResetBuffers(MIDI_HandleTypeDef *MIDI_Handle);
static bool MIDI_BufferPush(uint8_t buffer[][USBH_MIDI_EVENT_SIZE], uint16_t size,
                            uint16_t *head, uint16_t *tail,
                            const uint8_t *packet, uint32_t *overflow);
static bool MIDI_BufferPop(uint8_t buffer[][USBH_MIDI_EVENT_SIZE], uint16_t size,
                           uint16_t *head, uint16_t *tail, uint8_t *packet);

static MIDI_HandleTypeDef MIDI_Handle;

USBH_ClassTypeDef USBH_MIDI_Class =
{
  "MIDI",
  USB_AUDIO_CLASS,
  USBH_MIDI_InterfaceInit,
  USBH_MIDI_InterfaceDeInit,
  USBH_MIDI_ClassRequest,
  USBH_MIDI_Process,
  NULL,
  NULL
};

bool USBH_MIDI_IsReady(USBH_HandleTypeDef *phost)
{
  return (phost->gState == HOST_CLASS) && (phost->pActiveClass == &USBH_MIDI_Class);
}

bool USBH_MIDI_ReadEvent(USBH_HandleTypeDef *phost, uint8_t *packet4)
{
  if ((phost == NULL) || (packet4 == NULL) || (phost->pActiveClass != &USBH_MIDI_Class))
  {
    return false;
  }
  MIDI_HandleTypeDef *handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  return MIDI_BufferPop(handle->rx_buffer, USBH_MIDI_RX_QUEUE_SIZE,
                        &handle->rx_head, &handle->rx_tail, packet4);
}

bool USBH_MIDI_WriteEvent(USBH_HandleTypeDef *phost, const uint8_t *packet4)
{
  if ((phost == NULL) || (packet4 == NULL) || (phost->pActiveClass != &USBH_MIDI_Class))
  {
    return false;
  }
  MIDI_HandleTypeDef *handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  return MIDI_BufferPush(handle->tx_buffer, USBH_MIDI_TX_QUEUE_SIZE,
                         &handle->tx_head, &handle->tx_tail, packet4,
                         &handle->tx_overflow);
}

uint32_t USBH_MIDI_GetRxOverflow(USBH_HandleTypeDef *phost)
{
  if ((phost == NULL) || (phost->pActiveClass != &USBH_MIDI_Class))
  {
    return 0U;
  }
  MIDI_HandleTypeDef *handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  return handle->rx_overflow;
}

uint32_t USBH_MIDI_GetTxOverflow(USBH_HandleTypeDef *phost)
{
  if ((phost == NULL) || (phost->pActiveClass != &USBH_MIDI_Class))
  {
    return 0U;
  }
  MIDI_HandleTypeDef *handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  return handle->tx_overflow;
}

void USBH_MIDI_Flush(USBH_HandleTypeDef *phost)
{
  if ((phost == NULL) || (phost->pActiveClass != &USBH_MIDI_Class))
  {
    return;
  }
  MIDI_HandleTypeDef *handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  MIDI_ResetBuffers(handle);
}

static USBH_StatusTypeDef USBH_MIDI_InterfaceInit(USBH_HandleTypeDef *phost)
{
  uint8_t interface;
  USBH_InterfaceDescTypeDef *itf_desc;

  interface = USBH_FindInterface(phost, USB_AUDIO_CLASS,
                                 USB_AUDIO_SUBCLASS_MIDISTREAMING, 0U);

  if (interface == 0xFFU)
  {
    USBH_DbgLog("Cannot Find MIDI Streaming interface");
    return USBH_FAIL;
  }

  itf_desc = &phost->device.CfgDesc.Itf_Desc[interface];

  if (itf_desc->bNumEndpoints < 2U)
  {
    USBH_ErrLog("MIDI interface endpoints missing");
    return USBH_FAIL;
  }

  USBH_SelectInterface(phost, interface);

  MIDI_Handle.interface = interface;
  MIDI_Handle.InEp = 0U;
  MIDI_Handle.OutEp = 0U;
  MIDI_Handle.InEpSize = 0U;
  MIDI_Handle.OutEpSize = 0U;

  for (uint8_t idx = 0U; idx < itf_desc->bNumEndpoints; idx++)
  {
    USBH_EpDescTypeDef *ep_desc = &itf_desc->Ep_Desc[idx];
    if ((ep_desc->bmAttributes & 0x03U) == USB_EP_TYPE_BULK)
    {
      if ((ep_desc->bEndpointAddress & 0x80U) == 0x80U)
      {
        MIDI_Handle.InEp = ep_desc->bEndpointAddress;
        MIDI_Handle.InEpSize = ep_desc->wMaxPacketSize;
      }
      else
      {
        MIDI_Handle.OutEp = ep_desc->bEndpointAddress;
        MIDI_Handle.OutEpSize = ep_desc->wMaxPacketSize;
      }
    }
  }

  if ((MIDI_Handle.InEp == 0U) || (MIDI_Handle.OutEp == 0U))
  {
    USBH_ErrLog("MIDI bulk endpoints not found");
    return USBH_FAIL;
  }

  MIDI_Handle.InPipe = USBH_AllocPipe(phost, MIDI_Handle.InEp);
  MIDI_Handle.OutPipe = USBH_AllocPipe(phost, MIDI_Handle.OutEp);

  USBH_OpenPipe(phost, MIDI_Handle.InPipe, MIDI_Handle.InEp,
                phost->device.address, phost->device.speed,
                USB_EP_TYPE_BULK, MIDI_Handle.InEpSize);

  USBH_OpenPipe(phost, MIDI_Handle.OutPipe, MIDI_Handle.OutEp,
                phost->device.address, phost->device.speed,
                USB_EP_TYPE_BULK, MIDI_Handle.OutEpSize);

  USBH_LL_SetToggle(phost, MIDI_Handle.InPipe, 0U);
  USBH_LL_SetToggle(phost, MIDI_Handle.OutPipe, 0U);

  MIDI_Handle.in_state = MIDI_PIPE_STATE_IDLE;
  MIDI_Handle.out_state = MIDI_PIPE_STATE_IDLE;
  MIDI_ResetBuffers(&MIDI_Handle);

  phost->pActiveClass->pData = &MIDI_Handle;

  return USBH_OK;
}

static USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit(USBH_HandleTypeDef *phost)
{
  if (MIDI_Handle.InPipe != 0U)
  {
    USBH_ClosePipe(phost, MIDI_Handle.InPipe);
    USBH_FreePipe(phost, MIDI_Handle.InPipe);
    MIDI_Handle.InPipe = 0U;
  }

  if (MIDI_Handle.OutPipe != 0U)
  {
    USBH_ClosePipe(phost, MIDI_Handle.OutPipe);
    USBH_FreePipe(phost, MIDI_Handle.OutPipe);
    MIDI_Handle.OutPipe = 0U;
  }

  MIDI_ResetBuffers(&MIDI_Handle);
  phost->pActiveClass->pData = NULL;
  return USBH_OK;
}

static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost)
{
  (void)phost;
  return USBH_OK;
}

static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;

  if (handle == NULL)
  {
    return USBH_FAIL;
  }

  if (phost->gState != HOST_CLASS)
  {
    return USBH_OK;
  }

  MIDI_ProcessReception(phost, handle);
  MIDI_ProcessTransmission(phost, handle);

  return USBH_OK;
}

static void MIDI_ProcessReception(USBH_HandleTypeDef *phost, MIDI_HandleTypeDef *MIDI_Handle)
{
  if (MIDI_Handle->in_state == MIDI_PIPE_STATE_IDLE)
  {
    if (USBH_BulkReceiveData(phost, MIDI_Handle->in_packet, MIDI_Handle->InEpSize,
                             MIDI_Handle->InPipe) == USBH_OK)
    {
      MIDI_Handle->in_state = MIDI_PIPE_STATE_BUSY;
    }
    return;
  }

  USBH_URBStateTypeDef urb_state = USBH_LL_GetURBState(phost, MIDI_Handle->InPipe);

  if (urb_state == USBH_URB_DONE)
  {
    uint32_t packet_size = USBH_LL_GetLastXferSize(phost, MIDI_Handle->InPipe);
    uint32_t offset = 0U;
    while ((offset + USBH_MIDI_EVENT_SIZE) <= packet_size)
    {
      (void)MIDI_BufferPush(MIDI_Handle->rx_buffer, USBH_MIDI_RX_QUEUE_SIZE,
                            &MIDI_Handle->rx_head, &MIDI_Handle->rx_tail,
                            &MIDI_Handle->in_packet[offset],
                            &MIDI_Handle->rx_overflow);
      offset += USBH_MIDI_EVENT_SIZE;
    }
    MIDI_Handle->in_state = MIDI_PIPE_STATE_IDLE;
  }
  else if ((urb_state == USBH_URB_ERROR) || (urb_state == USBH_URB_STALL))
  {
    MIDI_Handle->in_state = MIDI_PIPE_STATE_IDLE;
  }
  else if (urb_state == USBH_URB_NOTREADY)
  {
    MIDI_Handle->in_state = MIDI_PIPE_STATE_IDLE;
  }
}

static void MIDI_ProcessTransmission(USBH_HandleTypeDef *phost, MIDI_HandleTypeDef *MIDI_Handle)
{
  if (MIDI_Handle->out_state == MIDI_PIPE_STATE_IDLE)
  {
    uint16_t available = (uint16_t)((MIDI_Handle->tx_head + USBH_MIDI_TX_QUEUE_SIZE - MIDI_Handle->tx_tail) % USBH_MIDI_TX_QUEUE_SIZE);
    if (available == 0U)
    {
      return;
    }

    uint16_t to_send = 0U;
    while ((to_send + USBH_MIDI_EVENT_SIZE) <= MIDI_Handle->OutEpSize &&
           MIDI_Handle->tx_tail != MIDI_Handle->tx_head)
    {
      memcpy(&MIDI_Handle->out_packet[to_send],
             MIDI_Handle->tx_buffer[MIDI_Handle->tx_tail], USBH_MIDI_EVENT_SIZE);
      MIDI_Handle->tx_tail = (uint16_t)((MIDI_Handle->tx_tail + 1U) % USBH_MIDI_TX_QUEUE_SIZE);
      to_send += USBH_MIDI_EVENT_SIZE;
    }

    if (to_send > 0U)
    {
      if (USBH_BulkSendData(phost, MIDI_Handle->out_packet, to_send,
                            MIDI_Handle->OutPipe, 0U) == USBH_OK)
      {
        MIDI_Handle->out_state = MIDI_PIPE_STATE_BUSY;
      }
    }
    return;
  }

  USBH_URBStateTypeDef urb_state = USBH_LL_GetURBState(phost, MIDI_Handle->OutPipe);

  if (urb_state == USBH_URB_DONE)
  {
    MIDI_Handle->out_state = MIDI_PIPE_STATE_IDLE;
  }
  else if ((urb_state == USBH_URB_ERROR) || (urb_state == USBH_URB_STALL))
  {
    MIDI_Handle->out_state = MIDI_PIPE_STATE_IDLE;
  }
  else if (urb_state == USBH_URB_NOTREADY)
  {
    MIDI_Handle->out_state = MIDI_PIPE_STATE_IDLE;
  }
}

static void MIDI_ResetBuffers(MIDI_HandleTypeDef *MIDI_Handle)
{
  MIDI_Handle->rx_head = MIDI_Handle->rx_tail = 0U;
  MIDI_Handle->tx_head = MIDI_Handle->tx_tail = 0U;
  MIDI_Handle->rx_overflow = 0U;
  MIDI_Handle->tx_overflow = 0U;
  memset(MIDI_Handle->rx_buffer, 0, sizeof(MIDI_Handle->rx_buffer));
  memset(MIDI_Handle->tx_buffer, 0, sizeof(MIDI_Handle->tx_buffer));
}

static bool MIDI_BufferPush(uint8_t buffer[][USBH_MIDI_EVENT_SIZE], uint16_t size,
                            uint16_t *head, uint16_t *tail,
                            const uint8_t *packet, uint32_t *overflow)
{
  uint16_t next = (uint16_t)((*head + 1U) % size);
  if (next == *tail)
  {
    (*overflow)++;
    return false;
  }
  memcpy(buffer[*head], packet, USBH_MIDI_EVENT_SIZE);
  *head = next;
  return true;
}

static bool MIDI_BufferPop(uint8_t buffer[][USBH_MIDI_EVENT_SIZE], uint16_t size,
                           uint16_t *head, uint16_t *tail, uint8_t *packet)
{
  if (*tail == *head)
  {
    return false;
  }
  memcpy(packet, buffer[*tail], USBH_MIDI_EVENT_SIZE);
  *tail = (uint16_t)((*tail + 1U) % size);
  return true;
}
