#pragma once
#include "startup.h"

// ADDFIFO(Tx,32,unsigned char)
// SIZE must be a power of two
// creates Txfifo_init() Txfifo_get() and Txfifo_put()
#define ADDFIFO(NAME, SIZE, TYPE)                                              \
    uint16_t volatile NAME##putidx;                                            \
    uint16_t volatile NAME##getidx;                                            \
    TYPE static NAME##fifo[SIZE];                                              \
    void NAME##fifo_init(void) {                                               \
        uint32_t sr = start_critical();                                        \
        NAME##putidx = NAME##getidx = 0;                                       \
        end_critical(sr);                                                      \
    }                                                                          \
    /* returns false if NAME##fifo is full*/                                   \
    bool NAME##fifo_put(TYPE data) {                                           \
        if ((NAME##putidx - NAME##getidx) & ~(SIZE - 1)) {                     \
            return false;                                                      \
        }                                                                      \
        NAME##fifo[NAME##putidx++ & (SIZE - 1)] = data;                        \
        return true;                                                           \
    }                                                                          \
    /* returns false if NAME##fifo is empty*/                                  \
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
