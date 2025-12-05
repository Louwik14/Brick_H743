/**
 * @file system_init.c
 * @brief Séquence d'initialisation déterministe pour Brick (STM32H743 + ChibiOS).
 *
 * Ordre d'init imposé : MPU/.ram_d2 → caches I/D → SDRAM (+ BIST rapide) →
 * drivers bas niveau → USB device/host + MIDI → SD (init/mount optionnel) →
 * audio (init + start optionnel). L'audio reste la source unique du temps :
 * aucune horloge parallèle n'est démarrée ici, seule l'activation du driver
 * audio est conditionnée par SYSTEM_AUTO_START_AUDIO.
 *
 * Dépendances mémoire : mpu_config_init_once() programme la région .ram_d2
 * non-cacheable (DMA audio/SD/LEDs). SDRAM configure ses propres régions MPU
 * via sdram_init(). D-Cache est activé après la configuration MPU pour
 * garantir la cohérence des attributs mémoire.
 *
 * Rationale : fournir une API idempotente (boot-only, sans lock), simple et
 * industrielle qui évite tout oubli dans l'ordre critique (MPU → SDRAM →
 * audio/USB/SD) et centralise la gestion d'erreurs boot-time. Les drivers
 * audio/USB ne remontent pas d'échec code, les erreurs éventuelles ne sont donc
 * pas observables ici.
 */

#include "system_init.h"

#include "ch.h"
#include "hal.h"
#include "drivers.h"
#include "drv_audio.h"
#include "drv_sd.h"
#include "midi.h"
#include "mpu_config.h"
#include "sdram_driver.h"
#include "usb_device.h"
#include "usb_host_midi.h"
#include "core_cm7.h"

#ifndef SYSTEM_AUTO_MOUNT_SD
#define SYSTEM_AUTO_MOUNT_SD 0
#endif

#ifndef SYSTEM_AUTO_START_AUDIO
#define SYSTEM_AUTO_START_AUDIO 0
#endif

static sys_status_t last_error = SYS_OK;
static bool early_done = false;
static bool drivers_done = false;
static bool late_done = false;
static bool caches_enabled = false;
static bool audio_initialized = false;
static bool audio_started = false;

static void record_error(sys_status_t err) {
  if (last_error == SYS_OK) {
    last_error = err;
  }
}

static void enable_caches_once(void) {
  if (caches_enabled) {
    return;
  }
  SCB_EnableICache();
  SCB_EnableDCache();
  caches_enabled = true;
}

sys_status_t system_init_early(void) {
  if (early_done) {
    return last_error;
  }

  if (!mpu_config_init_once()) {
    record_error(SYS_ERR_MPU);
    return last_error;
  }

  enable_caches_once();

  sdram_init(true);
  const sdram_state_t sdram_state = sdram_status();
  if (sdram_state != SDRAM_READY) {
    record_error(SYS_ERR_SDRAM);
    return last_error;
  }

  early_done = true;
  return last_error;
}

sys_status_t system_init_drivers(void) {
  if (last_error != SYS_OK) {
    return last_error;
  }

  if (drivers_done) {
    return last_error;
  }

  drivers_init_all();

  drivers_done = true;
  return last_error;
}

sys_status_t system_init_late(void) {
  if (last_error != SYS_OK) {
    return last_error;
  }

  if (late_done) {
    return last_error;
  }

  usb_device_start();
  usb_host_midi_init();
  midi_init();

  const sd_error_t sd_init_res = drv_sd_init();
  if (sd_init_res == SD_OK) {
    if (SYSTEM_AUTO_MOUNT_SD) {
      const sd_error_t sd_mount_res = drv_sd_mount(false);
      if (sd_mount_res != SD_OK && sd_mount_res != SD_ERR_NO_CARD) {
        record_error(SYS_ERR_SD);
        return last_error;
      }
    }
  } else if (sd_init_res != SD_ERR_NO_CARD) {
    record_error(SYS_ERR_SD);
    return last_error;
  }

  if (!audio_initialized) {
    drv_audio_init();
    audio_initialized = true;
  }

  if (SYSTEM_AUTO_START_AUDIO && !audio_started) {
    drv_audio_start();
    audio_started = true;
  }

  late_done = true;
  return last_error;
}

bool system_is_initialized(void) {
  return (last_error == SYS_OK) && early_done && drivers_done && late_done;
}

sys_status_t system_last_error(void) { return last_error; }
