#include "usb_host_fifo.h"

#include <string.h>

#define USB_HOST_MIDI_FIFO_DEPTH   128U
#define USB_HOST_MIDI_FIFO_MASK    (USB_HOST_MIDI_FIFO_DEPTH - 1U)

#if (USB_HOST_MIDI_FIFO_DEPTH & USB_HOST_MIDI_FIFO_MASK)
#error "USB_HOST_MIDI_FIFO_DEPTH must be a power of two"
#endif

typedef struct
{
  uint8_t data[4];
} usb_host_midi_packet_t;

static usb_host_midi_packet_t midi_fifo[USB_HOST_MIDI_FIFO_DEPTH];
static volatile uint16_t midi_head = 0U;
static volatile uint16_t midi_tail = 0U;
static volatile uint32_t midi_overflow_total = 0U;

bool usb_host_fifo_push(const uint8_t packet[4])
{
  uint16_t local_head = midi_head;
  uint16_t next_head = (local_head + 1U) & USB_HOST_MIDI_FIFO_MASK;

  if (next_head == midi_tail)
  {
    midi_overflow_total++;
    return false;
  }

  memcpy(midi_fifo[local_head].data, packet, sizeof(midi_fifo[local_head].data));
  __atomic_store_n(&midi_head, next_head, __ATOMIC_RELEASE);
  return true;
}

bool usb_host_fifo_pop(uint8_t packet[4])
{
  uint16_t local_tail = midi_tail;
  uint16_t local_head = __atomic_load_n(&midi_head, __ATOMIC_ACQUIRE);

  if (local_head == local_tail)
  {
    return false;
  }

  memcpy(packet, midi_fifo[local_tail].data, sizeof(midi_fifo[local_tail].data));
  midi_tail = (local_tail + 1U) & USB_HOST_MIDI_FIFO_MASK;
  return true;
}

uint32_t usb_host_fifo_overflow_total(void)
{
  return midi_overflow_total;
}

void usb_host_fifo_reset(void)
{
  midi_head = 0U;
  midi_tail = 0U;
}
