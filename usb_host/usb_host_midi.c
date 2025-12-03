/**
 * @file usb_host_midi.c
 * @brief ChibiOS wrapper for the STM32 USB Host MIDI class driver.
 */

#include "usb_host_midi.h"
#include "usbh_platform_chibios_h7.h"

#include <string.h>

/* Configuration ----------------------------------------------------------------*/
#define USB_HOST_MIDI_THREAD_STACK      1024
#define USB_HOST_MIDI_THREAD_PRIO       (NORMALPRIO + 2)
#define USB_HOST_MIDI_RX_MAILBOX_SIZE   32
#define USB_HOST_MIDI_TX_MAILBOX_SIZE   32

/* Local types ------------------------------------------------------------------*/
typedef union
{
  msg_t     msg;
  uint32_t  word;
  uint8_t   bytes[4];
} midi_mailbox_msg_t;

/* Local data -------------------------------------------------------------------*/
static USBH_HandleTypeDef hUsbHostFS;
static THD_WORKING_AREA(usb_host_midi_wa, USB_HOST_MIDI_THREAD_STACK);

static msg_t rx_queue_buf[USB_HOST_MIDI_RX_MAILBOX_SIZE];
static MAILBOX_DECL(rx_mailbox, rx_queue_buf, USB_HOST_MIDI_RX_MAILBOX_SIZE);

static msg_t tx_queue_buf[USB_HOST_MIDI_TX_MAILBOX_SIZE];
static MAILBOX_DECL(tx_mailbox, tx_queue_buf, USB_HOST_MIDI_TX_MAILBOX_SIZE);

static bool device_attached = false;
static bool midi_ready = false;

static usb_host_midi_device_callback_t attach_cb = NULL;
static usb_host_midi_device_callback_t detach_cb = NULL;

/* Prototypes -------------------------------------------------------------------*/
static THD_FUNCTION(usb_host_midi_thread, arg);
static void USBH_UserProcess(USBH_HandleTypeDef *pHost, uint8_t id);
static void pump_rx_events(void);
static void pump_tx_events(void);

/* Public functions -------------------------------------------------------------*/
void usb_host_midi_init(void)
{
  (void)chMBObjectInit(&rx_mailbox, rx_queue_buf, USB_HOST_MIDI_RX_MAILBOX_SIZE);
  (void)chMBObjectInit(&tx_mailbox, tx_queue_buf, USB_HOST_MIDI_TX_MAILBOX_SIZE);

  (void)USBH_Init(&hUsbHostFS, USBH_UserProcess, 0U);
  (void)USBH_RegisterClass(&hUsbHostFS, &USBH_MIDI_Class);
  (void)USBH_Start(&hUsbHostFS);

  (void)chThdCreateStatic(usb_host_midi_wa, sizeof(usb_host_midi_wa),
                          USB_HOST_MIDI_THREAD_PRIO, usb_host_midi_thread, NULL);
}

bool usb_host_midi_is_device_attached(void)
{
  return device_attached;
}

bool usb_host_midi_is_ready(void)
{
  return midi_ready && USBH_MIDI_IsReady(&hUsbHostFS);
}

bool usb_host_midi_fetch_event(uint8_t *packet4)
{
  midi_mailbox_msg_t msg;

  if ((packet4 == NULL) || !usb_host_midi_is_ready())
  {
    return false;
  }

  if (chMBFetchTimeout(&rx_mailbox, &msg.msg, TIME_IMMEDIATE) == MSG_OK)
  {
    (void)memcpy(packet4, msg.bytes, 4U);
    return true;
  }

  return false;
}

bool usb_host_midi_send_event(const uint8_t *packet4)
{
  midi_mailbox_msg_t msg;

  if ((packet4 == NULL) || !usb_host_midi_is_ready())
  {
    return false;
  }

  (void)memcpy(msg.bytes, packet4, 4U);
  return chMBPostTimeout(&tx_mailbox, msg.msg, TIME_IMMEDIATE) == MSG_OK;
}

void usb_host_midi_register_attach_callback(usb_host_midi_device_callback_t cb)
{
  attach_cb = cb;
}

void usb_host_midi_register_detach_callback(usb_host_midi_device_callback_t cb)
{
  detach_cb = cb;
}

/* Local functions --------------------------------------------------------------*/
static THD_FUNCTION(usb_host_midi_thread, arg)
{
  (void)arg;

  while (true)
  {
    (void)USBH_Process(&hUsbHostFS);

    pump_rx_events();
    pump_tx_events();

    chThdSleepMilliseconds(1);
  }
}

static void USBH_UserProcess(USBH_HandleTypeDef *pHost, uint8_t id)
{
  switch (id)
  {
    case HOST_USER_CONNECTION:
      device_attached = true;
      midi_ready = false;
      break;

    case HOST_USER_CLASS_ACTIVE:
      midi_ready = true;
      if (attach_cb != NULL)
      {
        attach_cb();
      }
      break;

    case HOST_USER_DISCONNECTION:
    default:
      device_attached = false;
      midi_ready = false;
      (void)chMBReset(&rx_mailbox);
      (void)chMBReset(&tx_mailbox);
      if (detach_cb != NULL)
      {
        detach_cb();
      }
      break;
  }
}

static void pump_rx_events(void)
{
  uint8_t packet[4];
  midi_mailbox_msg_t msg;

  if (!usb_host_midi_is_ready())
  {
    return;
  }

  while (USBH_MIDI_ReadEvent(&hUsbHostFS, packet))
  {
    (void)memcpy(msg.bytes, packet, 4U);
    if (chMBPostTimeout(&rx_mailbox, msg.msg, TIME_IMMEDIATE) != MSG_OK)
    {
      break; /* mailbox full; drop until consumer catches up */
    }
  }
}

static void pump_tx_events(void)
{
  midi_mailbox_msg_t msg;

  if (!usb_host_midi_is_ready())
  {
    return;
  }

  while (chMBFetchTimeout(&tx_mailbox, &msg.msg, TIME_IMMEDIATE) == MSG_OK)
  {
    if (!USBH_MIDI_WriteEvent(&hUsbHostFS, msg.bytes))
    {
      /* could not queue into class buffer, reinsert and exit */
      (void)chMBPostTimeout(&tx_mailbox, msg.msg, TIME_IMMEDIATE);
      break;
    }
  }
}

