// *************ADC.c**************
// EE445M/EE380L.6 Labs 1, 2, Lab 3, and Lab 4
// mid-level ADC functions
// you are allowed to call functions in the low level ADCSWTrigger driver
//
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano Jan 5, 2020, valvano@mail.utexas.edu
#include <stdint.h>
#include <tivaware/adc.h>
#include <tivaware/gpio.h>
#include <tivaware/hw_memmap.h>
#include <tivaware/rom.h>
#include <tivaware/sysctl.h>

// channelNum (0 to 11) specifies which pin is sampled with sequencer 3
// software start
// return with error 1, if channelNum>11,
// otherwise initialize ADC and return 0 (success)
int ADC_Init(uint32_t channelNum) {
  // TODO: switch to PE0
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);  // enable ADC0
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); // enable for AIN0 on E3
  ROM_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
  ROM_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
  ROM_ADCSequenceStepConfigure(ADC0_BASE, 3, 0,
                               ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);

  ROM_ADCSequenceEnable(ADC0_BASE, 3);

  ROM_ADCIntClear(ADC0_BASE, 3);

  return 0;
}
// software start sequencer 3 and return 12 bit ADC result
static uint32_t pui32ADC0Value[1];
uint32_t ADC_In(void) {
  ROM_ADCProcessorTrigger(ADC0_BASE, 3); // trigger ADC conversion
  while (
      !ROM_ADCIntStatus(ADC0_BASE, 3, false)) { // wait for conversion to finish
  }
  ROM_ADCIntClear(ADC0_BASE, 3);
  ROM_ADCSequenceDataGet(ADC0_BASE, 3, pui32ADC0Value); // read AIN0 E3

  return pui32ADC0Value[0];
}
