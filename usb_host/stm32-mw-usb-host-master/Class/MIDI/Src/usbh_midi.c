/**
  ******************************************************************************
  * @file    usbh_midi.c
  * @brief   USB Host MIDI class implementation for STM32 USB Host Library.
  ******************************************************************************
  */

#include "usbh_midi.h"
#include "usbh_ctlreq.h"
#include <string.h>

static USBH_StatusTypeDef USBH_MIDI_InterfaceInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost);
static void MIDI_ProcessReception(USBH_HandleTypeDef *phost, MIDI_HandleTypeDef *midi);
static void MIDI_ProcessTransmission(USBH_HandleTypeDef *phost, MIDI_HandleTypeDef *midi);
static bool MIDI_EnqueueRx(MIDI_HandleTypeDef *midi, const uint8_t *packet);
static bool MIDI_DequeueRx(MIDI_HandleTypeDef *midi, uint8_t *packet);
static bool MIDI_EnqueueTx(MIDI_HandleTypeDef *midi, const uint8_t *packet);
static bool MIDI_DequeueTx(MIDI_HandleTypeDef *midi, uint8_t *packet);
static bool MIDI_IsInterfaceValid(USBH_HandleTypeDef *phost, uint8_t interface);

USBH_ClassTypeDef  USBH_MIDI_Class =
{
  "MIDI",
  USB_AUDIO_CLASS_CODE,
  USBH_MIDI_InterfaceInit,
  USBH_MIDI_InterfaceDeInit,
  USBH_MIDI_ClassRequest,
  USBH_MIDI_Process,
  USBH_MIDI_SOFProcess,
  NULL,
};

static MIDI_HandleTypeDef midi_handle;

static USBH_StatusTypeDef USBH_MIDI_InterfaceInit(USBH_HandleTypeDef *phost)
{
  uint8_t interface = USBH_FindInterface(phost, USB_AUDIO_CLASS_CODE, USB_MIDISTREAMING_SUBCLASS_CODE, 0U);
  uint8_t ep_index;
  bool have_in = false;
  bool have_out = false;

  if (!MIDI_IsInterfaceValid(phost, interface))
  {
    return USBH_FAIL;
  }

  if (USBH_SelectInterface(phost, interface) != USBH_OK)
  {
    return USBH_FAIL;
  }

  (void)USBH_memset(&midi_handle, 0, sizeof(MIDI_HandleTypeDef));
  midi_handle.interface_index = interface;
  midi_handle.rx_state = MIDI_IDLE;
  midi_handle.tx_state = MIDI_IDLE;
  midi_handle.InPipe = 0xFFU;
  midi_handle.OutPipe = 0xFFU;
  midi_handle.interface_ready = false;
  phost->pActiveClass->pData = (void *)&midi_handle;

  for (ep_index = 0U; ep_index < phost->device.CfgDesc.Itf_Desc[interface].bNumEndpoints; ep_index++)
  {
    USBH_EpDescTypeDef *ep_desc = &phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[ep_index];
    if ((ep_desc->bmAttributes & USB_EP_TYPE_MASK) == USB_EP_TYPE_BULK)
    {
      if ((ep_desc->bEndpointAddress & 0x80U) != 0U)
      {
        midi_handle.InEp = ep_desc->bEndpointAddress;
        midi_handle.InEpSize = (uint16_t)MIN(ep_desc->wMaxPacketSize, USBH_MIDI_MAX_PACKET_SIZE);
        have_in = true;
      }
      else
      {
        midi_handle.OutEp = ep_desc->bEndpointAddress;
        midi_handle.OutEpSize = (uint16_t)MIN(ep_desc->wMaxPacketSize, USBH_MIDI_MAX_PACKET_SIZE);
        have_out = true;
      }
    }
  }

  if (!have_in || !have_out)
  {
    phost->pActiveClass->pData = NULL;
    return USBH_FAIL;
  }

  midi_handle.InPipe = USBH_AllocPipe(phost, midi_handle.InEp);
  midi_handle.OutPipe = USBH_AllocPipe(phost, midi_handle.OutEp);

  if ((midi_handle.InPipe == 0xFFU) || (midi_handle.OutPipe == 0xFFU))
  {
    (void)USBH_MIDI_InterfaceDeInit(phost);
    return USBH_FAIL;
  }

  if ((USBH_OpenPipe(phost, midi_handle.InPipe, midi_handle.InEp, phost->device.address,
                     phost->device.speed, USB_EP_TYPE_BULK, midi_handle.InEpSize) != USBH_OK) ||
      (USBH_OpenPipe(phost, midi_handle.OutPipe, midi_handle.OutEp, phost->device.address,
                     phost->device.speed, USB_EP_TYPE_BULK, midi_handle.OutEpSize) != USBH_OK))
  {
    (void)USBH_MIDI_InterfaceDeInit(phost);
    return USBH_FAIL;
  }

  midi_handle.interface_ready = true;
  return USBH_OK;
}

static USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *midi = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;

  if (midi != NULL)
  {
    if (midi->InPipe != 0xFFU)
    {
      (void)USBH_ClosePipe(phost, midi->InPipe);
      (void)USBH_FreePipe(phost, midi->InPipe);
      midi->InPipe = 0xFFU;
    }

    if (midi->OutPipe != 0xFFU)
    {
      (void)USBH_ClosePipe(phost, midi->OutPipe);
      (void)USBH_FreePipe(phost, midi->OutPipe);
      midi->OutPipe = 0xFFU;
    }

    midi->rx_head = midi->rx_tail = 0U;
    midi->tx_head = midi->tx_tail = 0U;
    midi->rx_state = MIDI_IDLE;
    midi->tx_state = MIDI_IDLE;
    midi->interface_ready = false;
    phost->pActiveClass->pData = NULL;
  }
  return USBH_OK;
}

static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost)
{
  UNUSED(phost);
  return USBH_OK;
}

static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *midi = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  if (midi == NULL)
  {
    return USBH_FAIL;
  }

  MIDI_ProcessReception(phost, midi);
  MIDI_ProcessTransmission(phost, midi);

  return USBH_OK;
}

static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost)
{
  UNUSED(phost);
  return USBH_OK;
}

bool USBH_MIDI_IsReady(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *midi;

  if ((phost == NULL) || (phost->gState != HOST_CLASS) ||
      (phost->pActiveClass != &USBH_MIDI_Class))
  {
    return false;
  }

  midi = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  if (midi == NULL)
  {
    return false;
  }

  return midi->interface_ready && (midi->InPipe != 0xFFU) && (midi->OutPipe != 0xFFU);
}

bool USBH_MIDI_ReadEvent(USBH_HandleTypeDef *phost, uint8_t *packet4)
{
  MIDI_HandleTypeDef *midi;

  if (!USBH_MIDI_IsReady(phost) || (packet4 == NULL))
  {
    return false;
  }

  midi = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;

  return MIDI_DequeueRx(midi, packet4);
}

bool USBH_MIDI_WriteEvent(USBH_HandleTypeDef *phost, const uint8_t *packet4)
{
  MIDI_HandleTypeDef *midi;

  if (!USBH_MIDI_IsReady(phost) || (packet4 == NULL))
  {
    return false;
  }

  midi = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  return MIDI_EnqueueTx(midi, packet4);
}

void USBH_MIDI_EncodeShortMessage(uint8_t cable, uint8_t status,
                                  uint8_t data1, uint8_t data2,
                                  uint8_t *packet4)
{
  uint8_t cin;

  switch (status & 0xF0U)
  {
    case 0x80U: cin = 0x8U; break;
    case 0x90U: cin = 0x9U; break;
    case 0xA0U: cin = 0xAU; break;
    case 0xB0U: cin = 0xBU; break;
    case 0xC0U: cin = 0xCU; data2 = 0U; break;
    case 0xD0U: cin = 0xDU; data2 = 0U; break;
    case 0xE0U: cin = 0xEU; break;
    default:
      if (status == 0xF1U)
      {
        cin = 0x2U;
        data2 = 0U;
      }
      else if (status == 0xF2U)
      {
        cin = 0x3U;
      }
      else if (status == 0xF3U)
      {
        cin = 0x2U;
        data2 = 0U;
      }
      else if (status == 0xF6U)
      {
        cin = 0x5U;
        data1 = 0U;
        data2 = 0U;
      }
      else
      {
        cin = 0xFU;
        data1 = 0U;
        data2 = 0U;
      }
      break;
  }

  packet4[0] = (uint8_t)(((cable & 0x0FU) << 4) | (cin & 0x0FU));
  packet4[1] = status;
  packet4[2] = data1;
  packet4[3] = data2;
}

static bool MIDI_IsInterfaceValid(USBH_HandleTypeDef *phost, uint8_t interface)
{
  if ((interface == 0xFFU) || (interface >= USBH_MAX_NUM_INTERFACES))
  {
    return false;
  }

  if (phost->device.CfgDesc.Itf_Desc[interface].bInterfaceClass != USB_AUDIO_CLASS_CODE)
  {
    return false;
  }

  return phost->device.CfgDesc.Itf_Desc[interface].bInterfaceSubClass == USB_MIDISTREAMING_SUBCLASS_CODE;
}

static void MIDI_ProcessReception(USBH_HandleTypeDef *phost, MIDI_HandleTypeDef *midi)
{
  USBH_URBStateTypeDef urb_state;
  uint32_t xfer_size;
  uint32_t offset;

  if ((midi == NULL) || (midi->InPipe == 0xFFU))
  {
    return;
  }

  if (midi->rx_state == MIDI_IDLE)
  {
    (void)USBH_BulkReceiveData(phost, midi->rx_buf, midi->InEpSize, midi->InPipe);
    midi->rx_state = MIDI_TRANSFER;
    return;
  }
  else if (midi->rx_state == MIDI_ERROR)
  {
    midi->rx_state = MIDI_IDLE;
    return;
  }

  urb_state = USBH_LL_GetURBState(phost, midi->InPipe);

  if (urb_state == USBH_URB_DONE)
  {
    xfer_size = USBH_LL_GetLastXferSize(phost, midi->InPipe);
    offset = 0U;
    while ((offset + 4U) <= xfer_size)
    {
      if (MIDI_EnqueueRx(midi, &midi->rx_buf[offset]))
      {
        midi->rx_packets++;
      }
      else
      {
        midi->rx_dropped++;
      }
      offset += 4U;
    }
    midi->rx_state = MIDI_IDLE;
  }
  else if ((urb_state == USBH_URB_NOTREADY) || (urb_state == USBH_URB_NYET))
  {
    midi->rx_state = MIDI_IDLE;
  }
  else if (urb_state == USBH_URB_ERROR)
  {
    midi->rx_state = MIDI_ERROR;
  }
  else if (urb_state == USBH_URB_STALL)
  {
    (void)USBH_ClrFeature(phost, midi->InEp);
    midi->rx_state = MIDI_IDLE;
  }
}

static void MIDI_ProcessTransmission(USBH_HandleTypeDef *phost, MIDI_HandleTypeDef *midi)
{
  USBH_URBStateTypeDef urb_state;
  uint16_t tx_count = 0U;
  uint8_t packet[4];

  if ((midi == NULL) || (midi->OutPipe == 0xFFU))
  {
    return;
  }

  switch (midi->tx_state)
  {
    case MIDI_IDLE:
      while ((tx_count + 4U) <= midi->OutEpSize)
      {
        if (!MIDI_DequeueTx(midi, packet))
        {
          break;
        }
        (void)memcpy(&midi->tx_buf[tx_count], packet, 4U);
        tx_count += 4U;
      }

      if (tx_count > 0U)
      {
        (void)USBH_BulkSendData(phost, midi->tx_buf, tx_count, midi->OutPipe, 1U);
        midi->tx_state = MIDI_TRANSFER;
        midi->pending_tx_packets = (uint16_t)(tx_count / 4U);
      }
      break;

    case MIDI_TRANSFER:
      urb_state = USBH_LL_GetURBState(phost, midi->OutPipe);
      if (urb_state == USBH_URB_DONE)
      {
        midi->tx_packets += midi->pending_tx_packets;
        midi->pending_tx_packets = 0U;
        midi->tx_state = MIDI_IDLE;
      }
      else if (urb_state == USBH_URB_NOTREADY)
      {
        midi->pending_tx_packets = 0U;
        midi->tx_state = MIDI_IDLE;
      }
      else if (urb_state == USBH_URB_ERROR)
      {
        midi->tx_dropped++;
        midi->pending_tx_packets = 0U;
        midi->tx_state = MIDI_ERROR;
      }
      else if (urb_state == USBH_URB_STALL)
      {
        (void)USBH_ClrFeature(phost, midi->OutEp);
        midi->pending_tx_packets = 0U;
        midi->tx_state = MIDI_IDLE;
      }
      break;

    case MIDI_ERROR:
    default:
      midi->tx_state = MIDI_IDLE;
      break;
  }
}

static bool MIDI_EnqueueRx(MIDI_HandleTypeDef *midi, const uint8_t *packet)
{
  uint16_t next = (uint16_t)((midi->rx_head + 1U) % USBH_MIDI_RX_EVENT_BUFFER);

  if (next == midi->rx_tail)
  {
    return false;
  }

  (void)memcpy(midi->rx_events[midi->rx_head], packet, 4U);
  midi->rx_head = next;
  return true;
}

static bool MIDI_DequeueRx(MIDI_HandleTypeDef *midi, uint8_t *packet)
{
  if (midi->rx_head == midi->rx_tail)
  {
    return false;
  }

  (void)memcpy(packet, midi->rx_events[midi->rx_tail], 4U);
  midi->rx_tail = (uint16_t)((midi->rx_tail + 1U) % USBH_MIDI_RX_EVENT_BUFFER);
  return true;
}

static bool MIDI_EnqueueTx(MIDI_HandleTypeDef *midi, const uint8_t *packet)
{
  uint16_t next = (uint16_t)((midi->tx_head + 1U) % USBH_MIDI_TX_EVENT_BUFFER);

  if (next == midi->tx_tail)
  {
    midi->tx_dropped++;
    return false;
  }

  (void)memcpy(midi->tx_events[midi->tx_head], packet, 4U);
  midi->tx_head = next;
  return true;
}

static bool MIDI_DequeueTx(MIDI_HandleTypeDef *midi, uint8_t *packet)
{
  if (midi->tx_head == midi->tx_tail)
  {
    return false;
  }

  (void)memcpy(packet, midi->tx_events[midi->tx_tail], 4U);
  midi->tx_tail = (uint16_t)((midi->tx_tail + 1U) % USBH_MIDI_TX_EVENT_BUFFER);
  return true;
}

