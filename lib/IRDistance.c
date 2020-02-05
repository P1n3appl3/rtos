// IRDistance.c

#include <stdint.h>

/* Calibration data
distance measured from front of the sensor to the wall
d(cm) 1/d    bL     al     aR   bR  adcSample d (0.01cm)  error
10    0.100  2813  2830  2820  2830  2823.25  1006        0.06
15    0.067  1935  1976  1986  1978  1968.75  1482       -0.18
20    0.050  1520  1500  1520  1550  1522.5   1966       -0.34
30    0.033  1040  1096  1028   933  1024.25  3099        0.99

      adcSample = 26813/d+159      2681300
      d = 26813/(adcSample-159)      -159
*/
const int32_t A[4] = {268130, 268130, 268130, 268130};
const int32_t B[4] = {-159, -159, -159, -159};
const int32_t C[4] = {0, 0, 0, 0};
const int32_t IRmax[4] = {494, 494, 494, 494};
int32_t IRDistance_Convert(int32_t adcSample,
                           uint32_t sensor) { // returns left distance in mm
    if (adcSample < IRmax[sensor]) {
        return 800;
    }
    return A[sensor] / (adcSample + B[sensor]) + C[sensor];
}
