#include "tivaware/gpio.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/hw_ssi.h"
#include "tivaware/hw_types.h"
#include "tivaware/pin_map.h"
#include "tivaware/rom.h"
#include "tivaware/ssi.h"
#include "tivaware/sysctl.h"

#include "eDisk.h"
#include <stdint.h>

static void chip_select(void) {
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3);
    ROM_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);
}

static void chip_deselect(void) {
    while (ROM_SSIBusy(SSI0_BASE)) {}
    ROM_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
}

// Initialize SSI0 interface to SDC
// inputs:  clock divider to set clock frequency
// outputs: none
// assumes: system clock rate is 80 MHz, SCR=0
// SSIClk = SysClk / (CPSDVSR * (1 + SCR)) = 80 MHz/CPSDVSR
// 200 for    400,000 bps slow mode, used during initialization
// 8   for 10,000,000 bps fast mode, used during disk I/O
void SSI0_Init(unsigned long CPSDVSR) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0);
    chip_deselect();
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, 0xC8); // 3, 6, and 7
    ROM_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    ROM_GPIOPinConfigure(GPIO_PA4_SSI0RX);
    ROM_GPIOPinConfigure(GPIO_PA5_SSI0TX);
    ROM_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_2);

    // PA7, PA3 high (disable LCD)
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 1);
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, 1);

    ROM_SSIDisable(SSI0_BASE);
    ROM_SSIClockSourceSet(SSI0_BASE, SSI_CLOCK_SYSTEM);
    ROM_SSIConfigSetExpClk(SSI0_BASE, ROM_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, ROM_SysCtlClockGet() / CPSDVSR, 8);
    ROM_SSIEnable(SSI0_BASE);

    // read any residual data from ssi port
    uint32_t _temp;
    while (ROM_SSIDataGetNonBlocking(SSI0_BASE, &_temp)) {}
}

// SSIClk = PIOSC / (CPSDVSR * (1 + SCR)) = 80 MHz/CPSDVSR
// 200 for   400,000 bps slow mode, used during initialization
// 8  for 10,000,000 bps fast mode, used during disk I/O
static inline void spi_clock_slow() {
    HWREG(SSI_O_CPSR) = (HWREG(SSI_O_CPSR) & ~HWREG(SSI_CPSR_CPSDVSR_M)) + 200;
}

static void spi_clock_fast() {
    HWREG(SSI_O_CPSR) = (HWREG(SSI_O_CPSR) & ~HWREG(SSI_CPSR_CPSDVSR_M)) + 8;
}

// MMC/SD command
#define CMD0 (0)           // GO_IDLE_STATE
#define CMD1 (1)           // SEND_OP_COND (MMC)
#define ACMD41 (0x80 + 41) // SEND_OP_COND (SDC)
#define CMD8 (8)           // SEND_IF_COND
#define CMD9 (9)           // SEND_CSD
#define CMD10 (10)         // SEND_CID
#define CMD12 (12)         // STOP_TRANSMISSION
#define ACMD13 (0x80 + 13) // SD_STATUS (SDC)
#define CMD16 (16)         // SET_BLOCKLEN
#define CMD17 (17)         // READ_SINGLE_BLOCK
#define CMD18 (18)         // READ_MULTIPLE_BLOCK
#define CMD23 (23)         // SET_BLOCK_COUNT (MMC)
#define ACMD23 (0x80 + 23) // SET_WR_BLK_ERASE_COUNT (SDC)
#define CMD24 (24)         // WRITE_BLOCK
#define CMD25 (25)         // WRITE_MULTIPLE_BLOCK
#define CMD32 (32)         // ERASE_ER_BLK_START
#define CMD33 (33)         // ERASE_ER_BLK_END
#define CMD38 (38)         // ERASE
#define CMD55 (55)         // APP_CMD
#define CMD58 (58)         // READ_OCR

static volatile DSTATUS Stat = STA_NOINIT; // Physical drive status

// 1kHz decrement timer stopped at zero (disk_timerproc())
static volatile uint32_t Timer1, Timer2;

static uint8_t CardType; // Card type flags

// Initialize MMC interface
static void init_spi(void) {
    SSI0_Init(200);
    chip_select();

    for (Timer1 = 10; Timer1;) {} // 10ms
}

// Exchange a byte
static uint8_t xchg_spi(uint8_t dat) {
    uint32_t rcvdat;
    // wait until SSI0 not busy/transmit FIFO empty
    while (ROM_SSIBusy(SSI0_BASE)) {}
    ROM_SSIDataPut(SSI0_BASE, dat);
    ROM_SSIDataGet(SSI0_BASE, &rcvdat);
    return rcvdat & 0xFF;
}

// Receive a byte
static uint8_t rcvr_spi(void) {
    // wait until SSI0 not busy/transmit FIFO empty
    while (ROM_SSIBusy(SSI0_BASE)) {}
    ROM_SSIDataPut(SSI0_BASE, 0xFF); // data out, garbage
    uint32_t response;
    ROM_SSIDataGet(SSI0_BASE, &response);
    return (uint8_t)(response & 0xFF);
}

// Receive multiple byte
// count: Number of bytes to receive (must be even)
static void rcvr_spi_multi(uint8_t* buff, uint32_t count) {
    while (count) {
        *buff = rcvr_spi(); // return by reference
        count--;
        buff++;
    }
}

// Send multiple bytes
// btx: Number of bytes to send (even number)
static void xmit_spi_multi(const uint8_t* buff, uint32_t btx) {
    uint32_t rcvdat;
    while (btx) {
        ROM_SSIDataPut(SSI0_BASE, *buff);   // data out
        ROM_SSIDataGet(SSI0_BASE, &rcvdat); // get response
        btx--;
        buff++;
    }
}

// Wait for card ready
// Input: time to wait in ms
// returns false on timeout
static bool wait_ready(uint32_t wt) {
    uint8_t d;
    Timer2 = wt;
    do {
        d = xchg_spi(0xFF);
        // This loop takes a time. Insert rot_rdq() here for multitask
        // environment.
    } while (d != 0xFF && Timer2); // Wait for card goes ready or timeout
    return (d == 0xFF) ? 1 : 0;
}

// Deselect card and release SPI
static void deselect(void) {
    chip_deselect();
    xchg_spi(0xFF); // Dummy clock (force DO hi-z for multiple slave SPI)
}

// Select card and wait for ready
// returns false on 500ms timeout
static bool select(void) {
    chip_select();
    xchg_spi(0xFF); // Dummy clock (force DO enabled)
    if (wait_ready(500))
        return 1; // OK
    chip_deselect();
    return 0; // Timeout
}

// Receive a data packet from the MMC
// btr: Number of bytes to receive (even number)
// returns false on timeout
static bool rcvr_datablock(uint8_t* buff, uint32_t btr) {
    uint8_t token;
    Timer1 = 200;
    do { // Wait for DataStart token in timeout of 200ms
        token = xchg_spi(0xFF);
        /* This loop will take a time. Insert rot_rdq() here for multitask
         * envilonment. */
    } while ((token == 0xFF) && Timer1);
    if (token != 0xFE)
        return 0; // Function fails if invalid DataStart token or timeout

    rcvr_spi_multi(buff, btr); // Store trailing data to the buffer
    xchg_spi(0xFF);
    xchg_spi(0xFF); // Discard CRC
    return 1;       // Function succeeded
}

// Send a data packet to the MMC
// Input:  buff: 512 byte packet which will be sent
// returns false on timeout
static bool xmit_datablock(const uint8_t* buff, uint8_t token) {
    uint8_t resp;
    if (!wait_ready(500))
        return 0; // Wait for card ready

    xchg_spi(token);               // Send token
    if (token != 0xFD) {           // Send data if token is other than StopTran
        xmit_spi_multi(buff, 512); // Data
        xchg_spi(0xFF);
        xchg_spi(0xFF); // Dummy CRC

        resp = xchg_spi(0xFF); // Receive data resp
        if ((resp & 0x1F) !=
            0x05) // Function fails if the data packet was not accepted
            return 0;
    }
    return 1;
}

// Send a command packet to the MMC
// Inputs:  Command index
// Outputs: response (bit7==1: Failed to send)
static uint8_t send_cmd(uint8_t cmd, uint32_t arg) {
    uint8_t n, res;
    if (cmd & 0x80) { // Send a CMD55 prior to ACMD<n>
        cmd &= 0x7F;
        res = send_cmd(CMD55, 0);
        if (res > 1)
            return res;
    }

    // Select the card and wait for ready except to stop multiple block read
    if (cmd != CMD12) {
        deselect();
        if (!select())
            return 0xFF;
    }

    // Send command packet
    xchg_spi(0x40 | cmd);           // Start + command index
    xchg_spi((uint8_t)(arg >> 24)); // Argument[31..24]
    xchg_spi((uint8_t)(arg >> 16)); // Argument[23..16]
    xchg_spi((uint8_t)(arg >> 8));  // Argument[15..8]
    xchg_spi((uint8_t)arg);         // Argument[7..0]
    n = 0x01;                       // Dummy CRC + Stop
    if (cmd == CMD0)
        n = 0x95; // Valid CRC for CMD0(0)
    if (cmd == CMD8)
        n = 0x87; // Valid CRC for CMD8(0x1AA)
    xchg_spi(n);

    // Receive command resp
    if (cmd == CMD12)
        xchg_spi(0xFF); // Diacard following one byte when CMD12
    n = 10;             // Wait for response (10 bytes max)
    do
        res = xchg_spi(0xFF);
    while ((res & 0x80) && --n);

    return res; // Return received response
}

DSTATUS eDisk_Init() {
    uint8_t n, cmd, ty, ocr[4];

    init_spi(); // Initialize SPI

    if (Stat & STA_NODISK)
        return Stat; // Is card existing in the socket?

    spi_clock_slow();
    for (n = 10; n; n--) xchg_spi(0xFF); // Send 80 dummy clocks

    ty = 0;
    if (send_cmd(CMD0, 0) == 1) {         // Put the card SPI/Idle state
        Timer1 = 1000;                    // Initialization timeout = 1 sec
        if (send_cmd(CMD8, 0x1AA) == 1) { // SDv2?
            for (n = 0; n < 4; n++)
                ocr[n] = xchg_spi(0xFF); // Get 32 bit return value of R7 resp
            if (ocr[2] == 0x01 &&
                ocr[3] == 0xAA) { // Is the card supports vcc of 2.7-3.6V?
                while (Timer1 && send_cmd(ACMD41, 1 << 30))
                    ; // Wait for end of initialization with ACMD41(HCS)
                if (Timer1 &&
                    send_cmd(CMD58, 0) == 0) { // Check CCS bit in the OCR
                    for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);
                    ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK
                                         : CT_SD2; // Card id SDv2
                }
            }
        } else {                            // Not SDv2 card
            if (send_cmd(ACMD41, 0) <= 1) { // SDv1 or MMC?
                ty = CT_SD1;
                cmd = ACMD41; // SDv1 (ACMD41(0))
            } else {
                ty = CT_MMC;
                cmd = CMD1; // MMCv3 (CMD1(0))
            }
            while (Timer1 && send_cmd(cmd, 0))
                ; // Wait for end of initialization
            if (!Timer1 || send_cmd(CMD16, 512) != 0) // Set block length: 512
                ty = 0;
        }
    }
    CardType = ty; // Card type
    deselect();

    if (ty) {                // OK
        spi_clock_fast();    // Set fast clock
        Stat &= ~STA_NOINIT; // Clear STA_NOINIT flag
    } else {                 // Failed
        Stat = STA_NOINIT;
    }

    return Stat;
}

DSTATUS eDisk_Status() {
    return Stat; // Return disk status
}

DRESULT eDisk_Read(uint8_t* buff, uint16_t sector, uint8_t count) {
    if (!count)
        return RES_PARERR; // Check parameter
    if (Stat & STA_NOINIT)
        return RES_NOTRDY; // Check if drive is ready

    if (!(CardType & CT_BLOCK))
        sector *= 512; // LBA ot BA conversion (byte addressing cards)

    if (count == 1) {                      // Single sector read
        if ((send_cmd(CMD17, sector) == 0) // READ_SINGLE_BLOCK
            && rcvr_datablock(buff, 512))
            count = 0;
    } else {                                // Multiple sector read
        if (send_cmd(CMD18, sector) == 0) { // READ_MULTIPLE_BLOCK
            do {
                if (!rcvr_datablock(buff, 512))
                    break;
                buff += 512;
            } while (--count);
            send_cmd(CMD12, 0); // STOP_TRANSMISSION
        }
    }
    deselect();

    return count ? RES_ERROR : RES_OK; // Return result
}

DRESULT eDisk_ReadBlock(uint8_t* buff, uint16_t sector) {
    return eDisk_Read(buff, sector, 1);
}

DRESULT eDisk_Write(const uint8_t* buff, uint16_t sector, uint8_t count) {
    if (Stat & STA_NOINIT)
        return RES_NOTRDY; // Check drive status
    if (Stat & STA_PROTECT)
        return RES_WRPRT; // Check write protect

    if (!(CardType & CT_BLOCK))
        sector *= 512; // LBA ==> BA conversion (byte addressing cards)

    if (count == 1) {                      // Single sector write
        if ((send_cmd(CMD24, sector) == 0) // WRITE_BLOCK
            && xmit_datablock(buff, 0xFE))
            count = 0;
    } else { // Multiple sector write
        if (CardType & CT_SDC)
            send_cmd(ACMD23, count);        // Predefine number of sectors
        if (send_cmd(CMD25, sector) == 0) { // WRITE_MULTIPLE_BLOCK
            do {
                if (!xmit_datablock(buff, 0xFC))
                    break;
                buff += 512;
            } while (--count);
            if (!xmit_datablock(0, 0xFD)) // STOP_TRAN token
                count = 1;
        }
    }
    deselect();

    return count ? RES_ERROR : RES_OK; // Return result
}

DRESULT eDisk_WriteBlock(const uint8_t* buff, uint16_t sector) {
    return eDisk_Write(buff, sector, 1); // 1 block
}

// Miscellaneous drive controls other than data read/write
DRESULT disk_ioctl(uint8_t cmd, void* buff) {
    DRESULT res;
    uint8_t n, csd[16];
    uint16_t *dp, st, ed, csize;

    if (Stat & STA_NOINIT)
        return RES_NOTRDY; // Check if drive is ready

    res = RES_ERROR;

    switch (cmd) {
    case CTRL_SYNC: // Wait for end of internal write process of the drive
        if (select())
            res = RES_OK;
        break;

    case GET_SECTOR_COUNT: // Get drive capacity in unit of sector (uint16_t)
        if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
            if ((csd[0] >> 6) == 1) { // SDC ver 2.00
                csize = csd[9] + ((uint16_t)csd[8] << 8) +
                        ((uint16_t)(csd[7] & 63) << 16) + 1;
                *(uint16_t*)buff = csize << 10;
            } else { // SDC ver 1.XX or MMC ver 3
                n = (csd[5] & 15) + ((csd[10] & 128) >> 7) +
                    ((csd[9] & 3) << 1) + 2;
                csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) +
                        ((uint16_t)(csd[6] & 3) << 10) + 1;
                *(uint16_t*)buff = csize << (n - 9);
            }
            res = RES_OK;
        }
        break;

    case GET_BLOCK_SIZE: // Get erase block size in unit of sector (uint16_t)
        if (CardType & CT_SD2) {            // SDC ver 2.00
            if (send_cmd(ACMD13, 0) == 0) { // Read SD status
                xchg_spi(0xFF);
                if (rcvr_datablock(csd, 16)) { // Read partial block
                    for (n = 64 - 16; n; n--)
                        xchg_spi(0xFF); // Purge trailing data
                    *(uint16_t*)buff = 16UL << (csd[10] >> 4);
                    res = RES_OK;
                }
            }
        } else { // SDC ver 1.XX or MMC
            if ((send_cmd(CMD9, 0) == 0) &&
                rcvr_datablock(csd, 16)) { // Read CSD
                if (CardType & CT_SD1) {   // SDC ver 1.XX
                    *(uint16_t*)buff = (((csd[10] & 63) << 1) +
                                        ((uint16_t)(csd[11] & 128) >> 7) + 1)
                                       << ((csd[13] >> 6) - 1);
                } else { // MMC
                    *(uint16_t*)buff =
                        ((uint16_t)((csd[10] & 124) >> 2) + 1) *
                        (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
                }
                res = RES_OK;
            }
        }
        break;

    case CTRL_TRIM: // Erase a block of sectors (used when _USE_ERASE == 1)
        if (!(CardType & CT_SDC))
            break; // Check if the card is SDC
        if (disk_ioctl(MMC_GET_CSD, csd))
            break; // Get CSD
        if (!(csd[0] >> 6) && !(csd[10] & 0x40))
            break; // Check if sector erase can be applied to the card
        dp = buff;
        st = dp[0];
        ed = dp[1]; // Load sector block
        if (!(CardType & CT_BLOCK)) {
            st *= 512;
            ed *= 512;
        }
        if (send_cmd(CMD32, st) == 0 && send_cmd(CMD33, ed) == 0 &&
            send_cmd(CMD38, 0) == 0 && wait_ready(30000)) // Erase sector block
            res = RES_OK; // FatFs does not check result of this command
        break;

    default: res = RES_PARERR;
    }

    deselect();

    return res;
}

// This function must be called from timer interrupt routine in period
// of 1 ms to generate card control timing.
// OS will schedule this periodic task
void disk_timerproc(void) {
    uint16_t n;
    uint8_t s;

    n = Timer1; // 1kHz decrement timer stopped at 0
    if (n)
        Timer1 = --n;
    n = Timer2;
    if (n)
        Timer2 = --n;

    s = Stat;
    // TODO: check write protection
    // if (MMC_WP) s |= STA_PROTECT;
    // else s &= ~STA_PROTECT;
    // TODO: check if card is in socket
    // if (MMC_CD) s &= ~STA_NODISK;
    // else s |= (STA_NODISK | STA_NOINIT);
    Stat = s;
}
