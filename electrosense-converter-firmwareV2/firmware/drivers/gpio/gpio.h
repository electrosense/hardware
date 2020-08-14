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

#ifndef DRIVERS_GPIO_GPIO_H_
#define DRIVERS_GPIO_GPIO_H_

#include "hal.h"
#include "../system.h"
#include <stdbool.h>
#include <stdint.h>

struct GPIOPortStruct;
typedef struct GPIOPortStruct GPIOPort;

typedef struct {
    bool (*setMode)(const GPIOPort* driver, uint8_t pin, uint8_t pinMode);
    uint8_t (*getMode)(const GPIOPort* driver, uint32_t pin);
    bool (*setValue)(const GPIOPort* driver, uint32_t pinsToChange, uint32_t newValue);
    bool (*getValue)(const GPIOPort* driver, uint32_t* value);
    void (*busIoDisable)(const GPIOPort* driver, bool disable);
    void (*status)(const GPIOPort* context, BaseSequentialStream* chp);
} GPIOPortFunctions;

struct GPIOPortStruct {
    void* driver;
    uint32_t numPins;
    const GPIOPortFunctions* functions;
};

typedef struct {
    uint16_t gpioPin;
    uint8_t gpioMode;
    bool defaultValue;
} GPIOPinInit;

#define MAKE_GPIO(port, pin) ((port<<8)|(pin & 0xFF))

extern const GPIOPortFunctions gpioDummyFunctions;

bool gpioInit(uint8_t numPorts);
GPIOPort* gpioRegisterPortDriver(uint8_t index);
void gpioInitPins(const GPIOPinInit* pins);

bool gpioSetPin(uint16_t pin, bool on);
bool gpioGetPin(uint16_t pin);
bool gpioSetPinMode(uint16_t pin, uint8_t mode);
uint8_t gpioGetPinMode(uint16_t pin);
bool gpioSetPort(uint8_t gpioPort, uint32_t value);
void gpioPortIoDisable(uint8_t gpioPort, bool disable);
uint32_t gpioGetPort(uint8_t gpioPort);

void gpioCPUInit(uint8_t gpioPortId, ioportid_t port);

void gpioPrintStatus(BaseSequentialStream* chp);

#endif /* DRIVERS_GPIO_GPIO_H_ */
