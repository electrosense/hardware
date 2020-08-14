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

#include "tca6408a.h"
#include "shell.h"
#include "chprintf.h"
#include "../../util/util.h"

static bool TCA6408AWriteRegisters(TCA6408Driver* driver, bool forceWrite, uint8_t outputValues, uint8_t pinDirections)
{
    if(forceWrite || (outputValues != driver->outputValues)) {
        if(forceWrite || !driver->busIoDisabled) {
            if(i2cSafeWriteRegStandard(driver->config->i2cPort, driver->config->i2cAddr, 0x1, outputValues) != MSG_OK) {
                return false;
            }
        }
        driver->outputValues = outputValues;
    }

    if(forceWrite) {
        /*
         * What is the point of a 'polarity inversion' register, when you can just invert
         * the data you send?
         */
        if(i2cSafeWriteRegStandard(driver->config->i2cPort, driver->config->i2cAddr, 0x2, 0x0) != MSG_OK) {
            return false;
        }
    }

    if(forceWrite || (pinDirections != driver->pinDirections)) {
        if(forceWrite || !driver->busIoDisabled) {
            if(i2cSafeWriteRegStandard(driver->config->i2cPort, driver->config->i2cAddr, 0x3, pinDirections) != MSG_OK) {
                return false;
            }
        }
        driver->pinDirections = pinDirections;
    }

    return true;
}

static bool TCA6408ADoMonitorTask(void* param)
{
    TCA6408Driver* driver = (TCA6408Driver*) param;

    osalMutexLock(&driver->mutex);
    TCA6408AWriteRegisters(driver, true, driver->outputValues, driver->pinDirections);
    osalMutexUnlock(&driver->mutex);

    return true;
}

static bool TCA6408SetPinMode(const GPIOPort* context, uint8_t pin, uint8_t pinMode)
{
    TCA6408Driver* driver = (TCA6408Driver*)context->driver;

    /* Start as input */
    uint32_t value = 1<<pin;

    /* Change to output if requested */
    if(pinMode == PAL_MODE_OUTPUT_PUSHPULL) {
        value = 0;
    }

    osalMutexLock(&driver->mutex);
    uint32_t pinDirections = (driver->pinDirections & ~(1<<pin)) | value;
    bool result = TCA6408AWriteRegisters(driver, false, driver->outputValues, pinDirections);
    osalMutexUnlock(&driver->mutex);
    return result;
}

static uint8_t TCA6408GetPinMode(const GPIOPort* context, uint32_t pin)
{
    TCA6408Driver* driver = (TCA6408Driver*)context->driver;

    if(driver->pinDirections & (1<<pin)) {
        return PAL_MODE_INPUT;
    } else {
        return PAL_MODE_OUTPUT_PUSHPULL;
    }
}

static bool TCA6408SetPins(const GPIOPort* context, uint32_t pinsToChange, uint32_t newValue)
{
    TCA6408Driver* driver = (TCA6408Driver*)context->driver;

    osalMutexLock(&driver->mutex);
    newValue = (driver->outputValues & ~pinsToChange) | (newValue & pinsToChange);
    bool result = TCA6408AWriteRegisters(driver, false, newValue, driver->pinDirections);
    osalMutexUnlock(&driver->mutex);
    return result;
}

static bool TCA6408GetPins(const GPIOPort* context, uint32_t* value)
{
    TCA6408Driver* driver = (TCA6408Driver*)context->driver;

    osalMutexLock(&driver->mutex);

    /* Are there any input pins? */
    if(!driver->pinDirections) {
        *value = driver->outputValues;
        osalMutexUnlock(&driver->mutex);
        return true;
    }

    uint8_t mergedValue = driver->outputValues & (~driver->pinDirections);
    uint8_t tmp;

    if(!driver->busIoDisabled) {
        if(i2cSafeReadRegStandard(driver->config->i2cPort, driver->config->i2cAddr, 0x0, &tmp) != MSG_OK) {
            osalMutexUnlock(&driver->mutex);
            return false;
        }
        driver->readbackValues = tmp;
    } else {
        tmp = driver->readbackValues;
    }

    mergedValue |= tmp & (driver->pinDirections);
    *value = mergedValue;
    osalMutexUnlock(&driver->mutex);

    return true;

}

static void TCA6408BusIoDisable(const GPIOPort* context, bool disable)
{
    TCA6408Driver* driver = (TCA6408Driver*)context->driver;

    osalMutexLock(&driver->mutex);

    if(driver->busIoDisabled==1 && !disable) {
        TCA6408AWriteRegisters(driver, true, driver->outputValues, driver->pinDirections);
    }

    if(disable) {
        driver->busIoDisabled++;
    } else if(driver->busIoDisabled) {
        driver->busIoDisabled--;
    }

    osalMutexUnlock(&driver->mutex);
}


static void TCA6408PrintStatus(const GPIOPort* context, BaseSequentialStream* chp)
{
    TCA6408Driver* driver = (TCA6408Driver*)context->driver;

    uint32_t pins;
    chprintf(chp, "\tDriver: TCA6408A (I2C 0x%02x)"SHELL_NEWLINE_STR, driver->config->i2cAddr);
    bool result = TCA6408GetPins(context, &pins);
    if(result) {
        for(int i=0; i<8; i++) {
            chprintf(chp, "\tPin %u (",i);
            if(driver->pinDirections & _BV(i)) {
                chprintf(chp, "Input):  ");
            } else {
                chprintf(chp, "Output): ");
            }
            chprintf(chp, "%u"SHELL_NEWLINE_STR, (pins & _BV(i)) > 0);
        }
    }

    /* Register 0 needs to be read back */
    chprintf(chp, "\tRegisters:"SHELL_NEWLINE_STR);
    uint8_t reg0;
    if(i2cSafeReadRegStandard(driver->config->i2cPort, driver->config->i2cAddr, 0x0, &reg0) == MSG_OK) {
        chprintf(chp, "\t\t0: %02x"SHELL_NEWLINE_STR, reg0);
    } else {
        chprintf(chp, "\t\t0: xx"SHELL_NEWLINE_STR);
    }
    chprintf(chp, "\t\t1: %02x"SHELL_NEWLINE_STR, driver->outputValues);
    chprintf(chp, "\t\t2: 00"SHELL_NEWLINE_STR);
    chprintf(chp, "\t\t1: %02x"SHELL_NEWLINE_STR, driver->pinDirections);
    chprintf(chp, "\tBus IO Disabled: %u"SHELL_NEWLINE_STR, driver->busIoDisabled);

}

static const GPIOPortFunctions TCA6408Functions = {
    .setMode = TCA6408SetPinMode,
    .getMode = TCA6408GetPinMode,
    .setValue = TCA6408SetPins,
    .getValue = TCA6408GetPins,
    .busIoDisable = TCA6408BusIoDisable,
    .status = TCA6408PrintStatus
};

bool TCA6408AInit(TCA6408Driver* driver, GPIOPort* gpioPort, const TCA6408Driver_config* config)
{
    if(!driver) {
        driver = pvPortMalloc(sizeof(*driver));
    }

    driver->config = config;
    driver->outputValues = 0x00;
    driver->pinDirections = 0xFF;
    driver->busIoDisabled = false;
    osalMutexObjectInit(&driver->mutex);

    bool result = TCA6408AWriteRegisters(driver, true, driver->outputValues, driver->pinDirections);
    if(!result) {
        return false;
    }

    monitorEntryRegister(&driver->ioMon, &TCA6408ADoMonitorTask, driver, "TCA6408");
    monitorKick(&driver->ioMon, true);

    if(gpioPort) {
        gpioPort->driver = driver;
        gpioPort->numPins = 8;
        gpioPort->functions = &TCA6408Functions;
    }

    return true;
}



