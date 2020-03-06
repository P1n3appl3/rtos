#pragma once
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/sysctl.h"
#include <stdbool.h>
#include <stdint.h>

// System time resolution is 1µs
#define SYSTEM_TIME_DIV 1

typedef uint32_t duration;

// Hertz to system units
#define hz(X) ((_Generic((X), int : hz_int, float : hz_float))(X))
duration hz_int(uint32_t hz);
duration hz_float(float hz);

// Microseconds to system units
duration us(uint32_t us);

// System units to microseconds
uint32_t to_us(duration time);

// Milliseconds to system units
duration ms(float ms);

// System units to milliseconds
uint32_t to_ms(duration time);

// Seconds to system units
duration seconds(float s);

// System units to seconds
uint32_t to_seconds(duration time);

// Enables periodic timer interrupts
// timer_num: 0-11 (6-11 use 64 bit timers)
// priority: 0-7 (0=highest)
void timer_enable(uint8_t timer_num, duration period, void (*task)(void),
                  uint8_t priority, bool periodic);

// Spin
void busy_wait(uint8_t timer_num, duration duration);

// Get the current value of a certain timer
duration get_timer_value(uint8_t timer_num);

// Get the reload value for a certain timer
duration get_timer_reload(uint8_t timer_num);

typedef struct {
    uint32_t sysctl_periph;
    uint32_t interrupt;
    uint32_t base;
} TimerConfig;

const TimerConfig timers[12];
