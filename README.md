## Features

Our OS implements the features covered in
[EE445M](http://users.ece.utexas.edu/~gerstl/ee445m_s20/) with some
modifications and additions. We rewrote the provided hardware drivers to use the
driverlib code embedded in the TM4C's ROM rather than Dr. Valvano's code. Here
are some notable features that extend upon the ones required for the class.

-   We use the MPU to provide stack protection for all threads. The region is
    swapped out on every context switch to ensure that interrupts or preemption
    can't get around the protection and when a stack overflow occurs the system
    is halted and debug information is provided.
-   We use [littlefs](https://github.com/ARMmbed/littlefs) instead of the
    provided FAT filesystem. We added a lot of file related functionality to our
    interpreter so the user can do unix-y commands like `ls`, `cat`, `rm`, `mv`,
    `touch`, etc. and we provide a utility to do file transfers to the SD card
    over UART.
-   In general our interpreter is more robust than required. We provide a
    stripped down readline implementation for basic line editing, allow lots of
    command aliases, include extra functionality like heap profiling and core
    temp monitoring, and put some effort into making it "pretty" with colored
    text. In addition we extended the remote interpreter support to allow
    concurrent execution of multiple interpreters (one running locally over
    serial and one running remotely through the ESP over TCP).
-   Rather than doing load time relocation of ELFs to allow code sharing between
    the OS and user programs, we re-purposed the system call interface to allow
    dynamic loading from any thread at runtime which allows greater flexibility
    as well as the potential to update function definitions in place without
    requiring restarts.
-   We support an arbitrary number of priority levels for foreground threads
    (currently limited to 256 because we only use 1 byte to store the field)
    because we only maintain a linked list for the threads of the "current"
    priority level in the system so no additional space is wasted by having many
    levels.
-   We support an arbitrary number of periodic tasks (with a maximum number
    chosen at compile time) and rather than wasting CPU time with a naive
    periodic timer interrupt that manages the tasks by subtracting from multiple
    counters, we arrange our tasks in an order that allows us to use precise
    timer values that need only one interrupt per task.
-   We aggressively heap allocate data structures and buffers rather than having
    dedicated parts of memory reserved for them. This means that if you're not
    using a certain feature (like the filesystem, UART, or ESP), you don't waste
    any RAM on it. This allows the OS to use very little of the overall system
    memory (less than 10% even with a USB device stack) which makes more of it
    available to the actual workload running on the OS.
-   Our heap optimizes for long running workloads without needing a "stop the
    world" defragmentation pass. As blocks are freed it merges them with
    adjacent ones, and doesn't over allocate (other than for alignment purposes)
    regardless of the requested size. This means that we get pretty good heap
    utilization at the cost of slow allocation and freeing. Performance could be
    increased using a doubly linked list implementation, but we didn't see it as
    necessary because timing critical workloads shouldn't be relying on heap
    interaction in their hot path, and our simpler singly linked list wastes
    less space on block metadata.
-   Our OS is relatively compiler independent. We have used both the LLVM and
    GNU toolchains, and since we don't link to external libraries (including the
    C standard library) it should be relatively easy to compile our OS on a new
    toolchain.

## Final Lab

To demonstrate the capabilities of our OS, our final lab project was to
integrate a USB device stack and use it to remotely control each other's mice
through the remote interpreter. We did this in a way that allowed multiple
movements in different threads to be performed simultaneously such as linear and
circular motions combining to create a spiral. Here is the list of commands
available for our project demo:

```
led COLOR [on, off, or toggle]  control the onboard RGB led
adc                             read a single sample from the ADC
temp                            get the internal system temperature
time [get/reset]                OS time helpers

jitter                          show periodic task jitter stats
heap                            show heap usage information

mount                           mount the sd card
unmount                         unmount the sd card
format yes really               format the sd card
upload FILENAME                 transfer a file over UART
exec FILENAME                   load and run process from file
touch FILENAME                  creates a new file
cat FILENAME                    display the contents of a file
append FILENAME WORD            append a word to a file
ls                              list the files in the directory
mv FILENAME NEWNAME             move a file
cp FILENAME NEWNAME             copy a file
rm FILENAME                     delete a file
checksum FILENAME               compute a checksum of a file

connect [SSID PASS]             connect to a wifi network.
server                          spawn remote interpreter
client IPV4                     spawn remote client
mouse_server                    spawn remote mouse server
mouse_client IPV4               spawn remote mouse controller
mouse                           control the mouse locally
exit                            leave this session
```

And these are the controls from within the mouse control app:

```
q w e      These keys all make the mouse move a straight direction
a s d      Holding shift allows you to make slight adjustments to
z x c      the cursor position. s stops this movement.

, . and / act as the left, middle, and right mouse buttons
normally they perform a single click, but holding shift (so < > and ?)
makes them toggle holding the button.

+ and - increase and decrease the speed of continuous movements

o makes the mouse start circling (press again to stop)

Space makes the cursor move to the center of the screen,
stops all mouse movement, and releases all mouse buttons
```

## Hardware

### GPIO Pins:

| Pin # | Description           |
| ----- | --------------------- |
| A0    | Main UART Rx          |
| A1    | Main UART Tx          |
| A2    | LCD/SDC SPI Clock     |
| A3    | LCD SPI Chip Select   |
| A4    | LCD/SDC SPI MISO      |
| A5    | LCD/SDC SPI MOSI      |
| A6    | LCD/SDC SPI D/C       |
| A7    | LCD Reset             |
| B0    | SDC SPI Chip Select   |
| B1    | ESP Reset             |
| D4    | USB0 D-               |
| D5    | USB0 D+               |
| D6    | ESP UART Rx           |
| D7    | ESP UART Tx           |
| F0    | Launchpad Left Button |
| F1    | Launchpad Red LED     |

In addition we allow use of any of the 12 ADC channels that don't conflict with
the above pins.

### Peripherals

-   UART0 is the main UART
-   UART2 is the ESP UART
-   SPI0 is for the LCD and SDC
-   SysTick is for preemptive thread scheduling
-   Timer1 is for thread sleep timing
-   Timer2 is for periodic task scheduling
-   Timer3 and WideTimer1 are for internal OS busy waiting
-   WideTimer5 is for tracking OS uptime
-   ADC0 Sequence 2 is for core temperature monitoring
-   ADC0 Sequence 3 is reserved for OS processor triggering
-   ADC1 Sequence 0 is reserved for OS timer triggered periodic reads
-   USB0 is for the HID
