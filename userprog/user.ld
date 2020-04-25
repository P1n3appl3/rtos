ENTRY(main)

PHDRS { code PT_LOAD; data PT_LOAD; }

SECTIONS {
    .text : {
        *(.text*)
    } : code
    .got : {
    } : data
    .data : {
/* address of GOT's start */
        LONG(.)
        *(.got*)
/* infinite scream to delimit GOT */
        LONG(0xAAAAAAAA)
/* unfortunately ro and rw data have to be combined due to -mno-PDITR */
        *(.rodata*)
        *(.data*)
    } : data
    .bss : {
        *(.bss*)
    } : data
}
