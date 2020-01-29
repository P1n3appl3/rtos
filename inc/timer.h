#include <stdint.h>

// Microseconds to cycles
uint32_t us(uint32_t us);

// Milliseconds to cycles
uint32_t ms(uint32_t ms);

// Seconds to cycles
uint32_t seconds(float s);

// Enables periodic timer interrupts
void periodic_timer_enable(uint8_t timer_num, uint32_t period);
