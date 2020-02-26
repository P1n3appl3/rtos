#pragma once
#include <stdbool.h>
#include <stdint.h>

#define STACK_SIZE 256

typedef struct tcb {
    uint32_t* sp;
    struct tcb* next_tcb;
    struct tcb* prev_tcb;
    uint32_t id;

    uint32_t sleep_time;
    bool sleep;

    struct tcb* next_blocked;
    bool blocked;

    bool dead;

    uint32_t stack[STACK_SIZE];
} TCB;
