#include "tivaware/gpio.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/pin_map.h"
#include "tivaware/rom.h"
#include "tivaware/ssi.h"
#include "tivaware/sysctl.h"

// there is a #define below to select SDC CS as PD7 or PB0

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) connected to PA4 (SSI0Rx)
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss) <- GPIO high to disable TFT
// CARD_CS (pin 5) connected to PD7/PB0 GPIO output
// Data/Command (pin 4) connected to PA6 (GPIO)<- GPIO low not using TFT
// RESET (pin 3) connected to PA7 (GPIO)<- GPIO high to disable TFT
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

// **********wide.hk ST7735R*******************
// Silkscreen Label (SDC side up; LCD side down) - Connection
// VCC  - +3.3 V
// GND  - Ground
// !SCL - PA2 Sclk SPI clock from microcontroller to TFT or SDC
// !SDA - PA5 MOSI SPI data from microcontroller to TFT or SDC
// DC   - PA6 TFT data/command
// RES  - PA7 TFT reset
// CS   - PA3 TFT_CS, active low to enable TFT
// *CS  - PD7/PB0 SDC_CS, active low to enable SDC
// MISO - PA4 MISO SPI data from SDC to microcontroller
// SDA  – (NC) I2C data for ADXL345 accelerometer
// SCL  – (NC) I2C clock for ADXL345 accelerometer
// SDO  – (NC) I2C alternate address for ADXL345 accelerometer
// Backlight + - Light, backlight connected to +3.3 V
#include "eDisk.h"
#include <stdint.h>

// these defines are in two places, here and in ST7735.c
#define SDC_CS_PB0 1
#define SDC_CS_PD7 0
#define TFT_CS (*((volatile unsigned long*)0x40004020))
#define TFT_CS_LOW 0 // CS normally controlled by hardware
#define TFT_CS_HIGH 0x08

#if SDC_CS_PD7
// CS is PD7
// to change CS to another GPIO, change SDC_CS and CS_Init
#define SDC_CS (*((volatile unsigned long*)0x40007200))
#define SDC_CS_LOW 0 // CS controlled by software
#define SDC_CS_HIGH 0x80
void CS_Init(void) {
    SYSCTL_RCGCGPIO_R |= 0x08; // activate port D
    while ((SYSCTL_PRGPIO_R & 0x08) == 0) {};
    GPIO_PORTD_LOCK_R = 0x4C4F434B; // 2) unlock PortD PD7
    GPIO_PORTD_CR_R |= 0xFF;        // allow changes to PD7-0
    ROM_GPIOPadConfigSet(GPIO_PORTD_BASE, 0x80, GPIO_STRENGTH_4MA,
                         GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, 0x80);
    SDC_CS = SDC_CS_HIGH;
}
#endif
#if SDC_CS_PB0
// CS is PB0
// to change CS to another GPIO, change SDC_CS and CS_Init
#define SDC_CS (*((volatile unsigned long*)0x40005004))
#define SDC_CS_LOW 0 // CS controlled by software
#define SDC_CS_HIGH 0x01
void CS_Init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) {}
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, 0x1, GPIO_STRENGTH_4MA,
                         GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, 0x1);
    SDC_CS = SDC_CS_HIGH;
}
#endif

//********SSI0_Init*****************
// Initialize SSI0 interface to SDC
// inputs:  clock divider to set clock frequency
// outputs: none
// assumes: system clock rate is 80 MHz, SCR=0
// SSIClk = SysClk / (CPSDVSR * (1 + SCR)) = 80 MHz/CPSDVSR
// 200 for    400,000 bps slow mode, used during initialization
// 8   for 10,000,000 bps fast mode, used during disk I/O
// void Timer5_Init(void);
void SSI0_Init(unsigned long CPSDVSR) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    //  Timer5_Init(); // in this version the OS will schedule disk_timerproc
    CS_Init(); // initialize whichever GPIO pin is CS for the SD card
    // initialize Port A
    while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_SSI0)) {}
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, 0xC8); // make A3,A6,A7 out
    ROM_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    ROM_GPIOPinConfigure(GPIO_PA4_SSI0RX);
    ROM_GPIOPinConfigure(GPIO_PA5_SSI0TX);
    ROM_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_2);

    // PA7, PA3 high (disable LCD)
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 1);
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, 1);

    ROM_SSIDisable(SSI0_BASE);
    ROM_SSIConfigSetExpClk(SSI0_BASE, ROM_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                           SSI_MODE_MASTER, ROM_SysCtlClockGet() / CPSDVSR, 8);
    ROM_SSIEnable(SSI0_BASE);

    // read any residual data from ssi port
    uint32_t temp;
    while (ROM_SSIDataGetNonBlocking(SSI0_BASE, &temp)) {}
}

// void Timer5_Init(void){volatile unsigned short delay;
//  SYSCTL_RCGCTIMER_R |= 0x20;
//  delay = SYSCTL_SCGCTIMER_R;
//  delay = SYSCTL_SCGCTIMER_R;
//  TIMER5_CTL_R = 0x00000000;       // 1) disable timer5A during setup
//  TIMER5_CFG_R = 0x00000000;       // 2) configure for 32-bit mode
//  TIMER5_TAMR_R = 0x00000002;      // 3) configure for periodic mode, default
//  down-count settings TIMER5_TAILR_R = 799999;         // 4) reload value, 10
//  ms, 80 MHz clock TIMER5_TAPR_R = 0;               // 5) bus clock resolution
//  TIMER5_ICR_R = 0x00000001;       // 6) clear timer5A timeout flag
//  TIMER5_IMR_R = 0x00000001;       // 7) arm timeout interrupt
//  NVIC_PRI23_R = (NVIC_PRI23_R&0xFFFFFF00)|0x00000040; // 8) priority 2
//// interrupts enabled in the main program after all devices initialized
//// vector number 108, interrupt number 92
//  NVIC_EN2_R = 0x10000000;         // 9) enable interrupt 92 in NVIC
//  TIMER5_CTL_R = 0x00000001;       // 10) enable timer5A
//}
//// Executed every 10 ms
// void Timer5A_Handler(void){
//  TIMER5_ICR_R = 0x00000001;       // acknowledge timer5A timeout
//  disk_timerproc();
//}

// SSIClk = PIOSC / (CPSDVSR * (1 + SCR)) = 80 MHz/CPSDVSR
// 200 for   400,000 bps slow mode, used during initialization
// 8  for 10,000,000 bps fast mode, used during disk I/O
#define FCLK_SLOW()                                                            \
    { SSI0_CPSR_R = (SSI0_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 200; }
#define FCLK_FAST()                                                            \
    { SSI0_CPSR_R = (SSI0_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 8; }

// de-asserts the CS pin to the card
#define CS_HIGH() SDC_CS = SDC_CS_HIGH;
// asserts the CS pin to the card
#define CS_LOW() SDC_CS = SDC_CS_LOW;
//#define  MMC_CD    !(GPIOC_IDR & _BV(4))  /* Card detect (yes:true, no:false,
// default:true) */
#define MMC_CD 1 /* Card detect (yes:true, no:false, default:true) */
#define MMC_WP 0 /* Write protected (yes:true, no:false, default:false) */
//#define  SPIx_CR1  SPI1_CR1
//#define  SPIx_SR    SPI1_SR
//#define  SPIx_DR    SPI1_DR
#define SPIxENABLE()                                                           \
    { SSI0_Init(200); }

/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/
/* MMC/SD command */
#define CMD0 (0)           /* GO_IDLE_STATE */
#define CMD1 (1)           /* SEND_OP_COND (MMC) */
#define ACMD41 (0x80 + 41) /* SEND_OP_COND (SDC) */
#define CMD8 (8)           /* SEND_IF_COND */
#define CMD9 (9)           /* SEND_CSD */
#define CMD10 (10)         /* SEND_CID */
#define CMD12 (12)         /* STOP_TRANSMISSION */
#define ACMD13 (0x80 + 13) /* SD_STATUS (SDC) */
#define CMD16 (16)         /* SET_BLOCKLEN */
#define CMD17 (17)         /* READ_SINGLE_BLOCK */
#define CMD18 (18)         /* READ_MULTIPLE_BLOCK */
#define CMD23 (23)         /* SET_BLOCK_COUNT (MMC) */
#define ACMD23 (0x80 + 23) /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24 (24)         /* WRITE_BLOCK */
#define CMD25 (25)         /* WRITE_MULTIPLE_BLOCK */
#define CMD32 (32)         /* ERASE_ER_BLK_START */
#define CMD33 (33)         /* ERASE_ER_BLK_END */
#define CMD38 (38)         /* ERASE */
#define CMD55 (55)         /* APP_CMD */
#define CMD58 (58)         /* READ_OCR */

static volatile DSTATUS Stat = STA_NOINIT; /* Physical drive status */

static volatile uint32_t Timer1,
    Timer2; /* 1kHz decrement timer stopped at zero (disk_timerproc()) */

static uint8_t CardType; /* Card type flags */

/*-----------------------------------------------------------------------*/
/* SPI controls (Platform dependent)                                     */
/*-----------------------------------------------------------------------*/

/* Initialize MMC interface */
static void init_spi(void) {
    SPIxENABLE(); /* Enable SPI function */
    CS_HIGH();    /* Set CS# high */

    for (Timer1 = 10; Timer1;)
        ; /* 10ms */
}

/* Exchange a byte */
// Inputs:  byte to be sent to SPI
// Outputs: byte received from SPI
// assumes it has been selected with CS low
static uint8_t xchg_spi(uint8_t dat) {
    uint32_t rcvdat;
    // wait until SSI0 not busy/transmit FIFO empty
    while (ROM_SSIBusy(SSI0_BASE)) {}
    ROM_SSIDataPut(SSI0_BASE, dat);
    ROM_SSIDataGet(SSI0_BASE, &rcvdat);
    return rcvdat & 0xFF;
}

/*-----------------------------------------------------------------------*/
/* Receive a byte from MMC via SPI  (Platform dependent)                 */
/*-----------------------------------------------------------------------*/
// Inputs:  none
// Outputs: byte received from SPI
// assumes it has been selected with CS low
static uint8_t rcvr_spi(void) {
    // wait until SSI0 not busy/transmit FIFO empty
    while (ROM_SSIBusy(SSI0_BASE)) {}
    ROM_SSIDataPut(SSI0_BASE, 0xFF); // data out, garbage
    uint32_t response;
    ROM_SSIDataGet(SSI0_BASE, &response);
    return (uint8_t)(response & 0xFF);
}

/* Receive multiple byte */
// Input:  buff Pointer to empty buffer into which data will be received
//         btr  Number of bytes to receive (even number)
// Output: none
static void rcvr_spi_multi(uint8_t* buff, uint32_t btr) {
    while (btr) {
        *buff = rcvr_spi(); // return by reference
        btr--;
        buff++;
    }
}

#if _USE_WRITE
/* Send multiple byte */
// Input:  buff Pointer to the data which will be sent
//         btx  Number of bytes to send (even number)
// Output: none
static void xmit_spi_multi(const uint8_t* buff, uint32_t btx) {
    uint32_t rcvdat;
    while (btx) {
        ROM_SSIDataPut(SSI0_BASE, *buff);   // data out
        ROM_SSIDataGet(SSI0_BASE, &rcvdat); // get response
        btx--;
        buff++;
    }
}
#endif

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/
// Input:  time to wait in ms
// Output: 1:Ready, 0:Timeout
static int wait_ready(uint32_t wt) {
    uint8_t d;
    Timer2 = wt;
    do {
        d = xchg_spi(0xFF);
        /* This loop takes a time. Insert rot_rdq() here for multitask
         * environment. */
    } while (d != 0xFF && Timer2); /* Wait for card goes ready or timeout */
    return (d == 0xFF) ? 1 : 0;
}

/*-----------------------------------------------------------------------*/
/* Deselect card and release SPI                                         */
/*-----------------------------------------------------------------------*/
static void deselect(void) {
    CS_HIGH();      /* CS = H */
    xchg_spi(0xFF); /* Dummy clock (force DO hi-z for multiple slave SPI) */
}

/*-----------------------------------------------------------------------*/
/* Select card and wait for ready                                        */
/*-----------------------------------------------------------------------*/
// Input:  none
// Output: 1:OK, 0:Timeout in 500ms
static int select(void) {
    TFT_CS = TFT_CS_HIGH; // make sure TFT is off
    CS_LOW();
    xchg_spi(0xFF); /* Dummy clock (force DO enabled) */
    if (wait_ready(500))
        return 1; /* OK */
    deselect();
    return 0; /* Timeout */
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from the MMC                                    */
/*-----------------------------------------------------------------------*/
// Input:  buff Pointer to empty buffer into which data will be received
//         btr  Number of bytes to receive (even number)
// Output: 1:OK, 0:Error on timeout
static int rcvr_datablock(uint8_t* buff, uint32_t btr) {
    uint8_t token;
    Timer1 = 200;
    do { /* Wait for DataStart token in timeout of 200ms */
        token = xchg_spi(0xFF);
        /* This loop will take a time. Insert rot_rdq() here for multitask
         * envilonment. */
    } while ((token == 0xFF) && Timer1);
    if (token != 0xFE)
        return 0; /* Function fails if invalid DataStart token or timeout */

    rcvr_spi_multi(buff, btr); /* Store trailing data to the buffer */
    xchg_spi(0xFF);
    xchg_spi(0xFF); /* Discard CRC */
    return 1;       /* Function succeeded */
}

/*-----------------------------------------------------------------------*/
/* Send a data packet to the MMC                                         */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
// Input:  buff Pointer to 512 byte data which will be sent
//         token  Token
// Output: 1:OK, 0:Failed on timeout
static int xmit_datablock(const uint8_t* buff, uint8_t token) {
    uint8_t resp;
    if (!wait_ready(500))
        return 0; /* Wait for card ready */

    xchg_spi(token);     /* Send token */
    if (token != 0xFD) { /* Send data if token is other than StopTran */
        xmit_spi_multi(buff, 512); /* Data */
        xchg_spi(0xFF);
        xchg_spi(0xFF); /* Dummy CRC */

        resp = xchg_spi(0xFF); /* Receive data resp */
        if ((resp & 0x1F) !=
            0x05) /* Function fails if the data packet was not accepted */
            return 0;
    }
    return 1;
}
#endif

/*-----------------------------------------------------------------------*/
/* Send a command packet to the MMC                                      */
/*-----------------------------------------------------------------------*/
// Inputs:  cmd Command index
//          arg    /* Argument
// Outputs: R1 resp (bit7==1:Failed to send)
static uint8_t send_cmd(uint8_t cmd, uint32_t arg) {
    uint8_t n, res;
    if (cmd & 0x80) { /* Send a CMD55 prior to ACMD<n> */
        cmd &= 0x7F;
        res = send_cmd(CMD55, 0);
        if (res > 1)
            return res;
    }

    /* Select the card and wait for ready except to stop multiple block read */
    if (cmd != CMD12) {
        deselect();
        if (!select())
            return 0xFF;
    }

    /* Send command packet */
    xchg_spi(0x40 | cmd);           /* Start + command index */
    xchg_spi((uint8_t)(arg >> 24)); /* Argument[31..24] */
    xchg_spi((uint8_t)(arg >> 16)); /* Argument[23..16] */
    xchg_spi((uint8_t)(arg >> 8));  /* Argument[15..8] */
    xchg_spi((uint8_t)arg);         /* Argument[7..0] */
    n = 0x01;                       /* Dummy CRC + Stop */
    if (cmd == CMD0)
        n = 0x95; /* Valid CRC for CMD0(0) */
    if (cmd == CMD8)
        n = 0x87; /* Valid CRC for CMD8(0x1AA) */
    xchg_spi(n);

    /* Receive command resp */
    if (cmd == CMD12)
        xchg_spi(0xFF); /* Diacard following one byte when CMD12 */
    n = 10;             /* Wait for response (10 bytes max) */
    do
        res = xchg_spi(0xFF);
    while ((res & 0x80) && --n);

    return res; /* Return received response */
}

/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* Initialize disk drive                                                 */
/*-----------------------------------------------------------------------*/
// Inputs:  Physical drive number, which must be 0
// Outputs: status (see DSTATUS)
DSTATUS eDisk_Init(uint8_t drv) {
    uint8_t n, cmd, ty, ocr[4];

    if (drv)
        return STA_NOINIT; /* Supports only drive 0 */
    init_spi();            /* Initialize SPI */

    if (Stat & STA_NODISK)
        return Stat; /* Is card existing in the soket? */

    FCLK_SLOW();
    for (n = 10; n; n--) xchg_spi(0xFF); /* Send 80 dummy clocks */

    ty = 0;
    if (send_cmd(CMD0, 0) == 1) {         /* Put the card SPI/Idle state */
        Timer1 = 1000;                    /* Initialization timeout = 1 sec */
        if (send_cmd(CMD8, 0x1AA) == 1) { /* SDv2? */
            for (n = 0; n < 4; n++)
                ocr[n] =
                    xchg_spi(0xFF); /* Get 32 bit return value of R7 resp */
            if (ocr[2] == 0x01 &&
                ocr[3] == 0xAA) { /* Is the card supports vcc of 2.7-3.6V? */
                while (Timer1 && send_cmd(ACMD41, 1 << 30))
                    ; /* Wait for end of initialization with ACMD41(HCS) */
                if (Timer1 &&
                    send_cmd(CMD58, 0) == 0) { /* Check CCS bit in the OCR */
                    for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);
                    ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK
                                         : CT_SD2; /* Card id SDv2 */
                }
            }
        } else {                            /* Not SDv2 card */
            if (send_cmd(ACMD41, 0) <= 1) { /* SDv1 or MMC? */
                ty = CT_SD1;
                cmd = ACMD41; /* SDv1 (ACMD41(0)) */
            } else {
                ty = CT_MMC;
                cmd = CMD1; /* MMCv3 (CMD1(0)) */
            }
            while (Timer1 && send_cmd(cmd, 0))
                ; /* Wait for end of initialization */
            if (!Timer1 ||
                send_cmd(CMD16, 512) != 0) /* Set block length: 512 */
                ty = 0;
        }
    }
    CardType = ty; /* Card type */
    deselect();

    if (ty) {                /* OK */
        FCLK_FAST();         /* Set fast clock */
        Stat &= ~STA_NOINIT; /* Clear STA_NOINIT flag */
    } else {                 /* Failed */
        Stat = STA_NOINIT;
    }

    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Get disk status                                                       */
/*-----------------------------------------------------------------------*/
// Inputs:  Physical drive number, which must be 0
// Outputs: status (see DSTATUS)
DSTATUS eDisk_Status(uint8_t drv) {
    if (drv)
        return STA_NOINIT; /* Supports only drive 0 */
    return Stat;           /* Return disk status */
}

/*-----------------------------------------------------------------------*/
/* Read sector(s)                                                        */
/*-----------------------------------------------------------------------*/
// Inputs:  drv    Physical drive number (0)
//         buff   Pointer to the data buffer to store read data
//         sector Start sector number (LBA)
//         count  Number of sectors to read (1..128)
// Outputs: status (see DRESULT)
DRESULT eDisk_Read(uint8_t drv, uint8_t* buff, uint16_t sector, uint8_t count) {
    if (drv || !count)
        return RES_PARERR; /* Check parameter */
    if (Stat & STA_NOINIT)
        return RES_NOTRDY; /* Check if drive is ready */

    if (!(CardType & CT_BLOCK))
        sector *= 512; /* LBA ot BA conversion (byte addressing cards) */

    if (count == 1) {                      /* Single sector read */
        if ((send_cmd(CMD17, sector) == 0) /* READ_SINGLE_BLOCK */
            && rcvr_datablock(buff, 512))
            count = 0;
    } else {                                /* Multiple sector read */
        if (send_cmd(CMD18, sector) == 0) { /* READ_MULTIPLE_BLOCK */
            do {
                if (!rcvr_datablock(buff, 512))
                    break;
                buff += 512;
            } while (--count);
            send_cmd(CMD12, 0); /* STOP_TRANSMISSION */
        }
    }
    deselect();

    return count ? RES_ERROR : RES_OK; /* Return result */
}

//*************** eDisk_ReadBlock ***********
// Read 1 block of 512 bytes from the SD card  (write to RAM)
// Inputs: pointer to an empty RAM buffer
//         sector number of SD card to read: 0,1,2,...
// Outputs: result
//  RES_OK        0: Successful
//  RES_ERROR     1: R/W Error
//  RES_WRPRT     2: Write Protected
//  RES_NOTRDY    3: Not Ready
//  RES_PARERR    4: Invalid Parameter
DRESULT
eDisk_ReadBlock(
    uint8_t* buff,     /* Pointer to the data buffer to store read data */
    uint16_t sector) { /* Start sector number (LBA) */
    return eDisk_Read(0, buff, sector, 1);
}

/*-----------------------------------------------------------------------*/
/* Write sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
// Inputs:  drv    Physical drive number (0)
//         buff   Pointer to the data buffer to write to disk
//         sector Start sector number (LBA)
//         count  Number of sectors to write (1..128)
// Outputs: status (see DRESULT)
DRESULT eDisk_Write(uint8_t drv, const uint8_t* buff, uint16_t sector,
                    uint8_t count) {
    if (drv || !count)
        return RES_PARERR; /* Check parameter */
    if (Stat & STA_NOINIT)
        return RES_NOTRDY; /* Check drive status */
    if (Stat & STA_PROTECT)
        return RES_WRPRT; /* Check write protect */

    if (!(CardType & CT_BLOCK))
        sector *= 512; /* LBA ==> BA conversion (byte addressing cards) */

    if (count == 1) {                      /* Single sector write */
        if ((send_cmd(CMD24, sector) == 0) /* WRITE_BLOCK */
            && xmit_datablock(buff, 0xFE))
            count = 0;
    } else { /* Multiple sector write */
        if (CardType & CT_SDC)
            send_cmd(ACMD23, count);        /* Predefine number of sectors */
        if (send_cmd(CMD25, sector) == 0) { /* WRITE_MULTIPLE_BLOCK */
            do {
                if (!xmit_datablock(buff, 0xFC))
                    break;
                buff += 512;
            } while (--count);
            if (!xmit_datablock(0, 0xFD)) /* STOP_TRAN token */
                count = 1;
        }
    }
    deselect();

    return count ? RES_ERROR : RES_OK; /* Return result */
}
//*************** eDisk_WriteBlock ***********
// Write 1 block of 512 bytes of data to the SD card
// Inputs: pointer to RAM buffer with information
//         sector number of SD card to write: 0,1,2,...
// Outputs: result
//  RES_OK        0: Successful
//  RES_ERROR     1: R/W Error
//  RES_WRPRT     2: Write Protected
//  RES_NOTRDY    3: Not Ready
//  RES_PARERR    4: Invalid Parameter
DRESULT
eDisk_WriteBlock(const uint8_t* buff, /* Pointer to the data to be written */
                 uint16_t sector) {   /* Start sector number (LBA) */
    return eDisk_Write(0, buff, sector, 1); // 1 block
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous drive controls other than data read/write               */
/*-----------------------------------------------------------------------*/
// Inputs:  drv,   Physical drive number (0)
//          cmd,   Control command code
//          buff   Pointer to the control data
// Outputs: status (see DRESULT)
#if _USE_IOCTL
DRESULT disk_ioctl(uint8_t drv, uint8_t cmd, void* buff) {
    DRESULT res;
    uint8_t n, csd[16];
    uint16_t *dp, st, ed, csize;

    if (drv)
        return RES_PARERR; /* Check parameter */
    if (Stat & STA_NOINIT)
        return RES_NOTRDY; /* Check if drive is ready */

    res = RES_ERROR;

    switch (cmd) {
    case CTRL_SYNC: /* Wait for end of internal write process of the drive */
        if (select())
            res = RES_OK;
        break;

    case GET_SECTOR_COUNT: /* Get drive capacity in unit of sector (uint16_t) */
        if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
            if ((csd[0] >> 6) == 1) { /* SDC ver 2.00 */
                csize = csd[9] + ((WORD)csd[8] << 8) +
                        ((uint16_t)(csd[7] & 63) << 16) + 1;
                *(uint16_t*)buff = csize << 10;
            } else { /* SDC ver 1.XX or MMC ver 3 */
                n = (csd[5] & 15) + ((csd[10] & 128) >> 7) +
                    ((csd[9] & 3) << 1) + 2;
                csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) +
                        ((WORD)(csd[6] & 3) << 10) + 1;
                *(uint16_t*)buff = csize << (n - 9);
            }
            res = RES_OK;
        }
        break;

    case GET_BLOCK_SIZE: /* Get erase block size in unit of sector (uint16_t) */
        if (CardType & CT_SD2) {            /* SDC ver 2.00 */
            if (send_cmd(ACMD13, 0) == 0) { /* Read SD status */
                xchg_spi(0xFF);
                if (rcvr_datablock(csd, 16)) { /* Read partial block */
                    for (n = 64 - 16; n; n--)
                        xchg_spi(0xFF); /* Purge trailing data */
                    *(uint16_t*)buff = 16UL << (csd[10] >> 4);
                    res = RES_OK;
                }
            }
        } else { /* SDC ver 1.XX or MMC */
            if ((send_cmd(CMD9, 0) == 0) &&
                rcvr_datablock(csd, 16)) { /* Read CSD */
                if (CardType & CT_SD1) {   /* SDC ver 1.XX */
                    *(uint16_t*)buff = (((csd[10] & 63) << 1) +
                                        ((WORD)(csd[11] & 128) >> 7) + 1)
                                       << ((csd[13] >> 6) - 1);
                } else { /* MMC */
                    *(uint16_t*)buff =
                        ((WORD)((csd[10] & 124) >> 2) + 1) *
                        (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
                }
                res = RES_OK;
            }
        }
        break;

    case CTRL_TRIM: /* Erase a block of sectors (used when _USE_ERASE == 1) */
        if (!(CardType & CT_SDC))
            break; /* Check if the card is SDC */
        if (disk_ioctl(drv, MMC_GET_CSD, csd))
            break; /* Get CSD */
        if (!(csd[0] >> 6) && !(csd[10] & 0x40))
            break; /* Check if sector erase can be applied to the card */
        dp = buff;
        st = dp[0];
        ed = dp[1]; /* Load sector block */
        if (!(CardType & CT_BLOCK)) {
            st *= 512;
            ed *= 512;
        }
        if (send_cmd(CMD32, st) == 0 && send_cmd(CMD33, ed) == 0 &&
            send_cmd(CMD38, 0) == 0 &&
            wait_ready(30000)) /* Erase sector block */
            res = RES_OK;      /* FatFs does not check result of this command */
        break;

    default: res = RES_PARERR;
    }

    deselect();

    return res;
}
#endif

/*-----------------------------------------------------------------------*/
/* Device timer function                                                 */
/*-----------------------------------------------------------------------*/
/* This function must be called from timer interrupt routine in period
/  of 1 ms to generate card control timing.
/  OS will schedule this periodic task
*/

void disk_timerproc(void) {
    uint16_t n;
    uint8_t s;

    n = Timer1; /* 1kHz decrement timer stopped at 0 */
    if (n)
        Timer1 = --n;
    n = Timer2;
    if (n)
        Timer2 = --n;

    s = Stat;
    if (MMC_WP) /* Write protected */
        s |= STA_PROTECT;
    else /* Write enabled */
        s &= ~STA_PROTECT;
    if (MMC_CD) /* Card is in socket */
        s &= ~STA_NODISK;
    else /* Socket empty */
        s |= (STA_NODISK | STA_NOINIT);
    Stat = s;
}
