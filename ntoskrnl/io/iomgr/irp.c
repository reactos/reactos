/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/irp.c
 * PURPOSE:         IRP Handling Functions
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Gunnar Dalsnes
 *                  Filip Navara (navaraf@reactos.org)
 *                  Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

PIRP IopDeadIrp;
RESERVE_IRP_ALLOCATOR IopReserveIrpAllocator;

/* PRIVATE FUNCTIONS  ********************************************************/

VOID
NTAPI
IopFreeIrpKernelApc(IN PKAPC Apc,
                    IN PKNORMAL_ROUTINE *NormalRoutine,
                    IN PVOID *NormalContext,
                    IN PVOID *SystemArgument1,
                    IN PVOID *SystemArgument2)
{
    /* Free the IRP */
    IoFreeIrp(CONTAINING_RECORD(Apc, IRP, Tail.Apc));
}

VOID
NTAPI
IopAbortIrpKernelApc(IN PKAPC Apc)
{
    /* Free the IRP */
    IoFreeIrp(CONTAINING_RECORD(Apc, IRP, Tail.Apc));
}

NTSTATUS
NTAPI
IopCleanupFailedIrp(IN PFILE_OBJECT FileObject,
                    IN PKEVENT EventObject OPTIONAL,
                    IN PVOID Buffer OPTIONAL)
{
    PAGED_CODE();

    /* Dereference the event */
    if (EventObject) ObDereferenceObject(EventObject);

    /* Free a buffer, if any */
    if (Buffer) ExFreePool(Buffer);

    /* If this was a file opened for synch I/O, then unlock it */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO) IopUnlockFileObject(FileObject);

    /* Now dereference it and return */
    ObDereferenceObject(FileObject);
    return STATUS_INSUFFICIENT_RESOURCES;
}

VOID
NTAPI
IopAbortInterruptedIrp(IN PKEVENT EventObject,
                       IN PIRP Irp)
{
    KIRQL OldIrql;
    BOOLEAN CancelResult;
    LARGE_INTEGER Wait;
    PAGED_CODE();

    /* Raise IRQL to APC */
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Check if nobody completed it yet */
    if (!KeReadStateEvent(EventObject))
    {
        /* First, cancel it */
        CancelResult = IoCancelIrp(Irp);
        KeLowerIrql(OldIrql);

        /* Check if we cancelled it */
        if (CancelResult)
        {
            /* Wait for the IRP to be cancelled */
            Wait.QuadPart = -100000;
            while (!KeReadStateEvent(EventObject))
            {
                /* Delay indefintely */
                KeDelayExecutionThread(KernelMode, FALSE, &Wait);
            }
        }
        else
        {
            /* No cancellation done, so wait for the I/O system to kill it */
            KeWaitForSingleObject(EventObject,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
    }
    else
    {
        /* We got preempted, so give up */
        KeLowerIrql(OldIrql);
    }
}

VOID
NTAPI
IopDisassociateThreadIrp(VOID)
{
    KIRQL OldIrql, LockIrql;
    PETHREAD IrpThread;
    PLIST_ENTRY IrpEntry;
    PIO_ERROR_LOG_PACKET ErrorLogEntry;
    PDEVICE_OBJECT DeviceObject = NULL;
    PIO_STACK_LOCATION IoStackLocation;

    /* First, raise to APC to protect IrpList */
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Get the Thread and check the list */
    IrpThread = PsGetCurrentThread();
    if (IsListEmpty(&IrpThread->IrpList))
    {
        /* It got completed now, so quit */
        KeLowerIrql(OldIrql);
        return;
    }

    /* Ensure no one will come disturb */
    LockIrql = KeAcquireQueuedSpinLock(LockQueueIoCompletionLock);

    /* Get the misbehaving IRP */
    IrpEntry = IrpThread->IrpList.Flink;
    IopDeadIrp = CONTAINING_RECORD(IrpEntry, IRP, ThreadListEntry);
    IOTRACE(IO_IRP_DEBUG,
            "%s - Deassociating IRP %p for %p\n",
            __FUNCTION__,
            IopDeadIrp,
            IrpThread);

    /* Don't cancel the IRP if it's already been completed far */
    if (IopDeadIrp->CurrentLocation == (IopDeadIrp->StackCount + 2))
    {
        /* Return */
        KeReleaseQueuedSpinLock(LockQueueIoCompletionLock, LockIrql);
        KeLowerIrql(OldIrql);
        return;
    }

    /* Disown the IRP! */
    IopDeadIrp->Tail.Overlay.Thread = NULL;
    RemoveHeadList(&IrpThread->IrpList);
    InitializeListHead(&IopDeadIrp->ThreadListEntry);

    /* Get the stack location and check if it's valid */
    IoStackLocation = IoGetCurrentIrpStackLocation(IopDeadIrp);
    if (IopDeadIrp->CurrentLocation <= IopDeadIrp->StackCount)
    {
        /* Get the device object */
        DeviceObject = IoStackLocation->DeviceObject;
    }

    KeReleaseQueuedSpinLock(LockQueueIoCompletionLock, LockIrql);
    /* Lower IRQL now, since we have the pointers we need */
    KeLowerIrql(OldIrql);

    /* Check if we can send an Error Log Entry*/
    if (DeviceObject)
    {
        /* Allocate an entry */
        ErrorLogEntry = IoAllocateErrorLogEntry(DeviceObject,
                                                sizeof(IO_ERROR_LOG_PACKET));
        if (ErrorLogEntry)
        {
            /* Write the entry */
            ErrorLogEntry->ErrorCode = IO_DRIVER_CANCEL_TIMEOUT;
            IoWriteErrorLogEntry(ErrorLogEntry);
        }
    }
}

VOID
NTAPI
IopCleanupIrp(IN PIRP Irp,
              IN PFILE_OBJECT FileObject)
{
    PMDL Mdl;
    IOTRACE(IO_IRP_DEBUG,
            "%s - Cleaning IRP %p for %p\n",
            __FUNCTION__,
            Irp,
            FileObject);

    /* Check if there's an MDL */
    while ((Mdl = Irp->MdlAddress))
    {
        /* Clear all of them */
        Irp->MdlAddress = Mdl->Next;
        IoFreeMdl(Mdl);
    }

    /* Check if the IRP has system buffer */
    if (Irp->Flags & IRP_DEALLOCATE_BUFFER)
    {
        /* Free the buffer */
        ExFreePoolWithTag(Irp->AssociatedIrp.SystemBuffer, TAG_IOBUF);
    }

    /* Check if this IRP has a user event, a file object, and is async */
    if ((Irp->UserEvent) &&
        !(Irp->Flags & IRP_SYNCHRONOUS_API) &&
        (FileObject))
    {
        /* Dereference the User Event */
        ObDereferenceObject(Irp->UserEvent);
    }

    /* Check if we have a file object and this isn't a create operation */
    if ((FileObject) && !(Irp->Flags & IRP_CREATE_OPERATION))
    {
        /* Dereference the file object */
        ObDereferenceObject(FileObject);
    }

    /* Free the IRP */
    IoFreeIrp(Irp);
}

VOID
NTAPI
IopCompleteRequest(IN PKAPC Apc,
                   IN PKNORMAL_ROUTINE* NormalRoutine,
                   IN PVOID* NormalContext,
                   IN PVOID* SystemArgument1,
                   IN PVOID* SystemArgument2)
{
    PFILE_OBJECT FileObject;
    PIRP Irp;
    PMDL Mdl, NextMdl;
    PVOID Port = NULL, Key = NULL;
    BOOLEAN SignaledCreateRequest = FALSE;

    /* Get data from the APC */
    FileObject = (PFILE_OBJECT)*SystemArgument1;
    Irp = CONTAINING_RECORD(Apc, IRP, Tail.Apc);
    IOTRACE(IO_IRP_DEBUG,
            "%s - Completing IRP %p for %p\n",
            __FUNCTION__,
            Irp,
            FileObject);

    /* Sanity check */
    ASSERT(Irp->IoStatus.Status != (NTSTATUS)0xFFFFFFFF);

    /* Check if we have a file object */
    if (*SystemArgument2)
    {
        /* Check if we're reparsing */
        if ((Irp->IoStatus.Status == STATUS_REPARSE) &&
            (Irp->IoStatus.Information == IO_REPARSE_TAG_MOUNT_POINT))
        {
            PREPARSE_DATA_BUFFER ReparseData;

            ReparseData = (PREPARSE_DATA_BUFFER)*SystemArgument2;

            ASSERT(ReparseData->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT);
            ASSERT(ReparseData->ReparseDataLength < MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
            ASSERT(ReparseData->Reserved < MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

            IopDoNameTransmogrify(Irp, FileObject, ReparseData);
        }
    }

    /* Handle Buffered case first */
    if (Irp->Flags & IRP_BUFFERED_IO)
    {
        /* Check if we have an input buffer and if we succeeded */
        if ((Irp->Flags & IRP_INPUT_OPERATION) &&
            (Irp->IoStatus.Status != STATUS_VERIFY_REQUIRED) &&
            !(NT_ERROR(Irp->IoStatus.Status)))
        {
            _SEH2_TRY
            {
                /* Copy the buffer back to the user */
                RtlCopyMemory(Irp->UserBuffer,
                              Irp->AssociatedIrp.SystemBuffer,
                              Irp->IoStatus.Information);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Fail the IRP */
                Irp->IoStatus.Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }

        /* Also check if we should de-allocate it */
        if (Irp->Flags & IRP_DEALLOCATE_BUFFER)
        {
            /* Deallocate it */
            ExFreePool(Irp->AssociatedIrp.SystemBuffer);
        }
    }

    /* Now we got rid of these two... */
    Irp->Flags &= ~(IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);

    /* Check if there's an MDL */
    for (Mdl = Irp->MdlAddress; Mdl; Mdl = NextMdl)
    {
        /* Free it */
        NextMdl = Mdl->Next;
        IoFreeMdl(Mdl);
    }

    /* No MDLs left */
    Irp->MdlAddress = NULL;

    /*
     * Check if either the request was completed without any errors
     * (but warnings are OK!), or if it was completed with an error, but
     * did return from a pending I/O Operation and is not synchronous.
     */
    if (!NT_ERROR(Irp->IoStatus.Status) ||
        (Irp->PendingReturned &&
         !IsIrpSynchronous(Irp, FileObject)))
    {
        /* Get any information we need from the FO before we kill it */
        if ((FileObject) && (FileObject->CompletionContext))
        {
            /* Save Completion Data */
            Port = FileObject->CompletionContext->Port;
            Key = FileObject->CompletionContext->Key;
        }

        /* Check for UserIos */
        if (Irp->UserIosb != NULL)
        {
            /* Use SEH to make sure we don't write somewhere invalid */
            _SEH2_TRY
            {
                /*  Save the IOSB Information */
                *Irp->UserIosb = Irp->IoStatus;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Ignore any error */
            }
            _SEH2_END;
        }

        /* Check if we have an event or a file object */
        if (Irp->UserEvent)
        {
            /* At the very least, this is a PKEVENT, so signal it always */
            KeSetEvent(Irp->UserEvent, 0, FALSE);

            /* Check if we also have a File Object */
            if (FileObject)
            {
                /* Check if this is an Asynch API */
                if (!(Irp->Flags & IRP_SYNCHRONOUS_API))
                {
                    /* Dereference the event */
                    ObDereferenceObject(Irp->UserEvent);
                }

                /*
                 * Now, if this is a Synch I/O File Object, then this event is
                 * NOT an actual Executive Event, so we won't dereference it,
                 * and instead, we will signal the File Object
                 */
                if ((FileObject->Flags & FO_SYNCHRONOUS_IO) &&
                    !(Irp->Flags & IRP_OB_QUERY_NAME))
                {
                    /* Signal the file object and set the status */
                    KeSetEvent(&FileObject->Event, 0, FALSE);
                    FileObject->FinalStatus = Irp->IoStatus.Status;
                }

                /*
                 * This could also be a create operation, in which case we want
                 * to make sure there's no APC fired.
                 */
                if (Irp->Flags & IRP_CREATE_OPERATION)
                {
                    /* Clear the APC Routine and remember this */
                    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
                    SignaledCreateRequest = TRUE;
                }
            }
        }
        else if (FileObject)
        {
            /* Signal the file object and set the status */
            KeSetEvent(&FileObject->Event, 0, FALSE);
            FileObject->FinalStatus = Irp->IoStatus.Status;

            /*
            * This could also be a create operation, in which case we want
            * to make sure there's no APC fired.
            */
            if (Irp->Flags & IRP_CREATE_OPERATION)
            {
                /* Clear the APC Routine and remember this */
                Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
                SignaledCreateRequest = TRUE;
            }
        }

        /* Update transfer count for everything but create operation */
        if (!(Irp->Flags & IRP_CREATE_OPERATION))
        {
            if (Irp->Flags & IRP_WRITE_OPERATION)
            {
                /* Update write transfer count */
                IopUpdateTransferCount(IopWriteTransfer,
                                       (ULONG)Irp->IoStatus.Information);
            }
            else if (Irp->Flags & IRP_READ_OPERATION)
            {
                /* Update read transfer count */
                IopUpdateTransferCount(IopReadTransfer,
                                       (ULONG)Irp->IoStatus.Information);
            }
            else
            {
                /* Update other transfer count */
                IopUpdateTransferCount(IopOtherTransfer,
                                       (ULONG)Irp->IoStatus.Information);
            }
        }

        /* Now that we've signaled the events, de-associate the IRP */
        IopUnQueueIrpFromThread(Irp);

        /* Now check if a User APC Routine was requested */
        if (Irp->Overlay.AsynchronousParameters.UserApcRoutine)
        {
            /* Initialize it */
            KeInitializeApc(&Irp->Tail.Apc,
                            KeGetCurrentThread(),
                            CurrentApcEnvironment,
                            IopFreeIrpKernelApc,
                            IopAbortIrpKernelApc,
                            (PKNORMAL_ROUTINE)Irp->
                            Overlay.AsynchronousParameters.UserApcRoutine,
                            Irp->RequestorMode,
                            Irp->
                            Overlay.AsynchronousParameters.UserApcContext);

            /* Queue it */
            KeInsertQueueApc(&Irp->Tail.Apc, Irp->UserIosb, NULL, 2);
        }
        else if ((Port) &&
                 (Irp->Overlay.AsynchronousParameters.UserApcContext))
        {
            /* We have an I/O Completion setup... create the special Overlay */
            Irp->Tail.CompletionKey = Key;
            Irp->Tail.Overlay.PacketType = IopCompletionPacketIrp;
            KeInsertQueue(Port, &Irp->Tail.Overlay.ListEntry);
        }
        else
        {
            /* Free the IRP since we don't need it anymore */
            IoFreeIrp(Irp);
        }

        /* Check if we have a file object that wasn't part of a create */
        if ((FileObject) && !(SignaledCreateRequest))
        {
            /* Dereference it, since it's not needed anymore either */
            ObDereferenceObjectDeferDelete(FileObject);
        }
    }
    else
    {
        /*
         * Either we didn't return from the request, or we did return but this
         * request was synchronous.
         */
        if ((Irp->PendingReturned) && (FileObject))
        {
            /* So we did return with a synch operation, was it the IRP? */
            if (Irp->Flags & IRP_SYNCHRONOUS_API)
            {
                /* Yes, this IRP was synchronous, so return the I/O Status */
                *Irp->UserIosb = Irp->IoStatus;

                /* Now check if the user gave an event */
                if (Irp->UserEvent)
                {
                    /* Signal it */
                    KeSetEvent(Irp->UserEvent, 0, FALSE);
                }
                else
                {
                    /* No event was given, so signal the FO instead */
                    KeSetEvent(&FileObject->Event, 0, FALSE);
                }
            }
            else
            {
                /*
                 * It's not the IRP that was synchronous, it was the FO
                 * that was opened this way. Signal its event.
                 */
                FileObject->FinalStatus = Irp->IoStatus.Status;
                KeSetEvent(&FileObject->Event, 0, FALSE);
            }
        }

        /* Now that we got here, we do this for incomplete I/Os as well */
        if ((FileObject) && !(Irp->Flags & IRP_CREATE_OPERATION))
        {
            /* Dereference the File Object unless this was a create */
            ObDereferenceObjectDeferDelete(FileObject);
        }

        /*
         * Check if this was an Executive Event (remember that we know this
         * by checking if the IRP is synchronous)
         */
        if ((Irp->UserEvent) &&
            (FileObject) &&
            !(Irp->Flags & IRP_SYNCHRONOUS_API))
        {
            /* This isn't a PKEVENT, so dereference it */
            ObDereferenceObject(Irp->UserEvent);
        }

        /* Now that we've signaled the events, de-associate the IRP */
        IopUnQueueIrpFromThread(Irp);

        /* Free the IRP as well */
        IoFreeIrp(Irp);
    }
}

BOOLEAN
NTAPI
IopInitializeReserveIrp(IN PRESERVE_IRP_ALLOCATOR ReserveIrpAllocator)
{
    /* Our allocated stack size */
    ReserveIrpAllocator->StackSize = 20;

    /* Allocate the IRP now */
    ReserveIrpAllocator->ReserveIrp = IoAllocateIrp(ReserveIrpAllocator->StackSize, FALSE);
    /* If we cannot, abort system boot */
    if (ReserveIrpAllocator->ReserveIrp == NULL)
    {
        return FALSE;
    }

    /* It's not in use */
    ReserveIrpAllocator->ReserveIrpInUse = 0;
    /* And init the event */
    KeInitializeEvent(&ReserveIrpAllocator->WaitEvent, SynchronizationEvent, FALSE);

    /* All good, keep booting */
    return TRUE;
}

PIRP
NTAPI
IopAllocateReserveIrp(IN CCHAR StackSize)
{
    /* If we need a stack size higher than what was allocated, then fail */
    if (StackSize > IopReserveIrpAllocator.StackSize)
    {
        return NULL;
    }

    /* Now, wait until the IRP becomes available and reserve it immediately */
    while (InterlockedExchange(&IopReserveIrpAllocator.ReserveIrpInUse, 1) == 1)
    {
        KeWaitForSingleObject(&IopReserveIrpAllocator.WaitEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    /* It's ours! Initialize it */
    IoInitializeIrp(IopReserveIrpAllocator.ReserveIrp, IoSizeOfIrp(StackSize), StackSize);

    /* And return it to the caller */
    return IopReserveIrpAllocator.ReserveIrp;
}

VOID
IopFreeReserveIrp(IN CCHAR PriorityBoost)
{
    /* Mark we don't use the IRP anymore */
    InterlockedExchange(&IopReserveIrpAllocator.ReserveIrpInUse, 0);

    /* And set the event if someone is waiting on the IRP */
    KeSetEvent(&IopReserveIrpAllocator.WaitEvent, PriorityBoost, FALSE);
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
PIRP
NTAPI
IoAllocateIrp(IN CCHAR StackSize,
              IN BOOLEAN ChargeQuota)
{
    PIRP Irp = NULL;
    USHORT Size = IoSizeOfIrp(StackSize);
    PKPRCB Prcb;
    UCHAR Flags = 0;
    PNPAGED_LOOKASIDE_LIST List = NULL;
    PP_NPAGED_LOOKASIDE_NUMBER ListType = LookasideSmallIrpList;

    /* Set Charge Quota Flag */
    if (ChargeQuota) Flags |= IRP_QUOTA_CHARGED;

    /* Get the PRCB */
    Prcb = KeGetCurrentPrcb();

    /* Figure out which Lookaside List to use */
    if ((StackSize <= 8) && (ChargeQuota == FALSE || Prcb->LookasideIrpFloat > 0))
    {
        /* Set Fixed Size Flag */
        Flags |= IRP_ALLOCATED_FIXED_SIZE;

        /* See if we should use big list */
        if (StackSize != 1)
        {
            Size = IoSizeOfIrp(8);
            ListType = LookasideLargeIrpList;
        }

        /* Get the P List First */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[ListType].P;

        /* Attempt allocation */
        List->L.TotalAllocates++;
        Irp = (PIRP)InterlockedPopEntrySList(&List->L.ListHead);

        /* Check if the P List failed */
        if (!Irp)
        {
            /* Let the balancer know */
            List->L.AllocateMisses++;

            /* Try the L List */
            List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[ListType].L;
            List->L.TotalAllocates++;
            Irp = (PIRP)InterlockedPopEntrySList(&List->L.ListHead);
        }
    }

    /* Check if we have to use the pool */
    if (!Irp)
    {
        /* Did we try lookaside and fail? */
        if (Flags & IRP_ALLOCATED_FIXED_SIZE) List->L.AllocateMisses++;

        /* Check if we should charge quota */
        if (ChargeQuota)
        {
            Irp = ExAllocatePoolWithQuotaTag(NonPagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                             Size,
                                             TAG_IRP);
        }
        else
        {
            /* Allocate the IRP with no quota charge */
            Irp = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_IRP);
        }

        /* Make sure it was sucessful */
        if (!Irp) return NULL;
    }
    else if (Flags & IRP_QUOTA_CHARGED)
    {
        /* Decrement lookaside float */
        InterlockedDecrement(&Prcb->LookasideIrpFloat);
        Flags |= IRP_LOOKASIDE_ALLOCATION;

        /* In this case there is no charge quota */
        Flags &= ~IRP_QUOTA_CHARGED;
    }

    /* Now Initialize it */
    IoInitializeIrp(Irp, Size, StackSize);

    /* Set the Allocation Flags */
    Irp->AllocationFlags = Flags;

    /* Return it */
    IOTRACE(IO_IRP_DEBUG,
            "%s - Allocated IRP %p with allocation flags %lx\n",
            __FUNCTION__,
            Irp,
            Flags);
    return Irp;
}

/*
 * @implemented
 */
PIRP
NTAPI
IopAllocateIrpMustSucceed(IN CCHAR StackSize)
{
    LONG i;
    PIRP Irp;
    LARGE_INTEGER Sleep;

    /* Try to get an IRP */
    Irp = IoAllocateIrp(StackSize, FALSE);
    if (Irp)
        return Irp;

    /* If we fail, start looping till we may get one */
    i = LONG_MAX;
    do {
        i--;

        /* First, sleep for 10ms */
        Sleep.QuadPart = -10 * 1000 * 10;
        KeDelayExecutionThread(KernelMode, FALSE, &Sleep);

        /* Then, retry allocation */
        Irp = IoAllocateIrp(StackSize, FALSE);
        if (Irp)
            return Irp;
    } while (i > 0);

    return Irp;
}

/*
 * @implemented
 */
PIRP
NTAPI
IoBuildAsynchronousFsdRequest(IN ULONG MajorFunction,
                              IN PDEVICE_OBJECT DeviceObject,
                              IN PVOID Buffer,
                              IN ULONG Length,
                              IN PLARGE_INTEGER StartingOffset,
                              IN PIO_STATUS_BLOCK IoStatusBlock)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;

    /* Allocate IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return NULL;

    /* Get the Stack */
    StackPtr = IoGetNextIrpStackLocation(Irp);

    /* Write the Major function and then deal with it */
    StackPtr->MajorFunction = (UCHAR)MajorFunction;

    /* Do not handle the following here */
    if ((MajorFunction != IRP_MJ_FLUSH_BUFFERS) &&
        (MajorFunction != IRP_MJ_SHUTDOWN) &&
        (MajorFunction != IRP_MJ_PNP) &&
        (MajorFunction != IRP_MJ_POWER))
    {
        /* Check if this is Buffered IO */
        if (DeviceObject->Flags & DO_BUFFERED_IO)
        {
            /* Allocate the System Buffer */
            Irp->AssociatedIrp.SystemBuffer =
                ExAllocatePoolWithTag(NonPagedPool, Length, TAG_IOBUF);
            if (!Irp->AssociatedIrp.SystemBuffer)
            {
                /* Free the IRP and fail */
                IoFreeIrp(Irp);
                return NULL;
            }

            /* Set flags */
            Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;

            /* Handle special IRP_MJ_WRITE Case */
            if (MajorFunction == IRP_MJ_WRITE)
            {
                /* Copy the buffer data */
                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, Buffer, Length);
            }
            else
            {
                /* Set the Input Operation flag and set this as a User Buffer */
                Irp->Flags |= IRP_INPUT_OPERATION;
                Irp->UserBuffer = Buffer;
            }
        }
        else if (DeviceObject->Flags & DO_DIRECT_IO)
        {
            /* Use an MDL for Direct I/O */
            Irp->MdlAddress = IoAllocateMdl(Buffer,
                                            Length,
                                            FALSE,
                                            FALSE,
                                            NULL);
            if (!Irp->MdlAddress)
            {
                /* Free the IRP and fail */
                IoFreeIrp(Irp);
                return NULL;
            }

            /* Probe and Lock */
            _SEH2_TRY
            {
                /* Do the probe */
                MmProbeAndLockPages(Irp->MdlAddress,
                                    KernelMode,
                                    MajorFunction == IRP_MJ_READ ?
                                    IoWriteAccess : IoReadAccess);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Free the IRP and its MDL */
                IoFreeMdl(Irp->MdlAddress);
                IoFreeIrp(Irp);

                /* Fail */
                _SEH2_YIELD(return NULL);
            }
            _SEH2_END;
        }
        else
        {
            /* Neither, use the buffer */
            Irp->UserBuffer = Buffer;
        }

        /* Check if this is a read */
        if (MajorFunction == IRP_MJ_READ)
        {
            /* Set the parameters for a read */
            StackPtr->Parameters.Read.Length = Length;
            StackPtr->Parameters.Read.ByteOffset = *StartingOffset;
        }
        else if (MajorFunction == IRP_MJ_WRITE)
        {
            /* Otherwise, set write parameters */
            StackPtr->Parameters.Write.Length = Length;
            StackPtr->Parameters.Write.ByteOffset = *StartingOffset;
        }
    }

    /* Set the Current Thread and IOSB */
    Irp->UserIosb = IoStatusBlock;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Return the IRP */
    IOTRACE(IO_IRP_DEBUG,
            "%s - Built IRP %p with Major, Buffer, DO %lx %p %p\n",
            __FUNCTION__,
            Irp,
            MajorFunction,
            Buffer,
            DeviceObject);
    return Irp;
}

/*
 * @implemented
 */
PIRP
NTAPI
IoBuildDeviceIoControlRequest(IN ULONG IoControlCode,
                              IN PDEVICE_OBJECT DeviceObject,
                              IN PVOID InputBuffer,
                              IN ULONG InputBufferLength,
                              IN PVOID OutputBuffer,
                              IN ULONG OutputBufferLength,
                              IN BOOLEAN InternalDeviceIoControl,
                              IN PKEVENT Event,
                              IN PIO_STATUS_BLOCK IoStatusBlock)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    ULONG BufferLength;

    /* Allocate IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return NULL;

    /* Get the Stack */
    StackPtr = IoGetNextIrpStackLocation(Irp);

    /* Set the DevCtl Type */
    StackPtr->MajorFunction = InternalDeviceIoControl ?
                              IRP_MJ_INTERNAL_DEVICE_CONTROL :
                              IRP_MJ_DEVICE_CONTROL;

    /* Set the IOCTL Data */
    StackPtr->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    StackPtr->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    StackPtr->Parameters.DeviceIoControl.OutputBufferLength =
        OutputBufferLength;

    /* Handle the Methods */
    switch (IO_METHOD_FROM_CTL_CODE(IoControlCode))
    {
        /* Buffered I/O */
        case METHOD_BUFFERED:

            /* Select the right Buffer Length */
            BufferLength = InputBufferLength > OutputBufferLength ?
                           InputBufferLength : OutputBufferLength;

            /* Make sure there is one */
            if (BufferLength)
            {
                /* Allocate the System Buffer */
                Irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithTag(NonPagedPool,
                                          BufferLength,
                                          TAG_IOBUF);
                if (!Irp->AssociatedIrp.SystemBuffer)
                {
                    /* Free the IRP and fail */
                    IoFreeIrp(Irp);
                    return NULL;
                }

                /* Check if we got a buffer */
                if (InputBuffer)
                {
                    /* Copy into the System Buffer */
                    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                                  InputBuffer,
                                  InputBufferLength);
                }

                /* Write the flags */
                Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
                if (OutputBuffer) Irp->Flags |= IRP_INPUT_OPERATION;

                /* Save the Buffer */
                Irp->UserBuffer = OutputBuffer;
            }
            else
            {
                /* Clear the Flags and Buffer */
                Irp->Flags = 0;
                Irp->UserBuffer = NULL;
            }
            break;

        /* Direct I/O */
        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:

            /* Check if we got an input buffer */
            if (InputBuffer)
            {
                /* Allocate the System Buffer */
                Irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithTag(NonPagedPool,
                                          InputBufferLength,
                                          TAG_IOBUF);
                if (!Irp->AssociatedIrp.SystemBuffer)
                {
                    /* Free the IRP and fail */
                    IoFreeIrp(Irp);
                    return NULL;
                }

                /* Copy into the System Buffer */
                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                              InputBuffer,
                              InputBufferLength);

                /* Write the flags */
                Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            }
            else
            {
                /* Clear the flags */
                Irp->Flags = 0;
            }

            /* Check if we got an output buffer */
            if (OutputBuffer)
            {
                /* Allocate the System Buffer */
                Irp->MdlAddress = IoAllocateMdl(OutputBuffer,
                                                OutputBufferLength,
                                                FALSE,
                                                FALSE,
                                                Irp);
                if (!Irp->MdlAddress)
                {
                    /* Free the IRP and fail */
                    IoFreeIrp(Irp);
                    return NULL;
                }

                /* Probe and Lock */
                _SEH2_TRY
                {
                    /* Do the probe */
                    MmProbeAndLockPages(Irp->MdlAddress,
                                        KernelMode,
                                        IO_METHOD_FROM_CTL_CODE(IoControlCode) ==
                                        METHOD_IN_DIRECT ?
                                        IoReadAccess : IoWriteAccess);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Free the MDL */
                    IoFreeMdl(Irp->MdlAddress);

                    /* Free the input buffer and IRP */
                    if (InputBuffer) ExFreePool(Irp->AssociatedIrp.SystemBuffer);
                    IoFreeIrp(Irp);

                    /* Fail */
                    _SEH2_YIELD(return NULL);
                }
                _SEH2_END;
            }
            break;

        case METHOD_NEITHER:

            /* Just save the Buffer */
            Irp->UserBuffer = OutputBuffer;
            StackPtr->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
    }

    /* Now write the Event and IoSB */
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;

    /* Sync IRPs are queued to requestor thread's irp cancel/cleanup list */
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IoQueueThreadIrp(Irp);

    /* Return the IRP */
    IOTRACE(IO_IRP_DEBUG,
            "%s - Built IRP %p with IOCTL, Buffers, DO %lx %p %p %p\n",
            __FUNCTION__,
            Irp,
            IoControlCode,
            InputBuffer,
            OutputBuffer,
            DeviceObject);
    return Irp;
}

/*
 * @implemented
 */
PIRP
NTAPI
IoBuildSynchronousFsdRequest(IN ULONG MajorFunction,
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PVOID Buffer,
                             IN ULONG Length,
                             IN PLARGE_INTEGER StartingOffset,
                             IN PKEVENT Event,
                             IN PIO_STATUS_BLOCK IoStatusBlock)
{
    PIRP Irp;

    /* Do the big work to set up the IRP */
    Irp = IoBuildAsynchronousFsdRequest(MajorFunction,
                                        DeviceObject,
                                        Buffer,
                                        Length,
                                        StartingOffset,
                                        IoStatusBlock );
    if (!Irp) return NULL;

    /* Set the Event which makes it Syncronous */
    Irp->UserEvent = Event;

    /* Sync IRPs are queued to requestor thread's irp cancel/cleanup list */
    IoQueueThreadIrp(Irp);
    return Irp;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoCancelIrp(IN PIRP Irp)
{
    KIRQL OldIrql;
    PDRIVER_CANCEL CancelRoutine;
    IOTRACE(IO_IRP_DEBUG,
            "%s - Canceling IRP %p\n",
            __FUNCTION__,
            Irp);
    ASSERT(Irp->Type == IO_TYPE_IRP);

    /* Acquire the cancel lock and cancel the IRP */
    IoAcquireCancelSpinLock(&OldIrql);
    Irp->Cancel = TRUE;

    /* Clear the cancel routine and get the old one */
    CancelRoutine = IoSetCancelRoutine(Irp, NULL);
    if (CancelRoutine)
    {
        /* We had a routine, make sure the IRP isn't completed */
        if (Irp->CurrentLocation > (Irp->StackCount + 1))
        {
            /* It is, bugcheck */
            KeBugCheckEx(CANCEL_STATE_IN_COMPLETED_IRP,
                         (ULONG_PTR)Irp,
                         (ULONG_PTR)CancelRoutine,
                         0,
                         0);
        }

        /* Set the cancel IRQL And call the routine */
        Irp->CancelIrql = OldIrql;
        CancelRoutine(IoGetCurrentIrpStackLocation(Irp)->DeviceObject, Irp);
        return TRUE;
    }

    /* Otherwise, release the cancel lock and fail */
    IoReleaseCancelSpinLock(OldIrql);
    return FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
IoCancelThreadIo(IN PETHREAD Thread)
{
    KIRQL OldIrql;
    ULONG Retries = 3000;
    LARGE_INTEGER Interval;
    PLIST_ENTRY ListHead, NextEntry;
    PIRP Irp;
    PAGED_CODE();

    /* Windows isn't using given thread, but using current. */
    Thread = PsGetCurrentThread();

    IOTRACE(IO_IRP_DEBUG,
            "%s - Canceling IRPs for Thread %p\n",
            __FUNCTION__,
            Thread);

    /* Raise to APC to protect the IrpList */
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Start by cancelling all the IRPs in the current thread queue. */
    ListHead = &Thread->IrpList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the IRP */
        Irp = CONTAINING_RECORD(NextEntry, IRP, ThreadListEntry);

        /* Cancel it */
        IoCancelIrp(Irp);

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;
    }

     /* Wait 100 milliseconds */
    Interval.QuadPart = -1000000;

    /* Wait till all the IRPs are completed or cancelled. */
    while (!IsListEmpty(&Thread->IrpList))
    {
        /* Now we can lower */
        KeLowerIrql(OldIrql);

        /* Wait a short while and then look if all our IRPs were completed. */
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);

        /*
         * Don't stay here forever if some broken driver doesn't complete
         * the IRP.
         */
        if (!(Retries--))
        {
            /* Print out a message and remove the IRP */
            DPRINT1("Broken driver did not complete!\n");
            IopDisassociateThreadIrp();
        }

        /* Raise the IRQL Again */
        KeRaiseIrql(APC_LEVEL, &OldIrql);
    }

    /* We're done, lower the IRQL */
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
#undef IoCallDriver
NTSTATUS
NTAPI
IoCallDriver(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    /* Call fastcall */
    return IofCallDriver(DeviceObject, Irp);
}

#define IoCallDriver IofCallDriver

/*
 * @implemented
 */
#undef IoCompleteRequest
VOID
NTAPI
IoCompleteRequest(IN PIRP Irp,
                  IN CCHAR PriorityBoost)
{
    /* Call the fastcall */
    IofCompleteRequest(Irp, PriorityBoost);
}

#define IoCompleteRequest IofCompleteRequest

/*
 * @implemented
 */
VOID
NTAPI
IoEnqueueIrp(IN PIRP Irp)
{
    /* This is the same as calling IoQueueThreadIrp */
    IoQueueThreadIrp(Irp);
}

/*
 * @implemented
 */
NTSTATUS
FASTCALL
IofCallDriver(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    PDRIVER_OBJECT DriverObject;
    PIO_STACK_LOCATION StackPtr;

    /* Make sure this is a valid IRP */
    ASSERT(Irp->Type == IO_TYPE_IRP);

    /* Get the Driver Object */
    DriverObject = DeviceObject->DriverObject;

    /* Decrease the current location and check if */
    Irp->CurrentLocation--;
    if (Irp->CurrentLocation <= 0)
    {
        /* This IRP ran out of stack, bugcheck */
        KeBugCheckEx(NO_MORE_IRP_STACK_LOCATIONS, (ULONG_PTR)Irp, 0, 0, 0);
    }

    /* Now update the stack location */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    Irp->Tail.Overlay.CurrentStackLocation = StackPtr;

    /* Get the Device Object */
    StackPtr->DeviceObject = DeviceObject;

    /* Call it */
    return DriverObject->MajorFunction[StackPtr->MajorFunction](DeviceObject,
                                                                Irp);
}

FORCEINLINE
VOID
IopClearStackLocation(IN PIO_STACK_LOCATION IoStackLocation)
{
    IoStackLocation->MinorFunction = 0;
    IoStackLocation->Flags = 0;
    IoStackLocation->Control &= SL_ERROR_RETURNED;
    IoStackLocation->Parameters.Others.Argument1 = 0;
    IoStackLocation->Parameters.Others.Argument2 = 0;
    IoStackLocation->Parameters.Others.Argument3 = 0;
    IoStackLocation->FileObject = NULL;
}

/*
 * @implemented
 */
VOID
FASTCALL
IofCompleteRequest(IN PIRP Irp,
                   IN CCHAR PriorityBoost)
{
    PIO_STACK_LOCATION StackPtr, LastStackPtr;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PETHREAD Thread;
    NTSTATUS Status;
    PMDL Mdl, NextMdl;
    ULONG MasterCount;
    PIRP MasterIrp;
    ULONG Flags;
    NTSTATUS ErrorCode = STATUS_SUCCESS;
    PREPARSE_DATA_BUFFER DataBuffer = NULL;
    IOTRACE(IO_IRP_DEBUG,
            "%s - Completing IRP %p\n",
            __FUNCTION__,
            Irp);

    /* Make sure this IRP isn't getting completed twice or is invalid */
    if ((Irp->CurrentLocation) > (Irp->StackCount + 1))
    {
        /* Bugcheck */
        KeBugCheckEx(MULTIPLE_IRP_COMPLETE_REQUESTS, (ULONG_PTR)Irp, 0, 0, 0);
    }

    /* Some sanity checks */
    ASSERT(Irp->Type == IO_TYPE_IRP);
    ASSERT(!Irp->CancelRoutine);
    ASSERT(Irp->IoStatus.Status != STATUS_PENDING);
    ASSERT(Irp->IoStatus.Status != (NTSTATUS)0xFFFFFFFF);

    /* Get the last stack */
    LastStackPtr = (PIO_STACK_LOCATION)(Irp + 1);
    if (LastStackPtr->Control & SL_ERROR_RETURNED)
    {
        /* Get the error code */
        ErrorCode = PtrToUlong(LastStackPtr->Parameters.Others.Argument4);
    }

    /*
     * Start the loop with the current stack and point the IRP to the next stack
     * and then keep incrementing the stack as we loop through. The IRP should
     * always point to the next stack location w.r.t the one currently being
     * analyzed, so completion routine code will see the appropriate value.
     * Because of this, we must loop until the current stack location is +1 of
     * the stack count, because when StackPtr is at the end, CurrentLocation is +1.
     */
    for (StackPtr = IoGetCurrentIrpStackLocation(Irp),
         Irp->CurrentLocation++,
         Irp->Tail.Overlay.CurrentStackLocation++;
         Irp->CurrentLocation <= (Irp->StackCount + 1);
         StackPtr++,
         Irp->CurrentLocation++,
         Irp->Tail.Overlay.CurrentStackLocation++)
    {
        /* Set Pending Returned */
        Irp->PendingReturned = StackPtr->Control & SL_PENDING_RETURNED;

        /* Check if we failed */
        if (!NT_SUCCESS(Irp->IoStatus.Status))
        {
            /* Check if it was changed by a completion routine */
            if (Irp->IoStatus.Status != ErrorCode)
            {
                /* Update the error for the current stack */
                ErrorCode = Irp->IoStatus.Status;
                StackPtr->Control |= SL_ERROR_RETURNED;
                LastStackPtr->Parameters.Others.Argument4 = UlongToPtr(ErrorCode);
                LastStackPtr->Control |= SL_ERROR_RETURNED;
            }
        }

        /* Check if there is a Completion Routine to Call */
        if ((NT_SUCCESS(Irp->IoStatus.Status) &&
             (StackPtr->Control & SL_INVOKE_ON_SUCCESS)) ||
            (!NT_SUCCESS(Irp->IoStatus.Status) &&
             (StackPtr->Control & SL_INVOKE_ON_ERROR)) ||
            (Irp->Cancel &&
             (StackPtr->Control & SL_INVOKE_ON_CANCEL)))
        {
            /* Clear the stack location */
            IopClearStackLocation(StackPtr);

            /* Check for highest-level device completion routines */
            if (Irp->CurrentLocation == (Irp->StackCount + 1))
            {
                /* Clear the DO, since the current stack location is invalid */
                DeviceObject = NULL;
            }
            else
            {
                /* Otherwise, return the real one */
                DeviceObject = IoGetCurrentIrpStackLocation(Irp)->DeviceObject;
            }

            /* Call the completion routine */
            Status = StackPtr->CompletionRoutine(DeviceObject,
                                                 Irp,
                                                 StackPtr->Context);

            /* Don't touch the Packet in this case, since it might be gone! */
            if (Status == STATUS_MORE_PROCESSING_REQUIRED) return;
        }
        else
        {
            /* Otherwise, check if this is a completed IRP */
            if ((Irp->CurrentLocation <= Irp->StackCount) &&
                (Irp->PendingReturned))
            {
                /* Mark it as pending */
                IoMarkIrpPending(Irp);
            }

            /* Clear the stack location */
            IopClearStackLocation(StackPtr);
        }
    }

    /* Check if the IRP is an associated IRP */
    if (Irp->Flags & IRP_ASSOCIATED_IRP)
    {
        /* Get the master IRP and count */
        MasterIrp = Irp->AssociatedIrp.MasterIrp;
        MasterCount = InterlockedDecrement(&MasterIrp->AssociatedIrp.IrpCount);

        /* Free the MDLs */
        for (Mdl = Irp->MdlAddress; Mdl; Mdl = NextMdl)
        {
            /* Go to the next one */
            NextMdl = Mdl->Next;
            IoFreeMdl(Mdl);
        }

        /* Free the IRP itself */
        IoFreeIrp(Irp);

        /* Complete the Master IRP */
        if (!MasterCount) IofCompleteRequest(MasterIrp, PriorityBoost);
        return;
    }

    /* Check whether we have to reparse */
    if (Irp->IoStatus.Status == STATUS_REPARSE)
    {
        if (Irp->IoStatus.Information > IO_REMOUNT)
        {
            /* If that's a reparse tag we understand, save the buffer from deletion */
            if (Irp->IoStatus.Information == IO_REPARSE_TAG_MOUNT_POINT)
            {
                ASSERT(Irp->Tail.Overlay.AuxiliaryBuffer != NULL);
                DataBuffer = (PREPARSE_DATA_BUFFER)Irp->Tail.Overlay.AuxiliaryBuffer;
                Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
            }
            else
            {
                Irp->IoStatus.Status = STATUS_IO_REPARSE_TAG_NOT_HANDLED;
            }
        }
    }

    /* Check if we have an auxiliary buffer */
    if (Irp->Tail.Overlay.AuxiliaryBuffer)
    {
        /* Free it */
        ExFreePool(Irp->Tail.Overlay.AuxiliaryBuffer);
        Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
    }

    /* Check if this is a Paging I/O or Close Operation */
    if (Irp->Flags & (IRP_PAGING_IO | IRP_CLOSE_OPERATION))
    {
        /* Handle a Close Operation or Sync Paging I/O */
        if (Irp->Flags & (IRP_SYNCHRONOUS_PAGING_IO | IRP_CLOSE_OPERATION))
        {
            /* Set the I/O Status and Signal the Event */
            Flags = Irp->Flags & (IRP_SYNCHRONOUS_PAGING_IO | IRP_PAGING_IO);
            *Irp->UserIosb = Irp->IoStatus;
            KeSetEvent(Irp->UserEvent, PriorityBoost, FALSE);

            /* Free the IRP for a Paging I/O Only, Close is handled by us */
            if (Flags)
            {
                /* If we were using the reserve IRP, then call the appropriate
                 * free function (to make the IRP available again)
                 */
                if (Irp == IopReserveIrpAllocator.ReserveIrp)
                {
                    IopFreeReserveIrp(PriorityBoost);
                }
                /* Otherwise, free for real! */
                else
                {
                    IoFreeIrp(Irp);
                }
            }
        }
        else
        {
#if 0
            /* Page 166 */
            KeInitializeApc(&Irp->Tail.Apc
                            &Irp->Tail.Overlay.Thread->Tcb,
                            Irp->ApcEnvironment,
                            IopCompletePageWrite,
                            NULL,
                            NULL,
                            KernelMode,
                            NULL);
            KeInsertQueueApc(&Irp->Tail.Apc,
                             NULL,
                             NULL,
                             PriorityBoost);
#else
            /* Not implemented yet. */
            UNIMPLEMENTED_DBGBREAK("Not supported!\n");
#endif
        }

        /* Get out of here */
        return;
    }

    /* Unlock MDL Pages, page 167. */
    Mdl = Irp->MdlAddress;
    while (Mdl)
    {
        MmUnlockPages(Mdl);
        Mdl = Mdl->Next;
    }

    /* Check if we should exit because of a Deferred I/O (page 168) */
    if ((Irp->Flags & IRP_DEFER_IO_COMPLETION) && !(Irp->PendingReturned))
    {
        /* Restore the saved reparse buffer for the caller */
        if (Irp->IoStatus.Status == STATUS_REPARSE &&
            Irp->IoStatus.Information == IO_REPARSE_TAG_MOUNT_POINT)
        {
            Irp->Tail.Overlay.AuxiliaryBuffer = (PCHAR)DataBuffer;
        }

        /*
         * Return without queuing the completion APC, since the caller will
         * take care of doing its own optimized completion at PASSIVE_LEVEL.
         */
        return;
    }

    /* Get the thread and file object */
    Thread = Irp->Tail.Overlay.Thread;
    FileObject = Irp->Tail.Overlay.OriginalFileObject;

    /* Make sure the IRP isn't canceled */
    if (!Irp->Cancel)
    {
        /* Initialize the APC */
        KeInitializeApc(&Irp->Tail.Apc,
                        &Thread->Tcb,
                        Irp->ApcEnvironment,
                        IopCompleteRequest,
                        NULL,
                        NULL,
                        KernelMode,
                        NULL);

        /* Queue it */
        KeInsertQueueApc(&Irp->Tail.Apc,
                         FileObject,
                         DataBuffer,
                         PriorityBoost);
    }
    else
    {
        /* The IRP just got canceled... does a thread still own it? */
        if (Thread)
        {
            /* Yes! There is still hope! Initialize the APC */
            KeInitializeApc(&Irp->Tail.Apc,
                            &Thread->Tcb,
                            Irp->ApcEnvironment,
                            IopCompleteRequest,
                            NULL,
                            NULL,
                            KernelMode,
                            NULL);

            /* Queue it */
            KeInsertQueueApc(&Irp->Tail.Apc,
                             FileObject,
                             DataBuffer,
                             PriorityBoost);
        }
        else
        {
            /* Nothing left for us to do, kill it */
            ASSERT(Irp->Cancel);
            IopCleanupIrp(Irp, FileObject);
        }
    }
}

NTSTATUS
NTAPI
IopSynchronousCompletion(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp,
                         IN PVOID Context)
{
    if (Irp->PendingReturned)
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoForwardIrpSynchronously(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    /* Check if next stack location is available */
    if (Irp->CurrentLocation > Irp->StackCount || Irp->CurrentLocation <= 1)
    {
        /* No more stack location */
        return FALSE;
    }

    /* Initialize event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Copy stack location for next driver */
    IoCopyCurrentIrpStackLocationToNext(Irp);

    /* Set a completion routine, which will signal the event */
    IoSetCompletionRoutine(Irp, IopSynchronousCompletion, &Event, TRUE, TRUE, TRUE);

    /* Call next driver */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Check if irp is pending */
    if (Status == STATUS_PENDING)
    {
        /* Yes, wait for its completion */
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
    }

    /* Return success */
    return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
IoFreeIrp(IN PIRP Irp)
{
    PNPAGED_LOOKASIDE_LIST List;
    PP_NPAGED_LOOKASIDE_NUMBER ListType = LookasideSmallIrpList;
    PKPRCB Prcb;
    IOTRACE(IO_IRP_DEBUG,
            "%s - Freeing IRPs %p\n",
            __FUNCTION__,
            Irp);

    /* Make sure the Thread IRP list is empty and that it OK to free it */
    ASSERT(Irp->Type == IO_TYPE_IRP);
    ASSERT(IsListEmpty(&Irp->ThreadListEntry));
    ASSERT(Irp->CurrentLocation >= Irp->StackCount);

    /* Get the PRCB */
    Prcb = KeGetCurrentPrcb();

    /* If this was a lookaside alloc, increment lookaside float */
    if (Irp->AllocationFlags & IRP_LOOKASIDE_ALLOCATION)
    {
        Irp->AllocationFlags &= ~IRP_LOOKASIDE_ALLOCATION;
        InterlockedIncrement(&Prcb->LookasideIrpFloat);
    }

    /* If this was a pool alloc, free it with the pool */
    if (!(Irp->AllocationFlags & IRP_ALLOCATED_FIXED_SIZE))
    {
        /* Free it */
        ExFreePoolWithTag(Irp, TAG_IRP);
    }
    else
    {
        /* Check if this was a Big IRP */
        if (Irp->StackCount != 1) ListType = LookasideLargeIrpList;

        /* Use the P List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[ListType].P;
        List->L.TotalFrees++;

        /* Check if the Free was within the Depth or not */
        if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
        {
            /* Let the balancer know */
            List->L.FreeMisses++;

            /* Use the L List */
            List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[ListType].L;
            List->L.TotalFrees++;

            /* Check if the Free was within the Depth or not */
            if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
            {
                /* All lists failed, use the pool */
                List->L.FreeMisses++;
                ExFreePoolWithTag(Irp, TAG_IRP);
                Irp = NULL;
            }
        }

        /* The free was within the Depth */
        if (Irp)
        {
            /* Remove the association with the process */
            if (Irp->AllocationFlags & IRP_QUOTA_CHARGED)
            {
                ExReturnPoolQuota(Irp);
                Irp->AllocationFlags &= ~IRP_QUOTA_CHARGED;
            }

            /* Add it to the lookaside list */
            InterlockedPushEntrySList(&List->L.ListHead,
                                      (PSLIST_ENTRY)Irp);
        }
    }
}

/*
 * @implemented
 */
IO_PAGING_PRIORITY
FASTCALL
IoGetPagingIoPriority(IN PIRP Irp)
{
    IO_PAGING_PRIORITY Priority;
    ULONG Flags;

    /* Get the flags */
    Flags = Irp->Flags;

    /* Check what priority it has */
    if (Flags & IRP_CLASS_CACHE_OPERATION)
    {
        /* High priority */
        Priority = IoPagingPriorityHigh;
    }
    else if (Flags & IRP_PAGING_IO)
    {
        /* Normal priority */
        Priority = IoPagingPriorityNormal;
    }
    else
    {
        /* Invalid -- not a paging IRP */
        Priority = IoPagingPriorityInvalid;
    }

    /* Return the priority */
    return Priority;
}

/*
 * @implemented
 */
PEPROCESS
NTAPI
IoGetRequestorProcess(IN PIRP Irp)
{
    /* Return the requestor process */
    if (Irp->Tail.Overlay.Thread)
    {
        if (Irp->ApcEnvironment == OriginalApcEnvironment)
        {
            return Irp->Tail.Overlay.Thread->ThreadsProcess;
        }
        else if (Irp->ApcEnvironment == AttachedApcEnvironment)
        {
            return (PEPROCESS)Irp->Tail.Overlay.Thread->Tcb.ApcState.Process;
        }
    }

    return NULL;
}

/*
 * @implemented
 */
ULONG
NTAPI
IoGetRequestorProcessId(IN PIRP Irp)
{
    PEPROCESS Process;

    /* Return the requestor process' id */
    Process = IoGetRequestorProcess(Irp);
    if (Process) return PtrToUlong(Process->UniqueProcessId);

    return 0;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoGetRequestorSessionId(IN PIRP Irp,
                        OUT PULONG pSessionId)
{
    PEPROCESS Process;

    /* Return the session */
    if (Irp->Tail.Overlay.Thread)
    {
        Process = Irp->Tail.Overlay.Thread->ThreadsProcess;
        *pSessionId = MmGetSessionId(Process);
        return STATUS_SUCCESS;
    }

    *pSessionId = (ULONG)-1;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
PIRP
NTAPI
IoGetTopLevelIrp(VOID)
{
    /* Return the IRP */
    return (PIRP)PsGetCurrentThread()->TopLevelIrp;
}

/*
 * @implemented
 */
VOID
NTAPI
IoInitializeIrp(IN PIRP Irp,
                IN USHORT PacketSize,
                IN CCHAR StackSize)
{
    /* Clear it */
    IOTRACE(IO_IRP_DEBUG,
            "%s - Initializing IRP %p\n",
            __FUNCTION__,
            Irp);
    RtlZeroMemory(Irp, PacketSize);

    /* Set the Header and other data */
    Irp->Type = IO_TYPE_IRP;
    Irp->Size = PacketSize;
    Irp->StackCount = StackSize;
    Irp->CurrentLocation = StackSize + 1;
    Irp->ApcEnvironment =  KeGetCurrentThread()->ApcStateIndex;
    Irp->Tail.Overlay.CurrentStackLocation = (PIO_STACK_LOCATION)(Irp + 1) + StackSize;

    /* Initialize the Thread List */
    InitializeListHead(&Irp->ThreadListEntry);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoIsOperationSynchronous(IN PIRP Irp)
{
    BOOLEAN SynchIO;
    BOOLEAN ForceAsync;

    /* If the IRP requests synchronous paging I/O, if the file object was opened
       for synchronous I/O, if the IRP_SYNCHRONOUS_API flag is set in the IRP
       the operation is synchronous */
    SynchIO = (IoGetCurrentIrpStackLocation(Irp)->FileObject->Flags & FO_SYNCHRONOUS_IO) ||
              (Irp->Flags & IRP_SYNCHRONOUS_API) || (Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO);

    /* If the IRP requests asynchronous paging I/O, the operation is asynchronous,
       even if one of the above conditions is true */
    ForceAsync = (Irp->Flags & IRP_PAGING_IO) && !(Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO);

    /* Check the flags */
    if (SynchIO && !ForceAsync)
    {
        /* Synch API or Paging I/O is OK, as is Sync File I/O */
        return TRUE;
    }

    /* Otherwise, it is an asynchronous operation. */
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
IoIsValidNameGraftingBuffer(IN PIRP Irp,
                            IN PREPARSE_DATA_BUFFER ReparseBuffer)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
PIRP
NTAPI
IoMakeAssociatedIrp(IN PIRP Irp,
                    IN CCHAR StackSize)
{
    PIRP AssocIrp;
    IOTRACE(IO_IRP_DEBUG,
            "%s - Associating IRP %p\n",
            __FUNCTION__,
            Irp);

    /* Allocate the IRP */
    AssocIrp = IoAllocateIrp(StackSize, FALSE);
    if (!AssocIrp) return NULL;

    /* Set the Flags */
    AssocIrp->Flags |= IRP_ASSOCIATED_IRP;

    /* Set the Thread */
    AssocIrp->Tail.Overlay.Thread = Irp->Tail.Overlay.Thread;

    /* Associate them */
    AssocIrp->AssociatedIrp.MasterIrp = Irp;
    return AssocIrp;
}

/*
 * @implemented
 */
VOID
NTAPI
IoQueueThreadIrp(IN PIRP Irp)
{
    IOTRACE(IO_IRP_DEBUG,
            "%s - Queueing IRP %p\n",
            __FUNCTION__,
            Irp);

    /* Use our inlined routine */
    IopQueueIrpToThread(Irp);
}

/*
 * @implemented
 * Reference: Chris Cant's "Writing WDM Device Drivers"
 */
VOID
NTAPI
IoReuseIrp(IN OUT PIRP Irp,
           IN NTSTATUS Status)
{
    UCHAR AllocationFlags;
    IOTRACE(IO_IRP_DEBUG,
            "%s - Reusing IRP %p\n",
            __FUNCTION__,
            Irp);

    /* Make sure it's OK to reuse it */
    ASSERT(!Irp->CancelRoutine);
    ASSERT(IsListEmpty(&Irp->ThreadListEntry));

    /* Get the old flags */
    AllocationFlags = Irp->AllocationFlags;

    /* Reinitialize the IRP */
    IoInitializeIrp(Irp, Irp->Size, Irp->StackCount);

    /* Duplicate the data */
    Irp->IoStatus.Status = Status;
    Irp->AllocationFlags = AllocationFlags;
}

/*
 * @implemented
 */
VOID
NTAPI
IoSetTopLevelIrp(IN PIRP Irp)
{
    /* Set the IRP */
    PsGetCurrentThread()->TopLevelIrp = (ULONG_PTR)Irp;
}

#if defined (_WIN64)
BOOLEAN
NTAPI
IoIs32bitProcess(
    IN PIRP Irp OPTIONAL)
{
    UNIMPLEMENTED;
    return FALSE;
}
#endif
