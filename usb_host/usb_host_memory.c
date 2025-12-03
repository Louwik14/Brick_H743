#include "ch.h"
#include "usbh_conf.h"

#include <stdint.h>

#define USBH_STATIC_ALIGNMENT               32U

static uint8_t usbh_static_pool[USBH_STATIC_MEM_SIZE] __attribute__((aligned(USBH_STATIC_ALIGNMENT)));
static size_t usbh_static_offset = 0U;
static uint32_t usbh_static_oom_count = 0U;

static size_t usbh_align_size(size_t size)
{
  return (size + (USBH_STATIC_ALIGNMENT - 1U)) & ~(USBH_STATIC_ALIGNMENT - 1U);
}

void USBH_static_mem_reset(void)
{
  chSysLock();
  usbh_static_offset = 0U;
  usbh_static_oom_count = 0U;
  chSysUnlock();
}

void *USBH_static_malloc(size_t size)
{
  void *ptr = NULL;
  size_t aligned = usbh_align_size(size);

  chSysLock();
  if ((USBH_STATIC_MEM_SIZE - usbh_static_offset) >= aligned)
  {
    ptr = &usbh_static_pool[usbh_static_offset];
    usbh_static_offset += aligned;
  }
  else
  {
    usbh_static_oom_count++;
    USBH_ErrLog("USBH OOM: requested %u bytes", (unsigned int)size);
  }
  chSysUnlock();

  return ptr;
}

void USBH_static_free(void *p)
{
  (void)p;
}

uint32_t USBH_static_get_oom_count(void)
{
  uint32_t count;

  chSysLock();
  count = usbh_static_oom_count;
  chSysUnlock();

  return count;
}
