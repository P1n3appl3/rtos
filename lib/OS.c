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

#define PD1 (*((volatile uint32_t*)0x40007008))

// Performance Measurements
int32_t MaxJitter; // largest time jitter between interrupts in usec
#define JITTERSIZE 64
uint32_t const JitterSize = JITTERSIZE;
uint32_t JitterHistogram[JITTERSIZE] = {
    0,
};

#define MAX_THREADS 3

TCB threads[MAX_THREADS];

TCB idle = {.next_tcb = &threads[0],
            .prev_tcb = &threads[0],
            .id = 255,
            .sleep = true,
            .sleep_time = 0,
            .dead = false,
            .sp = &idle.stack[STACK_SIZE]};

TCB* run_tcb = &idle;

unsigned long OS_LockScheduler(void) {
    // lab 4 might need this for disk formating
    return 0; // replace with solution
}
void OS_UnLockScheduler(unsigned long previous) {
    // lab 4 might need this for disk formating
}

void OS_Init(void) {
    // put Lab 2 (and beyond) solution here
    for (int i = 0; i < MAX_THREADS; i++) { threads[i].dead = true; }
}

void OS_InitSemaphore(Sema4Type* semaPt, int32_t value) {
    semaPt->Value = value;
    semaPt->blocked_head = 0;
}

void OS_Wait(Sema4Type* semaPt) {
    uint32_t crit = start_critical();
    if (--semaPt->Value >= 0) {
        return;
    }
    if (!semaPt->blocked_head) {
        run_tcb->next_blocked = 0;
        semaPt->blocked_head = run_tcb;
    } else {
        TCB* current = semaPt->blocked_head;
        while (current->next_blocked) { current = current->next_blocked; }
        current->next_blocked = run_tcb;
    }
    // TODO: remove from linked list
    end_critical(crit);
}

void OS_Signal(Sema4Type* semaPt) {
    uint32_t crit = start_critical();
    if (++semaPt->Value > 0) {
        return;
    }
    // TODO: add from linked list
    end_critical(crit);
}

void OS_bWait(Sema4Type* semaPt) {
    uint32_t crit = start_critical();
    while (!semaPt->Value) { OS_Suspend(); }
    semaPt->Value = 0;
    end_critical(crit);
}

void OS_bSignal(Sema4Type* semaPt) {
    uint32_t crit = start_critical();
    semaPt->Value = 1;
    end_critical(crit);
}

void dead(void) {
    while (1) {}
}

uint8_t thread_count = 0;
int OS_AddThread(void (*task)(void), uint32_t stackSize, uint32_t priority) {
    if (thread_count >= MAX_THREADS) {
        return 0;
    }
    uint8_t thread_index = 0;
    while (threads[thread_index].dead == false) { thread_index++; }
    TCB* adding = &threads[thread_index];

    adding->sleep = false;
    adding->sleep_time = 0;
    adding->id = thread_index;
    adding->sp = adding->stack;

    adding->next_tcb = run_tcb->next_tcb;
    run_tcb->next_tcb = adding;
    adding->next_tcb->prev_tcb = adding;

    if (run_tcb->id == 255) {
        adding->prev_tcb = run_tcb->prev_tcb;
    } else {
        adding->prev_tcb = run_tcb;
    }

    // initialize stack
    *(--adding->sp) = 0x21000000;     // PSR
    *(--adding->sp) = (uint32_t)task; // PC
    *(--adding->sp) = (uint32_t)dead; // LR
    *(--adding->sp) = 12;
    *(--adding->sp) = 3;
    *(--adding->sp) = 2;
    *(--adding->sp) = 1;
    *(--adding->sp) = 0;
    *(--adding->sp) = 4;
    *(--adding->sp) = 5;
    *(--adding->sp) = 6;
    *(--adding->sp) = 7;
    *(--adding->sp) = 8;
    *(--adding->sp) = 9;
    *(--adding->sp) = 10;
    *(--adding->sp) = 11;
    return 0;
}

uint32_t OS_Id(void) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

#define MAX_PTASKS 1
ptask ptasks[MAX_PTASKS];
uint8_t num_ptasks = 0;
void periodic_task(void) {
    if (num_ptasks == 0) {
        return;
    }

    ptask task = ptasks[0];
    uint32_t reload = timer_load(2);

    for (uint8_t i = 0; i < num_ptasks; i++) {
        if (ptasks[i].time <= reload) {
            if ((ptasks[i].time <= reload) &&
                (ptasks[i].priority < task.priority)) {
                ptasks[i].time = ptasks[i].reload;
                task = ptasks[i];
            }
        }
    }
    (task.task)();
}
int OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                         uint32_t priority) {
    ptasks[num_ptasks].task = task;
    ptasks[num_ptasks].priority = priority;
    ptasks[num_ptasks].time = 0;
    ptasks[num_ptasks].reload = period;

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

void sleep_task(void) {
    uint32_t reload = timer_load(1);
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].sleep) {
            if (threads[i].sleep_time <= reload) {
                threads[i].sleep_time = 0;
                threads[i].sleep = false;
            } else {
                threads[i].sleep_time -= reload;
            }
        }
    }
}

void OS_Sleep(uint32_t sleepTime) {
    run_tcb->sleep = true;
    run_tcb->prev_tcb->next_tcb = run_tcb->next_tcb;
    run_tcb->next_tcb->prev_tcb = run_tcb->prev_tcb;
    run_tcb->sleep_time = sleepTime;
    OS_Suspend();
}

void OS_Kill(void) {
    disable_interrupts();
    run_tcb->dead = true;
    run_tcb->prev_tcb->next_tcb = run_tcb->next_tcb;
    run_tcb->next_tcb->prev_tcb = run_tcb->prev_tcb;
    enable_interrupts(); // end of atomic section
    for (;;) {};         // can not return
}

void OS_Suspend(void) {
    ROM_IntPendSet(FAULT_PENDSV);
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
    // preemptive
    ROM_IntPrioritySet(FAULT_PENDSV, 0xff); // priority is high 3 bits
    ROM_IntPendSet(FAULT_PENDSV);
    periodic_timer_enable(1, ms(1), &sleep_task, 3);
    periodic_timer_enable(2, us(100), &periodic_task, 1);
    ROM_SysTickPeriodSet(theTimeSlice);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();
    enable_interrupts();
    while (1) {}
}

void systick_handler() {
    // PD1 ^= 2;
    ROM_IntPendSet(FAULT_PENDSV);
}

void pendsv_handler(void) {
    disable_interrupts();
    __asm("PUSH    {R4 - R11}\n"
          "LDR     R0, =run_tcb\n" // R0 = &run_tcb
          "LDR     R1, [R0]\n"     // R1 = run_tcb
          "STR     SP, [R1]\n"     // SP = *run_tcb aka run_tcb->sp
          "LDR     R1, [R1,#4]\n"  // R1 = run_tcb->next_tcb;
          "STR     R1, [R0]\n"     // run_tcb = run_tcb->next_tcb;
          "LDR     SP, [R1]\n"     // SP = run_tcb->sp
          "POP     {R4 - R11}");
    enable_interrupts();
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
