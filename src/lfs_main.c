#include "OS.h"
#include "eDisk.h"
#include "interpreter.h"
#include "littlefs.h"
#include "printf.h"
#include "tivaware/rom.h"

void PortD_Init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, 0x0F);
}

uint32_t IdleCount = 0;
void Idle(void) {
    while (1) {
        IdleCount++; // CPU utilization
    }
}

void debug_main(void) {
    OS_Init();
    PortD_Init();

    OS_AddPeriodicThread(&disk_timerproc, ms(1), 0);
    // OS_AddThread(&debug_test, "debug_test", 256, 0);
    OS_AddThread(interpreter, "Interpreter", 256, 0);
    OS_AddThread(&Idle, "Idle", 128, 3);
    OS_Launch(ms(10));
}

//*******************Trampoline for selecting main to execute**********
int main(void) { // main
    debug_main();
    return 0;
}
