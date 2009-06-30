/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engevent.c
 * PURPOSE:         Event Support Routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>
#define NDEBUG
#include <debug.h>

#define TAG_ENG TAG('E', 'n', 'g', ' ')

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
EngCreateEvent(OUT PEVENT* Event)
{
    PENG_EVENT EngEvent;

    /* Allocate memory for the event structure */
    EngEvent = EngAllocMem(FL_NONPAGED_MEMORY | FL_ZERO_MEMORY,
                           sizeof(ENG_EVENT),
                           TAG_ENG);

    /* Check if we are out of memory */
    if (!EngEvent)
    {
        /* We are, fail */
        return FALSE;
    }

    /* Set KEVENT pointer */
    EngEvent->pKEvent = &EngEvent->KEvent;

    /* Initialize the kernel event */
    KeInitializeEvent(EngEvent->pKEvent,
                      SynchronizationEvent,
                      FALSE);

    /* Pass pointer to our structure to the caller */
    *Event = EngEvent;

    DPRINT("EngCreateEvent() created %p\n", EngEvent);

    /* Return success */
    return TRUE;
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
    EngFreeMem(Event);

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
    DPRINT("EngMapEvent(%x %x %p %p %p)\n", hDev, hUserObject, Reserved1, Reserved2, Reserved3);
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
EngUnmapEvent(IN PEVENT Event)
{
    DPRINT("EngUnmapEvent(%p)\n", Event);
    UNIMPLEMENTED;
    return FALSE;
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
    if (!NT_SUCCESS(Status))
    {
        /* Return failure */
        return FALSE;
    }

    /* Return success */
    return TRUE;
}
