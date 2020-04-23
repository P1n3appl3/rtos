#include <stdint.h>

#define PF2 (*((volatile uint32_t*)0x40025010))
#define PF3 (*((volatile uint32_t*)0x40025020))

volatile uint32_t id = 0; // bss

volatile uint32_t other; // noinit

const int x = 17; // rodata

int y = 5; // data

__attribute__((naked)) void (*load_function(const char* name))(void) {
    __asm("SVC #0");
    __asm("BX LR");
}

void main(void) { // text
    uint32_t (*OS_Id)(void) = (uint32_t(*)(void))load_function("OS_Id");
    uint32_t (*printf)(const char*, ...) =
        (uint32_t(*)(const char*, ...))load_function("printf");
    id = OS_Id();
    if ((id && y) || other) {
        PF2 ^= 0x04;
    } else {
        PF3 ^= 0x08;
    }
    printf("Executed an entire loaded binary! (pid=%d)\n\r", id);
}
