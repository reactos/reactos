/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/copy.c
 * PURPOSE:         Implements cache managers copy interface
 *
 * PROGRAMMERS:
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

ULONG CcRosTraceLevel = 0;
ULONG CcFastMdlReadWait;
ULONG CcFastMdlReadNotPossible;
ULONG CcFastReadNotPossible;
ULONG CcFastReadWait;
ULONG CcFastReadNoWait;
ULONG CcFastReadResourceMiss;

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

    Size = (ULONG)(Vacb->SharedCacheMap->SectionSize.QuadPart - Vacb->FileOffset.QuadPart);
    if (Size > VACB_MAPPING_GRANULARITY)
    {
        Size = VACB_MAPPING_GRANULARITY;
    }

    Mdl = IoAllocateMdl(Vacb->BaseAddress, Size, FALSE, FALSE, NULL);
    if (!Mdl)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(Mdl);
    Mdl->MdlFlags |= MDL_IO_PAGE_READ;
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Status = IoPageRead(Vacb->SharedCacheMap->FileObject, Mdl, &Vacb->FileOffset, &Event, &IoStatus);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
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

    Vacb->Dirty = FALSE;
    Size = (ULONG)(Vacb->SharedCacheMap->SectionSize.QuadPart - Vacb->FileOffset.QuadPart);
    if (Size > VACB_MAPPING_GRANULARITY)
    {
        Size = VACB_MAPPING_GRANULARITY;
    }
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

    Mdl = IoAllocateMdl(Vacb->BaseAddress, Size, FALSE, FALSE, NULL);
    if (!Mdl)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    MmBuildMdlForNonPagedPool(Mdl);
    Mdl->MdlFlags |= MDL_IO_PAGE_READ;
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Status = IoSynchronousPageWrite(Vacb->SharedCacheMap->FileObject, Mdl, &Vacb->FileOffset, &Event, &IoStatus);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }
    IoFreeMdl(Mdl);
    if (!NT_SUCCESS(Status) && (Status != STATUS_END_OF_FILE))
    {
        DPRINT1("IoPageWrite failed, Status %x\n", Status);
        Vacb->Dirty = TRUE;
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

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
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

        if (Buffer)
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

        if (Buffer)
            Buffer = (PVOID)((ULONG_PTR)Buffer + PartialLength);
    }
    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = BytesCopied;
    return TRUE;
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
    CCTRACE(CC_API_DEBUG, "FileObject=%p BytesToWrite=%lu Wait=%d Retrying=%d\n",
        FileObject, BytesToWrite, Wait, Retrying);

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
 * @unimplemented
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
    CCTRACE(CC_API_DEBUG, "FileObject=%p PostRoutine=%p Context1=%p Context2=%p BytesToWrite=%lu Retrying=%d\n",
        FileObject, PostRoutine, Context1, Context2, BytesToWrite, Retrying);

    PostRoutine(Context1, Context2);
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
    NT_ASSERT(Success == TRUE);
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
    NT_ASSERT(Success == TRUE);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
CcWaitForCurrentLazyWriterActivity (
    VOID)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
