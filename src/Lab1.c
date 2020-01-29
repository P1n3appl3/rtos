#include "ADC.h"
#include "IRDistance.h"
#include "Interpreter.h"
#include "LPF.h"
#include "OS.h"
#include "ST7735.h"
#include "launchpad.h"
#include "startup.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tivaware/timer.h"
#include <stdint.h>
#include <timer.h>

// PE3 Ain3 sampled at 10Hz, sequencer 3, by DAS, using software start in ISR

int32_t ADCdata, FilterOutput, Distance;
uint32_t FilterWork;

// periodic task
void DAStask(void) { // runs at 10Hz in background
    ROM_TimerIntClear(TIMER4_BASE, TIMER_A);
    led_toggle(RED_LED);
    ADCdata = ADC_In(); // channel set when calling ADC_Init
    led_toggle(RED_LED);
    FilterOutput = Median(ADCdata); // 3-wide median filter
    Distance = IRDistance_Convert(FilterOutput, 0);
    FilterWork++; // calculation finished
    led_toggle(RED_LED);
}

int main(void) {
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);
    ST7735_InitR(INITR_REDTAB); // LCD initialization
    launchpad_init();
    ADC_Init(3); // channel 3 is PE0 <- connect an IR distance sensor to J5 to
                 // get a realistic analog signal
    periodic_timer_enable(4, 80000000 / 10, &DAStask, 1);
    OS_ClearMsTime(); // start a periodic interrupt to maintain time
    enable_interrupts();
    while (1) { Interpreter(); }
}
