#include "heap.h"
#include "OS.h"
#include "interpreter.h"
#include "io.h"
#include "printf.h"
#include "std.h"
#include <stdint.h>

const uint32_t MIN_ALLOCATION = 4;

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

HeapNode* heap_node_from_alloc(void* alloc) {
    return (HeapNode*)((uint8_t*)alloc - sizeof(HeapNode));
}

static HeapNode* heap_next_node(HeapNode* current) {
    return (HeapNode*)((uint8_t*)current + current->size + sizeof(HeapNode));
}

static bool try_splitting_block(HeapNode* node, uint32_t size) {
    if (node->size < size + sizeof(HeapNode) + MIN_ALLOCATION) {
        return false;
    }
    HeapNode* new = (HeapNode*)(((uint8_t*)node) + sizeof(HeapNode) +
                                max(size, MIN_ALLOCATION));
    new->next = node->next;
    new->size = node->size - sizeof(HeapNode) - size;
    node->size = max(size, MIN_ALLOCATION);
    node->next = new;
    free_space -= sizeof(HeapNode);
    return true;
}

// attempt to merge two heap nodes to reduce fragmentation
// 'a' should be the node at a lower memory address
static bool try_combining(HeapNode* a, HeapNode* b) {
    if (heap_next_node(a) == b) {
        a->size += b->size + sizeof(HeapNode);
        free_space += sizeof(HeapNode);
        return true;
    }
    return false;
}

static uint32_t align4(uint32_t n) {
    return n += n % 4 ? 4 - n % 4 : 0;
}

void* _malloc(uint32_t size) {
    size = align4(size);
    HeapNode* prev;
    HeapNode* current = head;
    while (current) {
        if (current->size >= size) {
            try_splitting_block(current, size);
            if (current == head) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            used_space += current->size;
            free_space -= current->size;
            return ((uint8_t*)current) + sizeof(HeapNode);
        }
        prev = current;
        current = current->next;
    }
    return 0;
}

void _free(void* allocation) {
    if (!allocation) {
        __asm("BKPT"); // you've fucked up
    }
    HeapNode* this = heap_node_from_alloc(allocation);
    // move along the linked list until prev/current bracket 'this' in terms
    // of their location
    HeapNode* current = head;
    HeapNode* prev;
    while (current && this > current) {
        prev = current, current = current->next;
    }
    used_space -= this->size;
    free_space += this->size;
    // attempt to merge this block with the adjacent ones before and after it
    if (current == head) {
        head = this;
    } else {
        if (try_combining(prev, this)) {
            this = prev;
        } else {
            prev->next = this;
        }
    }
    this->next = try_combining(this, current) ? current->next : current;
}

void* _realloc(void* allocation, uint32_t size) {
    if (!allocation) {
        return malloc(size);
    }
    size = align4(size);
    HeapNode* this = heap_node_from_alloc(allocation);
    uint16_t original_size = this->size;
    // if shrinking, try splitting the block to return unused space
    if (this->size > size) {
        bool temp = try_splitting_block(this, size);
        if (temp) {
            free_space += sizeof(HeapNode);
            used_space -= sizeof(HeapNode);
            _free((uint8_t*)heap_next_node(this) + sizeof(HeapNode));
        }
        return allocation;
    }
    // otherwise, grow into adjacent block if possible to avoid copying
    HeapNode* current = head;
    while (current && this > current) { current = current->next; }
    if (try_combining(this, current)) {
        this->next = current->next;
        try_splitting_block(this, size);
        uint16_t additional_space = this->size - original_size;
        used_space += additional_space;
        free_space -= additional_space;
        if (current == head) {
            head = this->next;
        }
        return allocation;
    }
    // if neither works, grab a new allocation, copy, and free the old
    void* new = _malloc(size);
    if (new) {
        memcpy(new, allocation, this->size);
        _free(allocation);
    }
    return new;
}

// TODO: check whether critical sections are required or if the mutex is fine
void* malloc(uint32_t size) {
    OS_Wait(&heap_mutex);
    void* temp = _malloc(size);
    OS_Signal(&heap_mutex);
    return temp;
}

void free(void* allocation) {
    OS_Wait(&heap_mutex);
    _free(allocation);
    OS_Signal(&heap_mutex);
}

void* realloc(void* allocation, uint32_t size) {
    OS_Wait(&heap_mutex);
    void* temp = _realloc(allocation, size);
    OS_Signal(&heap_mutex);
    return temp;
}

void* calloc(uint32_t size) {
    uint8_t* temp = (uint8_t*)malloc(size);
    if (temp) {
        memset(temp, 0, size);
    }
    return temp;
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

const int graph_width = 80;
void heap_stats(void) {
    iprintf("Total bytes available: %d\n\r", total_heap_size);
    iprintf("Bytes in use: " RED "%d" NORMAL "\n\r", used_space);
    iprintf("Bytes still available: " CYAN "%d" NORMAL "\n\r", free_space);
    uint16_t wasted = total_heap_size - used_space - free_space;
    iprintf("Bytes wasted: " YELLOW "%d" NORMAL "\n\r", wasted);
    iprintf("[" RED);
    for (int i = 0; i < graph_width; ++i) {
        if (used_space * graph_width / total_heap_size == i) {
            iprintf(YELLOW);
        }
        if ((used_space + wasted) * graph_width / total_heap_size == i) {
            iprintf(CYAN);
        }
#if defined(rpc_server) || defined(telnet_server)
        iputs("#");
#else
        putchar('#');
#endif
    }
    iputs(NORMAL "]");
    iprintf("\nLargest possible allocation: %d bytes.\n\r", heap_get_max());
}
