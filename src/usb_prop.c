#include <stm32/gpio.h>
#include <stm32/usb/lib.h>
#include "usb_conf.h"
#include "usb_istr.h"
#include "config.h"

/* This file handles basic libstm32usb callbacks. */

/* USB Standard Device Descriptor */
const uint8_t Virtual_Com_Port_DeviceDescriptor[] = {
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
const uint8_t Virtual_Com_Port_ConfigDescriptor[] = {
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
const uint8_t Virtual_Com_Port_StringLangID[] = {
  0x04,      /* bLength                            */
  0x03,      /* bDescriptorType: String            */
  0x09, 0x04 /* LangID         : 1033 U.S. English */
};

const uint8_t Virtual_Com_Port_StringVendor[] = {
  0x26,      /* bLength                 */
  0x03,      /* bDescriptorType: String */
  'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
  'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'c', 0, 's', 0
};

const uint8_t Virtual_Com_Port_StringProduct[] = {
  0x3c,      /* bLength                 */
  0x03,      /* bDescriptorType: String */
  'f', 0, 'l', 0, 'a', 0, 's', 0, 'h', 0, 'r', 0, 'o', 0, 'm', 0,
  '.', 0, 'o', 0, 'r', 0, 'g', 0, ' ', 0, 's', 0, 'e', 0, 'r', 0,
  'p', 0, 'r', 0, 'o', 0, 'g', 0, '-', 0, 'S', 0, 'T', 0, 'M', 0,
  '3', 0, '2', 0, 'V', 0, 'C', 0, 'P', 0
};

/* NOTE: at least on gcc you have to specify the size of array for this crap: */
uint8_t Virtual_Com_Port_StringSerial[26] = {
  0x1a,      /* bLength                 */
  0x03,      /* bDescriptorType: String */
  'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0, 'F', 0, '1', 0
};

typedef struct {
  uint32_t bitrate;
  uint8_t format;
  uint8_t paritytype;
  uint8_t datatype;
} LINE_CODING;

#define Virtual_Com_Port_GetConfiguration          NOP_Process
//#define Virtual_Com_Port_SetConfiguration          NOP_Process
#define Virtual_Com_Port_GetInterface              NOP_Process
#define Virtual_Com_Port_SetInterface              NOP_Process
#define Virtual_Com_Port_GetStatus                 NOP_Process
#define Virtual_Com_Port_ClearFeature              NOP_Process
#define Virtual_Com_Port_SetEndPointFeature        NOP_Process
#define Virtual_Com_Port_SetDeviceFeature          NOP_Process
//#define Virtual_Com_Port_SetDeviceAddress          NOP_Process
void Virtual_Com_Port_init(void);
void Virtual_Com_Port_Reset(void);
void Virtual_Com_Port_SetConfiguration(void);
void Virtual_Com_Port_SetDeviceAddress (void);
void Virtual_Com_Port_Status_In (void);
void Virtual_Com_Port_Status_Out (void);
RESULT Virtual_Com_Port_Data_Setup(uint8_t);
RESULT Virtual_Com_Port_NoData_Setup(uint8_t);
RESULT Virtual_Com_Port_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t *Virtual_Com_Port_GetDeviceDescriptor(uint16_t);
uint8_t *Virtual_Com_Port_GetConfigDescriptor(uint16_t);
uint8_t *Virtual_Com_Port_GetStringDescriptor(uint16_t);
uint8_t *Virtual_Com_Port_GetLineCoding(uint16_t Length);
uint8_t *Virtual_Com_Port_SetLineCoding(uint16_t Length);

#define SEND_ENCAPSULATED_COMMAND 0x00
#define GET_ENCAPSULATED_RESPONSE 0x01
#define SET_COMM_FEATURE          0x02
#define GET_COMM_FEATURE          0x03
#define CLEAR_COMM_FEATURE        0x04
#define SET_LINE_CODING           0x20
#define GET_LINE_CODING           0x21
#define SET_CONTROL_LINE_STATE    0x22
#define SEND_BREAK                0x23

uint8_t Request = 0;

/* Just pretend we have a physical UART interface */
LINE_CODING linecoding = {
    115200, /* Baud rate     */
    0x00,   /* 1 stop bit    */
    0x00,   /* No parity     */
    0x08,   /* 8 bits        */
};

DEVICE Device_Table = {
  EP_NUM,
  1,
};

DEVICE_PROP Device_Property = {
  Virtual_Com_Port_init,
  Virtual_Com_Port_Reset,
  Virtual_Com_Port_Status_In,
  Virtual_Com_Port_Status_Out,
  Virtual_Com_Port_Data_Setup,
  Virtual_Com_Port_NoData_Setup,
  Virtual_Com_Port_Get_Interface_Setting,
  Virtual_Com_Port_GetDeviceDescriptor,
  Virtual_Com_Port_GetConfigDescriptor,
  Virtual_Com_Port_GetStringDescriptor,
  0,
  VCP_DATA_SIZE,
};

USER_STANDARD_REQUESTS User_Standard_Requests = {
  Virtual_Com_Port_GetConfiguration,
  Virtual_Com_Port_SetConfiguration,
  Virtual_Com_Port_GetInterface,
  Virtual_Com_Port_SetInterface,
  Virtual_Com_Port_GetStatus,
  Virtual_Com_Port_ClearFeature,
  Virtual_Com_Port_SetEndPointFeature,
  Virtual_Com_Port_SetDeviceFeature,
  Virtual_Com_Port_SetDeviceAddress,
};

ONE_DESCRIPTOR Device_Descriptor = {
  (uint8_t *)Virtual_Com_Port_DeviceDescriptor,
  sizeof(Virtual_Com_Port_DeviceDescriptor)
};

ONE_DESCRIPTOR Config_Descriptor = {
  (uint8_t *)Virtual_Com_Port_ConfigDescriptor,
  sizeof(Virtual_Com_Port_ConfigDescriptor)
};

ONE_DESCRIPTOR String_Descriptor[] = {
  {(uint8_t *)Virtual_Com_Port_StringLangID , sizeof(Virtual_Com_Port_StringLangID )},
  {(uint8_t *)Virtual_Com_Port_StringVendor , sizeof(Virtual_Com_Port_StringVendor )},
  {(uint8_t *)Virtual_Com_Port_StringProduct, sizeof(Virtual_Com_Port_StringProduct)},
  {(uint8_t *)Virtual_Com_Port_StringSerial , sizeof(Virtual_Com_Port_StringSerial )},
};

static void IntToUnicode(uint32_t value, uint8_t *pbuf) {
  uint8_t idx = 0;

  for(idx = 0; idx < 4; idx ++) {
    if((value & 0x0f) < 0x0a ) pbuf[2 * idx] = (value & 0x0f) + '0';
    else                       pbuf[2 * idx] = (value & 0x0f) + 'A' - 10;
    value >>= 4;
    pbuf[2 * idx + 1] = 0;
  }
}

void Virtual_Com_Port_init(void) {
  /* Update the serial number string descriptor with the data from the unique
  ID*/
  uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

  Device_Serial0 = *(__IO uint32_t*)(0x1FFFF7E8);
  Device_Serial1 = *(__IO uint32_t*)(0x1FFFF7EC);
  Device_Serial2 = *(__IO uint32_t*)(0x1FFFF7F0);

  if(Device_Serial0 != 0) {
    IntToUnicode(Device_Serial0, &Virtual_Com_Port_StringSerial[ 2]);
    IntToUnicode(Device_Serial1, &Virtual_Com_Port_StringSerial[10]);
    IntToUnicode(Device_Serial2, &Virtual_Com_Port_StringSerial[18]);
  }

  pInformation->Current_Configuration = 0;

  /* Connect the device */
  PowerOn();

  /* Perform basic device initialization operations */
  /* USB interrupts initialization */
  /* clear pending interrupts */
  _SetISTR(0);
  wInterrupt_Mask = IMR_MSK;
  /* set interrupts mask */
  _SetCNTR(wInterrupt_Mask);

  bDeviceState = UNCONNECTED;
  led_off();
}

void Virtual_Com_Port_Reset(void) {
  /* Set Virtual_Com_Port DEVICE as not configured */
  pInformation->Current_Configuration = 0;

  /* Current Feature initialization */
  pInformation->Current_Feature = Virtual_Com_Port_ConfigDescriptor[7];

  /* Set Virtual_Com_Port DEVICE with the default Interface*/
  pInformation->Current_Interface = 0;

  SetBTABLE(BTABLE_ADDRESS);

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

  bDeviceState = ATTACHED;
  led_off();
}

void Virtual_Com_Port_SetConfiguration(void) {
  DEVICE_INFO *pInfo = &Device_Info;

  /* Device configured */
  if(pInfo->Current_Configuration != 0) {
    bDeviceState = CONFIGURED;
    led_on();
  }
}

void Virtual_Com_Port_SetDeviceAddress(void) {
  bDeviceState = ADDRESSED;
}

void Virtual_Com_Port_Status_In(void) {
  if(Request == SET_LINE_CODING) Request = 0;
}

void Virtual_Com_Port_Status_Out(void) {}

RESULT Virtual_Com_Port_Data_Setup(uint8_t RequestNo) {
  uint8_t *(*CopyRoutine)(uint16_t);
  CopyRoutine = NULL;

  if(RequestNo == GET_LINE_CODING){
    if(Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) CopyRoutine = Virtual_Com_Port_GetLineCoding;
  }
  else if(RequestNo == SET_LINE_CODING) {
    if(Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) CopyRoutine = Virtual_Com_Port_SetLineCoding;
    Request = SET_LINE_CODING;
  }

  if(CopyRoutine == NULL) return USB_UNSUPPORT;

  pInformation->Ctrl_Info.CopyData = CopyRoutine;
  pInformation->Ctrl_Info.Usb_wOffset = 0;
  (*CopyRoutine)(0);
  return USB_SUCCESS;
}

RESULT Virtual_Com_Port_NoData_Setup(uint8_t RequestNo) {
  if(Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {
    if(RequestNo == SET_COMM_FEATURE) return USB_SUCCESS;
    else if(RequestNo == SET_CONTROL_LINE_STATE) return USB_SUCCESS;
  }

  return USB_UNSUPPORT;
}

uint8_t *Virtual_Com_Port_GetDeviceDescriptor(uint16_t Length) {
  return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

uint8_t *Virtual_Com_Port_GetConfigDescriptor(uint16_t Length) {
  return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

uint8_t *Virtual_Com_Port_GetStringDescriptor(uint16_t Length) {
  uint8_t wValue0 = pInformation->USBwValue0;
  if(wValue0 > 4) return NULL;
  else return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
}

RESULT Virtual_Com_Port_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting) {
  if(AlternateSetting > 0) return USB_UNSUPPORT;
  else if(Interface > 1) return USB_UNSUPPORT;

  return USB_SUCCESS;
}

uint8_t *Virtual_Com_Port_GetLineCoding(uint16_t Length) {
  if(Length == 0) {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }
  return(uint8_t *)&linecoding;
}

uint8_t *Virtual_Com_Port_SetLineCoding(uint16_t Length) {
  if(Length == 0) {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }
  return(uint8_t *)&linecoding;
}
