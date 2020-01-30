#include <stdint.h>

void disable_interrupts(void);
void enable_interrupts(void);
void wait_for_interrupts(void);
uint32_t start_critical(void);
void end_critical(uint32_t);
