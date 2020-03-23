#pragma once
#include "tivaware/gpio.h"
#include "tivaware/hw_gpio.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/sysctl.h"
#include <stdbool.h>
#include <stdint.h>

// Channel -> Pin mapping:
// 0: PE3
// 1: PE2
// 2: PE1
// 3: PE0
// 4: PD3
// 5: PD2
// 6: PD1
// 7: PD0
// 8: PE4
// 9: PE5
// 10: PC6
// 11: PC7

// sets up a channel (0 to 11) to be software triggered for single samples using
// adc 0 sequencer 3
bool adc_init(uint8_t channel_num);

// software trigger adc 0 sequence 3 and return 12 bit result
uint16_t adc_in(void);

// uses adc 1 sequence 0 with the specified channel and timer to provide samples
// at a consistent rate
bool adc_timer_init(uint8_t channel_num, uint8_t timer_num, uint32_t period,
                    uint8_t priority, void (*task)(uint16_t));

typedef struct {
    uint32_t port;
    uint32_t port_base;
    uint8_t pin;
} ADCConfig;

const ADCConfig adcs[12];
