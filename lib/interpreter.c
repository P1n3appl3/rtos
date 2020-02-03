#include "ADC.h"
#include "OS.h"
#include "ST7735.h"
#include "io.h"
#include "launchpad.h"
#include "printf.h"
#include <stdint.h>

// Print jitter histogram
void Jitter(int32_t MaxJitter, uint32_t const JitterSize,
            uint32_t JitterHistogram[]) {
    // write this for Lab 3 (the latest)
}

char command[128];

void interpreter(void) {
    printf("abcdefhijklmnopqrstuvwxyz\n\r");
    printf("0123456789ABCDEF0123456789ABCDEF\n\r");
    printf("Input a character: ");
    char temp = getchar();
    led_toggle(RED_LED);
    printf("\n\rYou input the character: %d\n\r", temp);
}
