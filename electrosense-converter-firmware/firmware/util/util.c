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

#include "util.h"
#include "hal.h"
#include "chprintf.h"
#include <stdarg.h>
#include "../drivers/system.h"

unsigned int gcd(uint32_t u, uint32_t v)
{
    uint32_t shift;
    uint32_t t;

    if (u == 0) return v;
    if (v == 0) return u;

    for (shift = 0; ((u | v) & 1) == 0; ++shift) {
        u >>= 1;
        v >>= 1;
    }

    while ((u & 1) == 0) {
        u >>= 1;
    }

    do {
        while ((v & 1) == 0) {
            v >>= 1;
        }

        if (u > v) {
            t = v;
            v = u;
            u = t;
        }
        v = v - u;
    } while (v != 0);

    return u << shift;
}

int printfFixed(BaseSequentialStream* chn, int minLength, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    int result = chvprintf(chn, format, args);
    for(; result<minLength; result++) {
        chnWrite(chn, (unsigned char*)" ", 1);
    }

    va_end(args);

    return result;
}

typedef void (*func_t)(void);
THD_FUNCTION(runInThreadBody, p)
{
    func_t func = *(func_t*)(&p);
    func();
    vTaskDelete(NULL);
}

char hexNibble(unsigned char a)
{
    a&=0xF;
    if(a<10) {
        return '0'+a;
    } else {
        return 'a'+a-10;
    }
}

bool utilIsPinFloating(uint16_t gpioPin)
{
    gpioSetPinMode(gpioPin, PAL_MODE_INPUT_PULLUP);
    osalThreadSleepMilliseconds(1);
    if(gpioGetPin(gpioPin) == 0) {
        return false;
    }
    gpioSetPinMode(gpioPin, PAL_MODE_INPUT_PULLDOWN);
    osalThreadSleepMilliseconds(1);
    if(gpioGetPin(gpioPin) == 1) {
        return false;
    }

    return true;
}

THD_FUNCTION(mixLedTask, p)
{
    volatile uint32_t* delay = (volatile uint32_t*)p;

    while(1) {
        gpioSetPin(GPIO_LED_MIX, true);
        if(!*delay) {
            vTaskSuspend(NULL);
            continue;
        }
        osalThreadSleepMilliseconds(*delay);
        gpioSetPin(GPIO_LED_MIX, false);
        if(*delay) {
            osalThreadSleepMilliseconds(*delay);
        }
    }
}

int8_t charToInt(char input, uint8_t base)
{
    int8_t result = -1;

    /* Digits */
    if(input >= '0' && input <= '9') {
        result = input - '0';
    }

    /* Make uppercase */
    input &=~ 0x20;

    /* Letters */
    if(input >= 'A' && input <= 'Z') {
        result = input - 'A' + 10;
    }

    if(result >= base) {
        return -1;
    }

    return (result>=base)? -1 : result;
}

int32_t strToInt(char* string, uint8_t base)
{
    uint32_t value = 0;
    bool invert = false;
    unsigned int i = 0;

    if(string[0] == '\0') {
        return -1;
    }

    if(string[0] == '-') {
        invert = true;
        i++;
    }

    for(; string[i]; i++) {
        value *= base;

        int8_t ci = charToInt(string[i], base);
        if(ci < 0) {
            return -1;
        }

        value += ci;
    }

    return (invert)? -value : value;
}


