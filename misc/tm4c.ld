ENTRY(reset_handler)

MEMORY {
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 32K
}

SECTIONS {
    .text : {
        KEEP(*(.interrupt_table))
        _text = .;
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH

    .ARM.exidx : {} > FLASH

    .data : AT(_etext) {
        _data = .;
        *(.data*)
        _edata = .;
    } > RAM

    .bss : {
        __bss_start__ = .;
        _bss = .;
        *(.bss*)
        _ebss = .;
        __bss_end__ = .;
    } > RAM

    end = .;

    /DISCARD/ : { *(.ARM.exidx*) }
}
