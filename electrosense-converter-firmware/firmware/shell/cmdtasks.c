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

#include "appshell.h"
#include "chprintf.h"

#include <string.h>

static void cmdThreadPrintState(BaseSequentialStream *chp, eTaskState state)
{
    if(state == eRunning) printfFixed(chp, 9, "Running");
    else if(state == eReady) printfFixed(chp, 9, "Ready");
    else if(state == eBlocked) printfFixed(chp, 9, "Blocked");
    else if(state == eSuspended) printfFixed(chp, 9, "Suspended");
    else if(state == eDeleted) printfFixed(chp, 9, "Deleted");
    else printfFixed(chp, 9, "Invalid");
}

void cmdThreadInfo(void* user, BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    (void) user;

    unsigned int numberOfTasks = uxTaskGetNumberOfTasks();
    TaskStatus_t* taskStatusArray = pvPortMalloc(numberOfTasks * sizeof(TaskStatus_t));
    if(!taskStatusArray) {
        return;
    }

    uint32_t totalRunTime;
    unsigned int  result = uxTaskGetSystemState(taskStatusArray, numberOfTasks, &totalRunTime);
    printfFixed(chp, 13, "Task ID");
    printfFixed(chp, 10, "Task Name");
    printfFixed(chp, 11, "Stack Base");
    printfFixed(chp, 11, "Free Stack");
    printfFixed(chp, 6, "PrioB");
#if configUSE_MUTEXES
    printfFixed(chp, 6, "PrioC");
#endif
    printfFixed(chp, 9, "State");
    chprintf(chp,SHELL_NEWLINE_STR);
    for(unsigned int i = 0; i<result; i++) {
        printfFixed(chp, 13, "%u=%08x ", taskStatusArray[i].xTaskNumber, taskStatusArray[i].xHandle);
        if(!strcmp("IDLE", taskStatusArray[i].pcTaskName)) {
            printfFixed(chp, 10, "Idle");
        } else {
            printfFixed(chp, 10, "%s", taskStatusArray[i].pcTaskName);
        }
        printfFixed(chp, 11, "%08x", taskStatusArray[i].pxStackBase);
        printfFixed(chp, 11, "%u", (uint32_t)taskStatusArray[i].usStackHighWaterMark);
        printfFixed(chp, 6, "%u", taskStatusArray[i].uxBasePriority);
#if configUSE_MUTEXES
        printfFixed(chp, 6, "%u", taskStatusArray[i].uxCurrentPriority);
#endif
        if(taskStatusArray[i].xHandle == xGetCurrentTaskHandle()) {
            /* The scheduler is suspended, but the current task would be running otherwise */
            cmdThreadPrintState(chp, eRunning);
        } else {
            cmdThreadPrintState(chp, taskStatusArray[i].eCurrentState);
        }
        chprintf(chp,SHELL_NEWLINE_STR);
    }

    vPortFree(taskStatusArray);
    chprintf(chp,SHELL_NEWLINE_STR);
    monitorPrintStatus(chp);
}
