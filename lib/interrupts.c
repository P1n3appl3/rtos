#include <stdint.h>

// TODO: check that these actually work
uint32_t __attribute__((naked)) start_critical(void) {
    __asm("MRS    R0, PRIMASK\n"
          "CPSID  I\n"
          "BX LR\n");
}

void __attribute__((naked)) end_critical(uint32_t x) {
    __asm("MSR    PRIMASK, R0\n"
          "BX LR\n");
}
