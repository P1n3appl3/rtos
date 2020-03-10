.cpu cortex-m4
.fpu fpv4-sp-d16
.syntax unified
.thumb
.text

.global start_critical
.global end_critical

.thumb_func
start_critical:
    MRS   R0, PRIMASK
    CPSID I
    BX    LR

.thumb_func
end_critical:
    MSR PRIMASK, R0
    BX  LR
