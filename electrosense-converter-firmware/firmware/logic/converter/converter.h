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

#include "../../drivers/system.h"
#include "../../util/util.h"
#include <stdint.h>
#include <stdbool.h>

#include "converterSHF.h"

#ifndef LOGIC_CONVERTER_CONVERTER_H_
#define LOGIC_CONVERTER_CONVERTER_H_

#define CONVERTER_IO_PIN_ANT_HIGH 0
#define CONVERTER_IO_PIN_ANT_MID 1
#define CONVERTER_IO_PIN_SW_BYPASS 2
#define CONVERTER_IO_PIN_SW_MIX 3
#define CONVERTER_IO_PIN_SW_SW 4
#define CONVERTER_IO_PIN_MIX_X2 5
#define CONVERTER_IO_PIN_MIX_EN 6
#define CONVERTER_IO_PIN_LOWBAND 7
#define CONVERTER_IO_PIN_MIX_SW_EN 8
#define CONVERTER_IO_PIN_MIX_SW_LO 9
#define CONVERTER_IO_PIN_LED1 10
#define CONVERTER_IO_PIN_LED2 11

typedef struct {
    /* These are inputs */
    uint32_t inputFrequencyKHz;
    uint32_t antennaInput;

    /* This is read write */
    bool forceBand;
    uint8_t bandId;

    /* This is set after tuning */
    uint32_t outputFrequencyKHz;
    bool spectrumInversion;
} ConverterTuneRequest;

typedef struct ConverterManager_s ConverterManager;
typedef struct ConverterBand_s ConverterBand;

struct ConverterManager_s {
    ConverterTuneRequest currentTune;

    const ConverterBand* bands;
    uint8_t activeBand;
    uint32_t disabledBands;

    void(*setConverterGpio)(const ConverterManager* converter, uint32_t gpioValues);
};

typedef struct {
    bool (*converterTuneBand)(const ConverterManager* converter, const ConverterBand* band, ConverterTuneRequest* tuneRequest);
    bool (*converterLeaveBand)(const ConverterManager* converter, const ConverterBand* band);
} ConverterFunctions;

struct ConverterBand_s {
    const char* bandName;
    uint32_t minFrequencyKHz;
    uint32_t maxFrequencyKHz;
    void* userData;

    const ConverterFunctions* functions;
};

bool converterTune(ConverterManager* converter, ConverterTuneRequest* tuneRequest);

extern ConverterFunctions converterBypassFunctions;
extern ConverterFunctions converterSWFunctions;
extern ConverterFunctions converterSHFFunctions;

bool converterInit(ConverterManager* converter, const ConverterBand* bands,
                   void(*setGpio)(const ConverterManager* converter,uint32_t gpioValues));
void converterStatus(BaseSequentialStream* chp, ConverterManager* converter);

#endif /* LOGIC_CONVERTER_CONVERTER_H_ */
