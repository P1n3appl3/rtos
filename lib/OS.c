#include "OS.h"
#include "FIFO.h"
#include "ST7735.h"
#include "interrupts.h"
#include "io.h"
#include "launchpad.h"
#include "timer.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/hw_timer.h"
#include "tivaware/hw_types.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tivaware/timer.h"
#include <stdint.h>
#include <stdnoreturn.h>

// Performance Measurements
int32_t MaxJitter;
uint32_t JitterHistogram[128] = {0};

#define MAX_THREADS 8
#define STACK_SIZE 256

static TCB threads[MAX_THREADS];
static uint32_t stacks[MAX_THREADS][STACK_SIZE];
uint8_t thread_count = 0;

static uint32_t idle_stack[32];

static noreturn void idle_task(void) {
    while (1)
        ;
}

static TCB idle = {
    .next_tcb = &idle,
    .prev_tcb = &idle,
    .id = 0,
    .alive = true,
    .sp = &idle_stack[31],
};

TCB* current_thread = &idle;

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

uint32_t thread_uuid = 1;
bool OS_AddThread(void (*task)(void), uint32_t stackSize, uint32_t priority) {
    uint32_t crit = start_critical();
    if (thread_count >= MAX_THREADS - 1) {
        end_critical(crit);
        return false;
    }
    ++thread_count;
    uint8_t thread_index = 0;
    while (threads[thread_index].alive) { thread_index++; }
    TCB* adding = &threads[thread_index];

    adding->alive = true;
    adding->sleep = false;
    adding->sleep_time = 0;
    adding->id = thread_uuid++;

    // initialize stack
    adding->stack = stacks[thread_index];
    adding->sp = &adding->stack[STACK_SIZE - 1];
    adding->sp -= 18;                    // Space for floating point registers
    *(--adding->sp) = 0x21000000;        // PSR
    *(--adding->sp) = (uint32_t)task;    // PC
    *(--adding->sp) = (uint32_t)OS_Kill; // LR
    adding->sp -= 13;                    // Space for R0-R12

    insert_thread(adding);
    end_critical(crit);
    return true;
}

uint32_t OS_Id(void) {
    return current_thread->id;
}

typedef struct ptask {
    void (*task)(void);
    uint32_t reload;
    uint8_t priority;
} PTask;

#define MAX_PTASKS 8
PTask ptasks[MAX_PTASKS];
uint8_t num_ptasks;
PTask* current_ptask;

void periodic_task(void) {
    current_ptask->task();
}

bool OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                          uint32_t priority) {
    timer_enable(2, period, task, 1, true);
    return true;
    // rework for lab 3
    if (num_ptasks >= MAX_PTASKS) {
        return false;
    }
    PTask* current = ptasks;
    current->priority = priority;
    current->task = task;
    current->reload = period;
    if (!num_ptasks) {
        current_ptask = ptasks;
        timer_enable(2, period, periodic_task, 1, false);
        return true;
    }
    ptasks[num_ptasks++] = *current;
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
    current_thread->alive = false;
    remove_current_thread();
    OS_Suspend();
    end_critical(crit);
}

void OS_Suspend(void) {
    ROM_IntPendSet(FAULT_PENDSV);
}

static Sema4 fifo_data_available;
#define MAX_OS_FIFO 64
static uint32_t os_fifo_buf[MAX_OS_FIFO];
static uint8_t os_fifo_head;
static uint8_t os_fifo_tail;
static uint8_t os_fifo_size;
void OS_Fifo_Init(uint32_t size) {
    os_fifo_head = os_fifo_tail = 0;
    os_fifo_size = size + 1;
    OS_InitSemaphore(&fifo_data_available, -1);
}

bool OS_Fifo_Put(uint32_t data) {
    if (OS_Fifo_Size() == os_fifo_size - 1) {
        return false;
    }
    os_fifo_buf[os_fifo_head++] = data;
    os_fifo_head %= os_fifo_size;
    OS_Signal(&fifo_data_available);
    return true;
}

uint32_t OS_Fifo_Get(void) {
    OS_Wait(&fifo_data_available);
    uint32_t temp = os_fifo_buf[os_fifo_tail++];
    os_fifo_tail %= os_fifo_size;
    return temp;
}

int32_t OS_Fifo_Size(void) {
    return (os_fifo_size + os_fifo_head - os_fifo_tail) % os_fifo_size;
}

uint32_t mailbox_data;
Sema4 BoxFree, DataValid;
void OS_MailBox_Init(void) {
    OS_InitSemaphore(&BoxFree, 0);
    OS_InitSemaphore(&DataValid, -1);
}

void OS_MailBox_Send(uint32_t data) {
    OS_Wait(&BoxFree);
    mailbox_data = data;
    OS_Signal(&DataValid);
}

uint32_t OS_MailBox_Recv(void) {
    OS_Wait(&DataValid);
    uint32_t temp = mailbox_data;
    OS_Signal(&BoxFree);
    return temp;
}

uint32_t OS_TimeDifference(uint32_t a, uint32_t b) {
    return a < b ? b - a : a - b;
}

void OS_ClearTime(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER5);
    ROM_TimerConfigure(WTIMER5_BASE, TIMER_CFG_PERIODIC_UP);
    ROM_TimerControlStall(WTIMER5_BASE, TIMER_A, true);
    ROM_TimerEnable(WTIMER5_BASE, TIMER_A);
    HWREG(WTIMER5_BASE + TIMER_O_TAV) = 0;
}

uint32_t OS_Time(void) {
    return ROM_TimerValueGet(WTIMER5_BASE, TIMER_A);
}

void OS_Launch(uint32_t time_slice) {
    ROM_IntPrioritySet(FAULT_PENDSV, 0xff); // priority is high 3 bits
    ROM_IntPendSet(FAULT_PENDSV);
    ROM_SysTickPeriodSet(time_slice);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();
    timer_enable(1, time_slice, &sleep_task, 3, true);
    OS_ClearTime();
    // Set SP to idle's stack
    __asm("LDR R0, =idle\n"
          "LDR R0, [R0]\n"
          "MOV SP, R0\n");
    enable_interrupts();
    idle_task();
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
