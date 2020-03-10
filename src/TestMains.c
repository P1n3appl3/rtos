#include "OS.h"
#include "io.h"
#include "launchpad.h"
#include "printf.h"
#include "std.h"
#include <stdint.h>

#define PD0 (*((volatile uint32_t*)0x40007004))
#define PD1 (*((volatile uint32_t*)0x40007008))
#define PD2 (*((volatile uint32_t*)0x40007010))
#define PD3 (*((volatile uint32_t*)0x40007020))

extern uint32_t NumCreated;
extern void PortD_Init(void);

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
        OS_Suspend(); // cooperative multitasking
    }
}
void Thread2(void) {
    Count2 = 0;
    for (;;) {
        PD1 ^= 0x02; // heartbeat
        Count2++;
        OS_Suspend(); // cooperative multitasking
    }
}
void Thread3(void) {
    Count3 = 0;
    for (;;) {
        PD2 ^= 0x04; // heartbeat
        Count3++;
        OS_Suspend(); // cooperative multitasking
    }
}

int testmain1(void) { // testmain1
    OS_Init();        // initialize, disable interrupts
    PortD_Init();     // profile user threads
    NumCreated = 0;
    NumCreated += OS_AddThread(&Thread1, "Thread1", 128, 0);
    NumCreated += OS_AddThread(&Thread2, "Thread2", 128, 0);
    NumCreated += OS_AddThread(&Thread3, "Thread3", 128, 0);
    // Count1 Count2 Count3 should be equal or off by one at all times
    OS_Launch(ms(2)); // doesn't return, interrupts enabled in here
    return 0;         // this never executes
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

int testmain2(void) { // testmain2
    OS_Init();        // initialize, disable interrupts
    PortD_Init();     // profile user threads
    NumCreated = 0;
    NumCreated += OS_AddThread(&Thread1b, "Thread1", 128, 0);
    NumCreated += OS_AddThread(&Thread2b, "Thread2", 128, 0);
    NumCreated += OS_AddThread(&Thread3b, "Thread3", 128, 0);
    // Count1 Count2 Count3 should be equal on average
    // counts are larger than testmain1

    OS_Launch(ms(2)); // doesn't return, interrupts enabled in here
    return 0;         // this never executes
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
    for (i = 0; i <= 42; i++) {
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
        NumCreated += OS_AddThread(&Thread1c, "Thread1", 128, 0);
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

int testmain3(void) { // testmain3
    OS_Init();        // initialize, disable interrupts
    PortD_Init();     // profile user threads
    NumCreated = 0;
    NumCreated += OS_AddThread(&Thread2c, "Thread2", 128, 0);
    NumCreated += OS_AddThread(&Thread3c, "Thread3", 128, 0);
    // Count3 should be larger than Count2, Count1 should be 42

    OS_Launch(ms(2)); // doesn't return, interrupts enabled in here
    return 0;         // this never executes
}

//*******************Fourth TEST**********
// Once the third test runs, test this (Lab 2 part 2 and Lab 3)
// no UART1 interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// Timer interrupts, with or without period established by OS_AddPeriodicThread
// PortF GPIO interrupts, active low
// no ADC serial port or LCD output
// tests priorities and blocking semaphores, tests Sleep and Kill
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
    NumCreated += OS_AddThread(&Thread5d, "Thread5", 128, 1);
    OS_AddPeriodicThread(&BackgroundThread1d, ms(1), 0);
    for (;;) {
        OS_Wait(&Readyd);
        Count2++; // Count2 + Count5 should equal Count1
        Lost = Count1 - Count5 - Count2;
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
    Count4 = 0;
}
void BackgroundThread5d(void) { // called when Select button pushed
    NumCreated += OS_AddThread(&Thread4d, "Thread4", 128, 1);
}

int testmain4(void) { // testmain4
    Count4 = 0;
    OS_Init(); // initialize, disable interrupts
    // Count2 + Count5 should equal Count1
    // With priorities, Count5 should be zero
    // Count4 increases by 64 every time select is pressed
    NumCreated = 0;
    OS_AddSW1Task(&BackgroundThread5d, 2);
    NumCreated +=
        OS_AddThread(&Thread2d, "Thread2", 128, 0); // Lab 3 highest priority
    NumCreated += OS_AddThread(&Thread3d, "Thread3", 128, 1);
    NumCreated += OS_AddThread(&Thread4d, "Thread4", 128, 1);
    OS_Launch(ms(2)); // doesn't return, interrupts enabled in here
    return 0;         // this never executes
}

//*******************Fith TEST**********
// Once the fourth test runs, run this example (Lab 2 part 2 and Lab 3)
// no UART interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// Timer interrupts, with or without period established by OS_AddPeriodicThread
// Select switch interrupts, active low
// no ADC serial port or LCD output
// tests the blocking semaphores, tests Sleep and Kill
// uses priorities to test proper blocking of sempahore waits
Sema4 Readye;                   // set in background
void BackgroundThread1e(void) { // called at 1000 Hz
    static int i = 0;
    i++;
    if (i == 50) {
        i = 0; // every 50 ms
        Count1++;
        OS_Signal(&Readye);
    }
}
void Thread2e(void) {
    OS_InitSemaphore(&Readye, 0);
    Count1 = 0;
    Count2 = 0;
    for (;;) {
        OS_Wait(&Readye);
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
    NumCreated += OS_AddThread(&Thread4e, "Thread4", 128, 1);
}

int testmain5(void) { // testmain5
    Count4 = 0;
    OS_Init(); // initialize, disable interrupts
    // Count1 should exactly equal Count2
    // Count3 should be very large
    // Count4 increases by 640 every time select is pressed
    NumCreated = 0;
    OS_AddPeriodicThread(&BackgroundThread1e, us(500), 0);
    OS_AddSW1Task(&BackgroundThread5e, 2);
    NumCreated += OS_AddThread(&Thread2e, "Thread2", 128,
                               0); // Lab 3 set to highest priority
    NumCreated += OS_AddThread(&Thread3e, "Thread3", 128, 1);
    NumCreated += OS_AddThread(&Thread4e, "Thread4", 128, 1);
    OS_Launch(ms(2)); // doesn't return, interrupts enabled in here
    return 0;
}

//******************* Lab 3 Procedure 2**********
// Modify this so it runs with your RTOS (i.e., fix the time units to match your
// OS) run this with UART0, 115200 baud rate, used to output results SYSTICK
// interrupts, period established by OS_Launch first timer interrupts, period
// established by first call to OS_AddPeriodicThread second timer interrupts,
// period established by second call to OS_AddPeriodicThread SW1 no interrupts
// SW2 no interrupts
uint32_t CountA; // number of times Task A called
uint32_t CountB; // number of times Task B called
uint32_t Count1; // number of times thread1 loops

// simple time delay, simulates user program doing real work
// Input: amount of work in cycles
void PseudoWork(uint32_t work) {
    uint32_t startTime;
    startTime = OS_Time(); // time in 100ns units
    while (difference(startTime, OS_Time()) <= work) {}
}
void Thread6(void) { // foreground thread
    Count1 = 0;
    for (;;) {
        Count1++;
        PD0 ^= 0x01; // debugging toggle bit 0
    }
}
void Thread7(void) {
    puts("\n\rEE345M/EE380L, Lab 3 Procedure 2");
    OS_Sleep(seconds(5));
    led_write(RED_LED, true);
    OS_ReportJitter();
    puts("\n");
    OS_Kill();
}
void TaskA(void) { // called every {1000, 2990us} in background
    PD1 = 0x02;    // debugging profile
    CountA++;
    PseudoWork(us(500));
    PD1 = 0x00; // debugging profile
}
void TaskB(void) { // called every pB in background
    PD2 = 0x04;    // debugging profile
    CountB++;
    PseudoWork(us(250)); //  do work
    PD2 = 0x00;          // debugging profile
}

int testmain6(void) { // testmain6 Lab 3
    PortD_Init();
    OS_Init(); // initialize, disable interrupts
    NumCreated = 0;
    NumCreated += OS_AddThread(&Thread7, "Thread7", 128, 1);
    NumCreated += OS_AddThread(&Thread6, "Thread6", 128, 2);
    OS_AddPeriodicThread(&TaskA, ms(1), 0);
    OS_AddPeriodicThread(&TaskB, ms(2), 1);

    OS_Launch(ms(2)); // 2ms, doesn't return, interrupts enabled in here
    return 0;         // this never executes
}

//******************* Lab 3 Procedure 4**********
// Modify this so it runs with your RTOS used to test blocking semaphores
// run this with
// UART0, 115200 baud rate,  used to output results
// SYSTICK interrupts, period established by OS_Launch
// first timer interrupts, period established by first call to
// OS_AddPeriodicThread second timer interrupts, period established by second
// call to OS_AddPeriodicThread SW1 no interrupts, SW2 no interrupts
Sema4 s;               // test of this counting semaphore
uint32_t SignalCount1; // number of times s is signaled
uint32_t SignalCount2; // number of times s is signaled
uint32_t SignalCount3; // number of times s is signaled
uint32_t WaitCount1;   // number of times s is successfully waited on
uint32_t WaitCount2;   // number of times s is successfully waited on
uint32_t WaitCount3;   // number of times s is successfully waited on
#define MAXCOUNT 20000
void OutputThread(void) { // foreground thread
    puts("\n\rEE445M/EE380L, Lab 3 Procedure 4\n\r");
    while (SignalCount1 + SignalCount2 + SignalCount3 < 100 * MAXCOUNT) {
        OS_Sleep(seconds(1)); // 1 second
        putchar('.');
    }
    puts(" done");
    printf("Signalled=%d, Waited=%d\n\r",
           SignalCount1 + SignalCount2 + SignalCount3,
           WaitCount1 + WaitCount2 + WaitCount3);
    OS_Kill();
}
void Wait1(void) { // foreground thread
    for (;;) {
        OS_Wait(&s); // three threads waiting
        WaitCount1++;
    }
}
void Wait2(void) { // foreground thread
    for (;;) {
        OS_Wait(&s); // three threads waiting
        WaitCount2++;
    }
}
void Wait3(void) { // foreground thread
    for (;;) {
        OS_Wait(&s); // three threads waiting
        WaitCount3++;
    }
}
void Signal1(void) { // called every 799us in background
    if (SignalCount1 < MAXCOUNT) {
        OS_Signal(&s);
        SignalCount1++;
    }
}
// edit this so it changes the periodic rate
void Signal2(void) { // called every 1111us in background
    if (SignalCount2 < MAXCOUNT) {
        OS_Signal(&s);
        SignalCount2++;
    }
}
void Signal3(void) { // foreground
    while (SignalCount3 < 98 * MAXCOUNT) {
        OS_Signal(&s);
        SignalCount3++;
    }
    OS_Kill();
}

int testmain7(void) { // testmain7  Lab 3
    OS_Init();        // initialize, disable interrupts
    PortD_Init();
    SignalCount1 = 0;        // number of times s is signaled
    SignalCount2 = 0;        // number of times s is signaled
    SignalCount3 = 0;        // number of times s is signaled
    WaitCount1 = 0;          // number of times s is successfully waited on
    WaitCount2 = 0;          // number of times s is successfully waited on
    WaitCount3 = 0;          // number of times s is successfully waited on
    OS_InitSemaphore(&s, 0); // this is the test semaphore
    OS_AddPeriodicThread(&Signal1, us(799), 0);  // 0.799 ms, higher priority
    OS_AddPeriodicThread(&Signal2, us(1111), 1); // 1.111 ms, lower priority
    NumCreated = 0;
    NumCreated += OS_AddThread(&OutputThread, "OutputThread", 128,
                               2); // results output thread
    NumCreated +=
        OS_AddThread(&Signal3, "Signal3", 128, 2);       // signalling thread
    NumCreated += OS_AddThread(&Wait1, "Wait1", 128, 2); // waiting thread
    NumCreated += OS_AddThread(&Wait2, "Wait2", 128, 2); // waiting thread
    NumCreated += OS_AddThread(&Wait3, "Wait3", 128, 2); // waiting thread
    NumCreated += OS_AddThread(&Thread6, "Thread6", 128,
                               5); // idle thread to keep from crashing

    OS_Launch(ms(1)); // 1ms, doesn't return, interrupts enabled in here
    return 0;         // this never executes
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
int testmainCS(void) { // testmainCS
    PortD_Init();
    OS_Init(); // initialize, disable interrupts
    NumCreated = 0;
    NumCreated += OS_AddThread(&ThreadCS, "ThreadCS", 128, 0);
    OS_Launch(us(100));
    return 0; // this never executes
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
extern uint32_t DataLost;
void BackgroundThreadFIFOProducer(void) { // called periodically
    if (OS_Fifo_Put(Count1) == 0) {       // send to consumer
        DataLost++;
    }
    Count1++;
}

int testmainFIFO(void) { // testmainFIFO
    Count1 = 0;
    DataLost = 0;
    Expected8 = 0;
    Error8 = 0;
    OS_Init(); // initialize, disable interrupts
    NumCreated = 0;
    OS_AddPeriodicThread(&BackgroundThreadFIFOProducer, us(500), 0);
    OS_Fifo_Init(16);
    NumCreated += OS_AddThread(&ConsumerThreadFIFO, "consumer", 128, 2);
    NumCreated += OS_AddThread(&FillerThreadFIFO, "filler", 128, 3);
    OS_Launch(ms(2)); // doesn't return, interrupts enabled in here
    return 0;         // this never executes
}

Sema4 my_sem;
void mytask(void) {
    for (int i = 0; i < 100; ++i) { OS_Signal(&my_sem); }
}

uint32_t num_a = 0;
uint32_t num_b = 0;

void thread_a(void) {
    while (true) {
        OS_Wait(&my_sem);
        ++num_a;
    }
}

void thread_b(void) {
    while (true) {
        OS_Wait(&my_sem);
        ++num_b;
    }
}

void mytestmain(void) {
    OS_Init();
    OS_InitSemaphore(&my_sem, 0);
    OS_AddPeriodicThread(mytask, ms(10), 1);
    OS_AddThread(thread_a, "AAAA", 128, 0);
    OS_AddThread(thread_b, "BBBB", 128, 1);
    OS_Launch(ms(2));
}

extern void realmain(void);

void main(void) {
    // mytestmain();
    // testmain1();
    // testmain2();
    // testmain3();
    // testmain4();
    // testmain5();
    // testmain6();
    // testmain7();
    realmain();
}
