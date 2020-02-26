#pragma once
#include <stdbool.h>
#include <stdint.h>

#define STACK_SIZE 256

typedef struct tcb {
    uint32_t* sp;
    struct tcb* next_tcb;
    struct tcb* prev_tcb;
    struct tcb* next_blocked;
    uint32_t sleep_time;
    bool sleep;
    bool blocked;
    bool alive;
    uint8_t id;
    uint32_t stack[STACK_SIZE];
} TCB;
