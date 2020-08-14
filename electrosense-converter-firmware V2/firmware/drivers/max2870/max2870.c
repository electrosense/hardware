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

#include <string.h>

#include "max2870.h"
#include "../../util/util.h"
#include "../../util/syslog/syslog.h"
#include "../usb/usb.h"
#include "chprintf.h"
#include "shell.h"



static void max2870SetMUXOut(MAX2870Driver* driver, uint8_t muxOut);

static void max2870SpiState(MAX2870Driver* driver, bool enable)
{
    if(enable) {
        spiAcquireBus(driver->config->spiPort);
        spiStart(driver->config->spiPort, driver->config->spiConfig);
    } else {
        /* Disable MUX OUT to release SPI bus */
        max2870SetMUXOut(driver, 0x0);

        /* Update monitor state */
        monitorKick(&driver->pllMon, driver->enabled);

        spiReleaseBus(driver->config->spiPort);
    }
}

static bool max2870WriteRegister(MAX2870Driver* driver, const uint32_t addr, uint32_t value, bool force)
{
    osalDbgCheck(addr < 0x6);
    osalDbgCheck((value & 0x7) == 0);

    if(!force && driver->registers[addr] == value) {
        return false;
    }
    driver->registers[addr] = value;

    value |= addr;

    uint8_t data[4] = {value>>24, (value >> 16) & 0xFF, (value >> 8) & 0xFF,  (value >> 0) & 0xFF};


    spiSelect(&SPID1);
    spiSend(&SPID1, 4, data);
    spiUnselect(&SPID1);
    osalSysPolledDelayX(50);


    return true;
}

static bool max2870WriteAllRegisters(MAX2870Driver* driver, uint32_t* registers, bool force)
{
    /* Write in reverse order since some values are double buffered.
     * Reg 0 is always reloaded when any other was written since this triggers tuning */
    bool anyChanged = false;
    for(int i = 5; i>=0; i--) {
        anyChanged |= max2870WriteRegister(driver, i, registers[i], force || (i==0 && anyChanged));
    }
    return anyChanged;
}

static uint8_t max2870PowerToRegister(int8_t* power)
{
    uint8_t registerValue = 0;

    /* Turn the output off if <-4dBm*/
    if(*power < -4) {
        *power = -127;
        return registerValue;
    }

    registerValue = _BV(2); /* Output enabled */

    if(*power >= 5) {
        *power = 5;
        registerValue |= 3;
    } else if(*power >= 2) {
        *power = 2;
        registerValue |= 2;
    } else if(*power >= -1) {
        *power = -1;
        registerValue |= 1;
    } else {
        *power = -4;
    }

    return registerValue;
}

static bool max2870CalculateDividers(MAX2870Driver* driver)
{
    uint32_t register2 = 0;

    /* Calculate reference and stepsize */
    uint16_t pfdDivider = driver->config->referenceToPfdDivider;
    uint32_t pfdFrequency = driver->config->inputFrequency;
    if(!(pfdDivider % 2)) {
        /* Enable extra divider */
        pfdDivider /= 2;
        pfdFrequency /= 2;
        register2 |= _BV(MAX2870_REG2_RDIV2_OFFSET);
    }

    if(pfdDivider > 1023) {
        return false;
    }

    pfdFrequency /= pfdDivider;

    /* Save power */
    if(pfdDivider == 1) {
        pfdDivider = 0;
    }

    register2 |= pfdDivider << MAX2870_REG2_R_OFFSET;

    /* Stepsize possible? */
    if(pfdFrequency % driver->config->stepFrequency) {
        return false;
    }

    /* Calculate the divider */
    uint32_t fracDivider = pfdFrequency / driver->config->stepFrequency;
    if(fracDivider > 4095) {
        return false;
    }

    driver->refDividerRegister2 = register2;
    driver->fracDivider = fracDivider;

    /* Ok, calculate the BS divider. We need to create a clock <=50kHz from referenceFrequency */
    driver->bsValue = (pfdFrequency + 49999) / 50000;
    if(driver->bsValue > 1023) {
        return false;
    }

    driver->pfdFrequency = pfdFrequency;

    return true;
}

static void max2870SetMUXOut(MAX2870Driver* driver, uint8_t muxOut)
{
    uint32_t register2 = driver->registers[2] & ~ (0x7 <<  MAX2870_REG2_MUX_OFFSET);
    register2 |= (muxOut & 0x7) << MAX2870_REG2_MUX_OFFSET;
    uint32_t register5 = driver->registers[5] & ~ _BV(MAX2870_REG5_MUX_OFFSET);
    if(muxOut & 0x8) {
        register5 |= _BV(MAX2870_REG5_MUX_OFFSET);
    }

    /* It seems reg 5 should be written before reg 2 to update the MUX value */
    bool mustUpdateOther = max2870WriteRegister(driver, 5, register5, false);
    max2870WriteRegister(driver, 2, register2, mustUpdateOther);
}

static uint32_t max2870ReadRegister6(MAX2870Driver* driver)
{
    /* Step 1: Set MUX OUT to readback mode */
    max2870SetMUXOut(driver, 0xC);

    /* Step 2: Ask to read the register */
    uint8_t data[5]= {0, 0, 0, 0x6};
    spiSelect(&SPID1);
    spiSend(&SPID1, 4, data);
    spiUnselect(&SPID1);

    /* Step 3: Read in the data, note that the device should be unselected(!) */
    data[3] = 0;
    spiExchange(&SPID1, 4, data, data);

    driver->registers[6] = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    return driver->registers[6];
}

static void max2870Disable(MAX2870Driver* driver)
{
    if(driver->config->gpioChipEnable != 0xffff) {
        gpioSetPin(driver->config->gpioChipEnable, false);
    } else {
        max2870WriteRegister(driver, 2, _BV(MAX2870_REG2_SHDN_OFFSET), true);
    }

    if(driver->config->lockStatus) {
        driver->config->lockStatus(false);
    }
    driver->enabled = false;
}

static MAX2870TuneResult max2870TuneNoLock(MAX2870Driver* driver, MAX2870TuneRequest* tune)
{
    MAX2870TuneResult result = TUNE_OK;
    uint32_t registers[6] = {};

    uint64_t frequency = tune->frequency;

    if(frequency == 0) {
        max2870Disable(driver);
        return TUNE_OK;
    }

    if(frequency > MAX2870_VCO_MAX_FREQUENCY || frequency < MAX2870_VCO_MIN_FREQUENCY/128) {
        return TUNE_FREQUENCY_OUT_OF_RANGE;
    }

    /* The VCO only works between 3 and 6 GHz */
    uint8_t outDivide = 0;
    while(frequency < MAX2870_VCO_MIN_FREQUENCY) {
        frequency *= 2;
        outDivide++;
    }

    /* From now on frequency is defined as number of steps */
    uint32_t steps = frequency / driver->config->stepFrequency;

    /* Calculate N, F and M */
    uint32_t pllN = steps / driver->fracDivider;
    if(pllN < 16 || pllN > 4091) {
        return TUNE_PLL_N_OUT_OF_RANGE;
    }

    uint16_t pllFracF = steps - (pllN * driver->fracDivider);
    uint16_t pllFracM = driver->fracDivider;

    /* Fractional mode needed? */
    if(pllFracF) {
        /* Simplify fraction */
        uint16_t pllFracGcd = gcd(pllFracF, pllFracM);
        //pllFracM=1 is not valid, but this cannot happen with this code
        pllFracM /= pllFracGcd;
        pllFracF /= pllFracGcd;

        /* Set for fractional mode */
        registers[1] |= _BV(MAX2870_REG1_CPL_OFFSET);
    } else {
        /* Set for integer mode */
        registers[0] |= _BV(MAX2870_REG0_INT_OFFSET);
        registers[1] |= _BV(MAX2870_REG1_CPOC_OFFSET);
        registers[2] |= _BV(MAX2870_REG2_LDF_OFFSET);
    }

    registers[1] |= _BV(MAX2870_REG1_P_OFFSET); /* Feature not used, default value */

    /* Write dividers */
    registers[0] |= pllN << MAX2870_REG0_N_OFFSET;
    registers[0] |= pllFracF << MAX2870_REG0_FRAC_OFFSET;
    registers[1] |= pllFracM << MAX2870_REG1_M_OFFSET;
    registers[2] |= driver->refDividerRegister2;
    registers[4] |= outDivide << MAX2870_REG4_DIVA_OFFSET;

    /* Digital lock detect (0110) on mux out (MSB is in reg 5, but zero) */
    registers[2] |= 0x6 <<  MAX2870_REG2_MUX_OFFSET;

    /* Set misc things */
    if(!driver->config->invertingLoopFilter) {
        registers[2] |= _BV(MAX2870_REG2_PDP_OFFSET);
    }

    registers[2] |= driver->config->chargePumpCurrent << MAX2870_REG2_CP_OFFSET;
    registers[2] |= driver->config->spurMode << MAX2870_REG2_SDN_OFFSET;

    if(driver->pfdFrequency > 32000000) {
        registers[2] |= _BV(MAX2870_REG2_LDS_OFFSET);
    }

    if(tune->fastLockDurationMicroseconds) {
        if(driver->config->chargePumpCurrent) {
            return TUNE_CANT_FASTLOCK;
        }

        uint32_t tmp = (uint64_t)driver->pfdFrequency * (uint64_t)tune->fastLockDurationMicroseconds / 1000000;
        tmp /= pllFracM;
        if(tmp>4095) {
            tmp = 4095;
        }

        if(tmp) {
            registers[3] |= _BV(MAX2870_REG3_CDM_OFFSET);
            registers[3] |= tmp << MAX2870_REG3_CDIV_OFFSET;
        }
    }

    if(!tune->outBfundamental) {
        registers[4] |= _BV(MAX2870_REG4_BDIV_OFFSET);
    }
    registers[4] |= (driver->bsValue & 0xFF) << MAX2870_REG4_BS_OFFSET;
    registers[4] |= (driver->bsValue >> 8) << MAX2870_REG4_BS_MSB_OFFSET;
    registers[4] |= _BV(MAX2870_REG4_FB_OFFSET);

    registers[5] = 0;

    /* Look up if we know a manual VCO */
    bool vcoAutoTune = true;
    uint16_t vcoIndex = (frequency - MAX2870_VCO_MIN_FREQUENCY) / MAX2870_VCO_INCREMENT;
    if(vcoIndex > sizeof(driver->vcoCache)) {
        vcoIndex = sizeof(driver->vcoCache);
    }
    if((driver->vcoCache[vcoIndex] & 0x80) && !tune->useVCOAutotune) {
        vcoAutoTune = false;
        registers[3] |= (driver->vcoCache[vcoIndex] & 0x3F) << MAX2870_REG3_VCO_OFFSET;
        registers[3] |= _BV(MAX2870_REG3_VAS_SHDN_OFFSET);
    }

    /* Is the PLL already on? */
    if(!driver->enabled) {
        if(driver->config->gpioChipEnable != 0xffff) {
            gpioSetPin(driver->config->gpioChipEnable, true);
            osalThreadSleepMilliseconds(20);
        }

        /* Force write all registers */
        max2870WriteAllRegisters(driver, registers, true);

        /* Give the device some time to start */
        osalThreadSleepMilliseconds(20);
    }

    uint8_t timeoutCnt;

    uint16_t lock;
    do {
        /* Turn on outputs */
        registers[4] |= max2870PowerToRegister(&tune->powerA) << MAX2870_REG4_APWR_OFFSET;
        registers[4] |= max2870PowerToRegister(&tune->powerB) << MAX2870_REG4_BPWR_OFFSET;
        if(driver->pllMonReloadCounter >= 20) {
            driver->pllMonReloadCounter = 0;
            max2870WriteAllRegisters(driver, registers, true);
        } else {
            max2870WriteAllRegisters(driver, registers, !driver->enabled);
        }

        /* No device is selected, so we can read MUXOUT to see if the PLL is locked */
        timeoutCnt = 0;
        do {
            spiReceive(driver->config->spiPort, sizeof(lock), &lock);

            /* Locking usually takes around 200us, this is a very generous timeout */
            if(timeoutCnt == 255) {
                if(!vcoAutoTune) {
                    /* Clear the VCO selection cache if tuning failed and try again */
                    driver->vcoCache[vcoIndex] = 0;
                    vcoAutoTune = true;
                    registers[3] &=~ _BV(MAX2870_REG3_VAS_SHDN_OFFSET);
                    break;
                } else {
                    result = TUNE_NOT_LOCKED;
                    syslog("MAX2870 Tuning failed.");
                    goto tuneFailed;
                }

            }
            timeoutCnt++;
        } while(lock != 0xFFFF);
    } while(lock != 0xFFFF);

    /* Set actual frequency */
    tune->frequency = ((uint64_t)steps * (uint64_t)driver->config->stepFrequency);
    for(int i=0; i<outDivide; i++) {
        tune->frequency /= 2;
    }

    uint32_t reg6;

tuneFailed:
    reg6 = max2870ReadRegister6(driver);
    if(vcoAutoTune && result==TUNE_OK) {
        /* Cache the current VCO value */
        driver->vcoCache[vcoIndex] = 0x80 | ((reg6 >> MAX2870_REG6_V_OFFSET) & 0x3F);
    }

    tune->adcVoltage = (reg6 >>MAX2870_REG6_ADC_OFFSET) & 7;
    tune->usedVCO = (reg6 >> MAX2870_REG6_V_OFFSET) & 0x3F;
    tune->lockTime = timeoutCnt;
    tune->usedVCOAutotune = vcoAutoTune;
    tune->isLocked = (result == TUNE_OK);

    if(driver->config->lockStatus) {
        driver->config->lockStatus(tune->isLocked);
    }

    /* Notify the mon thread */
    driver->enabled = true;

    /* Save current tuning */
    driver->currentTune = *tune;

    return result;
}

/* This thread is shared among multiple PLLs if there is more than one */
static bool max2870DoMonitorTask(void* param)
{
    MAX2870Driver* driver = (MAX2870Driver*)param;
    bool didWork = false;

    /* Check up on the PLL and see if tuning voltage makes sense */
    max2870SpiState(driver, true);
    if(driver->enabled) {
        /* Every 5 minutes we rewrite the registers to protect against corruption.
         * This gives a small distortion in the output */
        if(driver->pllMonReloadCounter >= 29) {
            max2870WriteAllRegisters(driver, driver->registers, true);
            driver->pllMonReloadCounter = 0;
        } else {
            driver->pllMonReloadCounter++;


            uint32_t reg6 = max2870ReadRegister6(driver);

            driver->currentTune.adcVoltage = (reg6 >>MAX2870_REG6_ADC_OFFSET) & 7;
            driver->currentTune.usedVCO = (reg6 >> MAX2870_REG6_V_OFFSET) & 0x3F;

            if(driver->currentTune.adcVoltage == 0 ||
                    driver->currentTune.adcVoltage == 7 ||
                    !driver->currentTune.isLocked) {
                syslog("MAX2870 Lost lock, retuning.");

                /* Retry tuning */
                driver->currentTune.useVCOAutotune = true;
                max2870TuneNoLock(driver, &driver->currentTune);
            }
        }
        didWork = true;
    }

    max2870SpiState(driver, false);

    return didWork;
}

bool max2870Init(MAX2870Driver* driver, const MAX2870Driver_config* config)
{
    memset(driver, 0, sizeof(*driver));
    driver->config = config;

    driver->currentTune.powerA = -127;
    driver->currentTune.powerB = -127;

    /* Switch off the chip */
    max2870Tune(driver, &driver->currentTune);

    if(!max2870CalculateDividers(driver)) {
        return false;
    }

    monitorEntryRegister(&driver->pllMon, &max2870DoMonitorTask, driver, "MAX2870");

    return true;
}

MAX2870TuneResult max2870Tune(MAX2870Driver* driver, MAX2870TuneRequest* tune)
{
    max2870SpiState(driver, true);
    MAX2870TuneResult result = max2870TuneNoLock(driver, tune);
    max2870SpiState(driver, false);

    return result;
}

bool max2870VcoPrecal(MAX2870Driver* driver)
{
    bool allGood = true;
    bool pllWasEnabled = driver->enabled;
    MAX2870TuneRequest previousTune = driver->currentTune;
    MAX2870TuneRequest tune = {};
    tune.useVCOAutotune = true;
    tune.powerA = -127;
    tune.powerB = -127;
    tune.fastLockDurationMicroseconds = 0;

    max2870SpiState(driver, true);

    for(tune.frequency = MAX2870_VCO_MIN_FREQUENCY + MAX2870_VCO_INCREMENT/2;
            tune.frequency < MAX2870_VCO_MAX_FREQUENCY;
            tune.frequency += MAX2870_VCO_INCREMENT) {

        if(max2870TuneNoLock(driver, &tune)) {
            allGood = false;
        }
    }

    if(!pllWasEnabled) {
        max2870Disable(driver);
    } else {
        max2870TuneNoLock(driver, &previousTune);
    }

    /* Limited sanity check */
    if(driver->vcoCache[0] >= 0x9f ||
    		driver->vcoCache[sizeof(driver->vcoCache)-1] <= 0x9f) {
    	allGood = false;
    }

    max2870SpiState(driver, false);

    return allGood;
}

void max2870StatusPrint(MAX2870Driver* driver, BaseSequentialStream* stdout)
{
    max2870SpiState(driver, true);
    if(driver->enabled) {
        if(driver->currentTune.isLocked) {
            chprintf(stdout, "PLL Locked"SHELL_NEWLINE_STR);
        } else {
            chprintf(stdout, "PLL UNLOCK!!!"SHELL_NEWLINE_STR);
        }

        printfFixed(stdout, 30, "Divided VCO Frequency:");
        chprintf(stdout, "%u kHz"SHELL_NEWLINE_STR, driver->currentTune.frequency/1000);
        printfFixed(stdout, 30, "Output A Power:");
        if(driver->currentTune.powerA != -127) {
            chprintf(stdout, "%d dBm"SHELL_NEWLINE_STR, driver->currentTune.powerA);
        } else {
            chprintf(stdout, "Off"SHELL_NEWLINE_STR);
        }
        printfFixed(stdout, 30, "Output B Power:");
        if(driver->currentTune.powerB != -127) {
            chprintf(stdout, "%d dBm"SHELL_NEWLINE_STR, driver->currentTune.powerB);
        } else {
            chprintf(stdout, "Off"SHELL_NEWLINE_STR);
        }
        if(driver->currentTune.isLocked) {
            printfFixed(stdout, 30, "Lock Time:");
            chprintf(stdout, "%u cycles"SHELL_NEWLINE_STR, driver->currentTune.lockTime);
        }
        printfFixed(stdout, 30, "PFD Frequency:");
        chprintf(stdout, "%u kHz"SHELL_NEWLINE_STR, driver->pfdFrequency/1000);
        printfFixed(stdout, 30, "Step Frequency:");
        chprintf(stdout, "%u Hz"SHELL_NEWLINE_STR, driver->config->stepFrequency);
        printfFixed(stdout, 30, "BS Divider:");
        chprintf(stdout, "%u"SHELL_NEWLINE_STR, driver->bsValue);
        printfFixed(stdout, 30, "Reload Counter:");
        chprintf(stdout, "%u"SHELL_NEWLINE_STR, driver->pllMonReloadCounter);

        printfFixed(stdout, 30, "Current VCO Band From Cache:");
        chprintf(stdout, "%u"SHELL_NEWLINE_STR, !driver->currentTune.usedVCOAutotune);
        printfFixed(stdout, 30, "Current VCO Band:");
        chprintf(stdout, "%u"SHELL_NEWLINE_STR, driver->currentTune.usedVCO);

        printfFixed(stdout, 30, "Current VCO Tuning Voltage:");
        switch(driver->currentTune.adcVoltage) {
            case 0:
                chprintf(stdout, "0.2");
                break;
            case 1:
                chprintf(stdout, "0.6");
                break;
            case 2:
            case 3:
                chprintf(stdout, "1");
                break;
            case 4:
            case 5:
                chprintf(stdout, "1.7");
                break;
            case 6:
                chprintf(stdout, "2.3");
                break;
            default:
                chprintf(stdout, "3");
                break;
        }
        chprintf(stdout, "V"SHELL_NEWLINE_STR"Registers:"SHELL_NEWLINE_STR);
        for(int i=0; i<7; i++) {
            chprintf(stdout, "\t%u: %08x"SHELL_NEWLINE_STR, i, driver->registers[i]);
        }
    } else {
        chprintf(stdout, "Low power mode"SHELL_NEWLINE_STR);
    }
    max2870SpiState(driver, false);
}

void max2870VcoPrint(MAX2870Driver* driver, BaseSequentialStream* stdout)
{
    uint32_t frequency = MAX2870_VCO_MIN_FREQUENCY/1000000 + MAX2870_VCO_INCREMENT/2000000;
    for(unsigned int i=0; i<sizeof(driver->vcoCache); i++) {
        chprintf(stdout, "Frequency: %u MHz, VCO Band: ", frequency);
        uint8_t vco = driver->vcoCache[i];
        if(vco & 0x80) {
            chprintf(stdout, "%u"SHELL_NEWLINE_STR, vco & 0x3f);
        } else {
            chprintf(stdout, "Invalid"SHELL_NEWLINE_STR);
        }
        frequency += MAX2870_VCO_INCREMENT/1000000;
    }
}

bool max2870GetDigitalLockDetect(MAX2870Driver* driver)
{
    uint8_t lock;
    max2870SpiState(driver, true);
    max2870SetMUXOut(driver, 0x6);

    spiReceive(driver->config->spiPort, 1, &lock);

    max2870SpiState(driver, false);
    return lock == 0xFF;
}

const char* max2870TuneResultToString(MAX2870TuneResult result)
{
    if(result == TUNE_OK) return "OK";
    if(result == TUNE_CANT_FASTLOCK) return "Invalid fastlock command";
    if(result == TUNE_NOT_LOCKED) return "Lock failed";
    if(result == TUNE_PLL_N_OUT_OF_RANGE) return "N out of range";
    if(result == TUNE_FREQUENCY_OUT_OF_RANGE) return "Frequency out of range";
    return "Unknown error";
}

MAX2870TuneRequest* max2870getCurrentTuning(MAX2870Driver* driver)
{
    return &driver->currentTune;
}

bool max2870VcoCacheSet(MAX2870Driver* driver, unsigned int vcoIndex, uint8_t value)
{
    if(vcoIndex > sizeof(driver->vcoCache)) {
        return false;
    }

    driver->vcoCache[vcoIndex] = value;
    return true;
}
