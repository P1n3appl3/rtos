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

void cr4_fft_64_stm32(void* pssOUT, void* pssIN, unsigned short Nbin);
short PID_stm32(short Error, short* Coeff);

uint32_t NumCreated;    // number of foreground threads created
uint32_t PIDWork;       // current number of PID calculations finished
uint32_t FilterWork;    // number of digital filter calculations finished
uint32_t NumSamples;    // incremented every ADC sample, in Producer
#define SAMPLE_FREQ 400 // producer/consumer sampling
#define RUNLENGTH (20 * SAMPLE_FREQ) // 20-sec finite time experiment duration

int32_t x[64], y[64]; // input and output arrays for FFT

//---------------------User debugging-----------------------
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

//------------------Task 1--------------------------------
// 2 kHz sampling ADC channel 0, using software start trigger
// background thread executed at 2 kHz
// 60-Hz notch high-Q, IIR filter, assuming fs=2000 Hz
// y(n) = (256x(n) -503x(n-1) + 256x(n-2) + 498y(n-1)-251y(n-2))/256 (2k
// sampling)

// background thread, calculates 60Hz notch filter
// runs 2000 times/sec
// samples analog channel 0, PE3,
uint32_t DASoutput;
#define DAS_PERIOD us(500)
void DAS(void) {
    uint32_t input;
    unsigned static long LastTime; // time at previous ADC sample
    uint32_t thisTime;             // time at current ADC sample
    long jitter;                   // time between measured and expected, in us
    if (NumSamples < RUNLENGTH) {  // finite time run
        DEBUG_TOGGLE(0);
        input = ADC_in(); // channel set when calling ADC_Init
        DEBUG_TOGGLE(0);
        thisTime = OS_Time();
        DASoutput = Filter(input);
        if (FilterWork++ > 0) { // ignore timing of first interrupt
            uint32_t diff = OS_TimeDifference(LastTime, thisTime);
            jitter = abs((int32_t)diff - (int32_t)DAS_PERIOD);
            if (jitter > MaxJitter) {
                MaxJitter = jitter;
            }
            if (jitter >=
                sizeof(JitterHistogram) / sizeof(JitterHistogram[0])) {
                jitter =
                    sizeof(JitterHistogram) / sizeof(JitterHistogram[0]) - 1;
            }
            JitterHistogram[jitter]++;
        }
        LastTime = thisTime;
        DEBUG_TOGGLE(0);
    }
}

//------------------Task 2--------------------------------
// background thread executes with SW1 button
// one foreground task created with button push
// foreground treads run for 2 sec and die

void ButtonWork(void) {
    // uint32_t myId = OS_Id();
    DEBUG_TOGGLE(1);
    ST7735_Message_Num(1, 0, "NumCreated =", NumCreated);
    DEBUG_TOGGLE(1);
    OS_Sleep(ms(50));
    ST7735_Message_Num(1, 1, "PIDWork    =", PIDWork);
    ST7735_Message_Num(1, 2, "DataLost   =", DataLost);
    ST7735_Message_Num(1, 3, "Jitter (c)=", MaxJitter);
    DEBUG_TOGGLE(1);
    OS_Kill(); // done, OS does not return from a Kill
}

//************SW1Push*************
// Called when SW1 Button pushed
// Adds another foreground task
// background threads execute once and return
void SW1Push(void) {
    if (OS_Time() > ms(20)) { // debounce
        if (OS_AddThread(&ButtonWork, 100, 0)) {
            NumCreated++;
        }
        OS_ClearTime(); // at least 20ms between touches
    }
}

//------------------Task 3--------------------------------
// hardware timer-triggered ADC sampling at 400Hz
// Producer runs as part of ADC ISR
// Producer uses fifo to transmit 400 samples/sec to Consumer
// every 64 samples, Consumer calculates FFT
// every 2.5ms*64 = 160 ms (6.25 Hz), consumer sends data to Display via mailbox
// Display thread updates LCD with measurement

//******** Producer ***************
// The Producer in this lab will be called from an ADC ISR
// A timer runs at 400Hz, started through the provided ADCT0ATrigger.c driver
// The timer triggers the ADC, creating the 400Hz sampling
// The ADC ISR runs when ADC data is ready
// The ADC ISR calls this function with a 12-bit sample
// sends data to the consumer, runs periodically at 400Hz
void Producer(uint16_t data) {
    // DEBUG_TOGGLE(3);
    if (NumSamples < RUNLENGTH) {     // finite time run
        NumSamples++;                 // number of samples
        if (OS_Fifo_Put(data) == 0) { // send to consumer
            DataLost++;
        }
    }
}

// foreground thread, accepts data from consumer
// displays calculated results on the LCD
void Display(void) {
    uint32_t data, voltage, distance;
    // uint32_t myId = OS_Id();
    ST7735_Message_Num(0, 1, "Run length = ",
                       (RUNLENGTH) / SAMPLE_FREQ); // top half used for Display
    while (NumSamples < RUNLENGTH) {
        data = OS_MailBox_Recv();
        voltage =
            3000 * data / 4095; // calibrate your device so voltage is in mV
        distance =
            IRDistance_Convert(data, 1); // you will calibrate this in Lab 6
        ST7735_Message_Num(0, 2, "v(mV) =", voltage);
        ST7735_Message_Num(0, 3, "d(mm) =", distance);
    }
    OS_Kill(); // done
}

// foreground thread, accepts data from producer
// calculates FFT, sends DC component to Display
void Display(void);
void Consumer(void) {
    ROM_GPIOPinWrite(GPIO_PORTD_BASE, 1, 1);
    uint32_t data, DCcomponent; // 12-bit raw ADC sample, 0 to 4095
    uint32_t t;                 // time in 2.5 ms
    ADC_timer_init(1, 0, hz(SAMPLE_FREQ), 3, &Producer);
    NumCreated += OS_AddThread(&Display, 128, 0);
    while (NumSamples < RUNLENGTH) {
        DEBUG_TOGGLE(2);
        for (t = 0; t < 64; t++) { // collect 64 ADC samples
            data = OS_Fifo_Get();  // get from producer
            x[t] = data; // real part is 0 to 4095, imaginary part is 0
        }
        DEBUG_TOGGLE(2);
        cr4_fft_64_stm32(y, x, 64); // complex FFT of last 64 ADC values
        DCcomponent =
            y[0] &
            0xFFFF; // Real part at frequency 0, imaginary part should be zero
        OS_MailBox_Send(DCcomponent); // called every 2.5ms*64 = 160ms
    }
    OS_Kill(); // done
}

//------------------Task 4--------------------------------
// foreground thread that runs without waiting or sleeping
// it executes a digital controller

//******** PID ***************
// foreground thread, runs a PID controller
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none

short IntTerm;   // accumulated error, RPM-sec
short PrevError; // previous error, RPM
short Coeff[3];  // PID coefficients
short Actuator;
void PID(void) {
    short err; // speed error, range -100 to 100 RPM
    PIDWork = 0;
    IntTerm = 0;
    PrevError = 0;
    Coeff[0] = 384; // 1.5 = 384/256 proportional coefficient
    Coeff[1] = 128; // 0.5 = 128/256 integral coefficient
    Coeff[2] = 64;  // 0.25 = 64/256 derivative coefficient*
    while (NumSamples < RUNLENGTH) {
        for (err = -1000; err <= 1000; err++) { // made-up data
            Actuator = PID_stm32(err, Coeff) / 256;
        }
        PIDWork++; // calculation finished
    }
    for (;;) {} // done
}

//------------------Task 5--------------------------------
// UART background ISR performs serial input/output
// Two software fifos are used to pass I/O data to foreground
// The interpreter runs as a foreground thread
// The UART driver should call OS_Wait(&RxDataAvailable) when foreground tries
// to receive The UART ISR should call OS_Signal(&RxDataAvailable) when it
// receives data from Rx Similarly, the transmit channel waits on a semaphore in
// the foreground and the UART ISR signals this semaphore (TxRoomLeft) when
// getting data from fifo

// Modify your intepreter from Lab 1, adding commands to help debug
// Add the following commands, leave other commands, if they make sense
// 1) print performance measures
//    time-jitter, number of data points lost, number of calculations performed
//    i.e., NumSamples, NumCreated, MaxJitter, DataLost, FilterWork, PIDwork

// 2) print debugging parameters
//    i.e., x[], y[]

void main(void) { // realmain
    OS_Init();     // initialize, disable interrupts
    PortD_Init();  // debugging profile
    MaxJitter = 0; // in 1us units
    DataLost = 0;  // lost data between producer and consumer
    NumSamples = 0;
    FilterWork = 0;

    OS_MailBox_Init();
    OS_Fifo_Init(64); // NOTE: 4 is not big enough

    // hardware init
    ADC_init(0); // sequencer 3, channel 0, PE3, sampling in DAS()

    // attach background tasks
    OS_AddSW1Task(&SW1Push, 2);
    // 2 kHz real time sampling of PE3
    OS_AddPeriodicThread(&DAS, DAS_PERIOD, 1);

    // create initial foreground threads
    NumCreated = 0;
    NumCreated += OS_AddThread(&Consumer, 128, 0);
    NumCreated += OS_AddThread(&interpreter, 128, 0);
    NumCreated += OS_AddThread(&PID, 128, 0);

    OS_Launch(ms(2));
}
