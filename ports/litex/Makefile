# This file is part of the CircuitPython project: https://circuitpython.org
#
# SPDX-FileCopyrightText: Copyright (c) 2019 Scott Shawcroft for Adafruit Industries
#
# SPDX-License-Identifier: MIT

include ../../py/circuitpy_mkenv.mk

CROSS_COMPILE = riscv64-unknown-elf-

#######################################
# CFLAGS
#######################################

INC += -I.
INC += -I../..
INC += -I$(BUILD)
INC += -I$(BUILD)/genhdr
INC += -I./boards
INC += -I./boards/$(BOARD)
INC += -I./peripherals
INC += -I../../lib/mp-readline
INC += -I../../lib/tinyusb/src
INC += -I../../supervisor/shared/usb


#Debugging/Optimization
ifeq ($(DEBUG), 1)
  CFLAGS += -ggdb
  # You may want to enable these flags to make setting breakpoints easier.
  CFLAGS += -fno-inline -fno-ipa-sra
  OPTIMIZATION_FLAGS ?= -Og
else
  CFLAGS += -DNDEBUG -ggdb3
  OPTIMIZATION_FLAGS ?= -O2 -fno-inline-functions
endif

# option to override compiler optimization level, set in boards/$(BOARD)/mpconfigboard.mk
CFLAGS += $(OPTIMIZATION_FLAGS)

CFLAGS += $(INC) -Werror -Wall -std=gnu11 -nostdlib -fshort-enums $(BASE_CFLAGS) $(C_DEFS) $(CFLAGS_MOD) $(COPT) -Werror=missing-prototypes -Werror=old-style-definition

# TODO: check this
CFLAGS += -D__START=main -DFOMU

LD_FILE := boards/$(BOARD)/$(BOARD)-spi.ld

LDFLAGS = $(CFLAGS) -Wl,-nostdlib -Wl,-T,$(LD_FILE) -Wl,-Map=$@.map -Wl,-cref -Wl,-gc-sections -specs=nano.specs -Wl,-melf32lriscv
LIBS := -lgcc -lc


LDFLAGS += -ffreestanding -nostartfiles -Wl,--gc-section -Wl,-Bstatic -Wl,-melf32lriscv -nostartfiles \
			-Wl,--no-warn-mismatch \
			-Wl,--build-id=none

# Use toolchain libm if we're not using our own.
ifndef INTERNAL_LIBM
LIBS += -lm
endif

# TinyUSB defines
CFLAGS += -DCFG_TUSB_MCU=OPT_MCU_VALENTYUSB_EPTRI -DCFG_TUD_CDC_RX_BUFSIZE=1024 -DCFG_TUD_CDC_TX_BUFSIZE=1024 -DCFG_TUD_MSC_BUFSIZE=4096 -DCFG_TUD_MIDI_RX_BUFSIZE=128 -DCFG_TUD_MIDI_TX_BUFSIZE=128


######################################
# source
######################################


SRC_C += \
	background.c \
	mphalport.c \
	boards/$(BOARD)/board.c \
	boards/$(BOARD)/pins.c

ifneq ($(CIRCUITPY_USB),0)
SRC_C += lib/tinyusb/src/portable/valentyusb/eptri/dcd_eptri.c
endif

SRC_S_UPPER = \
	crt0-vexriscv.S \
	supervisor/shared/cpu_regs.S

$(BUILD)/lib/tlsf/tlsf.o: CFLAGS += -Wno-cast-align

ifneq ($(FROZEN_MPY_DIR),)
FROZEN_MPY_PY_FILES := $(shell find -L $(FROZEN_MPY_DIR) -type f -name '*.py')
FROZEN_MPY_MPY_FILES := $(addprefix $(BUILD)/,$(FROZEN_MPY_PY_FILES:.py=.mpy))
endif

OBJ += $(PY_O) $(SUPERVISOR_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_COMMON_HAL_SHARED_MODULE_EXPANDED:.c=.o))
ifeq ($(INTERNAL_LIBM),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_LIBM:.c=.o))
endif
OBJ += $(addprefix $(BUILD)/, $(SRC_CIRCUITPY_COMMON:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S_UPPER:.S=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_MOD:.c=.o))

$(BUILD)/$(FATFS_DIR)/ff.o: COPT += -Os
$(filter $(PY_BUILD)/../extmod/vfs_fat_%.o, $(PY_O)): COPT += -Os

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(SRC_SUPERVISOR) $(SRC_MOD) $(SRC_COMMON_HAL_SHARED_MODULE_EXPANDED) $(SRC_CIRCUITPY_COMMON)
# Sources that only hold QSTRs after pre-processing.
SRC_QSTR_PREPROCESSOR +=


all: $(BUILD)/firmware.bin $(BUILD)/firmware.dfu

ifeq ($(VALID_BOARD),)
$(BUILD)/firmware.elf: invalid-board
else
$(BUILD)/firmware.elf: $(OBJ)
	$(STEPECHO) "LINK $@"
	$(Q)$(CC) -o $@ $(LDFLAGS) $^ -Wl,--print-memory-usage -Wl,--start-group $(LIBS) -Wl,--end-group
	$(Q)$(SIZE) $@ | $(PYTHON) $(TOP)/tools/build_memory_info.py $(LD_FILE) $(BUILD)
endif

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(STEPECHO) "Create $@"
	$(Q)$(OBJCOPY) -O binary $^ $@
#   $(Q)$(OBJCOPY) -O binary -j .vectors -j .text -j .data $^ $@

$(BUILD)/firmware.hex: $(BUILD)/firmware.elf
	$(STEPECHO) "Create $@"
	$(Q)$(OBJCOPY) -O ihex $^ $@
#   $(Q)$(OBJCOPY) -O ihex -j .vectors -j .text -j .data $^ $@

$(BUILD)/firmware.dfu: $(BUILD)/firmware.bin
	$(ECHO) "Create $@"
	$(PYTHON) $(TOP)/tools/dfu.py -b $^ -D 0x1209:0x5bf0 "$(BUILD)/firmware.dfu"

include $(TOP)/py/mkrules.mk
