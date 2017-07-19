#Copyright (c) 2016, Bertold Van den Bergh
#All rights reserved.
#This work has been developed to support research funded by
#"Fund for Scientific Research, Flanders" (F.W.O.-Vlaanderen).
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of the author nor the
#      names of its contributors may be used to endorse or promote products
#      derived from this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR DISTRIBUTOR BE LIABLE FOR ANY
#DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

DEVICE  = attiny841
F_CPU   = 8000000
AVRDUDE = avrdude -c avrisp -p $(DEVICE) -b 57600

CCARCH=avr
CC=$(CCARCH)-gcc
SIZE=$(CCARCH)-size
OBJCOPY=$(CCARCH)-objcopy

CFLAGS=-c -Wall -Os -DF_CPU=$(F_CPU) -mmcu=$(DEVICE)
LDFLAGS=-mmcu=$(DEVICE)

EXECUTABLE=tune
INCLUDES=
SOURCES=main.c

OBJECTS_OBJ=$(addprefix obj/,$(SOURCES:.c=.o))
INCLUDES_SRC=$(addprefix src/,$(INCLUDES))
SOURCES_SRC=$(addprefix src/,$(SOURCES))

all: $(EXECUTABLE).hex

flash0: $(EXECUTABLE).hex
	sudo $(AVRDUDE) -U flash:w:$(EXECUTABLE).hex:i -P /dev/ttyUSB0

flash1: $(EXECUTABLE).hex
	sudo $(AVRDUDE) -U flash:w:$(EXECUTABLE).hex:i -P /dev/ttyUSB1

fuse:
	sudo $(AVRDUDE) -U efuse:w:0xF5:m -U hfuse:w:0xDD:m -U lfuse:w:0xE2:m 

$(EXECUTABLE).hex: obj/$(EXECUTABLE).elf
	$(OBJCOPY) obj/$(EXECUTABLE).elf -O ihex $(EXECUTABLE).hex	
	
obj/$(EXECUTABLE).elf: $(OBJECTS_OBJ)
	$(CC) $(LDFLAGS) $(OBJECTS_OBJ) -o $@
	$(SIZE) $@	
	
obj/%.o: src/%.c $(INCLUDES_SRC)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS_OBJ) obj/$(EXECUTABLE).elf $(EXECUTABLE).hex

nice:
	touch $(addsuffix .orig,$(INCLUDES_SRC))
	touch $(addsuffix .orig,$(SOURCES_SRC))
	astyle --style=k/r --indent=spaces=4 --indent-cases --indent-switches  $(INCLUDES_SRC) $(SOURCES_SRC)
	mv $(addsuffix .orig,$(INCLUDES_SRC)) bak/
	mv $(addsuffix .orig,$(SOURCES_SRC)) bak/
