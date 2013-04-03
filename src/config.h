#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Wirings */
#define PORT_LED             GPIOA
#define PIN_LED              GPIO_Pin_0
#define PORT_SS              GPIOA                 /* Software SS, assign as you like */
#define PIN_SS               GPIO_Pin_4

/* USB */
#define MASS_MEMORY_START    0x04002000
#define BULK_MAX_PACKET_SIZE 0x00000040            /* Max packet size for FullSpeed bulk transfer */
#define VCP_DATA_SIZE        0x40                  /* Should be the same as BULK_MAX_PACKET_SIZE */
#define ENDP0_RXADDR         0x40                  /* EP0 RX buffer base address */
#define ENDP0_TXADDR         0x80                  /* EP0 TX buffer base address */
#define ENDP1_TXADDR         0xC0                  /* EP1 TX buffer base address */
#define ENDP2_TXADDR         0x100                 /* EP2 TX buffer base address */
#define ENDP3_RXADDR         0x110                 /* EP3 RX buffer base address */

/* SPI */
#define SPI_BUS_USED         SPI1
#define SPI_ENGINE_RCC       RCC_APB2Periph_SPI1
#define SPI_DEFAULT_SPEED    9000000               /* Default SPI clock = 9MHz to support most chips.*/
#define SPI_DR_Base          (&(SPI_BUS_USED->DR))
#define SPI_TX_DMA_CH        DMA1_Channel3         /* SPI1 TX is only available on DMA1 CH3 */
#define SPI_TX_DMA_FLAG      DMA1_FLAG_TC3
#define SPI_RX_DMA_CH        DMA1_Channel2         /* SPI1 RX is only available on DMA1 CH2 */
#define SPI_RX_DMA_FLAG      DMA1_FLAG_TC2

/* serprog */
#define S_PGM_NAME            "serprog-STM32VCP"   /* The program's name, must < 16 bytes */
#define S_SUPPORTED_BUS       BUS_SPI
#define S_CMD_MAP ( \
  (1 << S_CMD_NOP)       | \
  (1 << S_CMD_Q_IFACE)   | \
  (1 << S_CMD_Q_CMDMAP)  | \
  (1 << S_CMD_Q_PGMNAME) | \
  (1 << S_CMD_Q_SERBUF)  | \
  (1 << S_CMD_Q_BUSTYPE) | \
  (1 << S_CMD_SYNCNOP)   | \
  (1 << S_CMD_O_SPIOP)   | \
  (1 << S_CMD_S_BUSTYPE) | \
  (1 << S_CMD_S_SPI_FREQ)  \
)

/* GPIO macros */
#define select_chip() GPIO_ResetBits( PORT_SS,  PIN_SS)
#define unselect_chip() GPIO_SetBits( PORT_SS,  PIN_SS)
#define led_off()     GPIO_ResetBits(PORT_LED, PIN_LED)
#define led_on()        GPIO_SetBits(PORT_LED, PIN_LED)

#endif  /* __CONFIG_H__ */