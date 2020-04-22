//***********************  ESP8266.h  ***********************
// Program written by:
// - Steven Prickett  steven.prickett@gmail.com
//
// Driver for ESP8266 module to act as a WiFi client or server
//
//*********************************************************
/* Modified by Jonathan Valvano, Sept 19, 2015
   Modified by Andreas Gerstlauer, Apr 13, 2020
 */

#ifndef ESP8266_H
#define ESP8266_H

#include <stdint.h>

#define ESP8266_ENCRYPT_MODE_OPEN 0
#define ESP8266_ENCRYPT_MODE_WEP 1
#define ESP8266_ENCRYPT_MODE_WPA_PSK 2
#define ESP8266_ENCRYPT_MODE_WPA2_PSK 3
#define ESP8266_ENCRYPT_MODE_WPA_WPA2_PSK 4

#define ESP8266_WIFI_MODE_CLIENT 1
#define ESP8266_WIFI_MODE_AP 2
#define ESP8266_WIFI_MODE_AP_AND_CLIENT 3

//-------------------ESP8266_Init --------------
// Initializes the module
// Inputs: RX and/or TX echo for debugging
// Outputs: 1 for success, 0 for failure (no ESP detected)
int ESP8266_Init(int rx_echo, int tx_echo);

//-------------------ESP8266_Connect --------------
// Bring interface up and connect to Wifi
// Inputs: enable debug output
// Outputs: 1 on success, 0 on failure
int ESP8266_Connect(int verbose);

//-------------------ESP8266_StartServer --------------
// Start server on specific port
// Inputs: port and server timeout
// Outputs: 1 on success, 0 on failure
int ESP8266_StartServer(uint16_t port, uint16_t timeout);

//-------------------ESP8266_StopServer --------------
// Stop server and set to single-client mode
// Inputs: none
// Outputs: 1 on success, 0 on failure
int ESP8266_StopServer(void);

//----------ESP8266_Reset------------
// soft resets the esp8266 module
// input:  none
// output: 1 if success, 0 if fail
int ESP8266_Reset(void);

//---------ESP8266_Restore-----
// restore the ESP8266 module to default values
// Inputs: none
// Outputs: 1 if success, 0 if fail
int ESP8266_Restore(void);

//---------ESP8266_GetVersionNumber----------
// get status
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_GetVersionNumber(void);

//---------ESP8266_GetMACAddress----------
// get MAC address
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_GetMACAddress(void);

//---------ESP8266_SetWifiMode----------
// configures the esp8266 to operate as a wifi client, access point, or both
// Input: mode accepts ESP8266_WIFI_MODE constants
// output: 1 if success, 0 if fail
int ESP8266_SetWifiMode(uint8_t mode);

//---------ESP8266_SetConnectionMux----------
// enables the esp8266 connection mux, required for starting tcp server
// Input: 0 (single) or 1 (multiple)
// output: 1 if success, 0 if fail
int ESP8266_SetConnectionMux(uint8_t enabled);

//---------ESP8266_ListAccessPoints----------
// lists available wifi access points
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_ListAccessPoints(void);

//----------ESP8266_JoinAccessPoint------------
// joins a wifi access point using specified ssid and password
// input:  SSID and PASSWORD
// output: 1 if success, 0 if fail
int ESP8266_JoinAccessPoint(const char* ssid, const char* password);

// ----------ESP8266_QuitAccessPoint-------------
// disconnects from currently connected wifi access point
// Inputs: none
// Outputs: 1 if success, 0 if fail
int ESP8266_QuitAccessPoint(void);

//----------ESP8266_ConfigureAccessPoint------------
// configures esp8266 wifi soft access point settings
// use this function only when in AP mode (and not in client mode)
// input:  SSID, Password, channel, security
// output: 1 if success, 0 if fail
int ESP8266_ConfigureAccessPoint(const char* ssid, const char* password,
                                 uint8_t channel, uint8_t encryptMode);

//---------ESP8266_GetIPAddress----------
// Get local IP address
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_GetIPAddress(void);

//---------ESP8266_MakeTCPConnection----------
// Establish TCP connection
// Inputs: IP address or web page as a string, port, and keepalive time (0 if
// none) output: 1 if success, 0 if fail
int ESP8266_MakeTCPConnection(char* IPaddress, uint16_t port,
                              uint16_t keepalive);

//---------ESP8266_Send----------
// Send a packet to server
// Input: payload to send
// output: 1 if success, 0 if fail
int ESP8266_Send(const char* fetch);

//---------ESP8266_SendBuffered----------
// Send a string to server using ESP TCP-send buffer
// Input: payload to send
// output: 1 if success, 0 if fail
int ESP8266_SendBuffered(const char* fetch);

//---------ESP8266_SendBuferedStatus----------
// Check status of last buffered segment
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_SendBufferedStatus(void);

//---------ESP8266_Receive----------
// Receive a string from server
// Reads from data input until end of line or max length is reached
// Input: buffer and max length
// Output: 1 and null-terminated string if success, 0 if fail (disconnected)
int ESP8266_Receive(char* fetch, uint32_t max);

//---------ESP8266_CloseTCPConnection----------
// Close TCP connection
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_CloseTCPConnection(void);

//---------ESP8266_SetDataTransmissionMode----------
// set data transmission passthrough mode
// Input: 0 not data mode, 1 data mode; return "Link is builded"
// output: 1 if success, 0 if fail
int ESP8266_SetDataTransmissionMode(uint8_t mode);

//---------ESP8266_GetStatus----------
// get network connection status
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_GetStatus(void);

// --------ESP8266_EnableServer------------------
// enables tcp server on specified port
// Inputs: port number
// Outputs: 1 if success, 0 if fail
int ESP8266_EnableServer(uint16_t port);

// ----------ESP8266_SetServerTimeout--------------
// set connection timeout for tcp server, 0-28800 seconds
// Inputs: timeout parameter
// Outputs: 1 if success, 0 if fail
int ESP8266_SetServerTimeout(uint16_t timeout);

// ----------ESP8266_WaitForConnection--------------
// wait for incoming connection on server
// Inputs: none
// Outputs: 1 if success, 0 if fail
int ESP8266_WaitForConnection(void);

//---------ESP8266_DisableServer----------
// disables tcp server
// Input: none
// output: 1 if success, 0 if fail
int ESP8266_DisableServer(void);

#endif
