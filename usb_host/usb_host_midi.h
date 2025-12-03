/**
 * @file usb_host_midi.h
 * @brief ChibiOS wrapper for USB Host MIDI class.
 */

#ifndef USB_HOST_MIDI_H
#define USB_HOST_MIDI_H

#include "ch.h"
#include "hal.h"
#include "usbh_core.h"
#include "usbh_midi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback invoked on device attach/detach events.
 */
typedef void (*usb_host_midi_device_callback_t)(void);

void usb_host_midi_init(void);
bool usb_host_midi_is_device_attached(void);
bool usb_host_midi_is_ready(void);

bool usb_host_midi_fetch_event(uint8_t *packet4);
bool usb_host_midi_send_event(const uint8_t *packet4);

void usb_host_midi_register_attach_callback(usb_host_midi_device_callback_t cb);
void usb_host_midi_register_detach_callback(usb_host_midi_device_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif /* USB_HOST_MIDI_H */
