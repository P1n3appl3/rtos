#include <stdbool.h>
#include <stdint.h>

// Activate the file system, without formating
// returns false on failure (already initialized)
bool fs_init(void);

// Deactivate the file system
// returns false on failure (not currently open)
bool fs_free(void);

// Erase all files, create blank directory, initialize free space manager
// returns false on failure (e.g., trouble writing to flash)
bool fs_format(void);

// Display the directory with filenames and sizes
// returns false on failure (e.g., trouble reading from flash)
bool fs_list_files(void);

// Create a new, empty file with one allocated block
// name is an ASCII string up to seven characters
// returns false on failure (e.g., trouble writing to flash)
bool fs_create_file(char* name);

// input: file name is a single ASCII letter
// returns false on failure (e.g., trouble writing to flash)
bool fs_delete_file(char* name);

// Open the file, read into RAM last block
// name is a single ASCII letter
// returns false on failure (e.g., trouble writing to flash)
bool fs_wopen(char* name);

// Open the file, read first block into RAM
// input: file name is a single ASCII letter
// returns false on failure (e.g., trouble read to flash)
bool fs_ropen(char* name);

// Save at end of the open file
// input: data to be saved
// returns false on failure (e.g., trouble writing to flash)
bool fs_write(uint8_t data);

// retreive data from open file
// returns output by reference
bool fs_read(uint8_t* output);

// Close the file, leaving disk in a state power can be removed
// returns false on failure (e.g., trouble writing to flash)
bool fs_close_wfile(void);

// close the reading file
// returns false on failure (e.g., wasn't open)
bool fs_close_rfile(void);
