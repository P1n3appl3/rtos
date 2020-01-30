#include <stdbool.h>
#include <stdint.h>

// PF0 - Left button
// PF1 - Red LED
// PF2 - Blue LED
// PF3 - Green LED
// PF4 - Right button

// Initializes the LED and button pins on the launchpad
void launchpad_init(void);

#define RED_LED 2
#define BLUE_LED 4
#define GREEN_LED 8

bool left_switch(void);

bool right_switch(void);

// led: RED_LED, GREEN_LED, or BLUE_LED
void led_toggle(uint8_t led);

// led: RED_LED, GREEN_LED, or BLUE_LED
void led_write(uint8_t led, bool value);
