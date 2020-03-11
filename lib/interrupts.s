.cpu cortex-m4
.fpu fpv4-sp-d16
.syntax unified
.thumb
.text

.global start_critical_orig
.global end_critical_orig

.thumb_func
start_critical_orig:
    MRS   R0, PRIMASK
    CPSID I
    BX    LR

.thumb_func
end_critical_orig:
    MSR PRIMASK, R0
    BX  LR
