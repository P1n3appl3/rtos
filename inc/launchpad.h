#include <stdbool.h>
#include <stdint.h>

// PF0 - Left button
// PF1 - Red LED
// PF2 - Blue LED
// PF3 - Green LED
// PF4 - Right button
// PA0 - USB UART RX
// PA1 - USB UART TX

// Initializes the LED and button pins on the launchpad, as well as
// UART <-> USB through the debugger.
void launchpad_init(void);

#define RED_LED 2
#define BLUE_LED 4
#define GREEN_LED 8

bool left_switch(void);

bool right_switch(void);

void led_toggle(uint8_t led);

void led_write(uint8_t led, bool value);
