/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/event.c
 * PURPOSE:         I/O Wrappers for the Executive Event Functions
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

PKEVENT
NTAPI
IopCreateEvent(IN PUNICODE_STRING EventName,
               IN PHANDLE EventHandle,
               IN EVENT_TYPE Type)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEVENT Event;
    HANDLE Handle;
    NTSTATUS Status;
    PAGED_CODE();

    /* Initialize the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               EventName,
                               OBJ_OPENIF | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Create the event */
    Status = ZwCreateEvent(&Handle,
                           EVENT_ALL_ACCESS,
                           &ObjectAttributes,
                           Type,
                           TRUE);
    if (!NT_SUCCESS(Status)) return NULL;

    /* Get a handle to it */
    ObReferenceObjectByHandle(Handle,
                              0,
                              ExEventObjectType,
                              KernelMode,
                              (PVOID*)&Event,
                              NULL);

    /* Dereference the extra count, and return the handle */
    ObDereferenceObject(Event);
    *EventHandle = Handle;
    return Event;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
PKEVENT
NTAPI
IoCreateNotificationEvent(IN PUNICODE_STRING EventName,
                          IN PHANDLE EventHandle)
{
    /* Call the internal API */
    return IopCreateEvent(EventName, EventHandle, NotificationEvent);
}

/*
 * @implemented
 */
PKEVENT
NTAPI
IoCreateSynchronizationEvent(IN PUNICODE_STRING EventName,
                             IN PHANDLE EventHandle)
{
    /* Call the internal API */
    return IopCreateEvent(EventName, EventHandle, SynchronizationEvent);
}

/* EOF */
