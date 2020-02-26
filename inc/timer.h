#include <stdbool.h>
#include <stdint.h>

// System time resolution is 1µs
#define SYSTEM_TIME_DIV 80

// Microseconds to system units
uint32_t us(uint32_t us);

// System units to microseconds
uint32_t to_us(uint32_t time);

// Milliseconds to system units
uint32_t ms(float ms);

// System units to microseconds
float to_ms(uint32_t time);

// Seconds to system units
uint32_t seconds(float s);

// System units to microseconds
float to_seconds(uint32_t time);

// Enables periodic timer interrupts
// timer_num: 0-11 (6-11 use 64 bit timers)
// priority: 0-7 (0=highest)
void timer_enable(uint8_t timer_num, uint32_t period, void (*task)(void),
                  uint8_t priority, bool periodic);

// Spin
void busy_wait(uint8_t timer_num, uint32_t duration);

// Get the current value of a certain timer
uint32_t get_timer_value(uint8_t timer_num);

// Get the reload value for a certain timer
uint32_t get_timer_reload(uint8_t timer_num);
