#include <stdint.h>

void switch1_init(void (*task)(void), uint8_t priority);
void switch2_init(void (*task)(void), uint8_t priority);
