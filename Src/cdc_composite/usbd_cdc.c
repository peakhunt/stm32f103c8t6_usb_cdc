#include "usbd_cdc.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"

static uint8_t  USBD_CDC_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_CDC_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_CDC_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t  USBD_CDC_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_CDC_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_CDC_EP0_RxReady (USBD_HandleTypeDef *pdev); 
static uint8_t  *USBD_CDC_GetFSCfgDesc (uint16_t *length);
static uint8_t  *USBD_CDC_GetHSCfgDesc (uint16_t *length);
static uint8_t  *USBD_CDC_GetOtherSpeedCfgDesc (uint16_t *length); 
uint8_t  *USBD_CDC_GetDeviceQualifierDescriptor (uint16_t *length);

static uint8_t    _cdc_in_eps[] =
{
  CDC0_IN_EP,
  CDC1_IN_EP,
};

static uint8_t    _cdc_out_eps[] =
{
  CDC0_OUT_EP,
  CDC1_OUT_EP,
};

static inline USBD_CDC_Instance
get_cdc_instance_from_interface(uint8_t intf)
{
  USBD_CDC_Instance instance;

  switch(intf)
  {
  case CDC0_CTRL_INTERFACE_NO:
    instance = USBD_CDC_Instance_0;
    break;

  case CDC1_CTRL_INTERFACE_NO:
    instance = USBD_CDC_Instance_1;
    break;

  default:
    instance = USBD_CDC_Instance_MAX;
  }

  return instance;
}

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CDC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,               // bLength
  USB_DESC_TYPE_DEVICE_QUALIFIER,           // bDescriptorType
  0x00,                                     // bcdUSB
  0x02,
  USB_CLASS_CDC,                            // bDeviceClass
  USB_CLASS_CDC_ACM,                        // bDeviceSubClass
  0x00,                                     // bDeviceProtocol
  CDC_DATA_FS_MAX_PACKET_SIZE,              // bMaxPacketSize0
  0x01,                                     // bNumConfigurations
  0x00,                                     // bReservefd
};

/* CDC interface class callbacks structure */
USBD_ClassTypeDef  USBD_CDC = 
{
  USBD_CDC_Init,
  USBD_CDC_DeInit,
  USBD_CDC_Setup,
  NULL,                 /* EP0_TxSent, */
  USBD_CDC_EP0_RxReady,
  USBD_CDC_DataIn,
  USBD_CDC_DataOut,
  NULL,
  NULL,
  NULL,     
  USBD_CDC_GetHSCfgDesc,  
  USBD_CDC_GetFSCfgDesc,    
  USBD_CDC_GetOtherSpeedCfgDesc, 
  USBD_CDC_GetDeviceQualifierDescriptor,
};

/* USB CDC device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_CDC_CfgHSDesc[USB_CDC_COMP_CONFIG_DESC_SIZE] __ALIGN_END =
{
  /* Configuration Descriptor */
  0x09,                             /* bLength: Configuration Descriptor size   */
  USB_DESC_TYPE_CONFIGURATION,      /* bDescriptorType: Configuration           */
  USB_CDC_COMP_CONFIG_DESC_SIZE,    /* wTotalLength:no of returned bytes        */
  0x00,
  0x04,                             /* bNumInterfaces: 4 interfaces for 2 CDC   */
  0x01,                             /* bConfigurationValue: Configuration value */
  0x00,                             /* iConfiguration: Index of string descriptor
                                       describing the configuration */
  0xC0,                             /* bmAttributes: self powered */
  0x32,                             /* MaxPower 0 mA */
  
  /*---------------------------------------------------------------------------*/
  /* CDC0 Interface Descriptor */
  /* IAD (Interface Association Descriptor) for CDC 0 */
  /*---------------------------------------------------------------------------*/
  USB_INTERFACE_ASSOCIATION_DESCSIZE,     /* bLength: Interface Descriptor size */
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR,   /* bDescriptorType                    */
  CDC0_CTRL_INTERFACE_NO,                 /* bFirstInterface                    */
  2,                                      /* bInterfaceCount                    */
  USB_CLASS_CDC,                          /* bFunctionClass                     */
  USB_CLASS_CDC_ACM,                      /* bFunctionSubClass                  */
  0,                                      /* bFunctionProtocol                  */
  0,                                      /* iFunction                          */
  
  /*---------------------------------------------------------------------------*/
  /* Communication Class Interface descriptor */
  0x09,                                   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType: Interface */
  CDC0_CTRL_INTERFACE_NO,                 /* bInterfaceNumber                   */
  0,                                      /* bAlternateSetting                  */
  1,                                      /* bNumEndpoints                      */
  USB_CLASS_CDC,                          /* bInterfaceClass                    */
  USB_CLASS_CDC_ACM,                      /* bInterfaceSubClass                 */
  0x0,                                    /* bInterfaceProtocol                 */
  0x0,                                    /* iInterface                         */

  /* Header Functional Descriptor */
  0x05,                                   /* bLength: Endpoint Descriptor size    */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x00,                                   /* bDescriptorSubtype: Header Func Desc */
  0x10,                                   /* bcdCDC: spec release number          */
  0x01,
  
  /* Call Management Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x01,                                   /* bDescriptorSubtype: Call Management
                                             Func Desc                            */
  0x00,                                   /* bmCapabilities: D0+D1                */
  CDC0_DATA_INTERFACE_NO,                 /* bDataInterface:                      */
  
  /* ACM Functional Descriptor */
  0x04,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x02,                                   /* bDescriptorSubtype: Abstract Control
                                             Management desc                      */
  0x02,                                   /* bmCapabilities                       */
  
  /* Union Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x06,                                   /* bDescriptorSubtype: Union func desc  */
  CDC0_CTRL_INTERFACE_NO,                 /* bMasterInterface:
                                             Communication class interface        */
  CDC0_DATA_INTERFACE_NO,                 /* bSlaveInterface0:
                                             Data Class Interface                 */
  /* Endpoint 2 Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size    */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint            */
  CDC0_CMD_EP,                            /* bEndpointAddress                     */
  0x03,                                   /* bmAttributes: Interrupt              */
  LOBYTE(CDC_CMD_PACKET_SIZE),            /* wMaxPacketSize:                      */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0x10,                                   /* bInterval:                           */ 
  
  /*---------------------------------------------------------------------------*/
  /* Data class interface descriptor */
  0x09,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType:                       */
  CDC0_DATA_INTERFACE_NO,                 /* bInterfaceNumber: Number of Interface  */
  0x00,                                   /* bAlternateSetting: Alternate setting   */
  0x02,                                   /* bNumEndpoints: Two endpoints used      */
  0x0A,                                   /* bInterfaceClass: CDC                   */
  0x00,                                   /* bInterfaceSubClass:                    */
  0x00,                                   /* bInterfaceProtocol:                    */
  0x00,                                   /* iInterface:                            */
  
  /* Endpoint OUT Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC0_OUT_EP,                            /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*Endpoint IN Descriptor*/
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC0_IN_EP,                             /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*---------------------------------------------------------------------------*/
  /* CDC1 Interface Descriptor */
  /* IAD (Interface Association Descriptor) for CDC 1 */
  /*---------------------------------------------------------------------------*/
  USB_INTERFACE_ASSOCIATION_DESCSIZE,     /* bLength: Interface Descriptor size */
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR,   /* bDescriptorType                    */
  CDC1_CTRL_INTERFACE_NO,                 /* bFirstInterface                    */
  2,                                      /* bInterfaceCount                    */
  USB_CLASS_CDC,                          /* bFunctionClass                     */
  USB_CLASS_CDC_ACM,                      /* bFunctionSubClass                  */
  0,                                      /* bFunctionProtocol                  */
  0,                                      /* iFunction                          */
  
  /*---------------------------------------------------------------------------*/
  /* Communication Class Interface descriptor */
  0x09,                                   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType: Interface */
  CDC1_CTRL_INTERFACE_NO,                 /* bInterfaceNumber                   */
  0,                                      /* bAlternateSetting                  */
  1,                                      /* bNumEndpoints                      */
  USB_CLASS_CDC,                          /* bInterfaceClass                    */
  USB_CLASS_CDC_ACM,                      /* bInterfaceSubClass                 */
  0x0,                                    /* bInterfaceProtocol                 */
  0x0,                                    /* iInterface                         */

  /* Header Functional Descriptor */
  0x05,                                   /* bLength: Endpoint Descriptor size    */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x00,                                   /* bDescriptorSubtype: Header Func Desc */
  0x10,                                   /* bcdCDC: spec release number          */
  0x01,
  
  /* Call Management Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x01,                                   /* bDescriptorSubtype: Call Management
                                             Func Desc                            */
  0x00,                                   /* bmCapabilities: D0+D1                */
  CDC1_DATA_INTERFACE_NO,                 /* bDataInterface:                      */
  
  /* ACM Functional Descriptor */
  0x04,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x02,                                   /* bDescriptorSubtype: Abstract Control
                                             Management desc                      */
  0x02,                                   /* bmCapabilities                       */
  
  /* Union Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x06,                                   /* bDescriptorSubtype: Union func desc  */
  CDC1_CTRL_INTERFACE_NO,                 /* bMasterInterface:
                                             Communication class interface        */
  CDC1_DATA_INTERFACE_NO,                 /* bSlaveInterface0:
                                             Data Class Interface                 */
  /* Endpoint 2 Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size    */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint            */
  CDC1_CMD_EP,                            /* bEndpointAddress                     */
  0x03,                                   /* bmAttributes: Interrupt              */
  LOBYTE(CDC_CMD_PACKET_SIZE),            /* wMaxPacketSize:                      */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0x10,                                   /* bInterval:                           */ 
  
  /*---------------------------------------------------------------------------*/
  /* Data class interface descriptor */
  0x09,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType:                       */
  CDC1_DATA_INTERFACE_NO,                 /* bInterfaceNumber: Number of Interface  */
  0x00,                                   /* bAlternateSetting: Alternate setting   */
  0x02,                                   /* bNumEndpoints: Two endpoints used      */
  0x0A,                                   /* bInterfaceClass: CDC                   */
  0x00,                                   /* bInterfaceSubClass:                    */
  0x00,                                   /* bInterfaceProtocol:                    */
  0x00,                                   /* iInterface:                            */

  /* Endpoint OUT Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC1_OUT_EP,                            /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*Endpoint IN Descriptor*/
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC1_IN_EP,                             /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */
  
#if 0
  /*---------------------------------------------------------------------------*/
  /* CDC2 Interface Descriptor */
  /* IAD (Interface Association Descriptor) for CDC 2 */
  /*---------------------------------------------------------------------------*/
  USB_INTERFACE_ASSOCIATION_DESCSIZE,     /* bLength: Interface Descriptor size */
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR,   /* bDescriptorType                    */
  CDC2_CTRL_INTERFACE_NO,                 /* bFirstInterface                    */
  2,                                      /* bInterfaceCount                    */
  USB_CLASS_CDC,                          /* bFunctionClass                     */
  USB_CLASS_CDC_ACM,                      /* bFunctionSubClass                  */
  0,                                      /* bFunctionProtocol                  */
  0,                                      /* iFunction                          */
  
  /*---------------------------------------------------------------------------*/
  /* Communication Class Interface descriptor */
  0x09,                                   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType: Interface */
  CDC2_CTRL_INTERFACE_NO,                 /* bInterfaceNumber                   */
  0,                                      /* bAlternateSetting                  */
  1,                                      /* bNumEndpoints                      */
  USB_CLASS_CDC,                          /* bInterfaceClass                    */
  USB_CLASS_CDC_ACM,                      /* bInterfaceSubClass                 */
  0x0,                                    /* bInterfaceProtocol                 */
  0x0,                                    /* iInterface                         */

  /* Header Functional Descriptor */
  0x05,                                   /* bLength: Endpoint Descriptor size    */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x00,                                   /* bDescriptorSubtype: Header Func Desc */
  0x10,                                   /* bcdCDC: spec release number          */
  0x01,
  
  /* Call Management Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x01,                                   /* bDescriptorSubtype: Call Management
                                             Func Desc                            */
  0x00,                                   /* bmCapabilities: D0+D1                */
  CDC2_DATA_INTERFACE_NO,                 /* bDataInterface:                      */
  
  /* ACM Functional Descriptor */
  0x04,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x02,                                   /* bDescriptorSubtype: Abstract Control
                                             Management desc                      */
  0x02,                                   /* bmCapabilities                       */
  
  /* Union Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x06,                                   /* bDescriptorSubtype: Union func desc  */
  CDC2_CTRL_INTERFACE_NO,                 /* bMasterInterface:
                                             Communication class interface        */
  CDC2_DATA_INTERFACE_NO,                 /* bSlaveInterface0:
                                             Data Class Interface                 */
  /* Endpoint 2 Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size    */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint            */
  CDC2_CMD_EP,                            /* bEndpointAddress                     */
  0x03,                                   /* bmAttributes: Interrupt              */
  LOBYTE(CDC_CMD_PACKET_SIZE),            /* wMaxPacketSize:                      */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0x10,                                   /* bInterval:                           */ 
  
  /*---------------------------------------------------------------------------*/
  /* Data class interface descriptor */
  0x09,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType:                       */
  CDC2_DATA_INTERFACE_NO,                 /* bInterfaceNumber: Number of Interface  */
  0x00,                                   /* bAlternateSetting: Alternate setting   */
  0x02,                                   /* bNumEndpoints: Two endpoints used      */
  0x0A,                                   /* bInterfaceClass: CDC                   */
  0x00,                                   /* bInterfaceSubClass:                    */
  0x00,                                   /* bInterfaceProtocol:                    */
  0x00,                                   /* iInterface:                            */
  
  /* Endpoint OUT Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC2_OUT_EP,                            /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*Endpoint IN Descriptor*/
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC2_IN_EP,                             /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */
#endif
} ;


/* USB CDC device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_CDC_CfgFSDesc[USB_CDC_COMP_CONFIG_DESC_SIZE] __ALIGN_END =
{
  /* Configuration Descriptor */
  0x09,                             /* bLength: Configuration Descriptor size   */
  USB_DESC_TYPE_CONFIGURATION,      /* bDescriptorType: Configuration           */
  USB_CDC_COMP_CONFIG_DESC_SIZE,    /* wTotalLength:no of returned bytes        */
  0x00,
  0x04,                             /* bNumInterfaces: 6 interfaces for 3 CDC   */
  0x01,                             /* bConfigurationValue: Configuration value */
  0x00,                             /* iConfiguration: Index of string descriptor
                                       describing the configuration */
  0xC0,                             /* bmAttributes: self powered */
  0x32,                             /* MaxPower 0 mA */
  
  /*---------------------------------------------------------------------------*/
  /* CDC0 Interface Descriptor */
  /* IAD (Interface Association Descriptor) for CDC 0 */
  /*---------------------------------------------------------------------------*/
  USB_INTERFACE_ASSOCIATION_DESCSIZE,     /* bLength: Interface Descriptor size */
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR,   /* bDescriptorType                    */
  CDC0_CTRL_INTERFACE_NO,                 /* bFirstInterface                    */
  2,                                      /* bInterfaceCount                    */
  USB_CLASS_CDC,                          /* bFunctionClass                     */
  USB_CLASS_CDC_ACM,                      /* bFunctionSubClass                  */
  0,                                      /* bFunctionProtocol                  */
  0,                                      /* iFunction                          */

  /*---------------------------------------------------------------------------*/
  /* Communication Class Interface descriptor */
  0x09,                                   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType: Interface */
  CDC0_CTRL_INTERFACE_NO,                 /* bInterfaceNumber                   */
  0,                                      /* bAlternateSetting                  */
  1,                                      /* bNumEndpoints                      */
  USB_CLASS_CDC,                          /* bInterfaceClass                    */
  USB_CLASS_CDC_ACM,                      /* bInterfaceSubClass                 */
  0x0,                                    /* bInterfaceProtocol                 */
  0x0,                                    /* iInterface                         */
  
  /* Header Functional Descriptor */
  0x05,                                   /* bLength: Endpoint Descriptor size    */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x00,                                   /* bDescriptorSubtype: Header Func Desc */
  0x10,                                   /* bcdCDC: spec release number          */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x01,                                   /* bDescriptorSubtype: Call Management
                                             Func Desc                            */
  0x00,                                   /* bmCapabilities: D0+D1                */
  CDC0_DATA_INTERFACE_NO,                 /* bDataInterface:                      */

  /* ACM Functional Descriptor */
  0x04,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x02,                                   /* bDescriptorSubtype: Abstract Control
                                             Management desc                      */
  0x02,                                   /* bmCapabilities                       */

  /* Union Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x06,                                   /* bDescriptorSubtype: Union func desc  */
  CDC0_CTRL_INTERFACE_NO,                 /* bMasterInterface:
                                             Communication class interface        */
  CDC0_DATA_INTERFACE_NO,                 /* bSlaveInterface0:
                                             Data Class Interface                 */
  /* Endpoint 2 Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size    */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint            */
  CDC0_CMD_EP,                            /* bEndpointAddress                     */
  0x03,                                   /* bmAttributes: Interrupt              */
  LOBYTE(CDC_CMD_PACKET_SIZE),            /* wMaxPacketSize:                      */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0x10,                                   /* bInterval:                           */ 
  
  /*---------------------------------------------------------------------------*/
  /* Data class interface descriptor */
  0x09,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType:                       */
  CDC0_DATA_INTERFACE_NO,                 /* bInterfaceNumber: Number of Interface  */
  0x00,                                   /* bAlternateSetting: Alternate setting   */
  0x02,                                   /* bNumEndpoints: Two endpoints used      */
  0x0A,                                   /* bInterfaceClass: CDC                   */
  0x00,                                   /* bInterfaceSubClass:                    */
  0x00,                                   /* bInterfaceProtocol:                    */
  0x00,                                   /* iInterface:                            */

  /* Endpoint OUT Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC0_OUT_EP,                            /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*Endpoint IN Descriptor*/
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC0_IN_EP,                             /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*---------------------------------------------------------------------------*/
  /* CDC1 Interface Descriptor */
  /* IAD (Interface Association Descriptor) for CDC 1 */
  /*---------------------------------------------------------------------------*/
  USB_INTERFACE_ASSOCIATION_DESCSIZE,     /* bLength: Interface Descriptor size */
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR,   /* bDescriptorType                    */
  CDC1_CTRL_INTERFACE_NO,                 /* bFirstInterface                    */
  2,                                      /* bInterfaceCount                    */
  USB_CLASS_CDC,                          /* bFunctionClass                     */
  USB_CLASS_CDC_ACM,                      /* bFunctionSubClass                  */
  0,                                      /* bFunctionProtocol                  */
  0,                                      /* iFunction                          */

  /*---------------------------------------------------------------------------*/
  /* Communication Class Interface descriptor */
  0x09,                                   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType: Interface */
  CDC1_CTRL_INTERFACE_NO,                 /* bInterfaceNumber                   */
  0,                                      /* bAlternateSetting                  */
  1,                                      /* bNumEndpoints                      */
  USB_CLASS_CDC,                          /* bInterfaceClass                    */
  USB_CLASS_CDC_ACM,                      /* bInterfaceSubClass                 */
  0x0,                                    /* bInterfaceProtocol                 */
  0x0,                                    /* iInterface                         */
  
  /* Header Functional Descriptor */
  0x05,                                   /* bLength: Endpoint Descriptor size    */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x00,                                   /* bDescriptorSubtype: Header Func Desc */
  0x10,                                   /* bcdCDC: spec release number          */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x01,                                   /* bDescriptorSubtype: Call Management
                                             Func Desc                            */
  0x00,                                   /* bmCapabilities: D0+D1                */
  CDC1_DATA_INTERFACE_NO,                 /* bDataInterface:                      */

  /* ACM Functional Descriptor */
  0x04,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x02,                                   /* bDescriptorSubtype: Abstract Control
                                             Management desc                      */
  0x02,                                   /* bmCapabilities                       */

  /* Union Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x06,                                   /* bDescriptorSubtype: Union func desc  */
  CDC1_CTRL_INTERFACE_NO,                 /* bMasterInterface:
                                             Communication class interface        */
  CDC1_DATA_INTERFACE_NO,                 /* bSlaveInterface0:
                                             Data Class Interface                 */
  /* Endpoint 2 Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size    */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint            */
  CDC1_CMD_EP,                            /* bEndpointAddress                     */
  0x03,                                   /* bmAttributes: Interrupt              */
  LOBYTE(CDC_CMD_PACKET_SIZE),            /* wMaxPacketSize:                      */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0x10,                                   /* bInterval:                           */ 
  
  /*---------------------------------------------------------------------------*/
  /* Data class interface descriptor */
  0x09,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType:                       */
  CDC1_DATA_INTERFACE_NO,                 /* bInterfaceNumber: Number of Interface  */
  0x00,                                   /* bAlternateSetting: Alternate setting   */
  0x02,                                   /* bNumEndpoints: Two endpoints used      */
  0x0A,                                   /* bInterfaceClass: CDC                   */
  0x00,                                   /* bInterfaceSubClass:                    */
  0x00,                                   /* bInterfaceProtocol:                    */
  0x00,                                   /* iInterface:                            */

  /* Endpoint OUT Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC1_OUT_EP,                            /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*Endpoint IN Descriptor*/
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC1_IN_EP,                             /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

#if 0
  /*---------------------------------------------------------------------------*/
  /* CDC2 Interface Descriptor */
  /* IAD (Interface Association Descriptor) for CDC 2 */
  /*---------------------------------------------------------------------------*/
  USB_INTERFACE_ASSOCIATION_DESCSIZE,     /* bLength: Interface Descriptor size */
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR,   /* bDescriptorType                    */
  CDC2_CTRL_INTERFACE_NO,                 /* bFirstInterface                    */
  2,                                      /* bInterfaceCount                    */
  USB_CLASS_CDC,                          /* bFunctionClass                     */
  USB_CLASS_CDC_ACM,                      /* bFunctionSubClass                  */
  0,                                      /* bFunctionProtocol                  */
  0,                                      /* iFunction                          */

  /*---------------------------------------------------------------------------*/
  /* Communication Class Interface descriptor */
  0x09,                                   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType: Interface */
  CDC2_CTRL_INTERFACE_NO,                 /* bInterfaceNumber                   */
  0,                                      /* bAlternateSetting                  */
  1,                                      /* bNumEndpoints                      */
  USB_CLASS_CDC,                          /* bInterfaceClass                    */
  USB_CLASS_CDC_ACM,                      /* bInterfaceSubClass                 */
  0x0,                                    /* bInterfaceProtocol                 */
  0x0,                                    /* iInterface                         */
  
  /* Header Functional Descriptor */
  0x05,                                   /* bLength: Endpoint Descriptor size    */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x00,                                   /* bDescriptorSubtype: Header Func Desc */
  0x10,                                   /* bcdCDC: spec release number          */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x01,                                   /* bDescriptorSubtype: Call Management
                                             Func Desc                            */
  0x00,                                   /* bmCapabilities: D0+D1                */
  CDC2_DATA_INTERFACE_NO,                 /* bDataInterface:                      */

  /* ACM Functional Descriptor */
  0x04,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x02,                                   /* bDescriptorSubtype: Abstract Control
                                             Management desc                      */
  0x02,                                   /* bmCapabilities                       */

  /* Union Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x06,                                   /* bDescriptorSubtype: Union func desc  */
  CDC2_CTRL_INTERFACE_NO,                 /* bMasterInterface:
                                             Communication class interface        */
  CDC2_DATA_INTERFACE_NO,                 /* bSlaveInterface0:
                                             Data Class Interface                 */
  /* Endpoint 2 Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size    */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint            */
  CDC2_CMD_EP,                            /* bEndpointAddress                     */
  0x03,                                   /* bmAttributes: Interrupt              */
  LOBYTE(CDC_CMD_PACKET_SIZE),            /* wMaxPacketSize:                      */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0x10,                                   /* bInterval:                           */ 
  
  /*---------------------------------------------------------------------------*/
  /* Data class interface descriptor */
  0x09,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType:                       */
  CDC2_DATA_INTERFACE_NO,                 /* bInterfaceNumber: Number of Interface  */
  0x00,                                   /* bAlternateSetting: Alternate setting   */
  0x02,                                   /* bNumEndpoints: Two endpoints used      */
  0x0A,                                   /* bInterfaceClass: CDC                   */
  0x00,                                   /* bInterfaceSubClass:                    */
  0x00,                                   /* bInterfaceProtocol:                    */
  0x00,                                   /* iInterface:                            */

  /* Endpoint OUT Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC2_OUT_EP,                            /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*Endpoint IN Descriptor*/
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC2_IN_EP,                             /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    /* wMaxPacketSize:                        */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                   /* bInterval: ignore for Bulk transfer    */
#endif
} ;

__ALIGN_BEGIN uint8_t USBD_CDC_OtherSpeedCfgDesc[USB_CDC_COMP_CONFIG_DESC_SIZE] __ALIGN_END =
{ 
  0x09,   /* bLength: Configuation Descriptor size */
  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION,   
  USB_CDC_COMP_CONFIG_DESC_SIZE,
  0x00,
  0x04,   /* bNumInterfaces: 2 interfaces */
  0x01,   /* bConfigurationValue: */
  0x04,   /* iConfiguration: */
  0xC0,   /* bmAttributes: */
  0x32,   /* MaxPower 100 mA */  

  /*---------------------------------------------------------------------------*/
  /* CDC0 Interface Descriptor */
  /* IAD (Interface Association Descriptor) for CDC 0 */
  /*---------------------------------------------------------------------------*/
  USB_INTERFACE_ASSOCIATION_DESCSIZE,     /* bLength: Interface Descriptor size */
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR,   /* bDescriptorType                    */
  CDC0_CTRL_INTERFACE_NO,                 /* bFirstInterface                    */
  2,                                      /* bInterfaceCount                    */
  USB_CLASS_CDC,                          /* bFunctionClass                     */
  USB_CLASS_CDC_ACM,                      /* bFunctionSubClass                  */
  0,                                      /* bFunctionProtocol                  */
  0,                                      /* iFunction                          */

  /*---------------------------------------------------------------------------*/
  /* Communication Class Interface descriptor */
  0x09,                                   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType: Interface */
  CDC0_CTRL_INTERFACE_NO,                 /* bInterfaceNumber                   */
  0,                                      /* bAlternateSetting                  */
  1,                                      /* bNumEndpoints                      */
  USB_CLASS_CDC,                          /* bInterfaceClass                    */
  USB_CLASS_CDC_ACM,                      /* bInterfaceSubClass                 */
  0x0,                                    /* bInterfaceProtocol                 */
  0x0,                                    /* iInterface                         */
  
  /* Header Functional Descriptor */
  0x05,                                   /* bLength: Endpoint Descriptor size    */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x00,                                   /* bDescriptorSubtype: Header Func Desc */
  0x10,                                   /* bcdCDC: spec release number          */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x01,                                   /* bDescriptorSubtype: Call Management
                                             Func Desc                            */
  0x00,                                   /* bmCapabilities: D0+D1                */
  CDC0_DATA_INTERFACE_NO,                 /* bDataInterface:                      */

  /* ACM Functional Descriptor */
  0x04,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x02,                                   /* bDescriptorSubtype: Abstract Control
                                             Management desc                      */
  0x02,                                   /* bmCapabilities                       */

  /* Union Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x06,                                   /* bDescriptorSubtype: Union func desc  */
  CDC0_CTRL_INTERFACE_NO,                 /* bMasterInterface:
                                             Communication class interface        */
  CDC0_DATA_INTERFACE_NO,                 /* bSlaveInterface0:
                                             Data Class Interface                 */
  /* Endpoint 2 Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size    */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint            */
  CDC0_CMD_EP,                            /* bEndpointAddress                     */
  0x03,                                   /* bmAttributes: Interrupt              */
  LOBYTE(CDC_CMD_PACKET_SIZE),            /* wMaxPacketSize:                      */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0xff,                                   /* bInterval:                           */ 
  
  /*---------------------------------------------------------------------------*/
  /* Data class interface descriptor */
  0x09,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType:                       */
  CDC0_DATA_INTERFACE_NO,                 /* bInterfaceNumber: Number of Interface  */
  0x00,                                   /* bAlternateSetting: Alternate setting   */
  0x02,                                   /* bNumEndpoints: Two endpoints used      */
  0x0A,                                   /* bInterfaceClass: CDC                   */
  0x00,                                   /* bInterfaceSubClass:                    */
  0x00,                                   /* bInterfaceProtocol:                    */
  0x00,                                   /* iInterface:                            */

  /* Endpoint OUT Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC0_OUT_EP,                            /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  16,                                     /* wMaxPacketSize:                        */
  0x00,
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*Endpoint IN Descriptor*/
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC0_IN_EP,                             /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  16,                                     /* wMaxPacketSize:                        */
  0x00,
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*---------------------------------------------------------------------------*/
  /* CDC1 Interface Descriptor */
  /* IAD (Interface Association Descriptor) for CDC 1 */
  /*---------------------------------------------------------------------------*/
  USB_INTERFACE_ASSOCIATION_DESCSIZE,     /* bLength: Interface Descriptor size */
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR,   /* bDescriptorType                    */
  CDC1_CTRL_INTERFACE_NO,                 /* bFirstInterface                    */
  2,                                      /* bInterfaceCount                    */
  USB_CLASS_CDC,                          /* bFunctionClass                     */
  USB_CLASS_CDC_ACM,                      /* bFunctionSubClass                  */
  0,                                      /* bFunctionProtocol                  */
  0,                                      /* iFunction                          */

  /*---------------------------------------------------------------------------*/
  /* Communication Class Interface descriptor */
  0x09,                                   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType: Interface */
  CDC1_CTRL_INTERFACE_NO,                 /* bInterfaceNumber                   */
  0,                                      /* bAlternateSetting                  */
  1,                                      /* bNumEndpoints                      */
  USB_CLASS_CDC,                          /* bInterfaceClass                    */
  USB_CLASS_CDC_ACM,                      /* bInterfaceSubClass                 */
  0x0,                                    /* bInterfaceProtocol                 */
  0x0,                                    /* iInterface                         */
  
  /* Header Functional Descriptor */
  0x05,                                   /* bLength: Endpoint Descriptor size    */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x00,                                   /* bDescriptorSubtype: Header Func Desc */
  0x10,                                   /* bcdCDC: spec release number          */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x01,                                   /* bDescriptorSubtype: Call Management
                                             Func Desc                            */
  0x00,                                   /* bmCapabilities: D0+D1                */
  CDC1_DATA_INTERFACE_NO,                 /* bDataInterface:                      */

  /* ACM Functional Descriptor */
  0x04,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x02,                                   /* bDescriptorSubtype: Abstract Control
                                             Management desc                      */
  0x02,                                   /* bmCapabilities                       */

  /* Union Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x06,                                   /* bDescriptorSubtype: Union func desc  */
  CDC1_CTRL_INTERFACE_NO,                 /* bMasterInterface:
                                             Communication class interface        */
  CDC1_DATA_INTERFACE_NO,                 /* bSlaveInterface0:
                                             Data Class Interface                 */
  /* Endpoint 2 Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size    */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint            */
  CDC1_CMD_EP,                            /* bEndpointAddress                     */
  0x03,                                   /* bmAttributes: Interrupt              */
  LOBYTE(CDC_CMD_PACKET_SIZE),            /* wMaxPacketSize:                      */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0xff,                                   /* bInterval:                           */ 
  
  /*---------------------------------------------------------------------------*/
  /* Data class interface descriptor */
  0x09,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType:                       */
  CDC1_DATA_INTERFACE_NO,                 /* bInterfaceNumber: Number of Interface  */
  0x00,                                   /* bAlternateSetting: Alternate setting   */
  0x02,                                   /* bNumEndpoints: Two endpoints used      */
  0x0A,                                   /* bInterfaceClass: CDC                   */
  0x00,                                   /* bInterfaceSubClass:                    */
  0x00,                                   /* bInterfaceProtocol:                    */
  0x00,                                   /* iInterface:                            */

  /* Endpoint OUT Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC1_OUT_EP,                            /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  16,                                     /* wMaxPacketSize:                        */
  0x00,
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*Endpoint IN Descriptor*/
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC1_IN_EP,                             /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  16,                                     /* wMaxPacketSize:                        */
  0x00,
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

#if 0
  /*---------------------------------------------------------------------------*/
  /* CDC2 Interface Descriptor */
  /* IAD (Interface Association Descriptor) for CDC 2 */
  /*---------------------------------------------------------------------------*/
  USB_INTERFACE_ASSOCIATION_DESCSIZE,     /* bLength: Interface Descriptor size */
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR,   /* bDescriptorType                    */
  CDC2_CTRL_INTERFACE_NO,                 /* bFirstInterface                    */
  2,                                      /* bInterfaceCount                    */
  USB_CLASS_CDC,                          /* bFunctionClass                     */
  USB_CLASS_CDC_ACM,                      /* bFunctionSubClass                  */
  0,                                      /* bFunctionProtocol                  */
  0,                                      /* iFunction                          */

  /*---------------------------------------------------------------------------*/
  /* Communication Class Interface descriptor */
  0x09,                                   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType: Interface */
  CDC2_CTRL_INTERFACE_NO,                 /* bInterfaceNumber                   */
  0,                                      /* bAlternateSetting                  */
  1,                                      /* bNumEndpoints                      */
  USB_CLASS_CDC,                          /* bInterfaceClass                    */
  USB_CLASS_CDC_ACM,                      /* bInterfaceSubClass                 */
  0x0,                                    /* bInterfaceProtocol                 */
  0x0,                                    /* iInterface                         */
  
  /* Header Functional Descriptor */
  0x05,                                   /* bLength: Endpoint Descriptor size    */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x00,                                   /* bDescriptorSubtype: Header Func Desc */
  0x10,                                   /* bcdCDC: spec release number          */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x01,                                   /* bDescriptorSubtype: Call Management
                                             Func Desc                            */
  0x00,                                   /* bmCapabilities: D0+D1                */
  CDC2_DATA_INTERFACE_NO,                 /* bDataInterface:                      */

  /* ACM Functional Descriptor */
  0x04,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x02,                                   /* bDescriptorSubtype: Abstract Control
                                             Management desc                      */
  0x02,                                   /* bmCapabilities                       */

  /* Union Functional Descriptor */
  0x05,                                   /* bFunctionLength                      */
  0x24,                                   /* bDescriptorType: CS_INTERFACE        */
  0x06,                                   /* bDescriptorSubtype: Union func desc  */
  CDC2_CTRL_INTERFACE_NO,                 /* bMasterInterface:
                                             Communication class interface        */
  CDC2_DATA_INTERFACE_NO,                 /* bSlaveInterface0:
                                             Data Class Interface                 */
  /* Endpoint 2 Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size    */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint            */
  CDC2_CMD_EP,                            /* bEndpointAddress                     */
  0x03,                                   /* bmAttributes: Interrupt              */
  LOBYTE(CDC_CMD_PACKET_SIZE),            /* wMaxPacketSize:                      */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0xff,                                   /* bInterval:                           */ 
  
  /*---------------------------------------------------------------------------*/
  /* Data class interface descriptor */
  0x09,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_INTERFACE,                /* bDescriptorType:                       */
  CDC2_DATA_INTERFACE_NO,                 /* bInterfaceNumber: Number of Interface  */
  0x00,                                   /* bAlternateSetting: Alternate setting   */
  0x02,                                   /* bNumEndpoints: Two endpoints used      */
  0x0A,                                   /* bInterfaceClass: CDC                   */
  0x00,                                   /* bInterfaceSubClass:                    */
  0x00,                                   /* bInterfaceProtocol:                    */
  0x00,                                   /* iInterface:                            */

  /* Endpoint OUT Descriptor */
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC2_OUT_EP,                            /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  16,                                     /* wMaxPacketSize:                        */
  0x00,
  0x00,                                   /* bInterval: ignore for Bulk transfer    */

  /*Endpoint IN Descriptor*/
  0x07,                                   /* bLength: Endpoint Descriptor size      */
  USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint              */
  CDC2_IN_EP,                             /* bEndpointAddress                       */
  0x02,                                   /* bmAttributes: Bulk                     */
  16,                                     /* wMaxPacketSize:                        */
  0x00,
  0x00,                                   /* bInterval: ignore for Bulk transfer    */
#endif
};

/**
  * @}
  */ 

/** @defgroup USBD_CDC_Private_Functions
  * @{
  */ 

/**
  * @brief  USBD_CDC_Init
  *         Initialize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t
USBD_CDC_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0;
  USBD_CDC_HandleTypeDef   *hcdc;

  if(pdev->dev_speed == USBD_SPEED_HIGH  ) 
  {  
    /* Open EP IN */
    USBD_LL_OpenEP(pdev, CDC0_IN_EP, USBD_EP_TYPE_BULK, CDC_DATA_HS_IN_PACKET_SIZE);

    /* Open EP OUT */
    USBD_LL_OpenEP(pdev, CDC0_OUT_EP, USBD_EP_TYPE_BULK, CDC_DATA_HS_OUT_PACKET_SIZE);

    /* Open EP IN */
    USBD_LL_OpenEP(pdev, CDC1_IN_EP, USBD_EP_TYPE_BULK, CDC_DATA_HS_IN_PACKET_SIZE);

    /* Open EP OUT */
    USBD_LL_OpenEP(pdev, CDC1_OUT_EP, USBD_EP_TYPE_BULK, CDC_DATA_HS_OUT_PACKET_SIZE);
  }
  else
  {
    /* Open EP IN */
    USBD_LL_OpenEP(pdev, CDC0_IN_EP, USBD_EP_TYPE_BULK, CDC_DATA_FS_IN_PACKET_SIZE);

    /* Open EP OUT */
    USBD_LL_OpenEP(pdev, CDC0_OUT_EP, USBD_EP_TYPE_BULK, CDC_DATA_FS_OUT_PACKET_SIZE);

    /* Open EP IN */
    USBD_LL_OpenEP(pdev, CDC1_IN_EP, USBD_EP_TYPE_BULK, CDC_DATA_FS_IN_PACKET_SIZE);

    /* Open EP OUT */
    USBD_LL_OpenEP(pdev, CDC1_OUT_EP, USBD_EP_TYPE_BULK, CDC_DATA_FS_OUT_PACKET_SIZE);
  }

  /* Open Command IN EP */
  USBD_LL_OpenEP(pdev, CDC0_CMD_EP, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);
  USBD_LL_OpenEP(pdev, CDC1_CMD_EP, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);

  pdev->pClassData = USBD_malloc(sizeof (USBD_CDC_HandleTypeDef));

  if(pdev->pClassData == NULL)
  {
    ret = 1; 
  }
  else
  {
    hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;

    /* Init  physical Interface components */
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Init();

    /* Init Xfer states */
    hcdc->TxState[0] =0;
    hcdc->TxState[1] =0;

    hcdc->RxState[0] =0;
    hcdc->RxState[1] =0;

    if(pdev->dev_speed == USBD_SPEED_HIGH  ) 
    {      
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev, CDC0_OUT_EP, hcdc->RxBuffer[0], CDC_DATA_HS_OUT_PACKET_SIZE);
      USBD_LL_PrepareReceive(pdev, CDC1_OUT_EP, hcdc->RxBuffer[1], CDC_DATA_HS_OUT_PACKET_SIZE);
    }
    else
    {
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev, CDC0_OUT_EP, hcdc->RxBuffer[0], CDC_DATA_FS_OUT_PACKET_SIZE);
      USBD_LL_PrepareReceive(pdev, CDC1_OUT_EP, hcdc->RxBuffer[1], CDC_DATA_FS_OUT_PACKET_SIZE);
    }
  }
  return ret;
}

/**
  * @brief  USBD_CDC_DeInit
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t
USBD_CDC_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0;

  /* Open EP IN */
  USBD_LL_CloseEP(pdev, CDC0_IN_EP);
  USBD_LL_CloseEP(pdev, CDC1_IN_EP);

  /* Open EP OUT */
  USBD_LL_CloseEP(pdev, CDC0_OUT_EP);
  USBD_LL_CloseEP(pdev, CDC1_OUT_EP);

  /* Open Command IN EP */
  USBD_LL_CloseEP(pdev, CDC0_CMD_EP);
  USBD_LL_CloseEP(pdev, CDC1_CMD_EP);

  /* DeInit  physical Interface components */
  if(pdev->pClassData != NULL)
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->DeInit();
    USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return ret;
}

/**
  * @brief  USBD_CDC_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t
USBD_CDC_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
  static uint8_t ifalt = 0;
  USBD_CDC_Instance   instance;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    instance = get_cdc_instance_from_interface(req->wIndex);
    if(instance == USBD_CDC_Instance_MAX)
    {
      while(1)
      {
        // debug point
      }
    }

    if (req->wLength)
    {
      if (req->bmRequest & 0x80)
      {
        ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(req->bRequest, (uint8_t *)hcdc->data, req->wLength, instance);
        USBD_CtlSendData (pdev, (uint8_t *)hcdc->data, req->wLength);
      }
      else
      {
        hcdc->CmdOpCode = req->bRequest;
        hcdc->CmdLength = req->wLength;
        hcdc->wIndex    = req->wIndex;

        USBD_CtlPrepareRx (pdev, (uint8_t *)hcdc->data, req->wLength);
      }
    }
    else
    {
      ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(req->bRequest, (uint8_t*)req, 0, instance);
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev, &ifalt, 1);
      break;

    case USB_REQ_SET_INTERFACE :
      break;
    }

  default: 
    break;
  }
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t
USBD_CDC_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;

  if(pdev->pClassData != NULL)
  {
    switch((epnum | 0x80))
    {
    case CDC0_IN_EP:
      hcdc->TxState[0] = 0;
      break;

    case CDC1_IN_EP:
      hcdc->TxState[1] = 0;
      break;
    }
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}

/**
  * @brief  USBD_CDC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t
USBD_CDC_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{      
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;

  /* USB data will be immediately processed, this allow next USB traffic being 
     NAKed till the end of the application Xfer */
  if(pdev->pClassData != NULL)
  {
    switch(epnum)
    {
    case CDC0_OUT_EP:
      hcdc->RxLength[0] = USBD_LL_GetRxDataSize (pdev, epnum);
      ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Receive(hcdc->RxBuffer[0], &hcdc->RxLength[0], USBD_CDC_Instance_0);
      break;

    case CDC1_OUT_EP:
      hcdc->RxLength[1] = USBD_LL_GetRxDataSize (pdev, epnum);
      ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Receive(hcdc->RxBuffer[1], &hcdc->RxLength[1], USBD_CDC_Instance_1);

      break;
    }

    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}



/**
  * @brief  USBD_CDC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t
USBD_CDC_EP0_RxReady (USBD_HandleTypeDef *pdev)
{ 
  USBD_CDC_HandleTypeDef  *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;

  if((pdev->pUserData != NULL) && (hcdc->CmdOpCode != 0xFF))
  {
    USBD_CDC_Instance       instance;

    instance = get_cdc_instance_from_interface(hcdc->wIndex);
    if(instance == USBD_CDC_Instance_MAX)
    {
      return USBD_OK;
    }
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(hcdc->CmdOpCode, (uint8_t *)hcdc->data, hcdc->CmdLength, instance);
    hcdc->CmdOpCode = 0xFF; 
  }
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_GetFSCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t
*USBD_CDC_GetFSCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_CDC_CfgFSDesc);
  return USBD_CDC_CfgFSDesc;
}

/**
  * @brief  USBD_CDC_GetHSCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t
*USBD_CDC_GetHSCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_CDC_CfgHSDesc);
  return USBD_CDC_CfgHSDesc;
}

/**
  * @brief  USBD_CDC_GetCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t
*USBD_CDC_GetOtherSpeedCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_CDC_OtherSpeedCfgDesc);
  return USBD_CDC_OtherSpeedCfgDesc;
}

/**
* @brief  DeviceQualifierDescriptor 
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t
*USBD_CDC_GetDeviceQualifierDescriptor (uint16_t *length)
{
  *length = sizeof (USBD_CDC_DeviceQualifierDesc);
  return USBD_CDC_DeviceQualifierDesc;
}

/**
* @brief  USBD_CDC_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
uint8_t
USBD_CDC_RegisterInterface  (USBD_HandleTypeDef   *pdev, USBD_CDC_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;

  if(fops != NULL)
  {
    pdev->pUserData= fops;
    ret = USBD_OK;    
  }

  return ret;
}

/**
  * @brief  USBD_CDC_SetTxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Tx Buffer
  * @retval status
  */
uint8_t
USBD_CDC_SetTxBuffer(USBD_HandleTypeDef   *pdev, uint8_t  *pbuff, uint16_t length,
    USBD_CDC_Instance instance)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;

  hcdc->TxBuffer[instance] = pbuff;
  hcdc->TxLength[instance] = length;  

  return USBD_OK;  
}


/**
  * @brief  USBD_CDC_SetRxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Rx Buffer
  * @retval status
  */
uint8_t
USBD_CDC_SetRxBuffer (USBD_HandleTypeDef   *pdev, uint8_t  *pbuff,
    USBD_CDC_Instance instance)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;

  hcdc->RxBuffer[instance] = pbuff;

  return USBD_OK;
}

/**
  * @brief  USBD_CDC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
uint8_t
USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev, USBD_CDC_Instance instance)
{      
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;

  if(pdev->pClassData != NULL)
  {
    if(hcdc->TxState[instance] == 0)
    {
      /* Tx Transfer in progress */
      hcdc->TxState[instance] = 1;

      /* Transmit next packet */
      USBD_LL_Transmit(pdev, _cdc_in_eps[instance], hcdc->TxBuffer[instance],
          hcdc->TxLength[instance]);

      return USBD_OK;
    }
    else
    {
      return USBD_BUSY;
    }
  }
  else
  {
    return USBD_FAIL;
  }
}


/**
  * @brief  USBD_CDC_ReceivePacket
  *         prepare OUT Endpoint for reception
  * @param  pdev: device instance
  * @retval status
  */
uint8_t
USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev, USBD_CDC_Instance instance)
{      
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;

  /* Suspend or Resume USB Out process */
  if(pdev->pClassData != NULL)
  {
    if(pdev->dev_speed == USBD_SPEED_HIGH  ) 
    {      
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev, _cdc_out_eps[instance],
          hcdc->RxBuffer[instance], CDC_DATA_HS_OUT_PACKET_SIZE);
    }
    else
    {
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev, _cdc_out_eps[instance],
          hcdc->RxBuffer[instance], CDC_DATA_FS_OUT_PACKET_SIZE);
    }
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}
