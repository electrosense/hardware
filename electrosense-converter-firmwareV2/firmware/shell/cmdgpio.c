/*
 * Copyright (c) 2017, Bertold Van den Bergh (vandenbergh@bertold.org)
 * All rights reserved.
 * This work has been developed to support research funded by
 * "Fund for Scientific Research, Flanders" (F.W.O.-Vlaanderen).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the author nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR DISTRIBUTOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "appshell.h"
#include "chprintf.h"

#include <string.h>
#include <stdlib.h>

static void cmdGPIOUsage(BaseSequentialStream *chp)
{
    chprintf(chp, "Usage:"SHELL_NEWLINE_STR);
    chprintf(chp, "\tgpio status"SHELL_NEWLINE_STR);
    chprintf(chp, "\tgpio [port] [pin] output"SHELL_NEWLINE_STR);
    chprintf(chp, "\tgpio [port] [pin] input"SHELL_NEWLINE_STR);
    chprintf(chp, "\tgpio [port] [pin] set"SHELL_NEWLINE_STR);
    chprintf(chp, "\tgpio [port] [pin] clear"SHELL_NEWLINE_STR);
    chprintf(chp, "\tgpio [port] [pin] get"SHELL_NEWLINE_STR);
    chprintf(chp, "\tgpio [port] bus [on/off]"SHELL_NEWLINE_STR);

}

void cmdGPIO(void* user, BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) user;

    if(argc < 1) {
        cmdGPIOUsage(chp);
        return;
    }

    if(!strcmp(argv[0], "status")) {
        gpioPrintStatus(chp);
        return;
    }


    if(argc == 3) {
        uint8_t portId = strToInt(argv[0], 10);

        if(!strcmp(argv[1], "bus")) {
            if(!strcmp(argv[2], "off")) {
                gpioPortIoDisable(portId, true);
            } else {
                gpioPortIoDisable(portId, false);
            }
        } else {

            uint8_t pinId = strToInt(argv[1], 10);
            uint16_t pin = MAKE_GPIO(portId, pinId);

            if(!strcmp(argv[2], "output")) {
                gpioSetPinMode(pin, PAL_MODE_OUTPUT_PUSHPULL);
            } else if(!strcmp(argv[2], "input")) {
                gpioSetPinMode(pin, PAL_MODE_INPUT);
            } else if(!strcmp(argv[2], "set")) {
                gpioSetPin(pin, true);
            } else if(!strcmp(argv[2], "clear")) {
                gpioSetPin(pin, false);
            } else {
                chprintf(chp, "Value: %u"SHELL_NEWLINE_STR, gpioGetPin(pin));
            }
        }

    } else {
        cmdGPIOUsage(chp);
    }

    //TXA6408PrintStatus(&mixerControllerIO, chp);
}
