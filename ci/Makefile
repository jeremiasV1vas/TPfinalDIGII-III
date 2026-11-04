# Compila el firmware con arm-none-eabi-gcc para verificar que no tenga errores.
# Es lo que corre GitHub Actions en cada PR. Para grabar la placa usamos MCUXpresso.
#
# Uso: make -C ci

CC = arm-none-eabi-gcc
SIZE = arm-none-eabi-size

FW = ../firmware
LIB = $(FW)/CMSISv2p00_LPC17xx

CFLAGS = -mcpu=cortex-m3 -mthumb -O1 -g -Wall -Wextra -std=gnu11
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -I$(FW)/inc -I$(LIB)/inc -I$(LIB)/Drivers/inc -Ibuild/shim

LDFLAGS = -mcpu=cortex-m3 -mthumb -T lpc1769.ld -nostartfiles -Wl,--gc-sections
LDFLAGS += --specs=nano.specs --specs=nosys.specs

# Los drivers de la cátedra no son nuestros: los compilamos sin -Wextra
# para que sus advertencias no tapen las nuestras.
LIB_CFLAGS = $(filter-out -Wextra,$(CFLAGS)) -w

APP_SRC = $(wildcard $(FW)/src/*.c) startup_lpc17xx.c
LIB_SRC = $(LIB)/src/system_LPC17xx.c $(wildcard $(LIB)/Drivers/src/*.c)

BUILD = build
APP_OBJ = $(addprefix $(BUILD)/app/,$(notdir $(APP_SRC:.c=.o)))
LIB_OBJ = $(addprefix $(BUILD)/lib/,$(notdir $(LIB_SRC:.c=.o)))

vpath %.c $(FW)/src . $(LIB)/src $(LIB)/Drivers/src

all: $(BUILD)/firmware.elf
	$(SIZE) $<

# Los drivers incluyen "lpc17xx.h" en minúscula y el archivo se llama LPC17xx.h.
# En Windows da igual, pero en Linux (el CI) no lo encuentra, así que armamos
# un header puente con el nombre en minúscula.
SHIM = $(BUILD)/shim/lpc17xx.h

$(SHIM):
	mkdir -p $(dir $@)
	echo '#include "LPC17xx.h"' > $@

$(APP_OBJ) $(LIB_OBJ): $(SHIM)

$(BUILD)/firmware.elf: $(APP_OBJ) $(LIB_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD)/app/%.o: %.c | $(BUILD)/app
	$(CC) $(CFLAGS) -Werror -c $< -o $@

$(BUILD)/lib/%.o: %.c | $(BUILD)/lib
	$(CC) $(LIB_CFLAGS) -c $< -o $@

$(BUILD)/app $(BUILD)/lib:
	mkdir -p $@

clean:
	rm -rf $(BUILD)

.PHONY: all clean
