#pragma once

#include "OS.h"
#include "heap.h"
#include "io.h"
#include "littlefs.h"
#include "printf.h"
#include "std.h"
#include <stdint.h>

// Declare extern OS_AddProcess function (implemented in OS.c)
int OS_AddProcess(void (*entry)(void), void* text, void* data,
                  unsigned long stackSize, unsigned long priority);

typedef uint32_t off_t;
typedef void(entry_t)(void);

#define LOADER_JUMP_TO(entry, text, data)                                      \
    OS_AddProcess(entry, text, data, 1024, 1)

#define DBG(msg, par) // printf(msg, par)
#define ERR(msg) puts("ELF: " msg "\n\r")
#define MSG(msg) puts("ELF: " msg "\n\r")
