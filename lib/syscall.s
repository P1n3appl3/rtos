.cpu cortex-m4
.fpu fpv4-sp-d16
.syntax unified
.thumb
.text

.global svcall_handler
.extern OS_SVC_handler

.thumb_func
svcall_handler:
    PUSH {r0-r3, r12, lr}
    MOV   r1, sp                 // Set pointer to parameters
    ADD   r0, r1, #48            // find lr on stack with sketchy hardcoded offset
    LDR   r0, [r0]               // load lr from stack
    LDRH  r0, [r0,#-2]           // Thumb: Load halfword and...
    BIC   r0, r0, #0xFF00        // ...extract comment field

    // r0 now contains SVC number
    // r1 now contains pointer to stacked registers

    BL      OS_SVC_handler         // Call main part of handler
    POP {r0-r3, r12, lr}
    BX LR
.end
