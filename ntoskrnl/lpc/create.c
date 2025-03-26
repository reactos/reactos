/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/create.c
 * PURPOSE:         Local Procedure Call: Port/Queue/Message Creation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
LpcpInitializePortQueue(IN PLPCP_PORT_OBJECT Port)
{
    PLPCP_NONPAGED_PORT_QUEUE MessageQueue;

    PAGED_CODE();

    /* Allocate the queue */
    MessageQueue = ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(*MessageQueue),
                                         'troP');
    if (!MessageQueue) return STATUS_INSUFFICIENT_RESOURCES;

    /* Set it up */
    KeInitializeSemaphore(&MessageQueue->Semaphore, 0, MAXLONG);
    MessageQueue->BackPointer = Port;

    /* And link it with the Paged Pool part */
    Port->MsgQueue.Semaphore = &MessageQueue->Semaphore;
    InitializeListHead(&Port->MsgQueue.ReceiveHead);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LpcpCreatePort(OUT PHANDLE PortHandle,
               IN POBJECT_ATTRIBUTES ObjectAttributes,
               IN ULONG MaxConnectionInfoLength,
               IN ULONG MaxMessageLength,
               IN ULONG MaxPoolUsage,
               IN BOOLEAN Waitable)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    UNICODE_STRING CapturedObjectName, *ObjectName;
#if DBG
    UNICODE_STRING CapturedPortName;
#endif
    PLPCP_PORT_OBJECT Port;
    HANDLE Handle;

    PAGED_CODE();

    RtlInitEmptyUnicodeString(&CapturedObjectName, NULL, 0);

    /* Check if the call comes from user mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the PortHandle */
            ProbeForWriteHandle(PortHandle);

            /* Probe the ObjectAttributes and its object name (not the buffer) */
            ProbeForRead(ObjectAttributes, sizeof(*ObjectAttributes), sizeof(ULONG));
            ObjectName = ((volatile OBJECT_ATTRIBUTES*)ObjectAttributes)->ObjectName;
            if (ObjectName)
                CapturedObjectName = ProbeForReadUnicodeString(ObjectName);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        ObjectName = ObjectAttributes->ObjectName;
        if (ObjectName)
            CapturedObjectName = *ObjectName;
    }

    /* Normalize the buffer pointer in case we don't have
     * a name, for initializing an unconnected port. */
    if (CapturedObjectName.Length == 0)
        CapturedObjectName.Buffer = NULL;

#if DBG
    /* Capture the port name for DPRINT only - ObCreateObject does its
     * own capture. As it is used only for debugging, ignore any failure;
     * the string is zeroed out in such case. */
    ProbeAndCaptureUnicodeString(&CapturedPortName, PreviousMode, ObjectName);
    LPCTRACE(LPC_CREATE_DEBUG, "Name: %wZ\n", &CapturedPortName);
    ReleaseCapturedUnicodeString(&CapturedPortName, PreviousMode);
#endif

    /* Create the Object */
    Status = ObCreateObject(PreviousMode,
                            LpcPortObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(LPCP_PORT_OBJECT),
                            0,
                            0,
                            (PVOID*)&Port);
    if (!NT_SUCCESS(Status)) return Status;

    /* Set up the Object */
    RtlZeroMemory(Port, sizeof(LPCP_PORT_OBJECT));
    Port->ConnectionPort = Port;
    Port->Creator = PsGetCurrentThread()->Cid;
    InitializeListHead(&Port->LpcDataInfoChainHead);
    InitializeListHead(&Port->LpcReplyChainHead);

    /* Check if we don't have a name */
    if (CapturedObjectName.Buffer == NULL)
    {
        /* Set up for an unconnected port */
        Port->Flags = LPCP_UNCONNECTED_PORT;
        Port->ConnectedPort = Port;
        Port->ServerProcess = NULL;
    }
    else
    {
        /* Set up for a named connection port */
        Port->Flags = LPCP_CONNECTION_PORT;
        Port->ServerProcess = PsGetCurrentProcess();

        /* Don't let the process die on us */
        ObReferenceObject(Port->ServerProcess);
    }

    /* Check if this is a waitable port */
    if (Waitable) Port->Flags |= LPCP_WAITABLE_PORT;

    /* Setup the port queue */
    Status = LpcpInitializePortQueue(Port);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ObDereferenceObject(Port);
        return Status;
    }

    /* Check if this is a waitable port */
    if (Port->Flags & LPCP_WAITABLE_PORT)
    {
        /* Setup the wait event */
        KeInitializeEvent(&Port->WaitEvent, NotificationEvent, FALSE);
    }

    /* Set the maximum message size allowed */
    Port->MaxMessageLength = LpcpMaxMessageSize -
                             FIELD_OFFSET(LPCP_MESSAGE, Request);

    /* Now subtract the actual message structures and get the data size */
    Port->MaxConnectionInfoLength = Port->MaxMessageLength -
                                    sizeof(PORT_MESSAGE) -
                                    sizeof(LPCP_CONNECTION_MESSAGE);

    /* Validate the sizes */
    if (Port->MaxConnectionInfoLength < MaxConnectionInfoLength)
    {
        /* Not enough space for your request */
        ObDereferenceObject(Port);
        return STATUS_INVALID_PARAMETER_3;
    }
    else if (Port->MaxMessageLength < MaxMessageLength)
    {
        /* Not enough space for your request */
        ObDereferenceObject(Port);
        return STATUS_INVALID_PARAMETER_4;
    }

    /* Now set the custom setting */
    Port->MaxMessageLength = MaxMessageLength;

    /* Insert it now */
    Status = ObInsertObject(Port,
                            NULL,
                            PORT_ALL_ACCESS,
                            0,
                            NULL,
                            &Handle);
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            /* Write back the handle, pointer was already probed */
            *PortHandle = Handle;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* An exception happened, close the opened handle */
            ObCloseHandle(Handle, PreviousMode);
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return success or the error */
    LPCTRACE(LPC_CREATE_DEBUG, "Port: %p. Handle: %p\n", Port, Handle);
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreatePort(OUT PHANDLE PortHandle,
             IN POBJECT_ATTRIBUTES ObjectAttributes,
             IN ULONG MaxConnectInfoLength,
             IN ULONG MaxDataLength,
             IN ULONG MaxPoolUsage)
{
    PAGED_CODE();

    /* Call the internal API */
    return LpcpCreatePort(PortHandle,
                          ObjectAttributes,
                          MaxConnectInfoLength,
                          MaxDataLength,
                          MaxPoolUsage,
                          FALSE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreateWaitablePort(OUT PHANDLE PortHandle,
                     IN POBJECT_ATTRIBUTES ObjectAttributes,
                     IN ULONG MaxConnectInfoLength,
                     IN ULONG MaxDataLength,
                     IN ULONG MaxPoolUsage)
{
    PAGED_CODE();

    /* Call the internal API */
    return LpcpCreatePort(PortHandle,
                          ObjectAttributes,
                          MaxConnectInfoLength,
                          MaxDataLength,
                          MaxPoolUsage,
                          TRUE);
}

/* EOF */
