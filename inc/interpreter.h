#include "esp8266.h"

#pragma once

// choices: telnet_server, rpc_server, rpc_client
#define rpc_server

char ibuffer[64];
#if defined(telnet_server) || defined(rpc_server)
#define iprintf(...)                                                           \
    sprintf(ibuffer, __VA_ARGS__);                                             \
    ESP8266_SendMessage(ibuffer);
#define iputs(S) ESP8266_SendMessage(S)
#define ERROR(...)                                                             \
    sprintf(ibuffer, "ERROR: " __VA_ARGS__);                                   \
    ESP8266_SendMessage(ibuffer);                                              \
    return;
#define ireadline(cmd, size) ESP8266_ReceiveMessage(cmd, size)
#else
#define iprintf printf
#define iputs puts
#define ERROR(...)                                                             \
    printf(RED "ERROR: " NORMAL __VA_ARGS__);                                  \
    return;
#define ireadline(cmd, size) readline(cmd, size)
#endif

// Processes shell instructions, either from local uart or over a tcp connection
// with the ESP
void interpreter(bool remote);

// Can be launched as a thread after a connection is established with a remote
// interpreter
void remote_client(void);
