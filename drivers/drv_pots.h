#ifndef DRV_POTS_H
#define DRV_POTS_H

#include <stdint.h>

void     drv_pots_init(void);
void     drv_pots_start(void);
void     drv_pots_stop(void);
uint16_t drv_pots_get_raw(uint8_t index);
uint32_t drv_pots_get_error_count(void);

#endif /* DRV_POTS_H */
