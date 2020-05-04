#include "ST7735.h"
#include "OS.h"
#include "eDisk.h"
#include "font.h"
#include "std.h"
#include "timer.h"
#include "tivaware/gpio.h"
#include "tivaware/rom.h"
#include <stdint.h>

// 16 rows (0 to 15) and 21 characters (0 to 20)
// Requires (11 + size*size*6*8) bytes of transmission for each character
static uint8_t cursor_x = 0; // position along the horizonal axis 0 to 20
static uint8_t cursor_y = 0; // position along the vertical axis 0 to 15
uint16_t text_color = LCD_CYAN;

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

#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160

static void select(void) {
    ROM_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0);
}

static void deselect(void) {
    while (ROM_SSIBusy(SSI0_BASE)) {}
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3);
}

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
static void write_command(uint8_t c) {
    uint32_t _response;
    while (ROM_SSIBusy(SSI0_BASE)) {}
    select();
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_6, 0); // command mode
    ROM_SSIDataPut(SSI0_BASE, c);
    ROM_SSIDataGet(SSI0_BASE, &_response);
    deselect();
}

static void write_data(uint8_t c) {
    uint32_t _response;
    while (ROM_SSIBusy(SSI0_BASE)) {}
    select();
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_PIN_6); // data mode
    ROM_SSIDataPut(SSI0_BASE, c);
    ROM_SSIDataGet(SSI0_BASE, &_response);
    deselect();
}

// Rather than a bazillion write_command() and write_data() calls, screen
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
static void command_list(const uint8_t* addr) {
    uint8_t numCommands, numArgs;
    uint16_t wait;

    numCommands = *(addr++);       // Number of commands to follow
    while (numCommands--) {        // For each command...
        write_command(*(addr++));  //   Read, issue command
        numArgs = *(addr++);       //   Number of args to follow
        wait = numArgs & DELAY;    //   If hibit set, delay follows args
        numArgs &= ~DELAY;         //   Mask out delay bit
        while (numArgs--) {        //   For each argument...
            write_data(*(addr++)); //     Read, issue argument
        }

        if (wait) {
            wait = *(addr++); // Read post-command delay time (ms)
            if (wait == 255)
                wait = 500; // If 255, delay for 500 ms
            busy_wait(3, ms(wait));
        }
    }
}

Sema4 LCDFree;

void lcd_init() {
    SSI0_Init(10);
    select();
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_PIN_7); // reset high
    busy_wait(3, ms(500));
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, 0); // reset low
    busy_wait(3, ms(500));
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_PIN_7); // reset high
    busy_wait(3, ms(500));

    command_list(Rcmd1);
    command_list(Rcmd2red);
    command_list(Rcmd3);

    lcd_set_cursor(0, 0);
    text_color = LCD_YELLOW;
    lcd_fill(LCD_BLACK);
    OS_InitSemaphore(&LCDFree, 0);
}

// Set the region of the screen RAM to be modified
// Pixel colors are sent left to right, top to bottom
// (same as Font table is encoded; different from regular bitmap)
// Requires 11 bytes of transmission
static void set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    write_command(ST7735_CASET); // Column addr set
    write_data(0x00);
    write_data(x0); // XSTART
    write_data(0x00);
    write_data(x1); // XEND

    write_command(ST7735_RASET); // Row addr set
    write_data(0x00);
    write_data(y0); // YSTART
    write_data(0x00);
    write_data(y1); // YEND

    write_command(ST7735_RAMWR); // write to RAM
}

// Send two bytes of data, most significant byte first
static void push_color(Color color) {
    write_data((color >> 8));
    write_data(color);
}

void lcd_pixel(int16_t x, int16_t y, Color color) {
    if ((x < 0) || (x >= ST7735_WIDTH) || (y < 0) || (y >= ST7735_HEIGHT))
        return;
    set_address_window(x, y, x, y);
    push_color(color);
}

void lcd_fast_vline(int16_t x, int16_t y, int16_t h, Color color) {
    uint8_t hi = color >> 8, lo = color;

    // Rudimentary clipping
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT))
        return;
    if ((y + h - 1) >= ST7735_HEIGHT)
        h = ST7735_HEIGHT - y;
    set_address_window(x, y, x, y + h - 1);

    while (h--) {
        write_data(hi);
        write_data(lo);
    }
}

void lcd_fast_hline(int16_t x, int16_t y, int16_t w, Color color) {
    uint8_t hi = color >> 8, lo = color;

    // Rudimentary clipping
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT))
        return;
    if ((x + w - 1) >= ST7735_WIDTH)
        w = ST7735_WIDTH - x;
    set_address_window(x, y, x + w - 1, y);

    while (w--) {
        write_data(hi);
        write_data(lo);
    }
}

void lcd_fill(Color color) {
    lcd_rect(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
    //  screen is actually 129 by 161 pixels, x 0 to 128, y goes from 0 to 160
}

void lcd_rect(int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
    uint8_t hi = color >> 8, lo = color;

    // rudimentary clipping (drawChar w/big text requires this)
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT))
        return;
    if ((x + w - 1) >= ST7735_WIDTH)
        w = ST7735_WIDTH - x;
    if ((y + h - 1) >= ST7735_HEIGHT)
        h = ST7735_HEIGHT - y;

    set_address_window(x, y, x + w - 1, y + h - 1);

    for (y = h; y > 0; y--) {
        for (x = w; x > 0; x--) {
            write_data(hi);
            write_data(lo);
        }
    }
}

Color rgb_packed(uint32_t packed_color) {
    return ((packed_color << 8) & ~0xF800) | ((packed_color >> 3) & ~0x07E0) |
           (packed_color >> 19);
}

Color rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3);
}

void lcd_bitmap(int16_t x, int16_t y, const uint16_t* image, int16_t w,
                int16_t h) {
    int16_t skipC = 0; // non-zero if columns need to be skipped due to clipping
    int16_t originalWidth =
        w; // save this value; even if not all columns fit on the screen, the
           // image is still this width in ROM
    int i = w * (h - 1);

    if ((x >= ST7735_WIDTH) || ((y - h + 1) >= ST7735_HEIGHT) ||
        ((x + w) <= 0) || (y < 0)) {
        return; // image is totally off the screen, do nothing
    }
    if ((w > ST7735_WIDTH) ||
        (h > ST7735_HEIGHT)) { // image is too wide for the screen, do nothing
        // This isn't necessarily a fatal error, but it makes the
        // following logic much more complicated, since you can have
        // an image that exceeds multiple boundaries and needs to be
        // clipped on more than one side.
        return;
    }
    if ((x + w - 1) >= ST7735_WIDTH) {  // image exceeds right of screen
        skipC = (x + w) - ST7735_WIDTH; // skip cut off columns
        w = ST7735_WIDTH - x;
    }
    if ((y - h + 1) < 0) {                   // image exceeds top of screen
        i = i - (h - y - 1) * originalWidth; // skip the last cut off rows
        h = y + 1;
    }
    if (x < 0) { // image exceeds left of screen
        w = w + x;
        skipC = -x; // skip cut off columns
        i = i - x;  // skip the first cut off columns
        x = 0;
    }
    if (y >= ST7735_HEIGHT) { // image exceeds bottom of screen
        h = h - (y - ST7735_HEIGHT + 1);
        y = ST7735_HEIGHT - 1;
    }

    set_address_window(x, y - h + 1, x + w - 1, y);

    for (y = 0; y < h; y = y + 1) {
        for (x = 0; x < w; x = x + 1) {
            // send the top 8 bits
            write_data((uint8_t)(image[i] >> 8));
            // send the bottom 8 bits
            write_data((uint8_t)image[i]);
            i = i + 1; // go to the next pixel
        }
        i = i + skipC;
        i = i - 2 * originalWidth;
    }
}

void lcd_char_slow(int16_t x, int16_t y, char c, int16_t textColor,
                   int16_t bgColor, uint8_t size) {
    uint8_t line; // vertical column of pixels of character in font
    int32_t i, j;
    if ((x >= ST7735_WIDTH) ||      // Clip right
        (y >= ST7735_HEIGHT) ||     // Clip bottom
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
                    lcd_pixel(x + i, y + j, textColor);
                else { // big size
                    lcd_rect(x + (i * size), y + (j * size), size, size,
                             textColor);
                }
            } else if (bgColor != textColor) {
                if (size == 1) // default size
                    lcd_pixel(x + i, y + j, bgColor);
                else { // big size
                    lcd_rect(x + i * size, y + j * size, size, size, bgColor);
                }
            }
            line >>= 1;
        }
    }
}

void lcd_char(int16_t x, int16_t y, char c, int16_t textColor, int16_t bgColor,
              uint8_t size) {
    uint8_t line;           // horizontal row of pixels of character
    int32_t col, row, i, j; // loop indices
    if (((x + 6 * size - 1) >= ST7735_WIDTH) ||  // Clip right
        ((y + 8 * size - 1) >= ST7735_HEIGHT) || // Clip bottom
        ((x + 6 * size - 1) < 0) ||              // Clip left
        ((y + 8 * size - 1) < 0)) {              // Clip top
        return;
    }

    set_address_window(x, y, x + 6 * size - 1, y + 8 * size - 1);

    line = 0x01; // print the top row first
    // print the rows, starting at the top
    for (row = 0; row < 8; row = row + 1) {
        for (i = 0; i < size; i = i + 1) {
            // print the columns, starting on the left
            for (col = 0; col < 5; col = col + 1) {
                if (Font[(c * 5) + col] & line) {
                    // bit is set in Font, print pixel(s) in text color
                    for (j = 0; j < size; j = j + 1) { push_color(textColor); }
                } else {
                    // bit is cleared in Font, print pixel(s) in background
                    // color
                    for (j = 0; j < size; j = j + 1) { push_color(bgColor); }
                }
            }
            // print blank column(s) to the right of character
            for (j = 0; j < size; j = j + 1) { push_color(bgColor); }
        }
        line = line << 1; // move up to the next row
    }
}

uint32_t lcd_string(uint16_t x, uint16_t y, char* pt, int16_t textColor) {
    uint32_t count = 0;
    if (y > 15)
        return 0;
    while (*pt) {
        lcd_char(x * 6, y * 10, *pt, textColor, LCD_BLACK, 1);
        pt++;
        x = x + 1;
        if (x > 20)
            return count; // number of characters printed
        count++;
    }
    return count; // number of characters printed
}

void lcd_set_cursor(uint32_t newX, uint32_t newY) {
    if ((newX > 20) || (newY > 15)) { // bad input
        return;                       // do nothing
    }
    cursor_x = newX;
    cursor_y = newY;
}

void lcd_message_num(uint32_t d, uint32_t l, char* pt, int32_t value) {
    OS_Wait(&LCDFree);
    lcd_message(d, l, pt);
    if (value < 0) {
        lcd_putchar('-');
        lcd_num(-value);
    } else {
        lcd_num(value);
    }
    OS_Signal(&LCDFree);
}

void lcd_num(uint32_t n) {
    char buf[12];
    lcd_puts(itoa(n, buf, 10));
}

void lcd_message(uint32_t d, uint32_t l, char* pt) {
    lcd_set_cursor(0, 8 * d + l);
    lcd_puts(pt);
}

void lcd_invert(bool i) {
    if (i) {
        write_command(ST7735_INVON);
    } else {
        write_command(ST7735_INVOFF);
    }
}

void lcd_putchar(char ch) {
    if ((ch == 10) || (ch == 13) || (ch == 27)) {
        cursor_y++;
        cursor_x = 0;
        if (cursor_y > 15) {
            cursor_y = 0;
        }
        lcd_string(0, cursor_y, "                     ", text_color);
        return;
    }
    lcd_char(cursor_x * 6, cursor_y * 10, ch, LCD_YELLOW, LCD_BLACK, 1);
    cursor_x++;
    if (cursor_x > 20) {
        cursor_x = 20;
        lcd_char(cursor_x * 6, cursor_y * 10, '*', LCD_RED, LCD_BLACK, 1);
    }
    return;
}

void lcd_puts(const char* s) {
    while (*s) { lcd_putchar(*s), ++s; }
}

void lcd_set_text_color(Color color) {
    text_color = color;
}
