#include "timer.h"
#include <stdint.h>

#pragma once

#define TRACK_JITTER

struct TCB;
typedef struct {
    int32_t value; // >=0 means free, negative means busy
    struct TCB* blocked_head;
} Sema4;

// initialize OS controlled IO, timers, and heap
// disables interrupts until OS_Launch
void OS_Init(void);

// start the scheduler, enable interrupts
// time_slice is the number of cycles between context switches
void OS_Launch(uint32_t time_slice);

// add a foregound task to the scheduler
// name is a static string label for the thread used for debugging
// stack_size is the number of bytes to allocate for the task's stack
// lower numbers = higher priority
// returns false if this thread can not be added
bool OS_AddThread(void (*task)(void), const char* name, uint32_t stack_size,
                  uint32_t priority);

// add a background periodic task
// the task can't block, but it can call OS_Signal or OS_AddThread
// period is in cycles
// lower numbers = higher priority
// returns false if this thread can not be added
bool OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                          uint32_t priority);

void OS_ReportJitter(void); // print jitter stats for periodic threads

// add a background task to run whenever the SW1 (PF4) button is pushed
// the task can't block, but it can call OS_Signal or OS_AddThread
void OS_AddSW1Task(void (*task)(void));

// add a background task to run whenever the SW2 (PF0) button is pushed
// the task can't block, but it can call OS_Signal or OS_AddThread
void OS_AddSW2Task(void (*task)(void));

uint32_t OS_Id(void);    // returns a unique id for the current_thread
uint32_t OS_Time(void);  // return the system time in cycles
void OS_ClearTime(void); // sets the system time to zero

// suspend the current thread for AT LEAST a given number of cycles
void OS_Sleep(uint32_t time);
void OS_Suspend(void); // suspend the current thread
void OS_Kill(void);    // kill the current thread, releasing its stack

// set the starting value for a semaphore, 0 or more means available
void OS_InitSemaphore(Sema4* semaPt, int32_t value);
void OS_Wait(Sema4* semaPt);   // block until a semaphore is available
void OS_Signal(Sema4* semaPt); // increment semaphore value (may wake thread)

// These are used to dynamically load user code
bool OS_AddProcess(void (*entry)(void), void* text, void* data,
                   uint32_t stack_size, uint32_t priority);
void OS_LoadProgram(char* name);

typedef enum {
    UART = 1,
    ESP = 2,
    FS = 4,
    SCREEN = 8,
} OutputDevice;

void OS_RedirectOutput(OutputDevice device);
void OS_RedirectString(const char* str);
void OS_RedirectChar(char c);
