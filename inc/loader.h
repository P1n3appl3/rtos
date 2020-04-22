// Written by Martin Ribelotta and Andreas Gerstlauer

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Protection flags of memory
typedef enum {
    ELF_SEC_WRITE = 0x1, // Enable for write
    ELF_SEC_READ = 0x2,  // Enable for read
    ELF_SEC_EXEC = 0x4,  // Enable for execution (instruction fetch)
} ELFSecPerm_t;

// Exported symbol struct
typedef struct {
    const char* name; // Name of symbol
    void* ptr;        // Pointer of symbol in memory
} ELFSymbol_t;

// Environment for execution
typedef struct {
    const ELFSymbol_t* exported; // Pointer to exported symbols array
    uint32_t exported_size;      // Number of elements in the array
} ELFEnv_t;

// Execute ELF file from "path" with environment "env"
extern bool exec_elf(const char* path, const ELFEnv_t* env);
