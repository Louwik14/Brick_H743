#ifndef DRV_POTS_H
#define DRV_POTS_H

#include <stdint.h>

void     drv_pots_init(void);
void     drv_pots_start(void);
uint16_t drv_pots_get_raw(uint8_t index);

#endif /* DRV_POTS_H */
