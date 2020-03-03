#pragma once
#include <stdbool.h>
#include <stdint.h>

int32_t abs(int32_t n);
int32_t min(int32_t a, int32_t b);
int32_t max(int32_t a, int32_t b);
bool streq(const char* a, const char* b);
int16_t find(const char* s, char c);
bool is_whitespace(char c);
bool is_numeric_char(const char c);
bool is_numeric_str(const char* c);
#define is_numeric(X)                                                          \
    ((_Generic((X), char : is_numeric_char, char* : is_numeric_str))(X))
int32_t atoi(char* s);
