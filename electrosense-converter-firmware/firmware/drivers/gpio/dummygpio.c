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
#include "shell.h"
#include "chprintf.h"

static bool gpioDummySetPinMode(const GPIOPort* driver, uint8_t pin, uint8_t pinMode)
{
    (void)driver;
    (void)pin;
    (void)pinMode;

    syslog("Use of dummy GPIO %u!", (uint32_t)driver->driver);

    return false;
}

static uint8_t gpioDummyGetPinMode(const GPIOPort* driver, uint32_t pin)
{
    (void)driver;
    (void)pin;

    syslog("Use of dummy GPIO %u!", (uint32_t)driver->driver);

    return PAL_MODE_INPUT;
}

static bool gpioDummySetValue(const GPIOPort* driver, uint32_t pinsToChange, uint32_t newValue)
{
    (void)driver;
    (void)pinsToChange;
    (void)newValue;

    syslog("Use of dummy GPIO %u!", (uint32_t)driver->driver);

    return false;
}

static bool gpioDummyGetValue(const GPIOPort* driver, uint32_t* value)
{
    (void)driver;
    (void)value;

    syslog("Use of dummy GPIO %u!", (uint32_t)driver->driver);

    return false;
}

void gpioDummyStatus(const GPIOPort* driver, BaseSequentialStream* chp)
{
    (void)driver;

    chprintf(chp, "\tDriver: Dummy"SHELL_NEWLINE_STR);
}

const GPIOPortFunctions gpioDummyFunctions = {
    .setMode = gpioDummySetPinMode,
    .getMode = gpioDummyGetPinMode,
    .setValue = gpioDummySetValue,
    .getValue = gpioDummyGetValue,
    .status = gpioDummyStatus
};

