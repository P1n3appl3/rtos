#include "FIFO.h"
#include "startup.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/pin_map.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "tivaware/uart.h"
#include <stdio.h>

ADDFIFO(tx, 128, uint8_t)
ADDFIFO(rx, 128, uint8_t)

void uart0_handler(void) {
    uint32_t source = ROM_UARTIntStatus(UART0_BASE, false);
    if (source & UART_INT_RX) {
        do {
            rxfifo_put(ROM_UARTCharGet(UART0_BASE));
        } while (ROM_UARTCharsAvail(UART0_BASE));
    }
    if (source & UART_INT_TX) {
        uint8_t temp;
        do {
            txfifo_get(&temp);
        } while (ROM_UARTCharPutNonBlocking(UART0_BASE, temp));
        ROM_UARTIntClear(UART0_BASE, UART_INT_RX | UART_INT_TX);
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
    ROM_UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
    ROM_UARTFIFOEnable(UART0_BASE);
    ROM_UARTEnable(UART0_BASE);
    // Disable stdlib buffered output, we're using our own software buffers
    setvbuf(stdout, NULL, _IONBF, 0);
    txfifo_init();
    rxfifo_init();
}

int _read(int file, char* ptr, int len) {
    int recieved = 0;
    if (file != 0) { // stdin is uart
        printf("ERROR: can only read from stdin with stdio functions");
        return 0;
    }
    for (; len--; recieved++) {
        if (!rxfifo_get((uint8_t*)(ptr)++)) {
            break;
        }
    }
    return recieved;
}

void send_byte(uint8_t x) {
    if (!ROM_UARTCharPutNonBlocking(UART0_BASE, x)) {
        txfifo_put(x);
    }
}

int _write(int file, char* ptr, int len) {
    uint32_t critical = start_critical();
    for (; len--;) { send_byte(*ptr++); }
    end_critical(critical);
    return 0;
}
