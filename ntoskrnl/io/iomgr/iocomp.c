/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iocomp.c
 * PURPOSE:         I/O Wrappers (called Completion Ports) for Kernel Queues
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

POBJECT_TYPE IoCompletionType;

NPAGED_LOOKASIDE_LIST IoCompletionPacketLookaside;

GENERIC_MAPPING IopCompletionMapping =
{
    STANDARD_RIGHTS_READ    | IO_COMPLETION_QUERY_STATE,
    STANDARD_RIGHTS_WRITE   | IO_COMPLETION_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | IO_COMPLETION_QUERY_STATE,
    IO_COMPLETION_ALL_ACCESS
};

static const INFORMATION_CLASS_INFO IoCompletionInfoClass[] =
{
     /* IoCompletionBasicInformation */
    ICI_SQ_SAME(sizeof(IO_COMPLETION_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY),
};

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
IopUnloadSafeCompletion(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp,
                        IN PVOID Context)
{
    NTSTATUS Status;
    PIO_UNLOAD_SAFE_COMPLETION_CONTEXT UnsafeContext =
        (PIO_UNLOAD_SAFE_COMPLETION_CONTEXT)Context;

    /* Reference the device object */
    ObReferenceObject(UnsafeContext->DeviceObject);

    /* Call the completion routine */
    Status= UnsafeContext->CompletionRoutine(DeviceObject,
                                             Irp,
                                             UnsafeContext->Context);

    /* Dereference the device object */
    ObDereferenceObject(UnsafeContext->DeviceObject);

    /* Free our context */
    ExFreePool(UnsafeContext);
    return Status;
}

VOID
NTAPI
IopFreeIoCompletionPacket(PIO_COMPLETION_PACKET Packet)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PNPAGED_LOOKASIDE_LIST List;

    /* Use the P List */
    List = (PNPAGED_LOOKASIDE_LIST)Prcb->
            PPLookasideList[LookasideCompletionList].P;
    List->L.TotalFrees++;

    /* Check if the Free was within the Depth or not */
    if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
    {
        /* Let the balancer know */
        List->L.FreeMisses++;

        /* Use the L List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->
                PPLookasideList[LookasideCompletionList].L;
        List->L.TotalFrees++;

        /* Check if the Free was within the Depth or not */
        if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
        {
            /* All lists failed, use the pool */
            List->L.FreeMisses++;
            ExFreePool(Packet);
        }
    }

    /* The free was within dhe Depth */
    InterlockedPushEntrySList(&List->L.ListHead, (PSINGLE_LIST_ENTRY)Packet);
}

VOID
NTAPI
IopDeleteIoCompletion(PVOID ObjectBody)
{
    PKQUEUE Queue = ObjectBody;
    PLIST_ENTRY FirstEntry;
    PLIST_ENTRY CurrentEntry;
    PIRP Irp;
    PIO_COMPLETION_PACKET Packet;

    /* Rundown the Queue */
    FirstEntry = KeRundownQueue(Queue);
    if (FirstEntry)
    {
        /* Loop the packets */
        CurrentEntry = FirstEntry;
        do
        {
            /* Get the Packet */
            Packet = CONTAINING_RECORD(CurrentEntry,
                                       IO_COMPLETION_PACKET,
                                       ListEntry);

            /* Go to next Entry */
            CurrentEntry = CurrentEntry->Flink;

            /* Check if it's part of an IRP, or a separate packet */
            if (Packet->PacketType == IrpCompletionPacket)
            {
                /* Get the IRP and free it */
                Irp = CONTAINING_RECORD(Packet, IRP, Tail.Overlay.ListEntry);
                IoFreeIrp(Irp);
            }
            else
            {
                /* Use common routine */
                IopFreeIoCompletionPacket(Packet);
            }
        } while (FirstEntry != CurrentEntry);
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoSetIoCompletion(IN PVOID IoCompletion,
                  IN PVOID KeyContext,
                  IN PVOID ApcContext,
                  IN NTSTATUS IoStatus,
                  IN ULONG_PTR IoStatusInformation,
                  IN BOOLEAN Quota)
{
    PKQUEUE Queue = (PKQUEUE)IoCompletion;
    PNPAGED_LOOKASIDE_LIST List;
    PKPRCB Prcb = KeGetCurrentPrcb();
    PIO_COMPLETION_PACKET Packet;

    /* Get the P List */
    List = (PNPAGED_LOOKASIDE_LIST)Prcb->
            PPLookasideList[LookasideCompletionList].P;

    /* Try to allocate the Packet */
    List->L.TotalAllocates++;
    Packet = (PVOID)InterlockedPopEntrySList(&List->L.ListHead);

    /* Check if that failed, use the L list if it did */
    if (!Packet)
    {
        /* Let the balancer know */
        List->L.AllocateMisses++;

        /* Get L List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->
                PPLookasideList[LookasideCompletionList].L;

        /* Try to allocate the Packet */
        List->L.TotalAllocates++;
        Packet = (PVOID)InterlockedPopEntrySList(&List->L.ListHead);
    }

    /* Still failed, use pool */
    if (!Packet)
    {
        /* Let the balancer know */
        List->L.AllocateMisses++;

        /* Allocate from Nonpaged Pool */
        Packet = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Packet), IOC_TAG);
    }

    /* Make sure we have one by now... */
    if (Packet)
    {
        /* Set up the Packet */
        Packet->PacketType = IrpMiniCompletionPacket;
        Packet->Key = KeyContext;
        Packet->Context = ApcContext;
        Packet->IoStatus.Status = IoStatus;
        Packet->IoStatus.Information = IoStatusInformation;

        /* Insert the Queue */
        KeInsertQueue(Queue, &Packet->ListEntry);
    }
    else
    {
        /* Out of memory, fail */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return Success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoSetCompletionRoutineEx(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp,
                         IN PIO_COMPLETION_ROUTINE CompletionRoutine,
                         IN PVOID Context,
                         IN BOOLEAN InvokeOnSuccess,
                         IN BOOLEAN InvokeOnError,
                         IN BOOLEAN InvokeOnCancel)
{
    PIO_UNLOAD_SAFE_COMPLETION_CONTEXT UnloadContext;

    /* Allocate the context */
    UnloadContext = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(*UnloadContext),
                                          TAG('I', 'o', 'U', 's'));
    if (!UnloadContext) return STATUS_INSUFFICIENT_RESOURCES;

    /* Set up the context */
    UnloadContext->DeviceObject = DeviceObject;
    UnloadContext->Context = Context;
    UnloadContext->CompletionRoutine = CompletionRoutine;

    /* Now set the completion routine */
    IoSetCompletionRoutine(Irp,
                           IopUnloadSafeCompletion,
                           UnloadContext,
                           InvokeOnSuccess,
                           InvokeOnError,
                           InvokeOnCancel);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtCreateIoCompletion(OUT PHANDLE IoCompletionHandle,
                     IN ACCESS_MASK DesiredAccess,
                     IN POBJECT_ATTRIBUTES ObjectAttributes,
                     IN ULONG NumberOfConcurrentThreads)
{
    PKQUEUE Queue;
    HANDLE hIoCompletionHandle;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if this was a user-mode call */
    if (PreviousMode != KernelMode)
    {
        /* Wrap probing in SEH */
        _SEH_TRY
        {
            /* Probe the handle */
            ProbeForWriteHandle(IoCompletionHandle);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Create the Object */
    Status = ObCreateObject(PreviousMode,
                            IoCompletionType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(KQUEUE),
                            0,
                            0,
                            (PVOID*)&Queue);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the Queue */
        KeInitializeQueue(Queue, NumberOfConcurrentThreads);

        /* Insert it */
        Status = ObInsertObject(Queue,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hIoCompletionHandle);
        if (NT_SUCCESS(Status))
        {
            /* Protect writing the handle in SEH */
            _SEH_TRY
            {
                /* Write the handle back */
                *IoCompletionHandle = hIoCompletionHandle;
            }
            _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
   }

   /* Return Status */
   return Status;
}

NTSTATUS
NTAPI
NtOpenIoCompletion(OUT PHANDLE IoCompletionHandle,
                   IN ACCESS_MASK DesiredAccess,
                   IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    HANDLE hIoCompletionHandle;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if this was a user-mode call */
    if (PreviousMode != KernelMode)
    {
        /* Wrap probing in SEH */
        _SEH_TRY
        {
            /* Probe the handle */
            ProbeForWriteHandle(IoCompletionHandle);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                IoCompletionType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &hIoCompletionHandle);
    if (NT_SUCCESS(Status))
    {
        /* Protect writing the handle in SEH */
        _SEH_TRY
        {
            /* Write the handle back */
            *IoCompletionHandle = hIoCompletionHandle;
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
NtQueryIoCompletion(IN  HANDLE IoCompletionHandle,
                    IN  IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
                    OUT PVOID IoCompletionInformation,
                    IN  ULONG IoCompletionInformationLength,
                    OUT PULONG ResultLength OPTIONAL)
{
    PKQUEUE Queue;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check buffers and parameters */
    Status = DefaultQueryInfoBufferCheck(IoCompletionInformationClass,
                                         IoCompletionInfoClass,
                                         sizeof(IoCompletionInfoClass) /
                                         sizeof(IoCompletionInfoClass[0]),
                                         IoCompletionInformation,
                                         IoCompletionInformationLength,
                                         ResultLength,
                                         PreviousMode);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the Object */
    Status = ObReferenceObjectByHandle(IoCompletionHandle,
                                       IO_COMPLETION_QUERY_STATE,
                                       IoCompletionType,
                                       PreviousMode,
                                       (PVOID*)&Queue,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Protect write in SEH */
        _SEH_TRY
        {
            /* Return Info */
            ((PIO_COMPLETION_BASIC_INFORMATION)IoCompletionInformation)->
                Depth = KeReadStateQueue(Queue);

            /* Dereference the queue */
            ObDereferenceObject(Queue);

            /* Return Result Length if needed */
            if (ResultLength)
            {
                *ResultLength = sizeof(IO_COMPLETION_BASIC_INFORMATION);
            }
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            /* Get exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
NtRemoveIoCompletion(IN HANDLE IoCompletionHandle,
                     OUT PVOID *KeyContext,
                     OUT PVOID *ApcContext,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     IN PLARGE_INTEGER Timeout OPTIONAL)
{
    LARGE_INTEGER SafeTimeout;
    PKQUEUE Queue;
    PIO_COMPLETION_PACKET Packet;
    PLIST_ENTRY ListEntry;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP Irp;
    PVOID Apc, Key;
    IO_STATUS_BLOCK IoStatus;
    PAGED_CODE();

    /* Check if the call was from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Protect probes in SEH */
        _SEH_TRY
        {
            /* Probe the pointers */
            ProbeForWritePointer(KeyContext);
            ProbeForWritePointer(ApcContext);

            /* Probe the I/O Status Block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);
            if (Timeout)
            {
                /* Probe and capture the timeout */
                SafeTimeout = ProbeForReadLargeInteger(Timeout);
                Timeout = &SafeTimeout;
            }
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(IoCompletionHandle,
                                       IO_COMPLETION_MODIFY_STATE,
                                       IoCompletionType,
                                       PreviousMode,
                                       (PVOID*)&Queue,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Remove queue */
        ListEntry = KeRemoveQueue(Queue, PreviousMode, Timeout);

        /* If we got a timeout or user_apc back, return the status */
        if (((NTSTATUS)ListEntry == STATUS_TIMEOUT) ||
            ((NTSTATUS)ListEntry == STATUS_USER_APC))
        {
            /* Set this as the status */
            Status = (NTSTATUS)ListEntry;
        }
        else
        {
            /* Get the Packet Data */
            Packet = CONTAINING_RECORD(ListEntry,
                                       IO_COMPLETION_PACKET,
                                       ListEntry);

            /* Check if this is piggybacked on an IRP */
            if (Packet->PacketType == IrpCompletionPacket)
            {
                /* Get the IRP */
                Irp = CONTAINING_RECORD(ListEntry,
                                        IRP,
                                        Tail.Overlay.ListEntry);

                /* Save values */
                Key = Irp->Tail.CompletionKey;
                Apc = Irp->Overlay.AsynchronousParameters.UserApcContext;
                IoStatus = Irp->IoStatus;

                /* Free the IRP */
                IoFreeIrp(Irp);
            }
            else
            {
                /* Save values */
                Key = Packet->Key;
                Apc = Packet->Context;
                IoStatus = Packet->IoStatus;

                /* Free the packet */
                IopFreeIoCompletionPacket(Packet);
            }

            /* Enter SEH to write back the values */
            _SEH_TRY
            {
                /* Write the values to caller */
                *ApcContext = Apc;
                *KeyContext = Key;
                *IoStatusBlock = IoStatus;
            }
            _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }

        /* Dereference the Object */
        ObDereferenceObject(Queue);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtSetIoCompletion(IN HANDLE IoCompletionPortHandle,
                  IN PVOID CompletionKey,
                  IN PVOID CompletionContext,
                  IN NTSTATUS CompletionStatus,
                  IN ULONG CompletionInformation)
{
    NTSTATUS Status;
    PKQUEUE Queue;
    PAGED_CODE();

    /* Get the Object */
    Status = ObReferenceObjectByHandle(IoCompletionPortHandle,
                                       IO_COMPLETION_MODIFY_STATE,
                                       IoCompletionType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&Queue,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Set the Completion */
        Status = IoSetIoCompletion(Queue,
                                   CompletionKey,
                                   CompletionContext,
                                   CompletionStatus,
                                   CompletionInformation,
                                   TRUE);

        /* Dereference the object */
        ObDereferenceObject(Queue);
    }

    /* Return status */
    return Status;
}
