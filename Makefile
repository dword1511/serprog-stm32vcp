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
OBJS += src/cdc.o
OBJS += src/serprog.o
OBJS += src/io_usb.o
OBJS += src/io_spi.o

# Do not touch
TARGET_ARCH     = -mthumb -mcpu=cortex-m3
include common.mk
