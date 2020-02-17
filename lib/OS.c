#include "OS.h"
#include "ST7735.h"
#include "interrupts.h"
#include "launchpad.h"
#include "tcb.h"
#include "timer.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tivaware/timer.h"
#include <stdint.h>

// Performance Measurements
int32_t MaxJitter; // largest time jitter between interrupts in usec
#define JITTERSIZE 64
uint32_t const JitterSize = JITTERSIZE;
uint32_t JitterHistogram[JITTERSIZE] = {
    0,
};

TCB threads[3] = {{.next_tcb = &threads[1], .sleep = true},
                  {.next_tcb = &threads[2], .sleep = true},
                  {.next_tcb = &threads[0], .sleep = true}};
TCB* run_tcb;

// Interrupts every 10ms for preemptive thread switch
void SysTick_Handler(void) {}

unsigned long OS_LockScheduler(void) {
    // lab 4 might need this for disk formating
    return 0; // replace with solution
}
void OS_UnLockScheduler(unsigned long previous) {
    // lab 4 might need this for disk formating
}

void SysTick_Init(unsigned long period) {}

void OS_Init(void) {
    // put Lab 2 (and beyond) solution here
}

void OS_InitSemaphore(Sema4Type* semaPt, int32_t value) {
    // put Lab 2 (and beyond) solution here
}

void OS_Wait(Sema4Type* semaPt) {
    // put Lab 2 (and beyond) solution here
}

void OS_Signal(Sema4Type* semaPt) {
    // put Lab 2 (and beyond) solution here
}

void OS_bWait(Sema4Type* semaPt) {
    // put Lab 2 (and beyond) solution here
}

void OS_bSignal(Sema4Type* semaPt) {
    // put Lab 2 (and beyond) solution here
}

int OS_AddThread(void (*task)(void), uint32_t stackSize, uint32_t priority) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

uint32_t OS_Id(void) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

int OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                         uint32_t priority) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

int OS_AddSW1Task(void (*task)(void), uint32_t priority) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

int OS_AddSW2Task(void (*task)(void), uint32_t priority) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

void OS_Sleep(uint32_t sleepTime) {
    // put Lab 2 (and beyond) solution here
}

void OS_Kill(void) {
    // put Lab 2 (and beyond) solution here
    enable_interrupts(); // end of atomic section
    for (;;) {};         // can not return
}

void OS_Suspend(void) {
    // put Lab 2 (and beyond) solution here
    run_tcb->sleep = true;
    asm("PUSH {R0-R15}\n");
    run_tcb = run_tcb->next_tcb;
}

void OS_Fifo_Init(uint32_t size) {
    // put Lab 2 (and beyond) solution here
}

int OS_Fifo_Put(uint32_t data) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

uint32_t OS_Fifo_Get(void) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

int32_t OS_Fifo_Size(void) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

void OS_MailBox_Init(void) {
    // put Lab 2 (and beyond) solution here
    // put solution here
}

void OS_MailBox_Send(uint32_t data) {
    // put Lab 2 (and beyond) solution here
    // put solution here
}

uint32_t OS_MailBox_Recv(void) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

uint32_t OS_Time(void) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

uint32_t OS_TimeDifference(uint32_t start, uint32_t stop) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

uint32_t CURRENT_MS;

void increment_global_time(void) {
    ++CURRENT_MS;
}

void OS_ClearMsTime(void) {
    periodic_timer_enable(5, ms(1), increment_global_time, 0);
    CURRENT_MS = 0;
}

uint32_t OS_MsTime(void) {
    return CURRENT_MS;
}

void OS_Launch(uint32_t theTimeSlice) {
    ROM_SysTickPeriodSet(theTimeSlice);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();
}

int OS_RedirectToFile(char* name) {
    return 1;
}

int OS_RedirectToUART(void) {
    return 1;
}

int OS_RedirectToST7735(void) {
    return 1;
}

int OS_EndRedirectToFile(void) {
    return 1;
}
