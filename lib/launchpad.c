#include "tivaware/gpio.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/pin_map.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tm4c123gh6pm.h"

void launchpad_init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIO_PORTF_LOCK_R = 0x4C4F434B; // unlock GPIO Port F
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, 0xE);
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, 0x11);
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, 0x11, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
}

bool left_switch(void) {
    return !ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
}

bool right_switch(void) {
    return !ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0);
}

void led_toggle(uint8_t led) {
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, led,
                     ~ROM_GPIOPinRead(GPIO_PORTF_BASE, led));
}

void led_write(uint8_t led, bool value) {
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, led, value ? led : 0);
}
