#pragma once
#include <stdint.h>

#define disable_interrupts() __asm("CPSID I")
#define enable_interrupts() __asm("CPSIE I")
#define wait_for_interrupts() __asm("WFI")

uint32_t start_critical_orig(void);
void end_critical_orig(uint32_t x);
