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

typedef enum _CC_CAN_WRITE_RETRY
{
    FirstTry = 0,
    RetryAllowRemote = 253,
    RetryForceCheckPerFile = 254,
    RetryMasterLocked = 255,
} CC_CAN_WRITE_RETRY;

ULONG CcRosTraceLevel = CC_API_DEBUG;
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

VOID
CcPostDeferredWrites(VOID)
{
    LIST_ENTRY ToInsertBack;

    InitializeListHead(&ToInsertBack);

    /* We'll try to write as much as we can */
    while (TRUE)
    {
        PDEFERRED_WRITE DeferredWrite;
        PLIST_ENTRY ListEntry;

        DeferredWrite = NULL;

        ListEntry = ExInterlockedRemoveHeadList(&CcDeferredWrites, &CcDeferredWriteSpinLock);

        if (!ListEntry)
            break;

        DeferredWrite = CONTAINING_RECORD(ListEntry, DEFERRED_WRITE, DeferredWriteLinks);

        /* Check if we can write */
        if (CcCanIWrite(DeferredWrite->FileObject, DeferredWrite->BytesToWrite, FALSE, RetryForceCheckPerFile))
        {
            /* If we have an event, set it and go along */
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
            continue;
        }

        /* Keep it for later */
        InsertHeadList(&ToInsertBack, &DeferredWrite->DeferredWriteLinks);

        /* If we don't accept modified pages, stop here */
        if (!DeferredWrite->LimitModifiedPages)
        {
            break;
        }
    }

    /* Insert what we couldn't write back in the list */
    while (!IsListEmpty(&ToInsertBack))
    {
        PLIST_ENTRY ListEntry = RemoveTailList(&ToInsertBack);
        ExInterlockedInsertHeadList(&CcDeferredWrites, ListEntry, &CcDeferredWriteSpinLock);
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
    ULONG Length;
    PPRIVATE_CACHE_MAP PrivateCacheMap;
    BOOLEAN Locked;
    BOOLEAN Success;

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
                                  ROUND_DOWN(CurrentOffset, VACB_MAPPING_GRANULARITY),
                                  &Vacb);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to request VACB: %lx!\n", Status);
            goto Clear;
        }

        _SEH2_TRY
        {
            Success = CcRosEnsureVacbResident(Vacb, TRUE, FALSE,
                    CurrentOffset % VACB_MAPPING_GRANULARITY, PartialLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Success = FALSE;
        }
        _SEH2_END

        if (!Success)
        {
            CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE);
            DPRINT1("Failed to read data: %lx!\n", Status);
            goto Clear;
        }

        CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE);

        Length -= PartialLength;
        CurrentOffset += PartialLength;
    }

    while (Length > 0)
    {
        ASSERT(CurrentOffset % VACB_MAPPING_GRANULARITY == 0);
        PartialLength = min(VACB_MAPPING_GRANULARITY, Length);
        Status = CcRosRequestVacb(SharedCacheMap,
                                  CurrentOffset,
                                  &Vacb);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to request VACB: %lx!\n", Status);
            goto Clear;
        }

        _SEH2_TRY
        {
            Success = CcRosEnsureVacbResident(Vacb, TRUE, FALSE, 0, PartialLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Success = FALSE;
        }
        _SEH2_END

        if (!Success)
        {
            CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE);
            DPRINT1("Failed to read data: %lx!\n", Status);
            goto Clear;
        }

        CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE);

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

#if DBG
    DPRINT1("Actively deferring write for: %p\n", FileObject);
    DPRINT1("Because:\n");
    if (CcTotalDirtyPages + Pages >= CcDirtyPageThreshold)
        DPRINT1("    There are too many cache dirty pages: %x + %x >= %x\n", CcTotalDirtyPages, Pages, CcDirtyPageThreshold);
    if (MmAvailablePages <= MmThrottleTop)
        DPRINT1("    Available pages are below throttle top: %lx <= %lx\n", MmAvailablePages, MmThrottleTop);
    if (MmModifiedPageListHead.Total >= 1000)
        DPRINT1("    There are too many modified pages: %lu >= 1000\n", MmModifiedPageListHead.Total);
    if (MmAvailablePages <= MmThrottleBottom)
        DPRINT1("    Available pages are below throttle bottom: %lx <= %lx\n", MmAvailablePages, MmThrottleBottom);
#endif
    /* Now, we'll loop until our event is set. When it is set, it means that caller
     * can immediately write, and has to
     */
    do
    {
        CcPostDeferredWrites();
    } while (KeWaitForSingleObject(&WaitEvent, Executive, KernelMode, FALSE, &CcIdleDelay) != STATUS_SUCCESS);

    return TRUE;
}

static
int
CcpCheckInvalidUserBuffer(PEXCEPTION_POINTERS Except, PVOID Buffer, ULONG Length)
{
    ULONG_PTR ExceptionAddress;
    ULONG_PTR BeginAddress = (ULONG_PTR)Buffer;
    ULONG_PTR EndAddress = (ULONG_PTR)Buffer + Length;

    if (Except->ExceptionRecord->ExceptionCode != STATUS_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    if (Except->ExceptionRecord->NumberParameters < 2)
        return EXCEPTION_CONTINUE_SEARCH;

    ExceptionAddress = Except->ExceptionRecord->ExceptionInformation[1];
    if ((ExceptionAddress >= BeginAddress) && (ExceptionAddress < EndAddress))
        return EXCEPTION_EXECUTE_HANDLER;

    return EXCEPTION_CONTINUE_SEARCH;
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
    PROS_VACB Vacb;
    PROS_SHARED_CACHE_MAP SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    NTSTATUS Status;
    LONGLONG CurrentOffset;
    LONGLONG ReadEnd = FileOffset->QuadPart + Length;
    ULONG ReadLength = 0;

    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%I64d Length=%lu Wait=%d\n",
        FileObject, FileOffset->QuadPart, Length, Wait);

    DPRINT("CcCopyRead(FileObject 0x%p, FileOffset %I64x, "
           "Length %lu, Wait %u, Buffer 0x%p, IoStatus 0x%p)\n",
           FileObject, FileOffset->QuadPart, Length, Wait,
           Buffer, IoStatus);

    if (!SharedCacheMap)
        return FALSE;

    /* Documented to ASSERT, but KMTests test this case... */
    // ASSERT((FileOffset->QuadPart + Length) <= SharedCacheMap->FileSize.QuadPart);

    CurrentOffset = FileOffset->QuadPart;
    while(CurrentOffset < ReadEnd)
    {
        Status = CcRosGetVacb(SharedCacheMap, CurrentOffset, &Vacb);
        if (!NT_SUCCESS(Status))
        {
            ExRaiseStatus(Status);
            return FALSE;
        }

        _SEH2_TRY
        {
            ULONG VacbOffset = CurrentOffset % VACB_MAPPING_GRANULARITY;
            ULONG VacbLength = min(Length, VACB_MAPPING_GRANULARITY - VacbOffset);
            SIZE_T CopyLength = VacbLength;

            if (!CcRosEnsureVacbResident(Vacb, Wait, FALSE, VacbOffset, VacbLength))
                return FALSE;

            _SEH2_TRY
            {
                RtlCopyMemory(Buffer, (PUCHAR)Vacb->BaseAddress + VacbOffset, CopyLength);
            }
            _SEH2_EXCEPT(CcpCheckInvalidUserBuffer(_SEH2_GetExceptionInformation(), Buffer, VacbLength))
            {
                ExRaiseStatus(STATUS_INVALID_USER_BUFFER);
            }
            _SEH2_END;

            ReadLength += VacbLength;

            Buffer = (PVOID)((ULONG_PTR)Buffer + VacbLength);
            CurrentOffset += VacbLength;
            Length -= VacbLength;
        }
        _SEH2_FINALLY
        {
            CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE);
        }
        _SEH2_END;
    }

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = ReadLength;

#if 0
    /* If that was a successful sync read operation, let's handle read ahead */
    if (Length == 0 && Wait)
    {
        PPRIVATE_CACHE_MAP PrivateCacheMap = FileObject->PrivateCacheMap;

        /* If file isn't random access and next read may get us cross VACB boundary,
         * schedule next read
         */
        if (!BooleanFlagOn(FileObject->Flags, FO_RANDOM_ACCESS) &&
            (CurrentOffset - 1) / VACB_MAPPING_GRANULARITY != (CurrentOffset + ReadLength - 1) / VACB_MAPPING_GRANULARITY)
        {
            CcScheduleReadAhead(FileObject, FileOffset, ReadLength);
        }

        /* And update read history in private cache map */
        PrivateCacheMap->FileOffset1.QuadPart = PrivateCacheMap->FileOffset2.QuadPart;
        PrivateCacheMap->BeyondLastByte1.QuadPart = PrivateCacheMap->BeyondLastByte2.QuadPart;
        PrivateCacheMap->FileOffset2.QuadPart = FileOffset->QuadPart;
        PrivateCacheMap->BeyondLastByte2.QuadPart = FileOffset->QuadPart + ReadLength;
    }
#endif

    return TRUE;
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
    PROS_VACB Vacb;
    PROS_SHARED_CACHE_MAP SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    NTSTATUS Status;
    LONGLONG CurrentOffset;
    LONGLONG WriteEnd;

    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%I64d Length=%lu Wait=%d Buffer=%p\n",
        FileObject, FileOffset->QuadPart, Length, Wait, Buffer);

    DPRINT("CcCopyWrite(FileObject 0x%p, FileOffset %I64x, "
           "Length %lu, Wait %u, Buffer 0x%p)\n",
           FileObject, FileOffset->QuadPart, Length, Wait, Buffer);

    if (!SharedCacheMap)
        return FALSE;

    Status = RtlLongLongAdd(FileOffset->QuadPart, Length, &WriteEnd);
    if (!NT_SUCCESS(Status))
        ExRaiseStatus(Status);

    ASSERT(WriteEnd <= SharedCacheMap->SectionSize.QuadPart);

    CurrentOffset = FileOffset->QuadPart;
    while(CurrentOffset < WriteEnd)
    {
        ULONG VacbOffset = CurrentOffset % VACB_MAPPING_GRANULARITY;
        ULONG VacbLength = min(WriteEnd - CurrentOffset, VACB_MAPPING_GRANULARITY - VacbOffset);

        Status = CcRosGetVacb(SharedCacheMap, CurrentOffset, &Vacb);
        if (!NT_SUCCESS(Status))
        {
            ExRaiseStatus(Status);
            return FALSE;
        }

        _SEH2_TRY
        {
            if (!CcRosEnsureVacbResident(Vacb, Wait, FALSE, VacbOffset, VacbLength))
            {
                return FALSE;
            }

            _SEH2_TRY
            {
                RtlCopyMemory((PVOID)((ULONG_PTR)Vacb->BaseAddress + VacbOffset), Buffer, VacbLength);
            }
            _SEH2_EXCEPT(CcpCheckInvalidUserBuffer(_SEH2_GetExceptionInformation(), Buffer, VacbLength))
            {
                ExRaiseStatus(STATUS_INVALID_USER_BUFFER);
            }
            _SEH2_END;

            Buffer = (PVOID)((ULONG_PTR)Buffer + VacbLength);
            CurrentOffset += VacbLength;

            /* Tell Mm */
            Status = MmMakeSegmentDirty(FileObject->SectionObjectPointer,
                                        Vacb->FileOffset.QuadPart + VacbOffset,
                                        VacbLength);
            if (!NT_SUCCESS(Status))
                ExRaiseStatus(Status);
        }
        _SEH2_FINALLY
        {
            /* Do not mark the VACB as dirty if an exception was raised */
            CcRosReleaseVacb(SharedCacheMap, Vacb, !_SEH2_AbnormalTermination(), FALSE);
        }
        _SEH2_END;
    }

    /* Flush if needed */
    if (FileObject->Flags & FO_WRITE_THROUGH)
        CcFlushCache(FileObject->SectionObjectPointer, FileOffset, Length, NULL);

    return TRUE;
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
    PROS_VACB Vacb;
    PROS_SHARED_CACHE_MAP SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    CCTRACE(CC_API_DEBUG, "FileObject=%p StartOffset=%I64u EndOffset=%I64u Wait=%d\n",
        FileObject, StartOffset->QuadPart, EndOffset->QuadPart, Wait);

    DPRINT("CcZeroData(FileObject 0x%p, StartOffset %I64x, EndOffset %I64x, "
           "Wait %u)\n", FileObject, StartOffset->QuadPart, EndOffset->QuadPart,
           Wait);

    Length = EndOffset->QuadPart - StartOffset->QuadPart;
    WriteOffset.QuadPart = StartOffset->QuadPart;

    if (!SharedCacheMap)
    {
        /* Make this a non-cached write */
        IO_STATUS_BLOCK Iosb;
        KEVENT Event;
        PMDL Mdl;
        ULONG i;
        ULONG CurrentLength;
        PPFN_NUMBER PfnArray;

        /* Setup our Mdl */
        Mdl = IoAllocateMdl(NULL, min(Length, MAX_ZERO_LENGTH), FALSE, FALSE, NULL);
        if (!Mdl)
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);

        PfnArray = MmGetMdlPfnArray(Mdl);
        for (i = 0; i < BYTES_TO_PAGES(Mdl->ByteCount); i++)
            PfnArray[i] = CcZeroPage;
        Mdl->MdlFlags |= MDL_PAGES_LOCKED;

        /* Perform the write sequencially */
        while (Length > 0)
        {
            CurrentLength = min(Length, MAX_ZERO_LENGTH);

            Mdl->ByteCount = CurrentLength;

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
                IoFreeMdl(Mdl);
                ExRaiseStatus(Status);
            }
            WriteOffset.QuadPart += CurrentLength;
            Length -= CurrentLength;
        }

        IoFreeMdl(Mdl);

        return TRUE;
    }

    /* See if we should simply truncate the valid data length */
    if ((StartOffset->QuadPart < SharedCacheMap->ValidDataLength.QuadPart) && (EndOffset->QuadPart >= SharedCacheMap->ValidDataLength.QuadPart))
    {
        DPRINT1("Truncating VDL.\n");
        SharedCacheMap->ValidDataLength = *StartOffset;
        return TRUE;
    }

    ASSERT(EndOffset->QuadPart <= SharedCacheMap->SectionSize.QuadPart);

    while(WriteOffset.QuadPart < EndOffset->QuadPart)
    {
        ULONG VacbOffset = WriteOffset.QuadPart % VACB_MAPPING_GRANULARITY;
        ULONG VacbLength = min(Length, VACB_MAPPING_GRANULARITY - VacbOffset);

        Status = CcRosGetVacb(SharedCacheMap, WriteOffset.QuadPart, &Vacb);
        if (!NT_SUCCESS(Status))
        {
            ExRaiseStatus(Status);
            return FALSE;
        }

        _SEH2_TRY
        {
            if (!CcRosEnsureVacbResident(Vacb, Wait, FALSE, VacbOffset, VacbLength))
            {
                return FALSE;
            }

            RtlZeroMemory((PVOID)((ULONG_PTR)Vacb->BaseAddress + VacbOffset), VacbLength);

            WriteOffset.QuadPart += VacbLength;
            Length -= VacbLength;

            /* Tell Mm */
            Status = MmMakeSegmentDirty(FileObject->SectionObjectPointer,
                                        Vacb->FileOffset.QuadPart + VacbOffset,
                                        VacbLength);
            if (!NT_SUCCESS(Status))
                ExRaiseStatus(Status);
        }
        _SEH2_FINALLY
        {
            /* Do not mark the VACB as dirty if an exception was raised */
            CcRosReleaseVacb(SharedCacheMap, Vacb, !_SEH2_AbnormalTermination(), FALSE);
        }
        _SEH2_END;
    }

    /* Flush if needed */
    if (FileObject->Flags & FO_WRITE_THROUGH)
        CcFlushCache(FileObject->SectionObjectPointer, StartOffset, EndOffset->QuadPart - StartOffset->QuadPart, NULL);

    return TRUE;
}
