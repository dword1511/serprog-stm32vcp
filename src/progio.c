#include <stm32/spi.h>
#include <stm32/dma.h>
#include <stm32/usb/lib.h>
#include "config.h"
#include "usb_conf.h"
#include "progio.h"

uint8_t  USB_Tx_Buf[VCP_DATA_SIZE];
uint16_t USB_Tx_ptr_in  = 0;

uint8_t  USB_Rx_Buf[VCP_DATA_SIZE];
uint16_t USB_Rx_ptr_out = 0;
uint8_t  USB_Rx_len     = 0;

SPI_InitTypeDef  SPI_InitStructure;

/*----------------*/
/* USB operations */
/*----------------*/
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

void usb_sync(void) {
  if(USB_Tx_ptr_in != 0) usb_putp();
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
  uint32_t val = 0;

  val  = (uint32_t)usb_getc() << 0;
  val |= (uint32_t)usb_getc() << 8;
  val |= (uint32_t)usb_getc() << 16;

  return val;
}

uint32_t usb_getu32(void) {
  uint32_t val = 0;

  val  = (uint32_t)usb_getc() << 0;
  val |= (uint32_t)usb_getc() << 8;
  val |= (uint32_t)usb_getc() << 16;
  val |= (uint32_t)usb_getc() << 24;

  return val;
}

void usb_putu32(uint32_t ww) {
  /* little-endian. */
  usb_putc(ww >>  0 & 0x000000ff);
  usb_putc(ww >>  8 & 0x000000ff);
  usb_putc(ww >> 16 & 0x000000ff);
  usb_putc(ww >> 24 & 0x000000ff);
}

/*----------------*/
/* SPI operations */
/*----------------*/
uint32_t spi_conf(uint32_t speed_hz) {
  static uint16_t clkdiv;
  static uint32_t relspd;

  /* SPI1 is on APB2 which runs @ 72MHz. */
  /* Lowest available */
  clkdiv = SPI_BaudRatePrescaler_256;
  relspd = 281250;
  if(speed_hz >= 562500) {
    clkdiv = SPI_BaudRatePrescaler_128;
    relspd = 562500;
  }
  if(speed_hz >= 1125000) {
    clkdiv = SPI_BaudRatePrescaler_64;
    relspd = 1125000;
  }
  if(speed_hz >= 2250000) {
    clkdiv = SPI_BaudRatePrescaler_32;
    relspd = 2250000;
  }
  if(speed_hz >= 4500000) {
    clkdiv = SPI_BaudRatePrescaler_16;
    relspd = 4500000;
  }
  if(speed_hz >= 9000000) {
    clkdiv = SPI_BaudRatePrescaler_8;
    relspd = 9000000;
  }
  if(speed_hz >= 18000000) {
    clkdiv = SPI_BaudRatePrescaler_4;
    relspd = 18000000;
  }
  if(speed_hz >= 36000000) {
    clkdiv = SPI_BaudRatePrescaler_2;
    relspd = 36000000;
  }

  SPI_I2S_DeInit(SPI1);

  SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = clkdiv;
  SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial     = 7;

  SPI_Init(SPI1, &SPI_InitStructure);

  SPI_Cmd(SPI1, ENABLE);

  return relspd;
}

#if 0
void spi_putc(uint8_t c) {
  /* transmit c on the SPI bus */
  SPI_I2S_SendData(SPI1, c);

  /* Wait for the TX and RX to be comlpeted, then clear buffer */
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
  SPI_I2S_ReceiveData(SPI1);
}


void spi_bulk_write(uint32_t size) {
  /* Prepare alignment */
  if(size >= (USB_Rx_len - USB_Rx_ptr_out)) {
    size -= (USB_Rx_len - USB_Rx_ptr_out);
    while(USB_Rx_ptr_out != USB_Rx_len) spi_putc(usb_getc());
  }
  /* else: size < VCP_DATA_SIZE, should jump to next section. */

  static int i;

  /* Do bulk transfer */
  /* Currently this takes less input than expected */
  while(size >= VCP_DATA_SIZE) {
    usb_getp();
    if(USB_Rx_len < VCP_DATA_SIZE) {
      /* Host is not feeding fast enough? */
      USB_Rx_ptr_out = 0;
      break;
    }
    for(i = 0; i < VCP_DATA_SIZE; i += 8){
      spi_putc(USB_Rx_Buf[i + 0]);
      spi_putc(USB_Rx_Buf[i + 1]);
      spi_putc(USB_Rx_Buf[i + 2]);
      spi_putc(USB_Rx_Buf[i + 3]);
      spi_putc(USB_Rx_Buf[i + 4]);
      spi_putc(USB_Rx_Buf[i + 5]);
      spi_putc(USB_Rx_Buf[i + 6]);
      spi_putc(USB_Rx_Buf[i + 7]);
    }
    size -= VCP_DATA_SIZE;
  }

  /* Finish the left-over bytes */
  while(size != 0) {
    spi_putc(usb_getc());
    size --;
  }
}
#else
void spi_bulk_write(uint32_t size) {
  while(size != 0) {
    SPI_I2S_SendData(SPI1, usb_getc());
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
    size --;
  }
}
#endif

void spi_bulk_read(uint32_t size) {
  /* Flush buffer and make room for DMA */
  if(USB_Tx_ptr_in != 0) usb_putp();

  static int i;

  /* Do bulk transfer */
  while(size >= VCP_DATA_SIZE) {
    for(i = 0; i < VCP_DATA_SIZE; i += 8) {
      SPI_I2S_SendData(SPI1, 0);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
      USB_Tx_Buf[i + 0] = SPI_I2S_ReceiveData(SPI1);

      SPI_I2S_SendData(SPI1, 0);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
      USB_Tx_Buf[i + 1] = SPI_I2S_ReceiveData(SPI1);

      SPI_I2S_SendData(SPI1, 0);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
      USB_Tx_Buf[i + 2] = SPI_I2S_ReceiveData(SPI1);

      SPI_I2S_SendData(SPI1, 0);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
      USB_Tx_Buf[i + 3] = SPI_I2S_ReceiveData(SPI1);

      SPI_I2S_SendData(SPI1, 0);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
      USB_Tx_Buf[i + 4] = SPI_I2S_ReceiveData(SPI1);

      SPI_I2S_SendData(SPI1, 0);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
      USB_Tx_Buf[i + 5] = SPI_I2S_ReceiveData(SPI1);

      SPI_I2S_SendData(SPI1, 0);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
      USB_Tx_Buf[i + 6] = SPI_I2S_ReceiveData(SPI1);

      SPI_I2S_SendData(SPI1, 0);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
      USB_Tx_Buf[i + 7] = SPI_I2S_ReceiveData(SPI1);
    }
    USB_Tx_ptr_in = VCP_DATA_SIZE;
    usb_putp();
    size -= VCP_DATA_SIZE;
  }

  /* Finish the left-over bytes */
  if(size != 0) {
    for(i = 0; i < size; i ++) {
      SPI_I2S_SendData(SPI1, 0);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
      USB_Tx_Buf[i] = SPI_I2S_ReceiveData(SPI1);
    }
    USB_Tx_ptr_in = size;
  }
}
