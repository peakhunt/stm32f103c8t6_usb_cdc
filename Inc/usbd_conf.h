#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__
#ifdef __cplusplus
 extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#include "usbd_def.h"

/*---------- -----------*/
#define USBD_MAX_NUM_INTERFACES     4
/*---------- -----------*/
#define USBD_MAX_NUM_CONFIGURATION     1
/*---------- -----------*/
#define USBD_MAX_STR_DESC_SIZ     512
/*---------- -----------*/
#define USBD_SUPPORT_USER_STRING     0
/*---------- -----------*/
#define USBD_DEBUG_LEVEL     0
/*---------- -----------*/
#define USBD_SELF_POWERED     1
/*---------- -----------*/
#define USBD_CDC_INTERVAL     1000
/*---------- -----------*/
#define MAX_STATIC_ALLOC_SIZE     512
/****************************************/
/* #define for FS and HS identification */
#define DEVICE_FS 		0

/* Memory management macros */  
#define USBD_malloc               (uint32_t *)USBD_static_malloc
#define USBD_free                 USBD_static_free
#define USBD_memset               /* Not used */
#define USBD_memcpy               /* Not used */

#define USBD_Delay   HAL_Delay

/* For footprint reasons and since only one allocation is handled in the HID class
   driver, the malloc/free is changed into a static allocation method */
void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);    

/* DEBUG macros */    
#if (USBD_DEBUG_LEVEL > 0)
#define  USBD_UsrLog(...)   printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_UsrLog(...)   
#endif 
                            
                            
#if (USBD_DEBUG_LEVEL > 1)

#define  USBD_ErrLog(...)   printf("ERROR: ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_ErrLog(...)   
#endif 
                            
                            
#if (USBD_DEBUG_LEVEL > 2)                         
#define  USBD_DbgLog(...)   printf("DEBUG : ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_DbgLog(...)                         
#endif
                            
#ifdef __cplusplus
}
#endif

#endif /*__USBD_CONF__H__*/
