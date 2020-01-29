#include "tivaware/hw_memmap.h"
#include "tivaware/rom.h"
#include <stdio.h>

int _read(int file, char* ptr, int len) {
    int recieved = 0;
    char current = '\0';
    while (len--) {
        switch (current = ROM_UARTCharGet(UART0_BASE)) {
        case '\r':
            putchar('\n');
            putchar('\r');
            if (recieved == 0) {
                break;
            }
            *ptr++ = '\n';
            return recieved + 1;
        case 127:
            if (recieved > 0) {
                putchar('\b');
                putchar(' ');
                putchar('\b');
                --ptr;
                --recieved;
            }
            break;
        default:
            putchar(current);
            *ptr++ = current;
            ++recieved;
        }
    }
    return recieved;
}

int _write(int file, char* ptr, int len) {
    for (int i = 0; i < len; ++ptr, ++i) {
        while (!ROM_UARTCharPutNonBlocking(UART0_BASE, *ptr)) {}
    }
    return len;
}
