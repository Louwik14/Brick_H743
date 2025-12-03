#include "ch.h"
#include "usbh_conf.h"

#include <stdint.h>

#define USBH_STATIC_ALIGNMENT               32U

static uint8_t usbh_static_pool[USBH_STATIC_MEM_SIZE] __attribute__((aligned(USBH_STATIC_ALIGNMENT)));
static size_t usbh_static_offset = 0U;

static size_t usbh_align_size(size_t size)
{
  return (size + (USBH_STATIC_ALIGNMENT - 1U)) & ~(USBH_STATIC_ALIGNMENT - 1U);
}

void USBH_static_mem_reset(void)
{
  osalSysLock();
  usbh_static_offset = 0U;
  osalSysUnlock();
}

void *USBH_static_malloc(size_t size)
{
  void *ptr = NULL;
  size_t aligned = usbh_align_size(size);

  osalSysLock();
  if ((USBH_STATIC_MEM_SIZE - usbh_static_offset) >= aligned)
  {
    ptr = &usbh_static_pool[usbh_static_offset];
    usbh_static_offset += aligned;
  }
  osalSysUnlock();

  return ptr;
}

void USBH_static_free(void *p)
{
  (void)p;
}
