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

#include "system.h"
#include "../util/util.h"
#include "../shell/appshell.h"
#include <string.h>

volatile uint32_t sanityRebootSeconds = 7*24*3600;

/* ~~~~~~~~~~~~~~~~~~~~~ Watchdog timer ~~~~~~~~~~~~~~~~~~~~~ */
static const WDGConfig watchdogConfig = {
    .pr = 6,
    .rlr = 0xFFF
};

/* ~~~~~~~~~~~~~~~~~~~~~ Debug serial port ~~~~~~~~~~~~~~~~~~~~~ */
static const SerialConfig serial3Config = {
    115200, 0, USART_CR2_STOP1_BITS, 0
};
/* ~~~~~~~~~~~~~~~~~~~~~ SHF LO PLL ~~~~~~~~~~~~~~~~~~~~~ */
static const SPIConfig pllSPIConfig = {
    NULL, GPIOA, 4, SPI_CR1_BR_0, 0
};

static void mac2870LockCallback(bool locked)
{
    if(locked) {
        gpioSetPin(GPIO_LED_LOCK, false);
    } else {
        gpioSetPin(GPIO_LED_LOCK, true);
    }
}

static const MAX2870Driver_config max2870Config = {
    .spiPort = &SPID1,
    .spiConfig = &pllSPIConfig,
    .gpioChipEnable = GPIO_PLL_CE,
    .spurMode = 2,
    /* This value is best when "sof 1" has been executed */
    .chargePumpCurrent = 6,
    .inputFrequency = 38400000,
    .referenceToPfdDivider = 2,
    .stepFrequency = 5000,
    .lockStatus = mac2870LockCallback
};

MAX2870Driver loPLL;

/* ~~~~~~~~~~~~~~~~~~~~~ I2C Bus ~~~~~~~~~~~~~~~~~~~~~ */

static const i2cSafeConfig I2C1SafeConfig = {
    .i2cfg = {
        OPMODE_I2C,
        400000,
        FAST_DUTY_CYCLE_2
    },
    .sclPin = GPIO_I2C_SCL,
    .sdaPin = GPIO_I2C_SDA,
    .peripheralMode = PAL_MODE_STM32_ALTERNATE_OPENDRAIN
};

/* ~~~~~~~~~~~~~~~~~~~~~ I2C IO Extender ~~~~~~~~~~~~~~~~~~~~~ */
static const TCA6408Driver_config tca6408Config = {
    .i2cPort = &I2CD1,
    .i2cAddr = 0x20
};

TCA6408Driver mixerControllerIO;

/* ~~~~~~~~~~~~~~~~~~~~~ I2C TCXO Temp sensor ~~~~~~~~~~~~~~~~~~~~~ */
static const MCP9804Driver_config mcp9804Config = {
    .i2cPort = &I2CD1,
    .i2cAddr = 0x18
};

MCP9804Driver tcxoTempSensor;

/* ~~~~~~~~~~~~~~~~~~~~~ Reboot every 7 days ~~~~~~~~~~~~~~~~~~~~~ */
static bool sanityRebootMonitorTask(void* param)
{
    (void)param;

    uint32_t numberOfSeconds = sanityRebootSeconds;

    if(numberOfSeconds &&
            osalOsGetSystemTimeX() >= OSAL_S2ST(numberOfSeconds)) {
        RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
        BKP->DR10 = 0x424D;
        NVIC_SystemReset();
    }

    wdgReset(&WDGD1);

    return true;
}

MonitorEntry sanityReboot;

/* ~~~~~~~~~~~~~~~~~~~~~ GPIO Init data ~~~~~~~~~~~~~~~~~~~~~ */
static const GPIOPinInit platformPinConfig[] = {
    {GPIO_PLL_SCK, PAL_MODE_STM32_ALTERNATE_PUSHPULL, false},
    {GPIO_PLL_MISO, PAL_MODE_STM32_ALTERNATE_PUSHPULL, false},
    {GPIO_PLL_MOSI, PAL_MODE_STM32_ALTERNATE_PUSHPULL, false},
    {GPIO_PLL_SS, PAL_MODE_OUTPUT_PUSHPULL, true},
    {GPIO_PLL_CE, PAL_MODE_OUTPUT_PUSHPULL, false},
    {GPIO_I2C_SCL, PAL_MODE_STM32_ALTERNATE_OPENDRAIN, false},
    {GPIO_I2C_SDA, PAL_MODE_STM32_ALTERNATE_OPENDRAIN, false},
    {GPIO_LED_USB, PAL_MODE_OUTPUT_PUSHPULL, true},
    {GPIO_LED_LOCK, PAL_MODE_OUTPUT_PUSHPULL, true},
    {GPIO_LED_MIX, PAL_MODE_OUTPUT_PUSHPULL, true},
    {GPIO_MIX_SW_EN, PAL_MODE_OUTPUT_OPENDRAIN, true},
    {GPIO_MIX_SW_LO, PAL_MODE_STM32_ALTERNATE_PUSHPULL, false},
    {GPIO_UART_TX, PAL_MODE_STM32_ALTERNATE_PUSHPULL, true},
    {GPIO_UART_RX, PAL_MODE_INPUT, true},
    {-1, 0, false}
};

static const GPIOPinInit platformI2CPinConfig[] = {
    {GPIO_ANT_HIGH, PAL_MODE_OUTPUT_PUSHPULL, false},
    {GPIO_ANT_MID, PAL_MODE_OUTPUT_PUSHPULL, false},
    {GPIO_SW_BYPASS, PAL_MODE_OUTPUT_PUSHPULL, false},
    {GPIO_SW_MIX, PAL_MODE_OUTPUT_PUSHPULL, false},
    {GPIO_SW_SW, PAL_MODE_OUTPUT_PUSHPULL, false},
    {GPIO_MIX_X2, PAL_MODE_OUTPUT_PUSHPULL, false},
    {GPIO_MIX_EN, PAL_MODE_OUTPUT_PUSHPULL, false},
    {GPIO_LOWBAND, PAL_MODE_OUTPUT_PUSHPULL, false},
    {-1, 0, false}
};

/* ~~~~~~~~~~~~~~~~~~~~~ Converter ~~~~~~~~~~~~~~~~~~~~~ */
ConverterManager converter;
static const uint32_t bandSpecificGpioSettings[] = {
    _BV(CONVERTER_IO_PIN_SW_SW) | _BV(CONVERTER_IO_PIN_LED1), 	 /* SW */
    _BV(CONVERTER_IO_PIN_LOWBAND) | _BV(CONVERTER_IO_PIN_SW_MIX) | _BV(CONVERTER_IO_PIN_LED2),    /* SHF, 1.5-3.5 GHz */
    _BV(CONVERTER_IO_PIN_SW_MIX) | _BV(CONVERTER_IO_PIN_LED2), /* SHF, 3.5-6.5 GHz */
    _BV(CONVERTER_IO_PIN_SW_MIX) | _BV(CONVERTER_IO_PIN_LED2), /* SHF, 6.5-10.5 GHz */
    _BV(CONVERTER_IO_PIN_SW_MIX) | _BV(CONVERTER_IO_PIN_LED2), /* SHF, 10.5-13.5 GHz */
    _BV(CONVERTER_IO_PIN_SW_BYPASS), /* Bypass */
};

static xTaskHandle mixLedTaskHandle;
static volatile uint32_t mixLedDelay = 0;

static void converterSetGpio(const ConverterManager* converter, uint32_t gpioValues)
{
    /* We need to set some IO depending on the active band */

    gpioValues |= bandSpecificGpioSettings[converter->activeBand];

    /* The lowest 8 bits are all connected over I2C */
    uint8_t i2cGpio = gpioValues & 0xFF;
    gpioSetPort(GPIO_PORT_I2C0, i2cGpio);

    /* Handle the others */
    gpioSetPin(GPIO_MIX_SW_EN, (gpioValues & _BV(CONVERTER_IO_PIN_MIX_SW_EN)) == 0);
    systemEnableMCO((gpioValues & _BV(CONVERTER_IO_PIN_MIX_SW_LO)) > 0);


    uint32_t mixBlinkDelay = 0;
    mixBlinkDelay += (gpioValues & _BV(CONVERTER_IO_PIN_LED1))? 100:0;
    mixBlinkDelay += (gpioValues & _BV(CONVERTER_IO_PIN_LED2))? 200:0;

    mixLedDelay = mixBlinkDelay;
    vTaskResume(mixLedTaskHandle);
}

static bool converterSHFLoTune(uint32_t freqKHz, int8_t power)
{
    MAX2870TuneRequest tune;
    tune.fastLockDurationMicroseconds = 0;
    tune.useVCOAutotune = false;
    tune.powerA = power;
    tune.powerB = -127;
    tune.frequency = (uint64_t)freqKHz * 1000;

    return max2870Tune(&loPLL, &tune) == TUNE_OK;
}

ConverterSHFConfig converterSHFBandConfig[4] = {
    {.useHighSideMixing = true,  .ifFrequencyKHz = CONVERTER_IF_FREQ, converterSHFLoTune},
    {.useHighSideMixing = true,  .ifFrequencyKHz = CONVERTER_IF_FREQ, converterSHFLoTune},
    {.useHighSideMixing = true,  .ifFrequencyKHz = CONVERTER_IF_FREQ, converterSHFLoTune},
    {.useHighSideMixing = false, .ifFrequencyKHz = CONVERTER_IF_FREQ, converterSHFLoTune},
};

static const ConverterBand converterBands[] = {
    {
        .bandName = "SW",
        .minFrequencyKHz = 0,
        .maxFrequencyKHz = 27000,
        .userData = NULL,
        .functions = &converterSWFunctions
    },
    {
        .bandName = "SHF-L",
        .minFrequencyKHz = 1500000,
        .maxFrequencyKHz = 3500000,
        .userData = &converterSHFBandConfig[0],
        .functions = &converterSHFFunctions
    },
    {
        .bandName = "SHF-M",
        .minFrequencyKHz = 3500000,
        .maxFrequencyKHz = 6500000,
        .userData = &converterSHFBandConfig[1],
        .functions = &converterSHFFunctions
    },
    {
        .bandName = "SHF-H",
        .minFrequencyKHz = 6500000,
        .maxFrequencyKHz = 12000000 - CONVERTER_IF_FREQ,
        .userData = &converterSHFBandConfig[2],
        .functions = &converterSHFFunctions
    },
    {
        .bandName = "SHF-H+",
        .minFrequencyKHz = 12000000 - CONVERTER_IF_FREQ,
        .maxFrequencyKHz = 12000000 + CONVERTER_IF_FREQ,
        .userData = &converterSHFBandConfig[3],
        .functions = &converterSHFFunctions
    },
    {
        .bandName = "Bypass",
        .minFrequencyKHz = 0,
        .maxFrequencyKHz = 20000000,
        .userData = NULL,
        .functions = &converterBypassFunctions
    },
    {.functions = NULL}
};

ConverterTuneRequest startupTuneRequest = {
    .inputFrequencyKHz = 0,
    .antennaInput = 0,
    .forceBand = true,
    .bandId = 5 /* Bypass */
};

/* ~~~~~~~~~~~~~~~~~~~~~ Init function ~~~~~~~~~~~~~~~~~~~~~ */
void startSystemComponents(void)
{
    bool hasI2CIO = false, hasLoPLL = false, hasSWMix = true;

    /* Unplug USB */
    usbDisconnectBus(serusbcfg.usbp);

    /* Init shell */
    shellInitApp();

    /* Init GPIO */
    if(!gpioInit(4)) {
        /* ?! Damn... */
        syslog("GPIO subsystem init failed.");
        return;
    } else {
        syslog("GPIO subsystem init done.");

        /* Add the CPU ports */
        gpioCPUInit(GPIO_PORT_GPIOA, GPIOA);
        gpioCPUInit(GPIO_PORT_GPIOB, GPIOB);
        gpioCPUInit(GPIO_PORT_GPIOC, GPIOC);

        /* Register command */
        shellCommandRegister("gpio", cmdGPIO, NULL);
    }

    if(utilIsPinFloating(GPIO_MIX_SW_EN)) {
        hasSWMix = false;
    }

    /* Set pin modes and values */
    gpioInitPins(platformPinConfig);

    /* Start I2C */
    i2cSafeInit(&I2CD1, &I2C1SafeConfig);
    shellCommandRegister("i2c", cmdI2C, &I2CD1);

    /* Start serial port */
    sdStart(&SD3, &serial3Config);

    /* Init IO extender */
    if(!TCA6408AInit(NULL, gpioRegisterPortDriver(GPIO_PORT_I2C0), &tca6408Config)) {
        syslog("TCA6408A init failed.");
    } else {
        syslog("TCA6408A init done.");
        gpioInitPins(platformI2CPinConfig);

        hasI2CIO = true;
    }

    xTaskCreate(mixLedTask, "Led", 64, (void*)&mixLedDelay, 1, &mixLedTaskHandle);

    /* Init PLL Driver and build VCO cache */
    if(max2870Init(&loPLL, &max2870Config) && max2870VcoPrecal(&loPLL)) {
		shellCommandRegister("max", cmdMax, &loPLL);

		syslog("MAX2870 init done.");

		hasLoPLL = true;
    } else {
    	syslog("MAX2870 init failed.");
    }

    /* Init temp sensor */
    if(!MCP9804Init(&tcxoTempSensor, &mcp9804Config)) {
        syslog("Temperature sensor init failed.");
    } else {
        syslog("Temperature sensor init done.");
        shellCommandRegister("temp", cmdTemp, &tcxoTempSensor);
    }

    /* Init the converter, if I2C is present */
    if(hasI2CIO) {
        converterInit(&converter, converterBands, converterSetGpio);
        shellCommandRegister("convert", cmdConvert, &converter);

        /* Without the LO PLL all SHF bands are not available */
        if(!hasLoPLL) {
            syslog("SHF bands disabled!");
            converter.disabledBands |= _BV(1) | _BV(2) | _BV(3) | _BV(4);
        }

        /* Without the SW mixer the SW band is not available (duh) */
        if(!hasSWMix) {
            syslog("SW band disabled!");
            converter.disabledBands |= _BV(0);
        }

        if(!converterTune(&converter, &startupTuneRequest)) {
            syslog("Converter startup failed.");
        } else {
            syslog("Converter startup done.");
        }

    }


    /* Start sanity reboot monitor */
    monitorEntryRegister(&sanityReboot, &sanityRebootMonitorTask, NULL, "Sanity");
    monitorKick(&sanityReboot, true);

    /* Init monitor thread */
    monitorInit(128);

    /* Init USB */
    usbInitSerialPort();
    syslog("USB init done.");
}


void systemEnableMCO(bool enable)
{
    if(enable) {
        RCC->CFGR |= STM32_MCOSEL_SYSCLK;
    } else {
        RCC->CFGR &=~ STM32_MCOSEL_SYSCLK;
    }
}

/*
 * This main function starts FreeRTOS, we also create one thread that
 * will eventually run main_.
 */
int main(void)
{
    halInit();

    /* Before anything else, start watchdog (~25 seconds timeout) */
    wdgStart(&WDGD1, &watchdogConfig);

    TaskHandle_t handle;
    xTaskCreate(runInThreadBody, "Init", 256, main_, 1, &handle);

    osalSysEnable();

    /* Does not happen */
    osalSysHalt("Return in main?");
    return 0;
}
