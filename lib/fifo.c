#include "fifo.h"
#include "OS.h"
#include "heap.h"

FIFO* fifo_new(uint16_t size) {
    if (size & size - 1) { // Size must be power of 2
        return 0;
    }
    FIFO* temp = malloc(sizeof(FIFO));
    if (!temp) {
        return 0;
    }
    temp->size = size - 1;
    temp->buf = malloc(size);
    if (!temp->buf) {
        free(temp);
        return 0;
    }
    temp->putidx = temp->getidx = 0;
    OS_InitSemaphore(&temp->data_available, -1);
    return temp;
}

void fifo_clear(FIFO* fifo) {
    fifo->putidx = fifo->getidx = 0;
}

void fifo_free(FIFO* fifo) {
    free(fifo->buf);
    free(fifo);
}

void fifo_put(FIFO* fifo, uint8_t n) {
    while (!fifo_space(fifo)) { OS_Suspend(); }
    fifo_try_put(fifo, n);
}

uint8_t fifo_get(FIFO* fifo) {
    OS_Wait(&fifo->data_available);
    uint8_t temp;
    fifo_try_get(fifo, &temp);
    return temp;
}

bool fifo_try_put(FIFO* fifo, uint8_t n) {
    if (fifo_full(fifo)) {
        return false;
    }
    fifo->buf[fifo->putidx++] = n;
    fifo->putidx &= fifo->size;
    return true;
}

bool fifo_try_get(FIFO* fifo, uint8_t* out) {
    if (fifo_empty(fifo)) {
        return false;
    }
    *out = fifo->buf[fifo->getidx++];
    fifo->getidx &= fifo->size;
    return true;
}

bool fifo_empty(FIFO* fifo) {
    return !fifo_size(fifo);
}

bool fifo_full(FIFO* fifo) {
    return !fifo_space(fifo);
}

uint16_t fifo_size(FIFO* fifo) {
    return (fifo->putidx - fifo->getidx) & fifo->size;
}

uint16_t fifo_space(FIFO* fifo) {
    return fifo->size - fifo_size(fifo) - 1;
}
