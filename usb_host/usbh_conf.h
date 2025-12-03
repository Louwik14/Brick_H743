#ifndef __USBH_CONF_H
#define __USBH_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define USBH_MAX_NUM_ENDPOINTS        2
#define USBH_MAX_NUM_INTERFACES       1
#define USBH_MAX_NUM_CONFIGURATION   1
#define USBH_KEEP_CFG_DESCRIPTOR     1
#define USBH_MAX_NUM_SUPPORTED_CLASS 1

#define USBH_DMA_ENABLE               1
#define USBH_USE_OS                   0

#define USBH_PROCESS_PRIO             5
#define USBH_PROCESS_STACK_SIZE       0x800

#define USBH_DEBUG_LEVEL              0

#define USBH_malloc                   malloc
#define USBH_free                     free

/* Debug print redirection */
#define USBH_UsrLog(...)   do {} while(0)
#define USBH_ErrLog(...)   do {} while(0)
#define USBH_DbgLog(...)   do {} while(0)

#ifdef __cplusplus
}
#endif

#endif /* __USBH_CONF_H */
