#include "ADC.h"
#include "OS.h"
#include "ST7735.h"
#include "io.h"
#include "launchpad.h"
#include "printf.h"
#include <stdint.h>

// Print jitter histogram
void Jitter(int32_t MaxJitter, uint32_t const JitterSize,
            uint32_t JitterHistogram[]) {
    // write this for Lab 3 (the latest)
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
    case '\n': return true;
    default: return false;
    }
}

char raw_command[128];
char token[32];

char* next_token(char* current) {
    uint8_t tok_idx;
    while (is_whitespace(*current)) { ++current; }
    for (tok_idx = 0; !is_whitespace(*current); ++tok_idx) {
        token[tok_idx] = *current++;
    }
    return current;
}

void interpreter(void) {
    printf("> ");
    gets(raw_command, sizeof(raw_command));
    char* current = raw_command;
    current = next_token(current);
    if (streq(token, "help")) {
        printf("This is the help string");
    } else if (streq(token, "toggle_led")) {
    }
    led_toggle(RED_LED);
    // printf("\nYou input the character: %d\n", temp);
}
