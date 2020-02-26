#include <stdbool.h>
#include <stdint.h>

// Microseconds to cycles
uint32_t us(uint32_t us);

// Milliseconds to cycles
uint32_t ms(uint32_t ms);

// Seconds to cycles
uint32_t seconds(float s);

// Enables periodic timer interrupts
// timer_num: 0-5 for 32 bit (53 second max) and 6-11 for 64 bit
// priority: 0-7 (0=highest)
void timer_enable(uint8_t timer_num, uint32_t period, void (*task)(void),
                  uint8_t priority, bool periodic);

// TODO: docs
void busy_wait(uint8_t timer_num, uint32_t duration);

// Get the current value of a certain timer
uint32_t get_timer_value(uint8_t timer_num);

// Get the reload value for a certain timer
uint32_t get_timer_reload(uint8_t timer_num);
