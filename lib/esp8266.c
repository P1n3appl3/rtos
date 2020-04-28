// esp8266.c
// Driver for ESP8266 module to act as a WiFi client or server
// Currently restricted to one incoming or outgoing connection at a time
//
// Steven Prickett (steven.prickett@gmail.com)
// Modified version by Dung Nguyen, Wally Guzman
// Modified by Jonathan Valvano, March 28, 2017
// Conolidated by Andreas Gerstlauer, April 6, 2020

/*
  THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
  OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
  VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
  OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
*/

// NOTE: ESP8266 resources below:
// General info and AT commands: http://nurdspace.nl/ESP8266
// General info and AT commands: http://www.electrodragon.com/w/Wi07c
// Community forum: http://www.esp8266.com/
// Offical forum: http://bbs.espressif.com/
// Example: http://zeflo.com/2014/esp8266-weather-display/
// Flashing: http://www.xess.com/blog/esp8266-reflash/

/* Hardware connections
 Vcc is a separate regulated 3.3V supply with at least 215mA
 /------------------------------\
 |              chip      1   8 |
 | Ant                    2   7 |
 | enna       processor   3   6 |
 |                        4   5 |
 \------------------------------/
 Connects (#define below) TM4C123 on either UART1 (PB) or UART2 (PD)
 Reset (#define below) on either PB5 or PB1

 ESP8266    TM4C123
  1 URxD    PB1/PD7   UART out of TM4C123, 115200 baud
  2 GPIO0             +3.3V for normal operation (ground to flash)
  3 GPIO2             +3.3V
  4 GND     Gnd       GND (70mA)
  5 UTxD    PB0/PD6   UART out of ESP8266, 115200 baud
  6 Ch_PD             chip select, 10k resistor to 3.3V
  7 Reset   PB5/PB1   TM4C123 can issue output low to cause hardware reset
  8 Vcc               regulated 3.3V supply with at least 70mA
 */

#include "esp8266.h"
#include "FIFO.h"
#include "WifiSettings.h" // access point parameters
#include "interrupts.h"
#include "io.h"
#include "printf.h"
#include "std.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_gpio.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/hw_types.h"
#include "tivaware/hw_uart.h"
#include "tivaware/pin_map.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include <stdbool.h>
#include <stdint.h>

/*
===========================================================
==========          CONSTANTS                    ==========
===========================================================
*/

// #define ESP8266_PB_RST   5  // PB5
#define ESP8266_PB_RST 1 // PB1

// #define ESP8266_UART     1   // UART1: PB1/PB0
#define ESP8266_UART 2 // UART2: PD7/PB6

// #define USE_UART_DRV  // Use external UART driver (only works with UART1)

#define FIFOSIZE 1024 // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1 // return value on success
#define FIFOFAIL 0    // return value on failure

#define MAXTRY 1 // number of attempts to send command

// ESP responses
static const char ESP8266_READY_RESPONSE[] = "\r\nready\r\n";
static const char ESP8266_OK_RESPONSE[] = "\r\nOK\r\n";
static const char ESP8266_ERROR_RESPONSE[] = "\r\nERROR\r\n";
static const char ESP8266_FAIL_RESPONSE[] = "\r\nFAIL\r\n";
static const char ESP8266_SENDOK_RESPONSE[] = "\r\nSEND OK\r\n";

/*
=============================================================
==========            GLOBAL VARIABLES             ==========
=============================================================
*/

// Globals for UART driver
bool ESP8266_EchoResponse = false;
bool ESP8266_EchoCommand = false;

// UART receive control/data & transmit FIFOs (see FIFO.h)
ADDFIFOSYNC(esprx, FIFOSIZE, char)
ADDFIFOSYNC(esprx0, FIFOSIZE, char)
#ifndef USE_UART_DRV
ADDFIFOSYNC(esptx, FIFOSIZE, char)
#endif

// ESP8266 state
uint16_t ESP8266_Server = 0; // server port, if any
uint16_t ESP8266_ConnectionMux = 0;
uint16_t ESP8266_Segment = 0; // last segment ID for buffered send

/*
=======================================================================
==========              helper FUNCTIONS                     ==========
=======================================================================
*/

// make letter lower-case
char lc(char letter) {
    if ((letter >= 'A') && (letter <= 'Z'))
        letter |= 0x20;
    return letter;
}

//---------DelayMs-----
// Busy wait n milliseconds
// Input: time to wait in msec
// Outputs: none
void DelayMs(uint32_t n) {
    volatile uint32_t time;
    while (n) {
        time = 6665; // 1msec, tuned at 80 MHz
        while (time) { time--; }
        n--;
    }
}

const char ReceiveDataSearchString[] = "+IPD,";
static uint32_t ReceiveDataSearchIndex = 0;
static uint32_t ReceiveDataState = 1;  // 0 to disable filtering
static uint32_t ReceiveDataStream = 0; // connection ID for received data
static uint32_t ReceiveDataLen = 0;    // receive data packet remaining size
volatile uint32_t ESP8266_DataAvailable =
    0;                                  // received data (about to be) available
volatile uint32_t ESP8266_DataLoss = 0; // lost data (for debugging)

//-------------------ReceiveDataFilter -------------------
// State machine to filter out received data stream from UART Rx input
// Inputs: character to check
// Outputs: true if data was filtered out, false otherwise
bool ReceiveDataFilter(char letter) {
    switch (ReceiveDataState) { // Filter FSM
    case 4: // filter out data and put it into the right receive FIFO
        if (ReceiveDataLen) {
            switch (ReceiveDataStream) {
            case 0:
                if (esprx0fifo_put(letter) == FIFOFAIL) { // overflow, data loss
                    ESP8266_DataAvailable--;
                    ESP8266_DataLoss++;
                }
                break;
                // only one connection currently supported
            }
            ReceiveDataLen--;
            return true;
        }
        ReceiveDataState = 1; // restart
        // fall through to start searching again if data is done
    case 1: // Look for +IPD
        if (letter ==
            ReceiveDataSearchString[ReceiveDataSearchIndex]) { // match letter?
            ReceiveDataSearchIndex++;
            if (ReceiveDataSearchString[ReceiveDataSearchIndex] ==
                0) { // end of match string?
                if (ESP8266_ConnectionMux) {
                    ReceiveDataState = 2;
                } else {
                    ReceiveDataState = 3;
                }
                ReceiveDataSearchIndex = 0;
                ReceiveDataStream = 0;
                ReceiveDataLen = 0;
            }
        } else {
            ReceiveDataSearchIndex = 0; // start over
        }
        return false;
    case 2: // look for connection ID (separated by comma)
        if (letter >= '0' && letter <= '9') {
            ReceiveDataStream =
                ReceiveDataStream * 10 + (letter - '0'); // add digit to ID
        } else if (letter == ',') {
            ReceiveDataState = 3; // ID completed, move on
        } else {
            ReceiveDataStream = 0;
            ReceiveDataState = 1; // error, start over
        }
        return false;
    case 3: // look for receive data size (separated by colon)
        if (letter >= '0' && letter <= '9') {
            ReceiveDataLen =
                ReceiveDataLen * 10 + (letter - '0'); // add digit to length
        } else if (letter == ':') {
            ReceiveDataState = 4; // size complete, move on
            ESP8266_DataAvailable +=
                ReceiveDataLen; // mark as becoming available
        } else {
            ReceiveDataStream = 0;
            ReceiveDataLen = 0;
            ReceiveDataState = 1; // error, start over
        }
        return false;
    }
    return false;
}

/*
======================================================================
==========         UART and private FUNCTIONS               ==========
======================================================================
*/

// Preprocessor magic to construct UARTx_ identifiers
#define STR(x) #x
#define CONCAT(x, y, z) x##y##z

#define UART_STR(uart) "UART" STR(uart)
#define UART_NAME(uart, identifier) CONCAT(UART, uart, identifier)

#define UART_ESP8266(identifier) HWREG(UART2_BASE + UART_##identifier)

//------------------- ESP8266InitUART-------------------
// Intializes uart needed to communicate with esp8266
// Configure UART for 115200bps operation
// Inputs: RX and/or TX echo for debugging
// Outputs: none
#define UART_LCRH_WLEN_8 0x00000060 // 8 bit word length
#define UART_LCRH_FEN 0x00000010    // UART Enable FIFOs
#define UART_IFLS_RX1_8 0x00000000  // RX FIFO >= 1/8 full
#define UART_IFLS_TX1_8 0x00000000  // TX FIFO <= 1/8 full
#define UART_IM_RXIM 0x00000010     // UART Receive Interrupt Mask
#define UART_IM_TXIM 0x00000020     // UART Transmit Interrupt Mask
#define UART_IM_RTIM 0x00000040     // UART Receive Time-Out Interrupt
#define UART_CTL_UARTEN 0x00000001  // UART Enable
#define UART_CTL_RXE 0x00000200     // UART Receive Enable
#define UART_CTL_TXE 0x00000100     // UART Transmit Enable
void ESP8266_InitUART(int rx_echo, int tx_echo) {
    esptxfifo_init();
    esprxfifo_init();
    esprx0fifo_init();
    ESP8266_EchoResponse = rx_echo;
    ESP8266_EchoCommand = tx_echo;
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_UART2)) {}
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY; // unlock port d
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= 0xFF; // allow changes to PD7
    ROM_GPIOPinConfigure(GPIO_PD6_U2RX);
    ROM_GPIOPinConfigure(GPIO_PD7_U2TX);
    ROM_GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    // Initialize the UART. Set the baud rate, number of data bit
    UART_ESP8266(O_CTL) &=
        ~UART_CTL_UARTEN; // Clear UART enable bit during config
    UART_ESP8266(O_IBRD) = (80000000 / 16) / BAUDRATE;
    UART_ESP8266(O_FBRD) =
        ((64 * ((80000000 / 16) % BAUDRATE)) + BAUDRATE / 2) / BAUDRATE;
    // UART_ESP8266(_IBRD_R) = 43;   // IBRD = int(80,000,000 / (16 * 115,200))
    // = int(43.403) UART_ESP8266(_FBRD_R) = 26;   // FBRD = round(0.4028 * 64 )
    // = 26
    UART_ESP8266(O_LCRH) =
        (UART_LCRH_WLEN_8 |
         UART_LCRH_FEN); // 8 bit word length, 1 stop, no parity, FIFOs enabled
    UART_ESP8266(O_IFLS) &=
        ~0x3F; // Clear TX and RX interrupt FIFO level fields
    UART_ESP8266(O_IFLS) +=
        (UART_IFLS_TX1_8 |
         UART_IFLS_RX1_8); // RX and TX FIFO interrupt threshold >= 1/8th full
    UART_ESP8266(O_IM) |=
        (UART_IM_RXIM | UART_IM_TXIM |
         UART_IM_RTIM); // Enable interupt on TX, RX and RX transmission end
    UART_ESP8266(O_CTL) |=
        (UART_CTL_UARTEN | UART_CTL_RXE | UART_CTL_TXE); // Set UART enable bit
}

//--------ESP8266_EnableInterrupt--------
// enables uart interrupt
// Inputs: none
// Outputs: none
void ESP8266_EnableInterrupt(void) {
    ROM_IntEnable(INT_UART2);
    ROM_IntPrioritySet(INT_UART2, 4 << 5);
}

//--------ESP8266_DisableInterrupt--------
// disables uart interrupt
// Inputs: none
// Outputs: none
void ESP8266_DisableInterrupt(void) {
    ROM_IntDisable(INT_UART2);
}

//----------ESP8266BufferToTx----------
// Copies TX buffer (software defined FIFO) to uart
// Inputs: none
// Outputs:none
#define UART_FR_TXFF 0x00000020 // UART Transmit FIFO Full
void static ESP8266BufferToTx(void) {
    char letter;
    while (((UART_ESP8266(O_FR) & UART_FR_TXFF) == 0) &&
           (esptxfifo_size() > 0)) {
        esptxfifo_get(&letter);
        if (ESP8266_EchoCommand) {
            putchar(letter);
        }
        UART_ESP8266(O_DR) = letter;
    }
}

//----------ESP8266RxToBuffer----------
// Copies uart fifo to RX buffer (software defined FIFO)
// Inputs: none
// Outputs:none
#define UART_FR_RXFE 0x00000010 // UART Receive FIFO Empty
void static ESP8266RxToBuffer(void) {
    char letter;
    while (((UART_ESP8266(O_FR) & UART_FR_RXFE) == 0) &&
           (esprxfifo_size() < (FIFOSIZE - 1))) {
        letter = UART_ESP8266(O_DR);
        if (ESP8266_EchoResponse) {
            putchar(letter);
        }
        if (!ReceiveDataFilter(letter)) {
            esprxfifo_put(letter);
        }
    }
}

//----------UART_Handler----------
// at least one of three things has happened:
// hardware TX FIFO goes from 3 to 2 or less items
// hardware RX FIFO goes from 1 to 2 or more items
// UART receiver has timed out
#define UART_RIS_TXRIS 0x00000020 // UART Transmit Raw Interrupt
#define UART_ICR_TXIC 0x00000020  // Transmit Interrupt Clear
#define UART_ICR_RXIC 0x00000010  // Receive Interrupt Clear
#define UART_RIS_RXRIS 0x00000010 // UART Receive Raw Interrupt
#define UART_RIS_RTRIS 0x00000040 // UART Receive Time-Out Raw
void uart2_handler(void) {
    if (UART_ESP8266(O_RIS) & UART_RIS_TXRIS) { // hardware TX FIFO <= 2 items
        UART_ESP8266(O_ICR) = UART_ICR_TXIC;    // acknowledge TX FIFO
        ESP8266BufferToTx();
        if (esptxfifo_size() == 0) {             // software TX FIFO is empty
            UART_ESP8266(O_IM) &= ~UART_IM_TXIM; // disable TX FIFO interrupt
        }
    }
    if (UART_ESP8266(O_RIS) & UART_RIS_RXRIS) { // hardware RX FIFO >= 2 items
        UART_ESP8266(O_ICR) = UART_ICR_RXIC;    // acknowledge RX FIFO
        ESP8266RxToBuffer();
    }
    if (UART_ESP8266(O_RIS) & UART_RIS_RTRIS) { // receiver timed out
        UART_ESP8266(O_ICR) = UART_ICR_RTIC;    // acknowledge receiver time out
        ESP8266RxToBuffer();
    }
}

//--------ESP8266_OutChar--------
// prints a character to the esp8226 via uart
// Inputs: character to transmit
// Outputs: none
void ESP8266_OutChar(char data) {
    while (esptxfifo_put(data) == FIFOFAIL) {};
    UART_ESP8266(O_IM) &= ~UART_IM_TXIM; // disable TX FIFO interrupt
    ESP8266BufferToTx();
    UART_ESP8266(O_IM) |= UART_IM_TXIM; // enable TX FIFO interrupt
}

//--------ESP8266_InChar--------
// read a character from the esp8226 via uart
// Inputs: none
// Outputs: character received
char ESP8266_InChar(void) {
    char letter;
    while (esprxfifo_get(&letter) == FIFOFAIL) {};
    return (letter);
}

//---------ESP8266_SendCommand-----
// Sends a string to the esp8266 module
// Inputs: string to send (null-terminated)
// Outputs: none
void ESP8266_SendCommand(const char* command) {
    int index = 0;
    while (command[index] != 0) { ESP8266_OutChar(command[index++]); }
}

//---------ESP8266_WaitForResponse-----
// Busy-wait until response found
// Inputs: Success or failure strings to search for
// Outputs: 1 on success, 0 on failure
int ESP8266_WaitForResponse(const char* success, const char* failure) {
    char d;
    const char* s = success;
    const char* f = failure;
    while ((!s || *s) && (!f || *f)) { // end of search string reached?
        d = ESP8266_InChar();
        if (s && (d == *s)) {
            s++;
        } else {
            s = success; // start over
            if (s && (d == *s))
                s++; // match first char?
        }
        if (f && (d == *f)) {
            f++;
        } else {
            f = failure; // start over
            if (f && (d == *f))
                f++; // match first char?
        }
    }
    if (failure && !(*f))
        return FAILURE;
    return SUCCESS;
}

/*
=======================================================================
==========          ESP8266 PUBLIC FUNCTIONS                 ==========
=======================================================================
*/

//-------------------ESP8266_Init --------------
// Initializes the module
// Inputs: RX and/or TX echo for debugging
// Outputs: 1 for success, 0 for failure (no ESP detected)
int ESP8266_Init(int rx_echo, int tx_echo) {
    char c;
    const char* s;
    uint32_t timer = 1;
    // Disable interrupt during initialization
    ESP8266_DisableInterrupt();

    // Initialize UART to communicate with ESP
    ESP8266_InitUART(rx_echo, tx_echo);

    // Initialize reset port
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_1);
    // GPIO_PORTB_PCTL_R = GPIO_PORTB_PCTL_R & ~(0x0F << (ESP8266_PB_RST * 4));

    // Hard reset
    ROM_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, 0); // reset low
    DelayMs(10);
    ROM_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, GPIO_PIN_1); // reset high

    // Wait for ready status with timeout
    // Use low-level UART communication, interrupts disabled
    s = ESP8266_READY_RESPONSE;
    timer = 5000000; // around a 1-2s total timeout
    while (*s) {
        while (timer && ((UART_ESP8266(O_FR) & UART_FR_RXFE) != 0)) { timer--; }
        if (!timer)
            break;
        c = (char)(UART_ESP8266(O_DR) & 0xFF);
        if (rx_echo)
            putchar(c); // echo, requires UART0 to be operational,
                        // will print garbage
        if (c == *s) {
            s++;
        } else {
            s = ESP8266_READY_RESPONSE;
            if (c == *s)
                s++;
        }
    }

    // Finally enable interrupt
    ESP8266_EnableInterrupt();

    if (!timer)
        return FAILURE;
    return SUCCESS;
}

//-------------------ESP8266_Connect --------------
// Bring interface up and connect to Wifi
// Inputs: enable debug output
// Outputs: 1 on success, 0 on failure
int ESP8266_Connect(int verbose) {
    if (ESP8266_Reset() == FAILURE)
        return FAILURE;

    if (verbose) // debug output: MAC address oF ESP8266
        if (ESP8266_GetMACAddress() == FAILURE)
            return FAILURE;

            // if (verbose) // debug output: see APs in area
            //     if (ESP8266_ListAccessPoints() == FAILURE)
            //         return FAILURE;

#if SOFTAP
    if (ESP8266_SoftAccessPoint(SSID_NAME, PASSKEY) == FAILURE)
        return FAILURE;
    if (ESP8266_SetWifiMode(ESP8266_WIFI_MODE_AP) == FAILURE)
        return FAILURE;
#else
    if (ESP8266_SetWifiMode(ESP8266_WIFI_MODE_CLIENT) == FAILURE)
        return FAILURE;
    if (ESP8266_JoinAccessPoint(SSID_NAME, PASSKEY) == FAILURE)
        return FAILURE;
#endif

    if (verbose) // debug output: our IP address
        if (ESP8266_GetIPAddress() == FAILURE)
            return FAILURE;

    return SUCCESS;
}

//-------------------ESP8266_StartServer --------------
// Start server on specific port
// Inputs: port and server timeout
// Outputs: 1 on success, 0 on failure
int ESP8266_StartServer(uint16_t port, uint16_t timeout) {
    if (ESP8266_SetConnectionMux(1) == FAILURE)
        return FAILURE;
    if (ESP8266_EnableServer(port) == FAILURE)
        return FAILURE;
    if (ESP8266_SetServerTimeout(timeout) == FAILURE)
        return FAILURE;
    return SUCCESS;
}

//-------------------ESP8266_StopServer --------------
// Stop server and set to single-client mode
// Inputs: none
// Outputs: 1 on success, 0 on failure
int ESP8266_StopServer(void) {
    if (ESP8266_DisableServer() == FAILURE)
        return FAILURE;
    if (ESP8266_SetConnectionMux(0) == FAILURE)
        return FAILURE;
    return SUCCESS;
}

//----------ESP8266_Reset------------
// soft resets the esp8266 module
// input:  none
// output: 1 if success, 0 if fail
int ESP8266_Reset(void) {
    int try
        = MAXTRY;
    while (try) {
        ESP8266_SendCommand("AT+RST\r\n");
        if (ESP8266_WaitForResponse(ESP8266_READY_RESPONSE, 0))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

//---------ESP8266_Restore-----
// restore the ESP8266 module to default values
// Inputs: none
// Outputs: 1 if success, 0 if fail
int ESP8266_Restore(void) {
    int try
        = MAXTRY;
    while (try) {
        ESP8266_SendCommand("AT+RESTORE\r\n");
        if (ESP8266_WaitForResponse(ESP8266_READY_RESPONSE, 0))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

//---------ESP8266_GetVersionNumber----------
// get status
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_GetVersionNumber(void) {
    int try
        = MAXTRY;
    while (try) {
        ESP8266_SendCommand("AT+GMR\r\n");
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0))
            return SUCCESS;
        try
            --;
    }
    return 0; // fail
}

//---------ESP8266_GetMACAddress----------
// get MAC address
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_GetMACAddress(void) {
    int try
        = MAXTRY;
    while (try) {
        ESP8266_SendCommand("AT+CIPSTAMAC?\r\n");
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0))
            return SUCCESS;
        try
            --;
    }
    return 0; // fail
}

//---------ESP8266_SetWifiMode----------
// configures the esp8266 to operate as a wifi client, access point, or both
// Input: mode accepts ESP8266_WIFI_MODE constants
// output: 1 if success, 0 if fail
int ESP8266_SetWifiMode(uint8_t mode) {
    int try
        = MAXTRY;
    char TXBuffer[32];
    while (try) {
        sprintf(TXBuffer, "AT+CWMODE=%d\r\n", mode);
        ESP8266_SendCommand(TXBuffer);
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

//---------ESP8266_SetConnectionMux----------
// enables the esp8266 connection mux, required for starting tcp server
// Input: 0 (single) or 1 (multiple)
// output: 1 if success, 0 if fail
int ESP8266_SetConnectionMux(uint8_t enabled) {
    int try
        = MAXTRY;
    char TXBuffer[32];
    while (try) {
        sprintf(TXBuffer, "AT+CIPMUX=%d\r\n", enabled);
        ESP8266_SendCommand(TXBuffer);
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0)) {
            ESP8266_ConnectionMux = enabled;
            return SUCCESS;
        }
        try
            --;
    }
    return FAILURE;
}

//---------ESP8266_ListAccessPoints----------
// lists available wifi access points
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_ListAccessPoints(void) {
    int try
        = MAXTRY;
    while (try) {
        ESP8266_SendCommand("AT+CWLAP\r\n");
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE,
                                    ESP8266_ERROR_RESPONSE))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

//----------ESP8266_JoinAccessPoint------------
// joins a wifi access point using specified ssid and password
// input:  SSID and PASSWORD
// output: 1 if success, 0 if fail
int ESP8266_JoinAccessPoint(const char* ssid, const char* password) {
    int try
        = MAXTRY;
    while (try) {
        ESP8266_SendCommand("AT+CWJAP=\"");
        ESP8266_SendCommand(ssid);
        ESP8266_SendCommand("\",\"");
        ESP8266_SendCommand(password);
        ESP8266_SendCommand("\"\r\n");
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, ESP8266_FAIL_RESPONSE))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

// ----------ESP8266_QuitAccessPoint-------------
// disconnects from currently connected wifi access point
// Inputs: none
// Outputs: 1 if success, 0 if fail
int ESP8266_QuitAccessPoint(void) {
    int try
        = MAXTRY;
    while (try) {
        ESP8266_SendCommand("AT+CWQAP\r\n");
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

//----------ESP8266_ConfigureAccessPoint------------
// configures esp8266 wifi soft access point settings
// use this function only when in AP mode (and not in client mode)
// input:  SSID, Password, channel, security
// output: 1 if success, 0 if fail
int ESP8266_ConfigureAccessPoint(const char* ssid, const char* password,
                                 uint8_t channel, uint8_t encryptMode) {
    int try
        = MAXTRY;
    char TXBuffer[32];
    while (try) {
        ESP8266_SendCommand("AT+CWSAP=\"");
        ESP8266_SendCommand(ssid);
        ESP8266_SendCommand("\",\"");
        ESP8266_SendCommand(password);
        sprintf(TXBuffer, "\",%d,%d\r\n", channel, encryptMode);
        ESP8266_SendCommand(TXBuffer);
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE,
                                    ESP8266_ERROR_RESPONSE))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

//---------ESP8266_GetIPAddress----------
// Get local IP address
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_GetIPAddress(void) {
    int try
        = MAXTRY;
    while (try) {
        ESP8266_SendCommand("AT+CIFSR\r\n");
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE,
                                    ESP8266_ERROR_RESPONSE))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

//---------ESP8266_MakeTCPConnection----------
// Establish TCP connection
// Inputs: IP address or web page as a string, port, and keepalive time (0 if
// none) output: 1 if success, 0 if fail
int ESP8266_MakeTCPConnection(char* IPaddress, uint16_t port,
                              uint16_t keepalive) {
    int try
        = MAXTRY;
    char TXBuffer[32];
    while (try) {
        ESP8266_SendCommand("AT+CIPSTART=\"TCP\",\"");
        ESP8266_SendCommand(IPaddress);
        if (keepalive) {
            sprintf(TXBuffer, "\",%d\r\n", port);
        } else {
            sprintf(TXBuffer, "\",%d\r\n", port);
        }
        ESP8266_SendCommand(TXBuffer); // open and connect to a socket
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE,
                                    ESP8266_ERROR_RESPONSE))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

//---------ESP8266_Send----------
// Send a string to server
// Input: payload to send
// output: 1 if success, 0 if fail
int ESP8266_Send(const char* fetch) {
    char TXBuffer[32];
    if (ESP8266_ConnectionMux) {
        sprintf(TXBuffer, "AT+CIPSEND=%d,%d\r\n", 0, strlen(fetch));
    } else {
        sprintf(TXBuffer, "AT+CIPSEND=%d\r\n", strlen(fetch));
    }
    ESP8266_SendCommand(TXBuffer);
    if (!ESP8266_WaitForResponse(">", ESP8266_ERROR_RESPONSE))
        return FAILURE;
    ESP8266_SendCommand(fetch);
    if (ESP8266_WaitForResponse(ESP8266_SENDOK_RESPONSE,
                                ESP8266_ERROR_RESPONSE))
        return SUCCESS;
    return FAILURE;
}

//---------ESP8266_SendBuffered----------
// Send a string to server using ESP TCP-send buffer
// Input: payload to send
// output: 1 if success, 0 if fail
int ESP8266_SendBuffered(const char* fetch) {
    char TXBuffer[32];
    if (ESP8266_ConnectionMux) {
        sprintf(TXBuffer, "AT+CIPSENDBUF=%d,%d\r\n", 0, strlen(fetch));
    } else {
        sprintf(TXBuffer, "AT+CIPSENDBUF=%d\r\n", strlen(fetch));
    }
    ESP8266_SendCommand(TXBuffer);
    if (!ESP8266_WaitForResponse(">", ESP8266_ERROR_RESPONSE))
        return FAILURE;
    ESP8266_Segment++;
    ESP8266_SendCommand(fetch);
    sprintf(TXBuffer, "Recv %d bytes", strlen(fetch));
    if (ESP8266_WaitForResponse(TXBuffer, ESP8266_ERROR_RESPONSE))
        return SUCCESS;
    return FAILURE;
}

int ESP8266_SendMessage(const char* fetch) {
    uint8_t len = strlen(fetch);
    char* fetch_append = (char*)fetch;
    fetch_append[len++] = EOT;
    fetch_append[len] = '\0';
    char TXBuffer[32];
    if (ESP8266_ConnectionMux) {
        sprintf(TXBuffer, "AT+CIPSENDBUF=%d,%d\r\n", 0, strlen(fetch));
    } else {
        sprintf(TXBuffer, "AT+CIPSENDBUF=%d\r\n", strlen(fetch));
    }
    ESP8266_SendCommand(TXBuffer);
    if (!ESP8266_WaitForResponse(">", ESP8266_ERROR_RESPONSE))
        return FAILURE;
    ESP8266_Segment++;
    ESP8266_SendCommand(fetch);
    sprintf(TXBuffer, "Recv %d bytes", strlen(fetch));
    if (ESP8266_WaitForResponse(TXBuffer, ESP8266_ERROR_RESPONSE))
        return SUCCESS;
    return FAILURE;
}

//---------ESP8266_SendBufferedStatus----------
// Check status of last buffered segment
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_SendBufferedStatus(void) {
    char OKBuffer[16];
    char FailBuffer[24];
    if (ESP8266_ConnectionMux) {
        sprintf(OKBuffer, "\n%d,%d,SEND OK\r\n", 0, ESP8266_Segment);
        sprintf(FailBuffer, "\n%d,%d,SEND FAIL\r\n", 0, ESP8266_Segment);
    } else {
        sprintf(OKBuffer, "\n%d,SEND OK\r\n", ESP8266_Segment);
        sprintf(FailBuffer, "\n%d,SEND FAIL\r\n", ESP8266_Segment);
    }
    if (ESP8266_WaitForResponse(OKBuffer, FailBuffer))
        return SUCCESS;
    return FAILURE;
}

//---------ESP8266_Receive----------
// Receive a string from server
// Reads from data input until end of line or max length is reached
// Input: buffer and max length
// Output: 1 and null-terminated string if success, 0 if fail (disconnected)
int ESP8266_Receive(char* fetch, uint32_t max) {
    long sr;
    const char* s;
    char letter;
    while (max > 1) {
        if (esprx0fifo_size() ||
            ESP8266_DataAvailable) { // data (about to be) available?
            while (esprx0fifo_get(&letter) == FIFOFAIL) {};
            // ESP8266_DisableInterrupt();  // critical section
            sr = start_critical();
            if (ESP8266_DataAvailable)
                ESP8266_DataAvailable--;
            end_critical(sr);
            // ESP8266_EnableInterrupt();
            if (letter == '\r')
                continue;
            if (letter == '\n')
                break;
            *fetch = letter;
            fetch++;
            max--;
        } else { // wait for next packet or connection close
            if (ESP8266_ConnectionMux) {
                s = "0,CLOSED";
            } else {
                s = "CLOSED";
            }
            if (!ESP8266_WaitForResponse(ReceiveDataSearchString, s)) {
                *fetch = 0; // connection closed
                return FAILURE;
            }
            while (ESP8266_InChar() != ':') {
            } // wait for DataAvailable to be updated
        }
    }
    *fetch = 0; // terminate with null character
    return SUCCESS;
}

int ESP8266_ReceiveMessage(char* fetch, uint32_t max) {
    long sr;
    const char* s;
    char letter;
    while (max > 1) {
        if (esprx0fifo_size() ||
            ESP8266_DataAvailable) { // data (about to be) available?
            while (esprx0fifo_get(&letter) == FIFOFAIL) {};
            // ESP8266_DisableInterrupt();  // critical section
            sr = start_critical();
            if (ESP8266_DataAvailable)
                ESP8266_DataAvailable--;
            end_critical(sr);
            // ESP8266_EnableInterrupt();
            if (letter == EOT)
                break;
            *fetch = letter;
            fetch++;
            max--;
        } else { // wait for next packet or connection close
            if (ESP8266_ConnectionMux) {
                s = "0,CLOSED";
            } else {
                s = "CLOSED";
            }
            if (!ESP8266_WaitForResponse(ReceiveDataSearchString, s)) {
                *fetch = 0; // connection closed
                return FAILURE;
            }
            while (ESP8266_InChar() != ':') {
            } // wait for DataAvailable to be updated
        }
    }
    *fetch = 0; // terminate with null character
    return SUCCESS;
}

//---------ESP8266_CloseTCPConnection----------
// Close TCP connection
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_CloseTCPConnection(void) {
    int try
        = MAXTRY;
    while (try) {
        if (ESP8266_ConnectionMux) {
            ESP8266_SendCommand("AT+CIPCLOSE=0\r\n");
        } else {
            ESP8266_SendCommand("AT+CIPCLOSE\r\n");
        }
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE,
                                    ESP8266_ERROR_RESPONSE)) {
            esprx0fifo_init(); // Discard any data
            return SUCCESS;
        }
        try
            --;
    }
    return FAILURE;
}

//---------ESP8266_SetDataTransmissionMode----------
// set data transmission passthrough mode
// Input: 0 not data mode, 1 data mode; return "Link is builded"
// output: 1 if success, 0 if fail
int ESP8266_SetDataTransmissionMode(uint8_t mode) {
    int try
        = MAXTRY;
    char TXBuffer[32];
    while (try) {
        sprintf(TXBuffer, "AT+CIPMODE=%d\r\n", mode);
        ESP8266_SendCommand(TXBuffer);
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

//---------ESP8266_GetStatus----------
// get network connection status
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_GetStatus(void) {
    int try
        = MAXTRY;
    while (try) {
        ESP8266_SendCommand("AT+CIPSTATUS\r\n");
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

// --------ESP8266_EnableServer------------------
// enables tcp server on specified port
// Inputs: port number
// Outputs: 1 if success, 0 if fail
int ESP8266_EnableServer(uint16_t port) {
    int try
        = MAXTRY;
    char TXBuffer[32];
    while (try) {
        sprintf(TXBuffer, "AT+CIPSERVER=1,%d\r\n", port);
        ESP8266_SendCommand(TXBuffer);
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0)) {
            ESP8266_Server = port;
            return SUCCESS;
        }
        try
            --;
    }
    return FAILURE;
}

// ----------ESP8266_SetServerTimeout--------------
// set connection timeout for tcp server, 0-28800 seconds
// Inputs: timeout parameter
// Outputs: 1 if success, 0 if fail
int ESP8266_SetServerTimeout(uint16_t timeout) {
    int try
        = MAXTRY;
    char TXBuffer[32];
    while (try) {
        sprintf(TXBuffer, "AT+CIPSTO=%d\r\n", timeout);
        ESP8266_SendCommand(TXBuffer);
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE,
                                    ESP8266_ERROR_RESPONSE))
            return SUCCESS;
        try
            --;
    }
    return FAILURE;
}

// ----------ESP8266_WaitForConnection--------------
// wait for incoming connection on server
// must ensure that no other ESP calls are done while waiting
// this should really be done in the background interrupt handler
// using a mailbox to communicate with this function
// Inputs: none
// Outputs: 1 if success, 0 if fail
int ESP8266_WaitForConnection(void) {
    if (!ESP8266_ConnectionMux)
        return FAILURE;
    if (!ESP8266_Server)
        return FAILURE;
    if (ESP8266_WaitForResponse("0,CONNECT\r\n", 0))
        return SUCCESS;
    return FAILURE;
}

//---------ESP8266_DisableServer----------
// disables tcp server
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_DisableServer(void) {
    int try
        = MAXTRY;
    while (try) {
        ESP8266_SendCommand("AT+CIPSERVER=0\r\n");
        if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0)) {
            ESP8266_Server = 0;
            return SUCCESS;
        }
        try
            --;
    }
    return FAILURE;
}
