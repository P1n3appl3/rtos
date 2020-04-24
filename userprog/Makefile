target = user.elf

CC = arm-none-eabi-gcc
STRIP = arm-none-eabi-strip
OBJCPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -mfloat-abi=hard
CFLAGS += -nodefaultlibs -nostdlib -nostartfiles -ffreestanding
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -Wall -pedantic -std=c2x
CFLAGS += -fpic -msingle-pic-base -mpic-register=r9 -mno-pic-data-is-text-relative
CFLAGS += -Wl,-z,max-page-size=1  # reduces padding after program headers
CFLAGS += -Wl,--gc-sections -T user.ld -Os

all: $(target)

$(target): user.c Makefile user.ld
	$(CC) user.c $(CFLAGS) -o user_debug.elf
	$(STRIP) user_debug.elf -o $@
	$(OBJCPY) --remove-section .ARM.attributes --remove-section .comment $@

llvm: # having problems with padding
	clang user.c --target=thumbv7em-unknown-none-eabi -nodefaultlibs -nostdlib \
		-Wall -pedantic -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -mfloat-abi=hard \
		-Os -ffreestanding -std=c2x -o llvm_debug.elf \
		-fpic -ffunction-sections -fdata-sections -g -Wl,-z,norelro \
		-Wl,-z,max-page-size=0x1000 -Wl,-z,common-page-size=0x1000
	llvm-strip llvm_debug.elf -o llvm.elf

clean:
	-rm *.elf *.o

