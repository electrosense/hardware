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


#include "packetqueue.h"

static uint8_t packetQueueReadByte(PacketQueue_t* queue)
{
    uint8_t retVal = queue->buffer[queue->readPtr];

    queue->readPtr++;
    if(queue->readPtr >= queue->bufferSize) {
        queue->readPtr = 0;
    }

    return retVal;
}

static void packetQueueWriteByte(PacketQueue_t* queue, uint8_t byte)
{
    if(queue->pktCount && queue->writePtr == queue->readPtr) {
        /* We are about to overwrite a packet, so discard it */
        uint16_t dropPktLen = (packetQueueReadByte(queue) << 8) |
                              (packetQueueReadByte(queue));
        queue->readPtr += dropPktLen;
        if(queue->readPtr >= queue->bufferSize) {
            queue->readPtr -= queue->bufferSize;
        }
        queue->pktCount--;
        queue->numDropped++;
    }

    queue->buffer[queue->writePtr] = byte;

    queue->writePtr++;
    if(queue->writePtr >= queue->bufferSize) {
        queue->writePtr = 0;
    }
}

void packetQueueStartWritePacket(PacketQueue_t* queue)
{
    /* Put a dummy length header for now */
    queue->lenHeaderStart = queue->writePtr;
    packetQueueWriteByte(queue, 0x00);
    packetQueueWriteByte(queue, 0x00);
    queue->pktLen = 0;
}

void packetQueuePutBytes(PacketQueue_t* queue, const uint8_t* payload, uint16_t size)
{
    for(uint32_t i=0; i<size; i++) {
        packetQueueWriteByte(queue, payload[i]);
    }

    queue->pktLen += size;
}

void packetQueueEndWritePacket(PacketQueue_t* queue)
{
    /* Did we write more bytes than the ENTIRE queue? */
    if(queue->pktLen >= queue->bufferSize-2) {
        queue->numDropped+=queue->pktCount+1;
        queue->pktCount = 0;
        queue->readPtr = 0;
        queue->writePtr = 0;
        return;
    }

    /* Packet too long? */
    if(queue->pktLen > 0xffff) {
        queue->writePtr = queue->lenHeaderStart;
        queue->numDropped++;
        return;
    }

    uint32_t hdrIndex = queue->lenHeaderStart;
    queue->buffer[hdrIndex] = queue->pktLen >> 8;
    hdrIndex++;
    if(hdrIndex >= queue->bufferSize) {
        hdrIndex = 0;
    }
    queue->buffer[hdrIndex] = queue->pktLen & 0xff;
    queue->pktCount++;
}

uint16_t packetQueueReadPacket(PacketQueue_t* queue)
{
    if(!queue->pktCount) {
        return 0;
    }

    /* Read the length */
    queue->pktLen = (packetQueueReadByte(queue) << 8) |
                    (packetQueueReadByte(queue));

    return queue->pktLen;
}

uint16_t packetQueueReadBytes(PacketQueue_t* queue, uint8_t* buffer, uint16_t bufLen)
{
    if(!queue->pktLen) {
        return 0;
    }

    if(bufLen > queue->pktLen) {
        bufLen = queue->pktLen;
    }

    if(buffer) {
        for(uint16_t i=0; i<bufLen; i++) {
            buffer[i] = packetQueueReadByte(queue);
        }
    } else {
        queue->readPtr += bufLen;
        if(queue->readPtr >= queue->bufferSize) {
            queue->readPtr -= queue->bufferSize;
        }
    }

    queue->pktLen -= bufLen;

    if(!queue->pktLen) {
        queue->pktCount--;
    }

    return bufLen;
}

void packetQueueStartReadTransaction(PacketQueue_t* queue)
{
    queue->readPtrStored = queue->readPtr;
    queue->pktCountStored = queue->pktCount;
}

void packetQueueEndReadTransaction(PacketQueue_t* queue, bool restoreIt)
{
    if(restoreIt) {
        queue->readPtr = queue->readPtrStored;
        queue->pktCount = queue->pktCountStored;
    }
}

void packetQueueInit(PacketQueue_t* queue, void* buffer, uint32_t bufferSize)
{
    memset(queue, 0, sizeof(*queue));

    queue->buffer = buffer;
    queue->bufferSize = bufferSize;
}

void packetQueueReset(PacketQueue_t* queue)
{
    void* buffer = queue->buffer;
    uint32_t bufferSize = queue->bufferSize;

    packetQueueInit(queue, buffer, bufferSize);

}
