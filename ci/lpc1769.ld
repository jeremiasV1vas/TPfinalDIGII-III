/* Mapa de memoria del LPC1769, solo para compilar en el CI. */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x10000000, LENGTH = 32K
    AHBRAM (rwx): ORIGIN = 0x2007C000, LENGTH = 32K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    _etext = .;

    .data : AT(_etext)
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss (NOLOAD) :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Segundo banco de RAM, para buffers grandes (por ejemplo, las secuencias grabadas) */
    .ahbram (NOLOAD) :
    {
        *(.ahbram*)
    } > AHBRAM
}
