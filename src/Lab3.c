// LED outputs to logic analyzer for use by OS profile
// PF1 is preemptive thread switch
// PF2 is periodic task (DAS samples PE3)
// PC4 is PF4 button touch (SW1 task)

// IR distance sensors
// J5/A3/PE3 analog channel 0  <- connect an IR distance sensor to J5 to get a
// realistic analog signal on PE3 J6/A2/PE2 analog channel 1  <- connect an IR
// distance sensor to J6 to get a realistic analog signal on PE2 J7/A1/PE1
// analog channel 2 J8/A0/PE0 analog channel 3

// Button inputs
// PF4 is SW1 button input

// Analog inputs
// PE3 Ain0 sampled at 2kHz, sequencer 3, by DAS, using software start in ISR
// PE2 Ain1 sampled at 250Hz, sequencer 0, by Producer, timer tigger

#include "ADC.h"
#include "IRDistance.h"
#include "LPF.h"
#include "OS.h"
#include "ST7735.h"
#include "interpreter.h"
#include "interrupts.h"
#include "io.h"
#include "launchpad.h"
#include "printf.h"
#include "std.h"
#include "timer.h"
#include "tivaware/hw_types.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include <stdint.h>

void cr4_fft_64_stm32(void* pssOUT, void* pssIN, uint16_t Nbin);
short PID_stm32(int16_t Error, int16_t* Coeff);

uint32_t NumCreated;    // number of foreground threads created
uint32_t IdleCount;     // CPU idle counter
uint32_t PIDWork;       // current number of PID calculations finished
uint32_t FilterWork;    // number of digital filter calculations finished
uint32_t NumSamples;    // incremented every ADC sample, in Producer
#define SAMPLE_FREQ 400 // producer/consumer sampling
#define RUNLENGTH (20 * SAMPLE_FREQ) // 20-sec finite time experiment duration
int32_t x[64], y[64];                // input and output arrays for FFT

// Idle reference count for 10ms of completely idle CPU
// This should really be calibrated in a 10ms delay loop in OS_Init()
uint32_t IdleCountRef = 30769;
uint32_t CPUUtil; // calculated CPU utilization (in 0.01%)

uint32_t DataLost;        // data sent by Producer, but not received by Consumer
extern int32_t MaxJitter; // largest time jitter between interrupts
extern uint32_t JitterHistogram[128];

#define DEBUG_TOGGLE(X)                                                        \
    ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_##X,                            \
                     ~ROM_GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_##X))

void PortD_Init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, 0x0F);
}

// runs 2000 times/sec
// samples analog channel 0, PE3,
uint32_t DASoutput;
void DAS(void) {
    uint32_t input;
    if (NumSamples < RUNLENGTH) { // finite time run
        DEBUG_TOGGLE(0);
        input = ADC_in();
        DEBUG_TOGGLE(0);
        DASoutput = Filter(input);
        FilterWork++;
        DEBUG_TOGGLE(0);
    }
}

void ButtonWork(void) {
    DEBUG_TOGGLE(1);
    ST7735_Message_Num(1, 0, "NumCreated =", NumCreated);
    DEBUG_TOGGLE(1);
    OS_Sleep(ms(50));
    ST7735_Message_Num(1, 1, "CPUUtil 0.01%=", CPUUtil);
    ST7735_Message_Num(1, 2, "DataLost   =", DataLost);
    ST7735_Message_Num(1, 3, "Jitter (us)=", MaxJitter);
    DEBUG_TOGGLE(1);
}

void SW1Push(void) {
    if (OS_AddThread(ButtonWork, "Button work", 100, 2)) {
        NumCreated++;
    }
}

void SW2Push(void) {
    if (OS_AddThread(ButtonWork, "Button work", 100, 2)) {
        NumCreated++;
    }
}

// hardware timer-triggered ADC sampling at 400Hz
// every 64 samples, Consumer calculates FFT
// every 2.5ms*64 = 160 ms (6.25 Hz), consumer sends data to Display via mailbox
// Display thread updates LCD with measurement
void Producer(uint16_t data) {
    if (NumSamples < RUNLENGTH) { // finite time run
        NumSamples++;
        if (!OS_Fifo_Put(data)) { // send to consumer
            DataLost++;
        }
    }
}

// displays calculated results on the LCD
void Display(void) {
    uint32_t data, voltage, distance;
    ST7735_Message_Num(0, 1, "Run length = ", RUNLENGTH / SAMPLE_FREQ);
    while (NumSamples < RUNLENGTH) {
        data = OS_MailBox_Recv();
        voltage =
            3000 * data / 4095; // calibrate your device so voltage is in mV
        distance =
            IRDistance_Convert(data, 1); // you will calibrate this in Lab 6
        ST7735_Message_Num(0, 2, "v(mV) =", voltage);
        ST7735_Message_Num(0, 3, "d(mm) =", distance);
    }
}

// foreground thread, accepts data from producer
// calculates FFT, sends DC component to Display
void Consumer(void) {
    uint32_t data, DCcomponent; // 12-bit raw ADC sample, 0 to 4095
    uint32_t t;                 // time in 2.5 ms
    ADC_timer_init(1, 0, hz(SAMPLE_FREQ), 3, &Producer);
    NumCreated += OS_AddThread(Display, "Display", 128, 0);
    while (NumSamples < RUNLENGTH) {
        for (t = 0; t < 64; t++) { // collect 64 ADC samples
            data = OS_Fifo_Get();  // get from producer
            x[t] = data; // real part is 0 to 4095, imaginary part is 0
        }
        cr4_fft_64_stm32(y, x, 64); // complex FFT of last 64 ADC values
        DCcomponent =
            y[0] &
            0xFFFF; // Real part at frequency 0, imaginary part should be zero
        OS_MailBox_Send(DCcomponent); // called every 2.5ms*64 = 160ms
    }
}

// background thread, runs a PID controller every 1ms
int16_t IntTerm;   // accumulated error, RPM-sec
int16_t PrevError; // previous error, RPM
int16_t Actuator;
int16_t Coeff[3] = {
    384, // 1.5 = 384/256 proportional coefficient
    128, // 0.5 = 128/256 integral coefficient
    64   // 0.25 = 64/256 derivative coefficient*
};
void PID(void) {
    static int16_t err = -1000; // speed error, range -100 to 100 RPM
    Actuator = PID_stm32(err, Coeff) / 256;
    if (++err > 1000) {
        err = -1000;
    }
    PIDWork++;
}

// foreground task, runs when nothing else does to measure CPU utilization
void Idle(void) {
    while (NumSamples < RUNLENGTH) { IdleCount++; }
    // compute CPU utilization (in 0.01%)
    CPUUtil = 10000 - (5 * IdleCount) / IdleCountRef;
    while (1) { wait_for_interrupts(); }
}

void realmain(void) { // realmain
    OS_Init();
    PortD_Init();
    MaxJitter = 0;
    DataLost = 0; // lost data between producer and consumer
    IdleCount = 0;
    CPUUtil = 0;
    NumSamples = 0;
    FilterWork = 0;
    PIDWork = 0;

    OS_MailBox_Init();
    OS_Fifo_Init(64);
    ADC_init(0); // sequencer 3, channel 0, PE3, sampling in DAS()

    OS_AddSW1Task(&SW1Push, 2);
    OS_AddSW2Task(&SW2Push, 2);
    OS_AddPeriodicThread(&DAS, hz(2000), 1);
    OS_AddPeriodicThread(&PID, hz(1000), 2);

    NumCreated = 0;
    NumCreated += OS_AddThread(Consumer, "Consumer", 128, 1);
    NumCreated += OS_AddThread(interpreter, "Interpreter", 128, 2);
    NumCreated += OS_AddThread(Idle, "Idle", 128, 5);

    OS_Launch(ms(2));
}
