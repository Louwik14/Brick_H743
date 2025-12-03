/**
  * @file usbh_midi.h
  * @brief USB Host MIDI class header for stm32-mw-usb-host-master.
  */

#ifndef __USBH_MIDI_H
#define __USBH_MIDI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbh_core.h"
#include <stdbool.h>

/**
 * @brief Maximum number of bulk endpoints supported by the MIDI class.
 */
#ifndef USBH_MIDI_MAX_ENDPOINTS
#define USBH_MIDI_MAX_ENDPOINTS             4U
#endif

/**
 * @brief Size in USB-MIDI event packets (4 bytes) of the RX ring buffer.
 */
#ifndef USBH_MIDI_RX_EVENT_BUFFER
#define USBH_MIDI_RX_EVENT_BUFFER           256U
#endif

/**
 * @brief Size in USB-MIDI event packets (4 bytes) of the TX ring buffer.
 */
#ifndef USBH_MIDI_TX_EVENT_BUFFER
#define USBH_MIDI_TX_EVENT_BUFFER           256U
#endif

/**
 * @brief Maximum size of a single bulk transfer in bytes.
 */
#ifndef USBH_MIDI_MAX_PACKET_SIZE
#define USBH_MIDI_MAX_PACKET_SIZE           64U
#endif

/**
 * @brief USB MIDI class specific protocol identifiers.
 */
#define USB_AUDIO_CLASS_CODE                0x01U
#define USB_MIDISTREAMING_SUBCLASS_CODE     0x03U

/**
 * @brief MIDI transfer states.
 */
typedef enum
{
  MIDI_IDLE = 0U,
  MIDI_TRANSFER,
  MIDI_ERROR
} MIDI_TransferStateTypeDef;

/**
 * @brief MIDI Host handle definition.
 */
typedef struct
{
  uint8_t              InEp;
  uint8_t              OutEp;
  uint8_t              InPipe;
  uint8_t              OutPipe;
  uint16_t             InEpSize;
  uint16_t             OutEpSize;

  MIDI_TransferStateTypeDef rx_state;
  MIDI_TransferStateTypeDef tx_state;

  uint8_t              rx_buf[USBH_MIDI_MAX_PACKET_SIZE];
  uint8_t              tx_buf[USBH_MIDI_MAX_PACKET_SIZE];

  uint8_t              rx_events[USBH_MIDI_RX_EVENT_BUFFER][4];
  uint16_t             rx_head;
  uint16_t             rx_tail;

  uint8_t              tx_events[USBH_MIDI_TX_EVENT_BUFFER][4];
  uint16_t             tx_head;
  uint16_t             tx_tail;

  uint32_t             rx_dropped;
  uint32_t             tx_dropped;
  uint32_t             rx_packets;
  uint32_t             tx_packets;

  uint16_t             pending_tx_packets;

  uint8_t              interface_index;

} MIDI_HandleTypeDef;

extern USBH_ClassTypeDef  USBH_MIDI_Class;

/**
 * @brief   Checks whether a MIDI device is ready for I/O operations.
 * @param   phost Pointer to the USB Host handle.
 * @retval  true when the class is active and endpoints are configured.
 */
bool USBH_MIDI_IsReady(USBH_HandleTypeDef *phost);

/**
 * @brief   Pops one USB-MIDI event packet (4 bytes) from the RX queue.
 * @param   phost Pointer to the USB Host handle.
 * @param   packet4 Pointer to a 4-byte buffer that receives the event.
 * @retval  true when an event was returned, false when the queue is empty
 *          or the class is not ready.
 */
bool USBH_MIDI_ReadEvent(USBH_HandleTypeDef *phost, uint8_t *packet4);

/**
 * @brief   Queues one USB-MIDI event packet (4 bytes) for transmission.
 * @param   phost Pointer to the USB Host handle.
 * @param   packet4 Pointer to the 4-byte event to transmit.
 * @retval  true when the event was queued successfully.
 */
bool USBH_MIDI_WriteEvent(USBH_HandleTypeDef *phost, const uint8_t *packet4);

/**
 * @brief   Helper to encode a MIDI short message into a USB-MIDI packet.
 * @param   cable Cable number (0-15 encoded in CIN upper bits).
 * @param   status MIDI status byte.
 * @param   data1 First MIDI data byte.
 * @param   data2 Second MIDI data byte (use 0 for 2-byte messages).
 * @param   packet4 Output buffer of 4 bytes.
 */
void USBH_MIDI_EncodeShortMessage(uint8_t cable, uint8_t status,
                                  uint8_t data1, uint8_t data2,
                                  uint8_t *packet4);

#ifdef __cplusplus
}
#endif

#endif /* __USBH_MIDI_H */
