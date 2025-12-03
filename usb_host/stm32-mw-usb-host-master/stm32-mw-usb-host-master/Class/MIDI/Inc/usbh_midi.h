/**
 * @file usbh_midi.h
 * @brief USB Host MIDI class driver definitions.
 */

#ifndef USBH_MIDI_H
#define USBH_MIDI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbh_core.h"
#include <stdbool.h>
#include <stdint.h>

#if defined(__GNUC__)
#define USBH_MIDI_DMA_ALIGN __attribute__((aligned(32)))
#else
#define USBH_MIDI_DMA_ALIGN
#endif

#define USB_AUDIO_CLASS              0x01U
#define USB_AUDIO_SUBCLASS_MIDISTREAMING 0x03U

#define USBH_MIDI_EVENT_SIZE         4U
#define USBH_MIDI_RX_QUEUE_SIZE      128U
#define USBH_MIDI_TX_QUEUE_SIZE      128U
#define USBH_MIDI_MAX_PACKET         64U

typedef enum
{
  MIDI_PIPE_STATE_IDLE = 0,
  MIDI_PIPE_STATE_BUSY
} MIDI_PipeStateTypeDef;

typedef struct
{
  uint8_t              InPipe;
  uint8_t              OutPipe;
  uint8_t              InEp;
  uint8_t              OutEp;
  uint16_t             InEpSize;
  uint16_t             OutEpSize;
  uint8_t              interface;
  MIDI_PipeStateTypeDef in_state;
  MIDI_PipeStateTypeDef out_state;
  uint8_t              in_packet[USBH_MIDI_MAX_PACKET] USBH_MIDI_DMA_ALIGN;
  uint8_t              out_packet[USBH_MIDI_MAX_PACKET] USBH_MIDI_DMA_ALIGN;
  uint8_t              rx_buffer[USBH_MIDI_RX_QUEUE_SIZE][USBH_MIDI_EVENT_SIZE] USBH_MIDI_DMA_ALIGN;
  uint8_t              tx_buffer[USBH_MIDI_TX_QUEUE_SIZE][USBH_MIDI_EVENT_SIZE] USBH_MIDI_DMA_ALIGN;
  uint16_t             rx_head;
  uint16_t             rx_tail;
  uint16_t             tx_head;
  uint16_t             tx_tail;
  uint32_t             rx_overflow;
  uint32_t             tx_overflow;
} MIDI_HandleTypeDef;

extern USBH_ClassTypeDef  USBH_MIDI_Class;

bool USBH_MIDI_IsReady(USBH_HandleTypeDef *phost);
bool USBH_MIDI_ReadEvent(USBH_HandleTypeDef *phost, uint8_t *packet4);
bool USBH_MIDI_WriteEvent(USBH_HandleTypeDef *phost, const uint8_t *packet4);
uint32_t USBH_MIDI_GetRxOverflow(USBH_HandleTypeDef *phost);
uint32_t USBH_MIDI_GetTxOverflow(USBH_HandleTypeDef *phost);
void USBH_MIDI_Flush(USBH_HandleTypeDef *phost);

#ifdef __cplusplus
}
#endif

#endif
