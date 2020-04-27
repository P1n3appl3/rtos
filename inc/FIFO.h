#pragma once

#include "OS.h"

// ADDFIFO(tx, 32, char) Creates txfifo_init(), txfifo_size(),
// txfifo_full(), txfifo_empty(), txfifo_get() and txfifo_put()
#define ADDFIFO(NAME, SIZE, TYPE)                                              \
    uint16_t volatile NAME##putidx;                                            \
    uint16_t volatile NAME##getidx;                                            \
    TYPE static NAME##fifo[SIZE];                                              \
    void NAME##fifo_init(void) { NAME##putidx = NAME##getidx = 0; }            \
    bool NAME##fifo_empty() { return NAME##putidx == NAME##getidx; }           \
    uint16_t NAME##fifo_size(void) {                                           \
        return ((uint16_t)(NAME##putidx + SIZE - NAME##getidx) % SIZE);        \
    }                                                                          \
    bool NAME##fifo_full() { return NAME##fifo_size() == SIZE - 1; }           \
    bool NAME##fifo_put(TYPE data) {                                           \
        if (NAME##fifo_full()) {                                               \
            return false;                                                      \
        }                                                                      \
        NAME##fifo[NAME##putidx] = data;                                       \
        NAME##putidx = (NAME##putidx + 1) % SIZE;                              \
        return true;                                                           \
    }                                                                          \
    bool NAME##fifo_get(TYPE* datapt) {                                        \
        if (NAME##fifo_empty()) {                                              \
            return false;                                                      \
        }                                                                      \
        *datapt = NAME##fifo[NAME##getidx];                                    \
        NAME##getidx = (NAME##getidx + 1) % SIZE;                              \
        return true;                                                           \
    }

#define ADDFIFOSYNC(NAME, SIZE, TYPE)                                          \
    uint16_t volatile NAME##putidx;                                            \
    uint16_t volatile NAME##getidx;                                            \
    Sema4 NAME##_data_available;                                               \
    TYPE static NAME##fifo[SIZE];                                              \
    void NAME##fifo_init(void) {                                               \
        NAME##putidx = NAME##getidx = 0;                                       \
        OS_InitSemaphore(&NAME##_data_available, -1);                          \
    }                                                                          \
    bool NAME##fifo_empty() { return NAME##putidx == NAME##getidx; }           \
    uint16_t NAME##fifo_size(void) {                                           \
        return ((uint16_t)(NAME##putidx + SIZE - NAME##getidx) % SIZE);        \
    }                                                                          \
    bool NAME##fifo_full() { return NAME##fifo_size() == SIZE - 1; }           \
    bool NAME##fifo_put(TYPE data) {                                           \
        if (NAME##fifo_full()) {                                               \
            return false;                                                      \
        }                                                                      \
        NAME##fifo[NAME##putidx] = data;                                       \
        NAME##putidx = (NAME##putidx + 1) % SIZE;                              \
        OS_Signal(&NAME##_data_available);                                     \
        return true;                                                           \
    }                                                                          \
    bool NAME##fifo_get(TYPE* datapt) {                                        \
        OS_Wait(&NAME##_data_available);                                       \
        *datapt = NAME##fifo[NAME##getidx];                                    \
        NAME##getidx = (NAME##getidx + 1) % SIZE;                              \
        return true;                                                           \
    }
