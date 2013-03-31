# serprog-stm32vcp:
## flashrom serprog programmer based on STM32F103 MCU & USB CDC protocol.

Perbuilt firmware binaries: http://dword1511.info/dword/serprog-stm32vcp/prebuilt/

Source archives: http://dword1511.info/dword/serprog-stm32vcp/

* * *
### Features
* Cheap and simple hardware:
  * a STM32F103 series MCU
  * a crystal, a 3.3V LDO
  * one or two LED(s)
  * some capacitors, resistors and wiring.
* Hardware full-duplex SPI with DMA, multiple clock speeds available:
  * 36MHz
  * 18MHz
  * 9MHz *(Default)*
  * 4.5MHz
  * 2.25MHz
  * 1.125MHz
  * 562.5kHz
  * 281.25kHz
* Hardware USB2.0 FullSpeed PHY with buffer, efficient virtual COM port emulation with USB CDC protocol.
* Fully functional status LED.
* flashrom serprog protocol.
* No UART device or settings needed, operates at **ANY** baud rates.
* Fully polled, double buffer operation, no glitches.
* Read speed up to 850KiB/s @ 36MHz SPI operation.
* Support 25 and 26 series SPI flash chips. 45 series is **NOT** supported by flashrom.
* Support LPC / FWH flash via parallel programming. **[WIP]**

* * *
### Advantages
* Fast:

  With ARM Cortex-M3 CPU @ 90DMIPS, 36MHz SPI engine, multi-channel DMA engine & on-chip USB 2.0 Full-Speed controller, serprog-STM32VCP is **MUCH** faster than many commercial CH341A-based USB programmers which are widely sold in China. Benchmark data can be found in the *PERFORMANCE* file.
* Convenient:

  Built-in Virtual COM Port emulation, no USB-to-UART bridge needed. Fully compatible with flashrom serprog protocol version 0x01.
* Stable:

  Main program is fully polled, not interrupt-driven. No physical UART means no baud rate and interference trouble. Natural 3.3V operation voltage.
* Affordable:

  *STM32F103C8T6* cost only ~$1.5 and its minimized develop board usually cost less than $6 in China (MCU, USB and status LED included).

*Ironically, you will still have to buy or borrow a USB-to-UART bridge (not RS-232 but TTL level) to program the programmer itself.*

* * *
### Installation
1. Download following software if you do not have them.
  * flashrom, of course

   ```bash
   svn co svn://flashrom.org/flashrom/trunk flashrom
   ```
  Or simply:

   ```bash
   sudo apt-get install flashrom
   ```
  * summon-arm-toolchain

   ```bash
   git clone git://github.com/esden/summon-arm-toolchain.git
   ```
   Edit summon-arm-toolchain, turn on LIBSTM32\_EN and turn off LIBOPENCM3\_EN, then run it.
   If you have already got an arm-none-eabi toolchain with libstm32, you may skip this.
  * stm32flash

   ```bash
   svn co http://stm32flash.googlecode.com/svn/trunk/ stm32flash
   make && sudo make install
   ```
1. Modify the Makefile if needed.

  Point TOOLCHAIN to the right path. If toolchain path is in the $PATH variable, simply leave the "arm-none-eabi-" string here.
1. Compile.

  Simply type:

   ```bash
   make
   ```
1. Program.

  Look at your STM32F103 board, find the BOOT0 jumper or ISP switch, put it into high or enabled, then connect you board's UART to your computer with the USB-to-UART bridge, apply  power to your board, then type:

   ```bash
   make program
   ```
1. Done!

  Throw the USB-to-UART bridge away and enjoy. Do not forget to pull BOOT0 low before resetting the board.

* * *
### Usage
1. To read a flash chip:
  * Connect an 25 type SPI Flash to your board according to the file SCHEMATICS.
  * Connect your board to your PC via USB.
  * Type:

   ```bash
   flashrom -p serprog:dev=/dev/ttyACM0:4000000 -r file-to-save.bin
   ```
  * Some times flashrom will ask you to choose a chip, add something like:

   ```bash
   -c SST25VF040B
   ```
   This is because sometimes multiple device with different timing requirements are only distinguished by the device code, however currently flashrom will not read it. Besides, some flash chips support more than one instruction sets.
1. To erase a flash chip:
  * Erase is automatically done when writing. However, if you simply want an empty chip, you will need to erase manually.
  * Type:

   ```bash
   flashrom -p serprog:dev=/dev/ttyACM0:4000000 -E
   ```
  * For certain chips like MX25L6445E, first pass could fail with old flashrom version, but if you do a second pass, all things will be alright. Seems like the first block needs more delay to be erased.
  * Flash status are verified to be empty automatically.
  * The whole process can take a few minutes.
1. To write a flash chip:
  * Type:

   ```bash
   flashrom -p serprog:dev=/dev/ttyACM0:4000000 -w file-to-load.bin
   ```
  * Flash chips are checked and blocks that are not empty are automatically erased.
  * Images are verified after writing automatically.
  * The whole process can take a few minutes.

* * *
### About the LED
* [READY] on, [BUSY] off:

  Device is idle.
* [BUSY] on, [READY] off:

  Waiting for USB to be configured.
  If this happens during read/rease/write procedure and hangs for too long, it is possible that a firmware bug occured. Please report to me, better with the output of the *strace* tool.
* [BUSY] flashes:

  Reading flash.
* [BUSY] and [READY] alternating:

  Erasing flash.
* [BUSY] and [READY] on:

  Writing (actually alternating fast but you cannot see it :) ).

* * *
### Problems?
1. Check your wirings and flashrom version. Do not forget to power the flash chip itself.
1. If you are sure it is caused by something wrong in the programmer's firmware, please file a ticket.
