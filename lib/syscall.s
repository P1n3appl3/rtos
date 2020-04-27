.cpu cortex-m4
.fpu fpv4-sp-d16
.syntax unified
.thumb
.text

.extern OS_function_lookup

.thumb_func
.global svcall_handler
svcall_handler:
    PUSH {lr}
    BL   OS_function_lookup
    POP  {lr}
    STR  r0, [sp] // save the return value of the function we called to the
                  // stack so that it gets "passed back" to the calling code
    BX LR
