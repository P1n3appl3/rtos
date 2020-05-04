// Driver originally written by Steven Prickett (steven.prickett@gmail.com)
// Modified by Dung Nguyen, Wally Guzman, and Jonathan Valvano
// Consolidated by Andreas Gerstlauer

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Initializes the module
bool ESP8266_Init(bool rx_echo, bool tx_echo);

// Bring interface up and connect to Wifi
bool ESP8266_Connect(bool verbose);

// Start server on specific port (timeout is in seconds)
bool ESP8266_StartServer(uint16_t port, uint16_t timeout);

// Stop server
bool ESP8266_StopServer(void);

// soft resets the esp8266 module
bool ESP8266_Reset(void);

// restore the ESP8266 module to default values
bool ESP8266_Restore(void);

// get version number
bool ESP8266_GetVersionNumber(void);

// get MAC address
bool ESP8266_GetMACAddress(void);

typedef enum { CLIENT = 1, AP, BOTH } ESP8266_WIFI_MODE;

// configures the esp8266 to operate as a wifi client, access point, or both
bool ESP8266_SetWifiMode(ESP8266_WIFI_MODE mode);

// lists available wifi access points
bool ESP8266_ListAccessPoints(void);

// joins a wifi access point using specified ssid and password
bool ESP8266_JoinAccessPoint(const char* ssid, const char* password);

// disconnects from currently connected wifi access point
bool ESP8266_QuitAccessPoint(void);

// Get local IP address
bool ESP8266_GetIPAddress(void);

// Establish TCP connection
// takes IP address or URL and a port number
bool ESP8266_MakeTCPConnection(char* address, uint16_t port);

// Send a packet to server
bool ESP8266_Send(const char* str);

// Send a string to server using ESP TCP-send buffer
bool ESP8266_SendBuffered(const char* str);

// Reads from data input until end of line or max length is reached
bool ESP8266_Receive(char* buf, uint32_t max);

// Echos from data input until EOT is reached
bool ESP8266_ReceiveEcho();

// Close TCP connection
bool ESP8266_CloseTCPConnection(void);

// get network connection status
bool ESP8266_GetStatus(void);

// set connection timeout for tcp server, 0-28800 seconds
bool ESP8266_SetServerTimeout(uint16_t timeout);

// wait for incoming connection on server
bool ESP8266_WaitForConnection(void);
