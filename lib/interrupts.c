#include <stdint.h>

uint32_t __attribute__((naked)) start_critical(void) {
    __asm("MRS   R0, PRIMASK");
    __asm("CPSID I");
    __asm("BX    LR");
}

void __attribute__((naked)) end_critical(uint32_t x) {
    __asm("MSR PRIMASK, R0");
    __asm("BX  LR");
}
