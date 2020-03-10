#include "ST7735.h"
#include "OS.h"
#include "eDisk.h"
#include "font.h"
#include "timer.h"
#include "tivaware/rom.h"
#include <stdint.h>

// these defines are in two places, here and in eDisk.c
#define SDC_CS_PB0 1
#define SDC_CS_PD7 0
#if SDC_CS_PD7
// CS is PD7
// to change CS to another GPIO, change SDC_CS and CS_Init
#define SDC_CS (*((volatile uint32_t*)0x40007200))
#define SDC_CS_LOW 0 // CS controlled by software
#define SDC_CS_HIGH 0x80

#endif
#if SDC_CS_PB0
// CS is PB0
// to change CS to another GPIO, change SDC_CS and CS_Init
#define SDC_CS (*((volatile uint32_t*)0x40005004))
#define SDC_CS_LOW 0 // CS controlled by software
#define SDC_CS_HIGH 0x01
#endif

// 16 rows (0 to 15) and 21 characters (0 to 20)
// Requires (11 + size*size*6*8) bytes of transmission for each character
uint32_t StX = 0; // position along the horizonal axis 0 to 20
uint32_t StY = 0; // position along the vertical axis 0 to 15
uint16_t StTextColor = ST7735_YELLOW;

#define ST7735_NOP 0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID 0x04
#define ST7735_RDDST 0x09

#define ST7735_SLPIN 0x10
#define ST7735_SLPOUT 0x11
#define ST7735_PTLON 0x12
#define ST7735_NORON 0x13

#define ST7735_INVOFF 0x20
#define ST7735_INVON 0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON 0x29
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
#define ST7735_RAMRD 0x2E

#define ST7735_PTLAR 0x30
#define ST7735_COLMOD 0x3A
#define ST7735_MADCTL 0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5

#define ST7735_RDID1 0xDA
#define ST7735_RDID2 0xDB
#define ST7735_RDID3 0xDC
#define ST7735_RDID4 0xDD

#define ST7735_PWCTR6 0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

#define TFT_CS (*((volatile uint32_t*)0x40004020))
#define TFT_CS_LOW 0 // CS normally controlled by hardware
#define TFT_CS_HIGH 0x08
#define DC (*((volatile uint32_t*)0x40004100))
#define DC_COMMAND 0
#define DC_DATA 0x40
#define RESET (*((volatile uint32_t*)0x40004200))
#define RESET_LOW 0
#define RESET_HIGH 0x80

#define SSI_CR0_SCR_M 0x0000FF00    // SSI Serial Clock Rate
#define SSI_CR0_SPH 0x00000080      // SSI Serial Clock Phase
#define SSI_CR0_SPO 0x00000040      // SSI Serial Clock Polarity
#define SSI_CR0_FRF_M 0x00000030    // SSI Frame Format Select
#define SSI_CR0_FRF_MOTO 0x00000000 // Freescale SPI Frame Format
#define SSI_CR0_DSS_M 0x0000000F    // SSI Data Size Select
#define SSI_CR0_DSS_8 0x00000007    // 8-bit data
#define SSI_CR1_MS 0x00000004       // SSI Master/Slave Select
#define SSI_CR1_SSE                                                            \
    0x00000002                        // SSI Synchronous Serial Port
                                      // Enable
#define SSI_SR_BSY 0x00000010         // SSI Busy Bit
#define SSI_SR_TNF 0x00000002         // SSI Transmit FIFO Not Full
#define SSI_CPSR_CPSDVSR_M 0x000000FF // SSI Clock Prescale Divisor
#define SSI_CC_CS_M 0x0000000F        // SSI Baud Clock Source
#define SSI_CC_CS_SYSPLL                                                       \
    0x00000000                        // Either the system clock (if the
                                      // PLL bypass is in effect) or the
                                      // PLL output (default)
#define SYSCTL_RCGC2_GPIOA 0x00000001 // port A Clock Gating Control
#define ST7735_TFTWIDTH 128
#define ST7735_TFTHEIGHT 160

#define ST7735_NOP 0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID 0x04
#define ST7735_RDDST 0x09

#define ST7735_SLPIN 0x10
#define ST7735_SLPOUT 0x11
#define ST7735_PTLON 0x12
#define ST7735_NORON 0x13

#define ST7735_INVOFF 0x20
#define ST7735_INVON 0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON 0x29
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
#define ST7735_RAMRD 0x2E

#define ST7735_PTLAR 0x30
#define ST7735_COLMOD 0x3A
#define ST7735_MADCTL 0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5

#define ST7735_RDID1 0xDA
#define ST7735_RDID2 0xDB
#define ST7735_RDID3 0xDC
#define ST7735_RDID4 0xDD

#define ST7735_PWCTR6 0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

static uint8_t ColStart, RowStart; // some displays need this changed
static uint8_t Rotation;           // 0 to 3
static enum initRFlags TabColor;
static int16_t _width =
    ST7735_TFTWIDTH; // this could probably be a constant, except it is used in
                     // Adafruit_GFX and depends on image rotation
static int16_t _height = ST7735_TFTHEIGHT;
Sema4 LCDFree; // used for mutual exclusion

// The Data/Command pin must be valid when the eighth bit is
// sent.  The SSI module has hardware input and output FIFOs
// that are 8 locations deep.  Based on the observation that
// the LCD interface tends to send a few commands and then a
// lot of data, the FIFOs are not used when writing
// commands, and they are used when writing data.  This
// ensures that the Data/Command pin status matches the byte
// that is actually being transmitted.
// The write command operation waits until all data has been
// sent, configures the Data/Command pin for commands, sends
// the command, and then waits for the transmission to
// finish.
// The write data operation waits until there is room in the
// transmit FIFO, configures the Data/Command pin for data,
// and then adds the data to the transmit FIFO.
// NOTE: These functions will crash or stall indefinitely if
// the SSI0 module is not initialized and enabled.
void static writecommand(uint8_t c) {
    uint32_t response;
    // wait until SSI0 not busy/transmit FIFO empty
    while (ROM_SSIBusy(SSI0_BASE)) {}
    SDC_CS = SDC_CS_HIGH;
    TFT_CS = TFT_CS_LOW;
    DC = DC_COMMAND;
    ROM_SSIDataPut(SSI0_BASE, c);
    ROM_SSIDataGet(SSI0_BASE, &response);
    TFT_CS = TFT_CS_HIGH;
}

void static writedata(uint8_t c) {
    uint32_t response;
    // wait until SSI0 not busy/transmit FIFO empty
    while (ROM_SSIBusy(SSI0_BASE)) {}
    SDC_CS = SDC_CS_HIGH;
    TFT_CS = TFT_CS_LOW;
    DC = DC_DATA;
    ROM_SSIDataPut(SSI0_BASE, c);
    ROM_SSIDataGet(SSI0_BASE, &response);
    TFT_CS = TFT_CS_HIGH;
}

// Rather than a bazillion writecommand() and writedata() calls, screen
// initialization commands and arguments are organized in these tables
// stored in ROM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.
#define DELAY 0x80
static const uint8_t Rcmd1[] = { // Init for 7735R, part 1 (red or green tab)
    15,                          // 15 commands in list:
    ST7735_SWRESET,
    DELAY, //  1: Software reset, 0 args, w/delay
    150,   //     150 ms delay
    ST7735_SLPOUT,
    DELAY, //  2: Out of sleep mode, 0 args, w/delay
    255,   //     500 ms delay
    ST7735_FRMCTR1,
    3, //  3: Frame rate ctrl - normal mode, 3 args:
    0x01,
    0x2C,
    0x2D, //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2,
    3, //  4: Frame rate control - idle mode, 3 args:
    0x01,
    0x2C,
    0x2D, //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3,
    6, //  5: Frame rate ctrl - partial mode, 6 args:
    0x01,
    0x2C,
    0x2D, //     Dot inversion mode
    0x01,
    0x2C,
    0x2D, //     Line inversion mode
    ST7735_INVCTR,
    1,    //  6: Display inversion ctrl, 1 arg, no delay:
    0x07, //     No inversion
    ST7735_PWCTR1,
    3, //  7: Power control, 3 args, no delay:
    0xA2,
    0x02, //     -4.6V
    0x84, //     AUTO mode
    ST7735_PWCTR2,
    1,    //  8: Power control, 1 arg, no delay:
    0xC5, //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3,
    2,    //  9: Power control, 2 args, no delay:
    0x0A, //     Opamp current small
    0x00, //     Boost frequency
    ST7735_PWCTR4,
    2,    // 10: Power control, 2 args, no delay:
    0x8A, //     BCLK/2, Opamp current small & Medium low
    0x2A,
    ST7735_PWCTR5,
    2, // 11: Power control, 2 args, no delay:
    0x8A,
    0xEE,
    ST7735_VMCTR1,
    1, // 12: Power control, 1 arg, no delay:
    0x0E,
    ST7735_INVOFF,
    0, // 13: Don't invert display, no args, no delay
    ST7735_MADCTL,
    1,    // 14: Memory access control (directions), 1 arg:
    0xC8, //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD,
    1,                              // 15: set color mode, 1 arg, no delay:
    0x05};                          //     16-bit color
static const uint8_t Rcmd2red[] = { // Init for 7735R, part 2 (red tab only)
    2,                              //  2 commands in list:
    ST7735_CASET,
    4, //  1: Column addr set, 4 args, no delay:
    0x00,
    0x00, //     XSTART = 0
    0x00,
    0x7F, //     XEND = 127
    ST7735_RASET,
    4, //  2: Row addr set, 4 args, no delay:
    0x00,
    0x00, //     XSTART = 0
    0x00,
    0x9F};                       //     XEND = 159
static const uint8_t Rcmd3[] = { // Init for 7735R, part 3 (red or green tab)
    4,                           //  4 commands in list:
    ST7735_GMCTRP1,
    16, //  1: Magical unicorn dust, 16 args, no delay:
    0x02,
    0x1c,
    0x07,
    0x12,
    0x37,
    0x32,
    0x29,
    0x2d,
    0x29,
    0x25,
    0x2B,
    0x39,
    0x00,
    0x01,
    0x03,
    0x10,
    ST7735_GMCTRN1,
    16, //  2: Sparkles and rainbows, 16 args, no delay:
    0x03,
    0x1d,
    0x07,
    0x06,
    0x2E,
    0x2C,
    0x29,
    0x2D,
    0x2E,
    0x2E,
    0x37,
    0x3F,
    0x00,
    0x00,
    0x02,
    0x10,
    ST7735_NORON,
    DELAY, //  3: Normal display on, no args, w/delay
    10,    //     10 ms delay
    ST7735_DISPON,
    DELAY, //  4: Main screen turn on, no args w/delay
    100};  //     100 ms delay

// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in ROM byte array.
void static commandList(const uint8_t* addr) {
    uint8_t numCommands, numArgs;
    uint16_t wait;

    numCommands = *(addr++);      // Number of commands to follow
    while (numCommands--) {       // For each command...
        writecommand(*(addr++));  //   Read, issue command
        numArgs = *(addr++);      //   Number of args to follow
        wait = numArgs & DELAY;   //   If hibit set, delay follows args
        numArgs &= ~DELAY;        //   Mask out delay bit
        while (numArgs--) {       //   For each argument...
            writedata(*(addr++)); //     Read, issue argument
        }

        if (wait) {
            wait = *(addr++); // Read post-command delay time (ms)
            if (wait == 255)
                wait = 500; // If 255, delay for 500 ms
            busy_wait(3, ms(wait));
        }
    }
}

// Initialization code common to both 'B' and 'R' type displays
void static commonInit(const uint8_t* cmdList) {
    ColStart = RowStart = 0; // May be overridden in init func

    TFT_CS = TFT_CS_LOW;
    RESET = RESET_HIGH;
    busy_wait(3, 500);
    RESET = RESET_LOW;
    busy_wait(3, 500);
    RESET = RESET_HIGH;
    busy_wait(3, 500);

    SSI0_Init(10);

    if (cmdList)
        commandList(cmdList);
}

//------------ST7735_InitR------------
// Initialization for ST7735R screens (red tab)
// Input: option one of the enumerated options depending on tabs
// Output: none
void ST7735_InitR(enum initRFlags option) {
    commonInit(Rcmd1);
    commandList(Rcmd2red);
    commandList(Rcmd3);

    // if black, change MADCTL color filter
    if (option == INITR_BLACKTAB) {
        writecommand(ST7735_MADCTL);
        writedata(0xC0);
    }
    TabColor = option;
    ST7735_SetCursor(0, 0);
    StTextColor = ST7735_YELLOW;
    ST7735_FillScreen(0);          // set screen to black
    OS_InitSemaphore(&LCDFree, 0); // means LCD free
}

// Set the region of the screen RAM to be modified
// Pixel colors are sent left to right, top to bottom
// (same as Font table is encoded; different from regular bitmap)
// Requires 11 bytes of transmission
void static setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    writecommand(ST7735_CASET); // Column addr set
    writedata(0x00);
    writedata(x0 + ColStart); // XSTART
    writedata(0x00);
    writedata(x1 + ColStart); // XEND

    writecommand(ST7735_RASET); // Row addr set
    writedata(0x00);
    writedata(y0 + RowStart); // YSTART
    writedata(0x00);
    writedata(y1 + RowStart); // YEND

    writecommand(ST7735_RAMWR); // write to RAM
}

// Send two bytes of data, most significant byte first
// Requires 2 bytes of transmission
void static pushColor(uint16_t color) {
    writedata((uint8_t)(color >> 8));
    writedata((uint8_t)color);
}

void ST7735_DrawPixel(int16_t x, int16_t y, uint16_t color) {
    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
        return;
    setAddrWindow(x, y, x, y);
    pushColor(color);
}

void ST7735_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    uint8_t hi = color >> 8, lo = color;

    // Rudimentary clipping
    if ((x >= _width) || (y >= _height))
        return;
    if ((y + h - 1) >= _height)
        h = _height - y;
    setAddrWindow(x, y, x, y + h - 1);

    while (h--) {
        writedata(hi);
        writedata(lo);
    }

    // deselect();
}

void ST7735_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    uint8_t hi = color >> 8, lo = color;

    // Rudimentary clipping
    if ((x >= _width) || (y >= _height))
        return;
    if ((x + w - 1) >= _width)
        w = _width - x;
    setAddrWindow(x, y, x + w - 1, y);

    while (w--) {
        writedata(hi);
        writedata(lo);
    }
}

void ST7735_FillScreen(uint16_t color) {
    ST7735_FillRect(0, 0, _width, _height, color); // original
    //  screen is actually 129 by 161 pixels, x 0 to 128, y goes from 0 to 160
}

void ST7735_FillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t color) {
    uint8_t hi = color >> 8, lo = color;

    // rudimentary clipping (drawChar w/big text requires this)
    if ((x >= _width) || (y >= _height))
        return;
    if ((x + w - 1) >= _width)
        w = _width - x;
    if ((y + h - 1) >= _height)
        h = _height - y;

    setAddrWindow(x, y, x + w - 1, y + h - 1);

    for (y = h; y > 0; y--) {
        for (x = w; x > 0; x--) {
            writedata(hi);
            writedata(lo);
        }
    }
}

int16_t const smallCircle[6][3] = {{2, 3, 2}, {1, 4, 4}, {0, 5, 6},
                                   {0, 5, 6}, {1, 4, 4}, {2, 3, 2}};
void ST7735_DrawSmallCircle(int16_t x, int16_t y, uint16_t color) {
    uint32_t i, w;
    uint8_t hi = color >> 8, lo = color;
    // rudimentary clipping
    if ((x > _width - 5) || (y > _height - 5))
        return; // doesn't fit
    for (i = 0; i < 6; i++) {
        setAddrWindow(x + smallCircle[i][0], y + i, x + smallCircle[i][1],
                      y + i);
        w = smallCircle[i][2];
        while (w--) {
            writedata(hi);
            writedata(lo);
        }
    }
}

int16_t const circle[10][3] = {{4, 5, 2},  {2, 7, 6},  {1, 8, 8}, {1, 8, 8},
                               {0, 9, 10}, {0, 9, 10}, {1, 8, 8}, {1, 8, 8},
                               {2, 7, 6},  {4, 5, 2}};
void ST7735_DrawCircle(int16_t x, int16_t y, uint16_t color) {
    uint32_t i, w;
    uint8_t hi = color >> 8, lo = color;
    // rudimentary clipping
    if ((x > _width - 9) || (y > _height - 9))
        return; // doesn't fit
    for (i = 0; i < 10; i++) {
        setAddrWindow(x + circle[i][0], y + i, x + circle[i][1], y + i);
        w = circle[i][2];
        while (w--) {
            writedata(hi);
            writedata(lo);
        }
    }
}

//------------ST7735_Color565------------
// Pass 8-bit (each) R,G,B and get back 16-bit packed color.
// Input: r red value
//        g green value
//        b blue value
// Output: 16-bit color
uint16_t ST7735_Color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3);
}

//------------ST7735_SwapColor------------
// Swaps the red and blue values of the given 16-bit packed color;
// green is unchanged.
// Input: x 16-bit color in format B, G, R
// Output: 16-bit color in format R, G, B
uint16_t ST7735_SwapColor(uint16_t x) {
    return (x << 11) | (x & 0x07E0) | (x >> 11);
}

void ST7735_DrawBitmap(int16_t x, int16_t y, const uint16_t* image, int16_t w,
                       int16_t h) {
    int16_t skipC = 0; // non-zero if columns need to be skipped due to clipping
    int16_t originalWidth =
        w; // save this value; even if not all columns fit on the screen, the
           // image is still this width in ROM
    int i = w * (h - 1);

    if ((x >= _width) || ((y - h + 1) >= _height) || ((x + w) <= 0) ||
        (y < 0)) {
        return; // image is totally off the screen, do nothing
    }
    if ((w > _width) ||
        (h > _height)) { // image is too wide for the screen, do nothing
        //***This isn't necessarily a fatal error, but it makes the
        // following logic much more complicated, since you can have
        // an image that exceeds multiple boundaries and needs to be
        // clipped on more than one side.
        return;
    }
    if ((x + w - 1) >= _width) {  // image exceeds right of screen
        skipC = (x + w) - _width; // skip cut off columns
        w = _width - x;
    }
    if ((y - h + 1) < 0) {                   // image exceeds top of screen
        i = i - (h - y - 1) * originalWidth; // skip the last cut off rows
        h = y + 1;
    }
    if (x < 0) { // image exceeds left of screen
        w = w + x;
        skipC = -1 * x; // skip cut off columns
        i = i - x;      // skip the first cut off columns
        x = 0;
    }
    if (y >= _height) { // image exceeds bottom of screen
        h = h - (y - _height + 1);
        y = _height - 1;
    }

    setAddrWindow(x, y - h + 1, x + w - 1, y);

    for (y = 0; y < h; y = y + 1) {
        for (x = 0; x < w; x = x + 1) {
            // send the top 8 bits
            writedata((uint8_t)(image[i] >> 8));
            // send the bottom 8 bits
            writedata((uint8_t)image[i]);
            i = i + 1; // go to the next pixel
        }
        i = i + skipC;
        i = i - 2 * originalWidth;
    }
}

void ST7735_DrawCharS(int16_t x, int16_t y, char c, int16_t textColor,
                      int16_t bgColor, uint8_t size) {
    uint8_t line; // vertical column of pixels of character in font
    int32_t i, j;
    if ((x >= _width) ||            // Clip right
        (y >= _height) ||           // Clip bottom
        ((x + 6 * size - 1) < 0) || // Clip left
        ((y + 8 * size - 1) < 0))   // Clip top
        return;

    for (i = 0; i < 6; i++) {
        if (i == 5)
            line = 0x0;
        else
            line = Font[(c * 5) + i];
        for (j = 0; j < 8; j++) {
            if (line & 0x1) {
                if (size == 1) // default size
                    ST7735_DrawPixel(x + i, y + j, textColor);
                else { // big size
                    ST7735_FillRect(x + (i * size), y + (j * size), size, size,
                                    textColor);
                }
            } else if (bgColor != textColor) {
                if (size == 1) // default size
                    ST7735_DrawPixel(x + i, y + j, bgColor);
                else { // big size
                    ST7735_FillRect(x + i * size, y + j * size, size, size,
                                    bgColor);
                }
            }
            line >>= 1;
        }
    }
}

void ST7735_DrawChar(int16_t x, int16_t y, char c, int16_t textColor,
                     int16_t bgColor, uint8_t size) {
    uint8_t line;           // horizontal row of pixels of character
    int32_t col, row, i, j; // loop indices
    if (((x + 6 * size - 1) >= _width) ||  // Clip right
        ((y + 8 * size - 1) >= _height) || // Clip bottom
        ((x + 6 * size - 1) < 0) ||        // Clip left
        ((y + 8 * size - 1) < 0)) {        // Clip top
        return;
    }

    setAddrWindow(x, y, x + 6 * size - 1, y + 8 * size - 1);

    line = 0x01; // print the top row first
    // print the rows, starting at the top
    for (row = 0; row < 8; row = row + 1) {
        for (i = 0; i < size; i = i + 1) {
            // print the columns, starting on the left
            for (col = 0; col < 5; col = col + 1) {
                if (Font[(c * 5) + col] & line) {
                    // bit is set in Font, print pixel(s) in text color
                    for (j = 0; j < size; j = j + 1) { pushColor(textColor); }
                } else {
                    // bit is cleared in Font, print pixel(s) in background
                    // color
                    for (j = 0; j < size; j = j + 1) { pushColor(bgColor); }
                }
            }
            // print blank column(s) to the right of character
            for (j = 0; j < size; j = j + 1) { pushColor(bgColor); }
        }
        line = line << 1; // move up to the next row
    }
}

uint32_t ST7735_DrawString(uint16_t x, uint16_t y, char* pt,
                           int16_t textColor) {
    uint32_t count = 0;
    if (y > 15)
        return 0;
    while (*pt) {
        ST7735_DrawCharS(x * 6, y * 10, *pt, textColor, ST7735_BLACK, 1);
        pt++;
        x = x + 1;
        if (x > 20)
            return count; // number of characters printed
        count++;
    }
    return count; // number of characters printed
}

//-----------------------fillmessage-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
char Message[12];
uint32_t Messageindex;

void fillmessage(uint32_t n) {
    // This function uses recursion to convert decimal number
    //   of unspecified length as an ASCII string
    if (n >= 10) {
        fillmessage(n / 10);
        n = n % 10;
    }
    Message[Messageindex] = (n + '0'); /* n is between 0 and 9 */
    if (Messageindex < 11)
        Messageindex++;
}

void fillmessage4(uint32_t n) {
    if (n > 9999)
        n = 9999;
    if (n >= 1000) { // 1000 to 9999
        Messageindex = 0;
    } else if (n >= 100) { // 100 to 999
        Message[0] = ' ';
        Messageindex = 1;
    } else if (n >= 10) { //
        Message[0] = ' '; /* n is between 10 and 99 */
        Message[1] = ' ';
        Messageindex = 2;
    } else {
        Message[0] = ' '; /* n is between 0 and 9 */
        Message[1] = ' ';
        Message[2] = ' ';
        Messageindex = 3;
    }
    fillmessage(n);
}
void fillmessage5(uint32_t n) {
    if (n > 99999)
        n = 99999;
    if (n >= 10000) { // 10000 to 99999
        Messageindex = 0;
    } else if (n >= 1000) { // 1000 to 9999
        Message[0] = ' ';
        Messageindex = 1;
    } else if (n >= 100) { // 100 to 999
        Message[0] = ' ';
        Message[1] = ' ';
        Messageindex = 2;
    } else if (n >= 10) { //
        Message[0] = ' '; /* n is between 10 and 99 */
        Message[1] = ' ';
        Message[2] = ' ';
        Messageindex = 3;
    } else {
        Message[0] = ' '; /* n is between 0 and 9 */
        Message[1] = ' ';
        Message[2] = ' ';
        Message[3] = ' ';
        Messageindex = 4;
    }
    fillmessage(n);
}

void ST7735_SetCursor(uint32_t newX, uint32_t newY) {
    if ((newX > 20) || (newY > 15)) { // bad input
        return;                       // do nothing
    }
    StX = newX;
    StY = newY;
}

void ST7735_OutUDec(uint32_t n) {
    Messageindex = 0;
    fillmessage(n);
    Message[Messageindex] = 0; // terminate
    ST7735_DrawString(StX, StY, Message, StTextColor);
    StX = StX + Messageindex;
    if (StX > 20) {
        StX = 20;
        ST7735_DrawCharS(StX * 6, StY * 10, '*', ST7735_RED, ST7735_BLACK, 1);
    }
}

void ST7735_OutUDec2(uint32_t n, uint32_t l) {
    Messageindex = 0;
    fillmessage(n);
    while (Messageindex < 5) {
        Message[Messageindex++] = 32; // fill space
    }
    Message[Messageindex] = 0; // terminate
    ST7735_DrawString(14, l, Message, ST7735_YELLOW);
}

void ST7735_Message_Num(uint32_t d, uint32_t l, char* pt, int32_t value) {
    OS_Wait(&LCDFree);
    ST7735_Message(d, l, pt);
    if (value < 0) {
        ST7735_OutChar('-');
        ST7735_OutUDec(-value);
    } else {
        ST7735_OutUDec(value);
    }
    OS_Signal(&LCDFree);
}

void ST7735_Message(uint32_t d, uint32_t l, char* pt) {
    if (d == 0) {
        ST7735_SetCursor(0, l);
    } else {
        ST7735_SetCursor(0, 8 + l);
    }
    ST7735_OutString(pt);
}

void ST7735_OutUDec4(uint32_t n) {
    Messageindex = 0;
    fillmessage4(n);
    Message[Messageindex] = 0; // terminate
    ST7735_DrawString(StX, StY, Message, StTextColor);
    StX = StX + Messageindex;
    if (StX > 20) {
        StX = 20;
        ST7735_DrawCharS(StX * 6, StY * 10, '*', ST7735_RED, ST7735_BLACK, 1);
    }
}

void ST7735_OutUDec5(uint32_t n) {
    Messageindex = 0;
    fillmessage5(n);
    Message[Messageindex] = 0; // terminate
    ST7735_DrawString(StX, StY, Message, StTextColor);
    StX = StX + Messageindex;
    if (StX > 20) {
        StX = 20;
        ST7735_DrawCharS(StX * 6, StY * 10, '*', ST7735_RED, ST7735_BLACK, 1);
    }
}

#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH 0x04

void ST7735_SetRotation(uint8_t m) {
    writecommand(ST7735_MADCTL);
    Rotation = m % 4; // can't be higher than 3
    switch (Rotation) {
    case 0:
        if (TabColor == INITR_BLACKTAB) {
            writedata(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
        } else {
            writedata(MADCTL_MX | MADCTL_MY | MADCTL_BGR);
        }
        _width = ST7735_TFTWIDTH;
        _height = ST7735_TFTHEIGHT;
        break;
    case 1:
        if (TabColor == INITR_BLACKTAB) {
            writedata(MADCTL_MY | MADCTL_MV | MADCTL_RGB);
        } else {
            writedata(MADCTL_MY | MADCTL_MV | MADCTL_BGR);
        }
        _width = ST7735_TFTHEIGHT;
        _height = ST7735_TFTWIDTH;
        break;
    case 2:
        if (TabColor == INITR_BLACKTAB) {
            writedata(MADCTL_RGB);
        } else {
            writedata(MADCTL_BGR);
        }
        _width = ST7735_TFTWIDTH;
        _height = ST7735_TFTHEIGHT;
        break;
    case 3:
        if (TabColor == INITR_BLACKTAB) {
            writedata(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
        } else {
            writedata(MADCTL_MX | MADCTL_MV | MADCTL_BGR);
        }
        _width = ST7735_TFTHEIGHT;
        _height = ST7735_TFTWIDTH;
        break;
    }
}

//------------ST7735_InvertDisplay------------
// Send the command to invert all of the colors.
// Requires 1 byte of transmission
// Input: i 0 to disable inversion; non-zero to enable inversion
// Output: none
void ST7735_InvertDisplay(int i) {
    if (i) {
        writecommand(ST7735_INVON);
    } else {
        writecommand(ST7735_INVOFF);
    }
    // deselect();
}
// graphics routines
// y coordinates 0 to 31 used for labels and messages
// y coordinates 32 to 159  128 pixels high
// x coordinates 0 to 127   128 pixels wide

int32_t Ymax, Ymin, X; // X goes from 0 to 127
int32_t Yrange;        // YrangeDiv2;

// *************** ST7735_PlotClear ********************
// Clear the graphics buffer, set X coordinate to 0
// This routine clears the display
// Inputs: ymin and ymax are range of the plot
// Outputs: none
void ST7735_PlotClear(int32_t ymin, int32_t ymax) {
    ST7735_FillRect(0, 32, 128, 128,
                    ST7735_Color565(228, 228, 228)); // light grey
    if (ymax > ymin) {
        Ymax = ymax;
        Ymin = ymin;
        Yrange = ymax - ymin;
    } else {
        Ymax = ymin;
        Ymin = ymax;
        Yrange = ymax - ymin;
    }
    // YrangeDiv2 = Yrange/2;
    X = 0;
}

// *************** ST7735_PlotPoint ********************
// Used in the voltage versus time plot, plot one point at y
// It does output to display
// Inputs: y is the y coordinate of the point plotted
// Outputs: none
void ST7735_PlotPoint(int32_t y) {
    int32_t j;
    if (y < Ymin)
        y = Ymin;
    if (y > Ymax)
        y = Ymax;
    // X goes from 0 to 127
    // j goes from 159 to 32
    // y=Ymax maps to j=32
    // y=Ymin maps to j=159
    j = 32 + (127 * (Ymax - y)) / Yrange;
    if (j < 32)
        j = 32;
    if (j > 159)
        j = 159;
    ST7735_DrawPixel(X, j, ST7735_BLUE);
    ST7735_DrawPixel(X + 1, j, ST7735_BLUE);
    ST7735_DrawPixel(X, j + 1, ST7735_BLUE);
    ST7735_DrawPixel(X + 1, j + 1, ST7735_BLUE);
}
// *************** ST7735_PlotLine ********************
// Used in the voltage versus time plot, plot line to new point
// It does output to display
// Inputs: y is the y coordinate of the point plotted
// Outputs: none
int32_t lastj = 0;
void ST7735_PlotLine(int32_t y) {
    int32_t i, j;
    if (y < Ymin)
        y = Ymin;
    if (y > Ymax)
        y = Ymax;
    // X goes from 0 to 127
    // j goes from 159 to 32
    // y=Ymax maps to j=32
    // y=Ymin maps to j=159
    j = 32 + (127 * (Ymax - y)) / Yrange;
    if (j < 32)
        j = 32;
    if (j > 159)
        j = 159;
    if (lastj < 32)
        lastj = j;
    if (lastj > 159)
        lastj = j;
    if (lastj < j) {
        for (i = lastj + 1; i <= j; i++) {
            ST7735_DrawPixel(X, i, ST7735_BLUE);
            ST7735_DrawPixel(X + 1, i, ST7735_BLUE);
        }
    } else if (lastj > j) {
        for (i = j; i < lastj; i++) {
            ST7735_DrawPixel(X, i, ST7735_BLUE);
            ST7735_DrawPixel(X + 1, i, ST7735_BLUE);
        }
    } else {
        ST7735_DrawPixel(X, j, ST7735_BLUE);
        ST7735_DrawPixel(X + 1, j, ST7735_BLUE);
    }
    lastj = j;
}

// *************** ST7735_PlotPoints ********************
// Used in the voltage versus time plot, plot two points at y1, y2
// It does output to display
// Inputs: y1 is the y coordinate of the first point plotted
//         y2 is the y coordinate of the second point plotted
// Outputs: none
void ST7735_PlotPoints(int32_t y1, int32_t y2) {
    int32_t j;
    if (y1 < Ymin)
        y1 = Ymin;
    if (y1 > Ymax)
        y1 = Ymax;
    // X goes from 0 to 127
    // j goes from 159 to 32
    // y=Ymax maps to j=32
    // y=Ymin maps to j=159
    j = 32 + (127 * (Ymax - y1)) / Yrange;
    if (j < 32)
        j = 32;
    if (j > 159)
        j = 159;
    ST7735_DrawPixel(X, j, ST7735_BLUE);
    if (y2 < Ymin)
        y2 = Ymin;
    if (y2 > Ymax)
        y2 = Ymax;
    j = 32 + (127 * (Ymax - y2)) / Yrange;
    if (j < 32)
        j = 32;
    if (j > 159)
        j = 159;
    ST7735_DrawPixel(X, j, ST7735_BLACK);
}
// *************** ST7735_PlotBar ********************
// Used in the voltage versus time bar, plot one bar at y
// It does not output to display until RIT128x96x4ShowPlot called
// Inputs: y is the y coordinate of the bar plotted
// Outputs: none
void ST7735_PlotBar(int32_t y) {
    int32_t j;
    if (y < Ymin)
        y = Ymin;
    if (y > Ymax)
        y = Ymax;
    // X goes from 0 to 127
    // j goes from 159 to 32
    // y=Ymax maps to j=32
    // y=Ymin maps to j=159
    j = 32 + (127 * (Ymax - y)) / Yrange;
    ST7735_DrawFastVLine(X, j, 159 - j, ST7735_BLACK);
}

// full scaled defined as 3V
// Input is 0 to 511, 0 => 159 and 511 => 32
uint8_t const dBfs[512] = {
    159, 159, 145, 137, 131, 126, 123, 119, 117, 114, 112, 110, 108, 107, 105,
    104, 103, 101, 100, 99,  98,  97,  96,  95,  94,  93,  93,  92,  91,  90,
    90,  89,  88,  88,  87,  87,  86,  85,  85,  84,  84,  83,  83,  82,  82,
    81,  81,  81,  80,  80,  79,  79,  79,  78,  78,  77,  77,  77,  76,  76,
    76,  75,  75,  75,  74,  74,  74,  73,  73,  73,  72,  72,  72,  72,  71,
    71,  71,  71,  70,  70,  70,  70,  69,  69,  69,  69,  68,  68,  68,  68,
    67,  67,  67,  67,  66,  66,  66,  66,  66,  65,  65,  65,  65,  65,  64,
    64,  64,  64,  64,  63,  63,  63,  63,  63,  63,  62,  62,  62,  62,  62,
    62,  61,  61,  61,  61,  61,  61,  60,  60,  60,  60,  60,  60,  59,  59,
    59,  59,  59,  59,  59,  58,  58,  58,  58,  58,  58,  58,  57,  57,  57,
    57,  57,  57,  57,  56,  56,  56,  56,  56,  56,  56,  56,  55,  55,  55,
    55,  55,  55,  55,  55,  54,  54,  54,  54,  54,  54,  54,  54,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  52,  52,  52,  52,  52,  52,  52,  52,
    52,  52,  51,  51,  51,  51,  51,  51,  51,  51,  51,  51,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  50,  49,  49,  49,  49,  49,  49,  49,  49,
    49,  49,  49,  48,  48,  48,  48,  48,  48,  48,  48,  48,  48,  48,  47,
    47,  47,  47,  47,  47,  47,  47,  47,  47,  47,  47,  46,  46,  46,  46,
    46,  46,  46,  46,  46,  46,  46,  46,  46,  45,  45,  45,  45,  45,  45,
    45,  45,  45,  45,  45,  45,  45,  44,  44,  44,  44,  44,  44,  44,  44,
    44,  44,  44,  44,  44,  44,  43,  43,  43,  43,  43,  43,  43,  43,  43,
    43,  43,  43,  43,  43,  43,  42,  42,  42,  42,  42,  42,  42,  42,  42,
    42,  42,  42,  42,  42,  42,  41,  41,  41,  41,  41,  41,  41,  41,  41,
    41,  41,  41,  41,  41,  41,  41,  40,  40,  40,  40,  40,  40,  40,  40,
    40,  40,  40,  40,  40,  40,  40,  40,  40,  39,  39,  39,  39,  39,  39,
    39,  39,  39,  39,  39,  39,  39,  39,  39,  39,  39,  39,  38,  38,  38,
    38,  38,  38,  38,  38,  38,  38,  38,  38,  38,  38,  38,  38,  38,  38,
    38,  37,  37,  37,  37,  37,  37,  37,  37,  37,  37,  37,  37,  37,  37,
    37,  37,  37,  37,  37,  36,  36,  36,  36,  36,  36,  36,  36,  36,  36,
    36,  36,  36,  36,  36,  36,  36,  36,  36,  36,  36,  35,  35,  35,  35,
    35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
    35,  35,  34,  34,  34,  34,  34,  34,  34,  34,  34,  34,  34,  34,  34,
    34,  34,  34,  34,  34,  34,  34,  34,  34,  34,  33,  33,  33,  33,  33,
    33,  33,  33,  33,  33,  33,  33,  33,  33,  33,  33,  33,  33,  33,  33,
    33,  33,  33,  33,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,
    32,  32};

// *************** ST7735_PlotdBfs ********************
// Used in the amplitude versus frequency plot, plot bar point at y
// 0 to 0.625V scaled on a log plot from min to max
// It does output to display
// Inputs: y is the y ADC value of the bar plotted
// Outputs: none
void ST7735_PlotdBfs(int32_t y) {
    int32_t j;
    y = y / 2; // 0 to 2047
    if (y < 0)
        y = 0;
    if (y > 511)
        y = 511;
    // X goes from 0 to 127
    // j goes from 159 to 32
    // y=511 maps to j=32
    // y=0 maps to j=159
    j = dBfs[y];
    ST7735_DrawFastVLine(X, j, 159 - j, ST7735_BLACK);
}

// *************** ST7735_PlotNext ********************
// Used in all the plots to step the X coordinate one pixel
// X steps from 0 to 127, then back to 0 again
// It does not output to display
// Inputs: none
// Outputs: none
void ST7735_PlotNext(void) {
    if (X == 127) {
        X = 0;
    } else {
        X++;
    }
}

// *************** ST7735_PlotNextErase ********************
// Used in all the plots to step the X coordinate one pixel
// X steps from 0 to 127, then back to 0 again
// It clears the vertical space into which the next pixel will be drawn
// Inputs: none
// Outputs: none
void ST7735_PlotNextErase(void) {
    if (X == 127) {
        X = 0;
    } else {
        X++;
    }
    ST7735_DrawFastVLine(X, 32, 128, ST7735_Color565(228, 228, 228));
}

void ST7735_OutChar(char ch) {
    if ((ch == 10) || (ch == 13) || (ch == 27)) {
        StY++;
        StX = 0;
        if (StY > 15) {
            StY = 0;
        }
        ST7735_DrawString(0, StY, "                     ", StTextColor);
        return;
    }
    ST7735_DrawCharS(StX * 6, StY * 10, ch, ST7735_YELLOW, ST7735_BLACK, 1);
    StX++;
    if (StX > 20) {
        StX = 20;
        ST7735_DrawCharS(StX * 6, StY * 10, '*', ST7735_RED, ST7735_BLACK, 1);
    }
    return;
}

void ST7735_OutString(char* ptr) {
    while (*ptr) {
        ST7735_OutChar(*ptr);
        ptr = ptr + 1;
    }
}

void ST7735_SetTextColor(uint16_t color) {
    StTextColor = color;
}
