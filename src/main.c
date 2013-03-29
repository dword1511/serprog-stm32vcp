#include <stm32/rcc.h>
#include <stm32/gpio.h>
#include <stm32/misc.h>
#include <stm32/spi.h>
#include <stm32/dma.h>
#include <stm32/usb/lib.h>
#include <string.h>
#include "serprog.h"
#include "config.h"
#include "usb_conf.h"
#include "usb_istr.h"

uint8_t  USB_Tx_Buf[VCP_DATA_SIZE];
uint16_t USB_Tx_ptr_in  = 0;

uint8_t  USB_Rx_Buf[VCP_DATA_SIZE];
uint16_t USB_Rx_ptr_out = 0;
uint8_t  USB_Rx_len     = 0;

GPIO_InitTypeDef GPIO_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;
SPI_InitTypeDef  SPI_InitStructure;

void serprog_handle_command(unsigned char command);

#ifdef _DEBUG_
void fault_led(void) {
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_ResetBits(GPIOA, GPIO_Pin_1);

  while(1);
}
#endif

/* 72MHz, 3 cycles per loop. */
void delay(volatile uint32_t cycles) {
  while(cycles -- != 0);
}

void pput(void) {
  /* Previous transmission complete? */
  while(GetEPTxStatus(ENDP1) != EP_TX_NAK);
  /* Send buffer contents */
  UserToPMABufferCopy(USB_Tx_Buf, ENDP1_TXADDR, USB_Tx_ptr_in);
  SetEPTxCount(ENDP1, USB_Tx_ptr_in);
  SetEPTxValid(ENDP1);
  /* Reset buffer pointer */
  USB_Tx_ptr_in = 0;
}

void pget(void) {
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

void putchar_uart(unsigned char data) {
  /* Feed new data */
  USB_Tx_Buf[USB_Tx_ptr_in] = data;
  USB_Tx_ptr_in ++;
  /* End of the buffer, send packet now */
  if(USB_Tx_ptr_in == VCP_DATA_SIZE) pput();
}

char getchar_uart(void) {
  /* End of the buffer, wait for new packet */
  if(USB_Rx_ptr_out == USB_Rx_len) pget();
  /* Get data from the packet */
  USB_Rx_ptr_out ++;
  return USB_Rx_Buf[USB_Rx_ptr_out - 1];
}

int main(void) {
  /* Configure Clocks (GPIO and DMA clocks already enabled by startup.c) */
  RCC_PCLK2Config(RCC_HCLK_Div2);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);

  /* Configure On-board LED */
  GPIO_InitStructure.GPIO_Pin   = PIN_LED;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PORT_LED, &GPIO_InitStructure);

  /* Configure SPI Port */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin   = PIN_SS;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(PORT_SS, &GPIO_InitStructure);

  /* Configure SPI Engine */
  SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BAUD_DIV;
  SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial     = 7;
  SPI_Init(SPI1, &SPI_InitStructure);

  /* Configure USB Interrupt */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel                   = USB_LP_CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable all peripherals */
  // TODO: implement spi and dma error handling
  SPI_CalculateCRC(SPI1, DISABLE);
  SPI_Cmd(SPI1, ENABLE);
  USB_Init();

  /* Main loop */
  while(1) {
    /* Get command */
    serprog_handle_command(getchar_uart());
    /* Flush output via USB */
    if(USB_Tx_ptr_in != 0) pput();
  }

  return 0;
}

void spi_putc(uint8_t c) {
  /* transmit c on the SPI bus */
  SPI_I2S_SendData(SPI1, c);

  /* Wait for the TX and RX to be comlpeted */
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
}

void spi_bulk_write(uint32_t size) {
  /* Prepare alignment */
  if(size >= (USB_Rx_len - USB_Rx_ptr_out)) {
    size -= (USB_Rx_len - USB_Rx_ptr_out);
    while(USB_Rx_ptr_out != USB_Rx_len) {
      spi_putc(getchar_uart());
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
    }
  }
  /* else: size < VCP_DATA_SIZE, should jump to next section. */

  static int i;

  /* Do bulk transfer */
/*  while(size >= VCP_DATA_SIZE) {
    pget();
    for(i = 0; i < VCP_DATA_SIZE; i += 8){
      spi_putc(USB_Rx_Buf[i + 0]);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
      spi_putc(USB_Rx_Buf[i + 1]);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
      spi_putc(USB_Rx_Buf[i + 2]);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
      spi_putc(USB_Rx_Buf[i + 3]);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
      spi_putc(USB_Rx_Buf[i + 4]);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
      spi_putc(USB_Rx_Buf[i + 5]);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
      spi_putc(USB_Rx_Buf[i + 6]);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
      spi_putc(USB_Rx_Buf[i + 7]);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
    }
    size -= VCP_DATA_SIZE;
  }*/

  /* Finish the left-over bytes */
  while(size != 0) {
    spi_putc(getchar_uart());
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);
    size --;
  }

  /* Empty SPI read buffer */
 /*   - OVR (OverRun error) flag is cleared by software sequence: a read 
  *     operation to SPI_DR register (SPI_I2S_ReceiveData()) followed by a read 
  *     operation to SPI_SR register (SPI_I2S_GetFlagStatus()).
  *   - UDR (UnderRun error) flag is cleared by a read operation to 
  *     SPI_SR register (SPI_I2S_GetFlagStatus()).
  *   - MODF (Mode Fault) flag is cleared by software sequence: a read/write 
  *     operation to SPI_SR register (SPI_I2S_GetFlagStatus()) followed by a 
  *     write operation to SPI_CR1 register (SPI_Cmd() to enable the SPI).
  */
//  SPI_I2S_ReceiveData(SPI1);
//  SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE);
//  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
//  SPI_I2S_ReceiveData(SPI1);
//  SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE);
//  SPI_I2S_ReceiveData(SPI1);
//  SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE);
}

void spi_bulk_read(uint32_t size) {
  /* Flush buffer and make room for DMA */
  if(USB_Tx_ptr_in != 0) pput();

  static int i;
  SPI_I2S_ReceiveData(SPI1);
  SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE);
  SPI_I2S_ReceiveData(SPI1);

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
    pput();
    size -= VCP_DATA_SIZE;
  }

  /* Finish the left-over bytes */
  if(size != 0) {
    for(i = 0; i < size; i ++) {
      spi_putc(0);
      while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
      USB_Tx_Buf[i] = SPI_I2S_ReceiveData(SPI1);
    }
    USB_Tx_ptr_in = size;
  }
}

uint32_t get24_le() {
  uint32_t val = 0;

  val  = (uint32_t)getchar_uart() << 0;
  val |= (uint32_t)getchar_uart() << 8;
  val |= (uint32_t)getchar_uart() << 16;

  return val;
}

void serprog_handle_command(unsigned char command) {
  led_off();

  static uint8_t  i;        /* Loop            */
  static uint8_t  l;        /* Length          */
  static uint32_t slen;     /* SPIOP write len */
  static uint32_t rlen;     /* SPIOP read len  */

  switch(command) {
    case S_CMD_NOP:
      putchar_uart(S_ACK);
      break;
    case S_CMD_Q_IFACE:
      putchar_uart(S_ACK);
      putchar_uart(S_IFACE_VERSION);
      /* little endian multibyte value to complete to 16bit */
      putchar_uart(0);
    break;
      case S_CMD_Q_CMDMAP:
      putchar_uart(S_ACK);
      /* little endian */
      putchar_uart(SUPPORTED_COMMANDS_LOW);
      putchar_uart(0x00);
      putchar_uart(SUPPORTED_COMMANDS_HIGH);
      for(i = 0; i < 29; i++) putchar_uart(0);
      break;
    case S_CMD_Q_PGMNAME:
      putchar_uart(S_ACK);
      l = strlen(S_PGM_NAME);
      for(i = 0; i <  l; i++) putchar_uart(S_PGM_NAME[i]);
      for(i = l; i < 16; i++) putchar_uart(0);
      break;
    case S_CMD_Q_SERBUF:
      putchar_uart(S_ACK);
      /* Pretend to be 64K (0xffff) */
      putchar_uart(0xff);
      putchar_uart(0xff);
      break;
    case S_CMD_Q_BUSTYPE:
      putchar_uart(S_ACK);
      putchar_uart(SUPPORTED_BUS);
      break;
    case S_CMD_Q_CHIPSIZE: break;
    case S_CMD_Q_OPBUF:    break;
    case S_CMD_Q_WRNMAXLEN:
      // TODO: Buffered & Parallel / LPC / FWH IO support
      break;
    case S_CMD_R_BYTE:     break;
    case S_CMD_R_NBYTES:   break;
    case S_CMD_O_INIT:     break;
    case S_CMD_O_WRITEB:   break;
    case S_CMD_O_WRITEN:   break;
    case S_CMD_O_DELAY:
      // TODO
      break;
    case S_CMD_O_EXEC:     break;
    case S_CMD_SYNCNOP:
      putchar_uart(S_NAK);
      putchar_uart(S_ACK);
      break;
    case S_CMD_Q_RDNMAXLEN:
      // TODO
      break;
    case S_CMD_S_BUSTYPE:
      switch(getchar_uart()) {
        case SUPPORTED_BUS:
          putchar_uart(S_ACK);
          break;
        default:
          putchar_uart(S_NAK);
          break;
      }
      break;
    case S_CMD_O_SPIOP:
      slen = get24_le();
      rlen = get24_le();

      select_chip();

      if(slen) spi_bulk_write(slen);
      //if(slen == 1 && rlen == 2) delay(6000000);
      putchar_uart(S_ACK);
      //pput();
      /* receive TODO: handle errors */
      if(rlen) spi_bulk_read(rlen);

      unselect_chip();
      break;
    default: break;
  }

  led_on();
}
