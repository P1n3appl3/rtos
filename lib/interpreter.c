#include "interpreter.h"
#include "ADC.h"
#include "OS.h"
#include "esp8266.h"
#include "heap.h"
#include "io.h"
#include "launchpad.h"
#include "littlefs.h"
#include "printf.h"
#include "std.h"
#include "timer.h"
#include <stdint.h>

const size_t COMMAND_BUF_LEN = 128;

char* raw_command;
char* current;
char* token;

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
    "led COLOR [on, off, or toggle]\tcontrol the onboard RGB led\n\r"
    "adc\t\t\t\tread a single sample from the ADC\n\r"
    "time [get/reset]\t\tOS time helpers\n\n\r"

#ifdef TRACK_JITTER
    "jitter\t\t\t\tshow periodic task jitter stats\n\r"
#endif
    "heap\t\t\t\tshow heap usage information\n\n\r"

    "mount\t\t\t\tmount the sd card\n\r"
    "unmount\t\t\t\tunmount the sd card\n\r"
    "format yes really\t\tformat the sd card\n\n\r"

    "upload FILENAME\t\t\ttransfer a file over UART\n\r"
    "exec FILENAME\t\t\tload and run process from file\n\r"
    "touch FILENAME\t\t\tcreates a new file\n\r"
    "cat FILENAME\t\t\tdisplay the contents of a file\n\r"
    "append FILENAME WORD\t\tappend a word to a file\n\r"
    "ls\t\t\t\tlist the files in the directory\n\r"
    "mv FILENAME NEWNAME\t\tmove a file\n\r"
    "cp FILENAME NEWNAME\t\tcopy a file\n\r"
    "rm FILENAME\t\t\tdelete a file\n\r"
    "checksum FILENAME\t\tcompute a checksum of a file\n\r";

void interpret_command(void) {
    iprintf("\n\r\xF0\x9F\x8D\x8D> ");
    ireadline(raw_command, COMMAND_BUF_LEN);
#ifdef rpc_client
    ESP8266_SendBuffered(raw_command);
    while (true) {
        ireadline(raw_command, COMMAND_BUF_LEN);
        if (streq(raw_command, "rpcstop")) {
            break;
        }
        puts(raw_command);
    }
    return;
#endif
    current = raw_command;
    if (!next_token()) {
        ERROR("enter a command\n\r");
    } else if (streq(token, "help") || streq(token, "h")) {
        iputs(HELPSTRING);
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
        iprintf("ADC reading: %d\n\r", adc_in());
    } else if (streq(token, "jitter")) {
        OS_ReportJitter();
    } else if (streq(token, "heap")) {
        heap_stats();
    } else if (streq(token, "time")) {
        if (!next_token() || streq(token, "get")) {
            iprintf("Current time: %dms\n\r", (uint32_t)to_ms(OS_Time()));
        } else if (streq(token, "reset")) {
            iprintf("OS time reset\n\r");
            OS_ClearTime();
        } else {
            ERROR("expected 'get' or 'reset', got '%s'\n\r", token);
        }
    } else if (streq(token, "mount")) {
        if (!littlefs_init() || !littlefs_mount()) {
            ERROR("mount failed");
        }
    } else if (streq(token, "unmount")) {
        littlefs_unmount();
    } else if (streq(token, "format")) {
        if (next_token() && streq(token, "yes") && next_token() &&
            streq(token, "really")) {
            if (!littlefs_format()) {
                ERROR("formatting failed\n\r");
            }
        } else {
            ERROR("You need to indicate that you're REALLY sure\n\r");
        }
    } else if (streq(token, "ls")) {
        littlefs_ls();
    } else if (streq(token, "touch")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        } else if (!(littlefs_open_file(token, true) &&
                     littlefs_close_file())) {
            ERROR("couldn't create file\n\r");
        }
    } else if (streq(token, "cat")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        } else if (!littlefs_open_file(token, false)) {
            ERROR("couldn't open file '%s'\n\r", token);
        }
#if defined(telnet_server) || defined(rpc_server)
        while (littlefs_read_line(raw_command, COMMAND_BUF_LEN)) {
            ESP8266_SendBuffered(raw_command);
        }
        ESP8266_SendBuffered(raw_command);
#else
        char temp;
        while (littlefs_read((uint8_t*)&temp)) { putchar(temp); }
#endif
        iprintf("\n\r");
        littlefs_close_file();
    } else if (streq(token, "append")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        } else if (!littlefs_open_file_append(token, true)) {
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
        iprintf("Wrote %d bytes\n\r", len);
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
        OS_Sleep(ms(1000));
    } else if (streq(token, "upload")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        }
        if (!littlefs_open_file(token, true)) {
            ERROR("failed to open file\n\r");
        }
        iprintf("Are you using the file transfer utility? [Y/n]\n\r");
        bool file_transfer = !(getchar() == 'n');
        bool binary = false;
        if (file_transfer) {
            iprintf("Is the file you're transferring binary? [y/N]\n\r");
            binary = getchar() == 'y';
        }
        char sizebuf[16];
        if (file_transfer) {
            iprintf("Close this session and run the file transfer utility...");
            uart_change_speed(9600);
        } else {
            iprintf("Enter your file's size in bytes: ");
        }
        readline(sizebuf, sizeof(sizebuf));
        uint32_t size = atoi(sizebuf);
        if (!file_transfer) {
            iprintf("Now paste the contents of your file...");
        }
        while (size--) {
            char temp = getchar();
            if (!littlefs_write(temp)) {
                littlefs_close_file();
                ERROR("failed to write to the file\n\r");
                if (file_transfer) {
                    __asm("BKPT");
                }
            }
            // translate to CRLF line endings
            if (!binary && (temp == '\r' || temp == '\n')) {
                littlefs_write(temp == '\n' ? '\r' : '\n');
            }
        }
        if (file_transfer) {
            uart_change_speed(115200);
        }
        iprintf("   Successfully Uploaded!\n\r");
        littlefs_close_file();
    } else if (streq(token, "checksum")) {
        if (!next_token()) {
            ERROR("must pass a filename\n\r");
        } else if (!littlefs_open_file(token, false)) {
            ERROR("couldn't open file '%s'\n\r", token);
        }
        char temp;
        uint32_t checksum = 0;
        while (littlefs_read((uint8_t*)&temp)) { checksum += temp; }
        iprintf("0x%08x\n\r", checksum);
        littlefs_close_file();
    } else {
        ERROR("unrecognized command '%s', try 'help'\n\r", token);
    }
#ifdef rpc_server
    ESP8266_SendBuffered("rpcstop");
#endif
}

void interpreter(void) {
    token = malloc(32);
    raw_command = malloc(COMMAND_BUF_LEN);
    iprintf("\x1b[1;1H\x1b[2JPress Enter to begin...");
    ireadline(raw_command, COMMAND_BUF_LEN);
    iputs(HELPSTRING);
    if (littlefs_init() && littlefs_mount()) {
        iputs(GREEN "Filesystem mounted" NORMAL);
    } else {
        iputs(RED "Filesystem failed to mount" NORMAL);
    }
    while (true) { interpret_command(); }
}
