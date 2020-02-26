#pragma once
#include <stdint.h>
#include <tcb.h>

typedef struct {
    int32_t value; // >0 means free, otherwise means busy
    TCB* blocked_head;
} Sema4;

typedef struct {
    void (*task)(void);
    uint32_t time;
    uint32_t reload;
    uint32_t priority;
} PTask;

// Initialize operating system, disable interrupts until OS_Launch.
// Initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers.
// Interrupts not yet enabled.
void OS_Init(void);

void OS_InitSemaphore(Sema4* semaPt, int32_t value);

// decrement semaphore
// Lab2 spinlock
// Lab3 block if less than zero
// input: pointer to a counting semaphore
void OS_Wait(Sema4* semaPt);

// increment semaphore
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate
// input:  pointer to a counting semaphore
void OS_Signal(Sema4* semaPt);

// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input: pointer to a binary semaphore
void OS_bWait(Sema4* semaPt);

// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate
// input: pointer to a binary semaphore
void OS_bSignal(Sema4* semaPt);

// add a foregound thread to the scheduler
// inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// returns: true if successful, false if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
bool OS_AddThread(void (*task)(void), uint32_t stackSize, uint32_t priority);

// returns the thread ID for the currently running thread
// returns: Thread ID, number greater than zero
uint32_t OS_Id(void);

// add a background periodic task
// typically this function receives the highest priority
// inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// returns: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal   OS_AddThread
// This task does not have a Thread ID
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority
// field determines the relative priority of these four threads
bool OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                          uint32_t priority);

// add a background task to run whenever the SW1 (PF4) button is pushed
// inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// returns: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal   OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority
// field determines the relative priority of these four threads
int OS_AddSW1Task(void (*task)(void), uint32_t priority);

// add a background task to run whenever the SW2 (PF0) button is pushed
// inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// returns: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority
// field determines the relative priority of these four threads
int OS_AddSW2Task(void (*task)(void), uint32_t priority);

// place this thread into a dormant state
// input: number of msec to sleep
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(uint32_t sleepTime);

// kill the currently running thread, release its TCB and stack
void OS_Kill(void);

// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking
// Same function as OS_Sleep(0)
void OS_Suspend(void);

// temporarily prevent foreground thread switch (but allow background
// interrupts)
unsigned long OS_LockScheduler(void);
// resume foreground thread switching
void OS_UnLockScheduler(unsigned long previous);

// Initialize the Fifo to be empty
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(uint32_t size);

// Enter one data sample into the Fifo
// Called from the background, so no waiting
// returns: true if data is properly saved, false if data not saved, because it
// was full
// Since this is called by interrupt handlers this function can not
// disable or enable interrupts
int OS_Fifo_Put(uint32_t data);

// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// returns: data
uint32_t OS_Fifo_Get(void);

// Check the status of the Fifo
// returns: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
int32_t OS_Fifo_Size(void);

// Initialize communication channel
void OS_MailBox_Init(void);

// enter mail into the MailBox
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received
void OS_MailBox_Send(uint32_t data);

// remove mail from the MailBox
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty
uint32_t OS_MailBox_Recv(void);

// return the system time
// resolution is 100ns
uint32_t OS_Time(void);

// Calculates difference between two times
// inputs: two times measured with OS_Time
// returns: time difference in cycles
// resolution is 100ns
uint32_t OS_TimeDifference(uint32_t start, uint32_t stop);

// sets the system time to zero
void OS_ClearMsTime(void);

// reads the current time in ms
uint32_t OS_MsTime(void);

// start the scheduler, enable interrupts
// inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// returns: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(uint32_t theTimeSlice);

// open the file for writing, redirect stream I/O (printf) to this file
// if the file exists it will append to the end
// If the file doesn't exist, it will create a new file with the name
// input: an ASCII string up to seven characters
// returns: 0 if successful and 1 on failure (e.g., can't open)
int OS_RedirectToFile(char* name);

// close the file for writing, redirect stream I/O (printf) back to the UART
// returns: 0 if successful and 1 on failure (e.g., trouble writing)
int OS_EndRedirectToFile(void);

// redirect stream I/O (printf) to the UART0
// returns: 0 if successful and 1 on failure
int OS_RedirectToUART(void);

// redirect stream I/O (printf) to the ST7735 LCD
// returns: 0 if successful and 1 on failure
int OS_RedirectToST7735(void);
