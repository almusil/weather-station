PROJECT = node
BUILD_DIR = bin

# Path to init file
GDB_INIT = ../.gdbinit
# Path to mass storage for upload
REMOTE_DIR ?= NODE_L031K6/

# You shouldn't have to edit anything below here.
SHARED_DIR = ../lib
INCLUDE_DIR = ../lib/include
CFILES = main.c
CFILES += periph/low_power.c periph/rtc.c periph/spi.c periph/sys_tick.c periph/usart.c periph/adc.c
CFILES += rfm69.c

DEVICE=stm32l031k6
OOCD_FILE = target/stm32l0.cfg

VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(INCLUDE_DIR))
OPENCM3_DIR ?=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
