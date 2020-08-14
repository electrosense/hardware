#!/bin/sh

arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm data/license.bin data_license.o --rename-section .data=.text
arm-none-eabi-objdump -x data_license.o 

arm-none-eabi-ar rcs libStaticData.a data_license.o

rm data_license.o

