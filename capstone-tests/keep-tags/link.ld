ENTRY(_start)
SECTIONS
{
    . = 0x80200000;
    .text : { *(.text) }
    . = ALIGN(4096);
    .data : { *(.data) }
    . = ALIGN(4096);
    .bss : { *(.bss) }
}
