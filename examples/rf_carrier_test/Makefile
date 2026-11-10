ROOT := $(abspath ../..)
BUILD ?= $(ROOT)/_build/rf_carrier_test
CROSS_COMPILE ?= arm-none-eabi-
PYTHON ?= python3

CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size

CPPFLAGS += -DNRF52840_XXAA -I$(ROOT)/lib/nrfx/mdk -I$(ROOT)/src/cmsis/include
CFLAGS += -mcpu=cortex-m4 -mthumb -Os -std=gnu11 \
  -ffreestanding -fno-builtin -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -ffunction-sections -fdata-sections -Wall -Wextra -Werror
LDFLAGS += -nostdlib -Wl,--gc-sections -Wl,-Map=$(BUILD)/rf-carrier-test.map -T linker.ld

.PHONY: all
all: $(BUILD)/rf-carrier-test.uf2

$(BUILD):
	mkdir -p "$@"

$(BUILD)/rf-carrier-test.elf: main.c linker.ld Makefile | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) main.c -o "$@"
	$(SIZE) "$@"

$(BUILD)/rf-carrier-test.hex: $(BUILD)/rf-carrier-test.elf
	$(OBJCOPY) -O ihex "$<" "$@"

$(BUILD)/rf-carrier-test.uf2: $(BUILD)/rf-carrier-test.hex
	$(PYTHON) "$(ROOT)/lib/uf2/utils/uf2conv.py" "$<" -c -f 0xADA52840 -o "$@"
