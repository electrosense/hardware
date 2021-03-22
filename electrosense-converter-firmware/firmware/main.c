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
#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"

#include "drivers/system.h"
#include "shell/appshell.h"
#include "util/syslog/syslog.h"

void main_(void)
{
    /* Start syslog */
    syslogInit(1024);

    /* Start all components */
    startSystemComponents();

    /* Start the shell on serial port (USB is done later) */
    shellStart((BaseSequentialStream*)&SD3, 128, "ShellTTL", NULL, NULL);
}

static volatile bool usbShellIsUp = false;
static void usbShellTerminated(void* param)
{
    (void)param;

    gpioSetPin(GPIO_LED_USB, true);
    osalThreadSleepSeconds(1);
    usbShellIsUp = false;
}

void vApplicationIdleHook(void)
{
    syslogIdleHook();

    if(usbConnectState && !usbShellIsUp) {
        usbShellIsUp = true;
        /*
         * Setting the pin from the idle hook requires a non-blocking
         * GPIO driver. In practice this means almost always a native
         * CPU pin.
         */
        gpioSetPin(GPIO_LED_USB, false);

        /* Try to start the shell */
        if(!shellStart((BaseSequentialStream*)&SDU1, 128, "ShellUSB", usbShellTerminated, NULL)) {
            /* We are very low on memory. This will keep trying, but priority is IDLE */
            usbShellIsUp = false;
        }
    }
}

void errorAssertCalled(const char* file, unsigned long line, const char* reason)
{
    (void)file;
    (void)line;
    (void)reason;
    while(1);
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    (void)xTask;
    (void)pcTaskName;
    while(1);
}
