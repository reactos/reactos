/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/cmname.c
 * PURPOSE:         Client-side of the Ob Callback Interface for Key Objects.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE CmpKeyObjectType;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpCloseKeyObject(IN PEPROCESS Process OPTIONAL,
                  IN PVOID Object,
                  IN ACCESS_MASK GrantedAccess,
                  IN ULONG ProcessHandleCount,
                  IN ULONG SystemHandleCount)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
}

VOID
NTAPI
CmpDeleteKeyObject(IN PVOID Object)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
}

NTSTATUS
NTAPI
CmpParseKey(IN PVOID ParseObject,
            IN PVOID ObjectType,
            IN OUT PACCESS_STATE AccessState,
            IN KPROCESSOR_MODE AccessMode,
            IN ULONG Attributes,
            IN OUT PUNICODE_STRING CompleteName,
            IN OUT PUNICODE_STRING RemainingName,
            IN OUT PVOID Context OPTIONAL,
            IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
            OUT PVOID *Object)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpQueryKeyName(IN PVOID Object,
                IN BOOLEAN HasObjectName,
                OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
                IN ULONG Length,
                OUT PULONG ReturnLength,
                IN KPROCESSOR_MODE AccessMode)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}


