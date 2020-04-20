#ifndef LOADER_CONFIG_H_
#define LOADER_CONFIG_H_

#include "OS.h"
#include "heap.h"
#include "io.h"
#include "littlefs.h"
#include "printf.h"
#include "std.h"
#include <stdint.h>

#define VALVANOWARE

// Declare extern OS_AddProcess function (implemented in OS.c)
int OS_AddProcess(void (*entry)(void), void* text, void* data,
                  unsigned long stackSize, unsigned long priority);

typedef unsigned long int off_t;
typedef void(entry_t)(void);

void LOADER_CLEAR(void* ptr, size_t size) {
    int i;
    int32_t* p;
    for (p = ptr, i = 0; i < size / sizeof(int32_t); i++, p++) *p = 0;
}

#define LOADER_JUMP_TO(entry, text, data)                                      \
    OS_AddProcess(entry, text, data, 1024, 1)

// #define DBG(msg, par) printf(msg, par)
#define DBG(msg, par)
#define ERR(msg) puts("ELF: " msg "\n\r")
#define MSG(msg) puts("ELF: " msg "\n\r")

#endif /* LOADER_CONFIG_H_ */
