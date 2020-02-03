#include "io.h"
#include "FIFO.h"
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
        if (!rxfifo_put(ROM_UARTCharGet(UART0_BASE))) {
            puts("\n\rERROR: UART recieving fifo overflowed");
            while (1) {}
        }
    } while (ROM_UARTCharsAvail(UART0_BASE));
}

static void sw_to_hw_fifo() {
    uint8_t temp;
    do {
        if (!txfifo_get(&temp)) {
            break;
        }
    } while (ROM_UARTCharPutNonBlocking(UART0_BASE, temp));
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
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_1 | GPIO_PIN_0);
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                UART_CONFIG_PAR_NONE);
    ROM_UARTFIFOEnable(UART0_BASE);
    ROM_UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
    ROM_UARTIntEnable(UART0_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);
    ROM_IntEnable(INT_UART0);
    ROM_UARTEnable(UART0_BASE);
    txfifo_init();
    rxfifo_init();
}

bool putchar(char x) {
    ROM_UARTCharPut(UART0_BASE, x);
    return true;
    // Skip the buffer and go straight to the hardware fifo if possible
    if (txfifo_empty() && ROM_UARTCharPutNonBlocking(UART0_BASE, x)) {
        return true;
    }
    ROM_UARTIntDisable(UART0_BASE, UART_INT_TX);
    while (!txfifo_put(x)) {}
    sw_to_hw_fifo();
    ROM_UARTIntEnable(UART0_BASE, UART_INT_TX);
    return true;
}

char getchar(void) {
    uint8_t temp;
    while (!rxfifo_get(&temp)) {}
    return temp;
}

bool puts(const char* str) {
    for (; *str;) { putchar(*str++); }
    putchar('\n');
    putchar('\r');
    return true;
}
