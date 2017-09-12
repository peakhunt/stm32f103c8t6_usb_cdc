#ifndef __USB_CDC_H
#define __USB_CDC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"

#define CDC0_IN_EP                                  0x81  /* EP1 for data IN for CDC0       */
#define CDC0_OUT_EP                                 0x01  /* EP1 for data OUT  for CDC0     */
#define CDC0_CMD_EP                                 0x82  /* EP2 for CDC commands for CDC0  */

#define CDC1_IN_EP                                  0x83  /* EP1 for data IN for CDC1       */
#define CDC1_OUT_EP                                 0x03  /* EP1 for data OUT for CDC1      */
#define CDC1_CMD_EP                                 0x84  /* EP2 for CDC commands for CDC1  */

#if 0
#define CDC2_IN_EP                                  0x85  /* EP1 for data IN for CDC2       */
#define CDC2_OUT_EP                                 0x05  /* EP1 for data OUT for CDC2      */
#define CDC2_CMD_EP                                 0x86  /* EP2 for CDC commands for CDC2  */
#endif

#define CDC0_CTRL_INTERFACE_NO                      0
#define CDC0_DATA_INTERFACE_NO                      1

#define CDC1_CTRL_INTERFACE_NO                      2
#define CDC1_DATA_INTERFACE_NO                      3

#if 0
#define CDC2_CTRL_INTERFACE_NO                      4
#define CDC2_DATA_INTERFACE_NO                      5
#endif

/* CDC Endpoints parameters: you can fine tune these values depending on the needed baudrates and performance. */
#define CDC_DATA_HS_MAX_PACKET_SIZE                 512   /* Endpoint IN & OUT Packet size */
#define CDC_DATA_FS_MAX_PACKET_SIZE                 64    /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SIZE                         8     /* Control Endpoint Packet size */ 

#define USB_CDC_CONFIG_DESC_SIZ                     67
//#define USB_CDC_COMP_CONFIG_DESC_SIZE               207
#define USB_CDC_COMP_CONFIG_DESC_SIZE               141
#define CDC_DATA_HS_IN_PACKET_SIZE                  CDC_DATA_HS_MAX_PACKET_SIZE
#define CDC_DATA_HS_OUT_PACKET_SIZE                 CDC_DATA_HS_MAX_PACKET_SIZE

#define CDC_DATA_FS_IN_PACKET_SIZE                  CDC_DATA_FS_MAX_PACKET_SIZE
#define CDC_DATA_FS_OUT_PACKET_SIZE                 CDC_DATA_FS_MAX_PACKET_SIZE

/*---------------------------------------------------------------------*/
/*  CDC definitions                                                    */
/*---------------------------------------------------------------------*/
#define CDC_SEND_ENCAPSULATED_COMMAND               0x00
#define CDC_GET_ENCAPSULATED_RESPONSE               0x01
#define CDC_SET_COMM_FEATURE                        0x02
#define CDC_GET_COMM_FEATURE                        0x03
#define CDC_CLEAR_COMM_FEATURE                      0x04
#define CDC_SET_LINE_CODING                         0x20
#define CDC_GET_LINE_CODING                         0x21
#define CDC_SET_CONTROL_LINE_STATE                  0x22
#define CDC_SEND_BREAK                              0x23

#define USB_INTERFACE_ASSOCIATION_DESCSIZE          8
#define USB_INTERFACE_ASSOCIATION_DESCRIPTOR        11
#define USB_CLASS_CDC                               2
#define USB_CLASS_CDC_ACM                           2

typedef enum
{
  USBD_CDC_Instance_0,
  USBD_CDC_Instance_1,
  //USBD_CDC_Instance_2,
  USBD_CDC_Instance_MAX,
} USBD_CDC_Instance;

typedef struct
{
  uint32_t bitrate;
  uint8_t  format;
  uint8_t  paritytype;
  uint8_t  datatype;
}USBD_CDC_LineCodingTypeDef;

typedef struct _USBD_CDC_Itf
{
  int8_t (* Init)          (void);
  int8_t (* DeInit)        (void);
  int8_t (* Control)       (uint8_t, uint8_t * , uint16_t, USBD_CDC_Instance instance);   
  int8_t (* Receive)       (uint8_t *, uint32_t *, USBD_CDC_Instance instance);  

}USBD_CDC_ItfTypeDef;


typedef struct
{
  uint32_t data[CDC_DATA_HS_MAX_PACKET_SIZE/4];      /* Force 32bits alignment */
  uint8_t  CmdOpCode;
  uint8_t  CmdLength;    
  uint8_t  wIndex;                                   /* XXX hkim. to retrieve interface */
  uint8_t  *RxBuffer[USBD_CDC_Instance_MAX];  
  uint8_t  *TxBuffer[USBD_CDC_Instance_MAX];   
  uint32_t RxLength[USBD_CDC_Instance_MAX];
  uint32_t TxLength[USBD_CDC_Instance_MAX];   
  
  __IO uint32_t TxState[USBD_CDC_Instance_MAX];     
  __IO uint32_t RxState[USBD_CDC_Instance_MAX];    
}
USBD_CDC_HandleTypeDef; 

extern USBD_ClassTypeDef  USBD_CDC;
#define USBD_CDC_CLASS    &USBD_CDC

uint8_t  USBD_CDC_RegisterInterface  (USBD_HandleTypeDef   *pdev, USBD_CDC_ItfTypeDef *fops);
uint8_t  USBD_CDC_SetTxBuffer        (USBD_HandleTypeDef   *pdev, uint8_t  *pbuff, uint16_t length, USBD_CDC_Instance instance);
uint8_t  USBD_CDC_SetRxBuffer        (USBD_HandleTypeDef   *pdev, uint8_t  *pbuff, USBD_CDC_Instance instance); 
uint8_t  USBD_CDC_ReceivePacket      (USBD_HandleTypeDef *pdev, USBD_CDC_Instance instance);
uint8_t  USBD_CDC_TransmitPacket     (USBD_HandleTypeDef *pdev, USBD_CDC_Instance instance);

#ifdef __cplusplus
}
#endif

#endif  /* __USB_CDC_H */
