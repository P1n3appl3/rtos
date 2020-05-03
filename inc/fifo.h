#pragma once

#include "OS.h"

typedef struct {
    uint8_t* buf;
    uint16_t putidx;
    uint16_t getidx;
    Sema4 data_available;
    uint16_t size;
} FIFO;

// size must be power of 2
FIFO* fifo_new(uint16_t size);
void fifo_clear(FIFO* fifo);
void fifo_free(FIFO* fifo);

void fifo_put(FIFO* fifo, uint8_t n);
uint8_t fifo_get(FIFO* fifo);

bool fifo_try_put(FIFO* fifo, uint8_t n);
bool fifo_try_get(FIFO* fifo, uint8_t* out);

bool fifo_empty(FIFO* fifo);
bool fifo_full(FIFO* fifo);

uint16_t fifo_size(FIFO* fifo);
uint16_t fifo_space(FIFO* fifo);
