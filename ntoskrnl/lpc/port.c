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

CODE_SEG("INIT")
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

    /* Create the Waitable Port Object Type */
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

BOOLEAN
NTAPI
LpcpValidateClientPort(
    PETHREAD ClientThread,
    PLPCP_PORT_OBJECT Port)
{
    PLPCP_PORT_OBJECT ThreadPort;

    /* Get the thread's port */
    ThreadPort = LpcpGetPortFromThread(ClientThread);
    if (ThreadPort == NULL)
    {
        return FALSE;
    }

    /* Check if the port matches directly */
    if ((Port == ThreadPort) ||
        (Port == ThreadPort->ConnectionPort) ||
        (Port == ThreadPort->ConnectedPort))
    {
        return TRUE;
    }

    /* Check if this is a communication port and the connection port matches */
    if (((Port->Flags & LPCP_PORT_TYPE_MASK) == LPCP_COMMUNICATION_PORT) &&
        (Port->ConnectionPort == ThreadPort))
    {
        return TRUE;
    }

    return FALSE;
}


/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
NtImpersonateClientOfPort(IN HANDLE PortHandle,
                          IN PPORT_MESSAGE ClientMessage)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    CLIENT_ID ClientId;
    ULONG MessageId;
    PLPCP_PORT_OBJECT Port = NULL, ConnectedPort = NULL;
    PETHREAD ClientThread = NULL;
    SECURITY_CLIENT_CONTEXT ClientContext;

    PAGED_CODE();

    /* Check if the call comes from user mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForRead(ClientMessage, sizeof(*ClientMessage), sizeof(PVOID));
            ClientId  = ((volatile PORT_MESSAGE*)ClientMessage)->ClientId;
            MessageId = ((volatile PORT_MESSAGE*)ClientMessage)->MessageId;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        ClientId  = ClientMessage->ClientId;
        MessageId = ClientMessage->MessageId;
    }

    /* Reference the port handle */
    Status = ObReferenceObjectByHandle(PortHandle,
                                       PORT_ALL_ACCESS,
                                       LpcPortObjectType,
                                       PreviousMode,
                                       (PVOID*)&Port,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference port handle: 0x%ls\n", Status);
        return Status;
    }

    /* Make sure this is a connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) != LPCP_COMMUNICATION_PORT)
    {
        /* It isn't, fail */
        DPRINT1("Port is not a communication port\n");
        Status = STATUS_INVALID_PORT_HANDLE;
        goto Cleanup;
    }

    /* Look up the client thread */
    Status = PsLookupProcessThreadByCid(&ClientId, NULL, &ClientThread);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to lookup client thread: 0x%ls\n", Status);
        goto Cleanup;
    }

    /* Acquire the lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Get the connected port and try to reference it */
    ConnectedPort = Port->ConnectedPort;
    if ((ConnectedPort == NULL) || !ObReferenceObjectSafe(ConnectedPort))
    {
        DPRINT1("Failed to reference the connected port\n");
        ConnectedPort = NULL;
        Status = STATUS_PORT_DISCONNECTED;
        goto CleanupWithLock;
    }

    /* Check for no-impersonation flag */
    if ((ULONG_PTR)ClientThread->LpcReplyMessage & LPCP_THREAD_FLAG_NO_IMPERSONATION)
    {
        DPRINT1("Reply message has no impersonation flag set\n");
        Status = STATUS_ACCESS_DENIED;
        goto CleanupWithLock;
    }

    /* Check for message id mismatch */
    if ((ClientThread->LpcReplyMessageId != MessageId) || (MessageId == 0))
    {
        DPRINT1("LpcReplyMessageId mismatch: 0x%lx/0x%lx.\n",
                ClientThread->LpcReplyMessageId, MessageId);
        Status = STATUS_REPLY_MESSAGE_MISMATCH;
        goto CleanupWithLock;
    }

    /* Validate the port */
    if (!LpcpValidateClientPort(ClientThread, Port))
    {
        DPRINT1("LpcpValidateClientPort failed\n");
        Status = STATUS_REPLY_MESSAGE_MISMATCH;
        goto CleanupWithLock;
    }

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);

    /* Check if security is static */
    if (!(ConnectedPort->Flags & LPCP_SECURITY_DYNAMIC))
    {
        /* Use the static security for impersonation */
        Status = SeImpersonateClientEx(&ConnectedPort->StaticSecurity, NULL);
        goto Cleanup;
    }

    /* Create new dynamic security */
    Status = SeCreateClientSecurity(ClientThread,
                                    &ConnectedPort->SecurityQos,
                                    FALSE,
                                    &ClientContext);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SeCreateClientSecurity failed\n");
        goto Cleanup;
    }

    /* Use dynamic security for impersonation */
    Status = SeImpersonateClientEx(&ClientContext, NULL);

    /* Get rid of the security context */
    SeDeleteClientSecurity(&ClientContext);

Cleanup:

    if (ConnectedPort != NULL)
        ObDereferenceObject(ConnectedPort);

    if (ClientThread != NULL)
        ObDereferenceObject(ClientThread);

    ObDereferenceObject(Port);

    return Status;

CleanupWithLock:

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);
    goto Cleanup;
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
