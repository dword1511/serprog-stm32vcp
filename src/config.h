#ifndef ___CONFIG_H
#define ___CONFIG_H

/* Board */
#define PORT_LED              GPIOA
#define PIN_LED               GPIO_Pin_0
#define PORT_SS               GPIOA                   /* Software SS, assign as you like */
#define PIN_SS                GPIO_Pin_4

/* USB */
#define MASS_MEMORY_START     0x04002000
#define BULK_MAX_PACKET_SIZE  0x00000040              /* 64 Bytes */
#define VCP_DATA_SIZE         0x40                    /* Should be the same as BULK_MAX_PACKET_SIZE */

/* SPI */
#define SPI_DEFAULT_SPEED     9000000                 /* Default SPI clock = 9MHz to support most chips.*/

/* serprog */
#define S_PGM_NAME            "serprog-STM32VCP"      /* The program's name, must < 16 bytes */
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

/* GPIO */
#define select_chip() GPIO_ResetBits( PORT_SS,  PIN_SS)
#define unselect_chip() GPIO_SetBits( PORT_SS,  PIN_SS)
#define led_off()     GPIO_ResetBits(PORT_LED, PIN_LED)
#define led_on()        GPIO_SetBits(PORT_LED, PIN_LED)

#endif  /*___CONFIG_H*/