#include <stdint.h>

// channel_num (0 to 11) specifies which pin is sampled with sequencer 3
// software start
// return with error 1, if channel_num>11,
// otherwise initialize ADC and return 0 (success)
int ADC_init(uint32_t channel_num);

// software start sequencer 3 and return 12 bit ADC result
uint32_t ADC_in(void);

void ADC0_InitTimer0(uint32_t channel_num, uint32_t fs, void (*task)(uint32_t));
