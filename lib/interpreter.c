#include "ADC.h"
#include "OS.h"
#include "ST7735.h"
#include "io.h"
#include "launchpad.h"
#include "printf.h"
#include "std.h"
#include "timer.h"
#include <stdint.h>

// Print jitter histogram
void Jitter(int32_t MaxJitter, uint32_t const JitterSize,
            uint32_t JitterHistogram[]) {
    // write this for Lab 3 (the latest)
}
extern uint32_t MaxJitter;
extern uint32_t JitterHistogram[128];

char raw_command[128];
char* current;
char token[32];

static bool next_token() {
    while (is_whitespace(*current)) { ++current; }
    if (!(token[0] = *current)) {
        return false;
    }
    uint8_t tok_idx = 0;
    do {
        token[tok_idx++] = *current++;
    } while (*current && !is_whitespace(*current));
    token[tok_idx] = '\0';
    return true;
}

static char* HELPSTRING =
    "Available commands:\n\r"
    "\thelp\t\t\t\t\tprint this help string\n\n\r"
    "\tled COLOR [ACTION]\t\t\tmanipulates the launchpad LEDs\n\r"
    "\t\tCOLOR: red, green, or blue\n\r"
    "\t\tACTION: on, off, or toggle\n\n\r"
    "\tadc\t\t\t\t\tread a value from the ADC\n\n\r"
    "\ttime [get|reset]\t\t\tget or reset the OS time\n\n\r"
    "\tdebug\t\t\tdisplay debug info\n\n\r"
    "\tlcd 'STRING' [top|bottom] [row #]\tprint a string to the LCD\n\n\r";

void interpret_command(void) {
    printf("\n\r> ");
    readline(raw_command, sizeof(raw_command));
    current = raw_command;
    if (!next_token()) {
        printf("ERROR: enter a command\n\r");
        return;
    }
    if (streq(token, "help") || streq(token, "h")) {
        puts(HELPSTRING);
    } else if (streq(token, "led")) {
        if (!next_token()) {
            printf("ERROR: expected another argument for color\n\r");
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
            printf("ERROR: invalid color '%s'\n\rTry red, green, or blue\n\r",
                   token);
            return;
        }
        if (!next_token() || streq(token, "toggle") || streq(token, "t")) {
            led_toggle(led);
        } else if (streq(token, "on") || streq(token, "1")) {
            led_write(led, true);
        } else if (streq(token, "off") || streq(token, "0")) {
            led_write(led, false);
        } else {
            printf("ERROR: invalid action '%s'\n\rTry on, off, or toggle\n\r",
                   token);
            return;
        }
    } else if (streq(token, "adc")) {
        printf("ADC reading: %d\n\r", ADC_in());
    } else if (streq(token, "lcd")) {
        uint8_t line = 0;
        char quote_type = '\0';
        char* str;
        bool bottom;
        while (*current) {
            if (*current == '\'')
                quote_type = '\'';
            else if (*current == '"')
                quote_type = '"';
            if (quote_type) {
                break;
            }
            ++current;
        }
        if (!quote_type) {
            printf("ERROR: expected quoted string\n\r");
            return;
        }
        str = ++current;
        while (*current && *current != quote_type) { ++current; }
        if (!*current) {
            printf("ERROR: missing closing quote\n\r");
            return;
        }
        *current++ = '\0';
        if (!next_token() || streq(token, "top")) {
            bottom = false;
        } else if (streq(token, "bottom")) {
            bottom = true;
        } else {
            printf("ERROR: expected 'top' or 'bottom', got '%s'\n\r", token);
        }
        if (next_token()) {
            int32_t temp = atoi(token);
            if (temp < 8 && temp >= 0 && is_numeric(token)) {
                line = temp;
            } else {
                printf("ERROR: expected a number in [0, 7], got '%s'\n\r",
                       token);
                return;
            }
        }
        ST7735_Message(bottom, line, str);
    } else if (streq(token, "time")) {
        if (!next_token() || streq(token, "get")) {
            printf("Current time: %dms\n\r", (uint32_t)to_ms(OS_Time()));
        } else if (streq(token, "reset")) {
            printf("OS time reset\n\r");
            OS_ClearTime();
        } else {
            printf("ERROR: expected 'get' or 'reset', got '%s'\n\r", token);
        }
    } else if (streq(token, "debug")) {
        extern uint32_t CPUUtil;
        OS_ReportJitter();
        printf("CPU Utilization: %d%%\n\r", CPUUtil / 100);
    } else {
        printf("ERROR: invalid command '%s'\n\r", token);
    }
}

void interpreter(void) {
    while (true) { interpret_command(); }
}
