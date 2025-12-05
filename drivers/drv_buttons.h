#ifndef DRV_BUTTONS_H
#define DRV_BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

#define DRV_BUTTONS_COUNT   20   /* 74HC165 : 20 boutons = 3 octets */

void     drv_buttons_init(void);
void     drv_buttons_start(void);
void     drv_buttons_stop(void);

bool     drv_button_get(uint8_t index);   /* lecture 1 bouton */
uint32_t drv_buttons_get_mask(void);      /* snapshot complet */

#endif /* DRV_BUTTONS_H */
