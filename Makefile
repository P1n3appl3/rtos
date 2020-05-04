build_dir = out
target = $(build_dir)/out.elf

SRCS = $(wildcard src/*.c) $(wildcard lib/*.c)
ASM_SRCS = $(wildcard src/*.s) $(wildcard lib/*.s)
OBJS = $(addprefix $(build_dir)/,$(notdir $(SRCS:.c=.o)))
OBJS += $(addprefix $(build_dir)/,$(notdir $(ASM_SRCS:.s=.o)))
DEPS = $(OBJS:%.o=%.d)

CC = clang --target=thumbv7em-unknown-none-eabi -Wno-keyword-macro -fshort-enums
ASSEMBLER = clang --target=thumbv7em-unknown-none-eabi

ARCHFLAGS = -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -mfloat-abi=hard
COMMONFLAGS = -ggdb3 -nodefaultlibs -nostdlib -Wall -Os

CFLAGS = $(COMMONFLAGS) $(ARCHFLAGS) -fdata-sections -ffunction-sections
CFLAGS += -pedantic -ffreestanding -MD -MP -std=c2x -Iinc

OPENOCD = openocd -c "source [find board/ek-tm4c123gxl.cfg]"

SHELL := /bin/zsh

all: $(target)

$(build_dir)/%.o: src/%.c Makefile
	$(CC) -o $@ $< -c $(CFLAGS)

$(build_dir)/%.o: lib/%.c Makefile
	$(CC) -o $@ $< -c $(CFLAGS) -D SSID_NAME='"${wifi_network}"' \
		-D PASSKEY='"${wifi_pass}"'

$(build_dir)/%.o: lib/%.s Makefile
	$(ASSEMBLER) -o $@ $< -c $(COMMONFLAGS) $(ARCHFLAGS)

$(target): $(OBJS)
	$(CC) -o $@ $^ $(COMMONFLAGS) -T misc/tm4c.ld

release:
	$(CC) src/*.c lib/*.c lib/*.s $(CFLAGS) -O2 -T misc/tm4c.ld -o $(target)

-include $(DEPS)

timestamp = $(build_dir)/timestamp
$(timestamp): $(target)
	$(shell date '+%y%m%d%H%M%S' > $(timestamp))
	$(OPENOCD) -c "program $(target) exit"

flash: $(timestamp)

run: flash
	$(OPENOCD) -c "init;reset;shutdown"

uart: run
	screen -L /dev/ttyACM0 115200

debug: flash
	arm-none-eabi-gdb $(target) -x misc/debug.gdb

debug_gui: flash
	gdbgui -g arm-none-eabi-gdb --gdb-args="-command=misc/debug_gui.gdb" \
		$(target)

size = $(build_dir)/size
$(size): $(target)
	llvm-nm --demangle --print-size --size-sort --no-weak --radix=d $(target) \
		> $(build_dir)/sizes

rom_size: $(size)
	@grep "^0" < $(build_dir)/sizes | cut -f 2,4 -d ' ' | \
		numfmt --field 1 --to=iec --padding -6 | sed "/^0/d"

ram_size: $(size)
	@grep "^[^0]" < $(build_dir)/sizes | cut -f 2,4 -d ' ' | \
		numfmt --field 1 --to=iec --padding -6 | sed "/^0/d"

space: $(target)
	@llvm-size $(target) | tail -1 | read -r rom data bss rest \
		&& ram=$$(expr $$data + $$bss) \
		&& echo "ROM: $$(numfmt --to=iec $$rom)/256K ($$(expr $$rom \* 100 / 262144)%)" \
		&& echo "RAM: $$(numfmt --to=iec $$ram)/32K ($$(expr $$ram \* 100 / 32768)%)"

disassemble: $(target)
	arm-none-eabi-objdump $(target) -S -C \
		--no-show-raw-insn > $(build_dir)/disassembly.asm

clean:
	-rm -rf $(build_dir)

$(shell mkdir -p $(build_dir))

.PHONY: all clean debug debug_gui run ram_size rom_size space
