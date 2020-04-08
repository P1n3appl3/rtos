#pragma once
#include "timer.h"
#include <stdint.h>

typedef struct tcb {
    uint32_t* sp;
    struct tcb* next_tcb;
    struct tcb* prev_tcb;

    uint32_t id;
    const char* name;

    struct tcb* next_blocked;

    uint32_t sleep_time;

    bool asleep;
    bool blocked;
    bool alive;
    uint8_t priority;

    uint32_t* stack;
} TCB;

typedef struct {
    int32_t value; // >=0 means free, negative means busy
    TCB* blocked_head;
} Sema4;

// initialize OS controlled I/O: UART, ADC, Systick, LaunchPad I/O and timers
// disable interrupts until OS_Launch
void OS_Init(void);

// add a foregound thread to the scheduler
// inputs: pointer to a foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// returns: true if successful, false if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
bool OS_AddThread(void (*task)(void), const char* name, uint32_t stackSize,
                  uint32_t priority);

// add a background periodic task
// typically this function receives the highest priority
// inputs: pointer to a void/void background function
//         period given in cycles
//         priority 0 is the highest, 5 is the lowest
// returns: true if successful, false if this thread can not be added
// This task can't block, but it can call OS_Signal or OS_AddThread
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority
// field determines the relative priority of these four threads
bool OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                          uint32_t priority);

// add a background task to run whenever the SW1 (PF4) button is pushed
// inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// This task can't block, but it can call OS_Signal or OS_AddThread
bool OS_AddSW1Task(void (*task)(void), uint32_t priority);

// add a background task to run whenever the SW2 (PF0) button is pushed
// inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// This task can't block, but it can call OS_Signal or OS_AddThread
bool OS_AddSW2Task(void (*task)(void), uint32_t priority);

void OS_InitSemaphore(Sema4* semaPt, int32_t value);

// block until a semaphore is available
// input: pointer to a semaphore
void OS_Wait(Sema4* semaPt);

// increment semaphore value and wake waiting thread if necessary
// input:  pointer to a semaphore
void OS_Signal(Sema4* semaPt);

// returns the thread id for the currently running thread
uint32_t OS_Id(void);

// place this thread into a dormant state
// input: number of cycles to sleep
void OS_Sleep(uint32_t sleep_time);

// kill the currently running thread, release its TCB and stack
void OS_Kill(void);

// suspend execution of currently running thread
// scheduler will choose another thread to execute
void OS_Suspend(void);

// temporarily prevent foreground thread switch (but allow background
// interrupts)
unsigned long OS_LockScheduler(void);
// resume foreground thread switching
void OS_UnLockScheduler(unsigned long previous);

// initialize the fifo to be empty
// size must be < 64    TODO: use dynamic memory to allow larger sizes
void OS_Fifo_Init(uint32_t size);

// attempts to add an element to the fifo without blocking
// returns: true if data is properly added, false if fifo was full
bool OS_Fifo_Put(uint32_t data);

// remove one data sample from the fifo (blocks if empty)
uint32_t OS_Fifo_Get(void);

// returns the number of elements in the fifo
int32_t OS_Fifo_Size(void);

// initialize the mailbox to be empty
void OS_MailBox_Init(void);

// enter mail into the MailBox, blocks if mailbox is full
void OS_MailBox_Send(uint32_t data);

// remove mail from the MailBox, blocks if mailbox is empty
uint32_t OS_MailBox_Recv(void);

// return the system time in cycles
uint32_t OS_Time(void);

// sets the system time to zero
void OS_ClearTime(void);

// start the scheduler, enable interrupts
// inputs: number of cycles for each time slice
void OS_Launch(uint32_t time_slice);

// open the file for writing, redirect stream I/O (printf) to this file
// if the file exists it will append to the end
// If the file doesn't exist, it will create a new file with the name
// input: an ASCII string up to seven characters
// returns: 0 if successful and 1 on failure (e.g., can't open)
int OS_RedirectToFile(const char* name);

// close the file for writing, redirect stream I/O (printf) back to the UART
// returns: 0 if successful and 1 on failure (e.g., trouble writing)
int OS_EndRedirectToFile(void);

// redirect stream I/O (printf) to the UART0
// returns: 0 if successful and 1 on failure
int OS_RedirectToUART(void);

// redirect stream I/O (printf) to the ST7735 LCD
// returns: 0 if successful and 1 on failure
int OS_RedirectToLCD(void);

// print jitter stats
void OS_ReportJitter(void);

int fputc(char ch);
