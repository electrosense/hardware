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

#include "hal.h"
#include "util.h"
#include "syslog/syslog.h"

#define NVIC_ICSR (*((volatile uint32_t*)0xE000ED04))
#define NVIC_CPR  (*((volatile uint32_t*)0xE000E280))
#define NVIC_CER  (*((volatile uint32_t*)0xE000E180))

static void clearInterrupt(unsigned int vectorId, bool disable)
{
    volatile uint32_t* pendReg = (&NVIC_CPR) + (vectorId / 32);
    *pendReg = _BV(vectorId%32);
    if(disable) {
        volatile uint32_t* ceReg = (&NVIC_CER) + (vectorId / 32);
        *ceReg = _BV(vectorId%32);
    }
}

void unhandledException(void)
{
    /* Check which vector is active */
    unsigned int vectorId = NVIC_ICSR & 0x1ff;

    if(vectorId >= 16) {
        /* This can be masked */
        vectorId -= 16;
        clearInterrupt(vectorId, true);

        /* Report it */
        syslogISRUnknownPriority("Unhandled vector %u.", vectorId);
    } else {
        osalSysHalt("Fault");
    }
}

