/**
 * @file usb_host_midi.h
 * @brief ChibiOS wrapper around the STM32 USB Host MIDI class.
 */

#ifndef USB_HOST_MIDI_H
#define USB_HOST_MIDI_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void usb_host_midi_init(void);
bool usb_host_midi_is_device_attached(void);
bool usb_host_midi_is_ready(void);
bool usb_host_midi_receive(uint8_t packet[4]);
bool usb_host_midi_send(const uint8_t packet[4]);
uint32_t usb_host_midi_rx_overflow(void);
uint32_t usb_host_midi_tx_overflow(void);
uint32_t usb_host_midi_tx_write_failures(void);
uint32_t usb_host_midi_reset_count(void);
uint32_t usb_host_midi_error_count(void);

#ifdef __cplusplus
}
#endif

#endif
