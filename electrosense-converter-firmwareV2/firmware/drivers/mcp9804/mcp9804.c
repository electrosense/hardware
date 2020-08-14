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

#include "mcp9804.h"
#include "../i2csafe/i2csafe.h"
#include <string.h>
#include "../system.h"

int16_t MCP9804MeasureTemperature(MCP9804Driver* driver, bool newMeasurement)
{
    if(!newMeasurement) {
        return driver->temperature;
    }

    uint8_t data[2];
    i2c_result result = i2cSafeReadRegBulkStandard(driver->config->i2cPort, driver->config->i2cAddr, 0x5, data, 2);

    if(result != I2C_BUS_OK) {
        driver->temperature = 0x7FFF;
    } else {
        uint16_t temp = ((data[0] & 0xF) << 8) | data[1];

        driver->temperature = temp;

        /* Check sign */
        if(data[0] & 0x10) {
            driver->temperature = 4096 - temp;
        }
    }

    return driver->temperature;
}

bool MCP9804Init(MCP9804Driver* driver, const MCP9804Driver_config* config)
{
    memset(driver, 0, sizeof(*driver));

    driver->config = config;

    /* Read device id */
    uint8_t devId;
    if(i2cSafeReadRegStandard(driver->config->i2cPort, driver->config->i2cAddr, 0x7, &devId) != MSG_OK) {
        return false;
    }

    if(devId != 0x2) {
        return false;
    }

    /* Set resolution to the highest */
    if(i2cSafeWriteRegStandard(driver->config->i2cPort, driver->config->i2cAddr, 0x8, 0x3) != MSG_OK) {
        return false;
    }

    driver->temperature = MCP9804MeasureTemperature(driver, true);

    return (driver->temperature != 0x7FFF);
}

