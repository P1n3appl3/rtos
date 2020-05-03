#pragma once

#include "OS.h"
#include <stdbool.h>
#include <stdint.h>

void uart_init(void);
void uart_change_speed(uint32_t baud);

bool uart_putchar(char x);
void uart_puts(const char* str);
#define puts(s) OS_RedirectString(s)
#define putchar(c) OS_RedirectChar(c)

char getchar(void);
uint16_t gets(char* str, uint16_t max);
uint16_t readline(char* str, uint16_t max);

#define BLACK "\x1b[0;30m"
#define RED "\x1b[0;31m"
#define GREEN "\x1b[0;32m"
#define YELLOW "\x1b[0;33m"
#define BLUE "\x1b[0;34m"
#define MAGENTA "\x1b[0;35m"
#define CYAN "\x1b[0;36m"
#define WHITE "\x1b[0;37m"
#define NORMAL "\x1b[0m"
