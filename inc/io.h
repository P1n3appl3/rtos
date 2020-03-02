#pragma once
#include <stdbool.h>
#include <stdint.h>

void uart_init(void);

bool putchar(char x);
bool puts(const char* str);
char getchar(void);
uint16_t gets(char* str, uint16_t len);
uint16_t readline(char* str, uint16_t len);

bool streq(const char* a, const char* b);
int16_t find(const char* s, char c);
bool is_whitespace(char c);
bool is_numeric_char(const char c);
bool is_numeric_str(const char* c);
#define is_numeric(X)                                                          \
    ((_Generic((X), char : is_numeric_char, char* : is_numeric_str))(X))
int32_t atoi(char* s);
