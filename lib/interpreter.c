#include "ADC.h"
#include "OS.h"
#include "heap.h"
#include "io.h"
#include "launchpad.h"
#include "littlefs.h"
#include "printf.h"
#include "std.h"
#include "timer.h"
#include <stdint.h>

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
    "Available commands:\n\n\r"
    "led COLOR [on, off, or toggle]\t\n\r"
    "\t\t\t\tCOLOR: red, green, or blue\n\r"
    "\t\t\t\tACTION: on, off, or toggle\n\r"
    "adc\t\t\t\tread a single sample from the ADC\n\r"
    "heap\t\t\t\tshow heap usage information\n\r"
    "time [get or reset]\t\tOS time helpers\n\r"
    "mount\t\t\t\tmount the sd card\n\r"
    "unmount\t\t\t\tunmount the sd card\n\r"
    "format yes really\t\tformat the sd card\n\r"
    "exec FILENAME\t\t\tload and run process from file\n\r"
    "touch FILENAME\t\t\tcreates a new file\n\r"
    "cat FILENAME\t\t\tdisplay the contents of a file\n\r"
    "append FILENAME WORD\t\tappend a word to a file\n\r"
    "ls\t\t\t\tlist the files in the directory\n\r"
    "mv FILENAME NEWNAME\t\tmove a file\n\r"
    "cp FILENAME NEWNAME\t\tcopy a file\n\r"
    "rm FILENAME\t\t\tdelete a file\n\r";

#define ERROR(...)                                                             \
    printf(RED "ERROR: " NORMAL __VA_ARGS__);                                  \
    return;

void interpret_command(void) {
    printf("\n\r> ");
    readline(raw_command, sizeof(raw_command));
    current = raw_command;
    if (!next_token()) {
        ERROR("enter a command\n\r");
    } else if (streq(token, "help") || streq(token, "h")) {
        puts(HELPSTRING);
    } else if (streq(token, "led")) {
        uint8_t led;
        if (!next_token()) {
            ERROR("expected another argument for color\n\r");
        } else if (streq(token, "red") || streq(token, "r")) {
            led = RED_LED;
        } else if (streq(token, "blue") || streq(token, "b")) {
            led = BLUE_LED;
        } else if (streq(token, "green") || streq(token, "g")) {
            led = GREEN_LED;
        } else {
            ERROR("invalid color '%s'\n\rTry red, green, or blue\n\r", token);
        }
        if (!next_token() || streq(token, "toggle") || streq(token, "t")) {
            led_toggle(led);
        } else if (streq(token, "on") || streq(token, "1")) {
            led_write(led, true);
        } else if (streq(token, "off") || streq(token, "0")) {
            led_write(led, false);
        } else {
            ERROR("invalid action '%s'\n\rTry on, off, or toggle\n\r", token);
        }
    } else if (streq(token, "adc")) {
        printf("ADC reading: %d\n\r", adc_in());
    } else if (streq(token, "heap")) {
        heap_stats();
    } else if (streq(token, "time")) {
        if (!next_token() || streq(token, "get")) {
            printf("Current time: %dms\n\r", (uint32_t)to_ms(OS_Time()));
        } else if (streq(token, "reset")) {
            printf("OS time reset\n\r");
            OS_ClearTime();
        } else {
            ERROR("expected 'get' or 'reset', got '%s'\n\r", token);
        }
    } else if (streq(token, "mount")) {
        littlefs_init() && littlefs_mount();
    } else if (streq(token, "unmount")) {
        littlefs_close();
    } else if (streq(token, "format")) {
        if (next_token() && streq(token, "yes") && next_token() &&
            streq(token, "really")) {
            if (littlefs_format()) {
                return;
            }
            ERROR("foramtting failed\n\r");
        }
        ERROR("You need to indicate that you're REALLY sure\n\r");
    } else if (streq(token, "ls")) {
        littlefs_ls();
    } else if (streq(token, "touch")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        } else if (!(littlefs_open_file(token) && littlefs_close())) {
            ERROR("couldn't create file\n\r");
        }
    } else if (streq(token, "cat")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        } else if (!littlefs_open_file(token)) {
            ERROR("couldn't open file '%s'\n\r", token);
        }
        char temp;
        printf("Contents of '%s':\n\r", token);
        while (littlefs_read_file((uint8_t*)&temp)) { putchar(temp); }
        printf("\n\r");
        littlefs_close_file();
    } else if (streq(token, "append")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        } else if (!littlefs_open_file(token)) {
            ERROR("couldn't open file\n\r", token);
        } else if (!next_token()) {
            littlefs_close_file();
            ERROR("must pass some characters to append\n\r");
        }
        uint8_t len = strlen(token);
        bool ret = littlefs_write_buffer(token, len);
        littlefs_close_file();
        if (!ret) {
            ERROR("failed to write to the file\n\r");
        }
        printf("Wrote %d bytes\n\r", len);
    } else if (streq(token, "rm")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        } else if (!littlefs_remove(token)) {
            ERROR("couldn't remove file '%s'\n\r", token);
        }
    } else if (streq(token, "mv")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        }
        char filename[32];
        memcpy(filename, token, sizeof(filename));
        if (!next_token()) {
            ERROR("must pass another filename\n\r");
        } else if (!littlefs_move(filename, token)) {
            ERROR("failed to move file\n\r");
        }
    } else if (streq(token, "cp")) {
        // TODO
        ERROR("unimplemented\n\r");
    } else if (streq(token, "exec")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        }
        OS_LoadProgram(token);
    } else {
        ERROR("unrecognized command '%s', try 'help'\n\r", token);
    }
}

void interpreter(void) {
    printf("\x1b[1;1H\x1b[2JPress Enter to begin...");
    readline(raw_command, sizeof(raw_command));
    puts(HELPSTRING);
    if (littlefs_init() && littlefs_mount()) {
        puts("Filesystem mounted\n");
    } else {
        puts("Filesystem failed to mount\n");
    }
    while (true) { interpret_command(); }
}
