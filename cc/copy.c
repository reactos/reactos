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

static PFN_TYPE CcZeroPage = 0;

#define MAX_ZERO_LENGTH    (256 * 1024)
#define MAX_RW_LENGTH    (256 * 1024)

#if defined(__GNUC__)
/* void * alloca(size_t size); */
#elif defined(_MSC_VER)
void *_alloca (size_t size);
#else
#error Unknown compiler for alloca intrinsic stack allocation "function"
#endif

ULONG CcFastMdlReadWait;
ULONG CcFastMdlReadNotPossible;
ULONG CcFastReadNotPossible;
ULONG CcFastReadWait;
ULONG CcFastReadNoWait;
ULONG CcFastReadResourceMiss;

extern FAST_MUTEX CcCacheViewLock;
extern LIST_ENTRY CcFreeCacheViewListHead;
extern LIST_ENTRY CcInUseCacheViewListHead;

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI MmMapViewInSystemCache (PCACHE_VIEW);


VOID NTAPI
CcInitCacheZeroPage (VOID)
{
    NTSTATUS Status;

    Status = MmRequestPageMemoryConsumer (MC_NPPOOL, TRUE, &CcZeroPage);
    if (!NT_SUCCESS (Status))
    {
        DbgPrint ("Can't allocate CcZeroPage.\n");
        KeBugCheck(CACHE_MANAGER);
    }
    Status = MiZeroPage (CcZeroPage);
    if (!NT_SUCCESS (Status))
    {
        DbgPrint ("Can't zero out CcZeroPage.\n");
        KeBugCheck(CACHE_MANAGER);
    }
}

/*
 * @unimplemented
 */
BOOLEAN NTAPI
CcCanIWrite (IN PFILE_OBJECT FileObject, 
             IN ULONG BytesToWrite, 
             IN BOOLEAN Wait, 
             IN BOOLEAN Retrying)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN NTAPI
CcCopyRead (IN PFILE_OBJECT FileObject,
            IN PLARGE_INTEGER FileOffset, 
            IN ULONG Length, 
            IN BOOLEAN Wait, 
            OUT PVOID Buffer, 
            OUT PIO_STATUS_BLOCK IoStatus)
{

    ULONG Index;
    PBCB Bcb;
    LARGE_INTEGER Offset;
    PLIST_ENTRY entry;
    PCACHE_VIEW current = NULL;
    ULONG CurrentLength;
    NTSTATUS Status;

    DPRINT ("CcCopyRead(FileObject 0x%p, FileOffset %I64x, "
            "Length %d, Wait %d, Buffer 0x%p, IoStatus 0x%p)\n",
            FileObject, FileOffset->QuadPart, Length, Wait, Buffer, IoStatus);


    if (!Wait)
    {
        IoStatus->Information = 0;
        IoStatus->Status = STATUS_UNSUCCESSFUL;
        return FALSE;
    }

    IoStatus->Information = Length;
    IoStatus->Status = STATUS_SUCCESS;

    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;

    if (FileOffset->QuadPart + Length > Bcb->FileSizes.FileSize.QuadPart)
    {
        KeBugCheck(CACHE_MANAGER);
    }

    if (Bcb->FileSizes.AllocationSize.QuadPart > sizeof (Bcb->CacheView) / sizeof (Bcb->CacheView[0]) * CACHE_VIEW_SIZE)
    {
        /* not implemented */
        KeBugCheck(CACHE_MANAGER);
    }

    Offset = *FileOffset;

    ExAcquireFastMutex (&CcCacheViewLock);
    while (Length)
    {
        Index = Offset.QuadPart / CACHE_VIEW_SIZE;
        if (Bcb->CacheView[Index] && Bcb->CacheView[Index]->Bcb == Bcb)
        {
            if (Bcb->CacheView[Index]->RefCount == 0)
            {
                RemoveEntryList (&Bcb->CacheView[Index]->ListEntry);
                InsertHeadList (&CcInUseCacheViewListHead, &Bcb->CacheView[Index]->ListEntry);
            }
            Bcb->CacheView[Index]->RefCount++;
        }
        else
        {
            if (IsListEmpty (&CcFreeCacheViewListHead))
            {
                /* not implemented */
                KeBugCheck(CACHE_MANAGER);
            }

            entry = CcFreeCacheViewListHead.Flink;
            while (entry != &CcFreeCacheViewListHead)
            {
                current = CONTAINING_RECORD (entry, CACHE_VIEW, ListEntry);
                entry = entry->Flink;
                if (current->Bcb == NULL)
                {
                    break;
                }
            }
            if (entry == &CcFreeCacheViewListHead)
            {
                KeBugCheck(CACHE_MANAGER);
            }

            if (current->Bcb)
            {
                current->Bcb->CacheView[current->SectionData.ViewOffset / CACHE_VIEW_SIZE] = NULL;
            }
            Bcb->CacheView[Index] = current;


            if (Bcb->CacheView[Index]->Bcb != NULL)
            {
                DPRINT1 ("%x\n", Bcb->CacheView[Index]->Bcb);
                /* not implemented */
                KeBugCheck(CACHE_MANAGER);
            }
            Bcb->CacheView[Index]->RefCount = 1;
            Bcb->CacheView[Index]->Bcb = Bcb;
            Bcb->CacheView[Index]->SectionData.ViewOffset = Index * CACHE_VIEW_SIZE;
            Bcb->CacheView[Index]->SectionData.Section = Bcb->Section;
            Bcb->CacheView[Index]->SectionData.Segment = Bcb->Section->Segment;

            RemoveEntryList (&Bcb->CacheView[Index]->ListEntry);
            InsertHeadList (&CcInUseCacheViewListHead, &Bcb->CacheView[Index]->ListEntry);

            Status = MmMapViewInSystemCache (Bcb->CacheView[Index]);

            if (!NT_SUCCESS (Status))
            {
                KeBugCheck(CACHE_MANAGER);
            }
        }
        ExReleaseFastMutex (&CcCacheViewLock);

        if (Offset.QuadPart % CACHE_VIEW_SIZE)
        {
            if (Length > CACHE_VIEW_SIZE - Offset.u.LowPart % CACHE_VIEW_SIZE)
            {
                CurrentLength = CACHE_VIEW_SIZE - Offset.u.LowPart % CACHE_VIEW_SIZE;
            }
            else
            {
                CurrentLength = Length;
            }
            memcpy (Buffer,
                    (PVOID) ((ULONG_PTR) Bcb->CacheView[Index]->BaseAddress + Offset.u.LowPart % CACHE_VIEW_SIZE), CurrentLength);
            Buffer = (PVOID) ((ULONG_PTR) Buffer + CurrentLength);
            Length -= CurrentLength;
            Offset.QuadPart += CurrentLength;
        }
        else
        {
            CurrentLength = Length > CACHE_VIEW_SIZE ? CACHE_VIEW_SIZE : Length;
            memcpy (Buffer, Bcb->CacheView[Index]->BaseAddress, CurrentLength);
            Buffer = (PVOID) ((ULONG_PTR) Buffer + CurrentLength);
            Length -= CurrentLength;
            Offset.QuadPart += CurrentLength;
        }
        ExAcquireFastMutex (&CcCacheViewLock);

        Bcb->CacheView[Index]->RefCount--;
        if (Bcb->CacheView[Index]->RefCount == 0)
        {
            RemoveEntryList (&Bcb->CacheView[Index]->ListEntry);
            InsertHeadList (&CcFreeCacheViewListHead, &Bcb->CacheView[Index]->ListEntry);
        }
    }
    ExReleaseFastMutex (&CcCacheViewLock);

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN NTAPI
CcCopyWrite (IN PFILE_OBJECT FileObject, 
             IN PLARGE_INTEGER FileOffset, 
             IN ULONG Length, 
             IN BOOLEAN Wait, 
             IN PVOID Buffer)
{

    ULONG Index;
    PBCB Bcb;
    LARGE_INTEGER Offset;
    PLIST_ENTRY entry;
    PCACHE_VIEW current = NULL;
    ULONG CurrentLength;
    NTSTATUS Status;

    DPRINT ("CcCopyWrite(FileObject 0x%p, FileOffset %I64x, "
            "Length %d, Wait %d, Buffer 0x%p)\n", FileObject, FileOffset->QuadPart, Length, Wait, Buffer);

    if (!Wait)
    {
        return FALSE;
    }

    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;

    if (FileOffset->QuadPart + Length > Bcb->FileSizes.FileSize.QuadPart)
    {
        KeBugCheck(CACHE_MANAGER);
    }

    if (Bcb->FileSizes.AllocationSize.QuadPart > sizeof (Bcb->CacheView) / sizeof (Bcb->CacheView[0]) * CACHE_VIEW_SIZE)
    {
        /* not implemented */
        KeBugCheck(CACHE_MANAGER);
    }

    Offset = *FileOffset;

    ExAcquireFastMutex (&CcCacheViewLock);
    while (Length)
    {
        Index = Offset.QuadPart / CACHE_VIEW_SIZE;
        if (Bcb->CacheView[Index] && Bcb->CacheView[Index]->Bcb == Bcb)
        {
            if (Bcb->CacheView[Index]->RefCount == 0)
            {
                RemoveEntryList (&Bcb->CacheView[Index]->ListEntry);
                InsertHeadList (&CcInUseCacheViewListHead, &Bcb->CacheView[Index]->ListEntry);
            }
            Bcb->CacheView[Index]->RefCount++;
        }
        else
        {
            if (IsListEmpty (&CcFreeCacheViewListHead))
            {
                /* not implemented */
                KeBugCheck(CACHE_MANAGER);
            }

            entry = CcFreeCacheViewListHead.Flink;
            while (entry != &CcFreeCacheViewListHead)
            {
                current = CONTAINING_RECORD (entry, CACHE_VIEW, ListEntry);
                entry = entry->Flink;
                if (current->Bcb == NULL)
                {
                    break;
                }
            }
            if (entry == &CcFreeCacheViewListHead)
            {
                KeBugCheck(CACHE_MANAGER);
            }

            if (current->Bcb)
            {
                current->Bcb->CacheView[current->SectionData.ViewOffset / CACHE_VIEW_SIZE] = NULL;
            }

            Bcb->CacheView[Index] = current;

            if (Bcb->CacheView[Index]->Bcb != NULL)
            {
                DPRINT1 ("%x\n", Bcb->CacheView[Index]->Bcb);
                /* not implemented */
                KeBugCheck(CACHE_MANAGER);
            }
            Bcb->CacheView[Index]->RefCount = 1;
            Bcb->CacheView[Index]->Bcb = Bcb;
            Bcb->CacheView[Index]->SectionData.ViewOffset = Index * CACHE_VIEW_SIZE;
            Bcb->CacheView[Index]->SectionData.Section = Bcb->Section;
            Bcb->CacheView[Index]->SectionData.Segment = Bcb->Section->Segment;
			           
            RemoveEntryList (&Bcb->CacheView[Index]->ListEntry);
            InsertHeadList (&CcInUseCacheViewListHead, &Bcb->CacheView[Index]->ListEntry);

            Status = MmMapViewInSystemCache (Bcb->CacheView[Index]);

            if (!NT_SUCCESS (Status))
            {
                KeBugCheck(CACHE_MANAGER);
            }
        }
        ExReleaseFastMutex (&CcCacheViewLock);

        if (Offset.QuadPart % CACHE_VIEW_SIZE)
        {
            if (Length > CACHE_VIEW_SIZE - Offset.u.LowPart % CACHE_VIEW_SIZE)
            {
                CurrentLength = CACHE_VIEW_SIZE - Offset.u.LowPart % CACHE_VIEW_SIZE;
            }
            else
            {
                CurrentLength = Length;
            }
            memcpy ((PVOID) ((ULONG_PTR) Bcb->CacheView[Index]->BaseAddress + Offset.u.LowPart % CACHE_VIEW_SIZE),
                    Buffer, CurrentLength);
            Buffer = (PVOID) ((ULONG_PTR) Buffer + CurrentLength);
            Length -= CurrentLength;
            Offset.QuadPart += CurrentLength;
        }
        else
        {
            CurrentLength = Length > CACHE_VIEW_SIZE ? CACHE_VIEW_SIZE : Length;
            memcpy (Bcb->CacheView[Index]->BaseAddress, Buffer, CurrentLength);
            Buffer = (PVOID) ((ULONG_PTR) Buffer + CurrentLength);
            Length -= CurrentLength;
            Offset.QuadPart += CurrentLength;
        }
        ExAcquireFastMutex (&CcCacheViewLock);

        Bcb->CacheView[Index]->RefCount--;
        if (Bcb->CacheView[Index]->RefCount == 0)
        {
            RemoveEntryList (&Bcb->CacheView[Index]->ListEntry);
            InsertHeadList (&CcFreeCacheViewListHead, &Bcb->CacheView[Index]->ListEntry);
        }
    }
    ExReleaseFastMutex (&CcCacheViewLock);

    return TRUE;
}

/*
 * @unimplemented
 */
VOID NTAPI
CcDeferWrite (IN PFILE_OBJECT FileObject,
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
VOID NTAPI
CcFastCopyRead (IN PFILE_OBJECT FileObject,
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
VOID NTAPI
CcFastCopyWrite (IN PFILE_OBJECT FileObject, 
                 IN ULONG FileOffset, 
                 IN ULONG Length, 
                 IN PVOID Buffer)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
CcWaitForCurrentLazyWriterActivity (VOID)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
BOOLEAN NTAPI
CcZeroData (IN PFILE_OBJECT FileObject, 
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
    LARGE_INTEGER Offset;
    ULONG Index;

    DPRINT ("CcZeroData(FileObject 0x%p, StartOffset %I64x, EndOffset %I64x, "
            "Wait %d)\n", FileObject, StartOffset->QuadPart, EndOffset->QuadPart, Wait);

    Length = EndOffset->u.LowPart - StartOffset->u.LowPart;
    WriteOffset.QuadPart = StartOffset->QuadPart;

    if (FileObject->SectionObjectPointer->SharedCacheMap == NULL)
    {
        /* File is not cached */

        Mdl = alloca (MmSizeOfMdl (NULL, MAX_ZERO_LENGTH));

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
            MmInitializeMdl (Mdl, (PVOID) WriteOffset.u.LowPart, CurrentLength);
            Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
            for (i = 0; i < ((Mdl->Size - sizeof (MDL)) / sizeof (ULONG)); i++)
            {
                ((PPFN_TYPE) (Mdl + 1))[i] = CcZeroPage;
            }
            KeInitializeEvent (&Event, NotificationEvent, FALSE);
            Status = IoSynchronousPageWrite (FileObject, Mdl, &WriteOffset, &Event, &Iosb);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL);
                Status = Iosb.Status;
            }
            MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
            if (!NT_SUCCESS (Status))
            {
                return (FALSE);
            }
            WriteOffset.QuadPart += CurrentLength;
            Length -= CurrentLength;
        }
    }
    else
    {
        /* File is cached */
        PBCB Bcb;
        PCACHE_VIEW current = NULL;
        PLIST_ENTRY entry;

        Bcb = FileObject->SectionObjectPointer->SharedCacheMap;

        if (!Wait)
        {
            return FALSE;
        }

        if (EndOffset->QuadPart > Bcb->FileSizes.FileSize.QuadPart)
        {
            KeBugCheck(CACHE_MANAGER);
        }

        if (Bcb->FileSizes.AllocationSize.QuadPart > sizeof (Bcb->CacheView) / sizeof (Bcb->CacheView[0]) * CACHE_VIEW_SIZE)
        {
            /* not implemented */
            KeBugCheck(CACHE_MANAGER);
        }

        Offset = *StartOffset;
        Length = EndOffset->QuadPart - StartOffset->QuadPart;

        ExAcquireFastMutex (&CcCacheViewLock);
        while (Length)
        {
            Index = Offset.QuadPart / CACHE_VIEW_SIZE;
            if (Bcb->CacheView[Index] && Bcb->CacheView[Index]->Bcb == Bcb)
            {
                if (Bcb->CacheView[Index]->RefCount == 0)
                {
                    RemoveEntryList (&Bcb->CacheView[Index]->ListEntry);
                    InsertHeadList (&CcInUseCacheViewListHead, &Bcb->CacheView[Index]->ListEntry);
                }
                Bcb->CacheView[Index]->RefCount++;
            }
            else
            {
                if (IsListEmpty (&CcFreeCacheViewListHead))
                {
                    /* not implemented */
                    KeBugCheck(CACHE_MANAGER);
                }

                entry = CcFreeCacheViewListHead.Flink;
                while (entry != &CcFreeCacheViewListHead)
                {
                    current = CONTAINING_RECORD (entry, CACHE_VIEW, ListEntry);
                    entry = entry->Flink;
                    if (current->Bcb == NULL)
                    {
                        break;
                    }
                }
                if (entry == &CcFreeCacheViewListHead)
                {
                    KeBugCheck(CACHE_MANAGER);
                }

                Bcb->CacheView[Index] = current;

                if (Bcb->CacheView[Index]->Bcb != NULL)
                {
                    DPRINT1 ("%x\n", Bcb->CacheView[Index]->Bcb);
                    /* not implemented */
                    KeBugCheck(CACHE_MANAGER);
                }
                Bcb->CacheView[Index]->RefCount = 1;
                Bcb->CacheView[Index]->Bcb = Bcb;
                Bcb->CacheView[Index]->SectionData.ViewOffset = Index * CACHE_VIEW_SIZE;
                Bcb->CacheView[Index]->SectionData.Section = Bcb->Section;
                Bcb->CacheView[Index]->SectionData.Segment = Bcb->Section->Segment;

                RemoveEntryList (&Bcb->CacheView[Index]->ListEntry);
                InsertHeadList (&CcInUseCacheViewListHead, &Bcb->CacheView[Index]->ListEntry);

                Status = MmMapViewInSystemCache (Bcb->CacheView[Index]);

                if (!NT_SUCCESS (Status))
                {
                    KeBugCheck(CACHE_MANAGER);
                }
            }
            ExReleaseFastMutex (&CcCacheViewLock);

            if (Offset.QuadPart % CACHE_VIEW_SIZE)
            {
                if (Length > CACHE_VIEW_SIZE - Offset.u.LowPart % CACHE_VIEW_SIZE)
                {
                    CurrentLength = CACHE_VIEW_SIZE - Offset.u.LowPart % CACHE_VIEW_SIZE;
                }
                else
                {
                    CurrentLength = Length;
                }
                memset ((PVOID) ((ULONG_PTR) Bcb->CacheView[Index]->BaseAddress + Offset.u.LowPart % CACHE_VIEW_SIZE), 0,
                        CurrentLength);
                Length -= CurrentLength;
                Offset.QuadPart += CurrentLength;
            }
            else
            {
                CurrentLength = Length > CACHE_VIEW_SIZE ? CACHE_VIEW_SIZE : Length;
                memset (Bcb->CacheView[Index]->BaseAddress, 0, CurrentLength);
                Length -= CurrentLength;
                Offset.QuadPart += CurrentLength;
            }
            ExAcquireFastMutex (&CcCacheViewLock);

            Bcb->CacheView[Index]->RefCount--;
            if (Bcb->CacheView[Index]->RefCount == 0)
            {
                RemoveEntryList (&Bcb->CacheView[Index]->ListEntry);
                InsertHeadList (&CcFreeCacheViewListHead, &Bcb->CacheView[Index]->ListEntry);
            }
        }
        ExReleaseFastMutex (&CcCacheViewLock);
    }
    return (TRUE);
}
