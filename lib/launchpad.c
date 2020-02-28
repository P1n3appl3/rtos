#include "timer.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/pin_map.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tm4c123gh6pm.h"

void launchpad_init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, 0xE);
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, 0x11);
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, 0x11, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
    ROM_IntPrioritySet(INT_GPIOF, 2);
    ROM_IntEnable(INT_GPIOF);
    ROM_GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4,
                       GPIO_RISING_EDGE);
}

void led_toggle(uint8_t led) {
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, led,
                     ~ROM_GPIOPinRead(GPIO_PORTF_BASE, led));
}

void led_write(uint8_t led, bool value) {
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, led, value ? led : 0);
}

bool left_switch(void) {
    return !ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
}

bool right_switch(void) {
    return !ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0);
}

static void (*sw1task)(void);
static void (*sw2task)(void);

#define SWITCH_PINS (GPIO_INT_PIN_0 | GPIO_INT_PIN_4)
#define PF0 (*((volatile uint32_t*)0x40025004))
#define PF4 (*((volatile uint32_t*)0x40025040))

void switch1_init(void (*task)(void), uint8_t priority) {
    sw1task = task;
}

void switch2_init(void (*task)(void), uint8_t priority) {
    sw2task = task;
}

void gpio_portf_handler(void) {
    ROM_IntPendClear(INT_GPIOF);
    if (GPIO_PORTF_RIS_R & 0x01) {
        if (sw1task) {
            sw1task();
        }
    }
    if (GPIO_PORTF_RIS_R & 0x08) {
        if (sw2task) {
            sw2task();
        }
    }
}
