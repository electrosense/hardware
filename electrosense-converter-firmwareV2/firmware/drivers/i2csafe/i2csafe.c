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

#include "../i2csafe/i2csafe.h"
#include "../../util/syslog/syslog.h"
#include "hal.h"

#include <string.h>


static inline void i2cSafeRawSetClock(I2CDriver* i2c, bool level)
{
    i2cSafeConfig* config = (i2cSafeConfig*)i2c->i2cSafeConfig;
    gpioSetPin(config->sclPin, level);
}
static inline bool i2cSafeRawGetClock(I2CDriver* i2c)
{
    i2cSafeConfig* config = (i2cSafeConfig*)i2c->i2cSafeConfig;
    return gpioGetPin(config->sclPin);
}

static inline void i2cSafeRawSetData(I2CDriver* i2c, bool level)
{
    i2cSafeConfig* config = (i2cSafeConfig*)i2c->i2cSafeConfig;
    gpioSetPin(config->sdaPin, level);
}
static inline bool i2cSafeRawGetData(I2CDriver* i2c)
{
    i2cSafeConfig* config = (i2cSafeConfig*)i2c->i2cSafeConfig;
    return gpioGetPin(config->sdaPin);
}

static inline void i2cSafeSoftwareControl(I2CDriver* i2c)
{
    /* Set SDA and SCL high */
    i2cSafeRawSetClock(i2c, true);
    i2cSafeRawSetData(i2c, true);

    /* Disconnect the pins from the peripheral and put them under software control */
    i2cSafeConfig* config = (i2cSafeConfig*)i2c->i2cSafeConfig;
    gpioSetPinMode(config->sdaPin, PAL_MODE_OUTPUT_OPENDRAIN);
    gpioSetPinMode(config->sclPin, PAL_MODE_OUTPUT_OPENDRAIN);
    
    /* Stop the I2C peripheral, otherwise it crashes */
    i2cStop(i2c);

}

static inline void i2cSafeRawHardwareControl(I2CDriver* i2c)
{
    /* Start the peripheral */
    i2cSafeConfig* config = (i2cSafeConfig*)i2c->i2cSafeConfig;
    i2cStart(i2c, &config->i2cfg);

    /* Attach the pins to the peripheral*/
    gpioSetPinMode(config->sclPin, config->peripheralMode);
    gpioSetPinMode(config->sdaPin, config->peripheralMode);

}

static i2c_result i2cSafeClockGoHigh(I2CDriver* i2c)
{
    uint8_t j;
    i2cSafeRawSetClock(i2c, true);
    vPortBusyDelay(i2cSafe_CYCLE_DELAY);

    /* Wait for SCL to go high (in case of clock stretching) */
    for(j=5; !i2cSafeRawGetClock(i2c); j--) {
        osalThreadSleepMilliseconds(1);
        if(j == 0) {
            /* SCL totally jammed */
            return I2C_BUS_STUCK_SCL_PULLED_LOW;
        }
    }

    return I2C_BUS_OK;
}

static i2c_result i2cSafeClockGoLow(I2CDriver* i2c)
{
    i2cSafeRawSetClock(i2c, false);
    vPortBusyDelay(i2cSafe_CYCLE_DELAY);

    /* Check if it is indeed low */
    if(i2cSafeRawGetClock(i2c)) {
        return I2C_BUS_STUCK_SCL_PULLED_HIGH;
    }
    
    return I2C_BUS_OK;
}

static i2c_result i2cSafeDataGoHigh(I2CDriver* i2c)
{
    i2cSafeRawSetData(i2c, true);
    vPortBusyDelay(i2cSafe_CYCLE_DELAY);

    /* Check if it is indeed high */
    if(!i2cSafeRawGetData(i2c)) {
        return I2C_BUS_STUCK_SDA_PULLED_LOW;
    }
    
    return I2C_BUS_OK;
}

static i2c_result i2cSafeDataGoLow(I2CDriver* i2c)
{
    i2cSafeRawSetData(i2c, false);
    vPortBusyDelay(i2cSafe_CYCLE_DELAY);

    /* Check if it is indeed low */
    if(i2cSafeRawGetData(i2c)) {
        return I2C_BUS_STUCK_SDA_PULLED_HIGH;
    }
    
    return I2C_BUS_OK;
}

static i2c_result i2cSafeSendClocks(I2CDriver* i2c, uint8_t clocks, bool checkData)
{
    i2c_result retVal = I2C_BUS_OK;
    uint8_t i;

    for(i=0; i<clocks; i++) {
        /* Set SCL high */
        if((retVal = i2cSafeClockGoHigh(i2c)) != I2C_BUS_OK) goto done;        
        /* Set SCL low */
        if((retVal = i2cSafeClockGoLow(i2c)) != I2C_BUS_OK) goto done;
        if(checkData && !i2cSafeRawGetData(i2c)){
            return I2C_BUS_STUCK_SHORTED_TOGETHER;
        } 
    }
done:
    return retVal;
}

/* This function will try to reset the I2C bus in case it got stuck */
static i2c_result i2cSafeRawUnclogBus(I2CDriver* i2c)
{
    i2c_result retVal;

    /* Take control */
    i2cSafeSoftwareControl(i2c);

    vPortBusyDelay(i2cSafe_CYCLE_DELAY);
    
    /* Set SDA high to NACK, don't care if it actually goes high */
    i2cSafeDataGoHigh(i2c);

    /* Send 8 transactions */
    if((retVal = i2cSafeSendClocks(i2c, 8*9, false)) != I2C_BUS_OK) goto done;        
    if((retVal = i2cSafeClockGoHigh(i2c)) != I2C_BUS_OK) goto done;        

    /* Data should be high now */
    if(!i2cSafeRawGetData(i2c)){
        retVal = I2C_BUS_STUCK_SDA_PULLED_LOW;
        goto done;
    }

    /* Now send a start condition */
    if((retVal = i2cSafeDataGoLow(i2c)) != I2C_BUS_OK) goto done;        
    if((retVal = i2cSafeClockGoLow(i2c)) != I2C_BUS_OK) goto done;        

    /* Read from 0x7F */
    i2cSafeDataGoHigh(i2c);
    if((retVal = i2cSafeSendClocks(i2c, 9, true)) != I2C_BUS_OK) goto done;        
    
    /* Now send a stop condition */
    if((retVal = i2cSafeDataGoLow(i2c)) != I2C_BUS_OK) goto done;        
    if((retVal = i2cSafeClockGoHigh(i2c)) != I2C_BUS_OK) goto done;        
    if((retVal = i2cSafeDataGoHigh(i2c)) != I2C_BUS_OK) goto done;        

done:
    i2cSafeRawHardwareControl(i2c);

    return retVal;
}

i2c_result i2cSafeMasterTransmitTimeoutWithRetry (
    I2CDriver* i2c,
    i2caddr_t devAddr,
    const uint8_t* txbuf,
    size_t txbytes,
    uint8_t* rxbuf,
    size_t rxbytes,
    systime_t timeoutPerTry,
    unsigned int maxTries)
{

    msg_t status = I2C_BUS_OK;
    i2c_result retVal;
    unsigned int i;

    if(!devAddr) return MSG_RESET;

    memset(rxbuf, 0xFE, rxbytes);

    for(i=0; i<maxTries; i++) {
        status = i2cMasterTransmitTimeout(i2c, devAddr, txbuf, txbytes, rxbuf, rxbytes, timeoutPerTry);
        if(status == MSG_OK){
            retVal = status;
            goto done;
        }

        osalSysLock();
        i2c->i2cErrors++;
        osalSysUnlock();

        /* Attempt to unclog */
        if((retVal = i2cSafeRawUnclogBus(i2c))) {
            syslog("I2C error, bus failure: %s.", i2cSafeResultToString(retVal));
            goto done;
        }
    }

    syslog("I2C error, %u failed attempts.", maxTries);

    retVal = I2C_BUS_RESET;
done:
    return retVal;
}

i2c_result i2cSafeReadRegBulkStandard(I2CDriver* i2c,
                                      uint8_t devAddr,
                                      uint8_t addr,
                                      uint8_t* values,
                                      unsigned int len)
{
    osalDbgAssert(i2c != NULL, "i2c == NULL");

    i2c_result i2c_status;
    uint8_t txBuf[1] = {addr};

    i2cAcquireBus(i2c);
    i2c_status = i2cSafeMasterTransmitTimeoutWithRetry(i2c, devAddr, txBuf,
                 sizeof(txBuf),
                 values,
                 len,
                 OSAL_MS2ST(5),
                 3);
    i2cReleaseBus(i2c);

    return i2c_status;
}


i2c_result i2cSafeWriteRegBulkStandard(I2CDriver* i2c,
                                       uint8_t devAddr,
                                       uint8_t addr,
                                       uint8_t* values,
                                       unsigned int len)
{
    osalDbgAssert(i2c != NULL, "i2c == NULL");

    i2c_result i2c_status;
    uint8_t txBuf[1 + len];

    txBuf[0]=addr;
    memcpy(&txBuf[1], values, len);

    i2cAcquireBus(i2c);
    i2c_status = i2cSafeMasterTransmitTimeoutWithRetry(i2c, devAddr, txBuf,
                 sizeof(txBuf),
                 NULL,
                 0,
                 OSAL_MS2ST(5),
                 3);
    i2cReleaseBus(i2c);

    return i2c_status;
}

i2c_result i2cSafeReadRegStandard(I2CDriver* i2c,
                                  uint8_t devAddr,
                                  uint8_t addr,
                                  uint8_t* value)
{
    osalDbgAssert(i2c != NULL, "i2c == NULL");

    /* The hardware/driver does not support 1 byte reads */
    uint8_t values[2];
    i2c_result result = i2cSafeReadRegBulkStandard(i2c, devAddr, addr, values, sizeof(values));
    *value=values[0];

    return result;
}

i2c_result i2cSafeWriteRegStandard(I2CDriver* i2c,
                                   uint8_t devAddr,
                                   uint8_t addr,
                                   uint8_t value)
{
    osalDbgAssert(i2c != NULL, "i2c == NULL");

    return i2cSafeWriteRegBulkStandard(i2c, devAddr, addr, &value, 1);
}

void i2cSafeInit(I2CDriver* i2c, const i2cSafeConfig* config)
{
    i2c->i2cSafeConfig = (void*)config;

    /* Configure IO for peripheral */
    i2cSafeRawHardwareControl(i2c);
}

i2c_result i2cSafeTestBus(I2CDriver* i2c)
{
    i2cAcquireBus(i2c);

    i2c_result retVal = i2cSafeRawUnclogBus(i2c);

    i2cReleaseBus(i2c);

    return retVal;
}


const char* i2cSafeResultToString(i2c_result result)
{
    if(result == I2C_BUS_OK) return "OK";
    else if(result == I2C_BUS_TIMEOUT) return "Timeout";
    else if(result == I2C_BUS_RESET) return "Transfer Error";
    else if(result == I2C_BUS_STUCK_SCL_PULLED_LOW) return "SCL stuck low";
    else if(result == I2C_BUS_STUCK_SDA_PULLED_LOW) return "SDA stuck low";
    else if(result == I2C_BUS_STUCK_SCL_PULLED_HIGH) return "SCL stuck high";
    else if(result == I2C_BUS_STUCK_SDA_PULLED_HIGH) return "SDA stuck high";
    else if(result == I2C_BUS_STUCK_SHORTED_TOGETHER) return "SDA/SCL shorted together";
    else return "Unknown";
}

uint32_t i2cSafeGetNumberOfErrors(I2CDriver* i2c)
{
    osalSysLock();
    uint32_t result = i2c->i2cErrors;
    osalSysUnlock();
    return result;
}

