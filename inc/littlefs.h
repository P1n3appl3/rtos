#include <stdbool.h>
#include <stdint.h>

bool littlefs_init(void);

bool littlefs_mount(void);

bool littlefs_format(void);

bool littlefs_open_file(const char* name, bool create);

bool littlefs_read_file(uint8_t* output);

// These return number of bytes read/written or -1 on error
int32_t littlefs_read_buffer(void* buffer, uint32_t size);
int32_t littlefs_write_buffer(void* buffer, uint32_t size);

bool littlefs_append(uint8_t c);

bool littlefs_close_file(void);

bool littlefs_close(void);

bool littlefs_remove(const char* name);

bool littlefs_move(const char* name, const char* new_name);

bool littlefs_seek(int32_t off);

bool littlefs_ls(void);

// Get position in file
int32_t littlefs_tell(void);

// Check that the filesystem is working
void littlefs_test(void);
