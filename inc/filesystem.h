#pragma once
#include <stdbool.h>
#include <stdint.h>

#define BLOCK_SIZE 512
#define DIR_SIZE (1024 * 1024 / BLOCK_SIZE)          // 1 MB
#define INITIAL_ALLOCATION (100 * 1024 / BLOCK_SIZE) // 100 kB

// Block 0's first 8 bytes contain the number of files and the next
// free block as uint32_t's.
// Blocks 1 and up contain unordered packed FILE structs

// keep every file entry at 32 bytes to align to 512 byte block
#define FILENAME_SIZE 19
typedef struct file_t {
    char name[FILENAME_SIZE];
    uint32_t sector;
    uint32_t size;
    uint32_t capacity;
    bool valid;
} FILE;

// All functions return false on any type of failure (which can include failed
// write to sd card)

// Activate the file system, without formating
bool fs_init(void);

// Erase all files, create blank directory, initialize free space manager
bool fs_format(void);

// Mount the file system, without formating
bool fs_mount(void);

// Create a new, empty file with one allocated block
bool fs_create_file(const char* name);

bool fs_delete_file(const char* name);

bool fs_rename_file(const char* name, const char* new_name);

// Open a file, read last block into RAM
bool fs_wopen(const char* name);

// Open a file, read first block into RAM
bool fs_ropen(const char* name);

// Append data to the open file
bool fs_append(const uint8_t data);

// Retreive data from open file
// returns output by reference
bool fs_read(char* output);

// Close the file, leaving disk in a state power can be removed
bool fs_close_wfile(void);

// Close the reading file
bool fs_close_rfile(void);

// Open a (sub)directory, read into RAM
// if subdirectories are supported (optional, empty string for root directory)
bool fs_dopen(const char* name);

// Retreive directory entry from open directory
// pointers to return file name and size by reference
bool fs_dnext(char** name, uint32_t* size);

// Close the directory
bool fs_dclose(void);

// Unmount and deactivate the file system.
bool fs_close(void);
