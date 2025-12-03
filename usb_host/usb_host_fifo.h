#ifndef USB_HOST_FIFO_H
#define USB_HOST_FIFO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool usb_host_fifo_push(const uint8_t packet[4]);
bool usb_host_fifo_pop(uint8_t packet[4]);
uint32_t usb_host_fifo_overflow_total(void);
void usb_host_fifo_reset(void);

#ifdef __cplusplus
}
#endif

#endif
