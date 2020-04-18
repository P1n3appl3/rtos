// Driver written by Limor Fried aka Ladyada for Adafruit Industries.

// Adafruit ST7735 breakout -> TM4C pins
// SCK -> PA2
// TFT_CS -> PA3
// MISO -> PA4
// MOSI -> PA5
// COMMAND/DATA -> PA6
// RESET -> PA7
// CARD_CS -> PB0

#pragma once
#include <stdbool.h>
#include <stdint.h>

#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160

#define LCD_BLACK 0x0000
#define LCD_BLUE 0xF800
#define LCD_RED 0x001F
#define LCD_GREEN 0x07E0
#define LCD_CYAN 0xFFE0
#define LCD_MAGENTA 0xF81F
#define LCD_YELLOW 0x07FF
#define LCD_WHITE 0xFFFF

// Initialization for ST7735R screens (green or red tabs).
void lcd_init();

// convert 24 bit color to the 16 bit BGR format used by the ST7735
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b);

// Color the pixel at the given coordinates with the given color.
// Requires 13 bytes of transmission
// Input: x     horizontal position of the pixel, columns from the left edge
//               must be less than 128
//               0 is on the left, 126 is near the right
//        y     vertical position of the pixel, rows from the top edge
//               must be less than 160
//               159 is near the wires, 0 is the side opposite the wires
//        color 16-bit color in BGR 5-6-5 format
void lcd_pixel(int16_t x, int16_t y, uint16_t color);

// Draw a vertical line at the given coordinates with the given height and
// color. A vertical line is parallel to the longer side of the rectangular
// display Requires (11 + 2*h) bytes of transmission (assuming image fully on
// screen)
void lcd_fast_vline(int16_t x, int16_t y, int16_t h, uint16_t color);

// Draw a horizontal line at the given coordinates with the given width and
// color. A horizontal line is parallel to the shorter side of the rectangular
// display Requires (11 + 2*w) bytes of transmission (assuming image fully on
// screen)
void lcd_fast_hline(int16_t x, int16_t y, int16_t w, uint16_t color);

// Input: color 16-bit color in BGR 5-6-5 format
void lcd_fill(uint16_t color);

// Draw a filled rectangle at the given coordinates with the given width,
// height, and color. Requires (11 + 2*w*h) bytes of transmission (assuming
// image fully on screen)
// Input: x     horizontal position of the top left corner of the rectangle,
//              columns from the left edge
//        y     vertical position of the top left corner of the rectangle, rows
//              from the top edge
//        w     horizontal width of the rectangle
//        h     vertical height of the rectangle
//        color 16-bit color in BGR 5-6-5 format
void lcd_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

// Displays a 16-bit color bitmap.
// array image[] has one 16-bit color for each pixel to be
// displayed on the screen (encoded in reverse order, which is
// standard for bitmap files)
// (x,y) is the screen location of the lower left corner of BMP image
// Requires (11 + 2*w*h) bytes of transmission (assuming image fully on screen)
// Input: x     horizontal position of the bottom left corner of the image,
//              columns from the left edge
//        y     vertical position of the bottom left corner of the image, rows
//              from the top edge
//        image pointer to a 16-bit color image data
//        w     number of pixels wide
//        h     number of pixels tall
// Must be less than or equal to 128 pixels wide by 160 pixels high
void lcd_bitmap(int16_t x, int16_t y, const uint16_t* image, int16_t w,
                int16_t h);

// If the background color is the same as the text color, no background will be
// printed, and text can be drawn right over existing images without covering
// them with a box. Requires (11 + 2*size*size)*6*8 (image fully on screen;
// textcolor != bgColor)
// Input: x         horizontal position of the top left corner of the character,
//                  columns from the left edge
//        y         vertical position of the top left corner of the character,
//                  rows from the top edge
//        c         character to be printed
//        textColor 16-bit color of the character
//        bgColor   16-bit color of the background
//        size      number of pixels per character pixel (e.g. size==2 prints
//        each pixel of font as 2x2 square)
void lcd_char(int16_t x, int16_t y, char c, int16_t textColor, int16_t bgColor,
              uint8_t size);

// String draw function.
// 16 rows (0 to 15) and 21 characters (0 to 20)
// Requires (11 + size*size*6*8) bytes of transmission for each character
// Input: x         columns from the left edge (0 to 20)
//        y         rows from the top edge (0 to 15)
//        pt        pointer to a null terminated string to be printed
//        textColor 16-bit color of the characters
// bgColor is Black and size is 1
// Output: number of characters printed
uint32_t lcd_string(uint16_t x, uint16_t y, char* pt, int16_t textColor);

// Move the cursor to the desired X- and Y-position.  The
// next character will be printed here.  X=0 is the leftmost
// column.  Y=0 is the top row.
// inputs: newX  new X-position of the cursor (0<=newX<=20)
//         newY  new Y-position of the cursor (0<=newY<=15)
void lcd_set_cursor(uint32_t newX, uint32_t newY);

// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
void lcd_num(uint32_t n);

// String draw and number output.
// Input: device  0 is on top, 1 is on bottom
//        line    row from top, 0 to 7 for each device
//        pt      pointer to a null terminated string to be printed
//        value   signed integer to be printed
void lcd_message_num(uint32_t d, uint32_t l, char* pt, int32_t value);

// String draw
// Input: device  0 is on top, 1 is on bottom
//        line    row from top, 0 to 7 for each device
//        pt      pointer to a null terminated string to be printed
void lcd_message(uint32_t d, uint32_t l, char* pt);

// Send the command to invert all of the colors.
// Requires 1 byte of transmission
// Input: false to disable inversion, true to enable inversion
void lcd_invert(bool i);

// Output one character to the LCD
// Inputs: 8-bit ASCII character
void lcd_putchar(char ch);

// Print a string of characters to the ST7735 LCD.
// The string will not automatically wrap.
// inputs: ptr  pointer to NULL-terminated ASCII string
void lcd_puts(char* ptr);

// Sets the color in which the characters will be printed
// Background color is fixed at black
// Input:  16-bit packed color
void lcd_set_text_color(uint16_t color);
