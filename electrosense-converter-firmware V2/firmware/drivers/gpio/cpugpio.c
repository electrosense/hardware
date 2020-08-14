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

#include "../system.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "../../util/util.h"


static bool gpioCPUSetPinMode(const GPIOPort* driver, uint8_t pin, uint8_t pinMode)
{
    (void)driver;
    (void)pin;
    (void)pinMode;

    palSetPadMode((ioportid_t)driver->driver, pin, pinMode);

    return true;
}

static uint8_t gpioCPUGetPinMode(const GPIOPort* driver, uint32_t pin)
{
    (void)driver;
    (void)pin;

    /* The HAL does not provide this. This is obviously CPU specific */
    syssts_t sts = osalSysGetStatusAndLockX();

    uint64_t cr;
    cr = (uint64_t)(((ioportid_t)driver->driver)->CRH) << 32;
    cr |= ((ioportid_t)driver->driver)->CRL;

    osalSysRestoreStatusX(sts);

    uint8_t config = (cr >> (4*pin)) & 0xF;

    static const uint8_t cfgToMode[] = {
        PAL_MODE_INPUT_ANALOG,
        PAL_MODE_OUTPUT_PUSHPULL,
        PAL_MODE_OUTPUT_PUSHPULL,
        PAL_MODE_OUTPUT_PUSHPULL,
        PAL_MODE_INPUT,
        PAL_MODE_OUTPUT_OPENDRAIN,
        PAL_MODE_OUTPUT_OPENDRAIN,
        PAL_MODE_OUTPUT_OPENDRAIN,
        PAL_MODE_INPUT,
        PAL_MODE_STM32_ALTERNATE_PUSHPULL,
        PAL_MODE_STM32_ALTERNATE_PUSHPULL,
        PAL_MODE_STM32_ALTERNATE_PUSHPULL,
        PAL_MODE_UNCONNECTED, /* Reserved */
        PAL_MODE_STM32_ALTERNATE_OPENDRAIN,
        PAL_MODE_STM32_ALTERNATE_OPENDRAIN,
        PAL_MODE_STM32_ALTERNATE_OPENDRAIN,
    };

    return cfgToMode[config];
}

static bool gpioCPUSetValue(const GPIOPort* driver, uint32_t pinsToChange, uint32_t newValue)
{
    (void)driver;
    (void)pinsToChange;
    (void)newValue;

    //syssts_t sts = osalSysGetStatusAndLockX();
    palWriteGroup((ioportid_t)driver->driver, pinsToChange, 0, newValue);
    //osalSysRestoreStatusX(sts);

    return true;
}

static bool gpioCPUGetValue(const GPIOPort* driver, uint32_t* value)
{
    (void)driver;
    (void)value;

    *value = palReadPort((ioportid_t)driver->driver);

    return true;
}

static const char* gpioCPUpinModeString(const GPIOPort* driver, uint8_t pin)
{
    uint8_t mode = gpioCPUGetPinMode(driver, pin);
    switch(mode) {
        case PAL_MODE_INPUT_ANALOG:
            return "Analog";
        case PAL_MODE_UNCONNECTED:
            return "Unconnected";
        case PAL_MODE_OUTPUT_PUSHPULL:
            return "Output";
        case PAL_MODE_INPUT:
            return "Input";
        case PAL_MODE_OUTPUT_OPENDRAIN:
            return "Open drain";
        case PAL_MODE_STM32_ALTERNATE_PUSHPULL:
            return "Alternate";
        case PAL_MODE_STM32_ALTERNATE_OPENDRAIN:
            return "Alternate open drain";
    }
    return "Unknown";
}

void gpioCPUStatus(const GPIOPort* driver, BaseSequentialStream* chp)
{
    (void)driver;

    chprintf(chp, "\tDriver: HAL (MM 0x%08x)"SHELL_NEWLINE_STR, (uint32_t)driver->driver);

    for(uint8_t i=0; i<16; i++) {
        printfFixed(chp, 32, "\tPin %u (%s):",
                    i,
                    gpioCPUpinModeString(driver, i));

        chprintf(chp,  "%u"SHELL_NEWLINE_STR, palReadPad((ioportid_t)driver->driver, i));
    }
}


static const GPIOPortFunctions gpioCPUFunctions = {
    .setMode = gpioCPUSetPinMode,
    .getMode = gpioCPUGetPinMode,
    .setValue = gpioCPUSetValue,
    .getValue = gpioCPUGetValue,
    .status = gpioCPUStatus
};

void gpioCPUInit(uint8_t gpioPortId, ioportid_t port)
{
    GPIOPort* portDriver = gpioRegisterPortDriver(gpioPortId);
    if(!portDriver) {
        return;
    }

    portDriver->driver = port;
    portDriver->functions = &gpioCPUFunctions;
    portDriver->numPins = 16;
}
