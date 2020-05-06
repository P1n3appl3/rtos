#include "OS.h"
#include "interpreter.h"
#include "mouse.h"
#include "printf.h"
#include "std.h"

void local_interpreter(void) {
    interpreter(false);
}

void trig_test(void) {
    float x[] = {-PI,    -3 * PI / 4, -PI / 2,    -PI / 4, 0,
                 PI / 4, PI / 2,      3 * PI / 4, PI};
    printf("i     x*1000     sin*1000     cos*1000\n\r");
    for (int i = 0; i < sizeof(x) / sizeof(float); ++i) {
        printf("%-6d%-11d%-13d%-8d\n\r", i, (int32_t)(x[i] * 1000),
               (int32_t)(sin(x[i]) * 1000), (int32_t)(cos(x[i]) * 1000));
    }
}

void main(void) {
    OS_Init();
    // OS_AddThread(trig_test, "trig", 512, 2);
    OS_AddThread(local_interpreter, "debug shell", 2048, 2);
    OS_Launch(ms(10));
}
