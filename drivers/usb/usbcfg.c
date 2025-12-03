/**
 * @file usbcfg.c
 * @brief Configuration bas niveau de l’interface USB MIDI (ChibiOS).
 *
 * Ce module définit tous les descripteurs et callbacks nécessaires à la
 * gestion d’un périphérique **USB MIDI Class-Compliant** :
 * - Interface Audio Control (AC)
 * - Interface MIDI Streaming (MS)
 * - Endpoints Bulk IN/OUT pour la communication MIDI.
 *
 * Il s’intègre au driver `USBDriver` de ChibiOS et assure :
 * - L’initialisation des endpoints lors de la configuration USB.
 * - Le réarmement de la réception OUT.
 * - La signalisation au thread MIDI lors de la fin d’envoi IN.
 *
 * @details
 * **Résumé matériel :**
 * - EP1 OUT (Bulk) : réception de messages MIDI depuis l’hôte.
 * - EP2 IN  (Bulk) : émission de messages MIDI vers l’hôte.
 * - Taille de paquet : 64 octets.
 *
 * Contraintes temps réel :
 * - Callbacks **courts** : pas d’appels bloquants en ISR.
 * - Toute transmission se fait via `usbStartTransmitI()` depuis un thread,
 *   déclenchée par un sémaphore signalé dans l’ISR (EP IN).
 *
 * @ingroup drivers
 * @see midi.c  pour la logique de transmission/réception MIDI applicative.
 */

#include "hal.h"
#include "usbcfg.h"
#include "ch.h"         /* chBSemSignalI */
#include <stdint.h>

/** @brief Indique si l’interface USB-MIDI est prête pour la transmission. */
volatile bool usb_midi_tx_ready = false;

/* ====================================================================== */
/*                        VÉRIFICATIONS DE CONFIG EP                      */
/* ====================================================================== */

#if MIDI_EP_IN != 2
#error "MIDI_EP_IN doit valoir 2 (EP2 IN)."
#endif
#if MIDI_EP_OUT != 1
#error "MIDI_EP_OUT doit valoir 1 (EP1 OUT)."
#endif
#if MIDI_EP_SIZE != 64
#error "MIDI_EP_SIZE doit valoir 64 (Full Speed)."
#endif

/* ====================================================================== */
/*                           DESCRIPTEURS USB                             */
/* ====================================================================== */

/**
 * @brief Descripteur standard du périphérique (USB 2.0).
 */
static const uint8_t device_descriptor_data[] = {
  USB_DESC_DEVICE(0x0200,        /* bcdUSB 2.00                  */
                  0x00,          /* bDeviceClass (IFC)           */
                  0x00,          /* bDeviceSubClass              */
                  0x00,          /* bDeviceProtocol              */
                  64,            /* bMaxPacketSize0              */
                  0x0483,        /* idVendor (STMicro pour test) */
                  0x5740,        /* idProduct (exemple)          */
                  0x0100,        /* bcdDevice 1.00               */
                  1,             /* iManufacturer                */
                  2,             /* iProduct                     */
                  3,             /* iSerialNumber                */
                  1)             /* bNumConfigurations           */
};

static const USBDescriptor device_descriptor = {
  sizeof device_descriptor_data,
  device_descriptor_data
};

/**
 * @brief Descripteur de configuration combinant :
 * - Interface AudioControl (IF 0)
 * - Interface MIDIStreaming (IF 1)
 */
static const uint8_t config_descriptor_data[] = {
  /* 1) Configuration (9) */
  USB_DESC_CONFIGURATION(82, 2, 1, 0, 0x80, 50),

  /* 2) Standard AC Interface (9) */
  USB_DESC_INTERFACE(0, 0, 0, 0x01, 0x01, 0x00, 0),

  /* 3) Class-specific AC Interface Header (9) */
  9, 0x24, 0x01, 0x00, 0x01, 0x09, 0x00, 0x01, 0x01,

  /* 4) Standard MS Interface (9) */
  USB_DESC_INTERFACE(1, 0, 2, 0x01, 0x03, 0x00, 0),

  /* 5) Class-specific MS Interface Header (7) */
  7, 0x24, 0x01, 0x00, 0x01, 32, 0,

  /* 6) MIDI IN Jack (Embedded) (6) */
  6, 0x24, 0x02, 0x01, 0x01, 0x01,

  /* 7) MIDI OUT Jack (Embedded) (9) */
  9, 0x24, 0x03, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00,

  /* 8) Standard Bulk OUT Endpoint (7) EP1 OUT */
  USB_DESC_ENDPOINT(USB_ENDPOINT_OUT(MIDI_EP_OUT), 0x02, MIDI_EP_SIZE, 0),

  /* 9) Class-specific Bulk OUT Endpoint (5) */
  5, 0x25, 0x01, 0x01, 0x01,

  /* 10) Standard Bulk IN Endpoint (7) EP2 IN */
  USB_DESC_ENDPOINT(USB_ENDPOINT_IN(MIDI_EP_IN), 0x02, MIDI_EP_SIZE, 0),

  /* 11) Class-specific Bulk IN Endpoint (5) */
  5, 0x25, 0x01, 0x01, 0x02,
};

static const USBDescriptor config_descriptor = {
  sizeof config_descriptor_data,
  config_descriptor_data
};

/**
 * @brief Descripteurs de chaînes USB (langue, fabricant, produit, série).
 */
static const uint8_t string0[] = { 4, USB_DESCRIPTOR_STRING, 0x09, 0x04 };
static const uint8_t string1[] = {
  16, USB_DESCRIPTOR_STRING,
  'C',0,'h',0,'i',0,'b',0,'i',0,'O',0,'S',0
};
static const uint8_t string2[] = {
  32, USB_DESCRIPTOR_STRING,
  'U',0,'S',0,'B',0,' ',0,'M',0,'I',0,'D',0,'I',0,' ',0,
  'D',0,'e',0,'v',0,'i',0,'c',0,'e',0
};
static const uint8_t string3[] = {
  10, USB_DESCRIPTOR_STRING,
  '0',0,'0',0,'0',0,'1',0
};

static const USBDescriptor strings[] = {
  {sizeof string0, string0},
  {sizeof string1, string1},
  {sizeof string2, string2},
  {sizeof string3, string3},
};

/* ====================================================================== */
/*                          CONFIGURATION DES EP                          */
/* ====================================================================== */

static USBInEndpointState   ep2_in_state;   /**< État runtime EP2 IN  */
static USBOutEndpointState  ep1_out_state;  /**< État runtime EP1 OUT */

/** @brief Buffer de réception de 4 octets (1 paquet MIDI). */
static uint8_t rx_pkt[4];

/**
 * @brief Callback OUT (EP1) — réarme la réception de paquets MIDI.
 * @param usbp Pointeur driver USB.
 * @param ep   Numéro d’endpoint (ignoré).
 */
static void ep1_out_cb(USBDriver *usbp, usbep_t ep) {
  (void)ep;
  usbStartReceiveI(usbp, MIDI_EP_OUT, rx_pkt, sizeof rx_pkt);
}

/**
 * @brief Callback IN (EP2) — signale la fin de transmission MIDI.
 * @param usbp Pointeur driver USB.
 * @param ep   Numéro d’endpoint (ignoré).
 *
 * @details
 * Appelle `chBSemSignalI()` sur le sémaphore `tx_sem` défini dans `midi.c`
 * pour réveiller le thread d’émission.
 */
extern binary_semaphore_t tx_sem;  /* défini dans midi.c */
static void ep2_in_cb(USBDriver *usbp, usbep_t ep) {
  (void)usbp; (void)ep;
  chBSemSignalI(&tx_sem);
}

/* --- EP1 : OUT (bulk) --- */
static const USBEndpointConfig ep1_out_cfg = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  NULL,
  ep1_out_cb,
  0,
  MIDI_EP_SIZE,
  NULL,
  &ep1_out_state,
  1,
  NULL
};

/* --- EP2 : IN (bulk) --- */
static const USBEndpointConfig ep2_in_cfg = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  ep2_in_cb,
  NULL,
  MIDI_EP_SIZE,
  0,
  &ep2_in_state,
  NULL,
  1,
  NULL
};

/* ====================================================================== */
/*                              CALLBACKS                                 */
/* ====================================================================== */

/**
 * @brief Sélectionne et renvoie le bon descripteur USB selon le type demandé.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {
  (void)usbp; (void)lang;
  switch (dtype) {
    case USB_DESCRIPTOR_DEVICE:        return &device_descriptor;
    case USB_DESCRIPTOR_CONFIGURATION: return &config_descriptor;
    case USB_DESCRIPTOR_STRING:
      if (dindex < 4) return &strings[dindex];
      break;
    default:
      break;
  }
  return NULL;
}

/**
 * @brief Callback d’événements du bus USB (configuré, reset, suspend…).
 *
 * - Lors de `USB_EVENT_CONFIGURED`, initialise les endpoints et démarre la RX.
 * - Réinitialise `usb_midi_tx_ready` sur les autres événements.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {
  switch (event) {
    case USB_EVENT_CONFIGURED:
      osalSysLock();
      usbInitEndpointI(usbp, MIDI_EP_OUT, &ep1_out_cfg);
      usbInitEndpointI(usbp, MIDI_EP_IN,  &ep2_in_cfg);
      usbStartReceiveI(usbp, MIDI_EP_OUT, rx_pkt, sizeof rx_pkt);
      usb_midi_tx_ready = true;
      chBSemSignalI(&tx_sem);
      osalSysUnlock();
      break;

    case USB_EVENT_RESET:
    case USB_EVENT_UNCONFIGURED:
    case USB_EVENT_SUSPEND:
      /* Pas besoin de verrou ici : lecture/écriture booléenne atomique. */
      usb_midi_tx_ready = false;
      break;

    default:
      break;
  }
}

/**
 * @brief Callback "Start Of Frame" (SOF). Inutilisé ici.
 */
static void sof_handler(USBDriver *usbp) {
  (void)usbp;
}

/* ====================================================================== */
/*                        CONFIGURATION DU DRIVER USB                     */
/* ====================================================================== */

/**
 * @brief Structure de configuration principale du driver USB.
 *
 * Reliée à `usbStart(&USBD1, &usbcfg)` dans le code d’initialisation global.
 */
const USBConfig usbcfg = {
  usb_event,
  get_descriptor,
  NULL,
  sof_handler
};
