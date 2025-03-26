/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/internal/io_x.h
* PURPOSE:         Internal Inlined Functions for the I/O Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

static
__inline
NTSTATUS
IopLockFileObject(
    _In_ PFILE_OBJECT FileObject,
    _In_ KPROCESSOR_MODE WaitMode)
{
    BOOLEAN LockFailed;

    /* Lock the FO and check for contention */
    if (InterlockedExchange((PLONG)&FileObject->Busy, TRUE) == FALSE)
    {
        ObReferenceObject(FileObject);
        return STATUS_SUCCESS;
    }
    else
    {
        return IopAcquireFileObjectLock(FileObject,
                                        WaitMode,
                                        BooleanFlagOn(FileObject->Flags, FO_ALERTABLE_IO),
                                        &LockFailed);
    }
}

static
__inline
VOID
IopUnlockFileObject(IN PFILE_OBJECT FileObject)
{
    /* Unlock the FO and wake any waiters up */
    NT_VERIFY(InterlockedExchange((PLONG)&FileObject->Busy, FALSE) == TRUE);
    if (FileObject->Waiters)
    {
        KeSetEvent(&FileObject->Lock, IO_NO_INCREMENT, FALSE);
    }
    ObDereferenceObject(FileObject);
}

FORCEINLINE
VOID
IopQueueIrpToThread(IN PIRP Irp)
{
    PETHREAD Thread = Irp->Tail.Overlay.Thread;

    /* Disable special kernel APCs so we can't race with IopCompleteRequest.
     * IRP's thread must be the current thread */
    KeEnterGuardedRegionThread(&Thread->Tcb);

    /* Insert it into the list */
    InsertHeadList(&Thread->IrpList, &Irp->ThreadListEntry);

    /* Leave the guarded region */
    KeLeaveGuardedRegionThread(&Thread->Tcb);
}

FORCEINLINE
VOID
IopUnQueueIrpFromThread(IN PIRP Irp)
{
    /* Special kernel APCs must be disabled so we can't race with
     * IopCompleteRequest (or because we are called from there) */
    ASSERT(KeAreAllApcsDisabled());

    /* Remove it from the list and reset it */
    if (IsListEmpty(&Irp->ThreadListEntry))
        return;
    RemoveEntryList(&Irp->ThreadListEntry);
    InitializeListHead(&Irp->ThreadListEntry);
}

static
__inline
VOID
IopUpdateOperationCount(IN IOP_TRANSFER_TYPE Type)
{
    PLARGE_INTEGER CountToChange;

    /* Make sure I/O operations are being counted */
    if (IoCountOperations)
    {
        if (Type == IopReadTransfer)
        {
            /* Increase read count */
            IoReadOperationCount++;
            CountToChange = &PsGetCurrentProcess()->ReadOperationCount;
        }
        else if (Type == IopWriteTransfer)
        {
            /* Increase write count */
            IoWriteOperationCount++;
            CountToChange = &PsGetCurrentProcess()->WriteOperationCount;
        }
        else
        {
            /* Increase other count */
            IoOtherOperationCount++;
            CountToChange = &PsGetCurrentProcess()->OtherOperationCount;
        }

        /* Increase the process-wide count */
        ExInterlockedAddLargeStatistic(CountToChange, 1);
    }
}

static
__inline
VOID
IopUpdateTransferCount(IN IOP_TRANSFER_TYPE Type, IN ULONG TransferCount)
{
    PLARGE_INTEGER CountToChange;
    PLARGE_INTEGER TransferToChange;

    /* Make sure I/O operations are being counted */
    if (IoCountOperations)
    {
        if (Type == IopReadTransfer)
        {
            /* Increase read count */
            CountToChange = &PsGetCurrentProcess()->ReadTransferCount;
            TransferToChange = &IoReadTransferCount;
        }
        else if (Type == IopWriteTransfer)
        {
            /* Increase write count */
            CountToChange = &PsGetCurrentProcess()->WriteTransferCount;
            TransferToChange = &IoWriteTransferCount;
        }
        else
        {
            /* Increase other count */
            CountToChange = &PsGetCurrentProcess()->OtherTransferCount;
            TransferToChange = &IoOtherTransferCount;
        }

        /* Increase the process-wide count */
        ExInterlockedAddLargeStatistic(CountToChange, TransferCount);

        /* Increase global count */
        ExInterlockedAddLargeStatistic(TransferToChange, TransferCount);
    }
}

static
__inline
BOOLEAN
IopValidateOpenPacket(IN POPEN_PACKET OpenPacket)
{
    /* Validate the packet */
    if (!(OpenPacket) ||
        (OpenPacket->Type != IO_TYPE_OPEN_PACKET) ||
        (OpenPacket->Size != sizeof(OPEN_PACKET)))
    {
        /* Fail */
        return FALSE;
    }

    /* Good packet */
    return TRUE;
}
