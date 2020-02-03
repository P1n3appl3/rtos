#include <stdint.h>

void disable_interrupts(void) {
    __asm("CPSID  I\n");
}

void enable_interrupts(void) {
    __asm("CPSIE  I\n");
}

void wait_for_interrupts(void) {
    __asm("WFI\n");
}

// TODO: check that these actually work
uint32_t start_critical(void) {
    __asm("MRS    R0, PRIMASK\n"
          "CPSID  I\n");
}

void end_critical(uint32_t x) {
    __asm("MSR    PRIMASK, R0\n");
}
