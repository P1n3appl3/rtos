.cpu cortex-m4
.fpu fpv4-sp-d16
.syntax unified
.thumb
.text

.global pendsv_handler
.extern OS_SVC_handler

.thumb_func
svcall_handler:
    // STMFD   sp!, {r0-r3, r12, lr}  // Store registers
    PUSH {r0-r3, r12, lr}
    MOV     r1, sp                 // Set pointer to parameters
    LDRH  r0, [lr,#-2]           // Thumb: Load halfword and...
    BIC   r0, r0, #0xFF00        // ...extract comment field

    // r0 now contains SVC number
    // r1 now contains pointer to stacked registers

    BL      OS_SVC_handler         // Call main part of handler
    // LDMFD   sp!, {r0-r3, r12, pc}^ // Restore registers and return
    // TODO: figure out if ^ is necessary
    POP {r0-r3, r12, pc}
    BX LR
.end
