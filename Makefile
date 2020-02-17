target = out.elf
build_dir = out

SRCS = $(wildcard src/*.c) $(wildcard lib/*.c)
ASM_SRCS = $(wildcard src/*.s) $(wildcard lib/*.s)
OBJS = $(addprefix $(build_dir)/,$(notdir $(SRCS:.c=.o)))
OBJS += $(addprefix $(build_dir)/,$(notdir $(ASM_SRCS:.s=.o)))
DEPS = $(OBJS:%.o=%.d)

CC = clang --target=thumbv7em-unknown-none-eabi -Wno-keyword-macro -fshort-enums
ASSEMBLER = clang --target=thumbv7em-unknown-none-eabi 

ASMFLAGS = -ggdb3 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -Wall -c -mthumb
ASMFLAGS += -mfloat-abi=hard -Os
CFLAGS = -ggdb3 -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -Wall -pedantic
CFLAGS += -fdata-sections -ffunction-sections -MD -MP -std=c2x -ffreestanding
CFLAGS += -mfloat-abi=hard -Iinc -c -Os
RELEASEFLAGS = -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -Wall -pedantic
RELEASEFLAGS += -std=c2x -ffreestanding -mfloat-abi=hard -Iinc -T misc/tm4c.ld
RELEASEFLAGS += -nodefaultlibs -nostdlib -O3

OPENOCD = openocd -c 'source [find board/ek-tm4c123gxl.cfg]'

all: $(build_dir)/$(target)

release:
	$(CC) $(RELEASEFLAGS) -o out/out.elf $(SRCS)

$(build_dir)/%.o: src/%.c
	$(CC) -o $@ $< $(CFLAGS)

$(build_dir)/%.o: lib/%.c
	$(CC) -o $@ $< $(CFLAGS)

$(build_dir)/%.o: lib/%.s
	$(ASSEMBLER) -o $@ $< $(ASMFLAGS)

$(build_dir)/$(target): $(OBJS)
	# TODO: LTO
	ld.lld -o $@ $^ -T misc/tm4c.ld

-include $(DEPS)

flash: $(build_dir)/$(target)
	$(OPENOCD) -c "program $(build_dir)/$(target) verify exit"

run: $(build_dir)/$(target)
	$(OPENOCD) -c "program $(build_dir)/$(target) verify reset exit"

uart: run
	screen -L /dev/ttyACM0 115200

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
