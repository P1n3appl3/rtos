// LPF.c
// Runs on any microcontroller
// implements seven FIR low-pass filters

#include <stdint.h>

// Newton's method
// s is an integer
// sqrt(s) is an integer
uint32_t sqrt2(uint32_t s) {
    uint32_t t;            // t*t will become s
    int n;                 // loop counter to make sure it stops running
    t = s / 10 + 1;        // initial guess
    for (n = 16; n; --n) { // guaranteed to finish
        t = ((t * t + s) / t) / 2;
    }
    return t;
}

#define FILTERMAX 16
//**************Low pass Digital filter**************
int32_t Size1;         // Size-point average, Size=2 to FILTERMAX
int32_t x1[FILTERMAX]; // one copy of data in MACQ
uint32_t I1;           // index to oldest
int32_t LPFSum1;       // sum of the last Size samples
int32_t sigma1;
int32_t snr1; // signal to noise ratio
void LPF_Init(int32_t initial, int32_t size) {
    int i;
    if (size > FILTERMAX)
        size = FILTERMAX; // max
    Size1 = size;
    I1 = Size1 - 1;
    LPFSum1 = Size1 * initial; // prime MACQ with initial data
    for (i = 0; i < Size1; i++) { x1[i] = initial; }
}
// calculate one filter output, called at sampling rate
// Input: new ADC data   Output: filter output
// y(n) = (x(n)+x(n-1)+...+x(n-Size-1)/Size
int32_t LPF_Calc(int32_t newdata) {
    if (I1 == 0) {
        I1 = Size1 - 1; // wrap
    } else {
        I1--; // make room for data
    }
    LPFSum1 = LPFSum1 + newdata - x1[I1]; // subtract oldest, add newest
    x1[I1] = newdata;                     // save new data
    return LPFSum1 / Size1;
}
int32_t Noise(void) {
    int32_t sum, mean;
    if (Size1 < 2)
        return 0;
    sum = 0;
    for (int i = 0; i < Size1; i++) { sum = sum + x1[i]; }
    mean = sum / Size1; // DC component
    sum = 0;
    for (int i = 0; i < Size1; i++) {
        sum = sum + (x1[i] - mean) * (x1[i] - mean); // total energy in AC part
    }
    sigma1 = sqrt2(sum / (Size1 - 1));
    snr1 = mean / sigma1;
    return sigma1;
}

int32_t Size2;         // Size-point average, Size=2 to FILTERMAX
int32_t x2[FILTERMAX]; // one copy of data in MACQ
uint32_t I2;           // index to oldest
int32_t LPFSum2;       // sum of the last Size samples
int32_t sigma2;
int32_t snr2; // signal to noise ratio
void LPF_Init2(int32_t initial, int32_t size) {
    int i;
    if (size > FILTERMAX)
        size = FILTERMAX; // max
    Size2 = size;
    I2 = Size2 - 1;
    LPFSum2 = Size2 * initial; // prime MACQ with initial data
    for (i = 0; i < Size2; i++) { x2[i] = initial; }
}
// calculate one filter output, called at sampling rate
// Input: new ADC data   Output: filter output, DAC data
// y(n) = (x(n)+x(n-1)+...+x(n-Size-1)/Size
int32_t LPF_Calc2(int32_t newdata) {
    if (I2 == 0) {
        I2 = Size2 - 1; // wrap
    } else {
        I2--; // make room for data
    }
    LPFSum2 = LPFSum2 + newdata - x2[I2]; // subtract oldest, add newest
    x2[I2] = newdata;                     // save new data
    return LPFSum2 / Size2;
}
int32_t Noise2(void) {
    int32_t sum, mean;
    if (Size2 < 2)
        return 0;
    sum = 0;
    for (int i = 0; i < Size2; i++) { sum = sum + x2[i]; }
    mean = sum / Size2; // DC component
    sum = 0;
    for (int i = 0; i < Size2; i++) {
        sum = sum + (x2[i] - mean) * (x2[i] - mean); // total energy in AC part
    }
    sigma2 = sqrt2(sum / (Size2 - 1));
    snr2 = mean / sigma2;
    return sigma2;
}

int32_t Size3;         // Size-point average, Size=2 to FILTERMAX
int32_t x3[FILTERMAX]; // one copy of data in MACQ
uint32_t I3;           // index to oldest
int32_t LPFSum3;       // sum of the last Size samples
int32_t sigma3;
int32_t snr3; // signal to noise ratio
void LPF_Init3(int32_t initial, int32_t size) {
    int i;
    if (size > FILTERMAX)
        size = FILTERMAX; // max
    Size3 = size;
    I3 = Size3 - 1;
    LPFSum3 = Size3 * initial; // prime MACQ with initial data
    for (i = 0; i < Size3; i++) { x3[i] = initial; }
}
// calculate one filter output, called at sampling rate
// Input: new ADC data   Output: filter output, DAC data
// y(n) = (x(n)+x(n-1)+...+x(n-Size-1)/Size
int32_t LPF_Calc3(int32_t newdata) {
    if (I3 == 0) {
        I3 = Size3 - 1; // wrap
    } else {
        I3--; // make room for data
    }
    LPFSum3 = LPFSum3 + newdata - x3[I3]; // subtract oldest, add newest
    x3[I3] = newdata;                     // save new data
    return LPFSum3 / Size3;
}
int32_t Noise3(void) {
    int32_t sum, mean;
    if (Size3 < 2)
        return 0;
    sum = 0;
    for (int i = 0; i < Size3; i++) { sum = sum + x3[i]; }
    mean = sum / Size3; // DC component
    sum = 0;
    for (int i = 0; i < Size3; i++) {
        sum = sum + (x3[i] - mean) * (x3[i] - mean); // total energy in AC part
    }
    sigma3 = sqrt2(sum / (Size3 - 1));
    snr3 = (mean) / sigma3;
    return sigma3;
}

int32_t Size4;         // Size-point average, Size=2 to FILTERMAX
int32_t x4[FILTERMAX]; // one copy of data in MACQ
uint32_t I4;           // index to oldest
int32_t LPFSum4;       // sum of the last Size samples
int32_t sigma4;
int32_t snr4; // signal to noise ratio
void LPF_Init4(int32_t initial, int32_t size) {
    int i;
    if (size > FILTERMAX)
        size = FILTERMAX; // max
    Size4 = size;
    I4 = Size4 - 1;
    LPFSum4 = Size4 * initial; // prime MACQ with initial data
    for (i = 0; i < Size4; i++) { x4[i] = initial; }
}
// calculate one filter output, called at sampling rate
// Input: new ADC data   Output: filter output, DAC data
// y(n) = (x(n)+x(n-1)+...+x(n-Size-1)/Size
int32_t LPF_Calc4(int32_t newdata) {
    if (I4 == 0) {
        I4 = Size4 - 1; // wrap
    } else {
        I4--; // make room for data
    }
    LPFSum4 = LPFSum4 + newdata - x4[I4]; // subtract oldest, add newest
    x4[I4] = newdata;                     // save new data
    return LPFSum4 / Size4;
}
int32_t Noise4(void) {
    int32_t sum, mean;
    if (Size4 < 2)
        return 0;
    sum = 0;
    for (int i = 0; i < Size4; i++) { sum = sum + x4[i]; }
    mean = sum / Size4; // DC component
    sum = 0;
    for (int i = 0; i < Size4; i++) {
        sum = sum + (x4[i] - mean) * (x4[i] - mean); // total energy in AC part
    }
    sigma4 = sqrt2(sum / (Size4 - 1));
    snr4 = mean / sigma4;
    return sigma4;
}

int32_t Size5;         // Size-point average, Size=2 to FILTERMAX
int32_t x5[FILTERMAX]; // one copy of data in MACQ
uint32_t I5;           // index to oldest
int32_t LPFSum5;       // sum of the last Size samples
int32_t sigma5;
int32_t snr5; // signal to noise ratio
void LPF_Init5(int32_t initial, int32_t size) {
    int i;
    if (size > FILTERMAX)
        size = FILTERMAX; // max
    Size5 = size;
    I5 = Size5 - 1;
    LPFSum5 = Size5 * initial; // prime MACQ with initial data
    for (i = 0; i < Size5; i++) { x5[i] = initial; }
}
// calculate one filter output, called at sampling rate
// Input: new ADC data   Output: filter output, DAC data
// y(n) = (x(n)+x(n-1)+...+x(n-Size-1)/Size
int32_t LPF_Calc5(int32_t newdata) {
    if (I5 == 0) {
        I5 = Size5 - 1; // wrap
    } else {
        I5--; // make room for data
    }
    LPFSum5 = LPFSum5 + newdata - x5[I5]; // subtract oldest, add newest
    x5[I5] = newdata;                     // save new data
    return LPFSum5 / Size5;
}
int32_t Noise5(void) {
    int32_t sum, mean;
    if (Size5 < 2)
        return 0;
    sum = 0;
    for (int i = 0; i < Size5; i++) { sum = sum + x5[i]; }
    mean = sum / Size5; // DC component
    sum = 0;
    for (int i = 0; i < Size5; i++) {
        sum = sum + (x5[i] - mean) * (x5[i] - mean); // total energy in AC part
    }
    sigma5 = sqrt2(sum / (Size5 - 1));
    snr5 = mean / sigma5;
    return sigma5;
}
int32_t Size6;         // Size-point average, Size=2 to FILTERMAX
int32_t x6[FILTERMAX]; // one copy of data in MACQ
uint32_t I6;           // index to oldest
int32_t LPFSum6;       // sum of the last Size samples
int32_t sigma6;
int32_t snr6; // signal to noise ratio
void LPF_Init6(int32_t initial, int32_t size) {
    int i;
    if (size > FILTERMAX)
        size = FILTERMAX; // max
    Size6 = size;
    I6 = Size6 - 1;
    LPFSum6 = Size6 * initial; // prime MACQ with initial data
    for (i = 0; i < Size6; i++) { x6[i] = initial; }
}
// calculate one filter output, called at sampling rate
// Input: new ADC data   Output: filter output, DAC data
// y(n) = (x(n)+x(n-1)+...+x(n-Size-1)/Size
int32_t LPF_Calc6(int32_t newdata) {
    if (I6 == 0) {
        I6 = Size6 - 1; // wrap
    } else {
        I6--; // make room for data
    }
    LPFSum6 = LPFSum6 + newdata - x6[I6]; // subtract oldest, add newest
    x6[I6] = newdata;                     // save new data
    return LPFSum6 / Size6;
}
int32_t Noise6(void) {
    int32_t sum, mean;
    if (Size6 < 2)
        return 0;
    sum = 0;
    for (int i = 0; i < Size6; i++) { sum = sum + x6[i]; }
    mean = sum / Size6; // DC component
    sum = 0;
    for (int i = 0; i < Size6; i++) {
        sum = sum + (x6[i] - mean) * (x6[i] - mean); // total energy in AC part
    }
    sigma6 = sqrt2(sum / (Size6 - 1));
    snr6 = mean / sigma6;
    return sigma6;
}
int32_t Size7;         // Size-point average, Size=2 to FILTERMAX
int32_t x7[FILTERMAX]; // one copy of data in MACQ
uint32_t I7;           // index to oldest
int32_t LPFSum7;       // sum of the last Size samples
int32_t sigma7;
int32_t snr7; // signal to noise ratio
void LPF_Init7(int32_t initial, int32_t size) {
    int i;
    if (size > FILTERMAX)
        size = FILTERMAX; // max
    Size7 = size;
    I7 = Size7 - 1;
    LPFSum7 = Size7 * initial; // prime MACQ with initial data
    for (i = 0; i < Size7; i++) { x7[i] = initial; }
}
// calculate one filter output, called at sampling rate
// Input: new ADC data   Output: filter output, DAC data
// y(n) = (x(n)+x(n-1)+...+x(n-Size-1)/Size
int32_t LPF_Calc7(int32_t newdata) {
    if (I7 == 0) {
        I7 = Size7 - 1; // wrap
    } else {
        I7--; // make room for data
    }
    LPFSum7 = LPFSum7 + newdata - x7[I7]; // subtract oldest, add newest
    x7[I7] = newdata;                     // save new data
    return LPFSum7 / Size7;
}
int32_t Noise7(void) {
    int32_t sum, mean;
    if (Size7 < 2)
        return 0;
    sum = 0;
    for (int i = 0; i < Size7; i++) { sum = sum + x7[i]; }
    mean = sum / Size7; // DC component
    sum = 0;
    for (int i = 0; i < Size7; i++) {
        sum = sum + (x7[i] - mean) * (x7[i] - mean); // total energy in AC part
    }
    sigma7 = sqrt2(sum / (Size7 - 1));
    snr7 = mean / sigma7;
    return sigma7;
}

int32_t u1, u2, u3; // last three points
int32_t Median(int32_t newdata) {
    int32_t result;
    u3 = u2;
    u2 = u1;
    u1 = newdata;
    if (u1 > u2)
        if (u2 > u3)
            result = u2; // u1>u2,u2>u3       u1>u2>u3
        else if (u1 > u3)
            result = u3; // u1>u2,u3>u2,u1>u3 u1>u3>u2
        else
            result = u1; // u1>u2,u3>u2,u3>u1 u3>u1>u2
    else if (u3 > u2)
        result = u2; // u2>u1,u3>u2       u3>u2>u1
    else if (u1 > u3)
        result = u1; // u2>u1,u2>u3,u1>u3 u2>u1>u3
    else
        result = u3; // u2>u1,u2>u3,u3>u1 u2>u3>u1
    return (result);
}

// if the ADC is sampled at 2 kHz
// assuming this filter is executed at 2 kHz
// 60-Hz notch high-Q, IIR filter, assuming fs=2000 Hz
// y(n) = (256x(n) -503x(n-1) + 256x(n-2) + 498y(n-1)-251y(n-2))/256 (2k
// sampling)
long Filter(long data) {
    static long x[6]; // this MACQ needs twice
    static long y[6];
    static unsigned long n = 3; // 3, 4, or 5
    n++;
    if (n == 6)
        n = 3;
    x[n] = x[n - 3] = data; // two copies of new data
    y[n] = (256 * (x[n] + x[n - 2]) - 503 * x[n - 1] + 498 * y[n - 1] -
            251 * y[n - 2] + 128) /
           256;
    // y(n) = (256x(n) -476x(n-1) + 256x(n-2) + 471y(n-1)-251y(n-2))/256 (1k
    // sampling)
    y[n - 3] = y[n]; // two copies of filter outputs too
    return y[n];
}
