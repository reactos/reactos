/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/waitsup.c
 * PURPOSE:     Pipes Waiting Support
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_WAITSUP)

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
NpCancelWaitQueueIrp(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    KIRQL OldIrql;
    PNP_WAIT_QUEUE_ENTRY WaitEntry;
    PNP_WAIT_QUEUE WaitQueue;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    WaitQueue = Irp->Tail.Overlay.DriverContext[0];

    KeAcquireSpinLock(&WaitQueue->WaitLock, &OldIrql);

    WaitEntry = Irp->Tail.Overlay.DriverContext[1];
    if (WaitEntry)
    {
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
        if (!KeCancelTimer(&WaitEntry->Timer))
        {
            WaitEntry->Irp = NULL;
            WaitEntry = NULL;
        }
    }

    KeReleaseSpinLock(&WaitQueue->WaitLock, OldIrql);

    if (WaitEntry)
    {
        ObDereferenceObject(WaitEntry->FileObject);
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

    KeAcquireSpinLock(&WaitEntry->WaitQueue->WaitLock, &OldIrql);

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

    KeReleaseSpinLock(&WaitEntry->WaitQueue->WaitLock, OldIrql);

    if (Irp)
    {
        Irp->IoStatus.Status = STATUS_IO_TIMEOUT;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    ObDereferenceObject(WaitEntry->FileObject);
    ExFreePool(WaitEntry);
}

VOID
NTAPI
NpInitializeWaitQueue(IN PNP_WAIT_QUEUE WaitQueue)
{
    InitializeListHead(&WaitQueue->WaitList);
    KeInitializeSpinLock(&WaitQueue->WaitLock);
}

static
BOOLEAN
NpEqualUnicodeString(IN PCUNICODE_STRING String1,
                     IN PCUNICODE_STRING String2)
{
    SIZE_T EqualLength;

    if (String1->Length != String2->Length)
        return FALSE;

    EqualLength = RtlCompareMemory(String1->Buffer,
                                   String2->Buffer,
                                   String1->Length);
    return EqualLength == String1->Length;
}

NTSTATUS
NTAPI
NpCancelWaiter(IN PNP_WAIT_QUEUE WaitQueue,
               IN PUNICODE_STRING PipePath,
               IN NTSTATUS Status,
               IN PLIST_ENTRY List)
{
    UNICODE_STRING PipePathUpper;
    KIRQL OldIrql;
    PWCHAR Buffer;
    PLIST_ENTRY NextEntry;
    PNP_WAIT_QUEUE_ENTRY WaitEntry, Linkage;
    PIRP WaitIrp;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitBuffer;
    UNICODE_STRING WaitName, PipeName;

    Linkage = NULL;

    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   PipePath->Length,
                                   NPFS_WAIT_BLOCK_TAG);
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    RtlInitEmptyUnicodeString(&PipePathUpper, Buffer, PipePath->Length);
    RtlUpcaseUnicodeString(&PipePathUpper, PipePath, FALSE);

    KeAcquireSpinLock(&WaitQueue->WaitLock, &OldIrql);

    NextEntry = WaitQueue->WaitList.Flink;
    while (NextEntry != &WaitQueue->WaitList)
    {
        WaitIrp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);
        NextEntry = NextEntry->Flink;
        WaitEntry = WaitIrp->Tail.Overlay.DriverContext[1];

        if (WaitEntry->AliasName.Length)
        {
            ASSERT(FALSE);
            /* We have an alias. Use that for comparison */
            WaitName = WaitEntry->AliasName;
            PipeName = PipePathUpper;
        }
        else
        {
            /* Use the name from the wait buffer to compare */
            WaitBuffer = WaitIrp->AssociatedIrp.SystemBuffer;
            WaitName.Buffer = WaitBuffer->Name;
            WaitName.Length = WaitBuffer->NameLength;
            WaitName.MaximumLength = WaitName.Length;

            /* WaitName doesn't have a leading backslash,
             * so skip the one in PipePathUpper for the comparison */
            PipeName.Buffer = PipePathUpper.Buffer + 1;
            PipeName.Length = PipePathUpper.Length - sizeof(WCHAR);
            PipeName.MaximumLength = PipeName.Length;
        }

        /* Can't use RtlEqualUnicodeString with a spinlock held */
        if (NpEqualUnicodeString(&WaitName, &PipeName))
        {
            /* Found a matching wait. Cancel it */
            RemoveEntryList(&WaitIrp->Tail.Overlay.ListEntry);
            if (KeCancelTimer(&WaitEntry->Timer))
            {
                WaitEntry->WaitQueue = (PNP_WAIT_QUEUE)Linkage;
                Linkage = WaitEntry;
            }
            else
            {
                WaitEntry->Irp = NULL;
                WaitIrp->Tail.Overlay.DriverContext[1] = NULL;
            }

            if (IoSetCancelRoutine(WaitIrp, NULL))
            {
                WaitIrp->IoStatus.Information = 0;
                WaitIrp->IoStatus.Status = Status;
                InsertTailList(List, &WaitIrp->Tail.Overlay.ListEntry);
            }
            else
            {
                WaitIrp->Tail.Overlay.DriverContext[1] = NULL;
            }
        }
    }

    KeReleaseSpinLock(&WaitQueue->WaitLock, OldIrql);

    ExFreePoolWithTag(Buffer, NPFS_WAIT_BLOCK_TAG);

    while (Linkage)
    {
        WaitEntry = Linkage;
        Linkage = (PNP_WAIT_QUEUE_ENTRY)Linkage->WaitQueue;
        ObDereferenceObject(WaitEntry->FileObject);
        ExFreePool(WaitEntry);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpAddWaiter(IN PNP_WAIT_QUEUE WaitQueue,
            IN LARGE_INTEGER WaitTime,
            IN PIRP Irp,
            IN PUNICODE_STRING AliasName)
{
    PIO_STACK_LOCATION IoStack;
    KIRQL OldIrql;
    NTSTATUS Status;
    PNP_WAIT_QUEUE_ENTRY WaitEntry;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitBuffer;
    LARGE_INTEGER DueTime;
    ULONG i;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    WaitEntry = ExAllocatePoolWithQuotaTag(NonPagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                           sizeof(*WaitEntry),
                                           NPFS_WRITE_BLOCK_TAG);
    if (!WaitEntry)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeDpc(&WaitEntry->Dpc, NpTimerDispatch, WaitEntry);
    KeInitializeTimer(&WaitEntry->Timer);

    if (AliasName)
    {
        WaitEntry->AliasName = *AliasName;
    }
    else
    {
        WaitEntry->AliasName.Length = 0;
        WaitEntry->AliasName.Buffer = NULL;
    }

    WaitEntry->WaitQueue = WaitQueue;
    WaitEntry->Irp = Irp;

    WaitBuffer = Irp->AssociatedIrp.SystemBuffer;
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

    KeAcquireSpinLock(&WaitQueue->WaitLock, &OldIrql);

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
        ObReferenceObject(WaitEntry->FileObject);

        KeSetTimer(&WaitEntry->Timer, DueTime, &WaitEntry->Dpc);
        WaitEntry = NULL;
    }

    KeReleaseSpinLock(&WaitQueue->WaitLock, OldIrql);
    if (WaitEntry) ExFreePool(WaitEntry);

    return Status;
}

/* EOF */
