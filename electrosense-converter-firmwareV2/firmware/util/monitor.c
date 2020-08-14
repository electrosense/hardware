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

#include "monitor.h"
#include "chprintf.h"
#include "shell.h"
#include "util.h"

static TaskHandle_t monThreadHandle;
static MonitorEntry* monThreadFirst;

/* This thread is shared among multiple devices */
THD_FUNCTION(taskPeriodicMon, param)
{
    (void) param;

    for(;;) {
        bool workToCome = false;

        MonitorEntry* entry = monThreadFirst;
        while(entry) {
            if(entry->active) {
                if(entry->callback) {
                    entry->active = entry->callback(entry->param);
                    workToCome |= entry->active;
                }
            }

            entry = entry->next;
        }

        if(!workToCome) {
            /* Sleep */
            uint32_t ulInterruptStatus;
            xTaskNotifyWait(0, ULONG_MAX, (uint32_t*)&ulInterruptStatus, portMAX_DELAY);
        }

        osalThreadSleepSeconds(10);
    }
}

void monitorInit(unsigned int stackSize)
{
    xTaskCreate(taskPeriodicMon, "Monitor", stackSize, NULL, 1, &monThreadHandle );
}

void monitorKick(MonitorEntry* entry, bool active)
{
    if(entry->active == active) {
        return;
    }

    entry->active = active;

    if(monThreadHandle && active) {
        xTaskNotify(monThreadHandle, 1, eSetValueWithOverwrite);
    }
}

void monitorEntryRegister(MonitorEntry* entry, bool(*callback)(void* param), void* param, char* name)
{
    entry->name = name;
    entry->callback = callback;
    entry->param = param;
    entry->next = monThreadFirst;
    monThreadFirst = entry;
}

void monitorPrintStatus(BaseSequentialStream* chp)
{
    printfFixed(chp, 13, "Monitor ID");
    printfFixed(chp, 10, "Task Name");
    printfFixed(chp, 11, "Descriptor");
    printfFixed(chp, 11, "Callback");
    chprintf(chp, "State"SHELL_NEWLINE_STR);

    MonitorEntry* entry = monThreadFirst;
    unsigned int index = 0;
    while(entry) {
        printfFixed(chp, 13, "%u", index);
        if(entry->name) {
            printfFixed(chp, 10, "%s",entry->name);
        } else {
            printfFixed(chp, 10, "");
        }
        printfFixed(chp, 11, "%08x", entry);
        printfFixed(chp, 11, "%08x", entry->callback);
        if(entry->active) {
            chprintf(chp, "Active"SHELL_NEWLINE_STR);
        } else {
            chprintf(chp, "Idle"SHELL_NEWLINE_STR);
        }
        index++;
        entry = entry->next;
    }
}
