/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engevent.c
 * PURPOSE:         Event Support Routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 *                  ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

BOOL
APIENTRY
EngCreateEvent(OUT PEVENT* Event)
{
    BOOLEAN Result = TRUE;
    PENG_EVENT EngEvent;

    /* Allocate memory for the event structure */
    EngEvent = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(ENG_EVENT) + sizeof(KEVENT),
                                     GDITAG_ENG_EVENT);
    if (EngEvent)
    {
        /* Set KEVENT pointer */
        EngEvent->fFlags = 0;
        EngEvent->pKEvent = EngEvent + 1;

        /* Initialize the kernel event */
        KeInitializeEvent(EngEvent->pKEvent,
                          SynchronizationEvent,
                          FALSE);

        /* Pass pointer to our structure to the caller */
        *Event = EngEvent;
        DPRINT("EngCreateEvent() created %p\n", EngEvent);
    }
    else
    {
        /* Out of memory */
        DPRINT("EngCreateEvent() failed\n");
        Result = FALSE;
    }

    /* Return result */
    return Result;
}

BOOL
APIENTRY
EngDeleteEvent(IN PEVENT Event)
{
    DPRINT("EngDeleteEvent(%p)\n", Event);

    /* Check if it's a usermapped event */
    if (Event->fFlags & ENG_EVENT_USERMAPPED)
    {
        /* Disallow deletion of usermapped events */
        DPRINT1("Driver attempted to delete a usermapped event!\n");
        return FALSE;
    }

    /* Free the allocated memory */
    ExFreePool(Event);

    /* Return success */
    return TRUE;
}

VOID
APIENTRY
EngClearEvent(IN PEVENT Event)
{
    /* Clear the event */
    KeClearEvent(Event->pKEvent);
}

LONG
APIENTRY
EngSetEvent(IN PEVENT Event)
{
    /* Set the event */
    return KeSetEvent(Event->pKEvent,
                      IO_NO_INCREMENT,
                      FALSE);
}

LONG
APIENTRY
EngReadStateEvent(IN PEVENT Event)
{
    /* Read the event state */
    return KeReadStateEvent(Event->pKEvent);
}

PEVENT
APIENTRY
EngMapEvent(IN HDEV hDev,
            IN HANDLE hUserObject,
            IN PVOID Reserved1,
            IN PVOID Reserved2,
            IN PVOID Reserved3)
{
    PENG_EVENT EngEvent;
    NTSTATUS Status;

    /* Allocate memory for the event structure */
    EngEvent = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(ENG_EVENT),
                                     GDITAG_ENG_EVENT);
    if (!EngEvent) return NULL;

    /* Zero it out */
    EngEvent->fFlags = 0;
    EngEvent->pKEvent = NULL;

    /* Create a handle, and have Ob fill out the pKEvent field */
    Status = ObReferenceObjectByHandle(EngEvent,
                                       EVENT_ALL_ACCESS,
                                       ExEventObjectType,
                                       UserMode,
                                       &EngEvent->pKEvent,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Pulse the event and set that it's mapped by user */
        KePulseEvent(EngEvent->pKEvent, EVENT_INCREMENT, FALSE);
        EngEvent->fFlags |= ENG_EVENT_USERMAPPED;
    }
    else
    {
        /* Free the allocation */
        ExFreePool(EngEvent);
        EngEvent = NULL;
    }

    /* Support legacy interface */
    if (Reserved1) *(PVOID*)Reserved1 = EngEvent;
    return EngEvent;
}

BOOL
APIENTRY
EngUnmapEvent(IN PEVENT Event)
{
    /* Must be a usermapped event */
    if (!(Event->fFlags & ENG_EVENT_USERMAPPED)) return FALSE;

    /* Dereference the object, destroying it */
    ObDereferenceObject(Event->pKEvent);

    /* Free the Eng object */
    ExFreePool(Event);
    return TRUE;
}

BOOL
APIENTRY
EngWaitForSingleObject(IN PEVENT Event,
                       IN PLARGE_INTEGER TimeOut)
{
    NTSTATUS Status;
    DPRINT("EngWaitForSingleObject(%p %I64d)\n", Event, TimeOut->QuadPart);

    /* Validate parameters */
    if (!Event) return FALSE;

    /* Check if it's a usermapped event and fail in that case */
    if (Event->fFlags & ENG_EVENT_USERMAPPED)
    {
        /* Disallow deletion of usermapped events */
        DPRINT1("Driver attempted to wait on a usermapped event!\n");
        return FALSE;
    }

    /* Wait for the event */
    Status = KeWaitForSingleObject(Event->pKEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   TimeOut);

    /* Check if there is a failure or a timeout */
    return NT_SUCCESS(Status);
}

/* EOF */
