#include "ADC.h"
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

void SWPush(void) {
    OS_AddThread(&ButtonWork, "Button work", 200, 2);
}

void realmain(void) {
    OS_Init();
    PortD_Init();

    adc_init(0); // sequencer 3, channel 0, PE3, sampling in Interpreter

    OS_AddPeriodicThread(&disk_timerproc, ms(1), 0);
    OS_AddSW1Task(&SWPush, 2);
    OS_AddSW2Task(&SWPush, 2);

    OS_AddThread(&interpreter, "interpreter", 2048, 2);

    OS_Launch(ms(2));
}

//*****************Test project 0*************************

void overflowA(void) {
    static int x = 0;
    printf("A: %d\n\r", x++);
    OS_Suspend();
    overflowA();
}

void overflowB(void) {
    static int x = 0;
    printf("B: %d\n\r", x++);
    OS_Suspend();
    overflowB();
}

void allocator_optimization(void) {
    uint8_t* a;
    uint8_t* b;
    uint8_t* c;
    puts("\n\rSINGLE ALLOCATION TEST:");
    printf("Free space before allocating = %d\n\r", heap_get_space());
    a = malloc(32);
    printf("Free space after allocating  = %d\n\r", heap_get_space());
    free(a);
    printf("Free space after freeing     = %d\n\n\r", heap_get_space());

    heap_stats();

    puts("\n\rMULTIPLE ALLOCATION TEST");
    printf("Before = %d\n\r", heap_get_space());
    a = malloc(32), b = malloc(32), c = malloc(32);
    printf("After allocating = %d\n\r", heap_get_space());
    free(a), free(b), free(c);
    printf("Freed in order = %d\n\r", heap_get_space());
    a = malloc(32), b = malloc(32), c = malloc(32),
    printf("After allocating = %d\n\r", heap_get_space());
    free(c), free(b), free(a);
    printf("Freed in reverse = %d\n\r", heap_get_space());
    a = malloc(32), b = malloc(32), c = malloc(32),
    printf("After allocating = %d\n\r", heap_get_space());
    free(a), free(c), free(b);
    printf("Middle freed last = %d\n\n\r", heap_get_space());

    heap_stats();

    puts("\n\rREALLOC TEST");
    printf("Before allocating = %d\n\r", heap_get_space());
    a = malloc(32);
    printf("a = 0x%08x\n\r", a);
    a = realloc(a, 64);
    printf("a = 0x%08x\n\r", a);
    b = malloc(64);
    a = realloc(a, 128);
    printf("a = 0x%08x\n\r", a);
    a = realloc(a, 64);
    free(a), free(b);
    printf("After Freeing = %d\n\n\r", heap_get_space());

    heap_stats();

    while (true) {}
}

void thrasherA(void) {
    char* a;
    while (true) {
        a = malloc(32);
        if (!a)
            break;
        // a = realloc(a, 16);
        // if (!a)
        //     break;
        // a = realloc(a, 128);
        // if (!a)
        //     break;
        // a = realloc(a, 64);
        // if (!a)
        //     break;
        free(a);
    }
    printf("Uh oh, it seems you're out of memory\n\r");
    while (true) {}
}

void thrasherB(void) {
    char* b;
    while (true) {
        b = malloc(32);
        if (!b)
            break;
        // b = realloc(b, 16);
        // if (!b)
        //     break;
        // b = realloc(b, 128);
        // if (!b)
        //     break;
        // b = realloc(b, 64);
        // if (!b)
        //     break;
        free(b);
    }
    printf("Uh oh, it seems you're out of memory\n\r");
    while (true) {}
}

void Testmain0(void) {
    OS_Init();

    OS_AddThread(thrasherA, "thrasher A", 1024, 1);
    OS_AddThread(thrasherB, "thrasher B", 1024, 1);
    // OS_AddThread(allocator_optimization, "allocator test", 2048, 1);
    // OS_AddThread(overflowA, "overflow test", 2048, 1);
    // OS_AddThread(overflowB, "overflow test", 1024, 1);
    // OS_AddThread(debug_test, "filesystem", 512, 0);
    // OS_AddPeriodicThread(&disk_timerproc, ms(1), 0);

    OS_Launch(us(100));
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
    OS_AddThread(&TestHeap, "Test Heap", 2048, 1);
}

void Testmain1(void) {
    OS_Init();
    PortD_Init();

    OS_AddSW1Task(&SW1Push1, 2);

    OS_AddThread(&TestHeap, "Test Heap", 2048, 1);

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
extern bool OS_AddProcess(void (*entry)(void), void* text, void* data,
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
    if (!OS_AddProcess(&TestUser, calloc(128), calloc(128), 2048, 1)) {
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
    OS_AddThread(&TestProcess, "Test Process", 1024, 1);
}

void Testmain2(void) {
    OS_Init();
    PortD_Init();

    OS_AddSW1Task(&SW1Push1, 2); // PF4, SW1
    OS_AddSW2Task(&SW2Push2, 2); // PF0, SW2

    OS_AddThread(&TestProcess, "Test Process", 512, 1);

    OS_Launch(ms(10));
}

//*****************Test project 3*************************
// Test supervisor calls (SVC exceptions)
// Using inline assembly, syntax is dependent on the compiler
// The following code compiles in Keil 5.x (even though the UI
// complains)

__attribute__((naked)) uint32_t SVC_OS_Id(void) {
    __asm("SVC #0\n"
          "BX LR");
}
__attribute__((naked)) void SVC_OS_Kill(void) {
    __asm("SVC #1\n"
          "BX LR");
}
__attribute__((naked)) void SVC_OS_Sleep(uint32_t t) {
    __asm("SVC #2\n"
          "BX LR");
}
__attribute__((naked)) uint32_t SVC_OS_Time(void) {
    __asm("SVC #3\n"
          "BX LR");
}
__attribute__((naked)) bool SVC_OS_AddThread(void (*t)(void), const char* name,
                                             uint32_t s, uint32_t p) {
    __asm("SVC #4\n"
          "BX LR");
}

uint32_t line = 0;
void TestSVCThread(void) {
    uint32_t id;
    id = SVC_OS_Id();
    PD3 ^= 0x08;
    line++;
    printf("Thread: %d", id);
    SVC_OS_Sleep(ms(500));
    line++;
    printf("Thread dying: %d", id);
    PD3 ^= 0x08;
    SVC_OS_Kill();
}
void TestSVC(void) {
    uint32_t id;
    uint32_t time;
    // simple SVC test, mimicking real user program
    printf("\n\rEE445M/EE380L, Lab 5 SCV Test\n\r");
    id = SVC_OS_Id();
    PD2 ^= 0x04;
    printf("SVC test: %d", id);
    line++;
    SVC_OS_AddThread(TestSVCThread, "TestSVCThread", 128, 1);
    time = SVC_OS_Time();
    SVC_OS_Sleep(seconds(1));
    time = to_us(SVC_OS_Time() - time);
    printf("Sleep time: %d", time);
    line++;
    PD2 ^= 0x04;
    if (line != 4) {
        printf("SVC test error");
        OS_Kill();
    }
    printf("Successful SVC test\n\r");
    SVC_OS_Kill();
}

void SWPush3(void) {
    if (line >= 4) {
        line = 0;
        OS_AddThread(&TestSVC, "TestSVC", 128, 1);
    }
}

void Testmain3(void) {
    OS_Init();
    PortD_Init();

    // attach background tasks
    OS_AddSW1Task(&SWPush3, 2); // PF4, SW1
    OS_AddSW2Task(&SWPush3, 2); // PF0, SW2

    // create initial foreground threads
    OS_AddThread(&TestSVC, "TestSVC", 128, 1);

    OS_Launch(ms(10));
}

void main(void) {
    // Testmain0();
    // Testmain1();
    Testmain2();
    // Testmain3();
    // realmain();
}
