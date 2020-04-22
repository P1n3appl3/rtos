#include "loader.h"
#include "elf.h"
#include "loader_config.h"

#define IS_FLAGS_SET(v, m) (((v) & (m)) == (m))
#define SECTION_OFFSET(e, n) (e->sectionTable + n * sizeof(Elf32_Shdr))
#define SEGMENT_OFFSET(e, n) (e->programHeaderTable + n * sizeof(Elf32_Phdr))

typedef struct {
    void* data;
    int secIdx;
    off_t relSecIdx;
} ELFSection_t;

typedef struct {
    void* data;
    int segIdx;
} ELFSegment_t;

typedef struct {
    size_t type;
    off_t entry;

    // Execution view
    size_t segments;
    off_t programHeaderTable;

    off_t relTable;
    size_t relCount;

    ELFSegment_t loadText;
    ELFSegment_t loadData;

    // Linking view
    size_t sections;
    off_t sectionTable;
    off_t sectionTableStrings;

    size_t symbolCount;
    off_t symbolTable;
    off_t symbolTableStrings;

    ELFSection_t text;
    ELFSection_t rodata;
    ELFSection_t data;
    ELFSection_t bss;

    void* stack;

    const ELFEnv_t* env;
} ELFExec_t;

typedef enum {
    FoundERROR = 0,
    FoundSymTab = (1 << 0),
    FoundStrTab = (1 << 2),
    FoundText = (1 << 3),
    FoundRodata = (1 << 4),
    FoundData = (1 << 5),
    FoundBss = (1 << 6),
    FoundRelText = (1 << 7),
    FoundRelRodata = (1 << 8),
    FoundRelData = (1 << 9),
    FoundRelBss = (1 << 10),
    FoundLoadData = (1 << 11),
    FoundLoadText = (1 << 12),
    FoundLoadDynamic = (1 << 13),
    FoundValid = FoundSymTab | FoundStrTab,
    FoundExec = FoundValid | FoundText,
    FoundProgram = FoundLoadData | FoundLoadText,
    FoundAll = FoundSymTab | FoundStrTab | FoundText | FoundRodata | FoundData |
               FoundBss | FoundRelText | FoundRelRodata | FoundRelData |
               FoundRelBss
} FindFlags_t;

static bool readSectionName(ELFExec_t* e, off_t off, char* buf, size_t max) {
    int ret = false;
    off_t offset = e->sectionTableStrings + off;
    off_t old = littlefs_tell();
    if (littlefs_seek(offset) && littlefs_read_buffer(buf, max) == max) {
        ret = true;
    }
    littlefs_seek(old);
    return ret;
}

static bool readSymbolName(ELFExec_t* e, off_t off, char* buf, size_t max) {
    int ret = false;
    off_t offset = e->symbolTableStrings + off;
    off_t old = littlefs_tell();
    if (littlefs_seek(offset) && littlefs_read_buffer(buf, max) == max) {
        ret = true;
    }
    littlefs_seek(old);
    return ret;
}

static void freeSection(ELFSection_t* s) {
    if (s->data)
        free(s->data);
}

static void freeSegment(ELFSegment_t* s) {
    if (s->data)
        free(s->data);
}

static bool loadSecData(ELFExec_t* e, ELFSection_t* s, Elf32_Shdr* h) {
    if (!h->sh_size) {
        MSG(" No data for section");
        return true;
    }
    s->data = malloc(h->sh_size);
    if (!s->data) {
        ERR("    GET MEMORY fail");
        return false;
    }
    if (!littlefs_seek(h->sh_offset)) {
        ERR("    seek fail");
        freeSection(s);
        return false;
    }
    if (h->sh_type != SHT_NOBITS) {
        if (littlefs_read_buffer(s->data, h->sh_size) != h->sh_size) {
            ERR("     read data fail");
            return false;
        }
    } else {
        memset(s->data, 0, h->sh_size);
    }
    return true;
}

static bool readSecHeader(ELFExec_t* e, int n, Elf32_Shdr* h) {
    off_t offset = SECTION_OFFSET(e, n);
    return littlefs_seek(offset) &&
           littlefs_read_buffer(h, sizeof(Elf32_Shdr)) == sizeof(Elf32_Shdr);
}

static bool readSection(ELFExec_t* e, int n, Elf32_Shdr* h, char* name,
                        size_t nlen) {
    if (!readSecHeader(e, n, h))
        return false;
    if (h->sh_name)
        return readSectionName(e, h->sh_name, name, nlen);
    return true;
}

static bool loadSegData(ELFExec_t* e, ELFSegment_t* s, Elf32_Phdr* h) {
    if (!h->p_memsz) {
        MSG(" No data for section");
        return true;
    }
    s->data = malloc(h->p_memsz);
    if (!s->data) {
        ERR("    GET MEMORY fail");
        return false;
    }
    if (!littlefs_seek(h->p_offset)) {
        ERR("    seek fail");
        freeSegment(s);
        return false;
    }
    if (littlefs_read_buffer(s->data, h->p_filesz) != h->p_filesz) {
        ERR("     read data fail");
        return false;
    }
    if (h->p_memsz > h->p_filesz) {
        memset((char*)s->data + h->p_filesz, 0, h->p_memsz - h->p_filesz);
    }
    return true;
}

static bool readSegHeader(ELFExec_t* e, int n, Elf32_Phdr* h) {
    off_t offset = SEGMENT_OFFSET(e, n);
    return littlefs_seek(offset) &&
           littlefs_read_buffer(h, sizeof(Elf32_Phdr)) == sizeof(Elf32_Phdr);
}

static bool readSymbol(ELFExec_t* e, int n, Elf32_Sym* sym, char* name,
                       size_t nlen) {
    int ret = false;
    off_t old = littlefs_tell();
    off_t pos = e->symbolTable + n * sizeof(Elf32_Sym);
    if (littlefs_seek(pos) &&
        littlefs_read_buffer(sym, sizeof(Elf32_Sym)) == sizeof(Elf32_Sym)) {
        if (sym->st_name)
            ret = readSymbolName(e, sym->st_name, name, nlen);
        else {
            Elf32_Shdr shdr;
            ret = readSection(e, sym->st_shndx, &shdr, name, nlen);
        }
    }
    littlefs_seek(old);
    return ret;
}

static void relJmpCall(Elf32_Addr relAddr, int type, Elf32_Addr symAddr) {
    uint16_t upper_insn = ((uint16_t*)relAddr)[0];
    uint16_t lower_insn = ((uint16_t*)relAddr)[1];
    uint32_t S = (upper_insn >> 10) & 1;
    uint32_t J1 = (lower_insn >> 13) & 1;
    uint32_t J2 = (lower_insn >> 11) & 1;

    int32_t offset = (S << 24) |                     // S     -> offset[24]
                     ((~(J1 ^ S) & 1) << 23) |       // J1    -> offset[23]
                     ((~(J2 ^ S) & 1) << 22) |       // J2    -> offset[22]
                     ((upper_insn & 0x03ff) << 12) | // imm10 -> offset[12:21]
                     ((lower_insn & 0x07ff) << 1);   // imm11 -> offset[1:11]
    if (offset & 0x01000000)
        offset -= 0x02000000;

    offset += symAddr - relAddr;

    S = (offset >> 24) & 1;
    J1 = S ^ (~(offset >> 23) & 1);
    J2 = S ^ (~(offset >> 22) & 1);

    upper_insn =
        ((upper_insn & 0xf800) | (S << 10) | ((offset >> 12) & 0x03ff));
    ((uint16_t*)relAddr)[0] = upper_insn;

    lower_insn = ((lower_insn & 0xd000) | (J1 << 13) | (J2 << 11) |
                  ((offset >> 1) & 0x07ff));
    ((uint16_t*)relAddr)[1] = lower_insn;
}

static bool relocateSymbol(Elf32_Addr relAddr, int type, Elf32_Addr symAddr) {
    switch (type) {
    case R_ARM_ABS32:
        *((uint32_t*)relAddr) += symAddr;
        DBG("  R_ARM_ABS32 relocated is 0x%08X\n\r",
            (unsigned int)*((uint32_t*)relAddr));
        break;
    case R_ARM_THM_CALL:
    case R_ARM_THM_JUMP24:
        relJmpCall(relAddr, type, symAddr);
        DBG("  R_ARM_THM_CALL/JMP relocated is 0x%08X\n\r",
            (unsigned int)*((uint32_t*)relAddr));
        break;
    default: DBG("  Undefined relocation %d\n\r", type); return false;
    }
    return true;
}

static ELFSection_t* sectionOf(ELFExec_t* e, int index) {
    if (e->text.secIdx == index) {
        return &e->text;
    } else if (e->rodata.secIdx == index) {
        return &e->rodata;
    } else if (e->data.secIdx == index) {
        return &e->data;
    } else if (e->bss.secIdx == index) {
        return &e->bss;
    }
    return 0;
}

static Elf32_Addr addressOf(ELFExec_t* e, Elf32_Sym* sym, const char* sName) {
    if (sym->st_shndx == SHN_UNDEF) {
        int i;
        for (i = 0; i < e->env->exported_size; i++)
            if (streq(e->env->exported[i].name, sName))
                return (Elf32_Addr)(e->env->exported[i].ptr);
    } else {
        ELFSection_t* symSec = sectionOf(e, sym->st_shndx);
        if (symSec)
            return ((Elf32_Addr)symSec->data) + sym->st_value;
    }
    DBG("  Can not find address for symbol %s\n\r", sName);
    return 0xffffffff;
}

static bool relocate(ELFExec_t* e, size_t relEntries, off_t relOfs, void* s) {
    if (!s) {
        MSG("Section not loaded");
        return false;
    }
    Elf32_Rel rel;
    size_t relCount;
    (void)littlefs_seek(relOfs);
    DBG(" Offset   Info     Type             Name\n\r", 0);
    for (relCount = 0; relCount < relEntries; relCount++) {
        if (littlefs_read_buffer(&rel, sizeof(rel)) == sizeof(rel)) {
            Elf32_Sym sym;
            Elf32_Addr symAddr;

            int symEntry = ELF32_R_SYM(rel.r_info);
            int relType = ELF32_R_TYPE(rel.r_info);
            Elf32_Addr relAddr = ((Elf32_Addr)s) + rel.r_offset;

            if ((relType == R_ARM_NONE) || (relType == R_ARM_RBASE))
                continue;

            char name[33] = "<unnamed>";
            readSymbol(e, symEntry, &sym, name, sizeof(name));
            DBG(" %08X", (unsigned int)rel.r_offset);
            DBG(" %08X", (unsigned int)rel.r_info);
            DBG(" %s\n\r", name);

            symAddr = addressOf(e, &sym, name);
            if (symAddr != 0xffffffff) {
                DBG("  symAddr=%08X", (unsigned int)symAddr);
                DBG(" relAddr=%08X\n\r", (unsigned int)relAddr);
                if (!relocateSymbol(relAddr, relType, symAddr)) {
                    return false;
                }
            } else {
                DBG("  No symbol address of %s\n\r", name);
                return false;
            }
        }
    }
    return true;
}

static FindFlags_t placeInfo(ELFExec_t* e, Elf32_Shdr* sh, const char* name,
                             int n) {
    if (streq(name, ".symtab")) {
        e->symbolTable = sh->sh_offset;
        e->symbolCount = sh->sh_size / sizeof(Elf32_Sym);
        return FoundSymTab;
    } else if (streq(name, ".strtab")) {
        e->symbolTableStrings = sh->sh_offset;
        return FoundStrTab;
    } else if (streq(name, ".text")) {
        if (!loadSecData(e, &e->text, sh))
            return FoundERROR;
        e->text.secIdx = n;
        return FoundText;
    } else if (streq(name, ".rodata")) {
        if (!loadSecData(e, &e->rodata, sh))
            return FoundERROR;
        e->rodata.secIdx = n;
        return FoundRodata;
    } else if (streq(name, ".data")) {
        if (!loadSecData(e, &e->data, sh))
            return FoundERROR;
        e->data.secIdx = n;
        return FoundData;
    } else if (streq(name, ".bss")) {
        if (!loadSecData(e, &e->bss, sh))
            return FoundERROR;
        e->bss.secIdx = n;
        return FoundBss;
    } else if (streq(name, ".rel.text")) {
        e->text.relSecIdx = n;
        return FoundRelText;
    } else if (streq(name, ".rel.rodata")) {
        e->rodata.relSecIdx = n;
        return FoundRelRodata;
    } else if (streq(name, ".rel.data")) {
        e->data.relSecIdx = n;
        return FoundRelData;
    }
    // BSS doesn't need relocation
    return FoundERROR;
}

static FindFlags_t placeDynamic(ELFExec_t* e, Elf32_Phdr* ph, int n) {
    int founded = FoundLoadDynamic;
    Elf32_Dyn dyn;
    if (!littlefs_seek(ph->p_offset))
        return FoundERROR;
    do {
        if (littlefs_read_buffer(&dyn, sizeof(Elf32_Dyn)) != sizeof(Elf32_Dyn))
            return FoundERROR;
        if (dyn.d_tag == DT_STRTAB) {
            e->symbolTableStrings = dyn.d_un.d_ptr + ph->p_offset;
            founded |= FoundStrTab;
        } else if (dyn.d_tag == DT_SYMTAB) {
            e->symbolTable = dyn.d_un.d_ptr + ph->p_offset;
            founded |= FoundSymTab;
        } else if (dyn.d_tag == DT_REL) {
            e->relTable = dyn.d_un.d_ptr + ph->p_offset;
        } else if (dyn.d_tag == DT_RELSZ) {
            e->relCount = dyn.d_un.d_val / sizeof(Elf32_Rel);
            founded |= FoundRelText;
        }
    } while (dyn.d_tag != DT_NULL);

    return founded;
}

static FindFlags_t loadSymbols(ELFExec_t* e) {
    int n;
    int founded = FoundERROR;
    MSG("Scan ELF indexs...");
    for (n = 1; n < e->sections; n++) {
        Elf32_Shdr sectHdr;
        if (!readSecHeader(e, n, &sectHdr)) {
            ERR("Error reading section");
            return FoundERROR;
        }
        char name[33] = "<unamed>";
        if (sectHdr.sh_name)
            readSectionName(e, sectHdr.sh_name, name, sizeof(name));
        DBG("Examining section %d", n);
        DBG(" %s\n\r", name);
        founded |= placeInfo(e, &sectHdr, name, n);
        if (IS_FLAGS_SET(founded, FoundAll))
            return FoundAll;
    }
    MSG("Done");
    return founded;
}

static FindFlags_t loadProgram(ELFExec_t* e) {
    int n;
    int founded = FoundERROR;
    MSG("Scan ELF segments...");
    for (n = 0; n < e->segments; n++) {
        Elf32_Phdr ph;
        if (!readSegHeader(e, n, &ph)) {
            ERR("Error reading segment");
            return FoundERROR;
        }

        if (ph.p_type == PT_DYNAMIC) {
            DBG("Examining dynamic segment %d\n\r", n);
            founded |= placeDynamic(e, &ph, n);
            continue;
        }
        if (ph.p_type != PT_LOAD)
            continue;

        DBG("Examining segment %d\n\r", n);

        if (ph.p_flags & PF_W) {
            if (!loadSegData(e, &e->loadData, &ph))
                return FoundERROR;
            e->loadData.segIdx = n;
            founded |= FoundLoadData;
        } else if (ph.p_flags & PF_X) {
            if (!loadSegData(e, &e->loadText, &ph))
                return FoundERROR;
            e->loadText.segIdx = n;
            founded |= FoundLoadText;
        }
    }
    MSG("Done");
    return founded;
}

static bool initElf(ELFExec_t* e, bool f) {
    Elf32_Ehdr h;
    Elf32_Shdr sH;

    if (!f) {
        return false;
    }

    memset(e, 0, sizeof(ELFExec_t));

    if (littlefs_read_buffer(&h, sizeof(h)) != sizeof(h)) {
        return false;
    }

    e->entry = h.e_entry;
    e->type = h.e_type;

    if (!littlefs_seek(h.e_shoff + h.e_shstrndx * sizeof(sH))) {
        return false;
    }
    if (littlefs_read_buffer(&sH, sizeof(Elf32_Shdr)) != sizeof(Elf32_Shdr)) {
        return false;
    }

    e->sections = h.e_shnum;
    e->sectionTable = h.e_shoff;
    e->sectionTableStrings = sH.sh_offset;

    e->segments = h.e_phnum;
    e->programHeaderTable = h.e_phoff;

    // TODO: Check ELF validity

    return true;
}

static void freeElf(ELFExec_t* e) {
    // freeSegment(&e->loadText);
    // freeSegment(&e->loadData);
    // freeSection(&e->text);
    // freeSection(&e->rodata);
    // freeSection(&e->data);
    // freeSection(&e->bss);
    littlefs_close_file();
}

static bool relocateSection(ELFExec_t* e, ELFSection_t* s, const char* name) {
    DBG("Relocating section %s\n\r", name);
    if (s->relSecIdx) {
        Elf32_Shdr sectHdr;
        if (readSecHeader(e, s->relSecIdx, &sectHdr)) {
            return relocate(e, sectHdr.sh_size / sizeof(Elf32_Rel),
                            sectHdr.sh_offset, s->data);
        } else {
            ERR("Error reading section header");
            return false;
        }
    } else {
        MSG("No relocation index"); // Not an error
    }
    return true;
}

static bool relocateSections(ELFExec_t* e) {
    return relocateSection(e, &e->text, ".text") &&
           relocateSection(e, &e->rodata, ".rodata") &&
           relocateSection(e, &e->data, ".data");
    // BSS doesn't need relocation
}

static bool relocateProgram(ELFExec_t* e) {
    DBG("Relocating program\n\r", 0);
    if (e->relCount) {
        return relocate(e, e->relCount, e->relTable, e->loadText.data);
    } else
        MSG("No relocation entries"); // Not an error
    return true;
}

static bool jumpTo(off_t ofs, void* text, void* data) {
    if (ofs) {
        entry_t* entry = (entry_t*)((char*)text + ofs);
        return LOADER_JUMP_TO(entry, text, data);
    } else {
        MSG("No entry defined.");
        return false;
    }
}

bool exec_elf(const char* path, const ELFEnv_t* env) {
    static ELFExec_t exec; // this is just static to save stack space
    if (!initElf(&exec, littlefs_open_file(path))) {
        DBG("Invalid elf %s\n\r", path);
        return false;
    }
    exec.env = env;

    if (exec.type == ET_EXEC) {
        int founded = 0;
        founded |= loadProgram(&exec);
        if (IS_FLAGS_SET(founded, FoundProgram)) {
            int ret = false;
            if (IS_FLAGS_SET(founded, FoundValid | FoundLoadDynamic)) {
                relocateProgram(&exec);
            }
            ret = jumpTo(exec.entry, exec.loadText.data, exec.loadData.data);
            freeElf(&exec);
            return ret;
        } else {
            MSG("Invalid PROGRAM");
        }
    } else {
        if (IS_FLAGS_SET(loadSymbols(&exec), FoundValid)) {
            int ret = false;
            if (relocateSections(&exec)) {
                ret = jumpTo(exec.entry, exec.text.data, 0);
            }
            freeElf(&exec);
            return ret;
        } else {
            MSG("Invalid EXEC");
        }
    }
    return false;
}
