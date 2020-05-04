#include "OS.h"
#include "interpreter.h"
#include "mouse.h"
#include "printf.h"

void local_interpreter(void) {
    interpreter(false);
}

void main(void) {
    OS_Init();
    // OS_AddThread(mouse_init, "usb mouse", 1024, 3);
    OS_AddThread(local_interpreter, "debug shell", 2048, 3);
    OS_Launch(ms(2));
}
