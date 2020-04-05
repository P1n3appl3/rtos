#include "ADC.h"
#include "OS.h"
#include "ST7735.h"
#include "filesystem.h"
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
    "\tled COLOR [ACTION]\n\r"
    "\t\tCOLOR: red, green, or blue\n\r"
    "\t\tACTION: on, off, or toggle\n\r"
    "\tadc\n\r\t\tread a single sample from the ADC\n\r"
    "\ttime [get|reset]\n\r\t\tget or reset the OS time\n\r"
    "\tdebug\n\r\t\tdisplay jitter info\n\r"
    "\tlcd 'STRING' [top|bottom] [row #]\n\r\t\tprint a string to the LCD\n\r"
    "\tmount\n\r\t\tmount the sd card"
    "\tunmount\n\r\t\tunmount the sd card"
    "\tformat yes really\n\r\t\tformat the sd card"
    "\tls\n\r\t\tlist the files in the directory\n\r"
    "\ttouch FILENAME\n\r\t\tcreates a new file\n\r"
    "\tcat FILENAME\n\r\t\tdisplay the contents of a file\n\r"
    "\tappend FILENAME 'STRING'\n\r\t\tappend a quoted string to a file\n\r"
    "\tmv FILENAME NEWNAME\n\r\t\tmove a file\n\r"
    "\tcp FILENAME NEWNAME\n\r\t\tcopy a file\n\r"
    "\trm FILENAME\n\r\t\tdelete a file\n\r";

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
        printf("ADC reading: %d\n\r", adc_in());
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
        lcd_message(bottom, line, str);
    } else if (streq(token, "time")) {
        if (!next_token() || streq(token, "get")) {
            printf("Current time: %dms\n\r", (uint32_t)to_ms(OS_Time()));
        } else if (streq(token, "reset")) {
            printf("OS time reset\n\r");
            OS_ClearTime();
        } else {
            printf("ERROR: expected 'get' or 'reset', got '%s'\n\r", token);
        }
    } else if (streq(token, "mount")) {
        fs_mount();
    } else if (streq(token, "unmount")) {
        fs_close();
    } else if (streq(token, "format")) {
        if (next_token() && streq(token, "yes") && next_token() &&
            streq(token, "really")) {
            if (!fs_format()) {
                printf("ERROR: foramtting failed\n\r");
            }
        } else {
            printf("ERROR: you need to show that you're sure by entering "
                   "'format yes realy'\n\r");
        }
    } else if (streq(token, "ls")) {
        // TODO: add fs_list()
    } else if (streq(token, "touch")) {
        if (!next_token()) {
            printf("ERROR: must pass a filename\n\r");
            return;
        }
        if (!fs_create_file(token)) {
            printf("ERROR: couldn't create file, maybe it already exists?\n\r");
        }
    } else if (streq(token, "cat")) {
        if (!next_token()) {
            printf("ERROR: must pass a filename\n\r");
            return;
        }
        if (!fs_ropen(token)) {
            printf("ERROR: couldn't open file '%s'\n\r", token);
        }
        char* temp = 0;
        printf("Contents of '%s':\n\r", token);
        while (fs_read(temp)) { putchar(*temp); }
        printf("\n\r");
    } else if (streq(token, "append")) {
        if (!next_token()) {
            printf("ERROR: must pass a filename\n\r");
            return;
        }
        if (!fs_wopen(token)) {
            printf("ERROR: couldn't open file", token);
        }
    } else if (streq(token, "rm")) {
        if (!next_token()) {
            printf("ERROR: must pass a filename\n\r");
            return;
        }
        if (!fs_delete_file(token)) {
            printf("ERROR: couldn't remove file '%s'\n\r", token);
        }
    } else if (streq(token, "mv")) {
        if (!next_token()) {
            printf("ERROR: must pass a filename\n\r");
            return;
        }
        // TODO: edit filename, check for overwrite
    } else if (streq(token, "cp")) {
        if (!next_token()) {
            printf("ERROR: must pass a filename\n\r");
            return;
        }
        // TODO: copy file
    }
}

void interpreter(void) {
    while (true) { interpret_command(); }
}
