#include "OS.h"
#include "ST7735.h"
#include "interrupts.h"
#include "io.h"
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
uint32_t JitterHistogram[JITTERSIZE] = {0};

#define MAX_THREADS 8

static TCB threads[MAX_THREADS];

static TCB idle = {.next_tcb = &threads[0],
                   .prev_tcb = &threads[0],
                   .id = 0,
                   .dead = false,
                   .sp = &idle.stack[STACK_SIZE]};

TCB* current_thread = &idle;

// must be called from critical section
static void insert_thread(TCB* adding) {
    if (OS_Id()) {
        adding->next_tcb = current_thread->next_tcb;
        adding->prev_tcb = current_thread;
    } else {
        adding->prev_tcb = adding;
        adding->next_tcb = adding;
    }

    current_thread->next_tcb->prev_tcb = adding;
    current_thread->next_tcb = adding;
}

// must be called from critical section
static void remove_current_thread() {
    if (current_thread->next_tcb == current_thread &&
        current_thread->prev_tcb == current_thread) {
        current_thread = &idle;
    } else {
        current_thread->prev_tcb->next_tcb = current_thread->next_tcb;
        current_thread->next_tcb->prev_tcb = current_thread->prev_tcb;
    }
}

unsigned long OS_LockScheduler(void) {
    // lab 4 might need this for disk formating
    return 0; // replace with solution
}
void OS_UnLockScheduler(unsigned long previous) {
    // lab 4 might need this for disk formating
}

void OS_Init(void) {
    disable_interrupts();
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);
    for (int i = 0; i < MAX_THREADS; i++) { threads[i].dead = true; }
    launchpad_init();
    uart_init();
}

void OS_InitSemaphore(Sema4* sem, int32_t value) {
    sem->value = value;
    sem->blocked_head = 0;
}

void OS_Wait(Sema4* sem) {
    uint32_t crit = start_critical();
    if (--sem->value >= 0) {
        end_critical(crit);
        return;
    }
    if (!sem->blocked_head) {
        current_thread->next_blocked = 0;
        sem->blocked_head = current_thread;
    } else {
        TCB* tail = sem->blocked_head;
        while (tail->next_blocked) { tail = tail->next_blocked; }
        tail->next_blocked = current_thread;
    }
    remove_current_thread();
    end_critical(crit);
    OS_Suspend();
}

void OS_Signal(Sema4* sem) {
    uint32_t crit = start_critical();
    if (++sem->value >= 0) {
        end_critical(crit);
        return;
    }
    insert_thread(sem->blocked_head);
    sem->blocked_head = sem->blocked_head->next_blocked;
    end_critical(crit);
}

void OS_bWait(Sema4* sem) {
    uint32_t crit = start_critical();
    // TODO: lab 3 switch this to not spinlock
    while (sem->value) {
        end_critical(crit);
        OS_Suspend();
        crit = start_critical();
    }
    sem->value = -1;
    end_critical(crit);
}

void OS_bSignal(Sema4* sem) {
    sem->value = 0;
}

uint8_t thread_count = 0;
uint32_t thread_uuid = 1;
bool OS_AddThread(void (*task)(void), uint32_t stackSize, uint32_t priority) {
    uint32_t crit = start_critical();
    if (thread_count >= MAX_THREADS) {
        return false;
    }
    uint8_t thread_index = 0;
    while (!threads[thread_index].dead) { thread_index++; }
    TCB* adding = &threads[thread_index];

    adding->sleep = false;
    adding->sleep_time = 0;
    adding->id = thread_uuid++;
    adding->sp = adding->stack;

    insert_thread(adding);

    // initialize stack
    adding->sp = &adding->stack[STACK_SIZE - 1];
    *(--adding->sp) = 0x21000000;        // PSR
    *(--adding->sp) = (uint32_t)task;    // PC
    *(--adding->sp) = (uint32_t)OS_Kill; // LR
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
    adding->stack[0] = 0xaBad1dea; // TODO: use to detect stack overflow
    end_critical(crit);
    return true;
}

uint32_t OS_Id(void) {
    return current_thread->id;
}

#define MAX_PTASKS 1
PTask ptasks[MAX_PTASKS];
uint8_t num_ptasks = 0;

void periodic_task(void) {
    if (num_ptasks == 0) {
        return;
    }
    PTask task = ptasks[0];
    uint32_t reload = get_timer_reload(2);

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

bool OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                          uint32_t priority) {
    ptasks[num_ptasks].task = task;
    ptasks[num_ptasks].priority = priority;
    ptasks[num_ptasks].time = 0;
    ptasks[num_ptasks].reload = period;
    return true;
}

int OS_AddSW1Task(void (*task)(void), uint32_t priority) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

int OS_AddSW2Task(void (*task)(void), uint32_t priority) {
    // put Lab 2 (and beyond) solution here
    return 0;
}

static void sleep_task(void) {
    uint32_t reload = get_timer_reload(1);
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].sleep) {
            if (threads[i].sleep_time <= reload) {
                threads[i].sleep_time = 0;
                threads[i].sleep = false;
                insert_thread(&threads[i]);
            } else {
                threads[i].sleep_time -= reload;
            }
        }
    }
}

void OS_Sleep(uint32_t time) {
    uint32_t crit = start_critical();
    current_thread->sleep = true;
    current_thread->sleep_time =
        time + get_timer_reload(1) - get_timer_value(1);
    remove_current_thread();
    OS_Suspend();
    end_critical(crit);
}

void OS_Kill(void) {
    uint32_t crit = start_critical();
    current_thread->dead = true;
    remove_current_thread();
    OS_Suspend();
    end_critical(crit);
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

void OS_Launch(uint32_t time_slice) {
    ROM_IntPrioritySet(FAULT_PENDSV, 0xff); // priority is high 3 bits
    ROM_IntPendSet(FAULT_PENDSV);
    ROM_SysTickPeriodSet(time_slice);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();
    periodic_timer_enable(1, ms(1), &sleep_task, 3);
    periodic_timer_enable(2, us(100), &periodic_task, 1);
    enable_interrupts();
    while (true) {}
}

void systick_handler() {
    ROM_IntPendSet(FAULT_PENDSV);
}

void pendsv_handler(void) {
    disable_interrupts();
    __asm("PUSH    {R4 - R11}\n"
          "LDR     R0, =current_thread\n" // R0 = &current_thread
          "LDR     R1, [R0]\n"            // R1 = current_thread
          "STR     SP, [R1]\n"    // SP = *current_thread aka current_thread->sp
          "LDR     R1, [R1,#4]\n" // R1 = current_thread->next_tcb;
          "STR     R1, [R0]\n"    // current_thread = current_thread->next_tcb;
          "LDR     SP, [R1]\n"    // SP = current_thread->sp
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
