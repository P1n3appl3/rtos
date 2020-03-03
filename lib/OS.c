#include "OS.h"
#include "FIFO.h"
#include "ST7735.h"
#include "interrupts.h"
#include "io.h"
#include "launchpad.h"
#include "tcb.h"
#include "timer.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/hw_timer.h"
#include "tivaware/hw_types.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tivaware/timer.h"
#include <stdint.h>

// Performance Measurements
int32_t MaxJitter; // largest time jitter between interrupts
#define JITTERSIZE 256
uint32_t const JitterSize = JITTERSIZE;
uint32_t JitterHistogram[JITTERSIZE] = {0};

#define MAX_THREADS 8

static TCB threads[MAX_THREADS];
uint8_t thread_count = 0;

static TCB idle = {.next_tcb = &threads[0],
                   .prev_tcb = &threads[0],
                   .id = 0,
                   .dead = false,
                   .sp = &idle.stack[STACK_SIZE]};

TCB* current_thread = &idle;

ADDFIFO(os, 16, uint32_t)

// must be called from critical section
static void insert_thread(TCB* adding) {
    if (thread_count > 0) {
        adding->next_tcb = current_thread->next_tcb->next_tcb;
        adding->prev_tcb = current_thread->next_tcb;
    } else {
        adding->prev_tcb = adding;
        adding->next_tcb = adding;
    }
    current_thread->next_tcb->next_tcb->prev_tcb = adding;
    current_thread->next_tcb->next_tcb = adding;
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
    ST7735_InitR(INITR_REDTAB);
}

void OS_InitSemaphore(Sema4* sem, int32_t value) {
    sem->value = value;
    sem->blocked_head = 0;
}

void OS_Wait(Sema4* sem) {
    uint32_t crit = start_critical();
    while (sem->value < 0) {
        end_critical(crit);
        OS_Suspend();
        crit = start_critical();
    }
    --sem->value;
    end_critical(crit);
    return;
    // TODO: use this stuff for lab 3
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
    ++sem->value;
    end_critical(crit);
    return;
    // TODO: use this stuff for lab 3
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
    while (sem->value < 0) {
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

uint32_t thread_uuid = 1;
bool OS_AddThread(void (*task)(void), uint32_t stackSize, uint32_t priority) {
    uint32_t crit = start_critical();
    if (thread_count >= MAX_THREADS - 1) {
        end_critical(crit);
        return false;
    }
    ++thread_count;
    uint8_t thread_index = 0;
    while (!threads[thread_index].dead) { thread_index++; }
    TCB* adding = &threads[thread_index];

    adding->dead = false;
    adding->sleep = false;
    adding->sleep_time = 0;
    adding->id = thread_uuid++;

    insert_thread(adding);

    // initialize stack
    adding->sp = &adding->stack[STACK_SIZE - 1];
    adding->sp -= 18;                    // Space for floating point registers
    *(--adding->sp) = 0x21000000;        // PSR
    *(--adding->sp) = (uint32_t)task;    // PC
    *(--adding->sp) = (uint32_t)OS_Kill; // LR
    *(--adding->sp) = 12;
    *(--adding->sp) = 3;
    *(--adding->sp) = 2;
    *(--adding->sp) = 1;
    *(--adding->sp) = 0;
    *(--adding->sp) = 11;
    *(--adding->sp) = 10;
    *(--adding->sp) = 9;
    *(--adding->sp) = 8;
    *(--adding->sp) = 7;
    *(--adding->sp) = 6;
    *(--adding->sp) = 5;
    *(--adding->sp) = 4;
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
PTask* phead = 0;

void plist_insert(PTask* task) {
    if (phead == 0) {
        phead = task;
    } else if (task->priority < phead->priority) {
        task->next = phead;
        phead = task;
    } else if (num_ptasks == 1) {
        phead->next = task;
    } else {
        PTask* ptr = phead;
        while (ptr->next->priority < task->priority) { ptr = ptr->next; }
        task->next = ptr->next;
        ptr->next = task;
    }
}

void periodic_task(void) {
    if (num_ptasks == 0) {
        return;
    }
    uint32_t reload = get_timer_reload(2);
    phead = 0;

    for (uint8_t i = 0; i < num_ptasks; i++) {
        ptasks[i].next = 0;
        if (ptasks[i].time <= reload) {
            ptasks[i].time = ptasks[i].reload;
            plist_insert(&ptasks[i]);
        } else {
            ptasks[i].time -= reload;
        }
    }

    PTask* task = phead;
    while (task != 0) {
        (task->task)();
        task = task->next;
    }
}

bool OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                          uint32_t priority) {
    ptasks[num_ptasks].task = task;
    ptasks[num_ptasks].priority = priority;
    ptasks[num_ptasks].time = period;
    ptasks[num_ptasks].next = 0;
    ptasks[num_ptasks].reload = period;
    num_ptasks++;
    return true;
}

bool OS_AddSW1Task(void (*task)(void), uint32_t priority) {
    switch1_init(task, priority);
    return true;
}

bool OS_AddSW2Task(void (*task)(void), uint32_t priority) {
    switch2_init(task, priority);
    return true;
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
    --thread_count;
    current_thread->dead = true;
    remove_current_thread();
    OS_Suspend();
    end_critical(crit);
}

void OS_Suspend(void) {
    ROM_IntPendSet(FAULT_PENDSV);
}

Sema4 FifoAvailable;
void OS_Fifo_Init(uint32_t size) {
    OS_InitSemaphore(&FifoAvailable, -1);
    osfifo_init();
}

bool OS_Fifo_Put(uint32_t data) {
    if (osfifo_full()) {
        return false;
    }
    osfifo_put(data);
    OS_Signal(&FifoAvailable);
    return true;
}

uint32_t OS_Fifo_Get(void) {
    OS_Wait(&FifoAvailable);
    uint32_t temp;
    osfifo_get(&temp);
    return temp;
}

int32_t OS_Fifo_Size(void) {
    return osfifo_size();
}

uint32_t MailBox;
Sema4 BoxFree, DataValid;

void OS_MailBox_Init(void) {
    OS_InitSemaphore(&BoxFree, 0);
    OS_InitSemaphore(&DataValid, -1);
}

void OS_MailBox_Send(uint32_t data) {
    OS_bWait(&BoxFree);
    MailBox = data;
    OS_bSignal(&DataValid);
}

uint32_t OS_MailBox_Recv(void) {
    OS_bWait(&DataValid);
    uint32_t temp = MailBox;
    OS_bSignal(&BoxFree);
    return temp;
}

uint32_t OS_TimeDifference(uint32_t start, uint32_t stop) {
    return stop - start;
}

void OS_ClearTime(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER5);
    ROM_TimerConfigure(WTIMER5_BASE, TIMER_CFG_PERIODIC_UP);
    ROM_TimerControlStall(WTIMER5_BASE, TIMER_A, true);
    // TODO: figure out why this doesn't work
    // ROM_TimerPrescaleSet(WTIMER5_BASE, TIMER_A, SYSTEM_TIME_DIV);
    ROM_TimerEnable(WTIMER5_BASE, TIMER_A);
    // TODO: figure out why the rom function doesn't work for clearing the value
    // ROM_TimerLoadSet(WTIMER5_BASE, TIMER_A, 0);
    HWREG(WTIMER5_BASE + TIMER_O_TAV) = 0;
}

uint32_t OS_Time(void) {
    return ROM_TimerValueGet(WTIMER5_BASE, TIMER_A) / SYSTEM_TIME_DIV;
}

void OS_Launch(uint32_t time_slice) {
    ROM_IntPrioritySet(FAULT_PENDSV, 0xff); // priority is high 3 bits
    ROM_IntPendSet(FAULT_PENDSV);
    ROM_SysTickPeriodSet(time_slice * SYSTEM_TIME_DIV);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();
    timer_enable(1, ms(1), &sleep_task, 3, true);
    timer_enable(2, us(100), &periodic_task, 2, true);
    OS_ClearTime();
    // Set SP to idle's stack
    __asm("LDR R0, =idle\n"
          "LDR R0, [R0]\n"
          "MOV SP, R0\n");
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
