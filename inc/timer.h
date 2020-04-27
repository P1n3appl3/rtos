#pragma once

#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/sysctl.h"
#include <stdbool.h>
#include <stdint.h>

// Hertz to cycles
uint32_t hz(float hz);

// Cycles to hertz
float to_hz(uint32_t time);

// Microseconds to cycles
uint32_t us(uint32_t us);

// Cycles to microseconds
uint32_t to_us(uint32_t time);

// Milliseconds to cycles
uint32_t ms(float ms);

// Cycles to milliseconds
float to_ms(uint32_t time);

// Seconds to cycles
uint32_t seconds(float s);

// Cycles to seconds
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

typedef struct {
    uint32_t sysctl_periph;
    uint32_t interrupt;
    uint32_t base;
} TimerConfig;

const TimerConfig timers[12];
