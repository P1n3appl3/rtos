#include "filesystem.h"
#include "OS.h"
#include "eDisk.h"
#include "printf.h"
#include <stdbool.h>
#include <stdint.h>

// sector number of first free block on sdc
static uint32_t free_block;

static uint8_t sd_buf[BUFFER_SIZE];
static uint16_t buf_ptr;
DIR root;

uint16_t mem_cpy(void* dest, void* src, uint16_t size) {
    uint8_t* src_b = (uint8_t*)src;
    uint8_t* dest_b = (uint8_t*)dest;

    for (uint16_t i = 0; i < size; i++) { dest_b[i] = src_b[i]; }
    return buf_ptr + size;
}

bool strcmp(const char* stra, const char* strb) {
    while (*stra == *strb) {
        if (stra == '\0') {
            return true;
        } else {
            stra++;
            strb++;
        }
    }
    return false;
}

bool fs_init(void) {
    DSTATUS result = eDisk_Init();
    if (result == STA_NODISK)
        printf("ERROR: disk not detected");
    else if (result)
        return false;
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) { sd_buf[i] = 0; }
    buf_ptr = 0;
    return true;
}

bool fs_format(void) {
    free_block = DIR_SIZE;
    root.num_entries = 0;
    return false;
}

bool fs_mount(void) {
    eDisk_Read(sd_buf, 0, 2);
    buf_ptr = mem_cpy(&root.num_entries, &sd_buf + buf_ptr, 4);
    buf_ptr = mem_cpy(&free_block, &sd_buf + buf_ptr, 4);
    return true;
}

bool fs_create_file(const char name[]) {
    uint32_t dir_ptr = root.num_entries * sizeof(FILE);
    uint32_t sector = dir_ptr / 512;
    eDisk_ReadBlock(sd_buf, sector);
    FILE new_file;
    mem_cpy(new_file.name, &name, FILENAME_SIZE);
    new_file.size = 0;
    new_file.valid = true;
    new_file.sector = free_block;
    free_block += INITIAL_ALLOCATION;
    new_file.capacity = INITIAL_ALLOCATION;
    buf_ptr = dir_ptr % 512;
    buf_ptr = mem_cpy(sd_buf, &new_file, sizeof(new_file));
    eDisk_WriteBlock(sd_buf, sector);
    return true;
}

bool fs_delete_file(const char* name) {
    buf_ptr = -1;
    uint32_t sector = 0;
    char filename[FILENAME_SIZE];
    for (uint32_t i = 0; i < DIR_SIZE; i += 2) {
        eDisk_Read(sd_buf, i, 2);
        for (uint16_t j = 0; j < BUFFER_SIZE; j += sizeof(FILE)) {
            mem_cpy(filename, sd_buf + j, FILENAME_SIZE);
            if (strcmp(name, filename)) {
                sector = i;
                buf_ptr = j;
                break;
            }
        }
    }

    if (buf_ptr > 0) {
        FILE to_del;
        mem_cpy(&to_del, sd_buf + buf_ptr, FILENAME_SIZE);
        to_del.valid = false;
        mem_cpy(sd_buf, &to_del, FILENAME_SIZE);
        eDisk_WriteBlock(sd_buf + buf_ptr % 512, sector + buf_ptr / 512);
        return true;
    }
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
    return true;
}

bool fs_dnext(char* name[], unsigned long* size) {
    return false;
}

bool fs_dclose(void) {
    return false;
}

bool fs_close(void) {
    return false;
}
