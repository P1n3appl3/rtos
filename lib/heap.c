#include "heap.h"
#include "OS.h"
#include "io.h"
#include "printf.h"
#include "std.h"
#include <stdint.h>

const uint32_t MIN_ALLOCATION = 4;

typedef struct HeapNode {
    struct HeapNode* next;
    uint32_t size;
} HeapNode;

extern uint32_t _heap;
extern uint32_t _eheap;

static HeapNode* head;
static Sema4 heap_mutex;

static uint16_t total_heap_size;
static uint16_t free_space;
static uint16_t used_space;

void heap_init(void) {
    total_heap_size = (uint32_t)&_eheap - (uint32_t)&_heap;
    free_space = total_heap_size - sizeof(HeapNode);
    used_space = 0;
    head = (HeapNode*)&_heap;
    *head = (HeapNode){0, free_space};
    OS_InitSemaphore(&heap_mutex, 0);
}

void* malloc(uint32_t size) {
    if (size % 4) {
        size += 4 - size % 4; // 4 byte alignment
    }
    OS_Wait(&heap_mutex);
    HeapNode* prev;
    HeapNode* current = head;
    while (current) {
        if (current->size >= size) {
            // split node if large enough
            if (current->size - size >= sizeof(HeapNode) + MIN_ALLOCATION) {
                HeapNode* new =
                    (HeapNode*)(((uint8_t*)current) + sizeof(HeapNode) +
                                max(size, MIN_ALLOCATION));
                new->next = current->next;
                new->size = current->size - sizeof(HeapNode) - size;
                current->size = max(size, MIN_ALLOCATION);
                current->next = new;
                free_space -= sizeof(HeapNode);
            }
            if (current == head) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            used_space += current->size;
            free_space -= current->size;
            OS_Signal(&heap_mutex);
            return ((uint8_t*)current) + sizeof(HeapNode);
        }
        prev = current;
        current = current->next;
    }
    OS_Signal(&heap_mutex);
    return 0;
}

void* calloc(uint32_t size) {
    uint8_t* temp = (uint8_t*)malloc(size);
    if (temp) {
        memset(temp, 0, size);
    }
    return temp;
}

void* realloc(void* allocation, uint32_t size) {
    // TODO: grow into contiguous block if possible to avoid copying
    HeapNode* this = (HeapNode*)((uint8_t*)allocation - sizeof(HeapNode));
    if (this->size > size) {
        // TODO: create new block from shrunken memory
        return allocation;
    }
    void* new = malloc(size);
    if (new) {
        memcpy(new, allocation, this->size);
        free(allocation);
    }
    return new;
}

void free(void* allocation) {
    if (!allocation) {
        __asm("BKPT"); // you've fucked up
    }
    HeapNode* this = (HeapNode*)((uint8_t*)allocation - sizeof(HeapNode));
    OS_Wait(&heap_mutex);
    HeapNode* current = head;
    HeapNode* prev;
    while (current && this > current) {
        prev = current;
        current = current->next;
    }
    used_space -= this->size;
    free_space += this->size;
    // TODO: merge adjacent blocks to defragment
    if (current == head) {
        this->next = head;
        head = this;
    } else {
        prev->next = this;
        this->next = current;
    }
    OS_Signal(&heap_mutex);
}

void heap_stats(void) {
    puts("Heap stats:");
    printf("Heap size  = %d\n\r", total_heap_size);
    printf("Heap used  = %d\n\r", used_space);
    printf("Heap free  = %d\n\r", free_space);
    printf("Heap waste = %d\n\r", total_heap_size - used_space - free_space);
}

uint32_t heap_get_size(void) {
    return total_heap_size;
}

uint32_t heap_get_space(void) {
    return free_space;
}

uint32_t heap_get_max(void) {
    uint16_t largest = 0;
    OS_Wait(&heap_mutex);
    HeapNode* current = head;
    while (current) {
        largest = max(largest, current->size);
        current = current->next;
    }
    OS_Signal(&heap_mutex);
    return largest;
}
