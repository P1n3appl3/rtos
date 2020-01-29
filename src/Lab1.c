#include "ADC.h"
#include "IRDistance.h"
#include "Interpreter.h"
#include "LPF.h"
#include "OS.h"
#include "ST7735.h"
#include "launchpad.h"
#include "timer.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tivaware/timer.h"
#include <stdint.h>

// PE3 Ain3 sampled at 10Hz, sequencer 3, by DAS, using software start in ISR

int32_t ADCdata, FilterOutput, Distance;
uint32_t FilterWork;

// periodic task
void DAStask(void) { // runs at 10Hz in background
    PF1 ^= 0x01;
    ADCdata = ADC_In(); // channel set when calling ADC_Init
    PF1 ^= 0x01;
    FilterOutput = Median(ADCdata); // 3-wide median filter
    Distance = IRDistance_Convert(FilterOutput, 0);
    FilterWork++; // calculation finished
    PF1 ^= 0x01;
}

int main(void) {
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);
    ST7735_InitR(INITR_REDTAB); // LCD initialization
    launchpad_init();
    ADC_Init(0); // channel 3 is PE0 <- connect an IR distance sensor to J5 to
                 // get a realistic analog signal
    Timer4A_Init(&DAStask, 80000000 / 10, 1); // 10 Hz sampling, priority=1
    timer_init(TIMER4_BASE, SYSCTL_PERIPH_TIMER4, INT_TIMER4A, 80000000 / 10);
    OS_ClearMsTime(); // start a periodic interrupt to maintain time
    EnableInterrupts();
    while (1) { Interpreter(); }
}
