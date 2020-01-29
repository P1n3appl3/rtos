//*****************************************************************************
//
// Lab1.c - main program
//*****************************************************************************

// Jonathan W. Valvano 1/10/20, valvano@mail.utexas.edu
// EE445M/EE380L.6
// Simply put, develop a TM4C123 project with
// 1) an interpreter running via the UART link to the PC,
// 2) an LCD that has two logically separate displays implemented on one
// physical display, 3) a periodic interrupt that maintains time, and 4) an ADC
// device driver that collects data using a second periodic interrupt. There are
// a lot of specifications outlined in this lab, however, you are free to modify
// specifications as long as the above four components are implemented and
// understood.

// IR distance sensors
// J5/A3/PE3 analog channel 0  <- connect an IR distance sensor to J5 to get a
// realistic analog signal on PE3 J6/A2/PE2 analog channel 1 J7/A1/PE1 analog
// channel 2 J8/A0/PE0 analog channel 3

#include <ADC.h>
#include <IRDistance.h>
#include <Interpreter.h>
#include <LPF.h>
#include <OS.h>
#include <ST7735.h>
#include <launchpad.h>
#include <startup.h>
#include <stdint.h>
#include <timer.h>
#include <tivaware/hw_memmap.h>
#include <tivaware/rom.h>
#include <tivaware/sysctl.h>
#include <tivaware/timer.h>

// PE3 Ain3 sampled at 10Hz, sequencer 3, by DAS, using software start in ISR

int32_t ADCdata, FilterOutput, Distance;
uint32_t FilterWork;

// periodic task
void timer4a_handler(void) { // runs at 10Hz in background
  ROM_TimerIntClear(TIMER4_BASE, TIMER_A);
  led_toggle(RED_LED);
  ADCdata = ADC_In(); // channel set when calling ADC_Init
  led_toggle(RED_LED);
  FilterOutput = Median(ADCdata); // 3-wide median filter
  Distance = IRDistance_Convert(FilterOutput, 0);
  FilterWork++; // calculation finished
  led_toggle(RED_LED);
}

//*******************lab 1 main **********
int main(void) {
  ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                     SYSCTL_OSC_MAIN);
  ST7735_InitR(INITR_REDTAB); // LCD initialization
  launchpad_init();
  ADC_Init(3); // channel 3 is PE0 <- connect an IR distance sensor to J5 to get
               // a realistic analog signal
  periodic_timer_enable(4, 80000000 / 10, 1);
  OS_ClearMsTime(); // start a periodic interrupt to maintain time
  enable_interrupts();
  while (1) {
    Interpreter();
  }
}
