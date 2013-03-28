#ifndef ___CONFIG_H
#define ___CONFIG_H

/* Board */
#define PORT_LED              GPIOA
#define PIN_LED               GPIO_Pin_0
#define PORT_SS               GPIOA              /* Software SS, assign as you like */
#define PIN_SS                GPIO_Pin_4

/* USB */
#define MASS_MEMORY_START     0x04002000
#define BULK_MAX_PACKET_SIZE  0x00000040         /* 64 Bytes */
#define VCP_DATA_SIZE         0x40               /* Should be the same as BULK_MAX_PACKET_SIZE */

/* serprog */
#define S_IFACE_VERSION       0x01               /* Version of the protocol */
#define S_PGM_NAME            "serprog-STM32VCP" /* The program's name, must < 16 bytes */
#define SUPPORTED_BUS         0x08               /* SPI only */

#define SUPPORTED_COMMANDS_LOW ( \
  ( \
    (1 << S_CMD_NOP)       | \
    (1 << S_CMD_Q_IFACE)   | \
    (1 << S_CMD_Q_CMDMAP)  | \
    (1 << S_CMD_Q_PGMNAME) | \
    (1 << S_CMD_Q_SERBUF)  | \
    (1 << S_CMD_Q_BUSTYPE)   \
  ) \
  & 0xff \
)
#define SUPPORTED_COMMANDS_HIGH ( \
  ( \
    (1 << (S_CMD_SYNCNOP   - 16)) | \
    (1 << (S_CMD_O_SPIOP   - 16)) | \
    (1 << (S_CMD_S_BUSTYPE - 16))   \
  ) & 0xff \
)

/* GPIO */
#define select_chip() GPIO_ResetBits( PORT_SS,  PIN_SS)
#define unselect_chip() GPIO_SetBits( PORT_SS,  PIN_SS)
#define led_off()     GPIO_ResetBits(PORT_LED, PIN_LED)
#define led_on()        GPIO_SetBits(PORT_LED, PIN_LED)

#endif  /*___CONFIG_H*/