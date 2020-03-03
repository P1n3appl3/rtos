target = out.elf
build_dir = out

SRCS = $(wildcard src/*.c) $(wildcard lib/*.c)
ASM_SRCS = $(wildcard src/*.s) $(wildcard lib/*.s)
OBJS = $(addprefix $(build_dir)/,$(notdir $(SRCS:.c=.o)))
OBJS += $(addprefix $(build_dir)/,$(notdir $(ASM_SRCS:.s=.o)))
DEPS = $(OBJS:%.o=%.d)

CC = clang --target=thumbv7em-unknown-none-eabi -Wno-keyword-macro -fshort-enums
ASSEMBLER = clang --target=thumbv7em-unknown-none-eabi

ARCHFLAGS = -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -mfloat-abi=hard
COMMONFLAGS = -ggdb3 -nodefaultlibs -nostdlib -Wall -O0

ASMFLAGS = -c $(COMMONFLAGS) $(ARCHFLAGS)
CFLAGS = -c $(COMMONFLAGS) $(ARCHFLAGS) -fdata-sections -ffunction-sections
CFLAGS += -pedantic -ffreestanding -MD -MP -std=c2x -Iinc

OPENOCD = openocd -c "source [find board/ek-tm4c123gxl.cfg]"

SHELL := /bin/zsh

all: $(build_dir)/$(target)

$(build_dir)/%.o: src/%.c
	$(CC) -o $@ $< $(CFLAGS)

$(build_dir)/%.o: lib/%.c
	$(CC) -o $@ $< $(CFLAGS)

$(build_dir)/%.o: lib/%.s
	$(ASSEMBLER) -o $@ $< $(ASMFLAGS)

$(build_dir)/$(target): $(OBJS)
	$(CC) -o $@ $^ $(COMMONFLAGS) -T misc/tm4c.ld

-include $(DEPS)

flash: $(build_dir)/$(target)
	$(OPENOCD) -c "program $(build_dir)/$(target) verify exit"

run: $(build_dir)/$(target)
	$(OPENOCD) -c "program $(build_dir)/$(target) verify reset exit"

quick_uart: 
	screen -L /dev/ttyACM0 115200

uart: | run quick_uart

quick_debug:
	arm-none-eabi-gdb $(build_dir)/$(target) -x misc/debug.gdb

debug: | flash quick_debug

debug_gui: flash
	gdbgui -g arm-none-eabi-gdb --gdb-args="-command=misc/debug_gui.gdb" \
		$(build_dir)/$(target)

size: $(build_dir)/$(target)
	llvm-nm --demangle --print-size --size-sort -no-weak --radix=d \
		$(build_dir)/$(target) | cut -f 2,4 -d " " | sed "/^ *$$/d" | \
		numfmt --field 1 --to=iec --padding -6 | sed "/^0/d"

space: $(build_dir)/$(target)
	llvm-size $(build_dir)/$(target) | tail -1 | read -r rom data bss rest \
		&& ram=$$(expr $$data + $$bss) \
		&& echo "\nROM: $$(numfmt --to=iec $$rom)/256K ($$(expr $$rom \* 100 / 262144)%)" \
		&& echo "RAM: $$(numfmt --to=iec $$ram)/32K ($$(expr $$ram \* 100 / 32768)%)"

disassemble: $(build_dir)/$(target)
	arm-none-eabi-objdump $(build_dir)/$(target) -S -C \
		--no-show-raw-insn > $(build_dir)/disassembly.asm

clean:
	-rm -rf $(build_dir)

$(shell mkdir -p $(build_dir))

.PHONY: all clean debug debug_gui flash run size space quick_debug
