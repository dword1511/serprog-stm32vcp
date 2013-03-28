# Toolchain path and prefix.
TOOLCHAIN       = arm-none-eabi-

# Enter your project name here.
PROJECT         = serprog_vcp

# If no bootloader, set to zero.
ROM_OFFSET_SIZE = 0x0

# Change according to your MCU model.
FLASH_SIZE      = 64K
SRAM_SIZE       = 20K
EXT_RAM_SIZE    = 0K

# Used to control source behavior.
SETTINGS        = -I./src

# Objects list.
OBJS += src/startup.o
OBJS += src/main.o
OBJS += src/usb_istr.o
OBJS += src/usb_prop.o

# Do not touch
TARGET_ARCH     = -mthumb -mcpu=cortex-m3
include common.mk

program:
	stm32flash -b 115200 -w $(PROJECT).bin -v /dev/ttyUSB0
