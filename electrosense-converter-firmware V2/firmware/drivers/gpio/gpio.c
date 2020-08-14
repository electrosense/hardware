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

#include "../system.h"
#include "../../util/util.h"
#include "shell.h"
#include "chprintf.h"


static struct {
    GPIOPort* ports;
    uint8_t numPorts;
    uint32_t registeredPorts;
} GPIODriverData;

static const GPIOPort gpioDummyPort = {
    .driver = NULL,
    .functions = &gpioDummyFunctions,
    .numPins = 0
};

bool gpioInit(uint8_t numPorts)
{
    if(numPorts >= 32) {
        numPorts = 32;
    }

    GPIODriverData.numPorts = numPorts;
    GPIODriverData.ports = pvPortMalloc(sizeof(GPIOPort) * numPorts);
    if(!GPIODriverData.ports) {
        /* This is likely fatal */
        return false;
    }

    /* Make all ports dummies */
    for(uint8_t i=0; i<numPorts; i++) {
        GPIODriverData.ports[i].driver = (void*)(unsigned int)i;
        GPIODriverData.ports[i].functions = &gpioDummyFunctions;
        GPIODriverData.ports[i].numPins = 0;
    }

    return true;
}

GPIOPort* gpioRegisterPortDriver(uint8_t index)
{
    if(index >= GPIODriverData.numPorts) {
        return NULL;
    }
    if(GPIODriverData.registeredPorts & _BV(index)) {
        return NULL;
    }

    GPIODriverData.registeredPorts |= _BV(index);

    return &GPIODriverData.ports[index];
}

static const GPIOPort* gpioGetPortDriver(uint8_t index)
{
    if(index >= GPIODriverData.numPorts) {
        return &gpioDummyPort;
    }

    return &GPIODriverData.ports[index];
}

bool gpioSetPin(uint16_t pin, bool on)
{
    uint8_t gpioPort = pin >> 8;
    uint8_t gpioPin = pin & 0xff;

    const GPIOPort* port = gpioGetPortDriver(gpioPort);

    return port->functions->setValue(port, 1<<gpioPin, on<<gpioPin);
}

bool gpioGetPin(uint16_t pin)
{
    uint8_t gpioPort = pin >> 8;
    uint8_t gpioPin = pin & 0xff;

    const GPIOPort* port = gpioGetPortDriver(gpioPort);

    uint32_t value = 0;
    port->functions->getValue(port, &value);

    return (value >> gpioPin) & 1;
}

bool gpioSetPinMode(uint16_t pin, uint8_t mode)
{
    uint8_t gpioPort = pin >> 8;
    uint8_t gpioPin = pin & 0xff;

    const GPIOPort* port = gpioGetPortDriver(gpioPort);

    return port->functions->setMode(port, gpioPin, mode);
}

uint8_t gpioGetPinMode(uint16_t pin)
{
    uint8_t gpioPort = pin >> 8;
    uint8_t gpioPin = pin & 0xff;

    const GPIOPort* port = gpioGetPortDriver(gpioPort);

    return port->functions->getMode(port, gpioPin);
}

bool gpioSetPort(uint8_t gpioPort, uint32_t value)
{
    const GPIOPort* port = gpioGetPortDriver(gpioPort);

    return port->functions->setValue(port, ~0, value);
}

uint32_t gpioGetPort(uint8_t gpioPort)
{
    const GPIOPort* port = gpioGetPortDriver(gpioPort);

    uint32_t value = 0;
    port->functions->getValue(port, &value);

    return value;
}

void gpioPortIoDisable(uint8_t gpioPort, bool disable)
{
    const GPIOPort* port = gpioGetPortDriver(gpioPort);

    if(port->functions->busIoDisable) {
        port->functions->busIoDisable(port, disable);
    }
}

void gpioPrintStatus(BaseSequentialStream* chp)
{
    for(uint8_t i=0; i<GPIODriverData.numPorts; i++) {
        const GPIOPort* port = gpioGetPortDriver(i);

        chprintf(chp, "Port %u: ", i);
        if(GPIODriverData.registeredPorts & _BV(i)) {
            chprintf(chp, "(Active, %u pins)", port->numPins);
        }
        chprintf(chp, SHELL_NEWLINE_STR);

        if(port->functions->status) {
            port->functions->status(port, chp);
        }
        chprintf(chp, SHELL_NEWLINE_STR);

    }
}

void gpioInitPins(const GPIOPinInit* pins)
{
    uint32_t index = 0;

    for(;;) {
        const GPIOPinInit* pin = &pins[index];

        if(pin->gpioPin == 0xffff) {
            return;
        }

        gpioSetPinMode(pin->gpioPin, pin->gpioMode);
        gpioSetPin(pin->gpioPin, pin->defaultValue);

        index++;
    }
}


