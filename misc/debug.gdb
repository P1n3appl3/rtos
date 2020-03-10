target remote | openocd -c "source [find board/ek-tm4c123gxl.cfg]" -c "gdb_port pipe; log_output out/openocd.log"
set history filename out/gdb_history.log
monitor reset halt

define show_threads
    set $current = current_thread
    echo Current thread linked list: (top is current thread)\n
    p $current->name
    while $current->next_tcb != current_thread
        set $current=$current->next_tcb
        p $current->name
    end
end

tui enable
winheight cmd +10
layout asm
layout src
break main
continue
