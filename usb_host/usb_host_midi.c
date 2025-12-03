/**
 * @file usb_host_midi.c
 * @brief ChibiOS thread and API for USB MIDI host operation.
 */

#include "ch.h"
#include "hal.h"
#include "usb_host_midi.h"
#include "usbh_core.h"
#include "usbh_midi.h"
#include "usbh_platform_chibios_h7.h"

#define USB_HOST_THREAD_PRIO       (NORMALPRIO + 2)
#define USB_HOST_THREAD_STACK      2048

static USBH_HandleTypeDef hUsbHostFS;
static THD_WORKING_AREA(usb_host_wa, USB_HOST_THREAD_STACK);
static thread_t *usb_host_thread_ref = NULL;

static volatile bool device_attached = false;
static volatile bool midi_ready = false;
static volatile uint32_t rx_overflow = 0U;
static volatile uint32_t tx_overflow = 0U;

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);
static THD_FUNCTION(usb_host_thread, arg);

void usb_host_midi_init(void)
{
  if (usb_host_thread_ref != NULL)
  {
    return;
  }

  USBH_Init(&hUsbHostFS, USBH_UserProcess, 0);
  USBH_RegisterClass(&hUsbHostFS, &USBH_MIDI_Class);
  USBH_Start(&hUsbHostFS);

  usb_host_thread_ref = chThdCreateStatic(usb_host_wa, sizeof(usb_host_wa),
                                          USB_HOST_THREAD_PRIO, usb_host_thread,
                                          NULL);
}

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
  (void)phost;
  switch (id)
  {
  case HOST_USER_CONNECTION:
    device_attached = true;
    midi_ready = false;
    break;
  case HOST_USER_DISCONNECTION:
    device_attached = false;
    midi_ready = false;
    break;
  case HOST_USER_CLASS_ACTIVE:
    midi_ready = true;
    break;
  case HOST_USER_UNRECOVERED_ERROR:
    device_attached = false;
    midi_ready = false;
    break;
  default:
    break;
  }
}

static THD_FUNCTION(usb_host_thread, arg)
{
  (void)arg;
  chRegSetThreadName("usb_host");

  while (true)
  {
    USBH_Process(&hUsbHostFS);
    midi_ready = USBH_MIDI_IsReady(&hUsbHostFS);
    rx_overflow = USBH_MIDI_GetRxOverflow(&hUsbHostFS);
    tx_overflow = USBH_MIDI_GetTxOverflow(&hUsbHostFS);
    chThdSleepMilliseconds(1);
  }
}

bool usb_host_midi_is_device_attached(void)
{
  return device_attached;
}

bool usb_host_midi_is_ready(void)
{
  return midi_ready;
}

bool usb_host_midi_receive(uint8_t packet[4])
{
  if (!midi_ready)
  {
    return false;
  }
  return USBH_MIDI_ReadEvent(&hUsbHostFS, packet);
}

bool usb_host_midi_send(const uint8_t packet[4])
{
  if (!midi_ready)
  {
    return false;
  }
  return USBH_MIDI_WriteEvent(&hUsbHostFS, packet);
}

uint32_t usb_host_midi_rx_overflow(void)
{
  return rx_overflow;
}

uint32_t usb_host_midi_tx_overflow(void)
{
  return tx_overflow;
}

uint32_t usb_host_midi_reset_count(void)
{
  return usbh_platform_get_reset_count();
}
