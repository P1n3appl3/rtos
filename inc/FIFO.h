#pragma once

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
    void NAME##fifo_put(TYPE data) {                                           \
        while (NAME##fifo_full()) {}                                           \
        NAME##fifo[NAME##putidx] = data;                                       \
        NAME##putidx = (NAME##putidx + 1) % SIZE;                              \
    }                                                                          \
    bool NAME##fifo_get(TYPE* datapt) {                                        \
        if (NAME##fifo_empty()) {                                              \
            return false;                                                      \
        }                                                                      \
        *datapt = NAME##fifo[NAME##getidx];                                    \
        NAME##getidx = (NAME##getidx + 1) % SIZE;                              \
        return true;                                                           \
    }
