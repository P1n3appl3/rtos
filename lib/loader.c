// Based on loader by Martin Ribelotta and Andreas Gerstlauer

#include "OS.h"
#include "heap.h"
#include "io.h"
#include "littlefs.h"
#include "printf.h"
#include "std.h"
#include <stddef.h>

const uint32_t user_process_stack = 1024;

#define ERR(msg) puts(RED "ELF ERROR: " NORMAL msg)
#define SEGMENT_OFFSET(e, n) (e->programHeaderTable + n * sizeof(ProgramHeader))

typedef struct {
    char ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} Header;

typedef struct {
    void* data;
    int segIdx;
} Segment;

typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
} ProgramHeader;

typedef struct {
    size_t type;
    uint32_t entry;

    size_t segments;
    uint32_t programHeaderTable;

    Segment load_text;
    Segment load_data;
} Executable;

static void free_segment(Segment* s) {
    if (s->data)
        free(s->data);
}

static bool load_segment(Executable* e, Segment* s, ProgramHeader* h) {
    if (!h->memsz) {
        printf(" No data for section");
        return true;
    }
    s->data = malloc(h->memsz);
    if (!s->data) {
        ERR("    GET MEMORY fail");
        return false;
    }
    if (!littlefs_seek(h->offset)) {
        ERR("    seek fail");
        free_segment(s);
        return false;
    }
    if (littlefs_read_buffer(s->data, h->filesz) != h->filesz) {
        ERR("     read data fail");
        return false;
    }
    if (h->memsz > h->filesz) {
        memset((uint8_t*)s->data + h->filesz, 0, h->memsz - h->filesz);
    }
    return true;
}

static bool read_segment_header(Executable* e, int n, ProgramHeader* h) {
    uint32_t offset = SEGMENT_OFFSET(e, n);
    return littlefs_seek(offset) &&
           littlefs_read_buffer(h, sizeof(ProgramHeader)) ==
               sizeof(ProgramHeader);
}

static bool init_elf(Executable* e) {
    memset(e, 0, sizeof(Executable));

    Header h;
    if (littlefs_read_buffer(&h, sizeof(h)) != sizeof(h)) {
        return false;
    }

    e->entry = h.entry;
    e->type = h.type;
    e->segments = h.phnum;
    e->programHeaderTable = h.phoff;

    // TODO: Check ELF validity using magic numbers
    return true;
}

static bool jump_to(uint32_t ofs, void* text, void* data) {
    if (ofs) {
        return OS_AddProcess((void (*)(void))((uint8_t*)text + ofs), text, data,
                             user_process_stack, 0);
    } else {
        ERR("No entry defined.");
        return false;
    }
}

bool exec_elf(const char* path) {
    static Executable exec; // this is only static to save stack space
    if (!(littlefs_open_file(path, false) && init_elf(&exec))) {
        printf("Invalid elf %s\n\r", path);
        return false;
    }
    if (exec.type != 2) { // 2 == EXEC
        ERR("Not of type EXEC");
        return false;
    }

    bool found_writable = false, found_executable = false;
    for (int n = 0; n < exec.segments; n++) {
        ProgramHeader ph;
        if (!read_segment_header(&exec, n, &ph)) {
            ERR("Failed to read segment");
            return false;
        }
        if (ph.type != 1) { // 1 == LOAD
            printf("Skipping segment %d\n\r", n);
            continue;
        }
        printf("Examining segment %d\n\r", n);

        if (ph.flags & 2) { // 2 == W
            if (!load_segment(&exec, &exec.load_data, &ph)) {
                return false;
            }
            exec.load_data.segIdx = n;
            if (found_writable) {
                ERR("Can't handle multiple writable segments");
                return false;
            }
            found_writable = true;
        } else if (ph.flags & 1) { // 1 == X
            if (!load_segment(&exec, &exec.load_text, &ph)) {
                return false;
            }
            exec.load_text.segIdx = n;
            if (found_executable) {
                ERR("Can't handle multiple executable segments");
                return false;
            }
            found_executable = true;
        } else {
            ERR("Section wasn't writable or executable");
            return false;
        }
    }
    bool ret = jump_to(exec.entry, exec.load_text.data, exec.load_data.data);
    littlefs_close_file();
    return ret;
}
