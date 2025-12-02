#include "ch.h"
#include "hal.h"
#include "drivers.h"      // qui inclut drv_display.h

int main(void) {
  halInit();
  chSysInit();

  drivers_init_all();     // appelle drv_display_init() ou drv_display_start() selon ton choix

  drv_display_clear();
  drv_display_draw_text(0, 0, "HELLO H743");
  drv_display_update();   // si tu n’utilises pas le thread

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
