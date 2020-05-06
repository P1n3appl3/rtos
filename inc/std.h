#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t size_t;

#define PI 3.14159265f

int32_t abs(int32_t n);
float sin(float x);
float cos(float x);
uint32_t difference(uint32_t a, uint32_t b);
int32_t min(int32_t a, int32_t b);
int32_t max(int32_t a, int32_t b);
bool streq(const char* a, const char* b);
uint16_t strlen(const char* s);
int16_t find(const char* s, char c);
bool is_whitespace(char c);
bool is_numeric_char(const char c);
bool is_numeric_str(const char* c);
#define is_numeric(X)                                                          \
    ((_Generic((X), char : is_numeric_char, char* : is_numeric_str))(X))
int32_t atoi(char* s);
char* reverse(char* s);
char* itoa(int32_t value, char* buf, uint8_t radix);
void memcpy(void* dest, const void* src, uint32_t n);
void memset(void* dest, uint8_t value, uint32_t n);
int memcmp(const void* s1, const void* s2, uint32_t n);
char* strcpy(char* dest, const char* src);
char* strchr(const char* str, int c);
size_t strspn(const char* str1, const char* str2);
size_t strcspn(const char* str1, const char* str2);
int8_t strncmp(const char* str1, const char* str2, size_t num);
char* strstr(char* str1, const char* str2);
