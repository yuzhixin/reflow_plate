/*
 * Project-local GD32E230C8 linker script.
 * Last two 1 KiB flash pages are reserved for persisted configuration.
 */

ENTRY(Reset_Handler)

_estack = 0x20002000;
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

MEMORY
{
    RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 8K
    CCRAM (xrw)    : ORIGIN = 0x10000000, LENGTH = 0K
    FLASH (rx)     : ORIGIN = 0x08000000, LENGTH = 62K
}

__config_page_size = 0x400;
__config_page_a = ORIGIN(FLASH) + LENGTH(FLASH);
__config_page_b = __config_page_a + __config_page_size;
__config_flash_end = __config_page_b + __config_page_size;

ASSERT(__config_page_a == 0x0800F800, "config page A must be at 0x0800F800")
ASSERT(__config_page_b == 0x0800FC00, "config page B must be at 0x0800FC00")
ASSERT(__config_flash_end == 0x08010000, "config area must end at 64 KiB flash boundary")

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)

    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  ._user_heap_stack :
  {
    . = ALIGN(4);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(4);
  } >RAM

  .ccram_bss :
  {
    __ccram_start_bss__ = .;
    KEEP(*(.ccram_bss))
    KEEP(*(.ccram_bss*))
    . = ALIGN(4);
    __ccram_end_bss__ = .;
  } >CCRAM

  _si_ccram_data = LOADADDR(.ccram_data);
  .ccram_data :
  {
    . = ALIGN(4);
    _ccram_start_data = .;
    *(.ccram_bss)
    *(.ccram_bss*)

    . = ALIGN(4);
    _ccram_end_data = .;
  } >CCRAM AT> FLASH

  ASSERT(LOADADDR(.data) + SIZEOF(.data) <= __config_page_a, "firmware overlaps reserved config flash")

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
