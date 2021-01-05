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

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "gpio/gpio.h"
#include "i2csafe/i2csafe.h"
#include "max2870/max2870.h"
#include "tca6408a/tca6408a.h"
#include "mcp9804/mcp9804.h"
#include "usb/usb.h"
#include "../logic/converter/converter.h"
#include "../util/monitor.h"
#include "../util/syslog/syslog.h"

/* Define all GPIOs */
#define GPIO_PORT_GPIOA 0
#define GPIO_PORT_GPIOB 1
#define GPIO_PORT_GPIOC 2
#define GPIO_PORT_I2C0  3

#define GPIO_PLL_SCK 	MAKE_GPIO(GPIO_PORT_GPIOA, GPIOA_PLL_SCK)
#define GPIO_PLL_MISO 	MAKE_GPIO(GPIO_PORT_GPIOA, GPIOA_PLL_MISO)
#define GPIO_PLL_MOSI 	MAKE_GPIO(GPIO_PORT_GPIOA, GPIOA_PLL_MOSI)
#define GPIO_PLL_SS 	MAKE_GPIO(GPIO_PORT_GPIOA, GPIOA_PLL_SS)
#define GPIO_PLL_CE 	MAKE_GPIO(GPIO_PORT_GPIOA, GPIOA_PLL_CE)
#define GPIO_I2C_SCL 	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_I2C_SCL)
#define GPIO_I2C_SDA 	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_I2C_SDA)
#define GPIO_LED_USB 	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_LED_USB)
#define GPIO_LED_LOCK 	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_LED_LOCK)
#define GPIO_LED_MIX 	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_LED_MIX)
#define GPIO_LED_USB 	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_LED_USB)
#define GPIO_LED_LOCK 	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_LED_LOCK)
#define GPIO_LED_MIX 	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_LED_MIX)
#define GPIO_MIX_SW_EN 	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_MIX_SW_EN)
#define GPIO_MIX_SW_LO	MAKE_GPIO(GPIO_PORT_GPIOA, GPIOA_MIX_SW_LO)
#define GPIO_UART_TX	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_UART_TX)
#define GPIO_UART_RX	MAKE_GPIO(GPIO_PORT_GPIOB, GPIOB_UART_RX)

#define CONVERTER_IF_FREQ 1576000

#define GPIO_ANT_HIGH 	MAKE_GPIO(GPIO_PORT_I2C0, CONVERTER_IO_PIN_ANT_HIGH)
#define GPIO_ANT_MID 	MAKE_GPIO(GPIO_PORT_I2C0, CONVERTER_IO_PIN_ANT_MID)
#define GPIO_SW_BYPASS	MAKE_GPIO(GPIO_PORT_I2C0, CONVERTER_IO_PIN_SW_BYPASS)
#define GPIO_SW_MIX 	MAKE_GPIO(GPIO_PORT_I2C0, CONVERTER_IO_PIN_SW_MIX)
#define GPIO_SW_SW 		MAKE_GPIO(GPIO_PORT_I2C0, CONVERTER_IO_PIN_SW_SW)
#define GPIO_MIX_X2 	MAKE_GPIO(GPIO_PORT_I2C0, CONVERTER_IO_PIN_MIX_X2)
#define GPIO_MIX_EN 	MAKE_GPIO(GPIO_PORT_I2C0, CONVERTER_IO_PIN_MIX_EN)
#define GPIO_LOWBAND 	MAKE_GPIO(GPIO_PORT_I2C0, CONVERTER_IO_PIN_LOWBAND)

extern volatile uint32_t sanityRebootSeconds;

extern MAX2870Driver loPLL;
extern TCA6408Driver mixerControllerIO;
extern MCP9804Driver tcxoTempSensor;
extern ConverterSHFConfig converterSHFBandConfig[4];

void startSystemComponents(void);
void systemEnableMCO(bool enable);
extern void main_(void);

#endif /* SYSTEM_H_ */
