/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/keyedevt.c
 * PURPOSE:         Support for keyed events
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
NtCreateKeyedEvent(OUT PHANDLE KeyedEventHandle,
                   IN ACCESS_MASK DesiredAccess,
                   IN POBJECT_ATTRIBUTES ObjectAttributes,
                   IN ULONG Flags)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtOpenKeyedEvent(OUT PHANDLE EventHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtReleaseKeyedEvent(IN HANDLE EventHandle,
                    IN PVOID Key,
                    IN BOOLEAN Alertable,
                    IN PLARGE_INTEGER Timeout OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtWaitForKeyedEvent(IN HANDLE EventHandle,
                    IN PVOID Key,
                    IN BOOLEAN Alertable,
                    IN PLARGE_INTEGER Timeout OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
