ENTRY(main)

SECTIONS {
    .text : {
        *(.text*)
    }
    .rodata : {
        *(.rodata*)
    }
    .got : {
        *(.got*)
    }
    .data : {
        *(.data*)
    }
    .bss : {
        *(.bss*)
    }
}
