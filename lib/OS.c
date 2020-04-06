#include "OS.h"
#include "FIFO.h"
#include "ST7735.h"
#include "eDisk.h"
#include "filesystem.h"
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
static uint32_t JitterHistogram[128] = {0};

#define MAX_THREADS 8
#define STACK_SIZE 256

static TCB threads[MAX_THREADS];
static uint32_t stacks[MAX_THREADS][STACK_SIZE];
static uint8_t thread_count = 0;

bool os_running;

uint32_t criticaltime;
volatile uint32_t time_disabled = 0;

static TCB idle = {
    .next_tcb = &idle,
    .prev_tcb = &idle,
    .id = 0,
    .alive = true,
    .priority = 255,
    .name = "OS Idle",
};

static noreturn void idle_task(void) {
    os_running = true;
    enable_interrupts();
    while (true) { wait_for_interrupts(); }
}

TCB* current_thread = &idle;

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
        if (adding->priority < next->priority) {
            adding->next_tcb = adding->prev_tcb = adding;
            current_thread->next_tcb = adding;
            OS_Suspend();
        } else if (next->priority == adding->priority) {
            insert_behind(adding, next);
        }
    } else if (adding->priority == current_thread->priority) {
        insert_behind(adding,
                      (current_thread->blocked || current_thread->asleep)
                          ? current_thread->next_tcb
                          : current_thread);
    }
    end_critical(crit);
}

// only called from sleep/kill/suspend so no additional critical section needed
static void remove_current_thread() {
    if (current_thread->next_tcb == current_thread) {
        uint8_t my_count = thread_count;
        TCB* new_current = &idle;
        idle.next_tcb = &idle;
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
        }
        current_thread->next_tcb = new_current;
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
    os_running = false;
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);
    launchpad_init();
    uart_init();
    SSI0_Init(10); // lcd_init();
    MaxJitter = 0;
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
    sem->blocked_head->blocked = false;
    insert_thread(sem->blocked_head);
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
    adding->next_tcb = adding->prev_tcb = &idle;

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

//******** OS_AddProcess ***************
// add a process with foregound thread to the scheduler
// Inputs: pointer to a void/void entry point
//         pointer to process text (code) segment
//         pointer to process data segment
//         number of bytes allocated for its stack
//         priority (0 is highest)
// Outputs: 1 if successful, 0 if this process can not be added
// This function will be needed for Lab 5
// In Labs 2-4, this function can be ignored
int OS_AddProcess(void (*entry)(void), void* text, void* data,
                  unsigned long stackSize, unsigned long priority) {
    // put Lab 5 solution here

    return 0; // replace this line with Lab 5 solution
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
        uint32_t jitter = to_us(
            difference(current - current_ptask->last, current_ptask->reload));
        current_ptask->last = current;
        MaxJitter = max(MaxJitter, jitter);
        uint8_t idx = min(
            sizeof(JitterHistogram) / sizeof(JitterHistogram[0]) - 1, jitter);
        ++JitterHistogram[idx];
        current_ptask->task();
    } while ((current_ptask = current_ptask->next));
    setup_next_ptask(OS_Time() - time);
}

static void setup_next_ptask(uint32_t lag) {
    int32_t min_time = INT32_MAX;
    for (int i = 0; i < num_ptasks; ++i) {
        min_time = min(min_time, ptasks[i].current);
    }
    uint32_t threshold = max(min_time, lag);
    for (int i = 0; i < num_ptasks; ++i) {
        if (ptasks[i].current <= threshold) {
            ptask_insert(&ptasks[i]);
            ptasks[i].current =
                max(0, ptasks[i].reload - (threshold - ptasks[i].current));
        } else {
            ptasks[i].current -= threshold;
        }
    }
    timer_enable(2, max(min_time - lag, 1), periodic_task, 1, false);
}

bool OS_AddPeriodicThread(void (*task)(void), uint32_t period,
                          uint32_t priority) {
    if (num_ptasks >= MAX_PTASKS - 1) {
        return false;
    }
    PTask* current = &ptasks[num_ptasks++];
    current->priority = priority;
    current->task = task;
    current->last = os_running ? OS_Time() : 0;
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

// TODO: structure this the same as periodic tasks to incur minimum overhead
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
    ROM_IntPrioritySet(FAULT_SYSTICK, 6 << 5);
    ROM_SysTickPeriodSet(time_slice);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();
    timer_enable(1, ms(1), &sleep_task, 3, true);
    if (num_ptasks) {
        setup_next_ptask(0);
    }
    OS_ClearTime();
    idle_task();
}

void systick_handler() {
    ROM_IntPendSet(FAULT_PENDSV);
}

void OS_ReportJitter(void) {
    printf("Max Jitter: %d microseconds\n\r", MaxJitter);
    uint32_t most = 0, most_idx;
    uint32_t sum = 0;
    uint32_t total_num = 0;
    for (int i = 0; i < sizeof(JitterHistogram) / sizeof(JitterHistogram[0]);
         ++i) {
        uint32_t num = JitterHistogram[i];
        sum += i * num;
        total_num += num;
        if (num > most) {
            most = num;
            most_idx = i;
        }
    }
    printf("Modal Jitter: %d microseconds\n\r", most_idx);
    printf("Average Jitter: %d microseconds\n\r", sum / total_num);
}

//************** I/O Redirection ***************
// redirect terminal I/O to UART or file (Lab 4)

int StreamToDevice = 0; // 0=UART, 1=stream to file (Lab 4)

int fputc(uint8_t ch, FILE* f) {
    if (StreamToDevice == 1) {      // Lab 4
        if (fs_append(ch)) {        // close file on error
            OS_EndRedirectToFile(); // cannot write to file
            return 1;               // failure
        }
        return 0; // success writing
    }

    // default UART output
    printf("%u", ch);
    return ch;
}

int fgetc(FILE* f) {
    char ch = getchar();
    puts(&ch);
    return ch;
}

int OS_RedirectToFile(const char* name) { // Lab 4
    fs_create_file(name);                 // ignore error if file already exists
    if (fs_wopen(name))
        return 1; // cannot open file
    StreamToDevice = 1;
    return 0;
}

int OS_EndRedirectToFile(void) { // Lab 4
    StreamToDevice = 0;
    if (fs_close_wfile())
        return 1; // cannot close file
    return 0;
}

int OS_RedirectToUART(void) {
    StreamToDevice = 0;
    return 0;
}

int OS_RedirectToLCD(void) {
    return 1;
}
