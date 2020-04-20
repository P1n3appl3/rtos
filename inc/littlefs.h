#include <stdbool.h>
#include <stdint.h>

bool littlefs_init(void);

bool littlefs_format(void);

bool littlefs_mount(void);

bool littlefs_open_file(const char* name);

bool littlefs_read_file(char* output);

int32_t littlefs_read_buffer(void* buffer, uint32_t size);

bool littlefs_write_buffer(void* buffer, uint32_t size);

bool littlefs_append(char c);

bool littlefs_close_file(void);

bool littlefs_close(void);

bool littlefs_remove(const char* name);

void littlefs_test(void);

int8_t littlefs_seek(uint32_t off);

int32_t littlefs_tell(void);
