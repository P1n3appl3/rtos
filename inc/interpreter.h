#include "esp8266.h"

#pragma once

#define telnet_server
#if defined(telnet_server) || defined(rpc_server)
char ibuffer[64];
#define iprintf(...)                                                           \
    sprintf(ibuffer, __VA_ARGS__);                                             \
    ESP8266_SendBuffered(ibuffer);
#define iputs(X) ESP8266_SendBuffered(X)
#define ERROR(...)                                                             \
    sprintf(ibuffer, "ERROR: " __VA_ARGS__);                                   \
    ESP8266_SendBuffered(ibuffer);                                             \
    return;
#define ireadline(cmd, size) ESP8266_Receive(cmd, size)
#else
#define iprintf printf
#define iputs puts
#define ERROR(...)                                                             \
    printf(RED "ERROR: " NORMAL __VA_ARGS__);                                  \
    return;
#define ireadline(cmd, size) readline(cmd, size)
#endif

// Run the user interface.
void interpreter(void);
