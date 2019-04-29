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

static void cmdI2CUsage(BaseSequentialStream *chp)
{
    chprintf(chp, "Usage:"SHELL_NEWLINE_STR);
    chprintf(chp, "\ti2c test"SHELL_NEWLINE_STR);
    chprintf(chp, "\ti2c scan"SHELL_NEWLINE_STR);
    chprintf(chp, "\ti2c errors"SHELL_NEWLINE_STR);
    chprintf(chp, "\ti2c get devAddr regAddr [stress]"SHELL_NEWLINE_STR);
    chprintf(chp, "\ti2c set devAddr regAddr regValue [stress]"SHELL_NEWLINE_STR);
}

bool cmdI2CIO(I2CDriver* i2c, BaseSequentialStream *chp, int argc, char *argv[])
{
    bool wantToRead;
    uint8_t regValue;
    bool stress = false;
    if(!strcmp(argv[0], "set")) {
        if(argc<4) return false;
        if(argc>4) stress = true;
        regValue = strToInt(argv[3], 16);
        wantToRead = false;
    } else if(!strcmp(argv[0], "get")) {
        if(argc<3) return false;
        if(argc>3) stress = true;
        wantToRead = true;
    } else {
        return false;
    }

    uint8_t devAddr = strToInt(argv[1], 16);
    uint8_t regAddr = strToInt(argv[2], 16);

    uint32_t stressCnt = 0;

    do {
        i2c_result result;
        if(wantToRead) {
            result = i2cSafeReadRegStandard(i2c, devAddr, regAddr, &regValue);
        } else {
            result = i2cSafeWriteRegStandard(i2c, devAddr, regAddr, regValue);
        }

        if(result != I2C_BUS_OK) {
            regValue = 0;
        }

        if(stress) {
            printfFixed(chp, 7, "%u", stressCnt);
        }
        chprintf(chp, "Status: %s, address: 0x%02x, register: 0x%02x, value: 0x%02x"SHELL_NEWLINE_STR,
                 i2cSafeResultToString(result),
                 devAddr,
                 regAddr,
                 regValue);
        stressCnt++;
    } while (stress && stressCnt <= 100000);
    return true;
}

void cmdI2CScan(I2CDriver* i2c, BaseSequentialStream *chp)
{
    uint8_t regValue;

    chprintf(chp, "   ");
    for(int addr = 0; addr <= 0xF; addr++) {
        chprintf(chp, "%1x  ", addr);
    }
    for(int addr = 0; addr <= 0x7F; addr++) {
        if(addr % 16 == 0) {
            chprintf(chp, SHELL_NEWLINE_STR"%1x ", addr/16);
        }
        i2c_result result = i2cSafeReadRegStandard(i2c, addr, 0, &regValue);
        if(result == I2C_BUS_OK) {
            chprintf(chp, "%02x ", addr);
        } else if(result == I2C_BUS_RESET) {
            chprintf(chp, "-- ");
        } else {
            chprintf(chp, "EE ");
        }
    }
    chprintf(chp, SHELL_NEWLINE_STR);
}

void cmdI2C(void* user, BaseSequentialStream *chp, int argc, char *argv[])
{
    I2CDriver* i2c = (I2CDriver*)user;

    if(argc == 0) {
        cmdI2CUsage(chp);
        return;
    }

    if(!strcmp(argv[0], "test")) {
        chprintf(chp, "Verdict: %s"SHELL_NEWLINE_STR, i2cSafeResultToString(i2cSafeTestBus(i2c)));
    } else if(cmdI2CIO(i2c, chp, argc, argv)) {
    } else if(!strcmp(argv[0], "errors")) {
        chprintf(chp, "I2C Errors: %u"SHELL_NEWLINE_STR, i2cSafeGetNumberOfErrors(i2c));
    } else if(!strcmp(argv[0], "scan")) {
        cmdI2CScan(i2c, chp);
    } else {
        cmdI2CUsage(chp);
        return;
    }
}
