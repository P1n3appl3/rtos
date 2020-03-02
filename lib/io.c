#include "io.h"
#include "FIFO.h"
#include "OS.h"
#include "interrupts.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/pin_map.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tivaware/uart.h"
#include <stdarg.h>
#include <stdint.h>

ADDFIFO(tx, 128, uint8_t)
ADDFIFO(rx, 128, uint8_t)

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
    }
    if (source & UART_INT_TX) {
        ROM_UARTIntClear(UART0_BASE, UART_INT_TX);
        sw_to_hw_fifo();
    }
}

Sema4 write_lock;
Sema4 read_lock;
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
    ROM_UARTIntEnable(UART0_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);
    ROM_IntEnable(INT_UART0);
    ROM_UARTEnable(UART0_BASE);
    txfifo_init();
    rxfifo_init();
    OS_InitSemaphore(&write_lock, 0);
    OS_InitSemaphore(&read_lock, 0);
}

bool putchar(char x) {
    // Skip the buffer and go straight to the hardware fifo if possible
    if (txfifo_empty() && ROM_UARTCharPutNonBlocking(UART0_BASE, x)) {
        return true;
    }
    txfifo_put(x);
    sw_to_hw_fifo();
    return true;
}

char getchar(void) {
    uint8_t temp;
    while (!rxfifo_get(&temp)) {}
    return temp;
}

bool puts(const char* str) {
    OS_bWait(&write_lock);
    for (; *str;) { putchar(*str++); }
    putchar('\n');
    putchar('\r');
    OS_bSignal(&write_lock);
    return true;
}

uint16_t gets(char* str, uint16_t len) {
    OS_bWait(&read_lock);
    char temp = '\0';
    uint16_t count = 0;
    while (temp != '\n' && temp != '\r' && len--) {
        temp = getchar();
        str[count++] = temp;
    }
    str[count] = '\0';
    OS_bSignal(&read_lock);
    return count;
}

uint16_t readline(char* str, uint16_t len) {
    OS_bWait(&write_lock);
    OS_bWait(&read_lock);
    int recieved = 0;
    char current = '\0';
    while (len--) {
        switch (current = getchar()) {
        case '\r':
            putchar('\n');
            putchar('\r');
            if (recieved == 0) {
                break;
            }
            *str = '\0';
            OS_bSignal(&read_lock);
            OS_bSignal(&write_lock);
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
    OS_bSignal(&read_lock);
    OS_bSignal(&write_lock);
    return recieved;
}

bool streq(const char* a, const char* b) {
    while (*a == *b) {
        if (!*a) {
            break;
        }
        ++a, ++b;
    }
    return *a == *b;
}

int16_t find(const char* s, char c) {
    for (int i = 0; *s; ++i) {
        if (c == *s++) {
            return i;
        }
    }
    return -1;
}

bool is_whitespace(char c) {
    switch (c) {
    case ' ':
    case '\t':
    case '\r':
    case '\n': return true;
    default: return false;
    }
}

bool is_numeric_char(const char c) {
    return c >= '0' && c <= '9';
}

bool is_numeric_str(const char* c) {
    c += c[0] == '-' || c[0] == '+';
    while (*c) {
        if (!is_numeric_char(*c++)) {
            return false;
        }
    }
    return true;
}

int32_t atoi(char* s) {
    bool neg = false;
    uint8_t i = 0;
    switch (s[i]) {
    case '-': neg = true;
    case '+': ++i; break;
    }
    int32_t result = 0;
    for (; s[i] && i < 11; ++i) { // TODO: error on overflowing 32 bits
        if (!is_numeric(s[i])) {
            return 0;
        }
        result *= 10;
        result += s[i] - '0';
    }
    return neg ? -result : result;
}
