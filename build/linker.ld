ENTRY (loader)
SECTIONS
{
  . = 0x100000;
  kernel_start = .;

  .mbheader :
  {
    *(.mbheader)
  }

  .text :
  {
    *(.text)
  }

  .rodata ALIGN (0x1000) :
  {
    *(.rodata)
  }

  .data ALIGN (0x1000) :
  {
    *(.data)
  }

  .bss : {
    sbss = .;
    *(COMMON)
    *(.bss)
    ebss = .;
  }
  kernel_end = .;
}