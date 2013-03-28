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

const uint8_t DMA_Clk_Buf[VCP_DATA_SIZE] = {0};

GPIO_InitTypeDef GPIO_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;
SPI_InitTypeDef  SPI_InitStructure;
DMA_InitTypeDef  DMA_InitStructure;

void serprog_handle_command(unsigned char command);

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
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
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

  /* SPI DMA Channels */
  DMA_DeInit(SPI_READ_DMA_CH);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)SPI1_DR_Base;
  DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)USB_Tx_Buf;
  DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize         = VCP_DATA_SIZE;
  DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
  DMA_Init(SPI_READ_DMA_CH, &DMA_InitStructure);
  DMA_DeInit(SPI_WRITE_DMA_CH);
  //DMA_InitStructure.DMA_PeripheralBaseAddr = SPI1_DR_Base;
  DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)USB_Rx_Buf;
  DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_Priority           = DMA_Priority_Low;
  DMA_Init(SPI_WRITE_DMA_CH, &DMA_InitStructure);
  DMA_DeInit(SPI_CLOCK_DMA_CH);
  DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)DMA_Clk_Buf;
  DMA_Init(SPI_CLOCK_DMA_CH, &DMA_InitStructure);

  /* Configure USB Interrupt */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel                   = USB_LP_CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable all peripherals */
  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);
  // TODO: implement spi and dma error handling
  SPI_CalculateCRC(SPI1, DISABLE);
  SPI_Cmd(SPI1, ENABLE);
  USB_Init();

//  /* Wait for everything settles */
//  while(!DMA_GetFlagStatus(DMA1_FLAG_TC4));
//  while(!DMA_GetFlagStatus(DMA1_FLAG_TC5));
//  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != RESET);
//  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

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
  /* Finish the left-over bytes */
  if(USB_Rx_ptr_out != 0) {
    size -= (USB_Rx_len - USB_Rx_ptr_out);
    while(USB_Rx_ptr_out != USB_Rx_len) spi_putc(getchar_uart());
    pget();
  }

  /* Do bulk transfer */
  while(size > VCP_DATA_SIZE) {
    DMA_Cmd(SPI_WRITE_DMA_CH, ENABLE);
    while(!DMA_GetFlagStatus(SPI_WRITE_DMA_FLAG));
    pget();
    size -= VCP_DATA_SIZE;
  }

  /* Finish the left-over bytes */
  while(size != 0) spi_putc(getchar_uart());
}

void spi_bulk_read(uint32_t size) {
  /* Flush buffer and make room for DMA */
  if(USB_Tx_ptr_in != 0) pput();

  /* Do bulk transfer */
  while(size > VCP_DATA_SIZE) {
    /* RX is enabled first to avoid data loss. */
    DMA_Cmd(SPI_READ_DMA_CH, ENABLE);
    DMA_Cmd(SPI_CLOCK_DMA_CH, ENABLE);
    while(!DMA_GetFlagStatus(SPI_READ_DMA_FLAG));
    USB_Tx_ptr_in = VCP_DATA_SIZE;
    pput();
    size -= VCP_DATA_SIZE;
  }

  /* Finish the left-over bytes */
  if(size != 0) {
    DMA_Cmd(SPI_READ_DMA_CH, ENABLE);
    DMA_Cmd(SPI_CLOCK_DMA_CH, ENABLE);
    while(!DMA_GetFlagStatus(SPI_READ_DMA_FLAG));
    USB_Tx_ptr_in = size;
    pput();
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
  //static char     c;
  static uint32_t slen = 0; /* SPIOP write len */
  static uint32_t rlen = 0; /* SPIOP read len  */

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
      /* SPI is configured in little endian */
      //while(slen--) {
      //  c = getchar_uart();
      //  readwrite_spi(c);
      //}
      spi_bulk_write(slen);
      putchar_uart(S_ACK);
      /* receive TODO: handle errors */
      //while(rlen--) putchar_uart(readwrite_spi(0x0));
      spi_bulk_read(rlen);
      unselect_chip();
      break;
    default: break;
  }

  led_on();
}
