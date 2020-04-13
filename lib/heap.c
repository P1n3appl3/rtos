#include "heap.h"
#include "std.h"
#include <stdint.h>

typedef struct HeapNode {
    struct HeapNode* next;
    uint32_t size;
} HeapNode;

extern uint32_t _heap;
extern uint32_t _eheap;

static HeapNode* head;

void heap_init(void) {
    *head = (HeapNode){0, _eheap - _heap};
}

void* malloc(uint32_t bytes) {
    return 0; // NULL
}

void* calloc(uint32_t bytes) {
    uint8_t* temp = (uint8_t*) malloc(bytes);
    return 0; // NULL
}

void* realloc(void* allocation, uint32_t bytes) {
    return 0; // NULL
}

void free(void* allocation) {
    return;
}

void heap_stats(heap_stats_t* stats) {
    return;
}
