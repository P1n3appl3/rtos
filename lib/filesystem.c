#include "filesystem.h"
#include "OS.h"
#include "eDisk.h"
#include <stdbool.h>
#include <stdint.h>

bool fs_init(void) {
    return false;
}

bool fs_free(void) {
    return false;
}

bool fs_format(void) {
    return false;
}

bool fs_list_files(void) {
    return false;
}

bool fs_create_file(char* name) {
    return false;
}

bool fs_delete_file(char* name) {
    return false;
}

bool fs_wopen(char* name) {
    return false;
}

bool fs_ropen(char* name) {
    return false;
}

bool fs_write(uint8_t data) {
    return false;
}

bool fs_read(uint8_t* output) {
    return false;
}

bool fs_close_wfile(void) {
    return false;
}

bool fs_close_rfile(void) {
    return false;
}
