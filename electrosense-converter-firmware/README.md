# Electrosense converter

This project contains the firmware for the downconverter part of the electrosense.org spectrum sensor. It extends the range of a standard RTL-SDR to >6GHz.

Directory structure:

 * binaries: A folder with precompiled binaries, ready for flashing.
 * bootloader: A slightly modified stm32duino-bootloader. There is no reason to recompile this, just use the esense_boot.bin binary.
 * FreeRTOS: This is the FreeRTOS operating system. If you want to compile the firmware you must run make here first.
 * ChibiOS: Some components from ChibiOS HAL.
 * firmware: This is the actual firmware. Run make here to build the image.

A picture of the hardware this runs on:
![Downconverter board](/images/prototype.png?raw=true)
