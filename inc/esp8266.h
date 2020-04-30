// Driver originally written by Steven Prickett (steven.prickett@gmail.com)
// Modified version by Dung Nguyen, Wally Guzman
// Modified by Jonathan Valvano, March 28, 2017
// Consolidated by Andreas Gerstlauer, April 6, 2020

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    OPEN = 0,
    WEP,
    WPA_PSK,
    WPA2_PSK,
    WPA_WPA2_PSK,
} ESP8266_ENCRYPT_MODE;

// Baudrate for UART connection to ESP8266
#define BAUDRATE 115200

enum Menu_Status { RX = 0, TX, CONNECTED };

// Station or soft access point mode
// 0 means regular station, 1 means act as soft AP
#define SOFTAP 0

#define ETX 3
#define EOT 4

// Initializes the module
bool ESP8266_Init(bool rx_echo, bool tx_echo);

// Bring interface up and connect to Wifi
bool ESP8266_Connect(bool verbose);

// Start server on specific port (timeout is in seconds)
bool ESP8266_StartServer(uint16_t port, uint16_t timeout);

// Stop server and set to single-client mode
bool ESP8266_StopServer(void);

// soft resets the esp8266 module
bool ESP8266_Reset(void);

// restore the ESP8266 module to default values
bool ESP8266_Restore(void);

//---------ESP8266_GetVersionNumber----------
// get version number
bool ESP8266_GetVersionNumber(void);

// get MAC address
bool ESP8266_GetMACAddress(void);

typedef enum { CLIENT = 1, AP, BOTH } ESP8266_WIFI_MODE;

// configures the esp8266 to operate as a wifi client, access point, or both
bool ESP8266_SetWifiMode(ESP8266_WIFI_MODE mode);

// enables the esp8266 connection mux, required for starting tcp server
bool ESP8266_SetConnectionMux(bool multiple);

// lists available wifi access points
bool ESP8266_ListAccessPoints(void);

// joins a wifi access point using specified ssid and password
bool ESP8266_JoinAccessPoint(const char* ssid, const char* password);

// disconnects from currently connected wifi access point
bool ESP8266_QuitAccessPoint(void);

// Configures esp8266 wifi soft access point settings
// use this function only when in AP mode (and not in client mode)
bool ESP8266_ConfigureAccessPoint(const char* ssid, const char* password,
                                  uint8_t channel, uint8_t encryptMode);

// Get local IP address
bool ESP8266_GetIPAddress(void);

// Establish TCP connection
// takes IP address or URL and a port number
bool ESP8266_MakeTCPConnection(char* address, uint16_t port);

// Send a packet to server
bool ESP8266_Send(const char* fetch);

// Send a string to server using ESP TCP-send buffer
bool ESP8266_SendBuffered(const char* fetch);
bool ESP8266_SendMessage(const char* fetch);

// Check status of last buffered segment
bool ESP8266_SendBufferedStatus(void);

// Receive a string from server
// Reads from data input until end of line or max length is reached
// Input: buffer and max length
// Output: 1 and null-terminated string if success, 0 if fail (disconnected)
bool ESP8266_Receive(char* fetch, uint32_t max);
bool ESP8266_ReceiveMessage(char* fetch, uint32_t max);
bool ESP8266_ReceiveEcho();

// Close TCP connection
bool ESP8266_CloseTCPConnection(void);

// set data transmission passthrough mode
// Mode: 0 not data mode
//       1 data mode; return "Link is builded"
bool ESP8266_SetDataTransmissionMode(uint8_t mode);

// get network connection status
bool ESP8266_GetStatus(void);

// enables tcp server on specified port
bool ESP8266_EnableServer(uint16_t port);

// set connection timeout for tcp server, 0-28800 seconds
bool ESP8266_SetServerTimeout(uint16_t timeout);

// wait for incoming connection on server
bool ESP8266_WaitForConnection(void);

// disables tcp server
bool ESP8266_DisableServer(void);
