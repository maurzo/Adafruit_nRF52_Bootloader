ENTRY(Reset_Handler)

MEMORY
{
    /* Preserve S140 7.3.0, bootloader, application data and bootloader settings. */
    FLASH (rx) : ORIGIN = 0x27000, LENGTH = 0xEA000 - 0x27000
    /* Avoid the bootloader's retained/double-reset memory in lower RAM. */
    RAM (rwx) : ORIGIN = 0x20008000, LENGTH = 0x38000
}

__StackTop = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
    .text : { *(.text*) *(.rodata*) . = ALIGN(4); } > FLASH
    .ARM.exidx : { *(.ARM.exidx*) } > FLASH
    .data :
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > FLASH
    __data_load__ = LOADADDR(.data);
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*) *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM
    /DISCARD/ : { *(.comment) *(.note*) }
}

ASSERT(__bss_end__ + 0x400 <= __StackTop, "Insufficient stack space")
