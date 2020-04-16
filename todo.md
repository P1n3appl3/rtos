-   move the timerproc periodic thread initialization into the OS so it stops
    breaking stuff when we forget it
-   only flash when necessary (allow parallel make for compilation but with
    serial steps like flash -> debug)
-   stack overflow protection with MPU
-   investigate static analysis for max stack usage (gcc fstack-usage)
-   printf rewrite
-   more optimal sleep scheduling (like periodic with one-shot timers)
-   check rom vs normal driverlib speed/space
-   better UART blocking for TX (don't just busy wait when the buffer is full)
-   mutual exclusion for all LCD display functions
-   core temp with adc
-   LTO
