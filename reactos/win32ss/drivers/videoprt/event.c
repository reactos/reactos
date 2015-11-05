/*
 * PROJECT:         ReactOS Video Port Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            win32ss/drivers/videoprt/event.c
 * PURPOSE:         Event Support Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "videoprt.h"
#include "../../gdi/eng/engevent.h"

#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortCreateEvent(IN PVOID HwDeviceExtension,
                     IN ULONG EventFlag,
                     IN PVOID Unused,
                     OUT PEVENT *Event)
{
    VP_STATUS Result = NO_ERROR;
    PVIDEO_PORT_EVENT EngEvent;

    /* Allocate memory for the event structure */
    EngEvent = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(VIDEO_PORT_EVENT) + sizeof(KEVENT),
                                     TAG_VIDEO_PORT);
    if (EngEvent)
    {
        /* Set KEVENT pointer */
        EngEvent->pKEvent = EngEvent + 1;
        
        /* Initialize the kernel event */
        KeInitializeEvent(EngEvent->pKEvent,
                          (EventFlag & EVENT_TYPE_MASK) ?
                          NotificationEvent : SynchronizationEvent,
                          EventFlag & INITIAL_EVENT_STATE_MASK);

        /* Pass pointer to our structure to the caller */
        *Event = (PEVENT)EngEvent;
        DPRINT("VideoPortCreateEvent() created %p\n", EngEvent);
    }
    else
    {
        /* Out of memory */
        DPRINT("VideoPortCreateEvent() failed\n");    
        Result = ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Return result */
    return Result;
}

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortDeleteEvent(IN PVOID HwDeviceExtension,
                     IN PEVENT Event)
{
    /* Handle error cases */
    if (!Event) return ERROR_INVALID_PARAMETER;
    if (Event->fFlags & ENG_EVENT_USERMAPPED) return ERROR_INVALID_PARAMETER;
    if (!Event->pKEvent) return ERROR_INVALID_PARAMETER;

    /* Free storage */
    ExFreePool(Event);

    /* Indicate success */
    return NO_ERROR;
}

/*
 * @implemented
 */
LONG
NTAPI
VideoPortSetEvent(IN PVOID HwDeviceExtension,
                  IN PEVENT Event)
{
    return KeSetEvent(Event->pKEvent, IO_NO_INCREMENT, FALSE);
}

/*
 * @implemented
 */
VOID
NTAPI
VideoPortClearEvent(IN PVOID HwDeviceExtension,
                    IN PEVENT Event)
{
    KeClearEvent(Event->pKEvent);
}

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortWaitForSingleObject(IN PVOID HwDeviceExtension,
                             IN PVOID Event,
                             IN PLARGE_INTEGER Timeout OPTIONAL)
{
    PVIDEO_PORT_EVENT EngEvent = Event;
    NTSTATUS Status;
    
    /* Handle error cases */
    if (!EngEvent) return ERROR_INVALID_PARAMETER;
    if (!EngEvent->pKEvent) return ERROR_INVALID_PARAMETER;
    if (EngEvent->fFlags & ENG_EVENT_USERMAPPED) return ERROR_INVALID_PARAMETER;
    
    /* Do the actual wait */
    Status = KeWaitForSingleObject(EngEvent->pKEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   Timeout);
    if (Status == STATUS_TIMEOUT)
    {
        /* Convert to wait timeout, otherwise NT_SUCCESS would return NO_ERROR */
        return WAIT_TIMEOUT;
    }
    else if (NT_SUCCESS(Status))
    {
        /* All other success codes are Win32 success */
        return NO_ERROR;
    }
    
    /* Otherwise, return default Win32 failure */
    return ERROR_INVALID_PARAMETER;
}

/* EOF */
