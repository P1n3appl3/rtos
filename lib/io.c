#include "FIFO.h"
#include "OS.h"
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

// #define USE_OUTPUT_BUFFER

ADDFIFO(tx, 128, uint8_t)
ADDFIFO(rx, 128, uint8_t)

Sema4 chars_avail;

static void hw_to_sw_fifo() {
    do {
        rxfifo_put(ROM_UARTCharGet(UART0_BASE));
    } while (ROM_UARTCharsAvail(UART0_BASE));
}

static void sw_to_hw_fifo() {
    uint8_t temp;
    while (ROM_UARTSpaceAvail(UART0_BASE)) {
        if (!txfifo_get(&temp)) {
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
        OS_Signal(&chars_avail);
    }
    if (source & UART_INT_TX) {
        ROM_UARTIntClear(UART0_BASE, UART_INT_TX);
        sw_to_hw_fifo();
    }
}

void uart_init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_1 | GPIO_PIN_0);
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                UART_CONFIG_PAR_NONE);
    ROM_UARTFIFOEnable(UART0_BASE);
    ROM_IntPrioritySet(INT_UART0, 0x20); // priority is high 3 bits
    ROM_UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
#ifdef USE_OUTPUT_BUFFER
    ROM_UARTIntEnable(UART0_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);
#else
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
#endif

    ROM_IntEnable(INT_UART0);
    ROM_UARTEnable(UART0_BASE);
    txfifo_init();
    rxfifo_init();
    OS_InitSemaphore(&chars_avail, 0);
}

bool putchar(char x) {
#ifdef USE_IO_BUFFERING
    // Skip the buffer and go straight to the hardware fifo if possible
    if (txfifo_empty() && ROM_UARTCharPutNonBlocking(UART0_BASE, x)) {
        return true;
    }
    while (txfifo_full()) { OS_Suspend(); }
    uint32_t crit = start_critical();
    txfifo_put(x);
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
    OS_Wait(&chars_avail);
    uint8_t temp;
    rxfifo_get(&temp);
    return temp;
}

bool puts(const char* str) {
    while (*str) { putchar(*str++); }
    putchar('\n');
    putchar('\r');
    return true;
}

uint16_t gets(char* str, uint16_t len) {
    char temp = '\0';
    uint16_t count = 0;
    while (temp != '\n' && temp != '\r' && len--) {
        temp = getchar();
        str[count++] = temp;
    }
    str[count] = '\0';
    return count;
}

uint16_t readline(char* str, uint16_t len) {
    int recieved = 0;
    char current = '\0';
    while (len--) {
        switch (current = getchar()) {
        case '\r':
            putchar('\n');
            putchar('\r');
            // if (recieved == 0) {
            //     break; // don't return empty strings
            // }
            *str = '\0';
            return recieved + 1;
        case 127:
            if (recieved > 0) {
                putchar('\b');
                putchar(' ');
                putchar('\b');
                --str;
                --recieved;
            }
            break;
        default:
            putchar(current);
            *str++ = current;
            ++recieved;
        }
    }
    *--str = '\0';
    return recieved;
}
