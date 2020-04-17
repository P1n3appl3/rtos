.cpu cortex-m4
.fpu fpv4-sp-d16
.syntax unified
.thumb
.text

.thumb_func
.global start_critical
start_critical:
    MRS   R0, PRIMASK
    CPSID I
    BX    LR

.thumb_func
.global end_critical
end_critical:
    MSR PRIMASK, R0
    BX  LR
