.section .text
.balign 2
.thumb

.global current_tcb

// systick_handler:
//     CPSID   I
//     PUSH    {R4-R11}
//     LDR     R0, =current_tcb
//     LDR     R1, [R0]
//     STR     SP, [R1]
//     LDR     R1, [R1,#4]
//     STR     R1, [R0]
//     LDR     SP, [R1]
//     POP     {R4 - R11}
//     CPSIE I
//     BX LR
