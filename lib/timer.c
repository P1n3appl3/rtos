#include "tivaware/timer.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"

typedef struct {
    uint32_t sysctl_periph;
    uint32_t intterrupt;
    uint32_t base;
} TimerConfig;

static TimerConfig timers[] = {
    {SYSCTL_PERIPH_TIMER0, INT_TIMER0A, TIMER0_BASE},
    {SYSCTL_PERIPH_TIMER1, INT_TIMER1A, TIMER1_BASE},
    {SYSCTL_PERIPH_TIMER2, INT_TIMER2A, TIMER2_BASE},
    {SYSCTL_PERIPH_TIMER3, INT_TIMER3A, TIMER3_BASE},
    {SYSCTL_PERIPH_TIMER4, INT_TIMER4A, TIMER4_BASE},
    {SYSCTL_PERIPH_TIMER5, INT_TIMER5A, TIMER5_BASE},
    {SYSCTL_PERIPH_WTIMER0, INT_WTIMER0A, WTIMER0_BASE},
    {SYSCTL_PERIPH_WTIMER1, INT_WTIMER1A, WTIMER1_BASE},
    {SYSCTL_PERIPH_WTIMER2, INT_WTIMER2A, WTIMER2_BASE},
    {SYSCTL_PERIPH_WTIMER3, INT_WTIMER3A, WTIMER3_BASE},
    {SYSCTL_PERIPH_WTIMER4, INT_WTIMER4A, WTIMER4_BASE},
    {SYSCTL_PERIPH_WTIMER5, INT_WTIMER5A, WTIMER5_BASE},
};

void (*tasks[12])(void);

#define TIMERHANDLER(n)                                                        \
    void timer##n##a_handler(void) {                                           \
        ROM_TimerIntClear(TIMER##n##_BASE, TIMER_A);                           \
        (*tasks[n])();                                                         \
    }
#define WTIMERHANDLER(n)                                                       \
    void wtimer##n##a_handler(void) {                                          \
        ROM_TimerIntClear(WTIMER##n##_BASE, TIMER_A);                          \
        (*tasks[n + 6])();                                                     \
    }

TIMERHANDLER(0)
TIMERHANDLER(1)
TIMERHANDLER(2)
TIMERHANDLER(3)
TIMERHANDLER(4)
TIMERHANDLER(5)
WTIMERHANDLER(0)
WTIMERHANDLER(1)
WTIMERHANDLER(2)
WTIMERHANDLER(3)
WTIMERHANDLER(4)
WTIMERHANDLER(5)

uint32_t us(uint32_t us) {
    return us * 80 - 1; // TODO: check if -1 is needed
}

uint32_t ms(uint32_t ms) {
    return us(ms * 1000);
}

uint32_t seconds(float s) {
    return us(s * 1000000);
}

void periodic_timer_enable(uint8_t timer_num, uint32_t period,
                           void (*task)(void), uint8_t priority) {
    TimerConfig config = timers[timer_num];
    ROM_SysCtlPeripheralEnable(config.sysctl_periph);
    ROM_TimerConfigure(config.base, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(config.base, TIMER_A, period);
    ROM_IntEnable(config.intterrupt);
    ROM_IntPrioritySet(config.intterrupt,
                       priority << 5); // priority is high 3 bits
    tasks[timer_num] = task;
    ROM_TimerIntEnable(config.base, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(config.base, TIMER_BOTH);
}

void busy_wait(uint8_t timer_num, uint32_t duration) {
    TimerConfig config = timers[timer_num];
    ROM_SysCtlPeripheralEnable(config.sysctl_periph);
    ROM_TimerConfigure(config.base, TIMER_CFG_ONE_SHOT);
    ROM_TimerLoadSet(config.base, TIMER_A, duration);
    ROM_TimerEnable(config.base, TIMER_BOTH);
    while (ROM_TimerValueGet(config.base, TIMER_A) != duration) {} // TODO: make good
}
