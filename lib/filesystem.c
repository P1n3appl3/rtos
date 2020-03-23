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

bool fs_mount(void) {
    return false;
}

bool fs_list_files(void) {
    return false;
}

bool fs_create_file(const char* name) {
    return false;
}

bool fs_delete_file(const char* name) {
    return false;
}

bool fs_wopen(const char* name) {
    return false;
}

bool fs_ropen(const char* name) {
    return false;
}

bool fs_write(uint8_t data) {
    return false;
}

bool fs_read(char* output) {
    return false;
}

bool fs_close_wfile(void) {
    return false;
}

bool fs_close_rfile(void) {
    return false;
}

bool fs_dopen(const char name[]) {
    return 1;
}

bool fs_dnext(char* name[], unsigned long* size) {
    return 1;
}

bool fs_dclose(void) {
    return 1;
}

bool fs_close(void) {
    return 1;
}
