#include <stdint.h>

#define mkstr(s) #s
#define DECLARE_HANDLER(name)                                                  \
    void default_##name##_handler(void) {                                      \
        while (1) {}                                                           \
    }                                                                          \
    __attribute__(                                                             \
        (weak,                                                                 \
         alias(mkstr(default_##name##_handler)))) void name##_handler(void);

DECLARE_HANDLER(nonmaskable_interrupt)
DECLARE_HANDLER(svcall)
DECLARE_HANDLER(debug_monitor)
DECLARE_HANDLER(pendsv)
DECLARE_HANDLER(systick)
DECLARE_HANDLER(hardfault)
DECLARE_HANDLER(memory_management_fault)
DECLARE_HANDLER(bus_fault)
DECLARE_HANDLER(usage_fault)
DECLARE_HANDLER(gpio_porta)
DECLARE_HANDLER(gpio_portb)
DECLARE_HANDLER(gpio_portc)
DECLARE_HANDLER(gpio_portd)
DECLARE_HANDLER(gpio_porte)
DECLARE_HANDLER(uart0)
DECLARE_HANDLER(uart1)
DECLARE_HANDLER(spi0)
DECLARE_HANDLER(i2c0)
DECLARE_HANDLER(pwm0_fault)
DECLARE_HANDLER(pwm0_generator0)
DECLARE_HANDLER(pwm0_generator1)
DECLARE_HANDLER(pwm0_generator2)
DECLARE_HANDLER(qei0)
DECLARE_HANDLER(adc0_sequence0)
DECLARE_HANDLER(adc0_sequence1)
DECLARE_HANDLER(adc0_sequence2)
DECLARE_HANDLER(adc0_sequence3)
DECLARE_HANDLER(watchdog_timer)
DECLARE_HANDLER(timer0a)
DECLARE_HANDLER(timer0b)
DECLARE_HANDLER(timer1a)
DECLARE_HANDLER(timer1b)
DECLARE_HANDLER(timer2a)
DECLARE_HANDLER(timer2b)
DECLARE_HANDLER(analog_comparator0)
DECLARE_HANDLER(analog_comparator1)
DECLARE_HANDLER(system_ctrl)
DECLARE_HANDLER(flash_ctrl)
DECLARE_HANDLER(gpio_portf)
DECLARE_HANDLER(uart2)
DECLARE_HANDLER(spi1)
DECLARE_HANDLER(timer3a)
DECLARE_HANDLER(timer3b)
DECLARE_HANDLER(i2c1)
DECLARE_HANDLER(qei1)
DECLARE_HANDLER(can0)
DECLARE_HANDLER(can1)
DECLARE_HANDLER(hibernation)
DECLARE_HANDLER(usb0)
DECLARE_HANDLER(pwm0_generator3)
DECLARE_HANDLER(udma_software)
DECLARE_HANDLER(udma_error)
DECLARE_HANDLER(adc1_sequence0)
DECLARE_HANDLER(adc1_sequence1)
DECLARE_HANDLER(adc1_sequence2)
DECLARE_HANDLER(adc1_sequence3)
DECLARE_HANDLER(spi2)
DECLARE_HANDLER(spi3)
DECLARE_HANDLER(uart3)
DECLARE_HANDLER(uart4)
DECLARE_HANDLER(uart5)
DECLARE_HANDLER(uart6)
DECLARE_HANDLER(uart7)
DECLARE_HANDLER(i2c2)
DECLARE_HANDLER(i2c3)
DECLARE_HANDLER(timer4a)
DECLARE_HANDLER(timer4b)
DECLARE_HANDLER(timer5a)
DECLARE_HANDLER(timer5b)
DECLARE_HANDLER(wide_timer0a)
DECLARE_HANDLER(wide_timer0b)
DECLARE_HANDLER(wide_timer1a)
DECLARE_HANDLER(wide_timer1b)
DECLARE_HANDLER(wide_timer2a)
DECLARE_HANDLER(wide_timer2b)
DECLARE_HANDLER(wide_timer3a)
DECLARE_HANDLER(wide_timer3b)
DECLARE_HANDLER(wide_timer4a)
DECLARE_HANDLER(wide_timer4b)
DECLARE_HANDLER(wide_timer5a)
DECLARE_HANDLER(wide_timer5b)
DECLARE_HANDLER(system_exception)
DECLARE_HANDLER(pwm1_generator0)
DECLARE_HANDLER(pwm1_generator1)
DECLARE_HANDLER(pwm1_generator2)
DECLARE_HANDLER(pwm1_generator3)
DECLARE_HANDLER(pwm1_fault)

__attribute__((used, weak, alias("default_reset_handler"))) void
reset_handler(void);

__attribute__((section(".interrupt_table"))) void (*const _vector_table[])(
    void) = {
    (void (*)(void))0x20008000,
    reset_handler,
    nonmaskable_interrupt_handler,
    hardfault_handler,
    memory_management_fault_handler,
    bus_fault_handler,
    usage_fault_handler,
    0,
    0,
    0,
    0,
    svcall_handler,
    debug_monitor_handler,
    0,
    pendsv_handler,
    systick_handler,
    gpio_porta_handler,
    gpio_portb_handler,
    gpio_portc_handler,
    gpio_portd_handler,
    gpio_porte_handler,
    uart0_handler,
    uart1_handler,
    spi0_handler,
    i2c0_handler,
    pwm0_fault_handler,
    pwm0_generator0_handler,
    pwm0_generator1_handler,
    pwm0_generator2_handler,
    qei0_handler,
    adc0_sequence0_handler,
    adc0_sequence1_handler,
    adc0_sequence2_handler,
    adc0_sequence3_handler,
    watchdog_timer_handler,
    timer0a_handler,
    timer0b_handler,
    timer1a_handler,
    timer1b_handler,
    timer2a_handler,
    timer2b_handler,
    analog_comparator0_handler,
    analog_comparator1_handler,
    0,
    system_ctrl_handler,
    flash_ctrl_handler,
    gpio_portf_handler,
    0,
    0,
    uart2_handler,
    spi1_handler,
    timer3a_handler,
    timer3b_handler,
    i2c1_handler,
    qei1_handler,
    can0_handler,
    can1_handler,
    0,
    0,
    hibernation_handler,
    usb0_handler,
    pwm0_generator3_handler,
    udma_software_handler,
    udma_error_handler,
    adc1_sequence0_handler,
    adc1_sequence1_handler,
    adc1_sequence2_handler,
    adc1_sequence3_handler,
    0,
    0,
    0,
    0,
    0,
    spi2_handler,
    spi3_handler,
    uart3_handler,
    uart4_handler,
    uart5_handler,
    uart6_handler,
    uart7_handler,
    0,
    0,
    0,
    0,
    i2c2_handler,
    i2c3_handler,
    timer4a_handler,
    timer4b_handler,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    timer5a_handler,
    timer5b_handler,
    wide_timer0a_handler,
    wide_timer0b_handler,
    wide_timer1a_handler,
    wide_timer1b_handler,
    wide_timer2a_handler,
    wide_timer2b_handler,
    wide_timer3a_handler,
    wide_timer3b_handler,
    wide_timer4a_handler,
    wide_timer4b_handler,
    wide_timer5a_handler,
    wide_timer5b_handler,
    system_exception_handler,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    pwm1_generator0_handler,
    pwm1_generator1_handler,
    pwm1_generator2_handler,
    pwm1_generator3_handler,
    pwm1_fault_handler,
};

extern int main(void);
extern uint32_t _bss;
extern uint32_t _ebss;
extern uint32_t _etext;
extern uint32_t _data;
extern uint32_t _edata;
volatile uint32_t* const CPACR = (uint32_t*)0xE000ED88;

void default_reset_handler(void) {
    uint32_t* src = &_etext;
    uint32_t* dest = &_data;
    while (dest < &_edata) { *dest++ = *src++; }

    dest = &_bss;
    while (dest < &_ebss) { *dest++ = 0; }

    *CPACR |= 0xF << 20;

    main();
    while (1) {}
}

void disable_interrupts(void) {
    __asm("CPSID  I\n");
}

void enable_interrupts(void) {
    __asm("CPSIE  I\n");
}

void start_critical(void) {
    __asm("MRS    R0, PRIMASK\n"
          "CPSID  I\n");
}

void end_critical(void) {
    __asm("MSR    PRIMASK, R0\n");
}

void wait_for_interrupts(void) {
    __asm("WFI\n");
}
