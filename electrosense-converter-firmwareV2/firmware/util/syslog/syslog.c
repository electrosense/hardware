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
#include "syslog.h"
#include "chprintf.h"
#include "shell.h"
#include "memstreams.h"


#include <stdarg.h>

static PacketQueue_t syslogQueue;
static SemaphoreHandle_t syslogSemaphore;

void syslogInit(uint32_t bufferSize)
{
    void* syslogBuffer = pvPortMalloc(bufferSize);
    packetQueueInit(&syslogQueue, syslogBuffer, bufferSize);

    syslogSemaphore = xSemaphoreCreateMutex();

    syslog("Syslog started.");
}

void syslogClear(void)
{
    xSemaphoreTake(syslogSemaphore, portMAX_DELAY);
    packetQueueReset(&syslogQueue);
    xSemaphoreGive(syslogSemaphore);
}

static size_t syslogWrite(void *instance, const uint8_t *bp, size_t n)
{
    (void)instance;

    packetQueuePutBytes(&syslogQueue, bp, n);

    return n;
}

static msg_t syslogPut(void *instance, uint8_t b)
{
    (void)instance;

    packetQueuePutBytes(&syslogQueue, &b, 1);

    return MSG_OK;
}

static const struct BaseSequentialStreamVMT syslogVMT = {
		.write = syslogWrite,
		.put = syslogPut
};
static const BaseSequentialStream syslogStream = {.vmt = &syslogVMT};


void syslog(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    xSemaphoreTake(syslogSemaphore, portMAX_DELAY);
    packetQueueStartWritePacket(&syslogQueue);
    chprintf((BaseSequentialStream*)&syslogStream, "[%-11u] ", osalOsGetSystemTimeX());
    chvprintf((BaseSequentialStream*)&syslogStream, format, args);
    packetQueueEndWritePacket(&syslogQueue);
    xSemaphoreGive(syslogSemaphore);

    va_end(args);
}

void syslogDump(BaseSequentialStream* chp)
{

    xSemaphoreTake(syslogSemaphore, portMAX_DELAY);
    if(syslogQueue.numDropped) {
        chprintf(chp, "... %u dropped ..."SHELL_NEWLINE_STR, syslogQueue.numDropped);
    }
    packetQueueStartReadTransaction(&syslogQueue);
    uint16_t len;
    while((len = packetQueueReadPacket(&syslogQueue))) {
        uint16_t i = 0;
        while(i < len) {
            uint8_t tmpBuf[12];

            uint16_t readBytes = packetQueueReadBytes(&syslogQueue, tmpBuf, sizeof(tmpBuf));
            chnWrite(chp, tmpBuf, readBytes);
            i += readBytes;
        }
        chnWrite(chp, (uint8_t*)SHELL_NEWLINE_STR, strlen(SHELL_NEWLINE_STR));
    }
    packetQueueEndReadTransaction(&syslogQueue, true);
    xSemaphoreGive(syslogSemaphore);
}

static bool syslogISRHasSomething;
static char syslogISRBuffer[64];

bool syslogISRUnknownPriority(const char *format, ...)
{
    if(__atomic_test_and_set(&syslogISRHasSomething, __ATOMIC_ACQUIRE)) {
        return false;
    }

    va_list args;
    va_start(args, format);

    MemoryStream ms;
    msObjectInit(&ms, (uint8_t *)syslogISRBuffer, sizeof(syslogISRBuffer)-1, 0);
    BaseSequentialStream *chp = (BaseSequentialStream *)(void *)&ms;

    chvprintf(chp, format, args);
    syslogISRBuffer[ms.eos] = 0;

    va_end(args);

    return true;
}

void syslogIdleHook(void)
{
    /*
     * Some events may happen from IRQs with unknown priority (possibly above kernel),
     * so they are written to a buffer (one entry only) and added to the syslog in
     * the IDLE thread.
     */
    if(__atomic_load_n(&syslogISRHasSomething, __ATOMIC_ACQUIRE)) {
        if(xSemaphoreTake(syslogSemaphore, 0)) {
            packetQueueStartWritePacket(&syslogQueue);
            chprintf((BaseSequentialStream*)&syslogStream, "[%-11u] {ISR} ", osalOsGetSystemTimeX());
            syslogISRBuffer[sizeof(syslogISRBuffer)-1] = 0;
            packetQueuePutBytes(&syslogQueue, (uint8_t*)syslogISRBuffer, strlen(syslogISRBuffer));
            packetQueueEndWritePacket(&syslogQueue);
            xSemaphoreGive(syslogSemaphore);

            __atomic_clear(&syslogISRHasSomething, __ATOMIC_RELEASE);
        }
    }
}
