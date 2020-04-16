#include "OS.h"
#include "eDisk.h"
#include "io.h"
#include "lfs.h"
#include "printf.h"

static uint8_t read_buffer[512];
static uint8_t prog_buffer[512];
static uint8_t erase_buffer[512] = {0};
static uint8_t lookahead_buffer[32] __attribute__((aligned(4)));

static int littlefs_prog(const struct lfs_config* c, lfs_block_t block,
                         lfs_off_t off, const void* buffer, lfs_size_t size) {
    eDisk_Write(buffer, block, size / 512);
    return 0;
}

static int littlefs_read(const struct lfs_config* c, lfs_block_t block,
                         lfs_off_t off, void* buffer, lfs_size_t size) {
    eDisk_Read(buffer, block, size / 512);
    return 0;
}

static int littlefs_erase(const struct lfs_config* c, lfs_block_t block) {
    eDisk_Write(erase_buffer, block, 1);
    return 0;
}

static int littlefs_sync(const struct lfs_config* c) {
    return 0;
}

// variables used by the filesystem
static lfs_t lfs;
static lfs_file_t file;

const struct lfs_config cfg = {
    // block device operations
    .read = &littlefs_read,
    .prog = &littlefs_prog,
    .erase = &littlefs_erase,
    .sync = &littlefs_sync,

    // block device configuration
    .read_size = 512,
    .prog_size = 512,
    .block_size = 512,
    .block_count = 32768,
    .cache_size = 512,
    .lookahead_size = 32,
    .block_cycles = 500,

    // buffers
    .read_buffer = &read_buffer,
    .prog_buffer = &prog_buffer,
    .lookahead_buffer = &lookahead_buffer,
};

bool littlefs_init(void) {
    return !eDisk_Init();
}

bool littlefs_format(void) {
    return lfs_format(&lfs, &cfg) >= 0;
}

bool littlefs_mount(void) {
    return lfs_mount(&lfs, &cfg) >= 0;
}

bool littlefs_open_file(const char* name) {
    return lfs_file_open(&lfs, &file, name, LFS_O_RDWR | LFS_O_CREAT) >= 0;
}

bool littlefs_read_file(char* output) {
    uint32_t ret = lfs_file_read(&lfs, &file, output, sizeof(char));
    if (ret < 1) {
        return false;
    }
    return true;
}

bool littlefs_close_file(void) {
    return lfs_file_close(&lfs, &file) >= 0;
}

bool littlefs_close(void) {
    return lfs_unmount(&lfs) >= 0;
}

bool littlefs_remove(const char* name) {
    return lfs_remove(&lfs, name) >= 0;
}

bool littlefs_append(char c) {
    return lfs_file_write(&lfs, &file, &c, 1) >= 0;
}

void debug_test(void) {
    if (eDisk_Init()) {
        printf("error: edisk init\n\r");
        OS_Kill();
    }

    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        printf("formatting\n\r");
        err = lfs_format(&lfs, &cfg);
        printf("mounting: %d\n\r", err);
        lfs_mount(&lfs, &cfg);
    }

    // read current count
    uint32_t boot_count = 0;
    lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(&lfs, &file);
    err = lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);

    // release any resources we were using
    lfs_unmount(&lfs);

    // print the boot count
    printf("boot_count: %d\n\r", boot_count);
    OS_Kill();
}
