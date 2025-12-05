#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SYS_OK = 0,
  SYS_ERR_MPU,
  SYS_ERR_SDRAM,
  SYS_ERR_AUDIO,
  SYS_ERR_SD,
  SYS_ERR_USB_DEVICE,
  SYS_ERR_USB_HOST
} sys_status_t;

sys_status_t system_init_early(void);
sys_status_t system_init_drivers(void);
sys_status_t system_init_late(void);

bool system_is_initialized(void);
sys_status_t system_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_INIT_H */
