#include "OS.h"
#include "eDisk.h"
#include "heap.h"
#include "io.h"
#include "lfs.h"
#include "printf.h"

#define BLOCK_SIZE 512

const static uint8_t erase_buffer[BLOCK_SIZE] = {0};

static int block_prog(const struct lfs_config* c, lfs_block_t block,
                      lfs_off_t off, const void* buffer, lfs_size_t size) {
    eDisk_Write(buffer, block, size / BLOCK_SIZE);
    return 0;
}

static int block_read(const struct lfs_config* c, lfs_block_t block,
                      lfs_off_t off, void* buffer, lfs_size_t size) {
    eDisk_Read(buffer, block, size / BLOCK_SIZE);
    return 0;
}

static int block_erase(const struct lfs_config* c, lfs_block_t block) {
    eDisk_Write(erase_buffer, block, 1);
    return 0;
}

static int sync(const struct lfs_config* c) {
    return 0;
}

// these structs are used used by the filesystem
static lfs_t* lfs;
static lfs_file_t* file;

static const struct lfs_config cfg = {
    // block device operations
    .read = block_read,
    .prog = block_prog,
    .erase = block_erase,
    .sync = sync,

    // block device configuration
    .read_size = BLOCK_SIZE,
    .prog_size = BLOCK_SIZE,
    .block_size = BLOCK_SIZE,
    .block_count = 1 << 21, // 1GiB
    .cache_size = BLOCK_SIZE,
    .lookahead_size = 32,
    .block_cycles = 500,

    .name_max = 63,
};

bool littlefs_init(void) {
    if (!lfs) {
        lfs = malloc(sizeof(lfs_t));
    }
    if (!file) {
        file = malloc(sizeof(lfs_file_t));
    }
    return (!eDisk_Init()) && lfs && file;
}

bool littlefs_format(void) {
    return lfs_format(lfs, &cfg) >= 0;
}

bool littlefs_mount(void) {
    return lfs_mount(lfs, &cfg) >= 0;
}

bool littlefs_open_file(const char* name, bool create) {
    return lfs_file_open(lfs, file, name,
                         LFS_O_RDWR | (create ? LFS_O_CREAT : 0)) >= 0;
}

bool littlefs_read(uint8_t* output) {
    return lfs_file_read(lfs, file, output, 1) == 1;
}

int32_t littlefs_read_buffer(void* buffer, uint32_t size) {
    return lfs_file_read(lfs, file, buffer, size);
}

bool littlefs_seek(int32_t off) {
    return lfs_file_seek(lfs, file, off, LFS_SEEK_SET) > -1;
}

int32_t littlefs_tell() {
    return lfs_file_tell(lfs, file);
}

int32_t littlefs_write_buffer(void* buffer, uint32_t size) {
    return lfs_file_write(lfs, file, buffer, size) >= 0;
}

bool littlefs_close_file(void) {
    return lfs_file_close(lfs, file) >= 0;
}

bool littlefs_unmount(void) {
    return lfs_unmount(lfs) >= 0;
}

bool littlefs_remove(const char* name) {
    return lfs_remove(lfs, name) >= 0;
}

bool littlefs_move(const char* name, const char* new_name) {
    return lfs_rename(lfs, name, new_name) >= 0;
}

bool littlefs_write(uint8_t c) {
    return lfs_file_write(lfs, file, &c, 1) >= 0;
}

bool littlefs_ls(void) {
    lfs_dir_t dir;
    if (lfs_dir_open(lfs, &dir, "")) { // empty name for root dir
        return false;
    }
    struct lfs_info info;
    while (true) {
        int result = lfs_dir_read(lfs, &dir, &info);
        if (result < 0) {
            return false;
        } else if (!result) {
            break;
        } else if (info.type == LFS_TYPE_DIR) {
            continue; // we are only concerned with the root
        }
        printf("%-32s %d bytes\n\r", info.name, info.size);
    }
    return !lfs_dir_close(lfs, &dir);
}

void littlefs_test(void) {
    if (eDisk_Init()) {
        printf("error: edisk init\n\r");
        OS_Kill();
    }

    // mount the filesystem
    int err = lfs_mount(lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        printf("formatting\n\r");
        err = lfs_format(lfs, &cfg);
        printf("mounting: %d\n\r", err);
        lfs_mount(lfs, &cfg);
    }

    // read current count
    uint32_t boot_count = 0;
    lfs_file_open(lfs, file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(lfs, file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(lfs, file);
    err = lfs_file_write(lfs, file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed
    // successfully
    lfs_file_close(lfs, file);

    // release any resources we were using
    lfs_unmount(lfs);

    // print the boot count
    printf("boot_count: %d\n\r", boot_count);
    OS_Kill();
}
