target remote | openocd -c "source [find board/ek-tm4c123gxl.cfg]" -c "gdb_port pipe; log_output out/openocd.log"
set history filename out/gdb_history.log
monitor reset halt
set backtrace limit 32

define show_heap
    printf "Heap starts at 0x%08x\n", &_heap
    echo \nFree Nodes:\n
    set $current = head
    printf "0x%08x size: %d\n", $current, $current->size
    while $current->next
        set $prev = $current
        set $current = $current->next
        set $temp = heap_next_node($prev) 
        if $temp != $current
            printf "%d bytes in use\n", ($current - $temp) * sizeof(HeapNode)
        end
        printf "0x%08x size: %d\n", $current, $current->size
    end
end

define show_threads
    set $current = current_thread
    set $max = thread_count
    printf "%s -> ", $current->name
    while $current->next_tcb != current_thread
        set $current=$current->next_tcb
        printf "%s -> ", $current->name
        set $max = $max - 1
        if $max < 0
            Quit
        end
    end
    echo (loops)\n
end

define show_all_threads
    set $current = 0
    set $count = thread_count
    printf "Current: %s\n", current_thread->name
    while $count > 0
        if threads[$current].alive
            set $count = $count - 1
            set $temp = &threads[$current]
            printf "%d: %-15snext: %-15sasleep: %d  blocked: %d  priority: %d\n", \
            $current, $temp->name, $temp->next_tcb->name, $temp->asleep, \
            $temp->blocked, $temp->priority
        end
        set $current = $current + 1
    end
end

tui enable
winheight cmd +10
layout asm
layout src
break main
continue
