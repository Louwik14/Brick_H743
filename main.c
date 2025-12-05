#include "ch.h"
#include "hal.h"
#include "drivers.h"      // qui inclut drv_display.h
#include "system_init.h"

int main(void) {
  halInit();
  chSysInit();

  if (system_init_early() != SYS_OK) {
    chSysHalt("SYS EARLY FAIL");
  }

  if (system_init_drivers() != SYS_OK) {
    chSysHalt("SYS DRIVERS FAIL");
  }

  if (system_init_late() != SYS_OK) {
    chSysHalt("SYS LATE FAIL");
  }

  drv_display_clear();
  drv_display_draw_text(0, 0, "HELLO H743");
  drv_display_update();   // si tu n’utilises pas le thread

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
