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

POBJECT_TYPE LpcPortObjectType, LpcWaitablePortObjectType;
ULONG LpcpMaxMessageSize;
PAGED_LOOKASIDE_LIST LpcpMessagesLookaside;
KGUARDED_MUTEX LpcpLock;
ULONG LpcpTraceLevel = 0;
ULONG LpcpNextMessageId = 1, LpcpNextCallbackId = 1;

static GENERIC_MAPPING LpcpPortMapping =
{
    READ_CONTROL | PORT_CONNECT,
    DELETE | PORT_CONNECT,
    0,
    PORT_ALL_ACCESS
};

/* PRIVATE FUNCTIONS *********************************************************/

BOOLEAN
NTAPI
INIT_FUNCTION
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
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(LPCP_NONPAGED_PORT_QUEUE);
    ObjectTypeInitializer.DefaultPagedPoolCharge = FIELD_OFFSET(LPCP_PORT_OBJECT, WaitEvent);
    ObjectTypeInitializer.GenericMapping = LpcpPortMapping;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.CloseProcedure = LpcpClosePort;
    ObjectTypeInitializer.DeleteProcedure = LpcpDeletePort;
    ObjectTypeInitializer.ValidAccessMask = PORT_ALL_ACCESS;
    ObjectTypeInitializer.InvalidAttributes = OBJ_VALID_ATTRIBUTES & ~OBJ_CASE_INSENSITIVE;
    ObCreateObjectType(&Name,
                       &ObjectTypeInitializer,
                       NULL,
                       &LpcPortObjectType);

    RtlInitUnicodeString(&Name, L"WaitablePort");
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge += sizeof(LPCP_PORT_OBJECT);
    ObjectTypeInitializer.DefaultPagedPoolCharge = 0;
    ObjectTypeInitializer.UseDefaultObject = FALSE;
    ObCreateObjectType(&Name,
                       &ObjectTypeInitializer,
                       NULL,
                       &LpcWaitablePortObjectType);

    /* Allocate the LPC lookaside list */
    LpcpMaxMessageSize = LPCP_MAX_MESSAGE_SIZE;
    ExInitializePagedLookasideList(&LpcpMessagesLookaside,
                                   NULL,
                                   NULL,
                                   0,
                                   LpcpMaxMessageSize,
                                   'McpL',
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
