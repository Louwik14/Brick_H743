/**
 * @file midi.c
 * @brief Implémentation du module MIDI (UART + USB) pour ChibiOS.
 *
 * Ce module fournit l’envoi de messages MIDI vers deux destinations :
 * - **DIN UART** (31250 bauds) via `SD2`,
 * - **USB MIDI** (class compliant) via l’endpoint IN (EP2).
 *
 * Principes d’implémentation :
 * - Un **thread dédié** agrège les paquets USB-MIDI en trames de 64 octets (EP IN bulk)
 *   à partir d’une **mailbox** non bloquante.
 * - Les messages “Realtime” (F8, FA/FB/FC/FE/FF) bénéficient d’un **chemin rapide**
 *   avec micro-attente pour l’envoi immédiat si l’endpoint est libre.
 * - Les statistiques d’envoi sont tenues dans `midi_tx_stats` pour le diagnostic.
 *
 * Contraintes temps réel :
 * - Le thread de TX USB doit avoir une priorité **au moins égale ou supérieure à l’UI**.
 * - Les callbacks d’USB doivent rester **courts** (signalement de sémaphore/flags uniquement).
 * - Aucun appel bloquant en ISR ; pas d’allocations dynamiques à l’exécution.
 *
 * @note L’API publique est déclarée dans `midi.h`.
 * @ingroup drivers
 */

#include "ch.h"
#include "hal.h"
#include "brick_config.h"
#include "midi.h"
#include "usbcfg.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Indicateur d’état de l’USB MIDI.
 * @details Positionné à `true` lorsque l’USB est configuré et l’EP prêt.
 *          Fourni par `usbcfg.c` (déclaré ici en `extern`).
 */
extern volatile bool usb_midi_tx_ready;

/* ====================================================================== */
/*                         CONFIGURATION / ÉTAT                            */
/* ====================================================================== */

/**
 * @brief Priorité du thread de transmission USB-MIDI.
 * @details Doit être ≥ priorité UI pour garantir une latence stable des I/O.
 */
#ifndef MIDI_USB_TX_PRIO
#define MIDI_USB_TX_PRIO   (NORMALPRIO + 1)
#endif

/**
 * @brief Timeout d’attente du sémaphore TX en thread (ms).
 * @details Évite les blocages si l’hôte USB ne réarme pas l’endpoint.
 */
#ifndef MIDI_USB_TX_WAIT_MS
#define MIDI_USB_TX_WAIT_MS  2
#endif

/**
 * @brief Périphérique série utilisé pour la sortie DIN MIDI (UART2).
 * @details Mapping Nucleo-144 STM32F429ZI : PA2=TX, PA3=RX.
 */
#define MIDI_UART   &SD2   /* PA2=TX, PA3=RX */

/**
 * @brief Taille de la file (mailbox) de messages USB-MIDI (en éléments de 32 bits).
 * @details Chaque élément correspond à un paquet USB-MIDI de 4 octets packé dans un `msg_t`.
 */
#define MIDI_USB_QUEUE_LEN   256

/** @brief Mailbox de transmission USB-MIDI (producteur/consommateur). */
static CCM_DATA mailbox_t midi_usb_mb;
/** @brief Buffer circulaire pour la mailbox USB-MIDI. */
static CCM_DATA msg_t     midi_usb_queue[MIDI_USB_QUEUE_LEN];
static uint16_t  midi_usb_queue_fill = 0;
static uint16_t  midi_usb_queue_high_water = 0;

static inline void midi_usb_queue_increment(void) {
  osalSysLock();
  if (midi_usb_queue_fill < MIDI_USB_QUEUE_LEN) {
    midi_usb_queue_fill++;
    if (midi_usb_queue_fill > midi_usb_queue_high_water) {
      midi_usb_queue_high_water = midi_usb_queue_fill;
    }
  }
  osalSysUnlock();
}

static inline void midi_usb_queue_decrement(void) {
  osalSysLock();
  if (midi_usb_queue_fill > 0U) {
    midi_usb_queue_fill--;
  }
  osalSysUnlock();
}

/**
 * @brief Sémaphore “endpoint libre”.
 * @details Signalé dans le callback d’EP IN (ex. `ep2_in_cb()` côté `usbcfg.c`).
 */
binary_semaphore_t tx_sem;

/** @brief Statistiques globales de transmission MIDI (USB/DIN). */
midi_tx_stats_t midi_tx_stats = {0};

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
#error "MIDI_EP_SIZE doit valoir 64."
#endif

/* ====================================================================== */
/*                        THREAD DE TRANSMISSION USB                      */
/* ====================================================================== */

/**
 * @brief Zone de travail du thread de transmission USB-MIDI.
 */
static CCM_DATA THD_WORKING_AREA(waMidiUsbTx, 512);

/**
 * @brief Thread d’agrégation et d’envoi USB-MIDI.
 *
 * Récupère des paquets USB-MIDI (4 octets packés dans un `msg_t`) depuis
 * la mailbox @ref midi_usb_mb, les agrège dans un buffer de 64 octets et
 * déclenche `usbStartTransmitI()` sur EP IN (EP2).
 *
 * Politique de robustesse :
 * - Si l’USB n’est pas prêt, les paquets sont comptabilisés en
 *   @ref midi_tx_stats.usb_not_ready_drops.
 * - Si le sémaphore n’est pas obtenu dans @ref MIDI_USB_TX_WAIT_MS ms,
 *   le lot courant est **abandonné** (drop contrôlé) pour éviter tout blocage.
 *
 * @param arg Argument inutilisé.
 */
static THD_FUNCTION(thdMidiUsbTx, arg) {
  (void)arg;
#if CH_CFG_USE_REGISTRY
  chRegSetThreadName("MIDI_USB_TX");
#endif
  uint8_t buf[64];
  size_t n = 0;

  while (true) {
    msg_t msg;
    msg_t res = chMBFetchTimeout(&midi_usb_mb, &msg, TIME_MS2I(1));

    if (res == MSG_OK) {
      midi_usb_queue_decrement();
      buf[n++] = (uint8_t)((msg >> 24) & 0xFF);
      buf[n++] = (uint8_t)((msg >> 16) & 0xFF);
      buf[n++] = (uint8_t)((msg >> 8)  & 0xFF);
      buf[n++] = (uint8_t)( msg        & 0xFF);

      if (n == sizeof(buf)) {
        if (usb_midi_tx_ready) {
          const systime_t tw = TIME_MS2I(MIDI_USB_TX_WAIT_MS);
          if (chBSemWaitTimeout(&tx_sem, tw) == MSG_OK) {
            osalSysLock();
            usbStartTransmitI(&USBD1, MIDI_EP_IN, buf, n);
            osalSysUnlock();
            midi_tx_stats.tx_sent_batched++;
          } else {
            /* Endpoint non réarmé à temps : abandon contrôlé du lot. */
            midi_tx_stats.usb_not_ready_drops += n / 4;
          }
        } else {
          midi_tx_stats.usb_not_ready_drops += n / 4;
        }
        n = 0;
      }
    } else if (n > 0) {
      /* Flush du lot partiel après courte période d'inactivité. */
      if (usb_midi_tx_ready) {
        const systime_t tw = TIME_MS2I(MIDI_USB_TX_WAIT_MS);
        if (chBSemWaitTimeout(&tx_sem, tw) == MSG_OK) {
          osalSysLock();
          usbStartTransmitI(&USBD1, MIDI_EP_IN, buf, n);
          osalSysUnlock();
          midi_tx_stats.tx_sent_batched++;
        } else {
          midi_tx_stats.usb_not_ready_drops += n / 4;
        }
      } else {
        midi_tx_stats.usb_not_ready_drops += n / 4;
      }
      n = 0;
    }
  }
}

/* ====================================================================== */
/*                          INITIALISATION DU MODULE                      */
/* ====================================================================== */

/**
 * @brief Initialise le sous-système MIDI.
 *
 * - Configure l’UART DIN à 31250 bauds,
 * - Initialise la mailbox et son buffer circulaire,
 * - Initialise le sémaphore d’EP libre,
 * - Démarre le thread de transmission USB-MIDI.
 */
void midi_init(void) {
  static const SerialConfig uart_cfg = { 31250, 0, 0, 0 };
  sdStart(MIDI_UART, &uart_cfg);
  midi_usb_queue_fill = 0;
  midi_usb_queue_high_water = 0;
  chMBObjectInit(&midi_usb_mb, midi_usb_queue, MIDI_USB_QUEUE_LEN);
  chBSemObjectInit(&tx_sem, true);
  chThdCreateStatic(waMidiUsbTx, sizeof(waMidiUsbTx),
                    MIDI_USB_TX_PRIO, thdMidiUsbTx, NULL);
}

/* ====================================================================== */
/*                           FONCTIONS BAS NIVEAU                         */
/* ====================================================================== */

/**
 * @brief Envoie un message brut sur la sortie DIN (UART).
 * @param msg Pointeur sur les octets du message MIDI.
 * @param len Longueur en octets du message.
 */
static void send_uart(const uint8_t *msg, size_t len) { sdWrite(MIDI_UART, msg, len); }

/**
 * @brief Poste un paquet USB-MIDI (4 octets packés) dans la mailbox, sinon le supprime.
 *
 * @param m Paquet USB-MIDI encodé dans un `msg_t` (octet 0 dans bits 31..24).
 * @param force_drop_oldest Si vrai, retire le plus ancien élément de la mailbox
 *                          pour insérer le nouveau (politique “drop-oldest”).
 *                          Sinon, le paquet courant est perdu si la file est pleine.
 */
static void post_mb_or_drop(msg_t m, bool force_drop_oldest) {
  if (chMBPostTimeout(&midi_usb_mb, m, TIME_IMMEDIATE) != MSG_OK) {
    if (force_drop_oldest || MIDI_MB_DROP_OLDEST) {
      msg_t throwaway;
      if (chMBFetchTimeout(&midi_usb_mb, &throwaway, TIME_IMMEDIATE) == MSG_OK) {
        midi_usb_queue_decrement();
      }
      if (chMBPostTimeout(&midi_usb_mb, m, TIME_IMMEDIATE) != MSG_OK)
        midi_tx_stats.tx_mb_drops++;
      else
        midi_usb_queue_increment();
    } else {
      midi_tx_stats.tx_mb_drops++;
    }
  } else {
    midi_usb_queue_increment();
  }
}

/**
 * @brief Micro-attente maximale (µs) tolérée pour envoyer une Note (endpoint opportuniste).
 * @details Si l’EP n’est pas saisi dans ce délai, le paquet partira via la mailbox (agrégé).
 */
#ifndef MIDI_NOTE_MICROWAIT_US
#define MIDI_NOTE_MICROWAIT_US  80
#endif

/* ====================================================================== */
/*                       TRANSMISSION USB (PROTOCOLE)                     */
/* ====================================================================== */

/**
 * @brief Construit un paquet USB-MIDI (4 octets) et déclenche son envoi.
 *
 * Règles principales :
 * - Mappage correct du **CIN** selon le type de message (NoteOn=0x9, CC=0xB, Realtime=0xF, etc.),
 * - Zéro-padding pour les messages courts (1 ou 2 octets),
 * - Priorité aux **Realtime** :
 *   - `0xF8` (Clock) : micro-attente plus longue pour envoi immédiat si possible,
 *   - `FA/FB/FC/FE/FF` : micro-attente courte, sinon fallback mailbox avec politique configurable,
 * - Pour les **Notes** : petite fenêtre d’envoi direct (`MIDI_NOTE_MICROWAIT_US`), sinon agrégation.
 *
 * @param msg Pointeur vers le message MIDI (status + data).
 * @param len Taille du message en octets (1 à 3 selon le type).
 */
static void send_usb(const uint8_t *msg, size_t len) {
  uint8_t packet[4]={0,0,0,0};
  const uint8_t st = msg[0];
  const uint8_t cable = (uint8_t)(MIDI_USB_CABLE<<4);

  bool is_note=false;

  /* Channel Voice */
  if ((st & 0xF0)==0x80 && len>=3){ packet[0]=cable|0x08; packet[1]=msg[0]; packet[2]=msg[1]; packet[3]=msg[2]; is_note=true; }
  else if ((st & 0xF0)==0x90 && len>=3){ packet[0]=cable|0x09; packet[1]=msg[0]; packet[2]=msg[1]; packet[3]=msg[2]; is_note=true; }
  else if ((st & 0xF0)==0xA0 && len>=3){ packet[0]=cable|0x0A; packet[1]=msg[0]; packet[2]=msg[1]; packet[3]=msg[2]; }
  else if ((st & 0xF0)==0xB0 && len>=3){ packet[0]=cable|0x0B; packet[1]=msg[0]; packet[2]=msg[1]; packet[3]=msg[2]; }
  else if ((st & 0xF0)==0xE0 && len>=3){ packet[0]=cable|0x0E; packet[1]=msg[0]; packet[2]=msg[1]; packet[3]=msg[2]; }
  else if ((st & 0xF0)==0xC0 && len>=2){ packet[0]=cable|0x0C; packet[1]=msg[0]; packet[2]=msg[1]; packet[3]=0; }
  else if ((st & 0xF0)==0xD0 && len>=2){ packet[0]=cable|0x0D; packet[1]=msg[0]; packet[2]=msg[1]; packet[3]=0; }

  /* System Common */
  else if (st==0xF1 && len>=2){ packet[0]=cable|0x02; packet[1]=0xF1; packet[2]=msg[1]; }
  else if (st==0xF2 && len>=3){ packet[0]=cable|0x03; packet[1]=0xF2; packet[2]=msg[1]; packet[3]=msg[2]; }
  else if (st==0xF3 && len>=2){ packet[0]=cable|0x02; packet[1]=0xF3; packet[2]=msg[1]; }
  else if (st==0xF6){          packet[0]=cable|0x0F; packet[1]=0xF6; }

  /* Realtime */
  else if (st>=0xF8){
    packet[0]=cable|0x0F; packet[1]=st;

    if (st==0xF8){
      systime_t tw=TIME_US2I(1000);
      if (usb_midi_tx_ready && chBSemWaitTimeout(&tx_sem, tw)==MSG_OK){
        osalSysLock(); usbStartTransmitI(&USBD1, MIDI_EP_IN, packet, 4); osalSysUnlock();
        midi_tx_stats.tx_sent_immediate++;
      } else {
        msg_t m=((msg_t)packet[0]<<24)|((msg_t)packet[1]<<16)|((msg_t)packet[2]<<8)|packet[3];
        post_mb_or_drop(m,false);
      }
      return;
    }

    if (st==0xFA || st==0xFB || st==0xFC || st==0xFE || st==0xFF){
      systime_t tw=TIME_US2I(50);
      if (usb_midi_tx_ready && chBSemWaitTimeout(&tx_sem, tw)==MSG_OK){
        osalSysLock(); usbStartTransmitI(&USBD1, MIDI_EP_IN, packet, 4); osalSysUnlock();
        midi_tx_stats.tx_sent_immediate++;
      } else {
        midi_tx_stats.rt_other_enq_fallback++;
        msg_t m=((msg_t)packet[0]<<24)|((msg_t)packet[1]<<16)|((msg_t)packet[2]<<8)|packet[3];
        post_mb_or_drop(m,true);
      }
      return;
    }

    msg_t m3=((msg_t)packet[0]<<24)|((msg_t)packet[1]<<16)|((msg_t)packet[2]<<8)|packet[3];
    post_mb_or_drop(m3,false);
    return;
  }

  else { packet[0]=cable|0x0F; packet[1]=len>0?msg[0]:0; packet[2]=len>1?msg[1]:0; packet[3]=len>2?msg[2]:0; }

  if (is_note){
    const systime_t tw=TIME_US2I(MIDI_NOTE_MICROWAIT_US);
    if (usb_midi_tx_ready && chBSemWaitTimeout(&tx_sem, tw)==MSG_OK){
      osalSysLock(); usbStartTransmitI(&USBD1, MIDI_EP_IN, packet, 4); osalSysUnlock();
      midi_tx_stats.tx_sent_immediate++; return;
    }
  }

  msg_t m=((msg_t)packet[0]<<24)|((msg_t)packet[1]<<16)|((msg_t)packet[2]<<8)|packet[3];
  post_mb_or_drop(m,false);
}

/* ====================================================================== */
/*                              ROUTAGE MIDI                              */
/* ====================================================================== */

/**
 * @brief Envoie un message MIDI vers la destination choisie.
 * @param d Destination d’envoi (UART, USB, ou les deux).
 * @param m Pointeur sur les octets du message MIDI.
 * @param n Longueur du message en octets.
 */
static void midi_send(midi_dest_t d, const uint8_t *m, size_t n){
  switch(d){
    case MIDI_DEST_UART: send_uart(m,n); break;
    case MIDI_DEST_USB:  send_usb(m,n);  break;
    case MIDI_DEST_BOTH: send_uart(m,n); send_usb(m,n); break;
    default: break;
  }
}

/* ====================================================================== */
/*                                API MIDI                                */
/* ====================================================================== */

void midi_note_on(midi_dest_t d,uint8_t ch,uint8_t n,uint8_t v){
  if ((v & 0x7F)==0){ midi_note_off(d,ch,n,0); return; }
  uint8_t m[3]={ (uint8_t)(0x90|(ch&0x0F)), (uint8_t)(n&0x7F), (uint8_t)(v&0x7F) };
  midi_send(d,m,3);
}

void midi_note_off(midi_dest_t d,uint8_t ch,uint8_t n,uint8_t v){
  uint8_t m[3]={ (uint8_t)(0x80|(ch&0x0F)), (uint8_t)(n&0x7F), (uint8_t)(v&0x7F) };
  midi_send(d,m,3);
}

void midi_poly_aftertouch(midi_dest_t d,uint8_t ch,uint8_t n,uint8_t p){
  uint8_t m[3]={ (uint8_t)(0xA0|(ch&0x0F)), (uint8_t)(n&0x7F), (uint8_t)(p&0x7F) };
  midi_send(d,m,3);
}

void midi_cc(midi_dest_t d,uint8_t ch,uint8_t c,uint8_t v){
  uint8_t m[3]={ (uint8_t)(0xB0|(ch&0x0F)), (uint8_t)(c&0x7F), (uint8_t)(v&0x7F) };
  midi_send(d,m,3);
}

void midi_program_change(midi_dest_t d,uint8_t ch,uint8_t pg){
  uint8_t m[2]={ (uint8_t)(0xC0|(ch&0x0F)), (uint8_t)(pg&0x7F) };
  midi_send(d,m,2);
}

void midi_channel_pressure(midi_dest_t d,uint8_t ch,uint8_t p){
  uint8_t m[2]={ (uint8_t)(0xD0|(ch&0x0F)), (uint8_t)(p&0x7F) };
  midi_send(d,m,2);
}

void midi_pitchbend(midi_dest_t d,uint8_t ch,int16_t v14){
  uint16_t u=(uint16_t)(v14+8192);
  uint8_t l=u&0x7F, m=(u>>7)&0x7F;
  uint8_t msg[3]={ (uint8_t)(0xE0|(ch&0x0F)), l, m };
  midi_send(d,msg,3);
}

void midi_mtc_quarter_frame(midi_dest_t d,uint8_t qf){
  uint8_t m[2]={ 0xF1,(uint8_t)(qf&0x7F) };
  midi_send(d,m,2);
}

void midi_song_position(midi_dest_t d,uint16_t p14){
  uint8_t l=(uint8_t)(p14&0x7F),h=(uint8_t)((p14>>7)&0x7F);
  uint8_t m[3]={0xF2,l,h};
  midi_send(d,m,3);
}

void midi_song_select(midi_dest_t d,uint8_t s){
  uint8_t m[2]={ 0xF3,(uint8_t)(s&0x7F) };
  midi_send(d,m,2);
}

void midi_tune_request(midi_dest_t d){
  (void)d;
  uint8_t m[1]={0xF6};
  midi_send(MIDI_DEST_BOTH,m,1);
}

void midi_clock(midi_dest_t d){
  (void)d;
  uint8_t m[1]={0xF8};
  midi_send(MIDI_DEST_BOTH,m,1);
}

void midi_start(midi_dest_t d){
  (void)d;
  uint8_t m[1]={0xFA};
  send_usb(m,1);
  send_uart(m,1);
}

void midi_continue(midi_dest_t d){
  (void)d;
  uint8_t m[1]={0xFB};
  send_usb(m,1);
  send_uart(m,1);
}

void midi_stop(midi_dest_t d){
  (void)d;
  uint8_t m[1]={0xFC};
  send_usb(m,1);
  send_uart(m,1);
}

void midi_active_sensing(midi_dest_t d){
  (void)d;
  uint8_t m[1]={0xFE};
  midi_send(MIDI_DEST_BOTH,m,1);
}

void midi_system_reset(midi_dest_t d){
  (void)d;
  uint8_t m[1]={0xFF};
  midi_send(MIDI_DEST_BOTH,m,1);
}

// --- FIX: centraliser les Channel Mode messages (All Notes Off & cie) dans midi.c ---
static void midi_channel_mode_cc(midi_dest_t dest, uint8_t ch, uint8_t control, uint8_t value) {
  uint8_t msg[3] = {
      (uint8_t)(0xB0 | (ch & 0x0F)),
      (uint8_t)(control & 0x7F),
      (uint8_t)(value & 0x7F)
  };
  midi_send(dest, msg, 3);
}

void midi_all_sound_off(midi_dest_t dest, uint8_t ch) {
  midi_channel_mode_cc(dest, ch, 120U, 0U);
}

void midi_reset_all_controllers(midi_dest_t dest, uint8_t ch) {
  midi_channel_mode_cc(dest, ch, 121U, 0U);
}

void midi_local_control(midi_dest_t dest, uint8_t ch, bool on) {
  midi_channel_mode_cc(dest, ch, 122U, on ? 127U : 0U);
}

void midi_all_notes_off(midi_dest_t dest, uint8_t ch) {
  midi_channel_mode_cc(dest, ch, 123U, 0U);
}

void midi_omni_mode_off(midi_dest_t dest, uint8_t ch) {
  midi_channel_mode_cc(dest, ch, 124U, 0U);
}

void midi_omni_mode_on(midi_dest_t dest, uint8_t ch) {
  midi_channel_mode_cc(dest, ch, 125U, 0U);
}

void midi_mono_mode_on(midi_dest_t dest, uint8_t ch, uint8_t num_channels) {
  midi_channel_mode_cc(dest, ch, 126U, num_channels);
}

void midi_poly_mode_on(midi_dest_t dest, uint8_t ch) {
  midi_channel_mode_cc(dest, ch, 127U, 0U);
}

uint16_t midi_usb_queue_high_watermark(void) {
  return midi_usb_queue_high_water;
}

/**
 * @brief Réinitialise les statistiques de transmission MIDI.
 */
void midi_stats_reset(void){
  midi_tx_stats=(midi_tx_stats_t){0};
}
