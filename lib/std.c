#include "std.h"
#include <stdbool.h>
#include <stdint.h>

uint32_t difference(uint32_t a, uint32_t b) {
    return a < b ? b - a : a - b;
}

int32_t abs(int32_t n) {
    return n < 0 ? -n : n;
}

float sin(float x) {
    while (x > PI) { x -= 2 * PI; }
    while (x < -PI) { x += 2 * PI; }
    // Taylor series
    float x3 = x * x * x;
    float x5 = x3 * x * x;
    float x7 = x3 * x * x;
    float x9 = x3 * x * x;
    return x - x3 / 6 + x5 / 120 - x7 / 5040 + x9 / 362880;
}

float cos(float x) {
    while (x > PI) { x -= 2 * PI; }
    while (x < -PI) { x += 2 * PI; }
    // Taylor series
    float x2 = x * x;
    float x4 = x2 * x * x;
    float x6 = x4 * x * x;
    float x8 = x6 * x * x;
    return 1 - x2 / 2 + x4 / 24 - x6 / 720 + x8 / 40320;
}

int32_t min(int32_t a, int32_t b) {
    return a < b ? a : b;
}

int32_t max(int32_t a, int32_t b) {
    return a > b ? a : b;
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

int8_t strncmp(const char* str1, const char* str2, size_t num) {
    for (size_t i = 0; i < num; i++) {
        if (*str1 < *str2) {
            return -1;
        } else if (*str1 > *str2) {
            return 1;
        }
        str1++;
        str2++;
    }
    return 0;
}

char* strstr(char* str1, const char* str2) {
    uint32_t len1 = strlen(str1);
    uint32_t len2 = strlen(str2);

    if (len1 < len2) {
        return ((char*)0);
    }

    for (uint32_t i = 0; i < len1 - len2 + 1; i++) {
        if (strncmp(str1, str2, len2) == 0) {
            return str1;
        }
        str1++;
    }
    return ((char*)0);
}

void memset(void* dest, uint8_t value, uint32_t n) {
    uint8_t* temp = (uint8_t*)dest;
    while (n--) { *temp++ = value; }
}

int32_t memcmp(const void* s1, const void* s2, uint32_t n) {
    uint8_t* a = (uint8_t*)s1;
    uint8_t* b = (uint8_t*)s2;

    for (uint32_t i = 0; i < n; i++) {
        if (a[i] < b[i]) {
            return -1;
        } else if (a[i] > b[i]) {
            return 1;
        }
    }
    return 0;
}

uint16_t strlen(const char* s) {
    uint16_t n = 0;
    while (*s++) { n++; }
    return n;
}

char* strcpy(char* dest, const char* src) {
    while (*src) { *dest++ = *src++; }
    *dest = '\0';
    return dest;
}

char* strchr(const char* str, int32_t c) {
    int16_t ret = find(str, c);
    if (ret > -1) {
        return (char*)(str + ret);
    }
    return 0;
}

size_t strspn(const char* str1, const char* str2) {
    size_t n = 0;
    while (*str1++) {
        if (find(str2, *str1) == -1) {
            return n;
        }
        n++;
    }
    return n;
}

size_t strcspn(const char* str1, const char* str2) {
    size_t n = 0;
    while (*str1++) {
        if (find(str2, *str1) != -1) {
            return n;
        }
        n++;
    }
    return n;
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

char* reverse(char* s) {
    uint16_t len = strlen(s);
    for (int i = 0; i < len / 2; ++i) {
        char temp = s[i];
        s[i] = s[len - 1 - i];
        s[len - 1 - i] = temp;
    }
    return s;
}

char* itoa(int32_t value, char* s, uint8_t radix) {
    uint8_t i = 0;
    bool neg = false;
    if (value < 0) {
        neg = true;
        value = -value;
    }
    do {
        char current = value % radix;
        if (current < 10) {
            current += '0';
        } else {
            current += 'a' - 10; // TODO: case
        }
        s[i++] = current;
        value /= radix;
    } while (value);
    if (neg) {
        s[i++] = '-';
    }
    s[i] = '\0';
    return reverse(s);
}

void memcpy(void* dest, const void* src, uint32_t n) {
    uint32_t* wide_a = dest;
    const uint32_t* wide_b = src;
    if (!((uint32_t)src % 4 || (uint32_t)dest % 4)) {
        while (n >= 4) {
            *wide_a++ = *wide_b++;
            n -= 4;
        }
    }
    uint8_t* a = (uint8_t*)wide_a;
    const uint8_t* b = (const uint8_t*)wide_b;
    while (n--) { *a++ = *b++; }
}

// Compiler generated intrinsics

void __aeabi_memclr4(void* mem, size_t bytes) {
    uint32_t* buf = (uint32_t*)mem;
    while (bytes > 0) {
        *buf = 0;
        ++buf;
        bytes -= 4;
    }
}

void __aeabi_memcpy4(void* dest, const void* src, uint32_t n) {
    uint32_t* a = dest;
    const uint32_t* b = src;
    while (n > 0) {
        *a++ = *b++;
        n -= 4;
    }
}

__attribute__((alias("memcpy"))) void
__aeabi_memcpy(void* dest, const void* src, uint32_t n);
