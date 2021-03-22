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

#ifndef PACKETQUEUE_H_
#define PACKETQUEUE_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    uint8_t* buffer;
    uint32_t bufferSize;

    uint32_t readPtr;
    uint32_t readPtrStored;
    uint32_t writePtr;
    uint16_t pktCount;
    uint16_t pktCountStored;

    uint32_t pktLen;
    uint32_t lenHeaderStart;

    uint32_t numDropped;


} PacketQueue_t;

void packetQueueInit(PacketQueue_t* queue, void* buffer, uint32_t bufferSize);

void packetQueueStartWritePacket(PacketQueue_t* queue);
void packetQueueEndWritePacket(PacketQueue_t* queue);
void packetQueuePutBytes(PacketQueue_t* queue, const uint8_t* payload, uint16_t size);

uint16_t packetQueueReadPacket(PacketQueue_t* queue);
uint16_t packetQueueReadBytes(PacketQueue_t* queue, uint8_t* buffer, uint16_t bufLen);
void packetQueueStopReadPacket(PacketQueue_t* queue, bool dropIt);

void packetQueueStartReadTransaction(PacketQueue_t* queue);
void packetQueueEndReadTransaction(PacketQueue_t* queue, bool restoreIt);

void packetQueueReset(PacketQueue_t* queue);

#endif /* PACKETQUEUE_H_ */
