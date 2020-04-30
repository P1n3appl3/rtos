// Driver for ESP8266 module to act as a WiFi client or server
// Currently restricted to one incoming or outgoing connection at a time
// ESP8266 resources below:
// General info and AT commands: http://nurdspace.nl/ESP8266
// General info and AT commands: http://www.electrodragon.com/w/Wi07c
// Vcc is a separate regulated 3.3V supply with at least 215mA
// /------------------------------\
// |              chip      1   8 |
// | Ant                    2   7 |
// | enna       processor   3   6 |
// |                        4   5 |
// \------------------------------/
// Connects (#define below) TM4C123 on either UART1 (PB) or UART2 (PD)
// Reset (#define below) on either PB5 or PB1

// ESP8266    TM4C123
//  1 URxD    PD7       UART out of TM4C123, 115200 baud
//  2 GPIO0             +3.3V for normal operation (ground to flash)
//  3 GPIO2             +3.3V
//  4 GND     Gnd       GND (70mA)
//  5 UTxD    PD6       UART out of ESP8266, 115200 baud
//  6 Ch_PD             chip select, 10k resistor to 3.3V
//  7 Reset   PB1       TM4C123 can issue output low to cause hardware reset
//  8 Vcc               regulated 3.3V supply with at least 70mA

#include "esp8266.h"
#include "FIFO.h"
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

#ifndef PASSKEY
#define PASSKEY ""
#endif
#ifndef SSID_NAME
#define SSID_NAME ""
#endif

#define ESP8266_PB_RST 1 // PB1
#define ESP8266_UART 2   // UART2: PD7/PB6

#define FIFOSIZE 1024 // size of the FIFOs (must be power of 2)

// ESP responses
static const char* ESP8266_READY_RESPONSE = "\r\nready\r\n";
static const char* ESP8266_OK_RESPONSE = "\r\nOK\r\n";
static const char* ESP8266_ERROR_RESPONSE = "\r\nERROR\r\n";
static const char* ESP8266_FAIL_RESPONSE = "\r\nFAIL\r\n";
static const char* ESP8266_SENDOK_RESPONSE = "\r\nSEND OK\r\n";

// Globals for UART driver
bool ESP8266_EchoResponse = false;
bool ESP8266_EchoCommand = false;

// UART receive control/data & transmit FIFOs (see FIFO.h)
ADDFIFOSYNC(esprx, FIFOSIZE, char)
ADDFIFOSYNC(esprx0, FIFOSIZE, char)
ADDFIFOSYNC(esptx, FIFOSIZE, char)

// ESP8266 state
uint16_t ESP8266_Server = 0; // server port, if any
uint16_t ESP8266_ConnectionMux = 0;
uint16_t ESP8266_Segment = 0; // last segment ID for buffered send

const char* ReceiveDataSearchString = "+IPD,";
static uint32_t ReceiveDataSearchIndex = 0;
static uint32_t ReceiveDataState = 1;  // 0 to disable filtering
static uint32_t ReceiveDataStream = 0; // connection ID for received data
static uint32_t ReceiveDataLen = 0;    // receive data packet remaining size
volatile uint32_t ESP8266_DataAvailable =
    0;                                  // received data (about to be) available
volatile uint32_t ESP8266_DataLoss = 0; // lost data (for debugging)

// State machine to filter out received data stream from UART Rx input
// Inputs: character to check
// returns true if data was filtered out, false otherwise
static bool ReceiveDataFilter(char letter) {
    switch (ReceiveDataState) { // Filter FSM
    case 4: // filter out data and put it into the right receive FIFO
        if (ReceiveDataLen) {
            switch (ReceiveDataStream) {
            case 0:
                if (!esprx0fifo_put(letter)) { // overflow, data loss
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

// Preprocessor magic to construct UARTx_ identifiers
#define STR(x) #x
#define CONCAT(x, y, z) x##y##z

#define UART_STR(uart) "UART" STR(uart)
#define UART_NAME(uart, identifier) CONCAT(UART, uart, identifier)

#define UART_ESP8266(identifier) HWREG(UART2_BASE + UART_##identifier)

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

// enables uart interrupt
void ESP8266_EnableInterrupt(void) {
    ROM_IntEnable(INT_UART2);
    ROM_IntPrioritySet(INT_UART2, 4 << 5);
}

// disables uart interrupt
void ESP8266_DisableInterrupt(void) {
    ROM_IntDisable(INT_UART2);
}

// Copies TX buffer (software defined FIFO) to uart
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

// Copies uart fifo to RX buffer (software defined FIFO)
#define UART_FR_RXFE 0x00000010 // UART Receive FIFO Empty
static void ESP8266RxToBuffer(void) {
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

static void ESP8266_OutChar(char data) {
    while (esptxfifo_put(data) == false) {};
    UART_ESP8266(O_IM) &= ~UART_IM_TXIM; // disable TX FIFO interrupt
    ESP8266BufferToTx();
    UART_ESP8266(O_IM) |= UART_IM_TXIM; // enable TX FIFO interrupt
}

static char ESP8266_InChar(void) {
    char letter;
    while (esprxfifo_get(&letter) == false) {};
    return (letter);
}

static void ESP8266_SendCommand(const char* command) {
    int index = 0;
    while (command[index]) { ESP8266_OutChar(command[index++]); }
}

bool ESP8266_WaitForResponse(const char* success, const char* failure) {
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
        return false;
    return true;
}

bool ESP8266_Init(bool rx_echo, bool tx_echo) {
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
    busy_wait(9, ms(10));
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

    return timer;
}

bool ESP8266_Connect(bool verbose) {
    return ESP8266_Reset() && (verbose ? ESP8266_GetMACAddress() : true) &&
           (verbose ? ESP8266_ListAccessPoints() : true) &&
           ESP8266_SetWifiMode(CLIENT) &&
           ESP8266_JoinAccessPoint(SSID_NAME, PASSKEY) &&
           (verbose ? ESP8266_GetIPAddress() : true);
}

bool ESP8266_StartServer(uint16_t port, uint16_t timeout) {
    return ESP8266_SetConnectionMux(1) && ESP8266_EnableServer(port) &&
           ESP8266_SetServerTimeout(timeout);
}

bool ESP8266_StopServer(void) {
    return ESP8266_DisableServer() && ESP8266_SetConnectionMux(0);
}

bool ESP8266_Reset(void) {
    ESP8266_SendCommand("AT+RST\r\n");
    return ESP8266_WaitForResponse(ESP8266_READY_RESPONSE, 0);
}

bool ESP8266_Restore(void) {
    ESP8266_SendCommand("AT+RESTORE\r\n");
    return ESP8266_WaitForResponse(ESP8266_READY_RESPONSE, 0);
}

bool ESP8266_GetVersionNumber(void) {
    ESP8266_SendCommand("AT+GMR\r\n");
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0);
}

bool ESP8266_GetMACAddress(void) {
    ESP8266_SendCommand("AT+CIPSTAMAC?\r\n");
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0);
}

bool ESP8266_SetWifiMode(uint8_t mode) {
    char TXBuffer[32];
    sprintf(TXBuffer, "AT+CWMODE=%d\r\n", mode);
    ESP8266_SendCommand(TXBuffer);
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0);
}

bool ESP8266_SetConnectionMux(bool multiple) {
    char TXBuffer[32];
    sprintf(TXBuffer, "AT+CIPMUX=%d\r\n", multiple);
    ESP8266_SendCommand(TXBuffer);
    if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0)) {
        ESP8266_ConnectionMux = multiple;
        return true;
    }
    return false;
}

bool ESP8266_ListAccessPoints(void) {
    ESP8266_SendCommand("AT+CWLAP\r\n");
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, ESP8266_ERROR_RESPONSE);
}

bool ESP8266_JoinAccessPoint(const char* ssid, const char* password) {
    ESP8266_SendCommand("AT+CWJAP=\"");
    ESP8266_SendCommand(ssid);
    ESP8266_SendCommand("\",\"");
    ESP8266_SendCommand(password);
    ESP8266_SendCommand("\"\r\n");
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, ESP8266_FAIL_RESPONSE);
}

bool ESP8266_QuitAccessPoint(void) {
    ESP8266_SendCommand("AT+CWQAP\r\n");
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0);
}

bool ESP8266_ConfigureAccessPoint(const char* ssid, const char* password,
                                  uint8_t channel, uint8_t encryptMode) {
    char TXBuffer[32];
    ESP8266_SendCommand("AT+CWSAP=\"");
    ESP8266_SendCommand(ssid);
    ESP8266_SendCommand("\",\"");
    ESP8266_SendCommand(password);
    sprintf(TXBuffer, "\",%d,%d\r\n", channel, encryptMode);
    ESP8266_SendCommand(TXBuffer);
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, ESP8266_ERROR_RESPONSE);
}

bool ESP8266_GetIPAddress(void) {
    ESP8266_SendCommand("AT+CIFSR\r\n");
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, ESP8266_ERROR_RESPONSE);
}

bool ESP8266_MakeTCPConnection(char* IPaddress, uint16_t port) {
    char TXBuffer[32];
    ESP8266_SendCommand("AT+CIPSTART=\"TCP\",\"");
    ESP8266_SendCommand(IPaddress);
    sprintf(TXBuffer, "\",%d\r\n", port);
    ESP8266_SendCommand(TXBuffer); // open and connect to a socket
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, ESP8266_ERROR_RESPONSE);
}

bool ESP8266_Send(const char* fetch) {
    char TXBuffer[32];
    if (ESP8266_ConnectionMux) {
        sprintf(TXBuffer, "AT+CIPSEND=%d,%d\r\n", 0, strlen(fetch));
    } else {
        sprintf(TXBuffer, "AT+CIPSEND=%d\r\n", strlen(fetch));
    }
    ESP8266_SendCommand(TXBuffer);
    if (!ESP8266_WaitForResponse(">", ESP8266_ERROR_RESPONSE))
        return false;
    ESP8266_SendCommand(fetch);
    return ESP8266_WaitForResponse(ESP8266_SENDOK_RESPONSE,
                                   ESP8266_ERROR_RESPONSE);
}

bool ESP8266_SendBuffered(const char* fetch) {
    char TXBuffer[32];
    if (ESP8266_ConnectionMux) {
        sprintf(TXBuffer, "AT+CIPSENDBUF=%d,%d\r\n", 0, strlen(fetch));
    } else {
        sprintf(TXBuffer, "AT+CIPSENDBUF=%d\r\n", strlen(fetch));
    }
    ESP8266_SendCommand(TXBuffer);
    if (!ESP8266_WaitForResponse(">", ESP8266_ERROR_RESPONSE))
        return false;
    ESP8266_Segment++;
    ESP8266_SendCommand(fetch);
    sprintf(TXBuffer, "Recv %d bytes", strlen(fetch));
    return ESP8266_WaitForResponse(TXBuffer, ESP8266_ERROR_RESPONSE);
}

bool ESP8266_SendMessage(const char* fetch) {
    char TXBuffer[32];
    if (ESP8266_ConnectionMux) {
        sprintf(TXBuffer, "AT+CIPSENDBUF=%d,%d\r\n", 0, strlen(fetch) + 2);
    } else {
        sprintf(TXBuffer, "AT+CIPSENDBUF=%d\r\n", strlen(fetch) + 2);
    }
    ESP8266_SendCommand(TXBuffer);
    if (!ESP8266_WaitForResponse(">", ESP8266_ERROR_RESPONSE))
        return false;
    ESP8266_Segment++;
    ESP8266_SendCommand(fetch);
    ESP8266_SendCommand("\r\n");
    sprintf(TXBuffer, "Recv %d bytes", strlen(fetch) + 2);
    return ESP8266_WaitForResponse(TXBuffer, ESP8266_ERROR_RESPONSE);
}

bool ESP8266_SendBufferedStatus(void) {
    char OKBuffer[16];
    char FailBuffer[24];
    if (ESP8266_ConnectionMux) {
        sprintf(OKBuffer, "\n%d,%d,SEND OK\r\n", 0, ESP8266_Segment);
        sprintf(FailBuffer, "\n%d,%d,SEND FAIL\r\n", 0, ESP8266_Segment);
    } else {
        sprintf(OKBuffer, "\n%d,SEND OK\r\n", ESP8266_Segment);
        sprintf(FailBuffer, "\n%d,SEND FAIL\r\n", ESP8266_Segment);
    }
    return ESP8266_WaitForResponse(OKBuffer, FailBuffer);
}

bool ESP8266_Receive(char* fetch, uint32_t max) {
    long sr;
    const char* s;
    char letter;
    while (max > 1) {
        if (esprx0fifo_size() ||
            ESP8266_DataAvailable) { // data (about to be) available?
            while (esprx0fifo_get(&letter) == false) {};
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
                return false;
            }
            while (ESP8266_InChar() != ':') {
            } // wait for DataAvailable to be updated
        }
    }
    *fetch = 0; // terminate with null character
    return true;
}

bool ESP8266_ReceiveMessage(char* fetch, uint32_t max) {
    long sr;
    const char* s;
    char letter;
    while (max > 1) {
        if (esprx0fifo_size() ||
            ESP8266_DataAvailable) { // data (about to be) available?
            while (esprx0fifo_get(&letter) == false) {};
            // ESP8266_DisableInterrupt();  // critical section
            sr = start_critical();
            if (ESP8266_DataAvailable)
                ESP8266_DataAvailable--;
            end_critical(sr);
            // ESP8266_EnableInterrupt();
            *fetch = letter;
            fetch++;
            max--;
            if (letter == '\n')
                break;
        } else { // wait for next packet or connection close
            if (ESP8266_ConnectionMux) {
                s = "0,CLOSED";
            } else {
                s = "CLOSED";
            }
            if (!ESP8266_WaitForResponse(ReceiveDataSearchString, s)) {
                *fetch = 0; // connection closed
                return false;
            }
            while (ESP8266_InChar() != ':') {
            } // wait for DataAvailable to be updated
        }
    }
    *fetch = 0; // terminate with null character
    return true;
}

bool ESP8266_ReceiveEcho() {
    long sr;
    const char* s;
    char letter;
    while (true) {
        if (esprx0fifo_size() ||
            ESP8266_DataAvailable) { // data (about to be) available?
            while (esprx0fifo_get(&letter) == false) {};
            // ESP8266_DisableInterrupt();  // critical section
            sr = start_critical();
            if (ESP8266_DataAvailable)
                ESP8266_DataAvailable--;
            end_critical(sr);
            // ESP8266_EnableInterrupt();
            if (letter == '\x03')
                break;
            // *fetch = letter;
            // fetch++;
            putchar(letter);
        } else { // wait for next packet or connection close
            if (ESP8266_ConnectionMux) {
                s = "0,CLOSED";
            } else {
                s = "CLOSED";
            }
            if (!ESP8266_WaitForResponse(ReceiveDataSearchString, s)) {
                return false;
            }
            while (ESP8266_InChar() != ':') {
            } // wait for DataAvailable to be updated
        }
    }
    return true;
}

bool ESP8266_CloseTCPConnection(void) {
    if (ESP8266_ConnectionMux) {
        ESP8266_SendCommand("AT+CIPCLOSE=0\r\n");
    } else {
        ESP8266_SendCommand("AT+CIPCLOSE\r\n");
    }
    if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, ESP8266_ERROR_RESPONSE)) {
        esprx0fifo_init(); // Discard any data
        return true;
    }
    return false;
}

bool ESP8266_SetDataTransmissionMode(uint8_t mode) {
    char TXBuffer[32];
    sprintf(TXBuffer, "AT+CIPMODE=%d\r\n", mode);
    ESP8266_SendCommand(TXBuffer);
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0);
}

bool ESP8266_GetStatus(void) {
    ESP8266_SendCommand("AT+CIPSTATUS\r\n");
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0);
}

bool ESP8266_EnableServer(uint16_t port) {
    char TXBuffer[32];
    sprintf(TXBuffer, "AT+CIPSERVER=1,%d\r\n", port);
    ESP8266_SendCommand(TXBuffer);
    if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0)) {
        ESP8266_Server = port;
        return true;
    }
    return false;
}

bool ESP8266_SetServerTimeout(uint16_t timeout) {
    char TXBuffer[32];
    sprintf(TXBuffer, "AT+CIPSTO=%d\r\n", timeout);
    ESP8266_SendCommand(TXBuffer);
    return ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, ESP8266_ERROR_RESPONSE);
}

bool ESP8266_WaitForConnection(void) {
    return ESP8266_ConnectionMux && ESP8266_Server &&
           ESP8266_WaitForResponse("0,CONNECT\r\n", 0);
}

bool ESP8266_DisableServer(void) {
    ESP8266_SendCommand("AT+CIPSERVER=0\r\n");
    if (ESP8266_WaitForResponse(ESP8266_OK_RESPONSE, 0)) {
        ESP8266_Server = 0;
        return true;
    }
    return false;
}
