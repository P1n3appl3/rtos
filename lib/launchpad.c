#include "OS.h"
#include "timer.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_gpio.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/hw_types.h"
#include "tivaware/pin_map.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"

void launchpad_init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 1;
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, 0xE);
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, 0x11);
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, 0x11, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4,
                       GPIO_RISING_EDGE);
    HWREG(GPIO_PORTF_BASE + GPIO_O_IM) = 0x11;
    ROM_IntPrioritySet(INT_GPIOF, 3 << 5); // priority is high 3 bits
    ROM_IntEnable(INT_GPIOF);
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

void switch1_init(void (*task)(void), uint8_t priority) {
    sw1task = task;
}

void switch2_init(void (*task)(void), uint8_t priority) {
    sw2task = task;
}

const uint32_t debounce_ms = 50;
static uint32_t last_sw1;
static uint32_t last_sw2;

void gpio_portf_handler(void) {
    uint32_t now = to_ms(OS_Time());
    if (HWREG(GPIO_PORTF_BASE + GPIO_O_RIS) & 0x01) {
        if (sw1task && now - last_sw1 > debounce_ms) {
            sw1task();
        }
        last_sw1 = now;
    }
    if (HWREG(GPIO_PORTF_BASE + GPIO_O_RIS) & 0x10) {
        if (sw2task && now - last_sw2 > debounce_ms) {
            sw2task();
        }
        last_sw2 = now;
    }
    HWREG(GPIO_PORTF_BASE + GPIO_O_ICR) = 0x11;
}
