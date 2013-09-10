#include "npfs.h"

VOID
NTAPI
NpCancelWaitQueueIrp(IN PDEVICE_OBJECT DeviceObject, 
                     IN PIRP Irp)
{
    KIRQL OldIrql;
    PNP_WAIT_QUEUE_ENTRY WaitEntry;
    PNP_WAIT_QUEUE WaitQueue;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    WaitQueue = (PNP_WAIT_QUEUE)Irp->Tail.Overlay.DriverContext[0];

    OldIrql = KfAcquireSpinLock(&WaitQueue->WaitLock);

    WaitEntry = (PNP_WAIT_QUEUE_ENTRY)Irp->Tail.Overlay.DriverContext[1];
    if (WaitEntry)
    {
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
        if (!KeCancelTimer(&WaitEntry->Timer))
        {
            WaitEntry->Irp = NULL;
            WaitEntry = NULL;
        }
    }

    KfReleaseSpinLock(&WaitQueue->WaitLock, OldIrql);

    if (WaitEntry)
    {
        ObfDereferenceObject(WaitEntry->FileObject);
        ExFreePool(WaitEntry);
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
}
 
VOID
NTAPI
NpTimerDispatch(IN PKDPC Dpc,
                IN PVOID Context,
                IN PVOID Argument1,
                IN PVOID Argument2)
{
    PIRP Irp;
    KIRQL OldIrql;
    PNP_WAIT_QUEUE_ENTRY WaitEntry = Context;

    OldIrql = KfAcquireSpinLock(&WaitEntry->WaitQueue->WaitLock);
    Irp = WaitEntry->Irp;
    if (Irp)
    {
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
        if (!IoSetCancelRoutine(Irp, NULL))
        {
            Irp->Tail.Overlay.DriverContext[1] = NULL;
            Irp = NULL;
        }
    }
    KfReleaseSpinLock(&WaitEntry->WaitQueue->WaitLock, OldIrql);
    if (Irp)
    {
        Irp->IoStatus.Status = STATUS_IO_TIMEOUT;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }
    ObfDereferenceObject(WaitEntry->FileObject);
    ExFreePool(WaitEntry);
}

VOID
NTAPI
NpInitializeWaitQueue(IN PNP_WAIT_QUEUE WaitQueue)
{
    InitializeListHead(&WaitQueue->WaitList);
    KeInitializeSpinLock(&WaitQueue->WaitLock);
}

NTSTATUS
NTAPI
NpCancelWaiter(IN PNP_WAIT_QUEUE WaitQueue,
               IN PUNICODE_STRING PipeName,
               IN NTSTATUS Status,
               IN PLIST_ENTRY ListEntry)
{
    UNICODE_STRING DestinationString;
    KIRQL OldIrql;
    PWCHAR Buffer;

    Buffer = ExAllocatePoolWithTag(NonPagedPool, PipeName->Length, NPFS_WAIT_BLOCK_TAG);
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    RtlInitEmptyUnicodeString(&DestinationString, Buffer, PipeName->Length);
    RtlUpcaseUnicodeString(&DestinationString, PipeName, FALSE);

    OldIrql = KfAcquireSpinLock(&WaitQueue->WaitLock);

    ASSERT(IsListEmpty(&WaitQueue->WaitList) == TRUE);

    KfReleaseSpinLock(&WaitQueue->WaitLock, OldIrql);
    ExFreePool(DestinationString.Buffer);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpAddWaiter(IN PNP_WAIT_QUEUE WaitQueue,
            IN LARGE_INTEGER WaitTime,
            IN PIRP Irp, 
            IN PUNICODE_STRING Name)
{
    PIO_STACK_LOCATION IoStack;
    KIRQL OldIrql;
    NTSTATUS Status;
    PNP_WAIT_QUEUE_ENTRY WaitEntry;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitBuffer;
    LARGE_INTEGER DueTime;
    ULONG i;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    WaitEntry = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(*WaitEntry), NPFS_WRITE_BLOCK_TAG);
    if (WaitEntry)
    {
        KeInitializeDpc(&WaitEntry->Dpc, NpTimerDispatch, WaitEntry);
        KeInitializeTimer(&WaitEntry->Timer);

        if (Name)
        {
            WaitEntry->String = *Name;
        }
        else
        {
            WaitEntry->String.Length = 0;
            WaitEntry->String.Buffer = 0;
        }

        WaitEntry->WaitQueue = WaitQueue;
        WaitEntry->Irp = Irp;

        WaitBuffer = (PFILE_PIPE_WAIT_FOR_BUFFER)Irp->AssociatedIrp.SystemBuffer;
        if (WaitBuffer->TimeoutSpecified)
        {
            DueTime = WaitBuffer->Timeout;
        }
        else
        {
            DueTime = WaitTime;
        }

        for (i = 0; i < WaitBuffer->NameLength / sizeof(WCHAR); i++)
        {
            WaitBuffer->Name[i] = RtlUpcaseUnicodeChar(WaitBuffer->Name[i]);
        }

        Irp->Tail.Overlay.DriverContext[0] = WaitQueue;
        Irp->Tail.Overlay.DriverContext[1] = WaitEntry;
        OldIrql = KfAcquireSpinLock(&WaitQueue->WaitLock);

        IoSetCancelRoutine(Irp, NpCancelWaitQueueIrp);

        if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
        {
            Status = STATUS_CANCELLED;
        }
        else
        {
            InsertTailList(&WaitQueue->WaitList, &Irp->Tail.Overlay.ListEntry);

            IoMarkIrpPending(Irp);
            Status = STATUS_PENDING;

            WaitEntry->FileObject = IoStack->FileObject;
            ObfReferenceObject(WaitEntry->FileObject);

            KeSetTimer(&WaitEntry->Timer, DueTime, &WaitEntry->Dpc);
            WaitEntry = NULL;

        }
        KfReleaseSpinLock(&WaitQueue->WaitLock, OldIrql);
        if (WaitEntry) ExFreePool(WaitEntry);
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return Status;
}
