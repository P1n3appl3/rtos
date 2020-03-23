#include "std.h"
#include <stdbool.h>
#include <stdint.h>

uint32_t difference(uint32_t a, uint32_t b) {
    return a < b ? b - a : a - b;
}

int32_t abs(int32_t n) {
    return n < 0 ? -n : n;
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

uint16_t strlen(const char* s) {
    uint16_t n = 0;
    while (*s++) { n++; }
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
