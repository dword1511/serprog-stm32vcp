#include <stm32/rcc.h>
#include <stm32/gpio.h>
#include <stm32/misc.h>
#include <stm32/usb/lib.h>
#include "io_usb.h"
#include "io_spi.h"
#include "serprog.h"
#include "config.h"

GPIO_InitTypeDef GPIO_InitStructure_LED    = {
  .GPIO_Pin                          = PIN_LED,
  .GPIO_Speed                        = GPIO_Speed_2MHz,
  .GPIO_Mode                         = GPIO_Mode_Out_PP,
};

GPIO_InitTypeDef GPIO_InitStructure_SPIOUT = {
  .GPIO_Pin                          = GPIO_Pin_5 | GPIO_Pin_7,
  .GPIO_Speed                        = GPIO_Speed_50MHz,
  .GPIO_Mode                         = GPIO_Mode_AF_PP,
};

GPIO_InitTypeDef GPIO_InitStructure_SPIIN  = {
  .GPIO_Pin                          = GPIO_Pin_6,
  .GPIO_Speed                        = GPIO_Speed_50MHz,
  .GPIO_Mode                         = GPIO_Mode_IN_FLOATING,
};

GPIO_InitTypeDef GPIO_InitStructure_SPISS  = {
  .GPIO_Pin                          = PIN_SS,
  .GPIO_Speed                        = GPIO_Speed_50MHz,
  .GPIO_Mode                         = GPIO_Mode_Out_PP,
};

NVIC_InitTypeDef NVIC_InitStructure        = {
  .NVIC_IRQChannel                   = USB_LP_CAN1_RX0_IRQn,
  .NVIC_IRQChannelPreemptionPriority = 1,
  .NVIC_IRQChannelSubPriority        = 0,
  .NVIC_IRQChannelCmd                = ENABLE,
};

void serprog_handle_command(unsigned char command);

/* 72MHz, 3 cycles per loop. */
void delay(volatile uint32_t cycles) {
  while(cycles -- != 0);
}

int main(void) {
  /* Configure Clocks (GPIO and DMA clocks already enabled by startup.c) */
  RCC_PCLK2Config(RCC_HCLK_Div2);
  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
  RCC_APB2PeriphClockCmd(    SPI_ENGINE_RCC, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);

  /* Configure On-board LED */
  GPIO_Init(PORT_LED, &GPIO_InitStructure_LED);

  /* Configure SPI Port */
  GPIO_Init(   GPIOA, &GPIO_InitStructure_SPIOUT);
  GPIO_Init(   GPIOA, &GPIO_InitStructure_SPIIN);
  GPIO_Init( PORT_SS, &GPIO_InitStructure_SPISS);

  /* Configure SPI Engine with DMA */
  spi_conf(SPI_DEFAULT_SPEED);

  /* Configure USB Interrupt */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_Init(&NVIC_InitStructure);

  /* Enable all peripherals */
  USB_Init();

  /* Main loop */
  while(1) {
    /* Get command */
    serprog_handle_command(usb_getc());
    /* Flush output via USB */
    usb_sync();
  }

  return 0;
}

void serprog_handle_command(unsigned char command) {
  led_off();

  static uint8_t   i;        /* Loop            */
  static uint8_t   l;        /* Length          */
  static uint32_t  slen;     /* SPIOP write len */
  static uint32_t  rlen;     /* SPIOP read len  */
  static uint32_t  freq_req;

  switch(command) {
    case S_CMD_NOP:
      usb_putc(S_ACK);
      break;
    case S_CMD_Q_IFACE:
      usb_putc(S_ACK);
      /* little endian multibyte value to complete to 16bit */
      usb_putc(S_IFACE_VERSION);
      usb_putc(0);
    break;
      case S_CMD_Q_CMDMAP:
      usb_putc(S_ACK);
      /* little endian */
      usb_putu32(S_CMD_MAP);
      for(i = 0; i < 32 - sizeof(uint32_t); i++) usb_putc(0);
      break;
    case S_CMD_Q_PGMNAME:
      usb_putc(S_ACK);
      l = 0;
      while(S_PGM_NAME[l]) {
        usb_putc(S_PGM_NAME[l]);
        l ++;
      }
      for(i = l; i < 16; i++) usb_putc(0);
      break;
    case S_CMD_Q_SERBUF:
      usb_putc(S_ACK);
      /* Pretend to be 64K (0xffff) */
      usb_putc(0xff);
      usb_putc(0xff);
      break;
    case S_CMD_Q_BUSTYPE:
      // TODO: LPC / FWH IO support via PP-Mode
      usb_putc(S_ACK);
      usb_putc(S_SUPPORTED_BUS);
      break;
    case S_CMD_Q_CHIPSIZE: break;
    case S_CMD_Q_OPBUF:
      // TODO: opbuf function 0
      break;
    case S_CMD_Q_WRNMAXLEN: break;
    case S_CMD_R_BYTE:     break;
    case S_CMD_R_NBYTES:   break;
    case S_CMD_O_INIT:     break;
    case S_CMD_O_WRITEB:
      // TODO: opbuf function 1
      break;
    case S_CMD_O_WRITEN:
      // TODO: opbuf function 2
      break;
    case S_CMD_O_DELAY:
      // TODO: opbuf function 3
      break;
    case S_CMD_O_EXEC:
      // TODO: opbuf function 4
      break;
    case S_CMD_SYNCNOP:
      usb_putc(S_NAK);
      usb_putc(S_ACK);
      break;
    case S_CMD_Q_RDNMAXLEN:
      // TODO
      break;
    case S_CMD_S_BUSTYPE:
      /* We do not have multiplexed bus interfaces,
       * so simply ack on supported types, no setup needed. */
      if((usb_getc() | S_SUPPORTED_BUS) == S_SUPPORTED_BUS) usb_putc(S_ACK);
      else usb_putc(S_NAK);
      break;
    case S_CMD_O_SPIOP:
      slen = usb_getu24();
      rlen = usb_getu24();

      select_chip();

      /* TODO: handle errors with S_NAK */
      if(slen) spi_bulk_write(slen);
      usb_putc(S_ACK);
      if(rlen) spi_bulk_read(rlen);

      unselect_chip();
      break;
    case S_CMD_S_SPI_FREQ:
      freq_req = usb_getu32();
      if(freq_req == 0) usb_putc(S_NAK);
      else {
        usb_putc(S_ACK);
        usb_putu32(spi_conf(freq_req));
      }
      break;
    default: break; // TODO: Debug malformed command
  }

  led_on();
}
