#pragma once

#include "tivaware/gpio.h"
#include <stdbool.h>
#include <stdint.h>

// PF0 - Left button
// PF1 - Red LED
// PF2 - Blue LED
// PF3 - Green LED
// PF4 - Right button

// Initializes the LED and button pins on the launchpad
// and enables portf interrupts for the buttons
void launchpad_init(void);

void enable_button_interupts(uint8_t priority);

#define RED_LED GPIO_PIN_1
#define BLUE_LED GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

bool left_switch(void);

bool right_switch(void);

// led: RED_LED, GREEN_LED, or BLUE_LED
void led_toggle(uint8_t led);

// led: RED_LED, GREEN_LED, or BLUE_LED
void led_write(uint8_t led, bool value);
