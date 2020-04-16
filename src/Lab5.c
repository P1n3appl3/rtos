#include "ADC.h"
#include "LPF.h"
#include "OS.h"
#include "ST7735.h"
#include "eDisk.h"
#include "heap.h"
#include "interpreter.h"
#include "interrupts.h"
#include "io.h"
#include "launchpad.h"
#include "littlefs.h"
#include "printf.h"
#include "std.h"
#include "tivaware/rom.h"
#include <stdint.h>

uint32_t NumCreated; // number of foreground threads created
uint32_t IdleCount;  // CPU idle counter

#define PD0 (*((volatile uint32_t*)0x40007004))
#define PD1 (*((volatile uint32_t*)0x40007008))
#define PD2 (*((volatile uint32_t*)0x40007010))
#define PD3 (*((volatile uint32_t*)0x40007020))

void PortD_Init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, 0x0F);
}

//------------------Task 1--------------------------------
// background thread executes with SW1 button
// one foreground task created with button push

void ButtonWork(void) {
    PD1 ^= 0x02;
    PD1 ^= 0x02;
    heap_stats();
    PD1 ^= 0x02;
    OS_Kill();
}

void SW1Push(void) {
    if (OS_AddThread(&ButtonWork, "Button work", 200, 2)) {
        NumCreated++;
    }
}

void SW2Push(void) {
    if (OS_AddThread(&ButtonWork, "Button work", 200, 2)) {
        NumCreated++;
    }
}

//------------------Idle Task--------------------------------
// foreground thread, runs when nothing else does
// never blocks, never sleeps, never dies
void Idle(void) {
    IdleCount = 0;
    while (1) {
        IdleCount++;
        PD0 ^= 0x01;
        wait_for_interrupts();
    }
}

void realmain(void) {
    OS_Init();    // initialize, disable interrupts
    PortD_Init(); // debugging profile

    adc_init(0); // sequencer 3, channel 0, PE3, sampling in Interpreter

    OS_AddPeriodicThread(&disk_timerproc, ms(1), 0);
    OS_AddSW1Task(&SW1Push, 2);
    OS_AddSW2Task(&SW2Push, 2);

    NumCreated = 0;
    NumCreated += OS_AddThread(&interpreter, "interpreter", 2048, 2);
    NumCreated += OS_AddThread(&Idle, "Idle", 1024, 5);

    OS_Launch(ms(2));
}

//*****************Test project 0*************************
// This is the simplest configuration,
// Just see if you can import your OS
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
    }
}
void Thread2(void) {
    Count2 = 0;
    for (;;) {
        PD1 ^= 0x02; // heartbeat
        Count2++;
    }
}
void Thread3(void) {
    Count3 = 0;
    for (;;) {
        PD2 ^= 0x04; // heartbeat
        Count3++;
    }
}

void Testmain0(void) { // Testmain0
    OS_Init();         // initialize, disable interrupts
    PortD_Init();      // profile user threads
    NumCreated = 0;
    NumCreated += OS_AddThread(&Thread1, "Thread 1", 256, 1);
    NumCreated += OS_AddThread(&Thread2, "Thread 2", 256, 2);
    NumCreated += OS_AddThread(&Thread3, "Thread 3", 256, 3);
    OS_AddThread(debug_test, "filesystem", 512, 0);
    OS_AddPeriodicThread(&disk_timerproc, ms(1), 0);
    // Count1 Count2 Count3 should be equal or off by one at all times
    OS_Launch(ms(10)); // doesn't return, interrupts enabled in here
}

//*****************Test project 1*************************
// Heap test, allocate and deallocate memory
void heapError(const char* errtype, const char* v, uint32_t n) {
    printf("%s heap error %s %u\n\r", errtype, v, n);
    OS_Kill();
}

int16_t* ptr; // Global so easier to see with the debugger
int16_t* p1;  // Proper style would be to make these variables local
int16_t* p2;
int16_t* p3;
uint8_t* q1;
uint8_t* q2;
uint8_t* q3;
uint8_t* q4;
uint8_t* q5;
uint8_t* q6;
int16_t maxBlockSize;
uint8_t* bigBlock;
void TestHeap(void) {
    int16_t i;
    printf("\n\n\n\rEE445M/EE380L, Lab 5 Heap Test\n\r");

    ptr = malloc(sizeof(int16_t));
    if (!ptr)
        heapError("malloc", "ptr", 0);
    *ptr = 0x1111;

    free(ptr);

    ptr = malloc(1);
    if (!ptr)
        heapError("malloc", "ptr", 1);

    free(ptr);

    p1 = (int16_t*)malloc(1 * sizeof(int16_t));
    if (!p1)
        heapError("malloc", "p", 1);
    p2 = (int16_t*)malloc(2 * sizeof(int16_t));
    if (!p2)
        heapError("malloc", "p", 2);
    p3 = (int16_t*)malloc(3 * sizeof(int16_t));
    if (!p3)
        heapError("malloc", "p", 3);
    p1[0] = 0xAAAA;
    p2[0] = 0xBBBB;
    p2[1] = 0xBBBB;
    p3[0] = 0xCCCC;
    p3[1] = 0xCCCC;
    p3[2] = 0xCCCC;

    heap_stats();
    free(p1);
    free(p3);
    free(p2);
    heap_stats();

    printf("\nRealloc test\n\r");
    q1 = malloc(1);
    if (!q1)
        heapError("malloc", "q", 1);
    q2 = malloc(2);
    if (!q2)
        heapError("malloc", "q", 2);
    q3 = malloc(3);
    if (!q3)
        heapError("malloc", "q", 3);
    q4 = malloc(4);
    if (!q4)
        heapError("malloc", "q", 4);
    q5 = malloc(5);
    if (!q5)
        heapError("malloc", "q", 5);

    *q1 = 0xDD;
    q6 = realloc(q1, 6);
    heap_stats();

    for (i = 0; i < 6; i++) { q6[i] = 0xEE; }
    q1 = realloc(q6, 2);
    heap_stats();

    printf("\nLarge block test\n\r");
    heap_stats();
    maxBlockSize = heap_get_max();
    maxBlockSize -= maxBlockSize % 4;
    printf("Largest block that can be allocated is %d bytes\n\r", maxBlockSize);
    bigBlock = malloc(maxBlockSize);
    memset(bigBlock, 0xFF, maxBlockSize);
    heap_stats();
    free(bigBlock);

    bigBlock = calloc(maxBlockSize);
    if (!bigBlock)
        heapError("calloc", "bigBlock", 0);
    if (*bigBlock)
        heapError("Zero initialization", "bigBlock", 0);
    heap_stats();

    free(bigBlock);
    heap_stats();

    uint32_t max_allocs = heap_get_space() / sizeof(int32_t);
    printf("should be able to allocate %d int32_t's\n\r", max_allocs);
    for (i = 0; i <= max_allocs; i++) {
        ptr = malloc(sizeof(int16_t));
        if (!ptr)
            break;
    }
    if (ptr)
        heapError("malloc", "i", i);
    heap_stats();

    printf("\nSuccessful heap test\n\r");
    OS_Kill();
}

void SW1Push1(void) {
    if (OS_AddThread(&TestHeap, "Test Heap", 2048, 1)) {
        NumCreated++;
    }
}

void Testmain1(void) {
    OS_Init();
    PortD_Init();

    OS_AddSW1Task(&SW1Push1, 2);

    NumCreated = 0;
    NumCreated += OS_AddThread(&TestHeap, "Test Heap", 2048, 1);
    NumCreated += OS_AddThread(&Idle, "Idle", 512, 3);

    OS_Launch(ms(10));
}

//*****************Test project 2*************************
// Process management test, add and reclaim dummy process
void TestUser(void) {
    uint32_t id;
    uint32_t time;
    id = OS_Id();
    PD2 ^= 0x04;
    printf("Hello world: %d\n\r", id);
    time = OS_Time();
    OS_Sleep(seconds(1));
    printf("Sleep time: %dms\n\r",
           (uint32_t)to_ms(difference(time, OS_Time())));
    PD2 ^= 0x04;
    OS_Kill();
}

//  OS-internal OS_AddProcess function
extern int OS_AddProcess(void (*entry)(void), void* text, void* data,
                         unsigned long stackSize, unsigned long priority);

void TestProcess(void) {
    // simple process management test, add process with dummy code and data
    // segments
    printf("\n\rEE445M/EE380L, Lab 5 Process Test\n\r");
    PD1 ^= 0x02;
    PD1 ^= 0x02;
    heap_stats();
    uint32_t original_free = heap_get_space();
    PD1 ^= 0x02;
    if (!OS_AddProcess(&TestUser, calloc(128), calloc(128), 128, 1)) {
        printf("OS_AddProcess error");
        OS_Kill();
    }
    PD1 ^= 0x02;
    // wait long enough for user thread and hence process to exit/die
    OS_Sleep(seconds(2));
    PD1 ^= 0x02;
    PD1 ^= 0x02;
    heap_stats();
    PD1 ^= 0x02;
    uint32_t now_free = heap_get_space();
    if (original_free != now_free) {
        printf("Had %d free bytes but now only have %d\n\r", original_free,
               now_free);
    } else {
        printf("Successful process test\n\r");
    }
    OS_Kill();
}

void SW2Push2(void) {
    if (OS_AddThread(&TestProcess, "Test Process", 256, 1)) {
        NumCreated++;
    }
}

void Testmain2(void) {
    OS_Init();
    PortD_Init();

    OS_AddSW1Task(&SW1Push1, 2); // PF4, SW1
    OS_AddSW2Task(&SW2Push2, 2); // PF0, SW2

    NumCreated = 0;
    NumCreated += OS_AddThread(&TestProcess, "Test Process", 256, 1);
    NumCreated += OS_AddThread(&Idle, "Test Process", 256, 3);

    OS_Launch(ms(10));
}

//*****************Test project 3*************************
// Test supervisor calls (SVC exceptions)
// Using inline assembly, syntax is dependent on the compiler
// The following code compiles in Keil 5.x (even though the UI complains)
/*
__asm uint32_t SVC_OS_Id(void) {
    SVC #0 BX LR
}
__asm void SVC_OS_Kill(void) {
    SVC #1 BX LR
}
__asm void SVC_OS_Sleep(uint32_t t) {
    SVC #2 BX LR
}
__asm uint32_t SVC_OS_Time(void) {
    SVC #3 BX LR
}
__asm int SVC_OS_AddThread(void (*t)(void), uint32_t s,
                           uint32_t p){SVC #4 BX LR} uint32_t line = 0;
void TestSVCThread(void) {
    uint32_t id;
    id = SVC_OS_Id();
    PD3 ^= 0x08;
    ST7735_Message(0, line++, "Thread: ", id);
    SVC_OS_Sleep(500);
    ST7735_Message(0, line++, "Thread dying: ", id);
    PD3 ^= 0x08;
    SVC_OS_Kill();
}
void TestSVC(void) {
    uint32_t id;
    uint32_t time;
    // simple SVC test, mimicking real user program
    ST7735_DrawString(0, 0, "SVC test         ", ST7735_WHITE);
    printf("\n\rEE445M/EE380L, Lab 5 SCV Test\n\r");
    id = SVC_OS_Id();
    PD2 ^= 0x04;
    ST7735_Message(0, line++, "SVC test: ", id);
    SVC_OS_AddThread(TestSVCThread, 128, 1);
    time = SVC_OS_Time();
    SVC_OS_Sleep(1000);
    time =
        (((OS_TimeDifference(time, SVC_OS_Time())) / 1000ul) * 125ul) / 10000ul;
    ST7735_Message(0, line++, "Sleep time: ", time);
    PD2 ^= 0x04;
    if (line != 4) {
        printf("SVC test error");
        OS_Kill();
    }
    printf("Successful SVC test\n\r");
    ST7735_Message(0, 0, "SVC test done ", id);
    SVC_OS_Kill();
}

void SWPush3(void) {
    if (line >= 4) {
        line = 0;
        if (OS_AddThread(&TestSVC, 128, 1)) {
            NumCreated++;
        }
    }
}

int Testmain3(void) { // Testmain3
    OS_Init();        // initialize, disable interrupts
    PortD_Init();

    // attach background tasks
    OS_AddSW1Task(&SWPush3, 2); // PF4, SW1
    OS_AddSW2Task(&SWPush3, 2); // PF0, SW2

    // create initial foreground threads
    NumCreated = 0;
    NumCreated += OS_AddThread(&TestSVC, 128, 1);
    NumCreated += OS_AddThread(&Idle, 128, 3);

    OS_Launch(10 * TIME_1MS); // doesn't return, interrupts enabled in here
    return 0;                 // this never executes
}
*/

void main(void) {
    // Testmain0();
    // Testmain1();
    // Testmain2();
    realmain();
}
