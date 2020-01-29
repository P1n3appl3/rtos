#include <stdint.h>

// Microseconds to cycles
uint32_t us(uint32_t us);

// Milliseconds to cycles
uint32_t ms(uint32_t ms);

// Seconds to cycles
uint32_t seconds(float s);

// Enables periodic timer interrupts
// Priority is 0-7 where 0 is highest priority
void periodic_timer_enable(uint8_t timer_num, uint32_t period,
                           void (*task)(void), uint8_t priority);
