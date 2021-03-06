#include "ADC.h"
#include "std.h"
#include "timer.h"
#include "tivaware/adc.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/rom.h"
#include "tivaware/timer.h"
#include <stdint.h>

bool adc_init(uint8_t channel_num) {
    if (channel_num > 11) {
        return false;
    }
    ADCConfig config = adcs[channel_num];
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    ROM_SysCtlPeripheralEnable(config.port);
    ROM_GPIOPinTypeADC(GPIO_PORTE_BASE, config.pin);
    ROM_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 3);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 3, 0,
                                 channel_num | ADC_CTL_IE | ADC_CTL_END);
    ROM_ADCSequenceEnable(ADC0_BASE, 3);
    ROM_ADCIntClear(ADC0_BASE, 3);
    return true;
}

void temperature_init(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) {}
    ROM_ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 3);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 2, 0,
                                 ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END);
    ROM_ADCSequenceEnable(ADC0_BASE, 2);
    ROM_ADCIntClear(ADC0_BASE, 2);
}

uint16_t adc_in(void) {
    uint32_t temp;
    ROM_ADCProcessorTrigger(ADC0_BASE, 3);
    while (!ROM_ADCIntStatus(ADC0_BASE, 3, false)) {}
    ROM_ADCIntClear(ADC0_BASE, 3);
    ROM_ADCSequenceDataGet(ADC0_BASE, 3, &temp);
    return temp;
}

float temperature(void) {
    uint32_t temp;
    ROM_ADCProcessorTrigger(ADC0_BASE, 2);
    while (!ROM_ADCIntStatus(ADC0_BASE, 2, false)) {}
    ROM_ADCIntClear(ADC0_BASE, 2);
    ROM_ADCSequenceDataGet(ADC0_BASE, 2, &temp);
    return 32 + 1.8f * (147.5f - 75 * temp * 3.3f / 4096);
}

static void (*process_sample)(uint16_t);

void adc1_sequence0_handler(void) {
    ROM_ADCIntClear(ADC1_BASE, 0);
    uint32_t temp;
    ROM_ADCSequenceDataGet(ADC1_BASE, 0, &temp);
    process_sample(temp);
}

bool adc_timer_init(uint8_t channel_num, uint8_t timer_num, uint32_t period,
                    uint8_t priority, void (*task)(uint16_t)) {
    period = max(period, hz(10000)); // max sample rate = 10kHz
    TimerConfig timer_config = timers[timer_num];
    ROM_SysCtlPeripheralEnable(timer_config.sysctl_periph);
    ROM_TimerConfigure(timer_config.base, TIMER_CFG_PERIODIC);
    ROM_TimerControlStall(timer_config.base, TIMER_A, true);
    ROM_TimerLoadSet(timer_config.base, TIMER_A, period);
    ROM_TimerControlTrigger(timer_config.base, TIMER_BOTH, true);

    ADCConfig adc_config = adcs[channel_num];
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    ROM_SysCtlPeripheralEnable(adc_config.port);
    ROM_GPIOPinTypeADC(adc_config.port_base, adc_config.pin);
    ROM_ADCHardwareOversampleConfigure(ADC1_BASE, 64);
    ROM_ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_TIMER, 0);
    ROM_ADCSequenceStepConfigure(ADC1_BASE, 0, 0,
                                 channel_num | ADC_CTL_IE | ADC_CTL_END);
    process_sample = task;
    ROM_IntEnable(INT_ADC1SS0);
    ROM_IntPrioritySet(INT_ADC1SS0, priority << 5);
    ROM_ADCIntEnable(ADC1_BASE, 0);
    ROM_ADCSequenceEnable(ADC1_BASE, 0);
    ROM_TimerEnable(timer_config.base, TIMER_BOTH);
    return true;
}

const ADCConfig adcs[12] = {{SYSCTL_PERIPH_GPIOE, GPIO_PORTE_BASE, GPIO_PIN_3},
                            {SYSCTL_PERIPH_GPIOE, GPIO_PORTE_BASE, GPIO_PIN_2},
                            {SYSCTL_PERIPH_GPIOE, GPIO_PORTE_BASE, GPIO_PIN_1},
                            {SYSCTL_PERIPH_GPIOE, GPIO_PORTE_BASE, GPIO_PIN_0},
                            {SYSCTL_PERIPH_GPIOD, GPIO_PORTD_BASE, GPIO_PIN_3},
                            {SYSCTL_PERIPH_GPIOD, GPIO_PORTD_BASE, GPIO_PIN_2},
                            {SYSCTL_PERIPH_GPIOD, GPIO_PORTD_BASE, GPIO_PIN_1},
                            {SYSCTL_PERIPH_GPIOD, GPIO_PORTD_BASE, GPIO_PIN_0},
                            {SYSCTL_PERIPH_GPIOE, GPIO_PORTE_BASE, GPIO_PIN_5},
                            {SYSCTL_PERIPH_GPIOE, GPIO_PORTE_BASE, GPIO_PIN_4},
                            {SYSCTL_PERIPH_GPIOB, GPIO_PORTB_BASE, GPIO_PIN_4},
                            {SYSCTL_PERIPH_GPIOB, GPIO_PORTB_BASE, GPIO_PIN_5}};
