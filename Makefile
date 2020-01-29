SRCS = $(wildcard src/*.c) $(wildcard lib/*.c)
OBJS = $(addprefix obj/,$(notdir $(SRCS:.c=.o)))

OS = $(shell lsb_release -si)

ifeq ($(OS), Arch)
	LIB_PREFIX = /usr
	LIB_SUFFIX = v7e-m+fp/hard
else
	LIB_PREFIX = /usr/lib
	LIB_SUFFIX = v7e-m/fpv4-sp
endif

CC = clang --target=thumbv7em-unknown-none-eabi -Wno-keyword-macro -fshort-enums
HEADERS = -I$(LIB_PREFIX)/arm-none-eabi/include
LIBS = -L$(LIB_PREFIX)/arm-none-eabi/lib/thumb/$(LIB_SUFFIX)
LIBS += -L/usr/lib/gcc/arm-none-eabi/9.2.0

CFLAGS = -ggdb -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -Wall -pedantic
CFLAGS += -fdata-sections -ffunction-sections -MD -std=c99
CFLAGS += $(HEADERS) -mfloat-abi=hard -O0

OPENOCD = openocd -c 'source [find board/ek-tm4c123gxl.cfg]'

all: out/out.elf

obj/%.o: src/%.c
	$(CC) -o $@ $^ -Iinc $(CFLAGS) -c

obj/%.o: lib/%.c
	$(CC) -o $@ $^ -Iinc $(CFLAGS) -c

out/out.elf: $(OBJS)
	ld.lld -o $@ $^ $(LIBS) -lgcc -lc_nano -lnosys -lm \
		-T misc/tm4c.ld -u _printf_float -u _scanf_float

flash: out/out.elf
	$(OPENOCD) -c "program out/out.elf verify exit"

run: out/out.elf
	$(OPENOCD) -c "program out/out.elf verify reset exit"

uart: run
	screen -L /dev/ttyACM0 115200

debug: flash
	gdb-multiarch out/out.elf -x misc/debug.gdb

debug_gui: flash
	gdbgui -g arm-none-eabi-gdb --gdb-args="-command=misc/debug_gui.gdb" \
		out/out.elf

size: out/out.elf
	llvm-nm  --demangle --print-size --size-sort --no-weak --radix=d \
	out/out.elf | cut -f 2,4 -d ' ' | numfmt --field 1 --to=iec \
	--padding -6 | sed '/^0/d'

disassemble: out/out.elf
	llvm-objdump out/out.elf -d -S -C --arch-name=thumb --no-show-raw-insn \
	--no-leading-addr > out/disassembly.asm

clean:
	-rm -rf obj
	-rm -rf out

$(shell mkdir -p out)
$(shell mkdir -p obj)

.PHONY: all clean debug debug_gui flash run size
