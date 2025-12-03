/**
  ******************************************************************************
  * @file    usbh_midi.c
  * @brief   USB Host MIDI class implementation for STM32 USB Host Library.
  ******************************************************************************
  */

#include "usbh_midi.h"
#include "usbh_ctlreq.h"
#include <string.h>

/* Private function prototypes ------------------------------------------------*/
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

/* Private variables ---------------------------------------------------------*/
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

/**
  * @brief  Initialize MIDI interface
  */
static USBH_StatusTypeDef USBH_MIDI_InterfaceInit(USBH_HandleTypeDef *phost)
{
  uint8_t interface;
  MIDI_HandleTypeDef *midi;
  uint8_t ep_index;

  interface = USBH_FindInterface(phost, USB_AUDIO_CLASS_CODE, USB_MIDISTREAMING_SUBCLASS_CODE, 0U);

  if (!MIDI_IsInterfaceValid(phost, interface))
  {
    USBH_DbgLog("%s No valid MIDI Streaming interface found", phost->pActiveClass->Name);
    return USBH_FAIL;
  }

  if (USBH_SelectInterface(phost, interface) != USBH_OK)
  {
    return USBH_FAIL;
  }

  midi = (MIDI_HandleTypeDef *)USBH_malloc(sizeof(MIDI_HandleTypeDef));
  if (midi == NULL)
  {
    USBH_DbgLog("%s Failed to allocate MIDI handle", phost->pActiveClass->Name);
    return USBH_FAIL;
  }

  (void)USBH_memset(midi, 0, sizeof(MIDI_HandleTypeDef));
  midi->interface_index = interface;
  midi->rx_state = MIDI_IDLE;
  midi->tx_state = MIDI_IDLE;

  /* Parse endpoints */
  for (ep_index = 0U; ep_index < phost->device.CfgDesc.Itf_Desc[interface].bNumEndpoints; ep_index++)
  {
    USBH_EpDescTypeDef *ep_desc = &phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[ep_index];
    if ((ep_desc->bmAttributes & USB_EP_TYPE_MASK) == USB_EP_TYPE_BULK)
    {
      if ((ep_desc->bEndpointAddress & 0x80U) != 0U)
      {
        midi->InEp = ep_desc->bEndpointAddress;
        midi->InEpSize = ep_desc->wMaxPacketSize;
      }
      else
      {
        midi->OutEp = ep_desc->bEndpointAddress;
        midi->OutEpSize = ep_desc->wMaxPacketSize;
      }
    }
  }

  if ((midi->InEp == 0U) || (midi->OutEp == 0U))
  {
    USBH_DbgLog("%s Missing MIDI bulk endpoints", phost->pActiveClass->Name);
    (void)USBH_free(midi);
    return USBH_FAIL;
  }

  midi->InPipe = USBH_AllocPipe(phost, midi->InEp);
  midi->OutPipe = USBH_AllocPipe(phost, midi->OutEp);

  /* Open pipes */
  (void)USBH_OpenPipe(phost, midi->InPipe, midi->InEp, phost->device.address,
                      phost->device.speed, USB_EP_TYPE_BULK, midi->InEpSize);
  (void)USBH_OpenPipe(phost, midi->OutPipe, midi->OutEp, phost->device.address,
                      phost->device.speed, USB_EP_TYPE_BULK, midi->OutEpSize);

  USBH_LL_SetToggle(phost, midi->InPipe, 0U);
  USBH_LL_SetToggle(phost, midi->OutPipe, 0U);

  phost->pActiveClass->pData = (void *)midi;
  return USBH_OK;
}

/**
  * @brief  DeInitialize MIDI interface
  */
static USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *midi = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;

  if (midi != NULL)
  {
    if (midi->InPipe != 0U)
    {
      (void)USBH_ClosePipe(phost, midi->InPipe);
      (void)USBH_FreePipe(phost, midi->InPipe);
      midi->InPipe = 0U;
    }

    if (midi->OutPipe != 0U)
    {
      (void)USBH_ClosePipe(phost, midi->OutPipe);
      (void)USBH_FreePipe(phost, midi->OutPipe);
      midi->OutPipe = 0U;
    }

    (void)USBH_free(midi);
    phost->pActiveClass->pData = NULL;
  }
  return USBH_OK;
}

/**
  * @brief  Requests are not needed for MIDI streaming class.
  */
static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost)
{
  UNUSED(phost);
  return USBH_OK;
}

/**
  * @brief  Main class process state machine.
  */
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

/**
  * @brief  Handle Start Of Frame events.
  */
static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost)
{
  UNUSED(phost);
  return USBH_OK;
}

/**
  * @brief Check if MIDI class is ready.
  */
bool USBH_MIDI_IsReady(USBH_HandleTypeDef *phost)
{
  if ((phost == NULL) || (phost->gState != HOST_CLASS))
  {
    return false;
  }

  return (phost->pActiveClass != NULL) && (phost->pActiveClass->pData != NULL);
}

/**
  * @brief Read one USB-MIDI event packet from RX queue.
  */
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

/**
  * @brief Queue one USB-MIDI event packet for transmission.
  */
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

/**
  * @brief Encode a MIDI short message into a USB-MIDI packet.
  */
void USBH_MIDI_EncodeShortMessage(uint8_t cable, uint8_t status,
                                  uint8_t data1, uint8_t data2,
                                  uint8_t *packet4)
{
  uint8_t cin = (status >> 4) & 0x0FU;
  uint8_t header = ((cable & 0x0FU) << 4) | (cin & 0x0FU);

  packet4[0] = header;
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

  if (midi->rx_state == MIDI_IDLE)
  {
    (void)USBH_BulkReceiveData(phost, midi->rx_buf, midi->InEpSize, midi->InPipe);
    midi->rx_state = MIDI_TRANSFER;
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
}

static void MIDI_ProcessTransmission(USBH_HandleTypeDef *phost, MIDI_HandleTypeDef *midi)
{
  USBH_URBStateTypeDef urb_state;
  uint16_t tx_count = 0U;
  uint8_t packet[4];

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
      }
      break;

    case MIDI_TRANSFER:
      urb_state = USBH_LL_GetURBState(phost, midi->OutPipe);
      if (urb_state == USBH_URB_DONE)
      {
        midi->tx_packets++;
        midi->tx_state = MIDI_IDLE;
      }
      else if (urb_state == USBH_URB_NOTREADY)
      {
        midi->tx_state = MIDI_IDLE;
      }
      else if (urb_state == USBH_URB_ERROR)
      {
        midi->tx_dropped++;
        midi->tx_state = MIDI_ERROR;
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
    return false; /* buffer full */
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
    return false; /* buffer full */
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

