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
}

void enable_button_interupts(uint8_t priority) {
    ROM_GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4,
                       GPIO_RISING_EDGE);
    HWREG(GPIO_PORTF_BASE + GPIO_O_IM) = 0x11;
    ROM_IntPrioritySet(INT_GPIOF, priority << 5);
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
