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

#include "converter.h"

static bool converterTuneSHF(const ConverterManager* converter, const ConverterBand* band, ConverterTuneRequest* tuneRequest)
{
    ConverterSHFConfig* config = (ConverterSHFConfig*)band->userData;
    uint32_t ioOut = _BV(CONVERTER_IO_PIN_MIX_EN);
    uint32_t loFreq = 0;

    if(tuneRequest->antennaInput) {
        ioOut |= _BV(CONVERTER_IO_PIN_ANT_MID);
    } else {
        ioOut |= _BV(CONVERTER_IO_PIN_ANT_HIGH);
    }

    if(config->useHighSideMixing) {
        loFreq = tuneRequest->inputFrequencyKHz + config->ifFrequencyKHz;
    } else {
        if(tuneRequest->inputFrequencyKHz < config->ifFrequencyKHz) {
            return false;
        }
        loFreq = tuneRequest->inputFrequencyKHz - config->ifFrequencyKHz;
    }

    /* If tuning failed, try to use LO/2 and enable doubler */
    if(!config->loTune(loFreq, 2)) {
        loFreq /= 2;
        ioOut |= _BV(CONVERTER_IO_PIN_MIX_X2);

        /* Try again, if it fails now it is fatal */
        if(!config->loTune(loFreq, 2)) {
            return false;
        }
    }

    converter->setConverterGpio(converter, ioOut);

    tuneRequest->outputFrequencyKHz = config->ifFrequencyKHz;
    tuneRequest->spectrumInversion = config->useHighSideMixing;

    return true;
}

static bool converterLeaveSHF(const ConverterManager* converter, const ConverterBand* band)
{
    ConverterSHFConfig* config = (ConverterSHFConfig*)band->userData;

    /* Turn everything off */
    config->loTune(0, -127);
    converter->setConverterGpio(converter, 0);

    return true;
}

ConverterFunctions converterSHFFunctions = {converterTuneSHF, converterLeaveSHF};




