SRCS = $(wildcard src/*.c) $(wildcard lib/*.c)
OBJS = $(addprefix $(build_dir)/,$(notdir $(SRCS:.c=.o)))
DEPS = $(OBJS:%.o=%.d)

target = out.elf
build_dir = out

CC = clang --target=thumbv7em-unknown-none-eabi -Wno-keyword-macro -fshort-enums

CFLAGS = -ggdb -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -Wall -pedantic
CFLAGS += -fdata-sections -ffunction-sections -MD -std=c2x -ffreestanding
CFLAGS += -mfloat-abi=hard -O0 -MP -Iinc -c

OPENOCD = openocd -c 'source [find board/ek-tm4c123gxl.cfg]'

all: $(build_dir)/$(target)

$(build_dir)/%.o: src/%.c
	$(CC) -o $@ $< $(CFLAGS)

$(build_dir)/%.o: lib/%.c
	$(CC) -o $@ $< $(CFLAGS)

$(build_dir)/$(target): $(OBJS)
	ld.lld -o $@ $^ -T misc/tm4c.ld

-include $(DEPS)

flash: $(build_dir)/$(target)
	$(OPENOCD) -c "program $(build_dir)/$(target) verify exit"

run: $(build_dir)/$(target)
	$(OPENOCD) -c "program $(build_dir)/$(target) verify reset exit"

uart: run
	screen -L /dev/ttyACM1 115200

debug: flash
	arm-none-eabi-gdb $(build_dir)/$(target) -x misc/debug.gdb

debug_gui: flash
	gdbgui -g arm-none-eabi-gdb --gdb-args="-command=misc/debug_gui.gdb" \
		$(build_dir)/$(target)

size: $(build_dir)/$(target)
	llvm-nm  --demangle --print-size --size-sort --no-weak --radix=d \
	$(build_dir)/$(target) | cut -f 2,4 -d ' ' | numfmt --field 1 --to=iec \
	--padding -6 | sed '/^0/d'

disassemble: $(build_dir)/$(target)
	llvm-objdump $(build_dir)/$(target) -d -S -C --arch-name=thumb --no-show-raw-insn \
	--no-leading-addr > $(build_dir)/disassembly.asm

clean:
	-rm -rf $(build_dir)

$(shell mkdir -p $(build_dir))

.PHONY: all clean debug debug_gui flash run size
