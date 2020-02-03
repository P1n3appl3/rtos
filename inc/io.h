#include <stdbool.h>
#include <stdint.h>

void uart_init(void);

bool putchar(char x);
bool puts(const char* str);
char getchar(void);
