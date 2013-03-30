CC       = $(TOOLCHAIN)gcc
LD       = $(TOOLCHAIN)gcc
NM       = $(TOOLCHAIN)nm
SIZE     = $(TOOLCHAIN)size
STRIP    = $(TOOLCHAIN)strip
OBJCOPY  = $(TOOLCHAIN)objcopy
OBJDUMP  = $(TOOLCHAIN)objdump

CFLAGS   = $(TARGET_ARCH) $(SETTINGS)
#CFLAGS  += -Wall -O3 -g -mlong-calls
CFLAGS  += -Wall -O3 -g
CFLAGS  += -fno-exceptions -ffunction-sections -fdata-sections

LDFLAGS  = -nostartfiles
LDFLAGS += -Wl,--gc-sections,-Map=$(PROJECT).map,-cref $(TARGET_ARCH)
LDFLAGS += -Wl,-L.,--start-group,-lstm32,-lstm32usb

STRIPFLAGS  = -s -R.comment
SIZEFLAGS   = -A
#OBJCPFLAGS  = --rename-section .text=.data -S
OBJCPFLAGS  = -S

all: $(PROJECT).sym $(PROJECT).hex $(PROJECT).bin $(PROJECT).lss
	@echo "  SIZE    " $(PROJECT).elf
	@echo
	@$(SIZE) $(SIZEFLAGS) $(PROJECT).elf

%.o: %.c
	@echo "  CC      " $@
	@$(CC) -c -o $*.o $(CFLAGS) $<

%.o: %.s
	@echo "  AS      " $@
	@$(CC) -c -o $*.o $(ASFLAGS) $<

%.lss: %.elf
	@echo "  OBJDUMP " $@
	@$(OBJDUMP) -h -S -D $< > $@

%.sym: %.elf
	@echo "  SYM     " $@
	@$(NM) -n $< > $@

$(OBJS): $(wildcard src/*.h)

$(PROJECT).elf: $(OBJS) stm32.lds
	@echo "  LD      " $@
	@$(LD) $(LDFLAGS) -o $@ $(OBJS) -Tstm32.lds

$(PROJECT).bin: $(PROJECT).elf
	@echo "  STRIP   " $<
	@$(STRIP) $(STRIPFLAGS) $<
	@echo "  OBJCOPY " $@
	@$(OBJCOPY) $(OBJCPFLAGS) -O binary $< $@

stm32.lds: stm32.ld.in stm32f-rom.ld
	@echo "  LDS     " $@
	@sed \
	-e 's/ROM_OFFSET_SIZE/$(ROM_OFFSET_SIZE)/' \
	-e 's/FLASH_SIZE/$(FLASH_SIZE)/' \
	-e 's/SRAM_SIZE/$(SRAM_SIZE)/' \
	-e 's/EXT_RAM_SIZE/$(EXT_RAM_SIZE)/' \
	$< >$@

$(PROJECT).hex: $(PROJECT).elf
	@echo "  STRIP   " $<
	@$(STRIP) $(STRIPFLAGS) $<
	@echo "  IHEX    " $@
	@$(OBJCOPY) $(OBJCPFLAGS) -O ihex $< $@

clean:
	@echo "  CLEAN   " "."
	@rm -f $(OBJS) $(STM32_COMM_OBJS) $(PROJECT).elf $(PROJECT).bin $(PROJECT).hex $(PROJECT).lss $(PROJECT).map $(PROJECT).sym stm32.lds
