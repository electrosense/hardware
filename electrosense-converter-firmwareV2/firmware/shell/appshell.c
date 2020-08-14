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
#include "../util/syslog/syslog.h"

#include <string.h>
#include "chprintf.h"

static uint8_t incr0bxlr[] __attribute__ ((aligned (4))) = {0x01, 0x30, 0x70, 0x47} ; /* adds r0, 1; bx lr; */

static void cmdReboot(void* user, BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) chp;
    (void) argc;
    (void) argv;
    (void) user;

    uint16_t rebootCode = 0;

    if(argc == 0) {
        chprintf(chp,"Usage:"SHELL_NEWLINE_STR);
        chprintf(chp,"\t reboot nvic"SHELL_NEWLINE_STR);
        chprintf(chp,"\t reboot loader"SHELL_NEWLINE_STR);
        chprintf(chp,"\t reboot loaderp"SHELL_NEWLINE_STR);
        chprintf(chp,"\t reboot crash"SHELL_NEWLINE_STR);
        chprintf(chp,"\t reboot memexec"SHELL_NEWLINE_STR);
        return;
    } else if(argc == 1 && !strcmp(argv[0], "loaderp")) {
        /* Set a flag for permanent bootloader */
        rebootCode = 0x424C;
    } else if(argc == 1 && !strcmp(argv[0], "loader")) {
        /* Normal reboot, timed bootloader */
    } else if(argc == 1 && !strcmp(argv[0], "crash")) {
        /* Try to crash the system, for testing */
        osalSysHalt("crash");
        return;
    } else if(argc == 1 && !strcmp(argv[0], "memexec")) {
        /* Try to execute code from RAM, for testing */
        uint32_t (*callIt)(uint32_t) = (uint32_t(*)(uint32_t))(incr0bxlr+1); /* +1 for thumb mode */
        if(callIt(4) == 5 && callIt(54) == 55) {
            chprintf(chp, "Success!"SHELL_NEWLINE_STR);
        } else {
            chprintf(chp, "Wrong result!"SHELL_NEWLINE_STR);
        }
        return;
    } else {
        /* Skip bootloader */
        rebootCode = 0x424D;
    }
    RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
    BKP->DR10 = rebootCode;
    /* All ends in reset */
    NVIC_SystemReset();
}

static void cmdSyslog(void* user, BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) user;

    if(argc == 1 && !strcmp(argv[0], "clear")) {
        syslogClear();
    } else if(argc == 2 && !strcmp(argv[0], "add")) {
        syslog("%s", argv[1]);
    } else {
        syslogDump(chp);
    }
}

static void cmdMco(void* user, BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) user;

    if(argc != 1) {
        chprintf(chp,"Usage:"SHELL_NEWLINE_STR);
        chprintf(chp,"\t mco [1/0]"SHELL_NEWLINE_STR);
    } else {
        systemEnableMCO(argv[0][0] == '1');
    }
}

extern char _binary_data_license_bin_start;
static void cmdLicense(void* user, BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) chp;
    (void) argc;
    (void) argv;
    (void) user;

#ifdef LICENSE_URL 
    chprintf(chp, "License info: %s"SHELL_NEWLINE_STR, LICENSE_URL);
#else
    chprintf(chp,"%s", &_binary_data_license_bin_start);
#endif
}


static uint8_t shellCommandsIndex = 7;
static ShellCommand shellCommands[15] = {
    {"tasks", cmdThreadInfo, NULL},
    {"syslog", cmdSyslog, NULL},
    {"reboot", cmdReboot, NULL},
    {"sof", cmdSof, NULL},
    {"mco", cmdMco, NULL},
    {"license", cmdLicense, NULL},
    {"sanity", cmdSanity, NULL},
};

void shellCommandRegister(char* name, shellcmd_t function, void* user)
{
    /* -1 since there should be a final line with NULL, NULL, NULL */
    if(shellCommandsIndex >= sizeof(shellCommands)/sizeof(ShellCommand)-1) {
        return;
    }
    shellCommands[shellCommandsIndex].sc_name = name;
    shellCommands[shellCommandsIndex].sc_function = function;
    shellCommands[shellCommandsIndex].sc_user = user;
    shellCommandsIndex++;
}

typedef struct {
    const char* taskName;
    ShellConfig shellCfg;
    BaseSequentialStream* stream;
    void(*terminateCallback)(void* param);
    void* param;
} ActiveShell;


static void shellTerminationCallback(event_source_t* source, eventflags_t set)
{
    (void)source;
    (void)set;

    /* The Shell API is a bit stupid. It doesn't tell you which shell exited... */
    ActiveShell* activeShell = pvTaskGetThreadLocalStoragePointer(NULL, 2);
    if(activeShell) {
        /* Free the memory */
        if(activeShell->shellCfg.sc_histbuf) {
            vPortFree(activeShell->shellCfg.sc_histbuf);
        }

        chprintf(activeShell->stream, "Shell 0x%08x closed.", activeShell);
        syslog("Shell 0x%08x closed.", activeShell);

        void(*callback)(void*) = activeShell->terminateCallback;
        void *param = activeShell->param;

        vPortFree(activeShell);

        if(callback) {
            callback(param);
        }
    }
}

void shellInitApp()
{
    shellInit();

    osalSysLock();
    osalEventRegisterCallbackI(&shell_terminated, shellTerminationCallback);
    osalSysUnlock();
}

static void shellStartThread(void* param)
{
    ActiveShell* activeShell = (ActiveShell*)param;

    vTaskSetThreadLocalStoragePointer(NULL, 2, activeShell);

    syslog("Shell 0x%08x started (%s).", activeShell, activeShell->taskName);

    shellThread(&activeShell->shellCfg);
}

bool shellStart(BaseSequentialStream* stream,
                unsigned int histSize, const char* taskName,
                void(*terminateCallback)(void* param), void* param)
{
    ActiveShell* activeShell = pvPortMalloc(sizeof(ActiveShell));
    if(!activeShell) {
        return false;
    }

    memset(activeShell, 0, sizeof(*activeShell));

    activeShell->param = param;
    activeShell->terminateCallback = terminateCallback;

    activeShell->shellCfg.sc_channel = stream;
    activeShell->shellCfg.sc_commands = shellCommands;

    if(histSize) {
        activeShell->shellCfg.sc_histbuf = pvPortMalloc(histSize);
        if(activeShell->shellCfg.sc_histbuf) {
            memset(activeShell->shellCfg.sc_histbuf, 0, histSize);
            activeShell->shellCfg.sc_histsize = histSize;
        }
    }

    activeShell->stream = stream;
    activeShell->taskName = taskName;
    xTaskCreate(shellStartThread, taskName, 256, activeShell, 2, NULL);

    return true;
}
