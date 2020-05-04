-   investigate static analysis for max stack usage (gcc fstack-usage)
-   LTO
-   printf rewrite
-   more optimal sleep scheduling (like periodic with one-shot timers)
-   check rom vs normal driverlib speed/space
-   better UART blocking: for TX don't just busy wait when the buffer is full,
    and for RX have a proper semaphore
-   mutual exclusion for LCD display functions
-   Document resources used (timers/adc channels/pins/uarts/spi)
-   64 bit time for more range (currently limited to 53 seconds)
