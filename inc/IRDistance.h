/**
 * @file      IRDistance.h
 * @brief     Take infrared distance measurements
 * @details   Provide mid-level functions that convert raw ADC
 * values from the GP2Y0A21YK0F infrared distance sensors to
 * distances in mm.
 * @version   derived from TI-RSLK MAX v1.1
 * @author    Daniel Valvano and Jonathan Valvano
 * @copyright Copyright 2020 by Jonathan W. Valvano, valvano@mail.utexas.edu,
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      January 15, 2020

 ******************************************************************************/

// 5V connected to all GP2Y0A21YK0F Vcc's (+5V)
// 5V connected to positive side of 10 uF capacitors physically near the sensors
// ground connected to all GP2Y0A21YK0F grounds and microcontroller ground
// ground connected to negative side of all 10 uF capacitors

#ifndef IRDISTANCE_H_
#define IRDISTANCE_H_
#include <stdint.h>

/**
 * Convert ADC sample into distance for the GP2Y0A21YK0F
 * infrared distance sensor.  Conversion uses a calibration formula<br>
 * D = A/(adcSample + B) + C
 * @param adcSample is the 12-bit ADC sample 0 to 4095
 * @param sensor is sensor number 0 to 3
 * @return distance from robot to wall (units mm)
 * @brief  Convert infrared distance measurement
 */
int32_t IRDistance_Convert(int32_t adcSample, uint32_t sensor);

#endif /* IRDISTANCE_H_ */
