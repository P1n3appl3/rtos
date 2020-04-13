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

static char* HELPSTRING = "Available commands:\n\n\r"
                          "led COLOR [on, off, or toggle]\t\n\r"
                          "\t\t\t\tCOLOR: red, green, or blue\n\r"
                          "\t\t\t\tACTION: on, off, or toggle\n\r"
                          "adc\t\t\t\tread a single sample from the ADC\n\r"
                          "time [get or reset]\t\tOS time helpers\n\r"
                          "mount\t\t\t\tmount the sd card\n\r"
                          "unmount\t\t\t\tunmount the sd card\n\r"
                          "format yes really\t\tformat the sd card\n\r"
                          "touch FILENAME\t\t\tcreates a new file\n\r"
                          "cat FILENAME\t\t\tdisplay the contents of a file\n\r"
                          "append FILENAME WORD\t\tappend a word to a file\n\r"
                          "ls\t\t\t\tlist the files in the directory\n\r"
                          "mv FILENAME NEWNAME\t\tmove a file\n\r"
                          "cp FILENAME NEWNAME\t\tcopy a file\n\r"
                          "rm FILENAME\t\t\tdelete a file\n\r";

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
        if (next_token() && streq(token, "yes")) {
            if (next_token() && streq(token, "really")) {
                if (!fs_format()) {
                    printf("ERROR: foramtting failed\n\r");
                }
                return;
            }
        }
        printf("ERROR: you need to show that you're sure by entering "
               "'format yes really'\n\r");
    } else if (streq(token, "ls")) {
        if (!fs_list_files()) {
            printf("ERROR: failed to list files\n\r");
        }
    } else if (streq(token, "touch")) {
        if (!next_token()) {
            printf("ERROR: must pass a filename\n\r");
            return;
        }
        if (!fs_create_file(token)) {
            printf("ERROR: couldn't create file\n\r");
        }
    } else if (streq(token, "cat")) {
        if (!next_token()) {
            printf("ERROR: must pass a filename\n\r");
            return;
        }
        if (!fs_ropen(token)) {
            printf("ERROR: couldn't open file '%s'\n\r", token);
            return;
        }
        char temp;
        printf("Contents of '%s':\n\r", token);
        while (fs_read(&temp)) { putchar(temp); }
        printf("\n\r");
        fs_close_rfile();
    } else if (streq(token, "append")) {
        if (!next_token()) {
            printf("ERROR: must pass a filename\n\r");
            return;
        }
        if (!fs_wopen(token)) {
            printf("ERROR: couldn't open file\n\r", token);
            return;
        }
        if (!next_token()) {
            printf("ERROR: must pass some characters to append\n\r");
            fs_close_wfile();
            return;
        }
        for (int i = 0; token[i]; ++i) {
            if (!fs_append(token[i])) {
                printf("ERROR: failed to write to the file\n\r");
            }
        }
        if (!fs_close_wfile()) {
            printf("ERROR: failed to close the file, try remounting\n\r");
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
        char filename[FILENAME_SIZE];
        memcpy(filename, token, FILENAME_SIZE);
        if (!next_token()) {
            printf("ERROR: must pass another filename\n\r");
            return;
        }
        if (!fs_rename_file(filename, token)) {
            printf("Error: failed to rename file\n\r");
        }
    } else if (streq(token, "cp")) {
        if (!next_token()) {
            printf("ERROR: must pass a filename\n\r");
            return;
        }
        // TODO: copy file
    } else {
        printf("Error: unrecognized command '%s', try 'help'\n\r", token);
    }
}

void interpreter(void) {
    printf("\x1b[1;1H\x1b[2JPress Enter to begin...");
    readline(raw_command, sizeof(raw_command));
    puts(HELPSTRING);
    while (true) { interpret_command(); }
}
