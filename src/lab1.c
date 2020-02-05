#include "ADC.h"
#include "IRDistance.h"
#include "LPF.h"
#include "OS.h"
#include "ST7735.h"
#include "interpreter.h"
#include "interrupts.h"
#include "io.h"
#include "launchpad.h"
#include "timer.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tivaware/timer.h"
#include <stdint.h>
#include <stdnoreturn.h>

// PE3 Ain3 sampled at 10Hz, sequencer 3, by DAS, using software start in ISR

int32_t ADCdata, FilterOutput, Distance;
uint32_t FilterWork;
extern bool heartbeat_enabled;

// periodic task
void DAStask(void) { // runs at 10Hz in background
    if (heartbeat_enabled)
        led_toggle(RED_LED);
    ADCdata = ADC_in(); // channel set when calling ADC_init
    if (heartbeat_enabled)
        led_toggle(RED_LED);
    FilterOutput = Median(ADCdata); // 3-wide median filter
    Distance = IRDistance_Convert(FilterOutput, 0);
    FilterWork++; // calculation finished
    if (heartbeat_enabled)
        led_toggle(RED_LED);
}

int noreturn main(void) {
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);
    launchpad_init();
    uart_init();
    ST7735_InitR(INITR_REDTAB); // LCD init
    // connect an IR distance sensor to J5 to get a realistic analog signal
    ADC_init(3); // channel 3 is PE0
    periodic_timer_enable(4, ms(100), &DAStask, 1);
    OS_ClearMsTime(); // start a periodic interrupt to maintain time
    enable_interrupts();
    while (1) { interpreter(); }
}
