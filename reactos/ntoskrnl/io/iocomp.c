/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/iocomp.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

POBJECT_TYPE IoCompletionType;

NPAGED_LOOKASIDE_LIST IoCompletionPacketLookaside;

static GENERIC_MAPPING IopCompletionMapping =
{
    STANDARD_RIGHTS_READ    | IO_COMPLETION_QUERY_STATE,
    STANDARD_RIGHTS_WRITE   | IO_COMPLETION_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | IO_COMPLETION_QUERY_STATE,
    IO_COMPLETION_ALL_ACCESS
};

static const INFORMATION_CLASS_INFO IoCompletionInfoClass[] = {

     /* IoCompletionBasicInformation */
    ICI_SQ_SAME( sizeof(IO_COMPLETION_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ),
};

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
IopFreeIoCompletionPacket(PIO_COMPLETION_PACKET Packet)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PNPAGED_LOOKASIDE_LIST List;
    
    /* Use the P List */
    List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[LookasideCompletionList].P;
    List->L.TotalFrees++;
        
    /* Check if the Free was within the Depth or not */
    if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
    {
        /* Let the balancer know */
        List->L.FreeMisses++;
            
        /* Use the L List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[LookasideCompletionList].L;
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
STDCALL
IopDeleteIoCompletion(PVOID ObjectBody)
{
    PKQUEUE Queue = ObjectBody;
    PLIST_ENTRY FirstEntry;
    PLIST_ENTRY CurrentEntry;
    PIRP Irp;
    PIO_COMPLETION_PACKET Packet;

    DPRINT("IopDeleteIoCompletion()\n");

    /* Rundown the Queue */
    FirstEntry = KeRundownQueue(Queue);

    /* Clean up the IRPs */
    if (FirstEntry) {

        CurrentEntry = FirstEntry;
        do {

            /* Get the Packet */
            Packet = CONTAINING_RECORD(CurrentEntry, IO_COMPLETION_PACKET, ListEntry);

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

/*
 * @implemented
 */
NTSTATUS
STDCALL
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
    List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[LookasideCompletionList].P;
    
    /* Try to allocate the Packet */
    List->L.TotalAllocates++;
    Packet = (PVOID)InterlockedPopEntrySList(&List->L.ListHead);
    
    /* Check if that failed, use the L list if it did */
    if (!Packet)
    {
        /* Let the balancer know */
        List->L.AllocateMisses++;
        
        /* Get L List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[LookasideCompletionList].L;
    
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
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return Success */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoSetCompletionRoutineEx(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp,
                         IN PIO_COMPLETION_ROUTINE CompletionRoutine,
                         IN PVOID Context,
                         IN BOOLEAN InvokeOnSuccess,
                         IN BOOLEAN InvokeOnError,
                         IN BOOLEAN InvokeOnCancel)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
FASTCALL
IopInitIoCompletionImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;

    DPRINT1("Creating IoCompletion Object Type\n");
  
    /* Initialize the Driver object type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"IoCompletion");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KQUEUE);
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = IO_COMPLETION_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.GenericMapping = IopCompletionMapping;
    ObjectTypeInitializer.DeleteProcedure = IopDeleteIoCompletion;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &IoCompletionType);
}

NTSTATUS
STDCALL
NtCreateIoCompletion(OUT PHANDLE IoCompletionHandle,
                     IN  ACCESS_MASK DesiredAccess,
                     IN  POBJECT_ATTRIBUTES ObjectAttributes,
                     IN  ULONG NumberOfConcurrentThreads)
{
    PKQUEUE Queue;
    HANDLE hIoCompletionHandle;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWrite(IoCompletionHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();
        } _SEH_END;

        if (!NT_SUCCESS(Status)) {

            return Status;
        }
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

    /* Check for success */
    if (NT_SUCCESS(Status)) {

        /* Initialize the Queue */
        KeInitializeQueue(Queue, NumberOfConcurrentThreads);

        /* Insert it */
        Status = ObInsertObject(Queue,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hIoCompletionHandle);
        ObDereferenceObject(Queue);

        if (NT_SUCCESS(Status)) {

            _SEH_TRY {

                *IoCompletionHandle = hIoCompletionHandle;
            } _SEH_HANDLE {

                Status = _SEH_GetExceptionCode();
            } _SEH_END;
        }
   }

   /* Return Status */
   return Status;
}

NTSTATUS
STDCALL
NtOpenIoCompletion(OUT PHANDLE IoCompletionHandle,
                   IN ACCESS_MASK DesiredAccess,
                   IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    HANDLE hIoCompletionHandle;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWrite(IoCompletionHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();
        } _SEH_END;

        if(!NT_SUCCESS(Status)) {

            return Status;
        }
    }

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                IoCompletionType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hIoCompletionHandle);

    if (NT_SUCCESS(Status)) {

        _SEH_TRY {

            *IoCompletionHandle = hIoCompletionHandle;
        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();
        } _SEH_END;
    }

    /* Return Status */
    return Status;
}


NTSTATUS
STDCALL
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
    DefaultQueryInfoBufferCheck(IoCompletionInformationClass,
                                IoCompletionInfoClass,
                                IoCompletionInformation,
                                IoCompletionInformationLength,
                                ResultLength,
                                PreviousMode,
                                &Status);
    if(!NT_SUCCESS(Status)) {

        DPRINT1("NtQueryMutant() failed, Status: 0x%x\n", Status);
        return Status;
    }

    /* Get the Object */
    Status = ObReferenceObjectByHandle(IoCompletionHandle,
                                       IO_COMPLETION_QUERY_STATE,
                                       IoCompletionType,
                                       PreviousMode,
                                       (PVOID*)&Queue,
                                       NULL);

    /* Check for Success */
   if (NT_SUCCESS(Status)) {

        _SEH_TRY {

            /* Return Info */
            ((PIO_COMPLETION_BASIC_INFORMATION)IoCompletionInformation)->Depth = KeReadStateQueue(Queue);
            ObDereferenceObject(Queue);

            /* Return Result Length if needed */
            if (ResultLength) {

                *ResultLength = sizeof(IO_COMPLETION_BASIC_INFORMATION);
            }
        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();
        } _SEH_END;
    }

    /* Return Status */
    return Status;
}

/*
 * Dequeues an I/O completion message from an I/O completion object
 */
NTSTATUS
STDCALL
NtRemoveIoCompletion(IN  HANDLE IoCompletionHandle,
                     OUT PVOID *CompletionKey,
                     OUT PVOID *CompletionContext,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     IN  PLARGE_INTEGER Timeout OPTIONAL)
{
    LARGE_INTEGER SafeTimeout;
    PKQUEUE Queue;
    PIO_COMPLETION_PACKET Packet;
    PLIST_ENTRY ListEntry;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWrite(CompletionKey,
                          sizeof(PVOID),
                          sizeof(ULONG));
            ProbeForWrite(CompletionContext,
                          sizeof(PVOID),
                          sizeof(ULONG));
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            if (Timeout != NULL) {

                ProbeForRead(Timeout,
                             sizeof(LARGE_INTEGER),
                             sizeof(ULONG));
                SafeTimeout = *Timeout;
                Timeout = &SafeTimeout;
            }
        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();
        } _SEH_END;

        if (!NT_SUCCESS(Status)) {

            return Status;
        }
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(IoCompletionHandle,
                                       IO_COMPLETION_MODIFY_STATE,
                                       IoCompletionType,
                                       PreviousMode,
                                       (PVOID*)&Queue,
                                       NULL);

    /* Check for success */
    if (NT_SUCCESS(Status)) {

        /* Remove queue */
        ListEntry = KeRemoveQueue(Queue, PreviousMode, Timeout);

        /* If we got a timeout or user_apc back, return the status */
        if ((NTSTATUS)ListEntry == STATUS_TIMEOUT || (NTSTATUS)ListEntry == STATUS_USER_APC) {

            Status = (NTSTATUS)ListEntry;

        } else {

            /* Get the Packet Data */
            Packet = CONTAINING_RECORD(ListEntry, IO_COMPLETION_PACKET, ListEntry);

            _SEH_TRY {

                /* Check if this is piggybacked on an IRP */
                if (Packet->PacketType == IrpCompletionPacket)
                {
                    /* Get the IRP */
                    PIRP Irp = NULL;
                    Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);
                    
                    /* Return values to user */
                    *CompletionKey = Irp->Tail.CompletionKey;
                    *CompletionContext = Irp->Overlay.AsynchronousParameters.UserApcContext;
                    *IoStatusBlock = Packet->IoStatus;
                    IoFreeIrp(Irp);
                }
                else
                {
                    /* This is a user-mode generated or API generated mini-packet */
                    *CompletionKey = Packet->Key;
                    *CompletionContext = Packet->Context;
                    *IoStatusBlock = Packet->IoStatus;
                    IopFreeIoCompletionPacket(Packet);
                }

            } _SEH_HANDLE {

                Status = _SEH_GetExceptionCode();
            } _SEH_END;
        }

        /* Dereference the Object */
        ObDereferenceObject(Queue);
    }

    /* Return status */
    return Status;
}

/*
 * Queues an I/O completion message to an I/O completion object
 */
NTSTATUS
STDCALL
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

    /* Check for Success */
    if (NT_SUCCESS(Status)) {

        /* Set the Completion */
        Status = IoSetIoCompletion(Queue,
                                   CompletionKey,
                                   CompletionContext,
                                   CompletionStatus,
                                   CompletionInformation,
                                   TRUE);
        ObDereferenceObject(Queue);
    }

    /* Return status */
    return Status;
}
