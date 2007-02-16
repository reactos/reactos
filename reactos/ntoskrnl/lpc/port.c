/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/port.c
 * PURPOSE:         Local Procedure Call: Port Management
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE LpcPortObjectType;
ULONG LpcpMaxMessageSize;
PAGED_LOOKASIDE_LIST LpcpMessagesLookaside;
KGUARDED_MUTEX LpcpLock;
ULONG LpcpTraceLevel = 0;
ULONG LpcpNextMessageId = 1, LpcpNextCallbackId = 1;

static GENERIC_MAPPING LpcpPortMapping = 
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    0,
    PORT_ALL_ACCESS
};

/* PRIVATE FUNCTIONS *********************************************************/

BOOLEAN
NTAPI
LpcInitSystem(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;

    /* Setup the LPC Lock */
    KeInitializeGuardedMutex(&LpcpLock);

    /* Create the Port Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Port");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(LPCP_PORT_OBJECT);
    ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(LPCP_NONPAGED_PORT_QUEUE);
    ObjectTypeInitializer.GenericMapping = LpcpPortMapping;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.CloseProcedure = LpcpClosePort;
    ObjectTypeInitializer.DeleteProcedure = LpcpDeletePort;
    ObjectTypeInitializer.ValidAccessMask = PORT_ALL_ACCESS;
    ObCreateObjectType(&Name,
                       &ObjectTypeInitializer,
                       NULL,
                       &LpcPortObjectType);

    /* Allocate the LPC lookaside list */
    LpcpMaxMessageSize = LPCP_MAX_MESSAGE_SIZE;
    ExInitializePagedLookasideList(&LpcpMessagesLookaside,
                                   NULL,
                                   NULL,
                                   0,
                                   LpcpMaxMessageSize,
                                   TAG('L', 'p', 'c', 'M'),
                                   32);

    /* We're done */
    return TRUE;
}

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
NtImpersonateClientOfPort(IN HANDLE PortHandle,
                          IN PPORT_MESSAGE ClientMessage)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtQueryPortInformationProcess(VOID)
{
    /* This is all this function does */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
NtQueryInformationPort(IN HANDLE PortHandle,
                       IN PORT_INFORMATION_CLASS PortInformationClass,
                       OUT PVOID PortInformation,
                       IN ULONG PortInformationLength,
                       OUT PULONG ReturnLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
