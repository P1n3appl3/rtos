#include "filesystem.h"
#include "OS.h"
#include "eDisk.h"
#include "io.h"
#include "printf.h"
#include "std.h"
#include <stdbool.h>
#include <stdint.h>

static uint32_t num_files;
static uint32_t free_block;
static bool mounted = false;

static FILE metadata_buf[16];
static uint32_t metadata_sector;
static Sema4 metadata_mutex;

static uint8_t read_buf[2 * BLOCK_SIZE];
static FILE wfile;

static uint8_t write_buf[2 * BLOCK_SIZE];
static FILE rfile;

bool fs_init(void) {
    OS_InitSemaphore(&metadata_mutex, 0);
    return !eDisk_Init();
}

bool fs_close(void) {
    if (!mounted) {
        return false;
    }
    OS_Wait(&metadata_mutex);
    fs_close_wfile();
    fs_close_rfile();
    mounted = false;
    OS_Signal(&metadata_mutex);
    return true;
}

static bool write_global_metadata() {
    uint32_t* temp = (uint32_t*)metadata_buf;
    temp[0] = num_files;
    temp[1] = free_block;
    return !eDisk_WriteBlock(metadata_buf, 0);
}

bool fs_format(void) {
    if (!mounted) {
        return false;
    }
    OS_Wait(&metadata_mutex);
    free_block = DIR_SIZE;
    num_files = 0;
    if (!write_global_metadata()) {
        OS_Signal(&metadata_mutex);
        return false;
    }
    // clear out all metadata (necessary for file space re-use optimization)
    memset(metadata_buf, 0, BLOCK_SIZE);
    puts("");
    for (int i = 0; i < DIR_SIZE; ++i) {
        if (!(i % 10)) {
            printf("\x1b[2K\r%d/%d blocks cleared", i, DIR_SIZE);
        }
        if (eDisk_WriteBlock(metadata_buf, i)) {
            printf("WRITE FAILURE ON BLOCK %d\n\r", i);
            OS_Signal(&metadata_mutex);
            return false;
        }
    }
    printf("\x1b[2K\rFormatted successfully.\n\r");
    OS_Signal(&metadata_mutex);
    return true;
}

bool fs_mount(void) {
    if (mounted) {
        return false;
    }
    if (eDisk_ReadBlock(metadata_buf, 0)) {
        return false;
    }
    mounted = true;
    uint32_t* temp = (uint32_t*)metadata_buf;
    num_files = temp[0];
    printf("Disk appears to contain %d files\n\r", num_files);
    free_block = temp[1];
    return true;
}

// returns null if lookup fails
static FILE* lookup_file(const char* name) {
    for (uint32_t found = 0, sector = 1; sector < DIR_SIZE; ++sector) {
        eDisk_ReadBlock(metadata_buf, sector);
        metadata_sector = sector;
        for (int i = 0; i < 16; ++i) {
            FILE* current = &metadata_buf[i];
            if (!current->valid) {
                continue;
            }
            if (streq(current->name, name)) {
                return current;
            }
            if (++found >= num_files) {
                return 0;
            }
        }
    }
    return 0;
}

bool fs_create_file(const char* name) {
    if (!mounted) {
        return false;
    }
    OS_Wait(&metadata_mutex);
    if (lookup_file(name)) {
        OS_Signal(&metadata_mutex);
        return false; // already exists
    }
    // walk the metadata again to fill in the first empty slot rather than
    // appending to the last metadata block which would fragment it
    for (uint32_t sector = 1; sector < DIR_SIZE; ++sector) {
        eDisk_ReadBlock(metadata_buf, sector);
        for (int i = 0; i < 16; ++i) {
            FILE* new_file = &metadata_buf[i];
            if (new_file->valid) {
                continue;
            }
            memcpy(new_file->name, name, FILENAME_SIZE);
            new_file->size = 0;
            // if a file had previously used this slot and been deleted,
            // then we know it has a valid data area on the disk, so we can
            // re-use it instead of allocating more space
            if (!new_file->sector) {
                new_file->sector = free_block;
                new_file->capacity = INITIAL_ALLOCATION;
                free_block += INITIAL_ALLOCATION;
            }
            new_file->valid = true;
            if (eDisk_WriteBlock(metadata_buf, sector)) {
                OS_Signal(&metadata_mutex);
                return false;
            }
            ++num_files;
            bool temp = write_global_metadata();
            OS_Signal(&metadata_mutex);
            return temp;
        }
    }
    OS_Signal(&metadata_mutex);
    return false; // full or corrupted
}

bool fs_delete_file(const char* name) {
    if (!mounted) {
        return false;
    }
    if ((wfile.valid && streq(wfile.name, name)) ||
        (rfile.valid && streq(rfile.name, name))) {
        return false; // can't delete open files
    }
    OS_Wait(&metadata_mutex);
    FILE* to_delete = lookup_file(name);
    if (!to_delete) {
        OS_Signal(&metadata_mutex);
        return false; // file doesn't exist
    }
    to_delete->valid = false;
    bool temp = !eDisk_WriteBlock(metadata_buf, metadata_sector);
    if (temp) {
        --num_files;
        temp = write_global_metadata();
    }
    OS_Signal(&metadata_mutex);
    return temp;
}

bool fs_rename_file(const char* name, const char* new_name) {
    if (!mounted) {
        return false;
    }
    if (wfile.valid && streq(wfile.name, name)) {
        memcpy(wfile.name, new_name, FILENAME_SIZE);
        return true;
    }
    if (rfile.valid && streq(rfile.name, name)) {
        memcpy(rfile.name, new_name, FILENAME_SIZE);
        return true;
    }
    OS_Wait(&metadata_mutex);
    FILE* to_rename = lookup_file(name);
    if (!to_rename) {
        OS_Signal(&metadata_mutex);
        return false; // file doesn't exist
    }
    memcpy(to_rename->name, new_name, FILENAME_SIZE);
    bool temp = !eDisk_WriteBlock(metadata_buf, metadata_sector);
    OS_Signal(&metadata_mutex);
    return temp;
}

bool fs_list_files(void) {
    if (!mounted) {
        return false;
    }
    if (!num_files) {
        puts("Disk contains no files");
        return true;
    }
    OS_Wait(&metadata_mutex);
    puts("-----------------------------------------------------------------");
    puts("| NAME              | SIZE (bytes) | CAPACITY (blocks) | SECTOR |");
    puts("|-------------------|--------------|-------------------|--------|");
    for (uint32_t found = 0, sector = 1; sector < DIR_SIZE; ++sector) {
        eDisk_ReadBlock(metadata_buf, sector);
        for (int i = 0; i < 16; ++i) {
            FILE* current = &metadata_buf[i];
            if (!current->valid) {
                continue;
            }
            printf("| %-17s | %-12d | %-17d | %-6d |\n\r", current->name,
                   current->size, current->capacity, current->sector);
            if (++found == num_files) {
                puts("---------------------------------------------------------"
                     "--------");
                OS_Signal(&metadata_mutex);
                return true;
            }
        }
    }
    OS_Signal(&metadata_mutex);
    return false;
}

bool fs_wopen(const char* name) {
    return false;
}

bool fs_ropen(const char* name) {
    return false;
}

bool fs_append(const uint8_t data) {
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
    if (!mounted) {
        return false;
    }
    if (name[0]) {
        return false; // subdirectories not supported
    }
    return true;
}

bool fs_dnext(char** name, uint32_t* size) {
    return false;
}

bool fs_dclose(void) {
    return true;
}
