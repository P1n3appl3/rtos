#include "ADC.h"
#include "OS.h"
#include "eDisk.h"
#include "esp8266.h"
#include "heap.h"
#include "interpreter.h"
#include "interrupts.h"
#include "io.h"
#include "littlefs.h"
#include "printf.h"
#include "std.h"
#include "tivaware/rom.h"
#include <stdbool.h>
#include <stdint.h>

#define PD0 (*((volatile uint32_t*)0x40007004))
#define PD1 (*((volatile uint32_t*)0x40007008))
#define PD2 (*((volatile uint32_t*)0x40007010))
#define PD3 (*((volatile uint32_t*)0x40007020))

void PortD_Init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, 0x0F);
}

//------------------Idle Task--------------------------------
// foreground thread, runs when nothing else does
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none
void Idle(void) {
    while (1) {
        PD0 ^= 0x01;
        wait_for_interrupts();
    }
}

char Response[64];
Sema4 WebServerSema;
void ServeTelnetRequest(void) {
    puts("Connected           ");

    interpreter();

    ESP8266_CloseTCPConnection();
    OS_Signal(&WebServerSema);
    OS_Kill();
}

void TelnetServer(void) {
    // Initialize and bring up Wifi adapter
    if (!ESP8266_Init(true, false)) { // verbose rx echo on UART for debugging
        puts("No Wifi adapter");
        OS_Kill();
    }
    // Get Wifi adapter version (for debugging)
    ESP8266_GetVersionNumber();
    // Connect to access point
    if (!ESP8266_Connect(true)) {
        puts("No Wifi network");
        OS_Kill();
    }
    puts("Wifi connected");
    if (!ESP8266_StartServer(23, 600)) { // port 80, 5min timeout
        puts("Server failure");
        OS_Kill();
    }
    puts("Server started");

    while (1) {
        // Wait for connection
        ESP8266_WaitForConnection();

        // Launch thread with higher priority to serve request
        OS_AddThread(&ServeTelnetRequest, "ServeTelnetRequest", 2048, 1);

        // The ESP driver only supports one connection, wait for the thread
        // to complete
        OS_Wait(&WebServerSema);
    }
}

void RPCClient(void) {
    // Initialize and bring up Wifi adapter
    if (!ESP8266_Init(true, false)) { // verbose rx echo on UART for debugging
        puts("No Wifi adapter");
        OS_Kill();
    }
    // Get Wifi adapter version (for debugging)
    ESP8266_GetVersionNumber();
    // Connect to access point
    if (!ESP8266_Connect(true)) {
        puts("No Wifi network");
        OS_Kill();
    }
    puts("Wifi connected");
    if (!ESP8266_MakeTCPConnection("71.135.6.224", 23,
                                   600)) { // port 80, 5min timeout
        puts("Client failure");
        OS_Kill();
    }
    puts("Client started");

    while (1) {
        // Launch thread with higher priority to serve request
        OS_AddThread(&iclient, "iclient", 2048, 1);

        // The ESP driver only supports one connection, wait for the thread
        // to complete
        OS_Wait(&WebServerSema);
    }
}

//--------------end of Idle Task-----------------------------

//*******************final user main - bare bones OS, extend with your
// code**********
int realmain(void) { // realmain
    OS_Init();       // initialize, disable interrupts
    PortD_Init();    // debugging profile
    heap_init();     // initialize heap

    OS_InitSemaphore(&WebServerSema, 0);
    // hardware init
    adc_init(0); // sequencer 3, channel 0, PE3, sampling in Interpreter

    // attach background tasks
    OS_AddPeriodicThread(&disk_timerproc, ms(1),
                         0); // time out routines for disk

    // create initial foreground threads
    // NumCreated += OS_AddThread(&interpreter, "Interpreter", 128, 2);
    OS_AddThread(&Idle, "Idle", 128, 5); // at lowest priority
#if defined(rpc_client)
    OS_AddThread(&RPCClient, "RPC Client", 1024, 2);
#else
    OS_AddThread(&TelnetServer, "TelnetServer", 1024, 2);
#endif

    OS_Launch(ms(2)); // doesn't return, interrupts enabled in here
    return 0;         // this never executes
}

//*****************Test project 2*************************
// ESP8266 Wifi client test, fetch weather info from internet

const char Fetch[] =
    "GET /data/2.5/weather?q=Austin&APPID=1bc54f645c5f1c75e681c102ed4bbca4 "
    "HTTP/1.1\r\nHost:api.openweathermap.org\r\n\r\n";
// char Fetch[] = "GET
// /data/2.5/weather?q=Austin%20Texas&APPID=1234567890abcdef1234567890abcdef
// HTTP/1.1\r\nHost:api.openweathermap.org\r\n\r\n";
// 1) go to http://openweathermap.org/appid#use
// 2) Register on the Sign up page
// 3) get an API key (APPID) replace the 1234567890abcdef1234567890abcdef with
// your APPID

uint32_t Running; // true while fetch is running
void FetchWeather(void) {
    uint32_t len;
    char* p;
    char* s;
    char* e;
    int32_t data;
    puts("                 ");
    ESP8266_GetStatus(); // debugging
    // Fetch weather from server
    if (!ESP8266_MakeTCPConnection("api.openweathermap.org", 80,
                                   0)) { // open socket to web server on port 80
        puts("Connection failed");
        Running = 0;
        OS_Kill();
    }
    // Send request to server
    if (!ESP8266_Send(Fetch)) {
        puts("Send failed");
        ESP8266_CloseTCPConnection();
        Running = 0;
        OS_Kill();
    }
    // Receive response
    if (!ESP8266_Receive(Response, 64)) {
        puts("No response");
        ESP8266_CloseTCPConnection();
        Running = 0;
        OS_Kill();
    }
    if (strncmp(Response, "HTTP", 4) == 0) {
        // We received a HTTP response
        puts("Weather fetched");
        // Parse HTTP headers until empty line
        len = 0;
        while (strlen(Response)) {
            if (!ESP8266_Receive(Response, 64)) {
                len = 0;
                break;
            }
            // check for body size
            if (strncmp(Response, "Content-Length: ", 16) == 0) {
                len = atoi(Response + 16);
            }
        }
        if (len) {
            // Get HTML body and parse for weather info
            p = malloc(len + 1);
            ESP8266_Receive(p, len + 1);
            s = strstr(p, "\"temp\":"); // get temperature
            if (s) {
                data = atoi(s + 7);
                printf("Temp [K]: %d", data);
            }
            s = strstr(p, "\"description\":"); // get description
            if (s) {
                e = strchr(s + 15, '"'); // find end of substring
                if (e) {
                    *e = 0; // temporarily terminate with zero
                    puts(s + 15);
                }
            }
            free(p);
        } else {
            puts("Empty response");
        }
    } else {
        puts("Invalid response");
    }
    // Close connection and end
    ESP8266_CloseTCPConnection();
    Running = 0;
    OS_Kill();
}
void ConnectWifi(void) {
    // Initialize and bring up Wifi adapter
    if (!ESP8266_Init(true, false)) { // verbose rx echo on UART for debugging
        puts("No Wifi adapter");
        OS_Kill();
    }
    // Get Wifi adapter version (for debugging)
    ESP8266_GetVersionNumber();
    // Connect to access point
    if (!ESP8266_Connect(true)) {
        puts("No Wifi network");
        OS_Kill();
    }
    puts("Wifi connected");
    // Launch thread to fetch weather
    OS_AddThread(&FetchWeather, "FetchWeather", 1024, 1);
    // Kill thread (should really loop to check and reconnect if necessary
    OS_Kill();
}

void SW1Push2(void) {
    if (!Running) {
        Running = 1; // don't start twice
        OS_AddThread(&FetchWeather, "FetchWeather", 1024, 1);
    }
}

void Testmain2(void) {
    OS_Init();
    PortD_Init();
    heap_init();
    Running = 1;
    OS_AddSW1Task(&SW1Push2);
    OS_AddThread(&ConnectWifi, "ConnectWifi", 1024, 2);
    OS_Launch(ms(10));
}

//*****************Test project 3*************************
// ESP8266 web server test, serve a web page with a form to submit message

const char formBody[] = "<!DOCTYPE html><html><body><center> \
  <h1>Enter a message to send to your microcontroller:</h1> \
  <form> \
  <input type=\"text\" name=\"message\" value=\"Hello ESP8266!\"> \
  <br><input type=\"submit\" value=\"Go!\"> \
  </form></center></body></html>";
const char statusBody[] = "<!DOCTYPE html><html><body><center> \
  <h1>Message sent successfully!</h1> \
  </body></html>";
int HTTP_ServePage(const char* body) {
    char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
                    "close\r\nContent-Length: ";

    char contentLength[16];
    sprintf(contentLength, "%d\r\n\r\n\0", strlen(body));

    if (!ESP8266_SendBuffered(header))
        return 0;
    if (!ESP8266_SendBuffered(contentLength))
        return 0;
    if (!ESP8266_SendBuffered(body))
        return 0;

    if (!ESP8266_SendBufferedStatus())
        return 0;

    return 1;
}
void ServeHTTPRequest(void) {
    puts("Connected           ");

    // Receive request
    if (!ESP8266_Receive(Response, 64)) {
        puts("No request");
        ESP8266_CloseTCPConnection();
        OS_Signal(&WebServerSema);
        OS_Kill();
    }

    // check for HTTP GET
    if (strncmp(Response, "GET", 3) == 0) {
        char* messagePtr = strstr(Response, "?message=");
        if (messagePtr) {
            // Clear any previous message
            puts("                    ");
            // Process form reply
            if (HTTP_ServePage(statusBody)) {
                puts("Served status");

            } else {
                puts("Error serving status");
            }
            // Terminate message at first separating space
            char* messageEnd = strchr(messagePtr, ' ');
            if (messageEnd)
                *messageEnd = 0; // terminate with null character
            // Print message on terminal
            puts(messagePtr + 9);
        } else {
            // Serve web page
            if (HTTP_ServePage(formBody)) {
                puts("Served form");
            } else {
                puts("Error serving form");
            }
        }
    } else {
        // handle data that may be sent via means other than HTTP GET
        puts("Not a GET request");
    }
    ESP8266_CloseTCPConnection();
    OS_Signal(&WebServerSema);
    OS_Kill();
}
void WebServer(void) {
    // Initialize and bring up Wifi adapter
    if (!ESP8266_Init(true,
                      false)) { // verbose rx echo on UART for debugging
        puts("No Wifi adapter");
        OS_Kill();
    }
    // Get Wifi adapter version (for debugging)
    ESP8266_GetVersionNumber();
    // Connect to access point
    if (!ESP8266_Connect(true)) {
        puts("No Wifi network");
        OS_Kill();
    }
    puts("Wifi connected");
    if (!ESP8266_StartServer(80, 600)) { // port 80, 5min timeout
        puts("Server failure");
        OS_Kill();
    }
    puts("Server started");

    while (1) {
        // Wait for connection
        ESP8266_WaitForConnection();

        // Launch thread with higher priority to serve request
        OS_AddThread(&ServeHTTPRequest, "ServeHTTPRequest", 1024, 1);

        // The ESP driver only supports one connection, wait for the
        // thread to complete
        OS_Wait(&WebServerSema);
    }
}

void Testmain3(void) {
    OS_Init();
    PortD_Init();
    OS_InitSemaphore(&WebServerSema, 0);

    // create initial foreground threads
    OS_AddThread(&Idle, "Idle", 128, 3);
    OS_AddThread(&WebServer, "WebServer", 1024, 2);

    OS_Launch(ms(10)); // doesn't return, interrupts enabled in here
}

void testmain_littlefs(void) {
    OS_Init();
    OS_AddThread(littlefs_test, "filesystem test", 2048, 0);
    OS_Launch(ms(10));
}

//*******************Trampoline for selecting main to execute**********

int main(void) {
    realmain();
    // Testmain3();
    // testmain_littlefs();
    return 0;
}
