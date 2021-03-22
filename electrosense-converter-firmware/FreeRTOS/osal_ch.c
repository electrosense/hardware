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

#include "osal_ch.h"

/* In this file there are slightly more complicated functions */

#define LOCAL_STORAGE_QUEUE_NEXT 0 
#define LOCAL_STORAGE_QUEUE_PREV 1

msg_t osalThreadEnqueueTimeoutS(threads_queue_t* thread_queue, systime_t timeout)
{
    if(!timeout) {
        return MSG_TIMEOUT;
    }

    osalDbgCheck(thread_queue != NULL);
    osalDbgCheckClassS();

    thread_t currentTask = xGetCurrentTaskHandle();
   
    /* Insert in the front */
    vTaskSetThreadLocalStoragePointer(currentTask, LOCAL_STORAGE_QUEUE_NEXT, thread_queue->head);
    vTaskSetThreadLocalStoragePointer(currentTask, LOCAL_STORAGE_QUEUE_PREV, NULL);

    if(thread_queue->head){
        vTaskSetThreadLocalStoragePointer(thread_queue->head, LOCAL_STORAGE_QUEUE_PREV, currentTask);
    }
    thread_queue->head = currentTask;
    if(!thread_queue->tail){
        thread_queue->tail = currentTask;
    }

    msg_t msg = osalThreadSuspendTimeoutS(NULL, timeout);
    
    /* Remove from the queue in case of timeout */
    if(msg == MSG_TIMEOUT) {
        thread_t nextTask = pvTaskGetThreadLocalStoragePointer(currentTask, LOCAL_STORAGE_QUEUE_NEXT);
        thread_t prevTask = pvTaskGetThreadLocalStoragePointer(currentTask, LOCAL_STORAGE_QUEUE_PREV);

        if(nextTask){
            vTaskSetThreadLocalStoragePointer(nextTask, LOCAL_STORAGE_QUEUE_PREV, prevTask);
        }else{
            thread_queue->tail = prevTask;
        }
        if(prevTask){
            vTaskSetThreadLocalStoragePointer(prevTask, LOCAL_STORAGE_QUEUE_NEXT, nextTask);
        }else{
            thread_queue->head = nextTask;
        }
    }

    return msg;
}

static bool osalThreadDequeueI(threads_queue_t* thread_queue, msg_t msg){
    if(!thread_queue->tail){
        return false;
    }

    thread_reference_t toWakeUp = thread_queue->tail;

    /* Lookup the last task, and pop it */
    thread_t prevTask = pvTaskGetThreadLocalStoragePointer(toWakeUp, LOCAL_STORAGE_QUEUE_PREV);
    if(prevTask){
        vTaskSetThreadLocalStoragePointer(prevTask, LOCAL_STORAGE_QUEUE_NEXT, NULL);
    }else{
        thread_queue->head = NULL;
    }
    thread_queue->tail = prevTask;
   
    /* Resume it */
    osalThreadResumeI(&toWakeUp, msg);

    return true;
}

void osalThreadDequeueAllI(threads_queue_t* thread_queue, msg_t msg)
{
    osalDbgCheck(thread_queue != NULL);
    osalDbgCheckClassI();

    while(osalThreadDequeueI(thread_queue, msg));
}

void osalThreadDequeueNextI(threads_queue_t* thread_queue, msg_t msg)
{
    osalDbgCheck(thread_queue != NULL);
    osalDbgCheckClassI();

    osalThreadDequeueI(thread_queue, msg);
}

msg_t osalThreadSuspendS(thread_reference_t* thread_reference)
{
    return osalThreadSuspendTimeoutS(thread_reference, portMAX_DELAY);
}

msg_t osalThreadSuspendTimeoutS(thread_reference_t* thread_reference, systime_t timeout)
{
    msg_t ulInterruptStatus;
    osalDbgCheckClassS();

    if(!timeout) {
        return MSG_TIMEOUT;
    }

    if(thread_reference) {
        *thread_reference = xGetCurrentTaskHandle();
    }

    if(!xTaskNotifyWait(ULONG_MAX, ULONG_MAX, (uint32_t*)&ulInterruptStatus, timeout )) {
        if(thread_reference) {
            *thread_reference = NULL;
        }

        return MSG_TIMEOUT;
    }

    return ulInterruptStatus;
}

eventflags_t osalEventWaitTimeoutS(event_source_t* event_source, systime_t timeout){
    eventflags_t result = 0;
    osalDbgCheck(event_source != NULL);
    osalDbgCheckClassS();

    osalThreadSuspendTimeoutS(&event_source->waitThread, timeout);

    result = event_source->setEvents;
    event_source->setEvents = 0;
    
    return result;
}

void osalEventBroadcastFlagsI(event_source_t* event_source, eventflags_t set)
{
    osalDbgCheck(event_source != NULL);
    osalDbgCheckClassI();

    event_source->setEvents |= set;
    eventflags_t localEvents = event_source->setEvents;

    if(event_source->eventCallback){
        event_source->eventCallback(event_source, localEvents);
    }

    /* Any repeaters? */
    event_repeater_t* repeater = event_source->firstRepeater;
    while(repeater){
        if(localEvents & repeater->triggerEvents){
            repeater->setEvents |= localEvents;
            osalEventBroadcastFlagsI(repeater->target, repeater->myEvent);
        }
        event_source->setEvents &=~ repeater->triggerEvents;
        repeater = repeater->nextRepeater;
    }
   
    /* Wake up any waiting threads that may be waiting on remaining events */
    if(event_source->setEvents){
        osalThreadResumeI(&event_source->waitThread, MSG_EVENT_W);
    }
}

void osalEventRegisterCallbackI(event_source_t* source, void (*eventCallback)(event_source_t* source, eventflags_t set)){
    osalDbgCheck(source != NULL);
    osalDbgCheckClassI();

    source->eventCallback = eventCallback;
}

void osalEventRepeaterRegister(event_repeater_t* event_repeater,
                               event_source_t* event_source,
                               eventflags_t trigger_events,
                               event_source_t* target_event_sink,
                               eventflags_t my_event){
    osalDbgCheck(event_source != NULL);
    osalDbgCheck(event_repeater != NULL);
    osalDbgCheck(target_event_sink != NULL);
    osalDbgCheckClassS();

    /* Initialize the structure */
    event_repeater->triggerEvents = trigger_events;
    event_repeater->myEvent = my_event;
    event_repeater->setEvents = 0;
    event_repeater->target = target_event_sink;
    event_repeater->source = event_source;

    /* Insert at the beginning */
    event_repeater_t* oldFirstRepeater = event_source->firstRepeater;
    event_source->firstRepeater = event_repeater;
    event_repeater->nextRepeater = oldFirstRepeater;
    event_repeater->prevRepeater = NULL;
    if(oldFirstRepeater){
        oldFirstRepeater->prevRepeater = event_repeater;
    }

    /* Broadcast an empty event, to trigger the system should one already be waiting */
    osalEventBroadcastFlagsI(event_source, 0);
}

void osalEventRepeaterUnregister(event_repeater_t* event_repeater){
    /* Are we the first? */
    if(!event_repeater->prevRepeater){
        event_repeater->source->firstRepeater = event_repeater->nextRepeater;
        event_repeater->source->firstRepeater->prevRepeater = NULL;
        return;
    }

    /* The next ones previous is my previous */
    if(event_repeater->nextRepeater){
        event_repeater->nextRepeater->prevRepeater = event_repeater->prevRepeater;
    }

    /* The previous next one is my next */
    event_repeater->prevRepeater->nextRepeater = event_repeater->nextRepeater;
}

UBaseType_t uxSavedInterruptStatus;
