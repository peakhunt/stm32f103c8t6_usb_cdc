#include "usbd_cdc_if.h"
#include "usart.h"
#include "tim.h"
#include "gpio.h"

#define APP_RX_DATA_SIZE  32
#define APP_TX_DATA_SIZE  32

static void ComPort_Config(USBD_CDC_Instance instance);

uint8_t UserRxBufferFS[USBD_CDC_Instance_MAX][APP_RX_DATA_SIZE];
uint8_t UserTxBufferFS[USBD_CDC_Instance_MAX][APP_TX_DATA_SIZE];

USBD_CDC_LineCodingTypeDef LineCoding[USBD_CDC_Instance_MAX] =
{
  {
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
  },
  {
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
  },
};

uint32_t UserTxBufPtrIn[USBD_CDC_Instance_MAX] = { 0, 0,};
uint32_t UserTxBufPtrOut[USBD_CDC_Instance_MAX] = { 0, 0};

static volatile uint8_t   _usb_connected = 0;

extern USBD_HandleTypeDef hUsbDeviceFS;

static int8_t CDC_Init_FS     (void);
static int8_t CDC_DeInit_FS   (void);
static int8_t CDC_Control_FS  (uint8_t cmd, uint8_t* pbuf, uint16_t length, USBD_CDC_Instance instance);
static int8_t CDC_Receive_FS  (uint8_t* pbuf, uint32_t *Len, USBD_CDC_Instance instance);

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = 
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,  
  CDC_Receive_FS
};

static inline UART_HandleTypeDef*
get_uart_handle(USBD_CDC_Instance instance)
{
  switch(instance)
  {
  case USBD_CDC_Instance_0:
    return &huart1;

  case USBD_CDC_Instance_1:
    return &huart2;

  default:
    break;
  }
  return NULL;
}

static inline USBD_CDC_Instance
get_uart_instance(UART_HandleTypeDef* huart)
{
  if(huart == &huart1)
  {
    return USBD_CDC_Instance_0;
  }
  return USBD_CDC_Instance_1;
}

/**
  * @brief  CDC_Init_FS
  *         Initializes the CDC media low layer over the FS USB IP
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t
CDC_Init_FS(void)
{ 
  UserTxBufPtrIn[0] =
  UserTxBufPtrIn[1] = 0;

  UserTxBufPtrOut[0] =
  UserTxBufPtrOut[1] = 0;

  ComPort_Config(USBD_CDC_Instance_0);
  ComPort_Config(USBD_CDC_Instance_1);

  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, &UserTxBufferFS[0][0], 0, USBD_CDC_Instance_0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &UserRxBufferFS[0][0], USBD_CDC_Instance_0);

  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, &UserTxBufferFS[1][0], 0, USBD_CDC_Instance_1);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &UserRxBufferFS[1][0], USBD_CDC_Instance_1);

  if(HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  _usb_connected = 1;
  return (USBD_OK);
}

/**
  * @brief  CDC_DeInit_FS
  *         DeInitializes the CDC media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t
CDC_DeInit_FS(void)
{
  if(HAL_TIM_Base_Stop_IT(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  if(HAL_UART_DeInit(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

  if(HAL_UART_DeInit(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

  _usb_connected = 0;

  return (USBD_OK);
}

static void
ComPort_Config(USBD_CDC_Instance instance)
{
  UART_HandleTypeDef* handle;

  handle = get_uart_handle(instance);

  if(HAL_UART_DeInit(handle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* set the Stop bit */
  switch (LineCoding[instance].format)
  {
    case 0:
      handle->Init.StopBits = UART_STOPBITS_1;
      break;
    case 2:
      handle->Init.StopBits = UART_STOPBITS_2;
      break;
    default :
      handle->Init.StopBits = UART_STOPBITS_1;
      break;
  }

  /* set the parity bit*/
  switch (LineCoding[instance].paritytype)
  {
    case 0:
      handle->Init.Parity = UART_PARITY_NONE;
      break;
    case 1:
      handle->Init.Parity = UART_PARITY_ODD;
      break;
    case 2:
      handle->Init.Parity = UART_PARITY_EVEN;
      break;
    default :
      handle->Init.Parity = UART_PARITY_NONE;
      break;
  }

  /*set the data type : only 8bits and 9bits is supported */
  switch (LineCoding[instance].datatype)
  {
    case 0x07:
      /* With this configuration a parity (Even or
       * Odd) must be set */
      handle->Init.WordLength = UART_WORDLENGTH_8B;
      break;
    case 0x08:
      if(handle->Init.Parity == UART_PARITY_NONE)
      {
        handle->Init.WordLength = UART_WORDLENGTH_8B;
      }
      else
      {
        handle->Init.WordLength = UART_WORDLENGTH_9B;
      }

      break;
    default :
      handle->Init.WordLength = UART_WORDLENGTH_8B;
      break;
  }

  handle->Init.BaudRate = LineCoding[instance].bitrate;
  handle->Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  handle->Init.Mode       = UART_MODE_TX_RX;

  if(HAL_UART_Init(handle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Start reception: provide the buffer pointer with
   * offset and the buffer size */
  HAL_UART_Receive_IT(handle, (uint8_t *)(&UserTxBufferFS[instance][UserTxBufPtrIn[instance]]), 1);
}
/**
  * @brief  CDC_Control_FS
  *         Manage the CDC class requests
  * @param  cmd: Command code            
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t
CDC_Control_FS  (uint8_t cmd, uint8_t* pbuf, uint16_t length, USBD_CDC_Instance instance)
{ 
  /* USER CODE BEGIN 5 */
  switch (cmd)
  {
  case CDC_SEND_ENCAPSULATED_COMMAND:
 
    break;

  case CDC_GET_ENCAPSULATED_RESPONSE:
 
    break;

  case CDC_SET_COMM_FEATURE:
 
    break;

  case CDC_GET_COMM_FEATURE:

    break;

  case CDC_CLEAR_COMM_FEATURE:

    break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */ 
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
  case CDC_SET_LINE_CODING:   
    LineCoding[instance].bitrate    = (uint32_t)(pbuf[0] | (pbuf[1] << 8) |
                                                (pbuf[2] << 16) | (pbuf[3] << 24));
    LineCoding[instance].format     = pbuf[4];
    LineCoding[instance].paritytype = pbuf[5];
    LineCoding[instance].datatype   = pbuf[6];

    /* Set the new configuration */
    ComPort_Config(instance);
    break;

  case CDC_GET_LINE_CODING:     
    pbuf[0] = (uint8_t)(LineCoding[instance].bitrate);
    pbuf[1] = (uint8_t)(LineCoding[instance].bitrate >> 8);
    pbuf[2] = (uint8_t)(LineCoding[instance].bitrate >> 16);
    pbuf[3] = (uint8_t)(LineCoding[instance].bitrate >> 24);
    pbuf[4] = LineCoding[instance].format;
    pbuf[5] = LineCoding[instance].paritytype;
    pbuf[6] = LineCoding[instance].datatype;
    break;

  case CDC_SET_CONTROL_LINE_STATE:

    break;

  case CDC_SEND_BREAK:
 
    break;    
    
  default:
    break;
  }

  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  CDC_Receive_FS
  *         Data received over USB OUT endpoint are sent over CDC interface 
  *         through this function.
  *           
  *         @note
  *         This function will block any OUT packet reception on USB endpoint 
  *         untill exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (ie. using DMA controller) it will result 
  *         in receiving more data while previous ones are still not sent.
  *                 
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t
CDC_Receive_FS (uint8_t* Buf, uint32_t *Len, USBD_CDC_Instance instance)
{
  UART_HandleTypeDef* huart = get_uart_handle(instance);

  HAL_UART_Transmit_DMA(huart, Buf, *Len);
  return (USBD_OK);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  USBD_CDC_Instance instance = get_uart_instance(huart);

  /* Initiate next USB packet transfer once UART completes transfer
   * (transmitting data over Tx line) */
  USBD_CDC_ReceivePacket(&hUsbDeviceFS, instance);
}

void
HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  USBD_CDC_Instance instance = get_uart_instance(huart);

  /* Increment Index for buffer writing */
  UserTxBufPtrIn[instance]++;

  /* To avoid buffer overflow */
  if(UserTxBufPtrIn[instance] == APP_RX_DATA_SIZE)
  {
    UserTxBufPtrIn[instance] = 0;
  }

  /* Start another reception: provide the buffer pointer with
   * offset and the buffer size */
  HAL_UART_Receive_IT(huart, (uint8_t *)(&UserTxBufferFS[instance][UserTxBufPtrIn[instance]]), 1);
}

void
HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
  /* Transfer error occured in reception and/or transmission process
   * */
  Error_Handler();
}


static void
check_tx_buffer(USBD_CDC_Instance instance)
{
  uint32_t buffptr;
  uint32_t buffsize;

  if(UserTxBufPtrOut[instance] != UserTxBufPtrIn[instance])
  {
    if(UserTxBufPtrOut[instance] > UserTxBufPtrIn[instance]) /* Rollback */
    {
      buffsize = APP_RX_DATA_SIZE - UserTxBufPtrOut[instance];
    }
    else
    {
      buffsize = UserTxBufPtrIn[instance] - UserTxBufPtrOut[instance];
    }

    buffptr = UserTxBufPtrOut[instance];

    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, (uint8_t*)&UserTxBufferFS[instance][buffptr], buffsize, instance);

    if(USBD_CDC_TransmitPacket(&hUsbDeviceFS, instance) == USBD_OK)
    {
      UserTxBufPtrOut[instance] += buffsize;
      if (UserTxBufPtrOut[instance] == APP_RX_DATA_SIZE)
      {
        UserTxBufPtrOut[instance] = 0;
      }
    }
  }
}

void
HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  check_tx_buffer(USBD_CDC_Instance_0);
  check_tx_buffer(USBD_CDC_Instance_1);
}
