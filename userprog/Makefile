target = user.elf

CC = arm-none-eabi-gcc
STRIP = arm-none-eabi-strip
OBJCPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -mfloat-abi=hard
CFLAGS += -nodefaultlibs -nostdlib -nostartfiles -ffreestanding
CFLAGS += -fdata-sections -ffunction-sections -Wall -pedantic -std=c2x
CFLAGS += -fpic -msingle-pic-base -mpic-register=r9 -mno-pic-data-is-text-relative
CFLAGS += -Wl,-z,max-page-size=1  # reduces padding after program headers
CFLAGS += -Wl,--gc-sections -fno-plt -T user.ld -Os -g
# CFLAGS += -mpoke-function-name # TODO: check if this works for backtraces

all: $(target)

$(target): user.c Makefile user.ld
	$(CC) user.c $(CFLAGS) -o user_debug.elf
	$(STRIP) user_debug.elf -o $@
	$(OBJCPY) --remove-section .ARM.attributes --remove-section .comment $@

clean:
	-rm *.elf *.o

