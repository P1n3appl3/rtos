#include "OS.h"
#include "FIFO.h"
#include "ST7735.h"
#include "interrupts.h"
#include "io.h"
#include "launchpad.h"
#include "printf.h"
#include "std.h"
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
static uint8_t thread_count = 0;

static bool os_running;

#define IDLE_STACK_SIZE 64
static uint32_t idle_stack[IDLE_STACK_SIZE];
static TCB idle = {
    .next_tcb = &idle,
    .prev_tcb = &idle,
    .id = 0,
    .alive = true,
    .priority = 255,
    .name = "Idle",
    .stack = idle_stack,
    .sp = &idle_stack[IDLE_STACK_SIZE - 1],
};

static noreturn void idle_task(void) {
    os_running = true;
    enable_interrupts();
    while (true) {
        idle.next_tcb = &idle;
        wait_for_interrupts();
    }
}

static TCB* current_thread = &idle;

static void insert_behind(TCB* a, TCB* b) {
    a->next_tcb = b;
    a->prev_tcb = b->prev_tcb;
    a->prev_tcb->next_tcb = a;
    b->prev_tcb = a;
}

static void insert_thread(TCB* adding) {
    uint32_t crit = start_critical();
    if (adding->priority < current_thread->priority) {
        TCB* next = current_thread->next_tcb;
        if (next->priority > adding->priority) {
            adding->next_tcb = adding->prev_tcb = adding;
            current_thread->next_tcb = adding;
            OS_Suspend();
        } else if (next->priority == adding->priority) {
            insert_behind(adding, next);
        }
    } else if (adding->priority == current_thread->priority) {
        insert_behind(adding, current_thread);
    }
    end_critical(crit);
}

// only called from sleep/kill/suspend so no additional critical section needed
static void remove_current_thread() {
    if (current_thread->next_tcb == current_thread &&
        current_thread->prev_tcb == current_thread) {
        uint8_t my_count = thread_count;
        TCB* new_current = &idle;
        for (int i = 0; my_count > 0; ++i) {
            if (!threads[i].alive) {
                continue;
            }
            --my_count;
            if (!(threads[i].asleep || threads[i].blocked)) {
                if (threads[i].priority < new_current->priority) {
                    new_current = &threads[i];
                    new_current->next_tcb = new_current->prev_tcb = new_current;
                } else if (threads[i].priority == new_current->priority) {
                    insert_behind(&threads[i], new_current);
                }
            }
            current_thread->next_tcb = new_current;
        }
    } else {
        current_thread->prev_tcb->next_tcb = current_thread->next_tcb;
        current_thread->next_tcb->prev_tcb = current_thread->prev_tcb;
    }
    OS_Suspend();
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
    current_thread->next_blocked = 0;
    if (sem->value-- >= 0) {
        end_critical(crit);
        return;
    }
    current_thread->blocked = true;
    if (!sem->blocked_head) {
        sem->blocked_head = current_thread;
    } else {
        TCB* tail = sem->blocked_head;
        while (tail->next_blocked &&
               tail->next_blocked->priority <= current_thread->priority) {
            tail = tail->next_blocked;
        }
        current_thread->next_blocked = tail->next_blocked;
        tail->next_blocked = current_thread;
    }
    remove_current_thread();
    end_critical(crit);
}

void OS_Signal(Sema4* sem) {
    uint32_t crit = start_critical();
    if (++sem->value >= 0) {
        end_critical(crit);
        return;
    }
    insert_thread(sem->blocked_head);
    sem->blocked_head->blocked = false;
    if (sem->blocked_head->priority > current_thread->priority) {
        OS_Suspend();
    }
    sem->blocked_head = sem->blocked_head->next_blocked;
    end_critical(crit);
}

static uint32_t thread_uuid = 1;
bool OS_AddThread(void (*task)(void), const char* name, uint32_t stackSize,
                  uint32_t priority) {
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
    adding->asleep = false;
    adding->sleep_time = 0;
    adding->priority = priority;
    adding->id = thread_uuid++;
    adding->name = name;

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
    struct ptask* next;
    uint32_t reload;
    uint32_t current;
    uint32_t last;
    uint8_t priority;
} PTask;

#define MAX_PTASKS 4
static PTask ptasks[MAX_PTASKS];
static uint8_t num_ptasks;
static PTask* current_ptask;

static void setup_next_ptask(uint32_t lag);

static void ptask_insert(PTask* task) {
    if (!current_ptask) {
        current_ptask = task;
        task->next = 0;
        return;
    }
    if (task->priority < current_ptask->priority) {
        task->next = current_ptask;
        current_ptask = task;
    }
    PTask* temp = current_ptask;
    while (temp->next && task->priority > temp->next->priority) {
        temp = temp->next;
    }
    task->next = temp->next;
    temp->next = task;
}

static void periodic_task(void) {
    uint32_t time = OS_Time();
    do {
        uint32_t current = OS_Time();
        uint32_t jitter = difference(difference(current, current_ptask->last),
                                     current_ptask->reload);
        MaxJitter = max(MaxJitter, jitter);
        uint8_t idx =
            min(sizeof(JitterHistogram) / sizeof(JitterHistogram[0]) - 1,
                jitter / 8000);
        ++JitterHistogram[idx];
        current_ptask->last = current;
        current_ptask->task();
    } while ((current_ptask = current_ptask->next));
    setup_next_ptask(OS_Time() - time);
}

static void setup_next_ptask(uint32_t lag) {
    int32_t min_time = INT32_MAX;
    for (int i = 0; i < num_ptasks; ++i) {
        min_time = min(min_time, ptasks[i].current);
    }
    min_time = max(min_time, lag);
    for (int i = 0; i < num_ptasks; ++i) {
        if (ptasks[i].current <= min_time) {
            ptask_insert(&ptasks[i]);
            ptasks[i].current = ptasks[i].reload;
        } else {
            ptasks[i].current -= min_time;
        }
    }
    timer_enable(2, min_time, periodic_task, 1, false);
}

bool OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                          uint32_t priority) {
    if (num_ptasks >= MAX_PTASKS - 1) {
        return false;
    }
    PTask* current = &ptasks[num_ptasks++];
    current->priority = priority;
    current->task = task;
    current->current = current->reload = period;
    // if the first periodic task is added after the OS starts, the oneshot
    // timer isn't running yet
    if (num_ptasks == 1 && os_running) {
        setup_next_ptask(0);
    }
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
        if (threads[i].asleep) {
            if (threads[i].sleep_time <= reload) {
                threads[i].sleep_time = 0;
                threads[i].asleep = false;
                insert_thread(&threads[i]);
            } else {
                threads[i].sleep_time -= reload;
            }
        }
    }
}

void OS_Sleep(uint32_t time) {
    uint32_t crit = start_critical();
    current_thread->asleep = true;
    current_thread->sleep_time =
        time + get_timer_reload(1) - get_timer_value(1);
    remove_current_thread();
    end_critical(crit);
}

void OS_Kill(void) {
    uint32_t crit = start_critical();
    --thread_count;
    current_thread->alive = false;
    remove_current_thread();
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

noreturn void OS_Launch(uint32_t time_slice) {
    ROM_IntPrioritySet(FAULT_PENDSV, 0xff); // priority is high 3 bits
    ROM_IntPrioritySet(FAULT_SYSTICK, 0xff);
    ROM_IntPendSet(FAULT_PENDSV);
    ROM_SysTickPeriodSet(time_slice);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();
    timer_enable(1, ms(1), &sleep_task, 3, true);
    if (num_ptasks) {
        setup_next_ptask(0);
    }
    OS_ClearTime();
    // Set SP to idle's stack and run the idle task
    __asm("LDR R0, =idle");
    __asm("LDR R0, [R0]");
    __asm("MOV SP, R0");
    idle_task();
}

void systick_handler() {
    ROM_IntPendSet(FAULT_PENDSV);
}

void pendsv_handler(void) {
    disable_interrupts();
    __asm("PUSH {R4 - R11}");
    __asm("LDR  R0, =current_thread"); // R0 = &current_thread
    __asm("LDR  R1, [R0]");            // R1 = current_thread
    __asm("STR  SP, [R1]");    // SP = *current_thread aka current_thread->sp
    __asm("LDR  R1, [R1,#4]"); // R1 = current_thread->next_tcb;
    __asm("STR  R1, [R0]");    // current_thread = current_thread->next_tcb;
    __asm("LDR  SP, [R1]");    // SP = current_thread->sp
    __asm("POP  {R4 - R11}");
    enable_interrupts();
}

void OS_ReportJitter(void) {
    printf("Max Jitter: %d\n\r", MaxJitter);
    uint32_t most = 0, most_idx;
    for (int i = 0; i < sizeof(JitterHistogram) / sizeof(JitterHistogram[0]);
         ++i) {
        uint32_t num = JitterHistogram[i];
        if (num > most) {
            most = num;
            most_idx = i;
        }
    }
    printf("Modal Jitter: %d\n\r", most_idx);
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
