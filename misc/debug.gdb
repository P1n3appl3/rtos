target remote | openocd -c "source [find board/ek-tm4c123gxl.cfg]" -c "gdb_port pipe; log_output out/openocd.log"
set history filename out/gdb_history.log
monitor reset halt
tui enable
break main
continue
