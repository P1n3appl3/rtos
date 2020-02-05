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
    case '\r':
    case '\n': return true;
    default: return false;
    }
}

char raw_command[128];
char* current;
char token[32];

bool next_token() {
    while (is_whitespace(*current)) { ++current; }
    printf("\ncurrent is %d\n", &current);
    if (!*current) {
        token[0] = '\0';
        return false; // TODO: currently seems to never return false
    }
    uint8_t tok_idx = 0;
    do { token[tok_idx++] = *current++; } while (!is_whitespace(*current));
    token[tok_idx] = '\0';
    return true;
}

void interpreter(void) {
    printf("> ");
    gets(raw_command, sizeof(raw_command));
    puts(raw_command);
    current = raw_command;
    if (!next_token()) {
        printf("ERROR: enter a command\n");
        return;
    }
    if (streq(token, "help")) {
        printf(
            "Available commands:\n"
            "\thelp\t\t\t\tprint this help string\n\n"
            "\tled COLOR [ACTION]\t\tmanipulates the launchpad LEDs\n"
            "\t\tCOLOR: red, green, or blue\n"
            "\t\tACTION: on, off, or toggle\n\n"
            "\tadc\t\t\t\tread a value from the ADC\n\n"
            "\tlcd [top|bottom] [row #] STRING\tprint a string to the LCD\n\n"
            "\ttime [get|reset]\t\t\tget or reset the OS time\n"
            "\t\treset: reset the OS time to 0\n\n");
    } else if (streq(token, "led")) {
        if (!next_token()) {
            printf("ERROR: expected another argument for color\n");
            return;
        }
        uint8_t led;
        if (streq(token, "red") || streq(token, "r")) {
            led = RED_LED;
        } else if (streq(token, "blue") || streq(token, "b")) {
            led = BLUE_LED;
        } else if (streq(token, "green") || streq(token, "g")) {
            led = GREEN_LED;
        } else {
            printf("ERROR: invalid led color '%s'\nTry red, green, or blue\n",
                   token);
            return;
        }
        if (!next_token() || streq(token, "toggle") || streq(token, "t")) {
            led_toggle(led);
        } else if (streq(token, "on") || streq(token, "1")) {
            led_write(led, true);
        } else if (streq(token, "off") || streq(token, "0")) {
            led_write(led, false);
        }
    } else if (streq(token, "adc")) {
        printf("ADC reading: %d\n", ADC_in());
    } else if (streq(token, "lcd")) {
        printf("unimplemented\n");
    } else if (streq(token, "time")) {
        if (!next_token() || streq(token, "get")) {
            printf("Current time: %dms\n", OS_MsTime());
        } else if (streq(token, "reset")) {
            printf("OS time reset\n");
            OS_ClearMsTime();
        }
    } else {
        printf("ERROR: invalid command '%s'\n", token);
    }
}
