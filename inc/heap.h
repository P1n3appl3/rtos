#include <stdint.h>

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
void heap_stats(void);

// Get total size of heap in bytes
uint32_t heap_get_size(void);

// Get number of bytes available for allocation in heap
uint32_t heap_get_space(void);
