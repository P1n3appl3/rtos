#include "CortexM.h"
#include "LaunchPad.h"
#include "PLL.h"
#include "RTOS_Labs_common/ST7735.h"
#include "RTOS_Labs_common/UART0int.h"
#include "RTOS_Labs_common/eFile.h"
#include "Timer4A.h"
#include "inc/ADCT0ATrigger.h"
#include "timer.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/sysctl.h"
#include "tivaware/timer.h"
#include <OS.h>
#include <stdint.h>
#include <stdio.h>

// Performance Measurements
int32_t MaxJitter; // largest time jitter between interrupts in usec
#define JITTERSIZE 64
uint32_t const JitterSize = JITTERSIZE;
uint32_t JitterHistogram[JITTERSIZE] = {
    0,
};

/*------------------------------------------------------------------------------
  Systick Interrupt Handler
  SysTick interrupt happens every 10 ms
  used for preemptive thread switch
 *------------------------------------------------------------------------------*/
void SysTick_Handler(void) {} // end SysTick_Handler

unsigned long OS_LockScheduler(void) {
  // lab 4 might need this for disk formating
  return 0; // replace with solution
}
void OS_UnLockScheduler(unsigned long previous) {
  // lab 4 might need this for disk formating
}

void SysTick_Init(unsigned long period) {}

/**
 * @details  Initialize operating system, disable interrupts until OS_Launch.
 * Initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers.
 * Interrupts not yet enabled.
 * @param  none
 * @return none
 * @brief  Initialize OS
 */
void OS_Init(void){
    // put Lab 2 (and beyond) solution here

};

// ******** OS_InitSemaphore ************
// initialize semaphore
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, int32_t value){
    // put Lab 2 (and beyond) solution here

};

// ******** OS_Wait ************
// decrement semaphore
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt){
    // put Lab 2 (and beyond) solution here

};

// ******** OS_Signal ************
// increment semaphore
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt){
    // put Lab 2 (and beyond) solution here

};

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt){
    // put Lab 2 (and beyond) solution here

};

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){
    // put Lab 2 (and beyond) solution here

};

//******** OS_AddThread ***************
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThread(void (*task)(void), uint32_t stackSize, uint32_t priority) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

//******** OS_Id ***************
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero
uint32_t OS_Id(void) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

//******** OS_AddPeriodicThread ***************
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal   OS_AddThread
// This task does not have a Thread ID
// In lab 1, this command will be called 1 time
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority
// field
//           determines the relative priority of these four threads
int OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                         uint32_t priority) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

/*----------------------------------------------------------------------------
  PF1 Interrupt Handler
 *----------------------------------------------------------------------------*/
void GPIOPortF_Handler(void) {}
//******** OS_AddSW1Task ***************
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal   OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority
// field
//           determines the relative priority of these four threads
int OS_AddSW1Task(void (*task)(void), uint32_t priority) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

//******** OS_AddSW2Task ***************
// add a background task to run whenever the SW2 (PF0) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority
// field
//           determines the relative priority of these four threads
int OS_AddSW2Task(void (*task)(void), uint32_t priority) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(uint32_t sleepTime){
    // put Lab 2 (and beyond) solution here

};

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void) {
  // put Lab 2 (and beyond) solution here

  EnableInterrupts(); // end of atomic section
  for (;;) {
  }; // can not return
};

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void){
    // put Lab 2 (and beyond) solution here

};

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(uint32_t size){
    // put Lab 2 (and beyond) solution here

};

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers
//  this function can not disable or enable interrupts
int OS_Fifo_Put(uint32_t data) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data
uint32_t OS_Fifo_Get(void) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
int32_t OS_Fifo_Size(void) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void){
    // put Lab 2 (and beyond) solution here

    // put solution here
};

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received
void OS_MailBox_Send(uint32_t data){
    // put Lab 2 (and beyond) solution here
    // put solution here

};

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty
uint32_t OS_MailBox_Recv(void) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

// ******** OS_Time ************
// return the system time
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32
// bits It is ok to change the resolution and precision of this function as long
// as
//   this function and OS_TimeDifference have the same resolution and precision
uint32_t OS_Time(void) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units
// The time resolution should be less than or equal to 1us, and the precision at
// least 12 bits It is ok to change the resolution and precision of this
// function as long as
//   this function and OS_Time have the same resolution and precision
uint32_t OS_TimeDifference(uint32_t start, uint32_t stop) {
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

uint32_t CURRENT_MS;

void increment_global_time(void) {}

void OS_ClearMsTime(void) {
  periodic_timer_enable(5, ms(1), increment_global_time, );
  CURRENT_MS = 0;
}

uint32_t OS_MsTime(void) { return CURRENT_MS; }

//******** OS_Launch ***************
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(uint32_t theTimeSlice){
    // put Lab 2 (and beyond) solution here

};

//******** I/O Redirection ***************
// redirect terminal I/O to UART

int fputc(int ch, FILE *f) {
  UART_OutChar(ch);
  return ch;
}

int fgetc(FILE *f) {
  char ch = UART_InChar(); // receive from keyboard
  UART_OutChar(ch);        // echo
  return ch;
}
int OS_RedirectToFile(char *name) { return 1; }
int OS_RedirectToUART(void) { return 1; }

int OS_RedirectToST7735(void) { return 1; }

int OS_EndRedirectToFile(void) { return 1; }
