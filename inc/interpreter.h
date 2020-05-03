#include "esp8266.h"

#pragma once

// Processes shell instructions, either from local uart or over a tcp connection
// with the ESP
void interpreter(bool remote);

// Can be launched as a thread after a connection is established with a remote
// interpreter
void remote_client(void);
