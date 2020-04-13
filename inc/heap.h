#pragma once

#include <stdint.h>

// struct for holding statistics on the state of the heap
typedef struct {
    uint32_t size; // heap size
    uint32_t used; // number of bytes used/allocated
    uint32_t free; // number of bytes available to allocate
} heap_stats_t;

void heap_init(void);

// Allocate uninitionalized heap space. Returns NULL if OOM.
void* malloc(uint32_t size);

// Allocate zero-initialized heap space. Returns NULL if OOM.
void* calloc(uint32_t size);

// Change size of allocation (may move data). Returns NULL if OOM.
void* realloc(void* allocation, uint32_t size);

// Deallocate
void free(void* allocation);

// Print usage statistics about the heap
void heap_stats(heap_stats_t* stats);
