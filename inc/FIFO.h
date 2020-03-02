#pragma once
#include "interrupts.h"
#include "OS.h"

// ADDFIFO(Tx, 32, char) Creates Txfifo_init(), Txfifo_size(),
// Txfifo_full(), Txfifo_empty(), Txfifo_get() and Txfifo_put()
// SIZE must be power of 2
#define ADDFIFO(NAME, SIZE, TYPE)                                              \
    uint16_t volatile NAME##putidx;                                            \
    uint16_t volatile NAME##getidx;                                            \
    TYPE static NAME##fifo[SIZE];                                              \
    void NAME##fifo_init(void) {                                               \
        uint32_t sr = start_critical();                                        \
        NAME##putidx = NAME##getidx = 0;                                       \
        end_critical(sr);                                                      \
    }                                                                          \
    bool NAME##fifo_empty() { return NAME##putidx == NAME##getidx; }           \
    bool NAME##fifo_full() {                                                   \
        return (NAME##putidx - NAME##getidx) & ~(SIZE - 1);                    \
    }                                                                          \
    void NAME##fifo_put(TYPE data) {                                           \
        while (NAME##fifo_full()) {}                                           \
        NAME##fifo[NAME##putidx++ & (SIZE - 1)] = data;                        \
    }                                                                          \
    bool NAME##fifo_get(TYPE* datapt) {                                        \
        if (NAME##putidx == NAME##getidx) {                                    \
            return false;                                                      \
        }                                                                      \
        *datapt = NAME##fifo[NAME##getidx++ & (SIZE - 1)];                     \
        return true;                                                           \
    }                                                                          \
    uint16_t NAME##fifo_size(void) {                                           \
        return ((uint16_t)(NAME##putidx - NAME##getidx));                      \
    }
