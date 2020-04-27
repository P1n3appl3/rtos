// Baudrate for UART connection to ESP8266
#define BAUDRATE 115200

// Return values
#define NORESPONSE (-1)
#define BADVALUE (-1)
#define SUCCESS 1
#define FAILURE 0

enum Menu_Status { RX = 0, TX, CONNECTED };

// Station or soft access point mode
// 0 means regular station, 1 means act as soft AP
#define SOFTAP 0
