/* cdc.c: Minimal client-side USB CDC driver.                      */
/* This file contains interrupt-driven USB routines and callbacks. */

#include <stm32/gpio.h>
#include <stm32/usb/lib.h>
#include "config.h"

/******************************************************
 * Macros and declarations                            *
 ******************************************************/

#define SET_COMM_FEATURE       0x02
#define SET_LINE_CODING        0x20
#define GET_LINE_CODING        0x21
#define SET_CONTROL_LINE_STATE 0x22

#define EMPTY_CALLBACKS           { \
  NOP_Process, \
  NOP_Process, \
  NOP_Process, \
  NOP_Process, \
  NOP_Process, \
  NOP_Process, \
  NOP_Process, \
}

typedef struct {
  uint32_t bitrate;
  uint8_t format;
  uint8_t paritytype;
  uint8_t datatype;
} LINE_CODING;

void     Device_Init(void);
void     Device_Reset(void);
void     SetConfiguration(void);
RESULT   Data_Setup(uint8_t RequestNo);
RESULT   NoData_Setup(uint8_t RequestNo);
uint8_t *GetDeviceDescriptor(uint16_t Length);
uint8_t *GetConfigDescriptor(uint16_t Length);
uint8_t *GetStringDescriptor(uint16_t Length);
RESULT   Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t *GetLineCoding(uint16_t Length);
uint8_t *SetLineCoding(uint16_t Length);

/******************************************************
 * Descriptors                                        *
 ******************************************************/

/* USB Standard Device Descriptor */
const uint8_t DeviceDescriptor[] = {
  0x12,          /* bLength                 */
  0x01,          /* bDescriptorType: Device */
  0x00, 0x02,    /* bcdUSB         : 2.00   */
  0x02,          /* bDeviceClass   : CDC    */
  0x00,          /* bDeviceSubClass         */
  0x00,          /* bDeviceProtocol         */
  VCP_DATA_SIZE, /* bMaxPacketSize0         */
  0x83, 0x04,    /* idVendor       : 0x0483 */
  0x40, 0x57,    /* idProduct      : 0x7540 */
  0x00, 0x02,    /* bcdDevice      : 2.00   */
  0x01,          /* iManufacturer           */
  0x02,          /* iProduct                */
  0x03,          /* iSerial                 */
  0x01           /* bNumConfigurations      */
};

/* Configuration & Interface Descriptor */
const uint8_t ConfigDescriptor[] = {
  /* Configuration Descriptor */
  0x09,       /* bLength                        */
  0x02,       /* bDescriptorType: Configuration */
  0x43, 0x00, /* wTotalLength                   */
  0x02,       /* bNumInterfaces                 */
  0x01,       /* bConfigurationValue            */
  0x00,       /* iConfiguration                 */
  0xc0,       /* bmAttributes   : Self-powered  */
  0x32,       /* MaxPower       : 100 mA        */

    /* Interface Descriptor */
    0x09, /* bLength                         */
    0x04, /* bDescriptorType   : Interface   */
    0x00, /* bInterfaceNumber                */
    0x00, /* bAlternateSetting               */
    0x01, /* bNumEndpoints                   */
    0x02, /* bInterfaceClass   : CIC         */
    0x02, /* bInterfaceSubClass: ACM         */
    0x01, /* bInterfaceProtocol: AT-commands */
    0x00, /* iInterface                      */

      /* Header Functional Descriptor */
      0x05,       /* bLength                                        */
      0x24,       /* bDescriptorType   : CS_INTERFACE               */
      0x00,       /* bDescriptorSubtype: Header Func Desc           */
      0x10, 0x01, /* bcdCDC            : 1.10                       */

      /* Call Management Functional Descriptor */
      0x05,       /* bFunctionLength                                */
      0x24,       /* bDescriptorType   : CS_INTERFACE               */
      0x01,       /* bDescriptorSubtype: Call Management Func Desc  */
      0x00,       /* bmCapabilities    : D0 + D1                    */
      0x01,       /* bDataInterface                                 */

      /* ACM Functional Descriptor */
      0x04,       /* bFunctionLength                                */
      0x24,       /* bDescriptorType   : CS_INTERFACE               */
      0x02,       /* bDescriptorSubtype: ACM Desc                   */
      0x02,       /* bmCapabilities    : Line coding + Serial state */

      /* Union Functional Descriptor */
      0x05,       /* bFunctionLength                                */
      0x24,       /* bDescriptorType   : CS_INTERFACE               */
      0x06,       /* bDescriptorSubtype: Union Func Desc            */
      0x00,       /* bMasterInterface                               */
      0x01,       /* bSlaveInterface0                               */

      /* Endpoint 2 Descriptor */
      0x07,       /* bLength                                        */
      0x05,       /* bDescriptorType   : Endpoint                   */
      0x82,       /* bEndpointAddress  : EP 2 IN                    */
      0x03,       /* bmAttributes      : Interrupt                  */
      0x08, 0x00, /* wMaxPacketSize                                 */
      0xff,       /* bInterval                                      */

    /* Data class interface descriptor */
    0x09, /* bLength                    */
    0x04, /* bDescriptorType: Interface */
    0x01, /* bInterfaceNumber           */
    0x00, /* bAlternateSetting          */
    0x02, /* bNumEndpoints              */
    0x0a, /* bInterfaceClass: CDC Data  */
    0x00, /* bInterfaceSubClass         */
    0x00, /* bInterfaceProtocol         */
    0x02, /* iInterface                 */

      /* Endpoint 3 Descriptor */
      0x07,                /* bLength                    */
      0x05,                /* bDescriptorType : Endpoint */
      0x03,                /* bEndpointAddress: EP 3 OUT */
      0x02,                /* bmAttributes    : Bulk     */
      VCP_DATA_SIZE, 0x00, /* wMaxPacketSize             */
      0x00,                /* bInterval                  */

      /* Endpoint 1 Descriptor */
      0x07,                /* bLength                    */
      0x05,                /* bDescriptorType : Endpoint */
      0x81,                /* bEndpointAddress: EP 1 IN  */
      0x02,                /* bmAttributes    : Bulk     */
      VCP_DATA_SIZE, 0x00, /* wMaxPacketSize             */
      0x00                 /* bInterval                  */
};

/* USB String Descriptors */
const uint8_t StringLangID[] = {
  0x04,      /* bLength                            */
  0x03,      /* bDescriptorType: String            */
  0x09, 0x04 /* LangID         : 1033 U.S. English */
};

const uint8_t StringVendor[] = {
  0x26,      /* bLength                 */
  0x03,      /* bDescriptorType: String */
  'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
  'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'c', 0, 's', 0
};

const uint8_t StringProduct[] = {
  0x3c,      /* bLength                 */
  0x03,      /* bDescriptorType: String */
  'f', 0, 'l', 0, 'a', 0, 's', 0, 'h', 0, 'r', 0, 'o', 0, 'm', 0,
  '.', 0, 'o', 0, 'r', 0, 'g', 0, ' ', 0, 's', 0, 'e', 0, 'r', 0,
  'p', 0, 'r', 0, 'o', 0, 'g', 0, '-', 0, 'S', 0, 'T', 0, 'M', 0,
  '3', 0, '2', 0, 'V', 0, 'C', 0, 'P', 0
};

uint8_t StringSerial[26] = {
  0x1a,      /* bLength                 */
  0x03,      /* bDescriptorType: String */
  /* S/N is written during device init proccess. */
};

ONE_DESCRIPTOR Device_Descriptor = {
  (uint8_t *)DeviceDescriptor, sizeof(DeviceDescriptor)
};

ONE_DESCRIPTOR Config_Descriptor = {
  (uint8_t *)ConfigDescriptor, sizeof(ConfigDescriptor)
};

ONE_DESCRIPTOR String_Descriptor[] = {
  {(uint8_t *)StringLangID , sizeof(StringLangID )},
  {(uint8_t *)StringVendor , sizeof(StringVendor )},
  {(uint8_t *)StringProduct, sizeof(StringProduct)},
  {(uint8_t *)StringSerial , sizeof(StringSerial )},
};

/******************************************************
 * Endpoints callbacks                                *
 ******************************************************/

void (*pEpInt_IN [7])(void) = EMPTY_CALLBACKS;
void (*pEpInt_OUT[7])(void) = EMPTY_CALLBACKS;

/******************************************************
 * Status                                             *
 ******************************************************/

__IO uint16_t wIstr; /* ISTR register last read value */

LINE_CODING linecoding = {
  115200, /* Baud rate  */
  0x00,   /* 1 stop bit */
  0x00,   /* No parity  */
  0x08,   /* 8 bits     */
};

/******************************************************
 * Device properties                                  *
 ******************************************************/

DEVICE Device_Table = {
  4, /* Endpoints  */
  1, /* Interfaces */
};

DEVICE_PROP Device_Property = {
  Device_Init,
  Device_Reset,
  NOP_Process, /* Status in  */
  NOP_Process, /* Status out */
  Data_Setup,
  NoData_Setup,
  Get_Interface_Setting,
  GetDeviceDescriptor,
  GetConfigDescriptor,
  GetStringDescriptor,
  0,
  VCP_DATA_SIZE,
};

USER_STANDARD_REQUESTS User_Standard_Requests = {
  NOP_Process, /* Get configuration    */
  SetConfiguration,
  NOP_Process, /* Get interface        */
  NOP_Process, /* Set interface        */
  NOP_Process, /* Get status           */
  NOP_Process, /* Clear feature        */
  NOP_Process, /* Set endpoint feature */
  NOP_Process, /* Set device feature   */
  NOP_Process, /* Set device address   */
};

/******************************************************
 * Functions                                          *
 ******************************************************/

static void IntToUnicode(uint32_t value, uint8_t *pbuf) {
  uint8_t idx = 0;

  for(idx = 0; idx < 4; idx ++) {
    if((value & 0x0f) < 0x0a ) pbuf[2 * idx] = (value & 0x0f) + '0';
    else                       pbuf[2 * idx] = (value & 0x0f) + 'A' - 10;
    value >>= 4;
    pbuf[2 * idx + 1] = 0;
  }
}

void Device_Init(void) {
  /* Update iSerial with MCU unique ID */
  IntToUnicode(*(__IO uint32_t*)(0x1FFFF7E8), &StringSerial[ 2]);
  IntToUnicode(*(__IO uint32_t*)(0x1FFFF7EC), &StringSerial[10]);
  IntToUnicode(*(__IO uint32_t*)(0x1FFFF7F0), &StringSerial[18]);

  pInformation->Current_Configuration = 0;

  /* Connect the device */
  _SetCNTR(CNTR_FRES);
  wInterrupt_Mask = 0;
  _SetCNTR(wInterrupt_Mask);
  _SetISTR(0);
  wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM;
  _SetCNTR(wInterrupt_Mask);

  /* Perform basic device initialization operations */
  _SetISTR(0);
  wInterrupt_Mask = CNTR_CTRM | CNTR_SOFM | CNTR_RESETM;
  _SetCNTR(wInterrupt_Mask);

  /* Device unconnected */
  led_off();
}

void Device_Reset(void) {
  /* Set the device as not configured */
  pInformation->Current_Configuration = 0;

  /* Current feature initialization */
  pInformation->Current_Feature = ConfigDescriptor[7];

  /* Set the device with the default Interface*/
  pInformation->Current_Interface = 0;

  SetBTABLE(0x00);

  /* Initialize Endpoint 0 */
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_STALL);
  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
  SetEPRxValid(ENDP0);

  /* Initialize Endpoint 1 */
  SetEPType(ENDP1, EP_BULK);
  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
  SetEPTxStatus(ENDP1, EP_TX_NAK);
  SetEPRxStatus(ENDP1, EP_RX_DIS);

  /* Initialize Endpoint 2 */
  SetEPType(ENDP2, EP_INTERRUPT);
  SetEPTxAddr(ENDP2, ENDP2_TXADDR);
  SetEPRxStatus(ENDP2, EP_RX_DIS);
  SetEPTxStatus(ENDP2, EP_TX_NAK);

  /* Initialize Endpoint 3 */
  SetEPType(ENDP3, EP_BULK);
  SetEPRxAddr(ENDP3, ENDP3_RXADDR);
  SetEPRxCount(ENDP3, VCP_DATA_SIZE);
  SetEPRxStatus(ENDP3, EP_RX_VALID);
  SetEPTxStatus(ENDP3, EP_TX_DIS);

  /* Set this device to response on default address */
  SetDeviceAddress(0);

  /* Device attached */
  led_off();
}

void SetConfiguration(void) {
  /* Device configured */
  if(Device_Info.Current_Configuration != 0) led_on();
}

RESULT Data_Setup(uint8_t RequestNo) {
  uint8_t *(*CopyRoutine)(uint16_t);
  CopyRoutine = NULL;

  if(RequestNo == GET_LINE_CODING) {
    if(Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) CopyRoutine = GetLineCoding;
  }
  else if(RequestNo == SET_LINE_CODING) {
    if(Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) CopyRoutine = SetLineCoding;
  }

  if(CopyRoutine == NULL) return USB_UNSUPPORT;

  pInformation->Ctrl_Info.CopyData = CopyRoutine;
  pInformation->Ctrl_Info.Usb_wOffset = 0;
  (*CopyRoutine)(0);
  return USB_SUCCESS;
}

RESULT NoData_Setup(uint8_t RequestNo) {
  if(Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {
    if((RequestNo == SET_COMM_FEATURE) | (RequestNo == SET_CONTROL_LINE_STATE)) return USB_SUCCESS;
  }

  return USB_UNSUPPORT;
}

uint8_t *GetDeviceDescriptor(uint16_t Length) {
  return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

uint8_t *GetConfigDescriptor(uint16_t Length) {
  return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

uint8_t *GetStringDescriptor(uint16_t Length) {
  if(pInformation->USBwValue0 > 4) return NULL;

  return Standard_GetDescriptorData(Length, &String_Descriptor[pInformation->USBwValue0]);
}

RESULT Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting) {
  if(AlternateSetting > 0) return USB_UNSUPPORT;
  if(Interface > 1)        return USB_UNSUPPORT;

  return USB_SUCCESS;
}

uint8_t *GetLineCoding(uint16_t Length) {
  if(Length == 0) {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }
  return(uint8_t *)&linecoding;
}

uint8_t *SetLineCoding(uint16_t Length) {
  if(Length == 0) {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }
  return(uint8_t *)&linecoding;
}

/* Minimum USB interrupt handler for USB CDC */
void USB_Istr(void) {
  wIstr = _GetISTR();

  if (wIstr & ISTR_SOF & wInterrupt_Mask) _SetISTR(CLR_SOF);
  if (wIstr & ISTR_CTR & wInterrupt_Mask) CTR_LP();

  if (wIstr & ISTR_RESET & wInterrupt_Mask) {
    _SetISTR(CLR_RESET);
    Device_Property.Reset();
  }
}
