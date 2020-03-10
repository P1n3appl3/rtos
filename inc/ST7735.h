// Written by Limor Fried/Ladyada for Adafruit Industries.

// hardware connections
// **********ST7735 TFT and SDC*******************
// ST7735
// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#pragma once
#include <stdint.h>
// some flags for ST7735_InitR()
enum initRFlags { none, INITR_GREENTAB, INITR_REDTAB, INITR_BLACKTAB };

#define ST7735_TFTWIDTH 128
#define ST7735_TFTHEIGHT 160

// Color definitions
#define ST7735_BLACK 0x0000
#define ST7735_BLUE 0xF800
#define ST7735_RED 0x001F
#define ST7735_GREEN 0x07E0
#define ST7735_CYAN 0xFFE0
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW 0x07FF
#define ST7735_WHITE 0xFFFF

//------------ST7735_InitR------------
// Initialization for ST7735R screens (green or red tabs).
// Input: option one of the enumerated options depending on tabs
// Output: none
void ST7735_InitR(enum initRFlags option);

//------------ST7735_DrawPixel------------
// Color the pixel at the given coordinates with the given color.
// Requires 13 bytes of transmission
// Input: x     horizontal position of the pixel, columns from the left edge
//               must be less than 128
//               0 is on the left, 126 is near the right
//        y     vertical position of the pixel, rows from the top edge
//               must be less than 160
//               159 is near the wires, 0 is the side opposite the wires
//        color 16-bit color, which can be produced by ST7735_Color565()
// Output: none
void ST7735_DrawPixel(int16_t x, int16_t y, uint16_t color);

//------------ST7735_DrawFastVLine------------
// Draw a vertical line at the given coordinates with the given height and
// color. A vertical line is parallel to the longer side of the rectangular
// display Requires (11 + 2*h) bytes of transmission (assuming image fully on
// screen) Input: x     horizontal position of the start of the line, columns
// from the left edge
//        y     vertical position of the start of the line, rows from the top
//        edge h     vertical height of the line color 16-bit color, which can
//        be produced by ST7735_Color565()
// Output: none
void ST7735_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);

//------------ST7735_DrawFastHLine------------
// Draw a horizontal line at the given coordinates with the given width and
// color. A horizontal line is parallel to the shorter side of the rectangular
// display Requires (11 + 2*w) bytes of transmission (assuming image fully on
// screen) Input: x     horizontal position of the start of the line, columns
// from the left edge
//        y     vertical position of the start of the line, rows from the top
//        edge w     horizontal width of the line color 16-bit color, which can
//        be produced by ST7735_Color565()
// Output: none
void ST7735_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

//------------ST7735_FillScreen------------
// Fill the screen with the given color.
// Requires 40,971 bytes of transmission
// Input: color 16-bit color, which can be produced by ST7735_Color565()
// Output: none
void ST7735_FillScreen(uint16_t color);

//------------ST7735_FillRect------------
// Draw a filled rectangle at the given coordinates with the given width,
// height, and color. Requires (11 + 2*w*h) bytes of transmission (assuming
// image fully on screen) Input: x     horizontal position of the top left
// corner of the rectangle, columns from the left edge
//        y     vertical position of the top left corner of the rectangle, rows
//        from the top edge w     horizontal width of the rectangle h vertical
//        height of the rectangle color 16-bit color, which can be produced by
//        ST7735_Color565()
// Output: none
void ST7735_FillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t color);

//------------ST7735_DrawSmallCircle------------
// Draw a small circle (diameter of 6 pixels)
// rectangle at the given coordinates with the given color.
// Requires (11*6+24*2)=114 bytes of transmission (assuming image fully on
// screen) Input: x     horizontal position of the top left corner of the
// circle, columns from the left edge
//        y     vertical position of the top left corner of the circle, rows
//        from the top edge color 16-bit color, which can be produced by
//        ST7735_Color565()
// Output: none
void ST7735_DrawSmallCircle(int16_t x, int16_t y, uint16_t color);

//------------ST7735_DrawCircle------------
// Draw a small circle (diameter of 10 pixels)
// rectangle at the given coordinates with the given color.
// Requires (11*10+68*2)=178 bytes of transmission (assuming image on screen)
// Input: x     horizontal position of the top left corner of the circle,
// columns from the left edge
//        y     vertical position of the top left corner of the circle, rows
//        from the top edge color 16-bit color, which can be produced by
//        ST7735_Color565()
// Output: none
void ST7735_DrawCircle(int16_t x, int16_t y, uint16_t color);
//------------ST7735_Color565------------
// Pass 8-bit (each) R,G,B and get back 16-bit packed color.
// Input: r red value
//        g green value
//        b blue value
// Output: 16-bit color
uint16_t ST7735_Color565(uint8_t r, uint8_t g, uint8_t b);

//------------ST7735_SwapColor------------
// Swaps the red and blue values of the given 16-bit packed color;
// green is unchanged.
// Input: x 16-bit color in format B, G, R
// Output: 16-bit color in format R, G, B
uint16_t ST7735_SwapColor(uint16_t x);

//------------ST7735_DrawBitmap------------
// Displays a 16-bit color BMP image.  A bitmap file that is created
// by a PC image processing program has a header and may be padded
// with dummy columns so the data have four byte alignment.  This
// function assumes that all of that has been stripped out, and the
// array image[] has one 16-bit halfword for each pixel to be
// displayed on the screen (encoded in reverse order, which is
// standard for bitmap files).  An array can be created in this
// format from a 24-bit-per-pixel .bmp file using the associated
// converter program.
// (x,y) is the screen location of the lower left corner of BMP image
// Requires (11 + 2*w*h) bytes of transmission (assuming image fully on screen)
// Input: x     horizontal position of the bottom left corner of the image,
// columns from the left edge
//        y     vertical position of the bottom left corner of the image, rows
//        from the top edge image pointer to a 16-bit color BMP image w number
//        of pixels wide h     number of pixels tall
// Output: none
// Must be less than or equal to 128 pixels wide by 160 pixels high
void ST7735_DrawBitmap(int16_t x, int16_t y, const uint16_t* image, int16_t w,
                       int16_t h);

//------------ST7735_DrawCharS------------
// Simple character draw function.  This is the same function from
// Adafruit_GFX.c but adapted for this processor.  However, each call
// to ST7735_DrawPixel() calls setAddrWindow(), which needs to send
// many extra data and commands.  If the background color is the same
// as the text color, no background will be printed, and text can be
// drawn right over existing images without covering them with a box.
// Requires (11 + 2*size*size)*6*8 (image fully on screen; textcolor != bgColor)
// Input: x         horizontal position of the top left corner of the character,
// columns from the left edge
//        y         vertical position of the top left corner of the character,
//        rows from the top edge c         character to be printed textColor
//        16-bit color of the character bgColor   16-bit color of the background
//        size      number of pixels per character pixel (e.g. size==2 prints
//        each pixel of font as 2x2 square)
// Output: none
void ST7735_DrawCharS(int16_t x, int16_t y, char c, int16_t textColor,
                      int16_t bgColor, uint8_t size);

//------------ST7735_DrawChar------------
// Advanced character draw function.  This is similar to the function
// from Adafruit_GFX.c but adapted for this processor.  However, this
// function only uses one call to setAddrWindow(), which allows it to
// run at least twice as fast.
// Requires (11 + size*size*6*8) bytes of transmission (assuming image fully on
// screen) Input: x         horizontal position of the top left corner of the
// character, columns from the left edge
//        y         vertical position of the top left corner of the character,
//        rows from the top edge c         character to be printed textColor
//        16-bit color of the character bgColor   16-bit color of the background
//        size      number of pixels per character pixel (e.g. size==2 prints
//        each pixel of font as 2x2 square)
// Output: none
void ST7735_DrawChar(int16_t x, int16_t y, char c, int16_t textColor,
                     int16_t bgColor, uint8_t size);

//------------ST7735_DrawString------------
// String draw function.
// 16 rows (0 to 15) and 21 characters (0 to 20)
// Requires (11 + size*size*6*8) bytes of transmission for each character
// Input: x         columns from the left edge (0 to 20)
//        y         rows from the top edge (0 to 15)
//        pt        pointer to a null terminated string to be printed
//        textColor 16-bit color of the characters
// bgColor is Black and size is 1
// Output: number of characters printed
uint32_t ST7735_DrawString(uint16_t x, uint16_t y, char* pt, int16_t textColor);

//********ST7735_SetCursor*****************
// Move the cursor to the desired X- and Y-position.  The
// next character will be printed here.  X=0 is the leftmost
// column.  Y=0 is the top row.
// inputs: newX  new X-position of the cursor (0<=newX<=20)
//         newY  new Y-position of the cursor (0<=newY<=15)
// outputs: none
void ST7735_SetCursor(uint32_t newX, uint32_t newY);

//-----------------------ST7735_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Position determined by ST7735_SetCursor command
// Color set by ST7735_SetTextColor
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void ST7735_OutUDec(uint32_t n);

//------------ST7735_Message------------
// String draw and number output.
// Input: device  0 is on top, 1 is on bottom
//        line    row from top, 0 to 7 for each device
//        pt      pointer to a null terminated string to be printed
//        value   signed integer to be printed
void ST7735_Message_Num(uint32_t d, uint32_t l, char* pt, int32_t value);

//------------ST7735_Message------------
// String draw
// Input: device  0 is on top, 1 is on bottom
//        line    row from top, 0 to 7 for each device
//        pt      pointer to a null terminated string to be printed
void ST7735_Message(uint32_t d, uint32_t l, char* pt);

//-----------------------ST7735_OutUDec4-----------------------
// Output a 32-bit number in unsigned 4-digit decimal format
// Position determined by ST7735_SetCursor command
// Color set by ST7735_SetTextColor
// Input: 32-bit number to be transferred
// Output: none
// Fixed format 4 digits with no space before or after
void ST7735_OutUDec4(uint32_t n);

//-----------------------ST7735_OutUDec5-----------------------
// Output a 32-bit number in unsigned 5-digit decimal format
// Position determined by ST7735_SetCursor command
// Color set by ST7735_SetTextColor
// Input: 32-bit number to be transferred
// Output: none
// Fixed format 5 digits with no space before or after
void ST7735_OutUDec5(uint32_t n);

//------------ST7735_SetRotation------------
// Change the image rotation.
// Requires 2 bytes of transmission
// Input: m new rotation value (0 to 3)
// Output: none
void ST7735_SetRotation(uint8_t m);

//------------ST7735_InvertDisplay------------
// Send the command to invert all of the colors.
// Requires 1 byte of transmission
// Input: i 0 to disable inversion; non-zero to enable inversion
// Output: none
void ST7735_InvertDisplay(int i);

// graphics routines
// y coordinates 0 to 31 used for labels and messages
// y coordinates 32 to 159  128 pixels high
// x coordinates 0 to 127   128 pixels wide

// *************** ST7735_PlotClear ********************
// Clear the graphics buffer, set X coordinate to 0
// This routine clears the display
// Inputs: ymin and ymax are range of the plot
// Outputs: none
void ST7735_PlotClear(int32_t ymin, int32_t ymax);

// *************** ST7735_PlotPoint ********************
// Used in the voltage versus time plot, plot one point at y
// It does output to display
// Inputs: y is the y coordinate of the point plotted
// Outputs: none
void ST7735_PlotPoint(int32_t y);

// *************** ST7735_PlotLine ********************
// Used in the voltage versus time plot, plot line to new point
// It does output to display
// Inputs: y is the y coordinate of the point plotted
// Outputs: none
void ST7735_PlotLine(int32_t y);

// *************** ST7735_PlotPoints ********************
// Used in the voltage versus time plot, plot two points at y1, y2
// It does output to display
// Inputs: y1 is the y coordinate of the first point plotted
//         y2 is the y coordinate of the second point plotted
// Outputs: none
void ST7735_PlotPoints(int32_t y1, int32_t y2);

// *************** ST7735_PlotBar ********************
// Used in the voltage versus time bar, plot one bar at y
// It does not output to display until RIT128x96x4ShowPlot called
// Inputs: y is the y coordinate of the bar plotted
// Outputs: none
void ST7735_PlotBar(int32_t y);

// *************** ST7735_PlotdBfs ********************
// Used in the amplitude versus frequency plot, plot bar point at y
// 0 to 0.625V scaled on a log plot from min to max
// It does output to display
// Inputs: y is the y ADC value of the bar plotted
// Outputs: none
void ST7735_PlotdBfs(int32_t y);

// *************** ST7735_PlotNext ********************
// Used in all the plots to step the X coordinate one pixel
// X steps from 0 to 127, then back to 0 again
// It does not output to display
// Inputs: none
// Outputs: none
void ST7735_PlotNext(void);

// *************** ST7735_PlotNextErase ********************
// Used in all the plots to step the X coordinate one pixel
// X steps from 0 to 127, then back to 0 again
// It clears the vertical space into which the next pixel will be drawn
// Inputs: none
// Outputs: none
void ST7735_PlotNextErase(void);

// *************** ST7735_OutChar ********************
// Output one character to the LCD
// Position determined by ST7735_SetCursor command
// Color set by ST7735_SetTextColor
// Inputs: 8-bit ASCII character
// Outputs: none
void ST7735_OutChar(char ch);

//********ST7735_OutString*****************
// Print a string of characters to the ST7735 LCD.
// Position determined by ST7735_SetCursor command
// Color set by ST7735_SetTextColor
// The string will not automatically wrap.
// inputs: ptr  pointer to NULL-terminated ASCII string
// outputs: none
void ST7735_OutString(char* ptr);

// ************** ST7735_SetTextColor ************************
// Sets the color in which the characters will be printed
// Background color is fixed at black
// Input:  16-bit packed color
// Output: none
// ********************************************************
void ST7735_SetTextColor(uint16_t color);
