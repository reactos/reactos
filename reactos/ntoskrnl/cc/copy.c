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
#define MAX_RW_LENGTH    (256 * 1024)

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
ReadCacheSegmentChain (
    PBCB Bcb,
    ULONG ReadOffset,
    ULONG Length,
    PVOID Buffer)
{
    PCACHE_SEGMENT head;
    PCACHE_SEGMENT current;
    PCACHE_SEGMENT previous;
    IO_STATUS_BLOCK Iosb;
    LARGE_INTEGER SegOffset;
    NTSTATUS Status;
    ULONG TempLength;
    KEVENT Event;
    PMDL Mdl;

    Mdl = _alloca(MmSizeOfMdl(NULL, MAX_RW_LENGTH));

    Status = CcRosGetCacheSegmentChain(Bcb, ReadOffset, Length, &head);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    current = head;
    while (current != NULL)
    {
        /*
         * If the current segment is valid then copy it into the
         * user buffer.
         */
        if (current->Valid)
        {
            TempLength = min(VACB_MAPPING_GRANULARITY, Length);
            memcpy(Buffer, current->BaseAddress, TempLength);

            Buffer = (PVOID)((ULONG_PTR)Buffer + TempLength);

            Length = Length - TempLength;
            previous = current;
            current = current->NextInChain;
            CcRosReleaseCacheSegment(Bcb, previous, TRUE, FALSE, FALSE);
        }
        /*
         * Otherwise read in as much as we can.
         */
        else
        {
            PCACHE_SEGMENT current2;
            ULONG current_size;
            ULONG i;
            PPFN_NUMBER MdlPages;

            /*
             * Count the maximum number of bytes we could read starting
             * from the current segment.
             */
            current2 = current;
            current_size = 0;
            while ((current2 != NULL) && !current2->Valid && (current_size < MAX_RW_LENGTH))
            {
                current2 = current2->NextInChain;
                current_size += VACB_MAPPING_GRANULARITY;
            }

            /*
             * Create an MDL which contains all their pages.
             */
            MmInitializeMdl(Mdl, NULL, current_size);
            Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
            current2 = current;
            current_size = 0;
            MdlPages = (PPFN_NUMBER)(Mdl + 1);
            while ((current2 != NULL) && !current2->Valid && (current_size < MAX_RW_LENGTH))
            {
                PVOID address = current2->BaseAddress;
                for (i = 0; i < (VACB_MAPPING_GRANULARITY / PAGE_SIZE); i++, address = RVA(address, PAGE_SIZE))
                {
                    *MdlPages++ = MmGetPfnForProcess(NULL, address);
                }
                current2 = current2->NextInChain;
                current_size += VACB_MAPPING_GRANULARITY;
            }

            /*
             * Read in the information.
             */
            SegOffset.QuadPart = current->FileOffset;
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            Status = IoPageRead(Bcb->FileObject,
                                Mdl,
                                &SegOffset,
                                &Event,
                                &Iosb);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = Iosb.Status;
            }
            if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
            {
                MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
            }
            if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
            {
                while (current != NULL)
                {
                    previous = current;
                    current = current->NextInChain;
                    CcRosReleaseCacheSegment(Bcb, previous, FALSE, FALSE, FALSE);
                }
                return Status;
            }
            current_size = 0;
            while (current != NULL && !current->Valid && current_size < MAX_RW_LENGTH)
            {
                previous = current;
                current = current->NextInChain;
                TempLength = min(VACB_MAPPING_GRANULARITY, Length);
                memcpy(Buffer, previous->BaseAddress, TempLength);

                Buffer = (PVOID)((ULONG_PTR)Buffer + TempLength);

                Length = Length - TempLength;
                CcRosReleaseCacheSegment(Bcb, previous, TRUE, FALSE, FALSE);
                current_size += VACB_MAPPING_GRANULARITY;
            }
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ReadCacheSegment (
    PCACHE_SEGMENT CacheSeg)
{
    ULONG Size;
    PMDL Mdl;
    NTSTATUS Status;
    LARGE_INTEGER SegOffset;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;

    SegOffset.QuadPart = CacheSeg->FileOffset;
    Size = (ULONG)(CacheSeg->Bcb->AllocationSize.QuadPart - CacheSeg->FileOffset);
    if (Size > VACB_MAPPING_GRANULARITY)
    {
        Size = VACB_MAPPING_GRANULARITY;
    }

    Mdl = IoAllocateMdl(CacheSeg->BaseAddress, Size, FALSE, FALSE, NULL);
    if (!Mdl)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(Mdl);
    Mdl->MdlFlags |= MDL_IO_PAGE_READ;
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Status = IoPageRead(CacheSeg->Bcb->FileObject, Mdl, &SegOffset, &Event, &IoStatus);
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

    if (VACB_MAPPING_GRANULARITY > Size)
    {
        RtlZeroMemory((char*)CacheSeg->BaseAddress + Size,
                      VACB_MAPPING_GRANULARITY - Size);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WriteCacheSegment (
    PCACHE_SEGMENT CacheSeg)
{
    ULONG Size;
    PMDL Mdl;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER SegOffset;
    KEVENT Event;

    CacheSeg->Dirty = FALSE;
    SegOffset.QuadPart = CacheSeg->FileOffset;
    Size = (ULONG)(CacheSeg->Bcb->AllocationSize.QuadPart - CacheSeg->FileOffset);
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
            MmGetPfnForProcess(NULL, (PVOID)((ULONG_PTR)CacheSeg->BaseAddress + (i << PAGE_SHIFT)));
        } while (++i < (Size >> PAGE_SHIFT));
    }

    Mdl = IoAllocateMdl(CacheSeg->BaseAddress, Size, FALSE, FALSE, NULL);
    if (!Mdl)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    MmBuildMdlForNonPagedPool(Mdl);
    Mdl->MdlFlags |= MDL_IO_PAGE_READ;
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Status = IoSynchronousPageWrite(CacheSeg->Bcb->FileObject, Mdl, &SegOffset, &Event, &IoStatus);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }
    IoFreeMdl(Mdl);
    if (!NT_SUCCESS(Status) && (Status != STATUS_END_OF_FILE))
    {
        DPRINT1("IoPageWrite failed, Status %x\n", Status);
        CacheSeg->Dirty = TRUE;
        return Status;
    }

    return STATUS_SUCCESS;
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
    UNIMPLEMENTED;
    return FALSE;
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
    ULONG ReadOffset;
    ULONG TempLength;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID BaseAddress;
    PCACHE_SEGMENT CacheSeg;
    BOOLEAN Valid;
    ULONG ReadLength = 0;
    PBCB Bcb;
    KIRQL oldirql;
    PLIST_ENTRY current_entry;
    PCACHE_SEGMENT current;

    DPRINT("CcCopyRead(FileObject 0x%p, FileOffset %I64x, "
           "Length %lu, Wait %u, Buffer 0x%p, IoStatus 0x%p)\n",
           FileObject, FileOffset->QuadPart, Length, Wait,
           Buffer, IoStatus);

    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
    ReadOffset = (ULONG)FileOffset->QuadPart;

    DPRINT("AllocationSize %I64d, FileSize %I64d\n",
           Bcb->AllocationSize.QuadPart,
           Bcb->FileSize.QuadPart);

    /*
     * Check for the nowait case that all the cache segments that would
     * cover this read are in memory.
     */
    if (!Wait)
    {
        KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
        /* FIXME: this loop doesn't take into account areas that don't have
         * a segment in the list yet */
        current_entry = Bcb->BcbSegmentListHead.Flink;
        while (current_entry != &Bcb->BcbSegmentListHead)
        {
            current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT,
                                        BcbSegmentListEntry);
            if (!current->Valid &&
                DoSegmentsIntersect(current->FileOffset, VACB_MAPPING_GRANULARITY,
                                    ReadOffset, Length))
            {
                KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
                IoStatus->Status = STATUS_UNSUCCESSFUL;
                IoStatus->Information = 0;
                return FALSE;
            }
            if (current->FileOffset >= ReadOffset + Length)
                break;
            current_entry = current_entry->Flink;
        }
        KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
    }

    TempLength = ReadOffset % VACB_MAPPING_GRANULARITY;
    if (TempLength != 0)
    {
        TempLength = min (Length, VACB_MAPPING_GRANULARITY - TempLength);
        Status = CcRosRequestCacheSegment(Bcb,
                                          ROUND_DOWN(ReadOffset,
                                                  VACB_MAPPING_GRANULARITY),
                                          &BaseAddress, &Valid, &CacheSeg);
        if (!NT_SUCCESS(Status))
        {
            IoStatus->Information = 0;
            IoStatus->Status = Status;
            DPRINT("CcRosRequestCacheSegment faild, Status %x\n", Status);
            return FALSE;
        }
        if (!Valid)
        {
            Status = ReadCacheSegment(CacheSeg);
            if (!NT_SUCCESS(Status))
            {
                IoStatus->Information = 0;
                IoStatus->Status = Status;
                CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
                return FALSE;
            }
        }
        memcpy (Buffer, (char*)BaseAddress + ReadOffset % VACB_MAPPING_GRANULARITY,
                TempLength);
        CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, FALSE, FALSE);
        ReadLength += TempLength;
        Length -= TempLength;
        ReadOffset += TempLength;
        Buffer = (PVOID)((char*)Buffer + TempLength);
    }

    while (Length > 0)
    {
        TempLength = min(max(VACB_MAPPING_GRANULARITY, MAX_RW_LENGTH), Length);
        Status = ReadCacheSegmentChain(Bcb, ReadOffset, TempLength, Buffer);
        if (!NT_SUCCESS(Status))
        {
            IoStatus->Information = 0;
            IoStatus->Status = Status;
            DPRINT("ReadCacheSegmentChain failed, Status %x\n", Status);
            return FALSE;
        }

        ReadLength += TempLength;
        Length -= TempLength;
        ReadOffset += TempLength;

        Buffer = (PVOID)((ULONG_PTR)Buffer + TempLength);
    }

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = ReadLength;
    DPRINT("CcCopyRead O.K.\n");
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
    NTSTATUS Status;
    ULONG WriteOffset;
    KIRQL oldirql;
    PBCB Bcb;
    PLIST_ENTRY current_entry;
    PCACHE_SEGMENT CacheSeg;
    ULONG TempLength;
    PVOID BaseAddress;
    BOOLEAN Valid;

    DPRINT("CcCopyWrite(FileObject 0x%p, FileOffset %I64x, "
           "Length %lu, Wait %u, Buffer 0x%p)\n",
           FileObject, FileOffset->QuadPart, Length, Wait, Buffer);

    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
    WriteOffset = (ULONG)FileOffset->QuadPart;

    if (!Wait)
    {
        /* testing, if the requested datas are available */
        KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
        /* FIXME: this loop doesn't take into account areas that don't have
         * a segment in the list yet */
        current_entry = Bcb->BcbSegmentListHead.Flink;
        while (current_entry != &Bcb->BcbSegmentListHead)
        {
            CacheSeg = CONTAINING_RECORD(current_entry, CACHE_SEGMENT,
                                         BcbSegmentListEntry);
            if (!CacheSeg->Valid &&
                DoSegmentsIntersect(CacheSeg->FileOffset, VACB_MAPPING_GRANULARITY,
                                    WriteOffset, Length))
            {
                KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
                /* datas not available */
                return FALSE;
            }
            if (CacheSeg->FileOffset >= WriteOffset + Length)
                break;
            current_entry = current_entry->Flink;
        }
        KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
    }

    TempLength = WriteOffset % VACB_MAPPING_GRANULARITY;
    if (TempLength != 0)
    {
        ULONG ROffset;
        ROffset = ROUND_DOWN(WriteOffset, VACB_MAPPING_GRANULARITY);
        TempLength = min (Length, VACB_MAPPING_GRANULARITY - TempLength);
        Status = CcRosRequestCacheSegment(Bcb, ROffset,
                                          &BaseAddress, &Valid, &CacheSeg);
        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }
        if (!Valid)
        {
            if (!NT_SUCCESS(ReadCacheSegment(CacheSeg)))
            {
                return FALSE;
            }
        }
        memcpy ((char*)BaseAddress + WriteOffset % VACB_MAPPING_GRANULARITY,
                Buffer, TempLength);
        CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, TRUE, FALSE);

        Length -= TempLength;
        WriteOffset += TempLength;

        Buffer = (PVOID)((ULONG_PTR)Buffer + TempLength);
    }

    while (Length > 0)
    {
        TempLength = min (VACB_MAPPING_GRANULARITY, Length);
        Status = CcRosRequestCacheSegment(Bcb,
                                          WriteOffset,
                                          &BaseAddress,
                                          &Valid,
                                          &CacheSeg);
        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }
        if (!Valid && TempLength < VACB_MAPPING_GRANULARITY)
        {
            if (!NT_SUCCESS(ReadCacheSegment(CacheSeg)))
            {
                CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
                return FALSE;
            }
        }
        memcpy (BaseAddress, Buffer, TempLength);
        CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, TRUE, FALSE);
        Length -= TempLength;
        WriteOffset += TempLength;

        Buffer = (PVOID)((ULONG_PTR)Buffer + TempLength);
    }
    return TRUE;
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
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
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
    ULONG Length;
    ULONG CurrentLength;
    PMDL Mdl;
    ULONG i;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;

    DPRINT("CcZeroData(FileObject 0x%p, StartOffset %I64x, EndOffset %I64x, "
           "Wait %u)\n", FileObject, StartOffset->QuadPart, EndOffset->QuadPart,
           Wait);

    Length = EndOffset->u.LowPart - StartOffset->u.LowPart;
    WriteOffset.QuadPart = StartOffset->QuadPart;

    if (FileObject->SectionObjectPointer->SharedCacheMap == NULL)
    {
        /* File is not cached */

        Mdl = _alloca(MmSizeOfMdl(NULL, MAX_ZERO_LENGTH));

        while (Length > 0)
        {
            if (Length + WriteOffset.u.LowPart % PAGE_SIZE > MAX_ZERO_LENGTH)
            {
                CurrentLength = MAX_ZERO_LENGTH - WriteOffset.u.LowPart % PAGE_SIZE;
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
        /* File is cached */
        KIRQL oldirql;
        PBCB Bcb;
        PLIST_ENTRY current_entry;
        PCACHE_SEGMENT CacheSeg, current, previous;
        ULONG TempLength;

        Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
        if (!Wait)
        {
            /* testing, if the requested datas are available */
            KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
            /* FIXME: this loop doesn't take into account areas that don't have
             * a segment in the list yet */
            current_entry = Bcb->BcbSegmentListHead.Flink;
            while (current_entry != &Bcb->BcbSegmentListHead)
            {
                CacheSeg = CONTAINING_RECORD(current_entry, CACHE_SEGMENT,
                                             BcbSegmentListEntry);
                if (!CacheSeg->Valid &&
                    DoSegmentsIntersect(CacheSeg->FileOffset, VACB_MAPPING_GRANULARITY,
                                        WriteOffset.u.LowPart, Length))
                {
                    KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
                    /* datas not available */
                    return FALSE;
                }
                if (CacheSeg->FileOffset >= WriteOffset.u.LowPart + Length)
                    break;
                current_entry = current_entry->Flink;
            }
            KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
        }

        while (Length > 0)
        {
            ULONG Offset;
            Offset = WriteOffset.u.LowPart % VACB_MAPPING_GRANULARITY;
            if (Length + Offset > MAX_ZERO_LENGTH)
            {
                CurrentLength = MAX_ZERO_LENGTH - Offset;
            }
            else
            {
                CurrentLength = Length;
            }
            Status = CcRosGetCacheSegmentChain (Bcb, WriteOffset.u.LowPart - Offset,
                                                Offset + CurrentLength, &CacheSeg);
            if (!NT_SUCCESS(Status))
            {
                return FALSE;
            }
            current = CacheSeg;

            while (current != NULL)
            {
                Offset = WriteOffset.u.LowPart % VACB_MAPPING_GRANULARITY;
                if ((Offset != 0) ||
                    (Offset + CurrentLength < VACB_MAPPING_GRANULARITY))
                {
                    if (!current->Valid)
                    {
                        /* read the segment */
                        Status = ReadCacheSegment(current);
                        if (!NT_SUCCESS(Status))
                        {
                            DPRINT1("ReadCacheSegment failed, status %x\n",
                                    Status);
                        }
                    }
                    TempLength = min (CurrentLength, VACB_MAPPING_GRANULARITY - Offset);
                }
                else
                {
                    TempLength = VACB_MAPPING_GRANULARITY;
                }
                memset ((PUCHAR)current->BaseAddress + Offset, 0, TempLength);

                WriteOffset.QuadPart += TempLength;
                CurrentLength -= TempLength;
                Length -= TempLength;

                current = current->NextInChain;
            }

            current = CacheSeg;
            while (current != NULL)
            {
                previous = current;
                current = current->NextInChain;
                CcRosReleaseCacheSegment(Bcb, previous, TRUE, TRUE, FALSE);
            }
        }
    }

    return TRUE;
}
