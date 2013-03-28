/* Section Definitions */
SECTIONS
{
   .text :
  {
    KEEP(*(.isr_vector .isr_vector.*))
    *(.text .text.*)
    *(.gnu.linkonce.t.*)
    *(.glue_7)
    *(.glue_7t)
    *(.gcc_except_table)
    *(.rodata .rodata*)
    *(.gnu.linkonce.r.*)
    _etext = .;
  } > FLASH

  .data : AT (_etext)
  {
    _data = .;
    *(vtable vtable.*)
    *(.data .data.*)
    *(.gnu.linkonce.d*)
    . = ALIGN(4);
    _edata = . ;
  } > SRAM

  /* .bss section which is used for uninitialized data */
  .bss (NOLOAD) :
  {
    _bss = . ;
    *(.bss .bss.*)
    *(.gnu.linkonce.b*)
    *(COMMON)
        . = ALIGN(4);
        _ebss = . ;
  } > SRAM

  .stackarea (NOLOAD) :
  {
    . = ALIGN(8);
        *(.stackarea .stackarea.*)
        . = ALIGN(8);
  } > SRAM

  . = ALIGN(4);
  _end = . ;
  PROVIDE (end = .);
  ._usrstack :
  {
    . = ALIGN(4);
    _susrstack = . ;
    . = . + _Minimum_Stack_Size ;
    . = ALIGN(4);
    _eusrstack = . ;
  } > SRAM

  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment       0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
}
