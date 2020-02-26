#include <stdbool.h>
#include <stdint.h>

#define STACK_SIZE 256

typedef struct tcb {
    uint32_t* sp;
    struct tcb* next_tcb;
    struct tcb* prev_tcb;
    uint8_t id;
    bool sleep;
    uint32_t sleep_time;
    bool dead;
    uint32_t stack[STACK_SIZE];
} TCB;
