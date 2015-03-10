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

#include <ntddvdeo.h>

#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

_Must_inspect_result_
_Success_(return != FALSE)
BOOL
APIENTRY
EngCreateEvent(
    _Outptr_ PEVENT *ppEvent)
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
        *ppEvent = EngEvent;
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
EngDeleteEvent(
    _In_ _Post_ptr_invalid_ PEVENT Event)
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
    ExFreePoolWithTag(Event, GDITAG_ENG_EVENT);

    /* Return success */
    return TRUE;
}

VOID
APIENTRY
EngClearEvent(
    _In_ PEVENT Event)
{
    /* Clear the event */
    KeClearEvent(Event->pKEvent);
}

LONG
APIENTRY
EngSetEvent(
    _In_ PEVENT Event)
{
    /* Set the event */
    return KeSetEvent(Event->pKEvent,
                      IO_NO_INCREMENT,
                      FALSE);
}

LONG
APIENTRY
EngReadStateEvent(
    _In_ PEVENT Event)
{
    /* Read the event state */
    return KeReadStateEvent(Event->pKEvent);
}

PEVENT
APIENTRY
EngMapEvent(
    _In_ HDEV hDev,
    _In_ HANDLE hUserObject,
    _Reserved_ PVOID Reserved1,
    _Reserved_ PVOID Reserved2,
    _Reserved_ PVOID Reserved3)
{
    PENG_EVENT EngEvent;
    PVOID pvEvent;
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
    Status = ObReferenceObjectByHandle(hUserObject,
                                       EVENT_ALL_ACCESS,
                                       *ExEventObjectType,
                                       UserMode,
                                       &pvEvent,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Pulse the event and set that it's mapped by user */
        KePulseEvent(pvEvent, EVENT_INCREMENT, FALSE);
        EngEvent->pKEvent = pvEvent;
        EngEvent->fFlags |= ENG_EVENT_USERMAPPED;
    }
    else
    {
        /* Free the allocation */
        ExFreePoolWithTag(EngEvent, GDITAG_ENG_EVENT);
        EngEvent = NULL;
    }

    /* Support legacy interface */
    if (Reserved1) *(PVOID*)Reserved1 = EngEvent;
    return EngEvent;
}

BOOL
APIENTRY
EngUnmapEvent(
    _In_ PEVENT Event)
{
    /* Must be a usermapped event */
    if (!(Event->fFlags & ENG_EVENT_USERMAPPED)) return FALSE;

    /* Dereference the object, destroying it */
    ObDereferenceObject(Event->pKEvent);

    /* Free the Eng object */
    ExFreePoolWithTag(Event, GDITAG_ENG_EVENT);
    return TRUE;
}

BOOL
APIENTRY
EngWaitForSingleObject(
    _In_ PEVENT Event,
    _In_opt_ PLARGE_INTEGER TimeOut)
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
