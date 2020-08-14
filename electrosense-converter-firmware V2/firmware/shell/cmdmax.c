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

#include <string.h>
#include <stdlib.h>


static void cmdMaxUsage(BaseSequentialStream* chp)
{
    chprintf(chp, "Usage:"SHELL_NEWLINE_STR);
    chprintf(chp, "\tmax tune [freqkHz] [power] [forceVAS]"SHELL_NEWLINE_STR);
    chprintf(chp, "\tmax status"SHELL_NEWLINE_STR);
    chprintf(chp, "\tmax vcocache"SHELL_NEWLINE_STR);
    chprintf(chp, "\tmax vcocache calibrate"SHELL_NEWLINE_STR);
    chprintf(chp, "\tmax vcocache set index value"SHELL_NEWLINE_STR);
    chprintf(chp, "\tmax dld"SHELL_NEWLINE_STR);
}

static void cmdMaxTune(MAX2870Driver* pll, BaseSequentialStream *chp, int argc, char *argv[])
{
    MAX2870TuneRequest tune = *max2870getCurrentTuning(pll);
    tune.fastLockDurationMicroseconds = 0;
    tune.useVCOAutotune = false;

    if(argc >= 1) {
        tune.frequency = (uint64_t)strToInt(argv[0], 10) * 1000;
    }
    if(argc >= 2) {
        tune.powerA = strToInt(argv[1], 10);
    }
    if(argc >= 3) {
        tune.useVCOAutotune = (strToInt(argv[2], 10) > 0);
    }

    MAX2870TuneResult result = max2870Tune(pll, &tune);

    /* Send status */
    if(result == TUNE_OK) {
        max2870StatusPrint(pll, chp);
        chprintf(chp, SHELL_NEWLINE_STR);
    }

    chprintf(chp, "Tuning result: %s"SHELL_NEWLINE_STR, max2870TuneResultToString(result));
}

static void cmdMaxVCOCache(MAX2870Driver* pll, BaseSequentialStream *chp, int argc, char *argv[])
{
    if(argc == 0) {
        max2870VcoPrint(pll, chp);
    } else if(!strcmp(argv[0], "calibrate")) {
        max2870VcoPrecal(pll);
        chprintf(chp, "Done"SHELL_NEWLINE_STR);
    } else if(argc >= 2 && !strcmp(argv[0], "set")) {
        unsigned int a = strToInt(argv[1], 10);
        unsigned int b = 0;
        if(argc == 3) {
            b = strToInt(argv[2], 10) | 0x80;
        }
        if(max2870VcoCacheSet(pll, a , b)) {
            chprintf(chp, "%u=0x%02x"SHELL_NEWLINE_STR,a,b);
        } else {
            chprintf(chp, "The cow says moo!"SHELL_NEWLINE_STR);
        }
    } else {
        cmdMaxUsage(chp);
        return;
    }
}


void cmdMax(void* user, BaseSequentialStream *chp, int argc, char *argv[])
{
    MAX2870Driver* pll = (MAX2870Driver*)user;

    if(argc == 0) {
        cmdMaxUsage(chp);
        return;
    }
    if(!strcmp(argv[0], "tune")) {
        cmdMaxTune(pll, chp, argc-1, argv+1);
    } else if(!strcmp(argv[0], "status")) {
        max2870StatusPrint(pll, chp);
    } else if(!strcmp(argv[0], "vcocache")) {
        cmdMaxVCOCache(pll, chp, argc-1, argv+1);
    } else if(!strcmp(argv[0], "dld")) {
        chprintf(chp, "Digital Lock Detect: %u"SHELL_NEWLINE_STR, max2870GetDigitalLockDetect(pll));
    } else {
        cmdMaxUsage(chp);
        return;
    }
}
