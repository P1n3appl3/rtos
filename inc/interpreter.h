#include "stdbool.h"

// A shell that processes instructions
// Takes input from local uart or a tcp connection (using the ESP)
void interpreter(bool remote);
void mouse_client(void);
