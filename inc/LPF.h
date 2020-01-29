/**
 * @file      LPF.h
 * @brief     implements four FIR low-pass filters
 * @details   Finite length LPF<br>
 1) Size is the depth 2 to 16<br>
 2) y(n) = (sum(x(n)+x(n-1)+...+x(n-size-1))/size<br>
 3) To use a filter<br>
   a) initialize it once<br>
   b) call the filter at the sampling rate<br>
 * @version   from TI-RSLK MAX v1.1
 * @author    Daniel Valvano and Jonathan Valvano
 * @copyright Copyright 2020 by Jonathan W. Valvano, valvano@mail.utexas.edu,
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      January 15, 2020


 ******************************************************************************/

/* This example accompanies the books
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
      ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2020
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
      ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2020
   "Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers",
      ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2020

 Copyright 2020 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
#include <stdint.h>
/**
 * Newton's method sqrt
 * @param s is an integer
 * @return sqrt(s) is an integer
 * @brief  square root
 */
uint32_t sqrt2(uint32_t s);

/**
 * Initialize first LPF<br>
 * Set all data to an initial value<br>
 * @param initial value to preload into MACQ
 * @param size depth of the filter, 2 to 16
 * @return none
 * @brief  Initialize first LPF
 */
void LPF_Init(int32_t initial, int32_t size);

/**
 * First LPF, calculate one filter output<br>
 * Called at sampling rate
 * @param newdata new ADC data
 * @return result filter output
 * @brief  FIR low pass filter
 */
int32_t LPF_Calc(int32_t newdata);
int32_t Noise(void);

/**
 * Initialize second LPF<br>
 * Set all data to an initial value<br>
 * @param initial value to preload into MACQ
 * @param size depth of the filter, 2 to 16
 * @return none
 * @brief  Initialize second LPF
 */
void LPF_Init2(int32_t initial, int32_t size);
/**
 * Second LPF, calculate one filter output<br>
 * Called at sampling rate
 * @param newdata new ADC data
 * @return result filter output
 * @brief  FIR low pass filter
 */
int32_t LPF_Calc2(int32_t newdata);
int32_t Noise2(void);
/**
 * Initialize third LPF<br>
 * Set all data to an initial value<br>
 * @param initial value to preload into MACQ
 * @param size depth of the filter, 2 to 16
 * @return none
 * @brief  Initialize third LPF
 */
void LPF_Init3(int32_t initial, int32_t size);

/**
 * Third LPF, calculate one filter output<br>
 * Called at sampling rate
 * @param newdata new ADC data
 * @return result filter output
 * @brief  FIR low pass filter
 */
int32_t LPF_Calc3(int32_t newdata);
int32_t Noise3(void);

/**
 * Initialize fourth LPF<br>
 * Set all data to an initial value<br>
 * @param initial value to preload into MACQ
 * @param size depth of the filter, 2 to 16
 * @return none
 * @brief  Initialize third LPF
 */
void LPF_Init4(int32_t initial, int32_t size);

/**
 * Fourth LPF, calculate one filter output<br>
 * Called at sampling rate
 * @param newdata new ADC data
 * @return result filter output
 * @brief  FIR low pass filter
 */
int32_t LPF_Calc4(int32_t newdata);
int32_t Noise4(void);

/**
 * Initialize fifth LPF<br>
 * Set all data to an initial value<br>
 * @param initial value to preload into MACQ
 * @param size depth of the filter, 2 to 16
 * @return none
 * @brief  Initialize fifth LPF
 */
void LPF_Init5(int32_t initial, int32_t size);

/**
 * fifth LPF, calculate one filter output<br>
 * Called at sampling rate
 * @param newdata new ADC data
 * @return result filter output
 * @brief  FIR low pass filter
 */
int32_t LPF_Calc5(int32_t newdata);
int32_t Noise5(void);
/**
 * Initialize Sixth LPF<br>
 * Set all data to an initial value<br>
 * @param initial value to preload into MACQ
 * @param size depth of the filter, 2 to 16
 * @return none
 * @brief  Initialize Sixth LPF
 */
void LPF_Init6(int32_t initial, int32_t size);

/**
 * Sixth LPF, calculate one filter output<br>
 * Called at sampling rate
 * @param newdata new ADC data
 * @return result filter output
 * @brief  FIR low pass filter
 */
int32_t LPF_Calc6(int32_t newdata);
int32_t Noise6(void);

/**
 * Initialize Seventh LPF<br>
 * Set all data to an initial value<br>
 * @param initial value to preload into MACQ
 * @param size depth of the filter, 2 to 16
 * @return none
 * @brief  Initialize Seventh LPF
 */
void LPF_Init7(int32_t initial, int32_t size);

/**
 * Seventh LPF, calculate one filter output<br>
 * Called at sampling rate
 * @param newdata new ADC data
 * @return result filter output
 * @brief  FIR low pass filter
 */
int32_t LPF_Calc7(int32_t newdata);
int32_t Noise7(void);

/**
 * 3-wide non recursive Median filter <br>
 * Called with new data at sampling rate
 * @param newdata new ADC data
 * @return result filter output
 * @brief  Median filter
 */
int32_t Median(int32_t newdata);

/**
 * 60-Hz notch high-Q, IIR filter, assuming fs=2000 Hz.
 * Assumes the ADC is sampled at 2 kHz. 
 * Assumes this filter is executed at 2 kHz. <br>
 * y(n) = (256x(n) -503x(n-1) + 256x(n-2) + 498y(n-1)-251y(n-2))/256 (2k sampling)<br>
 * @param data new ADC data
 * @return result filter output
 * @brief  60-Hz notch high-Q, IIR filter
 */
 long Filter(long data);
