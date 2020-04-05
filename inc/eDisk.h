#include <stdint.h>

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) connected to PA4 (SSI0Rx)
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss) <- GPIO high to disable TFT
// CARD_CS (pin 5) connected to PB0 GPIO output
// Data/Command (pin 4) connected to PA6 (GPIO)<- GPIO low not using TFT
// RESET (pin 3) connected to PA7 (GPIO)<- GPIO high to disable TFT
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

typedef enum {
    STA_OK = 0,
    STA_NOINIT = 1,  // Drive not initialized
    STA_NODISK = 2,  // No medium in the drive
    STA_PROTECT = 4, // Write protected
} DSTATUS;

// Results of Disk Functions
typedef enum {
    RES_OK = 0, // 0: Successful
    RES_ERROR,  // 1: R/W Error
    RES_WRPRT,  // 2: Write Protected
    RES_NOTRDY, // 3: Not Ready
    RES_PARERR  // 4: Invalid Parameter
} DRESULT;

// Initialize the interface to the SD card
// Since this program initializes the disk, it must run with the disk periodic
// task operating.
DSTATUS eDisk_Init();

// Checks the status of the card
DSTATUS eDisk_Status();

// Read data from the SD card
// count: number of sectors to read (1..255)
DRESULT eDisk_Read(uint8_t* buff, uint32_t sector, uint8_t count);

// Read 512-byte block from SD card
DRESULT eDisk_ReadBlock(void* buff, uint32_t sector);

// Write bytes to SD card.
// count: number of sectors to write (1..255)
DRESULT eDisk_Write(const uint8_t* buff, uint32_t sector, uint8_t count);

// Write one block to the SD card  (read to RAM)
DRESULT eDisk_WriteBlock(const void* buff, uint32_t sector);

void SSI0_Init(unsigned long CPSDVSR);

// This implements timeout functions (should be called every ms)
void disk_timerproc(void);

// General purpose function for all disk I/O
DRESULT disk_ioctl(uint8_t cmd, void* buff);

// Command codes for disk_ioctrl fucntion:

// Generic command (Used by FatFs)
#define CTRL_SYNC                                                              \
    0 // Complete pending write process (needed at _FS_READONLY == 0)
#define GET_SECTOR_COUNT 1 // Get media size (needed at _USE_MKFS == 1)
#define GET_SECTOR_SIZE 2  // Get sector size (needed at _MAX_SS != _MIN_SS)
#define GET_BLOCK_SIZE 3   // Get erase block size (needed at _USE_MKFS == 1)
#define CTRL_TRIM                                                              \
    4 // Inform device that the data on the block of sectors is no longer used

// Generic command (Not used by FatFs)
#define CTRL_FORMAT 5     // Create physical format on the media
#define CTRL_POWER_IDLE 6 // Put the device idle state
#define CTRL_POWER_OFF 7  // Put the device off state
#define CTRL_LOCK 8       // Lock media removal
#define CTRL_UNLOCK 9     // Unlock media removal
#define CTRL_EJECT 10     // Eject media

// MMC/SDC specific command (Not used by FatFs)
#define MMC_GET_TYPE 50   // Get card type
#define MMC_GET_CSD 51    // Get CSD
#define MMC_GET_CID 52    // Get CID
#define MMC_GET_OCR 53    // Get OCR
#define MMC_GET_SDSTAT 54 // Get SD status

// ATA/CF specific command (Not used by FatFs)
#define ATA_GET_REV 60   // Get F/W revision
#define ATA_GET_MODEL 61 // Get model name
#define ATA_GET_SN 62    // Get serial number

// MMC card type flags (MMC_GET_TYPE)
#define CT_MMC 0x01              // MMC ver 3
#define CT_SD1 0x02              // SD ver 1
#define CT_SD2 0x04              // SD ver 2
#define CT_SDC (CT_SD1 | CT_SD2) // SD
#define CT_BLOCK 0x08            // Block addressing
