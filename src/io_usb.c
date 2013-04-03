#include <stm32/usb/lib.h>
#include "config.h"
#include "io_usb.h"

/* Do not place const in front of declarations.                  *
 * const variables are stored in flash that needs a 2-cycle wait */
uint8_t  USB_Tx_Buf[VCP_DATA_SIZE];
uint16_t USB_Tx_ptr_in  = 0;

uint8_t  USB_Rx_Buf[VCP_DATA_SIZE];
uint16_t USB_Rx_ptr_out = 0;
uint8_t  USB_Rx_len     = 0;

uint32_t val;

void usb_putp(void) {
  /* Previous transmission complete? */
  while(GetEPTxStatus(ENDP1) != EP_TX_NAK);
  /* Send buffer contents */
  UserToPMABufferCopy(USB_Tx_Buf, ENDP1_TXADDR, USB_Tx_ptr_in);
  SetEPTxCount(ENDP1, USB_Tx_ptr_in);
  SetEPTxValid(ENDP1);
  /* Reset buffer pointer */
  USB_Tx_ptr_in = 0;
}

void usb_getp(void) {
  /* Anything new? */
  while(GetEPRxStatus(ENDP3) != EP_RX_NAK);
  /* Get the length */
  USB_Rx_len = GetEPRxCount(ENDP3);
  if(USB_Rx_len > VCP_DATA_SIZE) USB_Rx_len = VCP_DATA_SIZE;
  /* Fetch data and fill buffer */
  PMAToUserBufferCopy(USB_Rx_Buf, ENDP3_RXADDR, VCP_DATA_SIZE);
  /* We are good, next? */
  SetEPRxValid(ENDP3);
  USB_Rx_ptr_out = 0;
}

void usb_putc(char data) {
  /* Feed new data */
  USB_Tx_Buf[USB_Tx_ptr_in] = data;
  USB_Tx_ptr_in ++;
  /* End of the buffer, send packet now */
  if(USB_Tx_ptr_in == VCP_DATA_SIZE) usb_putp();
}

char usb_getc(void) {
  /* End of the buffer, wait for new packet */
  if(USB_Rx_ptr_out == USB_Rx_len) usb_getp();
  /* Get data from the packet */
  USB_Rx_ptr_out ++;
  return USB_Rx_Buf[USB_Rx_ptr_out - 1];
}

uint32_t usb_getu24(void) {
  val = 0;

  val  = (uint32_t)usb_getc() << 0;
  val |= (uint32_t)usb_getc() << 8;
  val |= (uint32_t)usb_getc() << 16;

  return val;
}

uint32_t usb_getu32(void) {
  val = 0;

  val  = (uint32_t)usb_getc() << 0;
  val |= (uint32_t)usb_getc() << 8;
  val |= (uint32_t)usb_getc() << 16;
  val |= (uint32_t)usb_getc() << 24;

  return val;
}

void usb_putu32(uint32_t ww) {
  usb_putc(ww >>  0 & 0x000000ff);
  usb_putc(ww >>  8 & 0x000000ff);
  usb_putc(ww >> 16 & 0x000000ff);
  usb_putc(ww >> 24 & 0x000000ff);
}

void usb_sync(void) {
  if(USB_Tx_ptr_in != 0) usb_putp();
}
