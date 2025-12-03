/**
 * @file usb_host_midi.c
 * @brief ChibiOS thread and API for USB MIDI host operation.
 */

#include "ch.h"
#include <string.h>

#include "usb_host_midi.h"
#include "usbh_conf.h"
#include "usbh_core.h"
#include "usbh_midi.h"
#include "usbh_platform_chibios_h7.h"
#include "usb_host_fifo.h"

#define USB_HOST_THREAD_PRIO       (NORMALPRIO + 2)
#define USB_HOST_THREAD_STACK      2048
#define USB_HOST_PROCESS_DELAY_US  250U
#define USB_IDLE_TIMEOUT_MS        5000U
#define USB_OVERFLOW_STREAK_LIMIT  8U

static USBH_HandleTypeDef hUsbHostFS;
static THD_WORKING_AREA(usb_host_wa, USB_HOST_THREAD_STACK);
static thread_t *usb_host_thread_ref = NULL;

static volatile bool device_attached = false;
static volatile bool midi_ready = false;
static volatile bool reset_requested = false;
static volatile uint32_t fifo_overflow_total = 0U;
static volatile uint32_t tx_overflow = 0U;
static volatile uint32_t usb_error_count = 0U;
static volatile uint32_t usb_recovery_count = 0U;
static uint32_t overflow_streak = 0U;
static systime_t last_activity = 0;
static bool host_started = false;

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);
static THD_FUNCTION(usb_host_thread, arg);
static void usb_host_schedule_reset(void);
static void usb_host_restart(void);
static void usb_host_update_activity(void);
static void usb_host_poll_midi(void);

void usb_host_midi_init(void)
{
  if (usb_host_thread_ref != NULL)
  {
    return;
  }

  USBH_static_mem_reset();
  usb_host_fifo_reset();
  usb_host_update_activity();
  usb_host_restart();

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
    usb_host_update_activity();
    break;
  case HOST_USER_DISCONNECTION:
    device_attached = false;
    midi_ready = false;
    usb_host_fifo_reset();
    overflow_streak = 0U;
    break;
  case HOST_USER_CLASS_ACTIVE:
    midi_ready = true;
    usb_host_update_activity();
    break;
  case HOST_USER_UNRECOVERED_ERROR:
    device_attached = false;
    midi_ready = false;
    usb_error_count++;
    usb_host_schedule_reset();
    break;
  default:
    break;
  }
}

static THD_FUNCTION(usb_host_thread, arg)
{
  (void)arg;
  chRegSetThreadName("usb_host");

  usb_host_update_activity();

  while (true)
  {
    USBH_Process(&hUsbHostFS);
    midi_ready = USBH_MIDI_IsReady(&hUsbHostFS);
    tx_overflow = USBH_MIDI_GetTxOverflow(&hUsbHostFS);
    fifo_overflow_total = usb_host_fifo_overflow_total();

    usb_host_poll_midi();

    if (device_attached &&
        (chVTTimeElapsedSinceX(last_activity) >= TIME_MS2I(USB_IDLE_TIMEOUT_MS)))
    {
      usb_error_count++;
      usb_host_schedule_reset();
    }

    if (reset_requested)
    {
      usb_host_restart();
    }

    chThdSleepMicroseconds(USB_HOST_PROCESS_DELAY_US);
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

  return usb_host_fifo_pop(packet);
}

bool usb_host_midi_send(const uint8_t packet[4])
{
  if (!midi_ready)
  {
    return false;
  }
  if (reset_requested)
  {
    return false;
  }

  if (USBH_MIDI_WriteEvent(&hUsbHostFS, packet))
  {
    usb_host_update_activity();
    return true;
  }

  return false;
}

uint32_t usb_host_midi_rx_overflow(void)
{
  return fifo_overflow_total;
}

uint32_t usb_host_midi_tx_overflow(void)
{
  return tx_overflow;
}

uint32_t usb_host_midi_reset_count(void)
{
  return usb_recovery_count + usbh_platform_get_reset_count();
}

uint32_t usb_host_midi_error_count(void)
{
  return usb_error_count;
}

void Error_Handler(void)
{
  usb_error_count++;
  usb_host_schedule_reset();
}

static void usb_host_schedule_reset(void)
{
  reset_requested = true;
}

static void usb_host_update_activity(void)
{
  last_activity = chVTGetSystemTime();
}

static void usb_host_poll_midi(void)
{
  uint8_t packet[4];

  if (!midi_ready || reset_requested)
  {
    return;
  }

  while (USBH_MIDI_ReadEvent(&hUsbHostFS, packet))
  {
    usb_host_update_activity();

    if (!usb_host_fifo_push(packet))
    {
      fifo_overflow_total = usb_host_fifo_overflow_total();
      overflow_streak++;
      if (overflow_streak >= USB_OVERFLOW_STREAK_LIMIT)
      {
        usb_error_count++;
        usb_host_schedule_reset();
        break;
      }
    }
    else
    {
      overflow_streak = 0U;
    }

    if (reset_requested)
    {
      break;
    }
  }
}

static void usb_host_restart(void)
{
  uint32_t oom_count = USBH_static_get_oom_count();

  if (oom_count > 0U)
  {
    usb_error_count++;
    USBH_ErrLog("USBH static pool OOM since last reset: %lu", (unsigned long)oom_count);
  }

  reset_requested = false;
  midi_ready = false;
  device_attached = false;
  overflow_streak = 0U;

  if ((hUsbHostFS.pData != NULL) || host_started)
  {
    USBH_Stop(&hUsbHostFS);
    USBH_DeInit(&hUsbHostFS);
  }

  memset(&hUsbHostFS, 0, sizeof(hUsbHostFS));

  usb_host_fifo_reset();
  USBH_static_mem_reset();

  if (USBH_Init(&hUsbHostFS, USBH_UserProcess, 0) != USBH_OK)
  {
    usb_error_count++;
    usb_host_schedule_reset();
    return;
  }

  if (USBH_RegisterClass(&hUsbHostFS, &USBH_MIDI_Class) != USBH_OK)
  {
    usb_error_count++;
    usb_host_schedule_reset();
    return;
  }

  if (USBH_Start(&hUsbHostFS) != USBH_OK)
  {
    usb_error_count++;
    usb_host_schedule_reset();
    return;
  }

  if (host_started)
  {
    usb_recovery_count++;
  }
  else
  {
    host_started = true;
  }
  usb_host_update_activity();
}
