.cpu cortex-m4
.fpu fpv4-sp-d16
.syntax unified
.thumb
.text

.global svcall_handler
.extern OS_SVC_handler

.thumb_func
svcall_handler:
    MOV   r1, sp                 // Set pointer to parameters
    ADD   r0, r1, #24            // find lr on stack with sketchy hardcoded offset
    LDR   r0, [r0]               // load lr from stack
    LDRH  r0, [r0,#-2]           // Thumb: Load halfword and...
    BIC   r0, r0, #0xFF00        // ...extract comment field

    // r0 now contains SVC number
    // r1 now contains pointer to stacked registers

    PUSH {lr}
    BL      OS_SVC_handler         // Call main part of handler
    POP {lr}
    BX LR
.end
