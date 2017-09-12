#ifndef __USBD_CDC_IF_H
#define __USBD_CDC_IF_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "usbd_cdc.h"

extern USBD_CDC_ItfTypeDef  USBD_Interface_fops_FS;
extern void usbd_cdc_if_check_tx(void);

#ifdef __cplusplus
}
#endif
  
#endif /* __USBD_CDC_IF_H */
