#include <stdint.h>

void uart_init(void);

// non-blocking
void send_byte(uint8_t x);
