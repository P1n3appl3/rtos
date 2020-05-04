#include "OS.h"
#include "heap.h"
#include "mouse.h"
#include "printf.h"
#include "tivaware/rom.h"

void PortD_Init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, 0x0F);
}

int main(void) {
    OS_Init();
    PortD_Init();
    heap_init();

    OS_AddThread(mouse_init, "usb mouse", 1024, 0);

    OS_Launch(ms(2));
    return 0;
}
