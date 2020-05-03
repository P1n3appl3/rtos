#include "OS.h"
#include "fifo.h"
#include "interrupts.h"
#include "std.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/pin_map.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tivaware/uart.h"
#include <stdarg.h>
#include <stdint.h>

// Speeds up transmission but annoying when debugging
// #define USE_OUTPUT_BUFFER

static FIFO* txfifo;
static FIFO* rxfifo;

static void hw_to_sw_fifo() {
    do {
        fifo_try_put(rxfifo, ROM_UARTCharGet(UART0_BASE));
    } while (ROM_UARTCharsAvail(UART0_BASE));
}

static void sw_to_hw_fifo() {
    uint8_t temp;
    while (ROM_UARTSpaceAvail(UART0_BASE)) {
        if (!fifo_try_get(txfifo, &temp)) {
            break;
        }
        ROM_UARTCharPutNonBlocking(UART0_BASE, temp);
    }
}

void uart0_handler(void) {
    uint32_t source = ROM_UARTIntStatus(UART0_BASE, false);
    if (source & (UART_INT_RX | UART_INT_RT)) {
        ROM_UARTIntClear(UART0_BASE, UART_INT_RX | UART_INT_RT);
        hw_to_sw_fifo();
    }
    if (source & UART_INT_TX) {
        ROM_UARTIntClear(UART0_BASE, UART_INT_TX);
        sw_to_hw_fifo();
    }
}

void uart_init(void) {
    txfifo = fifo_new(128);
    rxfifo = fifo_new(128);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_1 | GPIO_PIN_0);
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                UART_CONFIG_PAR_NONE);
    ROM_UARTFIFOEnable(UART0_BASE);
    ROM_IntPrioritySet(INT_UART0, 0 << 5); // priority is high 3 bits
    ROM_UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
#ifdef USE_OUTPUT_BUFFER
    ROM_UARTIntEnable(UART0_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);
#else
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
#endif

    ROM_IntEnable(INT_UART0);
    ROM_UARTEnable(UART0_BASE);
}

void uart_change_speed(uint32_t baud) {
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), baud,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                UART_CONFIG_PAR_NONE);
}

bool uart_putchar(char x) {
#ifdef USE_OUTPUT_BUFFER
    // Skip the buffer and go straight to the hardware fifo if possible
    if (fifo_empty(txfifo) && ROM_UARTCharPutNonBlocking(UART0_BASE, x)) {
        return true;
    }
    while (fifo_full(txfifo)) { OS_Suspend(); }
    uint32_t crit = start_critical();
    fifi_try_put(txfifo, x);
    end_critical(crit);
    sw_to_hw_fifo();
    return true;
#else
    uint32_t blocking_crit = start_critical();
    while (!ROM_UARTCharPutNonBlocking(UART0_BASE, x)) {}
    end_critical(blocking_crit);
    return true;
#endif
}

char getchar(void) {
    uint8_t temp;
    while (!fifo_try_get(rxfifo, &temp)) { OS_Suspend(); };
    return temp;
}

void uart_puts(const char* str) {
    while (*str) { uart_putchar(*str++); }
    uart_putchar('\n');
    uart_putchar('\r');
}

uint16_t gets(char* str, uint16_t max) {
    char temp = '\0';
    uint16_t count = 0;
    while (temp != '\n' && temp != '\r' && max--) {
        temp = getchar();
        str[count++] = temp;
    }
    str[count] = '\0';
    return count;
}

uint16_t readline(char* str, uint16_t max) {
    int received = 0;
    char current = '\0';
    while (max--) {
        switch (current = getchar()) {
        case '\n':
        case '\r':
            uart_putchar('\n');
            uart_putchar('\r');
            *str = '\0';
            return ++received;
        case 127:
            if (received > 0) {
                uart_putchar('\b');
                uart_putchar(' ');
                uart_putchar('\b');
                --str;
                --received;
            }
            break;
        default:
            uart_putchar(current);
            *str++ = current;
            ++received;
        }
    }
    *--str = '\0';
    return received;
}
