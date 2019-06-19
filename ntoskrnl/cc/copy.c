/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/copy.c
 * PURPOSE:         Implements cache managers copy interface
 *
 * PROGRAMMERS:     Some people?
 *                  Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static PFN_NUMBER CcZeroPage = 0;

#define MAX_ZERO_LENGTH    (256 * 1024)

typedef enum _CC_COPY_OPERATION
{
    CcOperationRead,
    CcOperationWrite,
    CcOperationZero
} CC_COPY_OPERATION;

typedef enum _CC_CAN_WRITE_RETRY
{
    FirstTry = 0,
    RetryAllowRemote = 253,
    RetryForceCheckPerFile = 254,
    RetryMasterLocked = 255,
} CC_CAN_WRITE_RETRY;

ULONG CcRosTraceLevel = 0;
ULONG CcFastMdlReadWait;
ULONG CcFastMdlReadNotPossible;
ULONG CcFastReadNotPossible;
ULONG CcFastReadWait;
ULONG CcFastReadNoWait;
ULONG CcFastReadResourceMiss;

/* Counters:
 * - Amount of pages flushed to the disk
 * - Number of flush operations
 */
ULONG CcDataPages = 0;
ULONG CcDataFlushes = 0;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
MiZeroPhysicalPage (
    IN PFN_NUMBER PageFrameIndex
);

VOID
NTAPI
CcInitCacheZeroPage (
    VOID)
{
    NTSTATUS Status;

    MI_SET_USAGE(MI_USAGE_CACHE);
    //MI_SET_PROCESS2(PsGetCurrentProcess()->ImageFileName);
    Status = MmRequestPageMemoryConsumer(MC_SYSTEM, TRUE, &CcZeroPage);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("Can't allocate CcZeroPage.\n");
        KeBugCheck(CACHE_MANAGER);
    }
    MiZeroPhysicalPage(CcZeroPage);
}

NTSTATUS
NTAPI
CcReadVirtualAddress (
    PROS_VACB Vacb)
{
    ULONG Size;
    PMDL Mdl;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    ULARGE_INTEGER LargeSize;

    LargeSize.QuadPart = Vacb->SharedCacheMap->SectionSize.QuadPart - Vacb->FileOffset.QuadPart;
    if (LargeSize.QuadPart > VACB_MAPPING_GRANULARITY)
    {
        LargeSize.QuadPart = VACB_MAPPING_GRANULARITY;
    }
    Size = LargeSize.LowPart;

    Size = ROUND_TO_PAGES(Size);
    ASSERT(Size <= VACB_MAPPING_GRANULARITY);
    ASSERT(Size > 0);

    Mdl = IoAllocateMdl(Vacb->BaseAddress, Size, FALSE, FALSE, NULL);
    if (!Mdl)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        MmProbeAndLockPages(Mdl, KernelMode, IoWriteAccess);
    }
    _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        DPRINT1("MmProbeAndLockPages failed with: %lx for %p (%p, %p)\n", Status, Mdl, Vacb, Vacb->BaseAddress);
        KeBugCheck(CACHE_MANAGER);
    } _SEH2_END;

    if (NT_SUCCESS(Status))
    {
        Mdl->MdlFlags |= MDL_IO_PAGE_READ;
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Status = IoPageRead(Vacb->SharedCacheMap->FileObject, Mdl, &Vacb->FileOffset, &Event, &IoStatus);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatus.Status;
        }

        MmUnlockPages(Mdl);
    }

    IoFreeMdl(Mdl);

    if (!NT_SUCCESS(Status) && (Status != STATUS_END_OF_FILE))
    {
        DPRINT1("IoPageRead failed, Status %x\n", Status);
        return Status;
    }

    if (Size < VACB_MAPPING_GRANULARITY)
    {
        RtlZeroMemory((char*)Vacb->BaseAddress + Size,
                      VACB_MAPPING_GRANULARITY - Size);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CcWriteVirtualAddress (
    PROS_VACB Vacb)
{
    ULONG Size;
    PMDL Mdl;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    ULARGE_INTEGER LargeSize;

    LargeSize.QuadPart = Vacb->SharedCacheMap->SectionSize.QuadPart - Vacb->FileOffset.QuadPart;
    if (LargeSize.QuadPart > VACB_MAPPING_GRANULARITY)
    {
        LargeSize.QuadPart = VACB_MAPPING_GRANULARITY;
    }
    Size = LargeSize.LowPart;
    //
    // Nonpaged pool PDEs in ReactOS must actually be synchronized between the
    // MmGlobalPageDirectory and the real system PDE directory. What a mess...
    //
    {
        ULONG i = 0;
        do
        {
            MmGetPfnForProcess(NULL, (PVOID)((ULONG_PTR)Vacb->BaseAddress + (i << PAGE_SHIFT)));
        } while (++i < (Size >> PAGE_SHIFT));
    }

    ASSERT(Size <= VACB_MAPPING_GRANULARITY);
    ASSERT(Size > 0);

    Mdl = IoAllocateMdl(Vacb->BaseAddress, Size, FALSE, FALSE, NULL);
    if (!Mdl)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
    }
    _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        DPRINT1("MmProbeAndLockPages failed with: %lx for %p (%p, %p)\n", Status, Mdl, Vacb, Vacb->BaseAddress);
        KeBugCheck(CACHE_MANAGER);
    } _SEH2_END;

    if (NT_SUCCESS(Status))
    {
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Status = IoSynchronousPageWrite(Vacb->SharedCacheMap->FileObject, Mdl, &Vacb->FileOffset, &Event, &IoStatus);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatus.Status;
        }

        MmUnlockPages(Mdl);
    }
    IoFreeMdl(Mdl);
    if (!NT_SUCCESS(Status) && (Status != STATUS_END_OF_FILE))
    {
        DPRINT1("IoPageWrite failed, Status %x\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ReadWriteOrZero(
    _Inout_ PVOID BaseAddress,
    _Inout_opt_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ CC_COPY_OPERATION Operation)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Operation == CcOperationZero)
    {
        /* Zero */
        RtlZeroMemory(BaseAddress, Length);
    }
    else
    {
        _SEH2_TRY
        {
            if (Operation == CcOperationWrite)
                RtlCopyMemory(BaseAddress, Buffer, Length);
            else
                RtlCopyMemory(Buffer, BaseAddress, Length);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    return Status;
}

BOOLEAN
CcCopyData (
    _In_ PFILE_OBJECT FileObject,
    _In_ LONGLONG FileOffset,
    _Inout_ PVOID Buffer,
    _In_ LONGLONG Length,
    _In_ CC_COPY_OPERATION Operation,
    _In_ BOOLEAN Wait,
    _Out_ PIO_STATUS_BLOCK IoStatus)
{
    NTSTATUS Status;
    LONGLONG CurrentOffset;
    ULONG BytesCopied;
    KIRQL OldIrql;
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    PLIST_ENTRY ListEntry;
    PROS_VACB Vacb;
    ULONG PartialLength;
    PVOID BaseAddress;
    BOOLEAN Valid;
    PPRIVATE_CACHE_MAP PrivateCacheMap;

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    PrivateCacheMap = FileObject->PrivateCacheMap;
    CurrentOffset = FileOffset;
    BytesCopied = 0;

    if (!Wait)
    {
        /* test if the requested data is available */
        KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &OldIrql);
        /* FIXME: this loop doesn't take into account areas that don't have
         * a VACB in the list yet */
        ListEntry = SharedCacheMap->CacheMapVacbListHead.Flink;
        while (ListEntry != &SharedCacheMap->CacheMapVacbListHead)
        {
            Vacb = CONTAINING_RECORD(ListEntry,
                                     ROS_VACB,
                                     CacheMapVacbListEntry);
            ListEntry = ListEntry->Flink;
            if (!Vacb->Valid &&
                DoRangesIntersect(Vacb->FileOffset.QuadPart,
                                  VACB_MAPPING_GRANULARITY,
                                  CurrentOffset, Length))
            {
                KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, OldIrql);
                /* data not available */
                return FALSE;
            }
            if (Vacb->FileOffset.QuadPart >= CurrentOffset + Length)
                break;
        }
        KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, OldIrql);
    }

    PartialLength = CurrentOffset % VACB_MAPPING_GRANULARITY;
    if (PartialLength != 0)
    {
        PartialLength = min(Length, VACB_MAPPING_GRANULARITY - PartialLength);
        Status = CcRosRequestVacb(SharedCacheMap,
                                  ROUND_DOWN(CurrentOffset,
                                             VACB_MAPPING_GRANULARITY),
                                  &BaseAddress,
                                  &Valid,
                                  &Vacb);
        if (!NT_SUCCESS(Status))
            ExRaiseStatus(Status);
        if (!Valid)
        {
            Status = CcReadVirtualAddress(Vacb);
            if (!NT_SUCCESS(Status))
            {
                CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
                ExRaiseStatus(Status);
            }
        }
        Status = ReadWriteOrZero((PUCHAR)BaseAddress + CurrentOffset % VACB_MAPPING_GRANULARITY,
                                 Buffer,
                                 PartialLength,
                                 Operation);

        CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, Operation != CcOperationRead, FALSE);

        if (!NT_SUCCESS(Status))
            ExRaiseStatus(STATUS_INVALID_USER_BUFFER);

        Length -= PartialLength;
        CurrentOffset += PartialLength;
        BytesCopied += PartialLength;

        if (Operation != CcOperationZero)
            Buffer = (PVOID)((ULONG_PTR)Buffer + PartialLength);
    }

    while (Length > 0)
    {
        ASSERT(CurrentOffset % VACB_MAPPING_GRANULARITY == 0);
        PartialLength = min(VACB_MAPPING_GRANULARITY, Length);
        Status = CcRosRequestVacb(SharedCacheMap,
                                  CurrentOffset,
                                  &BaseAddress,
                                  &Valid,
                                  &Vacb);
        if (!NT_SUCCESS(Status))
            ExRaiseStatus(Status);
        if (!Valid &&
            (Operation == CcOperationRead ||
             PartialLength < VACB_MAPPING_GRANULARITY))
        {
            Status = CcReadVirtualAddress(Vacb);
            if (!NT_SUCCESS(Status))
            {
                CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
                ExRaiseStatus(Status);
            }
        }
        Status = ReadWriteOrZero(BaseAddress, Buffer, PartialLength, Operation);

        CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, Operation != CcOperationRead, FALSE);

        if (!NT_SUCCESS(Status))
            ExRaiseStatus(STATUS_INVALID_USER_BUFFER);

        Length -= PartialLength;
        CurrentOffset += PartialLength;
        BytesCopied += PartialLength;

        if (Operation != CcOperationZero)
            Buffer = (PVOID)((ULONG_PTR)Buffer + PartialLength);
    }

    /* If that was a successful sync read operation, let's handle read ahead */
    if (Operation == CcOperationRead && Length == 0 && Wait)
    {
        /* If file isn't random access and next read may get us cross VACB boundary,
         * schedule next read
         */
        if (!BooleanFlagOn(FileObject->Flags, FO_RANDOM_ACCESS) &&
            (CurrentOffset - 1) / VACB_MAPPING_GRANULARITY != (CurrentOffset + BytesCopied - 1) / VACB_MAPPING_GRANULARITY)
        {
            CcScheduleReadAhead(FileObject, (PLARGE_INTEGER)&FileOffset, BytesCopied);
        }

        /* And update read history in private cache map */
        PrivateCacheMap->FileOffset1.QuadPart = PrivateCacheMap->FileOffset2.QuadPart;
        PrivateCacheMap->BeyondLastByte1.QuadPart = PrivateCacheMap->BeyondLastByte2.QuadPart;
        PrivateCacheMap->FileOffset2.QuadPart = FileOffset;
        PrivateCacheMap->BeyondLastByte2.QuadPart = FileOffset + BytesCopied;
    }

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = BytesCopied;
    return TRUE;
}

VOID
CcPostDeferredWrites(VOID)
{
    ULONG WrittenBytes;

    /* We'll try to write as much as we can */
    WrittenBytes = 0;
    while (TRUE)
    {
        KIRQL OldIrql;
        PLIST_ENTRY ListEntry;
        PDEFERRED_WRITE DeferredWrite;

        DeferredWrite = NULL;

        /* Lock our deferred writes list */
        KeAcquireSpinLock(&CcDeferredWriteSpinLock, &OldIrql);
        for (ListEntry = CcDeferredWrites.Flink;
             ListEntry != &CcDeferredWrites;
             ListEntry = ListEntry->Flink)
        {
            /* Extract an entry */
            DeferredWrite = CONTAINING_RECORD(ListEntry, DEFERRED_WRITE, DeferredWriteLinks);

            /* Compute the modified bytes, based on what we already wrote */
            WrittenBytes += DeferredWrite->BytesToWrite;
            /* We overflowed, give up */
            if (WrittenBytes < DeferredWrite->BytesToWrite)
            {
                DeferredWrite = NULL;
                break;
            }

            /* Check we can write */
            if (CcCanIWrite(DeferredWrite->FileObject, WrittenBytes, FALSE, RetryForceCheckPerFile))
            {
                /* We can, so remove it from the list and stop looking for entry */
                RemoveEntryList(&DeferredWrite->DeferredWriteLinks);
                break;
            }

            /* If we don't accept modified pages, stop here */
            if (!DeferredWrite->LimitModifiedPages)
            {
                DeferredWrite = NULL;
                break;
            }

            /* Reset count as nothing was written yet */
            WrittenBytes -= DeferredWrite->BytesToWrite;
            DeferredWrite = NULL;
        }
        KeReleaseSpinLock(&CcDeferredWriteSpinLock, OldIrql);

        /* Nothing to write found, give up */
        if (DeferredWrite == NULL)
        {
            break;
        }

        /* If we have an event, set it and quit */
        if (DeferredWrite->Event)
        {
            KeSetEvent(DeferredWrite->Event, IO_NO_INCREMENT, FALSE);
        }
        /* Otherwise, call the write routine and free the context */
        else
        {
            DeferredWrite->PostRoutine(DeferredWrite->Context1, DeferredWrite->Context2);
            ExFreePoolWithTag(DeferredWrite, 'CcDw');
        }
    }
}

VOID
CcPerformReadAhead(
    IN PFILE_OBJECT FileObject)
{
    NTSTATUS Status;
    LONGLONG CurrentOffset;
    KIRQL OldIrql;
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    PROS_VACB Vacb;
    ULONG PartialLength;
    PVOID BaseAddress;
    BOOLEAN Valid;
    ULONG Length;
    PPRIVATE_CACHE_MAP PrivateCacheMap;
    BOOLEAN Locked;

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    /* Critical:
     * PrivateCacheMap might disappear in-between if the handle
     * to the file is closed (private is attached to the handle not to
     * the file), so we need to lock the master lock while we deal with
     * it. It won't disappear without attempting to lock such lock.
     */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    PrivateCacheMap = FileObject->PrivateCacheMap;
    /* If the handle was closed since the read ahead was scheduled, just quit */
    if (PrivateCacheMap == NULL)
    {
        KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
        ObDereferenceObject(FileObject);
        return;
    }
    /* Otherwise, extract read offset and length and release private map */
    else
    {
        KeAcquireSpinLockAtDpcLevel(&PrivateCacheMap->ReadAheadSpinLock);
        CurrentOffset = PrivateCacheMap->ReadAheadOffset[1].QuadPart;
        Length = PrivateCacheMap->ReadAheadLength[1];
        KeReleaseSpinLockFromDpcLevel(&PrivateCacheMap->ReadAheadSpinLock);
    }
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

    /* Time to go! */
    DPRINT("Doing ReadAhead for %p\n", FileObject);
    /* Lock the file, first */
    if (!SharedCacheMap->Callbacks->AcquireForReadAhead(SharedCacheMap->LazyWriteContext, FALSE))
    {
        Locked = FALSE;
        goto Clear;
    }

    /* Remember it's locked */
    Locked = TRUE;

    /* Don't read past the end of the file */
    if (CurrentOffset >= SharedCacheMap->FileSize.QuadPart)
    {
        goto Clear;
    }
    if (CurrentOffset + Length > SharedCacheMap->FileSize.QuadPart)
    {
        Length = SharedCacheMap->FileSize.QuadPart - CurrentOffset;
    }

    /* Next of the algorithm will lock like CcCopyData with the slight
     * difference that we don't copy data back to an user-backed buffer
     * We just bring data into Cc
     */
    PartialLength = CurrentOffset % VACB_MAPPING_GRANULARITY;
    if (PartialLength != 0)
    {
        PartialLength = min(Length, VACB_MAPPING_GRANULARITY - PartialLength);
        Status = CcRosRequestVacb(SharedCacheMap,
                                  ROUND_DOWN(CurrentOffset,
                                             VACB_MAPPING_GRANULARITY),
                                  &BaseAddress,
                                  &Valid,
                                  &Vacb);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to request VACB: %lx!\n", Status);
            goto Clear;
        }

        if (!Valid)
        {
            Status = CcReadVirtualAddress(Vacb);
            if (!NT_SUCCESS(Status))
            {
                CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
                DPRINT1("Failed to read data: %lx!\n", Status);
                goto Clear;
            }
        }

        CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, FALSE, FALSE);

        Length -= PartialLength;
        CurrentOffset += PartialLength;
    }

    while (Length > 0)
    {
        ASSERT(CurrentOffset % VACB_MAPPING_GRANULARITY == 0);
        PartialLength = min(VACB_MAPPING_GRANULARITY, Length);
        Status = CcRosRequestVacb(SharedCacheMap,
                                  CurrentOffset,
                                  &BaseAddress,
                                  &Valid,
                                  &Vacb);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to request VACB: %lx!\n", Status);
            goto Clear;
        }

        if (!Valid)
        {
            Status = CcReadVirtualAddress(Vacb);
            if (!NT_SUCCESS(Status))
            {
                CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
                DPRINT1("Failed to read data: %lx!\n", Status);
                goto Clear;
            }
        }

        CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, FALSE, FALSE);

        Length -= PartialLength;
        CurrentOffset += PartialLength;
    }

Clear:
    /* See previous comment about private cache map */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    PrivateCacheMap = FileObject->PrivateCacheMap;
    if (PrivateCacheMap != NULL)
    {
        /* Mark read ahead as unactive */
        KeAcquireSpinLockAtDpcLevel(&PrivateCacheMap->ReadAheadSpinLock);
        InterlockedAnd((volatile long *)&PrivateCacheMap->UlongFlags, ~PRIVATE_CACHE_MAP_READ_AHEAD_ACTIVE);
        KeReleaseSpinLockFromDpcLevel(&PrivateCacheMap->ReadAheadSpinLock);
    }
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

    /* If file was locked, release it */
    if (Locked)
    {
        SharedCacheMap->Callbacks->ReleaseFromReadAhead(SharedCacheMap->LazyWriteContext);
    }

    /* And drop our extra reference (See: CcScheduleReadAhead) */
    ObDereferenceObject(FileObject);

    return;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
CcCanIWrite (
    IN PFILE_OBJECT FileObject,
    IN ULONG BytesToWrite,
    IN BOOLEAN Wait,
    IN BOOLEAN Retrying)
{
    KIRQL OldIrql;
    KEVENT WaitEvent;
    ULONG Length, Pages;
    BOOLEAN PerFileDefer;
    DEFERRED_WRITE Context;
    PFSRTL_COMMON_FCB_HEADER Fcb;
    CC_CAN_WRITE_RETRY TryContext;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    CCTRACE(CC_API_DEBUG, "FileObject=%p BytesToWrite=%lu Wait=%d Retrying=%d\n",
        FileObject, BytesToWrite, Wait, Retrying);

    /* Write through is always OK */
    if (BooleanFlagOn(FileObject->Flags, FO_WRITE_THROUGH))
    {
        return TRUE;
    }

    TryContext = Retrying;
    /* Allow remote file if not from posted */
    if (IoIsFileOriginRemote(FileObject) && TryContext < RetryAllowRemote)
    {
        return TRUE;
    }

    /* Don't exceed max tolerated size */
    Length = MAX_ZERO_LENGTH;
    if (BytesToWrite < MAX_ZERO_LENGTH)
    {
        Length = BytesToWrite;
    }

    Pages = BYTES_TO_PAGES(Length);

    /* By default, assume limits per file won't be hit */
    PerFileDefer = FALSE;
    Fcb = FileObject->FsContext;
    /* Do we have to check for limits per file? */
    if (TryContext >= RetryForceCheckPerFile ||
        BooleanFlagOn(Fcb->Flags, FSRTL_FLAG_LIMIT_MODIFIED_PAGES))
    {
        /* If master is not locked, lock it now */
        if (TryContext != RetryMasterLocked)
        {
            OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
        }

        /* Let's not assume the file is cached... */
        if (FileObject->SectionObjectPointer != NULL &&
            FileObject->SectionObjectPointer->SharedCacheMap != NULL)
        {
            SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
            /* Do we have limits per file set? */
            if (SharedCacheMap->DirtyPageThreshold != 0 &&
                SharedCacheMap->DirtyPages != 0)
            {
                /* Yes, check whether they are blocking */
                if (Pages + SharedCacheMap->DirtyPages > SharedCacheMap->DirtyPageThreshold)
                {
                    PerFileDefer = TRUE;
                }
            }
        }

        /* And don't forget to release master */
        if (TryContext != RetryMasterLocked)
        {
            KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
        }
    }

    /* So, now allow write if:
     * - Not the first try or we have no throttling yet
     * AND:
     * - We don't exceed threshold!
     * - We don't exceed what Mm can allow us to use
     *   + If we're above top, that's fine
     *   + If we're above bottom with limited modified pages, that's fine
     *   + Otherwise, throttle!
     */
    if ((TryContext != FirstTry || IsListEmpty(&CcDeferredWrites)) &&
        CcTotalDirtyPages + Pages < CcDirtyPageThreshold &&
        (MmAvailablePages > MmThrottleTop ||
         (MmModifiedPageListHead.Total < 1000 && MmAvailablePages > MmThrottleBottom)) &&
        !PerFileDefer)
    {
        return TRUE;
    }

    /* If we can wait, we'll start the wait loop for waiting till we can
     * write for real
     */
    if (!Wait)
    {
        return FALSE;
    }

    /* Otherwise, if there are no deferred writes yet, start the lazy writer */
    if (IsListEmpty(&CcDeferredWrites))
    {
        KIRQL OldIrql;

        OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
        CcScheduleLazyWriteScan(TRUE);
        KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
    }

    /* Initialize our wait event */
    KeInitializeEvent(&WaitEvent, NotificationEvent, FALSE);

    /* And prepare a dummy context */
    Context.NodeTypeCode = NODE_TYPE_DEFERRED_WRITE;
    Context.NodeByteSize = sizeof(DEFERRED_WRITE);
    Context.FileObject = FileObject;
    Context.BytesToWrite = BytesToWrite;
    Context.LimitModifiedPages = BooleanFlagOn(Fcb->Flags, FSRTL_FLAG_LIMIT_MODIFIED_PAGES);
    Context.Event = &WaitEvent;

    /* And queue it */
    if (Retrying)
    {
        /* To the top, if that's a retry */
        ExInterlockedInsertHeadList(&CcDeferredWrites,
                                    &Context.DeferredWriteLinks,
                                    &CcDeferredWriteSpinLock);
    }
    else
    {
        /* To the bottom, if that's a first time */
        ExInterlockedInsertTailList(&CcDeferredWrites,
                                    &Context.DeferredWriteLinks,
                                    &CcDeferredWriteSpinLock);
    }

    DPRINT1("Actively deferring write for: %p\n", FileObject);
    /* Now, we'll loop until our event is set. When it is set, it means that caller
     * can immediately write, and has to
     */
    do
    {
        CcPostDeferredWrites();
    } while (KeWaitForSingleObject(&WaitEvent, Executive, KernelMode, FALSE, &CcIdleDelay) != STATUS_SUCCESS);

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
CcCopyRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus)
{
    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%I64d Length=%lu Wait=%d\n",
        FileObject, FileOffset->QuadPart, Length, Wait);

    DPRINT("CcCopyRead(FileObject 0x%p, FileOffset %I64x, "
           "Length %lu, Wait %u, Buffer 0x%p, IoStatus 0x%p)\n",
           FileObject, FileOffset->QuadPart, Length, Wait,
           Buffer, IoStatus);

    return CcCopyData(FileObject,
                      FileOffset->QuadPart,
                      Buffer,
                      Length,
                      CcOperationRead,
                      Wait,
                      IoStatus);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
CcCopyWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN PVOID Buffer)
{
    IO_STATUS_BLOCK IoStatus;

    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%I64d Length=%lu Wait=%d Buffer=%p\n",
        FileObject, FileOffset->QuadPart, Length, Wait, Buffer);

    DPRINT("CcCopyWrite(FileObject 0x%p, FileOffset %I64x, "
           "Length %lu, Wait %u, Buffer 0x%p)\n",
           FileObject, FileOffset->QuadPart, Length, Wait, Buffer);

    return CcCopyData(FileObject,
                      FileOffset->QuadPart,
                      Buffer,
                      Length,
                      CcOperationWrite,
                      Wait,
                      &IoStatus);
}

/*
 * @implemented
 */
VOID
NTAPI
CcDeferWrite (
    IN PFILE_OBJECT FileObject,
    IN PCC_POST_DEFERRED_WRITE PostRoutine,
    IN PVOID Context1,
    IN PVOID Context2,
    IN ULONG BytesToWrite,
    IN BOOLEAN Retrying)
{
    KIRQL OldIrql;
    PDEFERRED_WRITE Context;
    PFSRTL_COMMON_FCB_HEADER Fcb;

    CCTRACE(CC_API_DEBUG, "FileObject=%p PostRoutine=%p Context1=%p Context2=%p BytesToWrite=%lu Retrying=%d\n",
        FileObject, PostRoutine, Context1, Context2, BytesToWrite, Retrying);

    /* Try to allocate a context for queueing the write operation */
    Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEFERRED_WRITE), 'CcDw');
    /* If it failed, immediately execute the operation! */
    if (Context == NULL)
    {
        PostRoutine(Context1, Context2);
        return;
    }

    Fcb = FileObject->FsContext;

    /* Otherwise, initialize the context */
    RtlZeroMemory(Context, sizeof(DEFERRED_WRITE));
    Context->NodeTypeCode = NODE_TYPE_DEFERRED_WRITE;
    Context->NodeByteSize = sizeof(DEFERRED_WRITE);
    Context->FileObject = FileObject;
    Context->PostRoutine = PostRoutine;
    Context->Context1 = Context1;
    Context->Context2 = Context2;
    Context->BytesToWrite = BytesToWrite;
    Context->LimitModifiedPages = BooleanFlagOn(Fcb->Flags, FSRTL_FLAG_LIMIT_MODIFIED_PAGES);

    /* And queue it */
    if (Retrying)
    {
        /* To the top, if that's a retry */
        ExInterlockedInsertHeadList(&CcDeferredWrites,
                                    &Context->DeferredWriteLinks,
                                    &CcDeferredWriteSpinLock);
    }
    else
    {
        /* To the bottom, if that's a first time */
        ExInterlockedInsertTailList(&CcDeferredWrites,
                                    &Context->DeferredWriteLinks,
                                    &CcDeferredWriteSpinLock);
    }

    /* Try to execute the posted writes */
    CcPostDeferredWrites();

    /* Schedule a lazy writer run to handle deferred writes */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    if (!LazyWriter.ScanActive)
    {
        CcScheduleLazyWriteScan(FALSE);
    }
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcFastCopyRead (
    IN PFILE_OBJECT FileObject,
    IN ULONG FileOffset,
    IN ULONG Length,
    IN ULONG PageCount,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus)
{
    LARGE_INTEGER LargeFileOffset;
    BOOLEAN Success;

    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%lu Length=%lu PageCount=%lu Buffer=%p\n",
        FileObject, FileOffset, Length, PageCount, Buffer);

    DBG_UNREFERENCED_PARAMETER(PageCount);

    LargeFileOffset.QuadPart = FileOffset;
    Success = CcCopyRead(FileObject,
                         &LargeFileOffset,
                         Length,
                         TRUE,
                         Buffer,
                         IoStatus);
    ASSERT(Success == TRUE);
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcFastCopyWrite (
    IN PFILE_OBJECT FileObject,
    IN ULONG FileOffset,
    IN ULONG Length,
    IN PVOID Buffer)
{
    LARGE_INTEGER LargeFileOffset;
    BOOLEAN Success;

    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%lu Length=%lu Buffer=%p\n",
        FileObject, FileOffset, Length, Buffer);

    LargeFileOffset.QuadPart = FileOffset;
    Success = CcCopyWrite(FileObject,
                          &LargeFileOffset,
                          Length,
                          TRUE,
                          Buffer);
    ASSERT(Success == TRUE);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
CcZeroData (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER StartOffset,
    IN PLARGE_INTEGER EndOffset,
    IN BOOLEAN Wait)
{
    NTSTATUS Status;
    LARGE_INTEGER WriteOffset;
    LONGLONG Length;
    ULONG CurrentLength;
    PMDL Mdl;
    ULONG i;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;

    CCTRACE(CC_API_DEBUG, "FileObject=%p StartOffset=%I64u EndOffset=%I64u Wait=%d\n",
        FileObject, StartOffset->QuadPart, EndOffset->QuadPart, Wait);

    DPRINT("CcZeroData(FileObject 0x%p, StartOffset %I64x, EndOffset %I64x, "
           "Wait %u)\n", FileObject, StartOffset->QuadPart, EndOffset->QuadPart,
           Wait);

    Length = EndOffset->QuadPart - StartOffset->QuadPart;
    WriteOffset.QuadPart = StartOffset->QuadPart;

    if (FileObject->SectionObjectPointer->SharedCacheMap == NULL)
    {
        /* File is not cached */

        Mdl = _alloca(MmSizeOfMdl(NULL, MAX_ZERO_LENGTH));

        while (Length > 0)
        {
            if (Length + WriteOffset.QuadPart % PAGE_SIZE > MAX_ZERO_LENGTH)
            {
                CurrentLength = MAX_ZERO_LENGTH - WriteOffset.QuadPart % PAGE_SIZE;
            }
            else
            {
                CurrentLength = Length;
            }
            MmInitializeMdl(Mdl, (PVOID)(ULONG_PTR)WriteOffset.QuadPart, CurrentLength);
            Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
            for (i = 0; i < ((Mdl->Size - sizeof(MDL)) / sizeof(ULONG)); i++)
            {
                ((PPFN_NUMBER)(Mdl + 1))[i] = CcZeroPage;
            }
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            Status = IoSynchronousPageWrite(FileObject, Mdl, &WriteOffset, &Event, &Iosb);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = Iosb.Status;
            }
            if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
            {
                MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
            }
            if (!NT_SUCCESS(Status))
            {
                return FALSE;
            }
            WriteOffset.QuadPart += CurrentLength;
            Length -= CurrentLength;
        }
    }
    else
    {
        IO_STATUS_BLOCK IoStatus;

        return CcCopyData(FileObject,
                          WriteOffset.QuadPart,
                          NULL,
                          Length,
                          CcOperationZero,
                          Wait,
                          &IoStatus);
    }

    return TRUE;
}
