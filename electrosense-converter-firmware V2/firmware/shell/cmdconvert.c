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


#include "appshell.h"
#include "chprintf.h"
#include <stdlib.h>
#include <string.h>

#include "../logic/converter/converter.h"


void cmdConvertUsage(BaseSequentialStream *chp)
{
    chprintf(chp, "Usage:"SHELL_NEWLINE_STR);
    chprintf(chp, "\tconvert setup [freqkHz] [forceBand]"SHELL_NEWLINE_STR);
    chprintf(chp, "\tconvert hs [shfband] [1/0]"SHELL_NEWLINE_STR);
    chprintf(chp, "\tconvert status"SHELL_NEWLINE_STR);
}

void cmdConvert(void* user, BaseSequentialStream *chp, int argc, char *argv[])
{
    ConverterManager* converter = (ConverterManager*)user;

    if(argc == 1) {
        if(!strcmp(argv[0], "status")) {
            converterStatus(chp, converter);
            return;
        }
    } else if (argc >= 2) {
        if(argc == 3 && !strcmp(argv[0], "hs")) {
            uint32_t band = strToInt(argv[1], 10);
            if(band < sizeof(converterSHFBandConfig)/sizeof(ConverterSHFConfig)) {
                converterSHFBandConfig[band].useHighSideMixing = (argv[2][0] == '1');
                return;
            }
        }
        if(!strcmp(argv[0], "setup")) {
            uint32_t frequency = strToInt(argv[1], 10);

            ConverterTuneRequest tuneRequest;
            tuneRequest.inputFrequencyKHz = frequency;
            tuneRequest.antennaInput = 0;
            tuneRequest.forceBand = false;

            if(argc >= 3) {
                tuneRequest.forceBand = true;
                tuneRequest.bandId = strToInt(argv[2], 10);
            }

            bool result = converterTune(converter, &tuneRequest);
            if(result) {
                chprintf(chp, "IF Frequency: %u kHz, inversion: %u (band %u)"SHELL_NEWLINE_STR, tuneRequest.outputFrequencyKHz, tuneRequest.spectrumInversion, tuneRequest.bandId);
            } else {
                chprintf(chp, "Error!"SHELL_NEWLINE_STR);
            }
            return;
        }
    }

    cmdConvertUsage(chp);
}
