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
extern uint32_t const JitterSize;
extern uint32_t JitterHistogram[];

#define PD0 HWREGBITB(GPIO_PORTD_BASE + GPIO_O_DATA, 0)
#define PD1 HWREGBITB(GPIO_PORTD_BASE + GPIO_O_DATA, 1)
#define PD2 HWREGBITB(GPIO_PORTD_BASE + GPIO_O_DATA, 2)
#define PD3 HWREGBITB(GPIO_PORTD_BASE + GPIO_O_DATA, 3)

void PortD_Init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, 0x0F);
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
        PD0 ^= 0x01;
        input = ADC_in(); // channel set when calling ADC_Init
        PD0 ^= 0x01;
        thisTime = OS_Time();
        DASoutput = Filter(input);
        FilterWork++;         // calculation finished
        if (FilterWork > 1) { // ignore timing of first interrupt
            uint32_t diff = OS_TimeDifference(LastTime, thisTime);
            jitter = abs((int32_t)diff - (int32_t)DAS_PERIOD);
            if (jitter > MaxJitter) {
                MaxJitter = jitter;
            }
            if (jitter >= JitterSize) {
                jitter = JitterSize - 1;
            }
            JitterHistogram[jitter]++;
        }
        LastTime = thisTime;
        PD0 ^= 0x01;
    }
}

//------------------Task 2--------------------------------
// background thread executes with SW1 button
// one foreground task created with button push
// foreground treads run for 2 sec and die

void ButtonWork(void) {
    // uint32_t myId = OS_Id();
    PD1 ^= 0x02;
    ST7735_Message_Num(1, 0, "NumCreated =", NumCreated);
    PD1 ^= 0x02;
    OS_Sleep(ms(50));
    ST7735_Message_Num(1, 1, "PIDWork     =", PIDWork);
    ST7735_Message_Num(1, 2, "DataLost    =", DataLost);
    ST7735_Message_Num(1, 3, "Jitter 0.1us=", MaxJitter);
    PD1 ^= 0x02;
    OS_Kill(); // done, OS does not return from a Kill
}

//************SW1Push*************
// Called when SW1 Button pushed
// Adds another foreground task
// background threads execute once and return
void SW1Push(void) {
    if (ms(OS_Time()) > 20) { // debounce
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
        PD3 = 0x08;
        ST7735_Message_Num(0, 2, "v(mV) =", voltage);
        ST7735_Message_Num(0, 3, "d(mm) =", distance);
        PD3 = 0x00;
    }
    OS_Kill(); // done
}

// foreground thread, accepts data from producer
// calculates FFT, sends DC component to Display
void Display(void);
void Consumer(void) {
    uint32_t data, DCcomponent; // 12-bit raw ADC sample, 0 to 4095
    uint32_t t;                 // time in 2.5 ms
    ADC_timer_init(1, 2, hz(SAMPLE_FREQ), 3, &Producer);
    NumCreated += OS_AddThread(&Display, 128, 0);
    while (NumSamples < RUNLENGTH) {
        PD2 = 0x04;
        for (t = 0; t < 64; t++) { // collect 64 ADC samples
            data = OS_Fifo_Get();  // get from producer
            x[t] = data; // real part is 0 to 4095, imaginary part is 0
        }
        PD2 = 0x00;
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

void realmain(void) { // realmain
    OS_Init();        // initialize, disable interrupts
    PortD_Init();     // debugging profile
    MaxJitter = 0;    // in 1us units
    DataLost = 0;     // lost data between producer and consumer
    NumSamples = 0;
    FilterWork = 0;

    // initialize communication channels
    OS_MailBox_Init();
    OS_Fifo_Init(64); // NOTE: 4 is not big enough

    // hardware init
    ADC_init(0); // sequencer 3, channel 0, PE3, sampling in DAS()

    // attach background tasks
    OS_AddSW1Task(&SW1Push, 2);
    OS_AddPeriodicThread(&DAS, DAS_PERIOD,
                         1); // 2 kHz real time sampling of PE3

    // create initial foreground threads
    NumCreated = 0;
    NumCreated += OS_AddThread(&Consumer, 128, 0);
    NumCreated += OS_AddThread(&interpreter, 128, 0);
    NumCreated += OS_AddThread(&PID, 128, 0);

    OS_Launch(ms(2));
}

//+++++++++++++++++++++++++DEBUGGING CODE++++++++++++++++++++++++
// ONCE YOUR RTOS WORKS YOU CAN COMMENT OUT THE REMAINING CODE
//
//*******************Initial TEST**********
// This is the simplest configuration, test this first, (Lab 2 part 1)
// run this with
// no UART interrupts
// no SYSTICK interrupts
// no timer interrupts
// no switch interrupts
// no ADC serial port or LCD output
// no calls to semaphores
uint32_t Count1; // number of times thread1 loops
uint32_t Count2; // number of times thread2 loops
uint32_t Count3; // number of times thread3 loops
uint32_t Count4; // number of times thread4 loops
uint32_t Count5; // number of times thread5 loops
void Thread1(void) {
    Count1 = 0;
    for (;;) {
        PD0 ^= 0x01; // heartbeat
        Count1++;
        // led_toggle(BLUE_LED);
        // busy_wait(4, ms(500));
        OS_Suspend(); // cooperative multitasking
    }
}
void Thread2(void) {
    Count2 = 0;
    for (;;) {
        PD1 ^= 0x02; // heartbeat
        Count2++;
        // led_toggle(GREEN_LED);
        // busy_wait(4, ms(500));
        OS_Suspend(); // cooperative multitasking
    }
}
void Thread3(void) {
    Count3 = 0;
    for (;;) {
        PD2 ^= 0x04; // heartbeat
        Count3++;
        // led_toggle(RED_LED);
        // busy_wait(4, ms(500));
        OS_Suspend(); // cooperative multitasking
    }
}

void Testmain1(void) { // Testmain1
    OS_Init();         // initialize, disable interrupts
    PortD_Init();      // profile user threads
    NumCreated = 0;
    NumCreated += OS_AddThread(&Thread1, 128, 0);
    NumCreated += OS_AddThread(&Thread2, 128, 0);
    NumCreated += OS_AddThread(&Thread3, 128, 0);
    // Count1 Count2 Count3 should be equal or off by one at all times
    OS_Launch(ms(2));
}

//*******************Second TEST**********
// Once the initalize test runs, test this (Lab 2 part 1)
// no UART interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// no timer interrupts
// no switch interrupts
// no ADC serial port or LCD output
// no calls to semaphores
void Thread1b(void) {
    Count1 = 0;
    for (;;) {
        PD0 ^= 0x01; // heartbeat
        Count1++;
    }
}
void Thread2b(void) {
    Count2 = 0;
    for (;;) {
        PD1 ^= 0x02; // heartbeat
        Count2++;
    }
}
void Thread3b(void) {
    Count3 = 0;
    for (;;) {
        PD2 ^= 0x04; // heartbeat
        Count3++;
    }
}

void Testmain2(void) { // Testmain2
    OS_Init();         // initialize, disable interrupts
    PortD_Init();      // profile user threads
    NumCreated = 0;
    NumCreated += OS_AddThread(&Thread1b, 128, 0);
    NumCreated += OS_AddThread(&Thread2b, 128, 0);
    NumCreated += OS_AddThread(&Thread3b, 128, 0);
    // Count1 Count2 Count3 should be equal on average
    // counts are larger than Testmain1

    OS_Launch(ms(2));
}

//*******************Third TEST**********
// Once the second test runs, test this (Lab 2 part 2)
// no UART interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// no timer interrupts
// no switch interrupts
// no ADC serial port or LCD output
// no calls to semaphores
// tests AddThread, Sleep and Kill
void Thread1c(void) {
    int i;
    Count1 = 0;
    for (i = 0; i < 42; i++) {
        PD0 ^= 0x01; // heartbeat
        Count1++;
    }
    OS_Kill();
    Count1 = 0;
}
void Thread2c(void) {
    Count2 = 0;
    for (;;) {
        PD1 ^= 0x02; // heartbeat
        Count2++;
        NumCreated += OS_AddThread(&Thread1c, 128, 0);
        OS_Sleep(ms(5));
    }
}
void Thread3c(void) {
    Count3 = 0;
    for (;;) {
        PD2 ^= 0x04; // heartbeat
        Count3++;
    }
}

void Testmain3(void) { // Testmain3
    OS_Init();         // initialize, disable interrupts
    PortD_Init();      // profile user threads
    NumCreated = 0;
    NumCreated += OS_AddThread(&Thread2c, 128, 0);
    NumCreated += OS_AddThread(&Thread3c, 128, 0);
    // Count3 should be larger than Count2, Count1 should be 42

    OS_Launch(ms(2));
}

//*******************Fourth TEST**********
// Once the third test runs, test this (Lab 2 part 2)
// no UART1 interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// Timer interrupts, with or without period established by OS_AddPeriodicThread
// PortF GPIO interrupts, active low
// no ADC serial port or LCD output
// tests the spinlock semaphores, tests Sleep and Kill
Sema4 Readyd; // set in background
int Lost;
void BackgroundThread1d(void) { // called at 1000 Hz
    Count1++;
    OS_Signal(&Readyd);
}
void Thread5d(void) {
    for (;;) {
        OS_Wait(&Readyd);
        Count5++; // Count2 + Count5 should equal Count1
        Lost = Count1 - Count5 - Count2;
    }
}
void Thread2d(void) {
    OS_InitSemaphore(&Readyd, 0);
    Count1 = 0; // number of times signal is called
    Count2 = 0;
    Count5 = 0; // Count2 + Count5 should equal Count1
    NumCreated += OS_AddThread(&Thread5d, 128, 0);
    OS_AddPeriodicThread(&BackgroundThread1d, ms(1), 0);
    for (;;) {
        OS_Wait(&Readyd);
        Count2++; // Count2 + Count5 should equal Count1
    }
}
void Thread3d(void) {
    Count3 = 0;
    for (;;) { Count3++; }
}
void Thread4d(void) {
    int i;
    for (i = 0; i < 64; i++) {
        Count4++;
        OS_Sleep(ms(10));
    }
    OS_Kill();
    Count4 = 0; // should never run (Count4 should be 64)
}
void BackgroundThread5d(void) { // called when Select button pushed
    NumCreated += OS_AddThread(&Thread4d, 128, 0);
}

void Testmain4(void) { // Testmain4
    Count4 = 0;
    OS_Init(); // initialize, disable interrupts
    // Count2 + Count5 should equal Count1
    // Count4 increases by 64 every time select is pressed
    NumCreated = 0;
    OS_AddSW1Task(&BackgroundThread5d, 2);
    NumCreated += OS_AddThread(&Thread2d, 128, 0);
    NumCreated += OS_AddThread(&Thread3d, 128, 0);
    NumCreated += OS_AddThread(&Thread4d, 128, 0);
    OS_Launch(ms(2));
}

//*******************Fith TEST**********
// Once the fourth test runs, run this example (Lab 2 part 2)
// no UART interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// Timer interrupts, with or without period established by OS_AddPeriodicThread
// Select switch interrupts, active low
// no ADC serial port or LCD output
// tests the spinlock semaphores, tests Sleep and Kill
Sema4 Readye;                   // set in background
void BackgroundThread1e(void) { // called at 2000 Hz
    static int i = 0;
    i++;
    if (i == 50) {
        i = 0; // every 25 ms
        Count1++;
        OS_bSignal(&Readye);
    }
}
void Thread2e(void) {
    OS_InitSemaphore(&Readye, 0);
    Count1 = 0;
    Count2 = 0;
    for (;;) {
        OS_bWait(&Readye);
        Count2++; // Count2 should be equal to Count1
    }
}
void Thread3e(void) {
    Count3 = 0;
    for (;;) {
        Count3++; // Count3 should be large
    }
}
void Thread4e(void) {
    int i;
    for (i = 0; i < 640; i++) {
        Count4++; // Count4 should increase on button press
        OS_Sleep(ms(1));
    }
    OS_Kill();
}
void BackgroundThread5e(void) { // called when Select button pushed
    NumCreated += OS_AddThread(&Thread4e, 128, 0);
}

void Testmain5(void) { // Testmain5
    Count4 = 0;
    OS_Init(); // initialize, disable interrupts
    // Count1 should exactly equal Count2
    // Count3 should be very large
    // Count4 increases by 640 every time select is pressed
    NumCreated = 0;
    OS_AddPeriodicThread(&BackgroundThread1e, ms(0.5), 0);
    OS_AddSW1Task(&BackgroundThread5e, 2);
    NumCreated += OS_AddThread(&Thread2e, 128, 0);
    NumCreated += OS_AddThread(&Thread3e, 128, 0);
    NumCreated += OS_AddThread(&Thread4e, 128, 0);
    OS_Launch(ms(2));
}

//*******************Measurement of context switch time**********
// Run this to measure the time it takes to perform a task switch
// UART0 not needed
// SYSTICK interrupts, period established by OS_Launch
// first timer not needed
// second timer not needed
// SW1 not needed,
// SW2 not needed
// logic analyzer on PF1 for systick interrupt (in your OS)
//                on PD0 to measure context switch time
void ThreadCS(void) { // only thread running
    while (1) {
        PD0 ^= 0x01; // debugging profile
    }
}
void TestmainCS(void) { // TestmainCS
    PortD_Init();
    OS_Init(); // initialize, disable interrupts
    NumCreated = 0;
    NumCreated += OS_AddThread(&ThreadCS, 128, 0);
    OS_Launch(us(100));
}

//*******************FIFO TEST**********
// FIFO test
// Count1 should exactly equal Count2
// Count3 should be very large
// Timer interrupts, with period established by OS_AddPeriodicThread
uint32_t OtherCount1;
uint32_t Expected8; // last data read+1
uint32_t Error8;
void ConsumerThreadFIFO(void) {
    Count2 = 0;
    for (;;) {
        OtherCount1 = OS_Fifo_Get();
        if (OtherCount1 != Expected8) {
            Error8++;
        }
        Expected8 = OtherCount1 + 1; // should be sequential
        Count2++;
    }
}
void FillerThreadFIFO(void) {
    Count3 = 0;
    for (;;) { Count3++; }
}
void BackgroundThreadFIFOProducer(void) { // called periodically
    if (!OS_Fifo_Put(Count1)) {           // send to consumer
        DataLost++;
    }
    Count1++;
}

void TestmainFIFO(void) { // TestmainFIFO
    Count1 = 0;
    DataLost = 0;
    Expected8 = 0;
    Error8 = 0;
    OS_Init(); // initialize, disable interrupts
    NumCreated = 0;
    OS_AddPeriodicThread(&BackgroundThreadFIFOProducer, us(500), 0);
    OS_Fifo_Init(16);
    NumCreated += OS_AddThread(&ConsumerThreadFIFO, 128, 0);
    NumCreated += OS_AddThread(&FillerThreadFIFO, 128, 0);
    OS_Launch(ms(2));
}

int main(void) {
    // Testmain1();
    // Testmain2();
    // TestmainCS();
    // Testmain3();
    // Testmain4();
    // Testmain5();
    // TestmainFIFO();
    realmain();
    return 0;
}
