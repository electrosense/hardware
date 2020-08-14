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

#ifndef DRIVERS_I2CSAFE_I2CSAFE_H_
#define DRIVERS_I2CSAFE_I2CSAFE_H_


#include "hal.h"
#include "../system.h"


/* This results in around 100kHz */
#define i2cSafe_CYCLE_DELAY 400

typedef enum {
    I2C_BUS_OK = MSG_OK,
    I2C_BUS_TIMEOUT = MSG_TIMEOUT,
    I2C_BUS_RESET = MSG_RESET,
    I2C_BUS_STUCK_SCL_PULLED_LOW = 1,
    I2C_BUS_STUCK_SDA_PULLED_LOW = 2,
    I2C_BUS_STUCK_SCL_PULLED_HIGH = 3,
    I2C_BUS_STUCK_SDA_PULLED_HIGH = 4,
    I2C_BUS_STUCK_SHORTED_TOGETHER = 5
} i2c_result;

typedef struct {
    I2CConfig i2cfg;
    uint16_t sclPin;
    uint16_t sdaPin;
    uint32_t peripheralMode;
} i2cSafeConfig;

i2c_result i2cSafeMasterTransmitTimeoutWithRetry (
    I2CDriver* i2c,
    i2caddr_t devAddr,
    const uint8_t* txbuf,
    size_t txbytes,
    uint8_t* rxbuf,
    size_t rxbytes,
    systime_t timeoutPerTry,
    unsigned int maxTries);

i2c_result i2cSafeReadRegBulkStandard(I2CDriver* i2c,
                                      uint8_t devAddr,
                                      uint8_t addr,
                                      uint8_t* values,
                                      unsigned int len);

i2c_result i2cSafeWriteRegBulkStandard(I2CDriver* i2c,
                                       uint8_t devAddr,
                                       uint8_t addr,
                                       uint8_t* values,
                                       unsigned int len);

i2c_result i2cSafeReadRegStandard(I2CDriver* i2c,
                                  uint8_t devAddr,
                                  uint8_t addr,
                                  uint8_t* value);

i2c_result i2cSafeWriteRegStandard(I2CDriver* i2c,
                                   uint8_t devAddr,
                                   uint8_t addr,
                                   uint8_t value);

void i2cSafeInit(I2CDriver* i2c, const i2cSafeConfig* config);

i2c_result i2cSafeTestBus(I2CDriver* i2c);
const char* i2cSafeResultToString(i2c_result result);
uint32_t i2cSafeGetNumberOfErrors(I2CDriver* i2c);


#endif /* DRIVERS_I2CSAFE_I2CSAFE_H_ */
