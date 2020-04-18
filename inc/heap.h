#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct HeapNode {
    struct HeapNode* next;
    uint16_t size;
} HeapNode;

void heap_init(void);

// Allocate uninitionalized heap space. Returns NULL if OOM.
void* malloc(uint32_t size);

// Allocate zero-initialized heap space. Returns NULL if OOM.
void* calloc(uint32_t size);

// Change size of allocation (may move data). Returns NULL if OOM.
// If the allocation passed is NULL, acts like malloc(size)
void* realloc(void* allocation, uint32_t size);

// Deallocate
void free(void* allocation);

// Print usage statistics about the heap
void heap_stats(void);

// Get number of bytes available for allocation in heap
uint32_t heap_get_space(void);

// Get the size of the largest possible allocation that can currently be made
uint32_t heap_get_max(void);

// Helper for accessing heap nodes
HeapNode* heap_node_from_alloc(void* alloc);
