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
#include <stdbool.h>
#include "converter.h"
#include "shell.h"
#include "chprintf.h"

bool converterTune(ConverterManager* converter, ConverterTuneRequest* tuneRequest)
{
    /* Iterate over all bands */
    for(uint8_t i=0; converter->bands[i].functions; i++) {
        if(_BV(i) & converter-> disabledBands) {
            continue;
        }

        uint32_t tmpFreq = tuneRequest->inputFrequencyKHz;

        if((tuneRequest->forceBand && tuneRequest->bandId == i) ||
                (!tuneRequest->forceBand &&
                 tmpFreq >= converter->bands[i].minFrequencyKHz &&
                 tmpFreq <= converter->bands[i].maxFrequencyKHz)) {


            if(converter->activeBand != 0xff && converter->activeBand != i) {
                if(converter->bands[converter->activeBand].functions->converterLeaveBand) {
                    converter->bands[converter->activeBand].functions->converterLeaveBand(converter,
                            &converter->bands[converter->activeBand]);
                }
            }

            converter->activeBand = i;
            tuneRequest->bandId = i;

            bool retVal = converter->bands[i].functions->converterTuneBand(converter, &converter->bands[i], tuneRequest);
            if(retVal) {
                converter->currentTune = *tuneRequest;
            }

            return retVal;
        }
    }


    return false;
}

bool converterInit(ConverterManager* converter, const ConverterBand* bands,
                   void(*setGpio)(const ConverterManager* converter,uint32_t gpioValues))
{
    converter->activeBand = 0xff;
    converter->bands = bands;
    converter->setConverterGpio = setGpio;

    return true;
}

void converterStatus(BaseSequentialStream* chp, ConverterManager* converter)
{
    chprintf(chp, "Bands: [Disable mask: 0x%08x]"SHELL_NEWLINE_STR, converter->disabledBands);
    printfFixed(chp, 9, "\tBand ID");
    printfFixed(chp, 7, "Name");
    printfFixed(chp, 14, "FreqMin [kHz]");
    chprintf(chp,	     "FreqMax [kHz]"SHELL_NEWLINE_STR);
    for(uint8_t i=0; converter->bands[i].functions; i++) {
        if(_BV(i) & converter-> disabledBands) {
            continue;
        }
        if(i == converter->activeBand) {
            printfFixed(chp, 9, "\t%u [*]", i);
        } else {
            printfFixed(chp, 9, "\t%u", i);
        }
        printfFixed(chp, 7, "%s", converter->bands[i].bandName);
        printfFixed(chp, 14, "%u", converter->bands[i].minFrequencyKHz);
        chprintf(chp,	     "%u"SHELL_NEWLINE_STR, converter->bands[i].maxFrequencyKHz);
    }

    chprintf(chp, SHELL_NEWLINE_STR);

    if(converter->activeBand == 0xff) {
        chprintf(chp, "Converter disabled." SHELL_NEWLINE_STR);
    } else {
        chprintf(chp, "Input Frequency:    %u kHz" SHELL_NEWLINE_STR, converter->currentTune.inputFrequencyKHz);
        chprintf(chp, "Output Frequency:   %u kHz" SHELL_NEWLINE_STR, converter->currentTune.outputFrequencyKHz);
        chprintf(chp, "Spectral inversion: %u" SHELL_NEWLINE_STR, converter->currentTune.spectrumInversion);
        chprintf(chp, "Selected antenna:   %u" SHELL_NEWLINE_STR , converter->currentTune.antennaInput);
    }
}
