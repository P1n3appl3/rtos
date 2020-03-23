#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct _iobuf {
    char* _ptr;
    int _cnt;
    char* _base;
    int _flag;
    int _file;
    int _charbuf;
    int _bufsiz;
    char* _tmpfname;
} FILE;

// Activate the file system, without formating
// returns false on failure (already initialized)
bool fs_init(void);

// Deactivate the file system
// returns false on failure (not currently open)
bool fs_free(void);

// Erase all files, create blank directory, initialize free space manager
// returns false on failure (e.g., trouble writing to flash)
bool fs_format(void);

// Mount the file system, without formating
bool fs_mount(void);

// Display the directory with filenames and sizes
// returns false on failure (e.g., trouble reading from flash)
bool fs_list_files(void);

// Create a new, empty file with one allocated block
// name is an ASCII string up to seven characters
// returns false on failure (e.g., trouble writing to flash)
bool fs_create_file(const char* name);

// input: file name is a single ASCII letter
// returns false on failure (e.g., trouble writing to flash)
bool fs_delete_file(const char* name);

// Open the file, read into RAM last block
// name is a single ASCII letter
// returns false on failure (e.g., trouble writing to flash)
bool fs_wopen(const char* name);

// Open the file, read first block into RAM
// input: file name is a single ASCII letter
// returns false on failure (e.g., trouble read to flash)
bool fs_ropen(const char* name);

// Save at end of the open file
// input: data to be saved
// returns false on failure (e.g., trouble writing to flash)
bool fs_write(const uint8_t data);

// retreive data from open file
// returns output by reference
bool fs_read(char* output);

// Close the file, leaving disk in a state power can be removed
// returns false on failure (e.g., trouble writing to flash)
bool fs_close_wfile(void);

// close the reading file
// returns false on failure (e.g., wasn't open)
bool fs_close_rfile(void);

// Open a (sub)directory, read into RAM
// directory name is an ASCII string up to seven characters
// if subdirectories are supported (optional, empty string for root directory)
bool fs_dopen(const char name[]);

// Retreive directory entry from open directory
// pointers to return file name and size by reference
bool fs_dnext(char* name[], unsigned long* size);

// Close the directory
bool fs_dclose(void);

// Unmount and deactivate the file system.
bool fs_close(void);
