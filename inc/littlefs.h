#include <stdbool.h>

bool littlefs_init(void);

bool littlefs_format(void);

bool littlefs_mount(void);

bool littlefs_open_file(const char* name);

bool littlefs_read_file(char* output);

bool littlefs_close_file(void);

bool littlefs_close(void);

void debug_test(void);
