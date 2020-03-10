.cpu cortex-m4
.fpu fpv4-sp-d16
.syntax unified
.thumb
.text

.extern current_thread
.extern context_switch_hook
.global pendsv_handler

.thumb_func
pendsv_handler:
    CPSID I
    PUSH {R4 - R11}

// For debugging context switches in C
    // PUSH {LR}
    // BL context_switch_hook
    // POP  {LR}

    LDR  R0, =current_thread    // R0 = &current_thread
    LDR  R1, [R0]               // R1 = current_thread
    STR  SP, [R1]               // SP = *current_thread aka current_thread->sp
    LDR  R1, [R1,#4]            // R1 = current_thread->next_tcb;
    STR  R1, [R0]               // current_thread = current_thread->next_tcb;
    LDR  SP, [R1]               // SP = current_thread->sp
    POP  {R4 - R11}
    CPSIE I
    BX LR
