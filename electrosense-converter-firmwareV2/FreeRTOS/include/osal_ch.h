/*
 * OSAL_CH v0.1.0
 * Copyright (C) 2017 Bertold Van den Bergh.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */
#ifndef _OSAL_CH_H_
#define _OSAL_CH_H_

#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "portmacro.h"

/* HAL Compatibility functions */
UBaseType_t uxYieldPending( void );
xTaskHandle xGetCurrentTaskHandle( void );

#define FALSE false
#define TRUE true
#define OSAL_SUCCESS false
#define OSAL_FAILED true

/* FreeRTOS can work with both 16 and 32-bit ticks */
#if configUSE_16_BIT_TICKS == 0
#define OSAL_ST_RESOLUTION 32
#else
#define OSAL_ST_RESOLUTION 16
#endif

/* Standard time constants */
#define TIME_IMMEDIATE ((systime_t)0)
#define TIME_INFINITE  ((systime_t)portMAX_DELAY)

/* ChibiOS HAL can setup SysTick for you, but FreeRTOS already did it */
#define OSAL_ST_MODE_NONE 0
#define OSAL_ST_MODE      OSAL_ST_MODE_NONE

/* Standard message types used by the HAL */
#define MSG_OK      (msg_t)0
#define MSG_TIMEOUT (msg_t)-1
#define MSG_RESET   (msg_t)-2
#define MSG_EVENT_W (msg_t)-3

/* Systick rate set by FreeRTOS */
#define OSAL_ST_FREQUENCY configTICK_RATE_HZ

/* A lot of typedefs, matching FreeRTOS types with ChibiOS */
/* Thread related */
typedef TaskHandle_t thread_t;
typedef thread_t * thread_reference_t;
typedef struct {
    thread_t head;
    thread_t tail;
} threads_queue_t;

/* Stores the system state before enterring critical section */
typedef UBaseType_t syssts_t;

/* SysTick and cycle time */
typedef TickType_t systime_t;
typedef unsigned long rtcnt_t;

/* Message ID */
typedef int32_t msg_t;

/* Event/Poll related */
typedef UBaseType_t eventflags_t;
typedef struct event_repeater event_repeater_t;

typedef struct event_source_s event_source_t;

struct event_source_s {
    event_repeater_t* firstRepeater;
    eventflags_t setEvents;
    thread_reference_t waitThread;
    void (*eventCallback)(event_source_t* source, eventflags_t set);
};

typedef struct event_repeater {
    event_repeater_t* nextRepeater;
    event_repeater_t* prevRepeater;
    event_source_t* source;
    event_source_t* target;
    eventflags_t triggerEvents;
    eventflags_t setEvents;
    eventflags_t myEvent;
} event_repeater_t;

/* Mutex type */
typedef struct {
    SemaphoreHandle_t handle;
    StaticSemaphore_t staticData;
} mutex_t;

/* Some more complex functions are defined in osal_ch.c */
#ifdef __cplusplus
extern "C" {
#endif
msg_t osalThreadEnqueueTimeoutS(threads_queue_t* thread_queue, systime_t timeout);
void osalThreadDequeueAllI(threads_queue_t* thread_queue, msg_t msg);
void osalThreadDequeueNextI(threads_queue_t* thread_queue, msg_t msg);

msg_t osalThreadSuspendS(thread_reference_t* thread_reference);
msg_t osalThreadSuspendTimeoutS(thread_reference_t* thread_reference, systime_t timeout);

eventflags_t osalEventWaitTimeoutS(event_source_t* event_source, systime_t timeout);
void osalEventBroadcastFlagsI(event_source_t* event_source, eventflags_t set);
void osalEventRepeaterRegister(event_repeater_t* event_repeater,
                               event_source_t* event_source,
                               eventflags_t trigger_events,
                               event_source_t* target_event_sink,
                               eventflags_t my_event);
void osalEventRepeaterUnregister(event_repeater_t* event_repeater);
void osalEventRegisterCallbackI(event_source_t* source, void (*eventCallback)(event_source_t* source, eventflags_t set));
#ifdef __cplusplus
}
#endif

/* FreeRTOS thread function template */
#define THD_FUNCTION(func, param) void func(void* param)

/* Debug functions */
#if( configASSERT_DEFINED == 1 )
#define osalDbgAssert(mustBeTrue, msg) do {  \
    if (!(mustBeTrue)) {                     \
        osalSysHalt(msg);                    \
    }                                        \
} while(false);

#define osalSysHalt(text) do {                                      \
    osalSysDisable();                                               \
    errorAssertCalled( __FILE__, __LINE__, text );                  \
    while(1);                                                       \
} while(0)

#else
#define osalDbgAssert(mustBeTrue, msg)

#define osalSysHalt(text) do {                                      \
    osalSysDisable();                                               \
    while(1);                                                       \
} while(0)
#endif

#define osalDbgCheck(mustBeTrue) osalDbgAssert(mustBeTrue, __func__)

/* Many HAL functions use these for safety checking */
#if( configASSERT_DEFINED == 1 )
#define osalDbgCheckClassI() do {                                       \
    osalDbgAssert(xPortIsCriticalSection(), "not in critical section"); \
}while(0);

#define osalDbgCheckClassS() do {                                   \
    osalDbgCheckClassI();                                           \
    osalDbgAssert(!xPortIsInsideInterrupt(), "in interrupt");       \
}while(0);

#else

#define osalDbgCheckClassI()
#define osalDbgCheckClassS()
#endif

/* Proxy functions to FreeRTOS */
#define osalSysDisable vTaskEndScheduler
#define osalSysEnable vTaskStartScheduler
#define osalSysLock taskENTER_CRITICAL
#define osalSysGetStatusAndLockX taskENTER_CRITICAL_FROM_ISR
#define osalSysRestoreStatusX(state) taskEXIT_CRITICAL_FROM_ISR(state)
#define osalOsGetSystemTimeX xTaskGetTickCountFromISR
#define osalThreadSleepS(time) vTaskDelay(time)
#define osalThreadSleep(time) vTaskDelay(time)

/* More proxying using simple inline functions */
extern UBaseType_t uxSavedInterruptStatus;

static inline void osalOsRescheduleS(void)
{
    osalDbgCheckClassS();
    if(uxYieldPending()) taskYIELD();
}

static inline void osalSysLockFromISR(void)
{
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
}

static inline void osalSysUnlockFromISR(void)
{
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}

static inline void osalSysUnlock(void){
    osalOsRescheduleS();
    taskEXIT_CRITICAL();
}

static inline void osalEventBroadcastFlags(event_source_t* event_source, eventflags_t set)
{
    osalDbgCheck(event_source != NULL);

    osalSysLock();
    osalEventBroadcastFlagsI(event_source, set);
    osalSysUnlock();
}

static inline eventflags_t osalEventRepeaterGetS(event_repeater_t* event_repeater){
    eventflags_t flags = event_repeater->setEvents;
    event_repeater->setEvents = 0;
    return flags;
}

static inline void osalMutexLock(mutex_t* mutex)
{
    osalDbgCheck(mutex != NULL);
    xSemaphoreTake(mutex->handle, portMAX_DELAY);
}

static inline void osalMutexUnlock(mutex_t* mutex)
{
    osalDbgCheck(mutex != NULL);
    xSemaphoreGive(mutex->handle);
}

static inline void osalThreadResumeI(thread_reference_t* thread_reference, msg_t msg)
{
    osalDbgCheckClassI();
    if(*thread_reference) {
        xTaskNotifyFromISR( *thread_reference, msg, eSetValueWithOverwrite, NULL );
        *thread_reference = NULL;
    }
}

static inline void osalThreadResumeS(thread_reference_t* thread_reference, msg_t msg)
{
    osalDbgCheckClassS();
    if(*thread_reference) {
        xTaskNotify( *thread_reference, msg, eSetValueWithOverwrite );
        *thread_reference = NULL;
    }
}

static inline void osalSysPolledDelayX(rtcnt_t cycles)
{
    vPortBusyDelay(cycles);
}

/* Init objects */
static inline void osalThreadQueueObjectInit(threads_queue_t* thread_queue)
{
    osalDbgCheck(thread_queue != NULL);
    thread_queue->head = NULL;
    thread_queue->tail = NULL;
}

static inline void osalMutexObjectInit(mutex_t* mutex)
{
    osalDbgCheck(mutex != NULL);
    mutex->handle = xSemaphoreCreateMutexStatic(&mutex->staticData);
}

static inline void osalEventObjectInit(event_source_t* event_source)
{
    osalDbgCheck(event_source != NULL);
    event_source->setEvents = 0;
    event_source->firstRepeater = NULL;
    event_source->waitThread = NULL;
}

#define OSAL_IRQ_PROLOGUE()
/* At the end of the interrupt handler we need to check if we need
 * to yield */
#define OSAL_IRQ_EPILOGUE() {                  \
    portYIELD_FROM_ISR(uxYieldPending());      \
}
#define OSAL_IRQ_HANDLER(handleName) void handleName(void)

/* Macros converting real time to systicks */
typedef uint32_t conv_t;
#define OSAL_T2ST(r, t, hz, div) ((r)(((conv_t)(hz) * (conv_t)(t) + (conv_t)(div-1)) / (conv_t)(div)))

#define OSAL_US2ST(t) OSAL_T2ST(systime_t, t, configTICK_RATE_HZ, 1000000)
#define OSAL_MS2ST(t) OSAL_T2ST(systime_t, t, configTICK_RATE_HZ, 1000)
#define OSAL_S2ST(t)  OSAL_T2ST(systime_t, t, configTICK_RATE_HZ, 1)

/* And easy-to-use sleep macros */
#define osalThreadSleepMicroseconds(t) osalThreadSleep(OSAL_US2ST(t))
#define osalThreadSleepMilliseconds(t) osalThreadSleep(OSAL_MS2ST(t))
#define osalThreadSleepSeconds(t)      osalThreadSleep(OSAL_S2ST(t))

/* Why is this in the OSAL? It is always the same? */
static inline bool osalOsIsTimeWithinX(systime_t now, systime_t begin, systime_t end)
{
    systime_t duration = end - begin;
    systime_t past = now - begin;

    if(past < duration) return true;
    
    return false;
}

/* FreeRTOS has a runtime check for invalid priorities, making this check redudant */
#define OSAL_IRQ_IS_VALID_PRIORITY(n) true

/* No initialization needed */
static inline void osalInit(void)
{
}

/* Some code in ChibiOS seems to call the kernel directly instead of using the osal */
#define chSysLock osalSysLock
#define chSysUnock osalSysUnlock
#define chEvtObjectInit osalEventObjectInit
#define chVTGetSystemTime osalOsGetSystemTimeX
#define chCoreGetStatusX xPortGetFreeHeapSize
static inline void chThdExitS(msg_t msg){
    (void)msg;

    vTaskDelete(NULL);
}
static inline void chEvtBroadcast(event_source_t* event_source){
    osalEventBroadcastFlags(event_source, 1);
}
static inline void chEvtBroadcastI(event_source_t* event_source){
    osalEventBroadcastFlagsI(event_source, 1);
}
#define CH_KERNEL_VERSION "FreeRTOS v10"
#define CH_CFG_USE_MEMCORE TRUE
#define CH_CFG_USE_HEAP TRUE
#endif

size_t chHeapStatus  (void* ignore, size_t* memFree, size_t* largestBlock);
