#Copyright (c) 2017, Bertold Van den Bergh

DEVICE  = cortex-m3
PORTABLE= portable/GCC/ARM_CM3


CCARCH=arm-none-eabi
CC=$(CCARCH)-gcc
AR=$(CCARCH)-ar

GCCPATH=$(shell $(CC) -print-search-dirs | awk '/install/{print $$2}')

#Most CFLAGs are ignored when an LTO compatible linker is used, but not otherwise. This is why they have been set.
CFLAGS=-c -Wall -Os -mcpu=$(DEVICE) -fomit-frame-pointer -falign-functions=16 -ffunction-sections -fdata-sections -fno-common -mthumb -flto -I include -I . -I $(PORTABLE)
ARFLAGS=--plugin $(GCCPATH)/liblto_plugin.so

EXECUTABLE=libFreeRTOS

SOURCES_SRC = $(wildcard *.c) 
PORT_SRC = $(wildcard $(PORTABLE)/*.c)
INCLUDES_SRC = $(wildcard include/*.h) $(wildcard $(PORTABLE)/*.h) FreeRTOSConfig.h

OBJECTS_OBJ = $(addprefix obj/,$(SOURCES_SRC:.c=.o)) $(addprefix obj/,$(notdir $(PORT_SRC:.c=.o)))

all: $(EXECUTABLE).a

$(EXECUTABLE).a: $(OBJECTS_OBJ)
	$(AR) rcs $(@) $(OBJECTS_OBJ) $(ARFLAGS) 
	
obj/%.o: %.c $(INCLUDES_SRC)
	@mkdir -p obj
	$(CC) $(CFLAGS) $< -o $@

obj/%.o: $(PORTABLE)/%.c $(INCLUDES_SRC)
	@mkdir -p obj
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS_OBJ) $(EXECUTABLE).a
	rm -rf obj/
