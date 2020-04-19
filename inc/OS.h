#include "timer.h"
#include <stdint.h>

struct TCB;
typedef struct {
    int32_t value; // >=0 means free, negative means busy
    struct TCB* blocked_head;
} Sema4;

// initialize OS controlled IO, timers, and heap
// disables interrupts until OS_Launch
void OS_Init(void);

// start the scheduler, enable interrupts
// inputs: number of cycles for each time slice
void OS_Launch(uint32_t time_slice);

// add a foregound thread to the scheduler
// inputs: pointer to a foreground task
//         string with a label for debugging
//         number of bytes allocated for its stack
//         priority (lower number = higher priority)
// returns: true if successful, false if this thread can not be added
bool OS_AddThread(void (*task)(void), const char* name, uint32_t stack_size,
                  uint32_t priority);

// add a background periodic task
// typically this function receives the highest priority
// inputs: pointer to a void/void background function
//         period in cycles
//         priority (lower number = higher priority)
// returns: true if successful, false if this thread can not be added
// This task can't block, but it can call OS_Signal or OS_AddThread
bool OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                          uint32_t priority);

void OS_ReportJitter(void); // print jitter stats for periodic threads

// add a background task to run whenever the SW1 (PF4) button is pushed
// This task can't block, but it can call OS_Signal or OS_AddThread
void OS_AddSW1Task(void (*task)(void));

// add a background task to run whenever the SW2 (PF0) button is pushed
// This task can't block, but it can call OS_Signal or OS_AddThread
void OS_AddSW2Task(void (*task)(void));

uint32_t OS_Time(void);  // return the system time in cycles
void OS_ClearTime(void); // sets the system time to zero

// set the starting value for a semaphore, 0 or more means available
void OS_InitSemaphore(Sema4* semaPt, int32_t value);
void OS_Wait(Sema4* semaPt);   // block until a semaphore is available
void OS_Signal(Sema4* semaPt); // increment semaphore value (may wake thread)

// returns the thread id for the currently running thread
uint32_t OS_Id(void);

// place this thread into a dormant state
// input: number of cycles to sleep
void OS_Sleep(uint32_t sleep_time);

void OS_Kill(void);    // kill the current thread, releasing its stack
void OS_Suspend(void); // suspend execution of current thread

bool OS_Fifo_Init(uint32_t size); // attempt initialize the fifo to be empty
bool OS_Fifo_Put(uint32_t data);  // attempt to add element
uint32_t OS_Fifo_Get(void);       // remove element (blocks if empty)
int32_t OS_Fifo_Size(void);       // get the number of elements in the fifo

void OS_MailBox_Init(void);          // initialize the mailbox to be empty
void OS_MailBox_Send(uint32_t data); // add data. blocks if full
uint32_t OS_MailBox_Recv(void);      // pull out data. blocks if empty
