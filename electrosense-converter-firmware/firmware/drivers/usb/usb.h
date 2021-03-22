/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio
      						2017 	   Bertold Van den Bergh


    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "hal.h"

#ifndef USBCFG_H
#define USBCFG_H

extern SerialUSBDriver SDU1;
extern volatile bool usbConnectState;

void usbDisableSOF(bool doIt);
uint32_t usbGetSofCounter(bool* sofDisableAllowed);
void usbInitSerialPort(void);

#endif  /* USBCFG_H */

/** @} */
