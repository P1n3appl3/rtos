#include "timer.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/rom.h"

volatile static unsigned long sw1last; // previous
static void (*sw1task)(void);

volatile static unsigned long sw2last; // previous
static void (*sw2task)(void);

#define SWITCH_PINS (GPIO_INT_PIN_0 | GPIO_INT_PIN_4)
#define PF0 (*((volatile uint32_t*)0x40025004))
#define PF4 (*((volatile uint32_t*)0x40025040))

void gpio_arm(void) {
    sw1last = PF0;
    sw2last = PF4;
    GPIOIntEnable(GPIO_PORTF_BASE, SWITCH_PINS);
}

static uint8_t swpriority = 4;
void timer_arm(void) {
    timer_enable(3, ms(10), &gpio_arm, swpriority, false);
}

void switch1_init(void (*task)(void), uint8_t priority) {
    ROM_IntPrioritySet(INT_GPIOF, priority);
    sw1last = PF0; // initial button state
    sw1task = task;
    swpriority = priority;
    gpio_arm();
}

void switch2_init(void (*task)(void), uint8_t priority) {
    ROM_IntPrioritySet(INT_GPIOF, priority);
    sw2last = PF4; // initial button state
    sw2task = task;
    swpriority = priority;
    gpio_arm();
}

void gpio_portf_handler(void) {
    uint32_t status = GPIOIntStatus(GPIO_PORTF_BASE, false);
    GPIOIntClear(GPIO_PORTF_BASE, SWITCH_PINS);
    if (status & GPIO_INT_PIN_0) {
        if (sw1last) {
            sw1task();
        }
    }
    if (status & GPIO_INT_PIN_4) {
        if (sw2last) {
            sw2task();
        }
    }
    timer_arm();
}
