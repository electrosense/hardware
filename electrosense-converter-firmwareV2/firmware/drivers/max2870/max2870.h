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

#include <stdint.h>
#include "../system.h"
#include "../../util/monitor.h"
#include "hal.h"

#ifndef DRIVERS_MAX2870_MAX2870_H_
#define DRIVERS_MAX2870_MAX2870_H_

#define MAX2870_VCO_MIN_FREQUENCY 3000000000
#define MAX2870_VCO_MAX_FREQUENCY 6000000000
#define MAX2870_VCO_INCREMENT 20000000

typedef struct {
    SPIDriver* spiPort;
    const SPIConfig* spiConfig;
    const uint16_t gpioChipEnable;
    bool invertingLoopFilter;
    uint8_t chargePumpCurrent;
    uint8_t spurMode;
    uint32_t inputFrequency;
    uint16_t referenceToPfdDivider;
    uint32_t stepFrequency;
    void(*lockStatus)(bool locked);
} MAX2870Driver_config;

typedef struct {
    uint64_t frequency;
    int8_t powerA, powerB; /* -127 = Off */
    bool outBfundamental;
    uint32_t fastLockDurationMicroseconds;
    bool useVCOAutotune;

    uint8_t usedVCO;
    uint8_t adcVoltage;
    uint8_t lockTime;
    bool isLocked;
    bool usedVCOAutotune;
} MAX2870TuneRequest;

typedef struct MAX2870Driver MAX2870Driver;

typedef struct MAX2870Driver {
    const MAX2870Driver_config* config;

    bool enabled;

    MAX2870TuneRequest currentTune;
    uint8_t vcoCache[(MAX2870_VCO_MAX_FREQUENCY-MAX2870_VCO_MIN_FREQUENCY) / MAX2870_VCO_INCREMENT];

    uint32_t pfdFrequency;

    uint16_t fracDivider;
    uint32_t refDividerRegister2;
    uint16_t bsValue;

    uint32_t registers[7];

    uint8_t pllMonReloadCounter;
    MonitorEntry pllMon;
} MAX2870Driver;

typedef enum {
    TUNE_OK = 0,
    TUNE_FREQUENCY_OUT_OF_RANGE = -1,
    TUNE_PLL_N_OUT_OF_RANGE = -2,
    TUNE_CANT_FASTLOCK = -3,
    TUNE_NOT_LOCKED = -4
} MAX2870TuneResult;

bool max2870Init(MAX2870Driver* driver, const MAX2870Driver_config* config);
MAX2870TuneResult max2870Tune(MAX2870Driver* driver, MAX2870TuneRequest* tune);
bool max2870VcoPrecal(MAX2870Driver* driver);

void max2870StatusPrint(MAX2870Driver* driver, BaseSequentialStream* stdout);
void max2870VcoPrint(MAX2870Driver* driver, BaseSequentialStream* stdout);
bool max2870GetDigitalLockDetect(MAX2870Driver* driver);
const char* max2870TuneResultToString(MAX2870TuneResult result);

MAX2870TuneRequest* max2870getCurrentTuning(MAX2870Driver* driver);
bool max2870VcoCacheSet(MAX2870Driver* driver, unsigned int vcoIndex, uint8_t value);

/* Register defines */
#define MAX2870_REG0_INT_OFFSET			31
#define MAX2870_REG0_N_OFFSET			15
#define MAX2870_REG0_FRAC_OFFSET		3

#define MAX2870_REG1_CPOC_OFFSET		31
#define MAX2870_REG1_CPL_OFFSET			29
#define MAX2870_REG1_CPT_OFFSET			27
#define MAX2870_REG1_P_OFFSET			15
#define MAX2870_REG1_M_OFFSET			3

#define MAX2870_REG2_LDS_OFFSET			31
#define MAX2870_REG2_SDN_OFFSET			29
#define MAX2870_REG2_MUX_OFFSET			26
#define MAX2870_REG2_DBR_OFFSET			25
#define MAX2870_REG2_RDIV2_OFFSET		24
#define MAX2870_REG2_R_OFFSET			14
#define MAX2870_REG2_REG4DB_OFFSET		13
#define MAX2870_REG2_CP_OFFSET			9
#define MAX2870_REG2_LDF_OFFSET			8
#define MAX2870_REG2_LDP_OFFSET			7
#define MAX2870_REG2_PDP_OFFSET			6
#define MAX2870_REG2_SHDN_OFFSET		5
#define MAX2870_REG2_TRI_OFFSET			4
#define MAX2870_REG2_RST_OFFSET			3

#define MAX2870_REG3_VCO_OFFSET			26
#define MAX2870_REG3_VAS_SHDN_OFFSET	25
#define MAX2870_REG3_RETUNE_OFFSET		24
#define MAX2870_REG3_CDM_OFFSET			15
#define MAX2870_REG3_CDIV_OFFSET		3

#define MAX2870_REG4_BS_MSB_OFFSET		24
#define MAX2870_REG4_FB_OFFSET			23
#define MAX2870_REG4_DIVA_OFFSET		20
#define MAX2870_REG4_BS_OFFSET			12
#define MAX2870_REG4_BDIV_OFFSET		9
#define MAX2870_REG4_RFB_EN_OFFSET		8
#define MAX2870_REG4_BPWR_OFFSET		6
#define MAX2870_REG4_RFA_EN_OFFSET		5
#define MAX2870_REG4_APWR_OFFSET		3

#define MAX2870_REG5_FO1_OFFSET			24
#define MAX2870_REG5_LD_OFFSET			22
#define MAX2870_REG5_MUX_OFFSET			18

/* The database description of register 6 is shifted by two bits
 * (one caused by our way of readout, one unkown) */
#define MAX2870_REG6_POR_OFFSET			21
#define MAX2870_REG6_ADC_OFFSET			18
#define MAX2870_REG6_V_OFFSET			1






#endif /* DRIVERS_MAX2870_MAX2870_H_ */
