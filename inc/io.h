#include <stdbool.h>
#include <stdint.h>

void uart_init(void);

bool putchar(char x);
bool puts(const char* str);
char getchar(void);
uint16_t gets(char* str, uint16_t len);
uint16_t readline(char* str, uint16_t len);
