/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/pin.c
 * PURPOSE:         Implements cache managers pinning interface
 *
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern NPAGED_LOOKASIDE_LIST iBcbLookasideList;

extern FAST_MUTEX CcCacheViewLock;
extern LIST_ENTRY CcFreeCacheViewListHead;
extern LIST_ENTRY CcInUseCacheViewListHead;

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI MmMapViewInSystemCache (PCACHE_VIEW);


/*
 * @implemented
 */
BOOLEAN NTAPI
CcMapData (IN PFILE_OBJECT FileObject,
           IN PLARGE_INTEGER FileOffset, 
           IN ULONG Length, 
           IN ULONG Flags, 
           OUT PVOID * piBcb, 
           OUT PVOID * pBuffer)
{
    PINTERNAL_BCB iBcb;
    PBCB Bcb;
    ULONG Index;
    NTSTATUS Status;
    PLIST_ENTRY entry;
    PCACHE_VIEW current = NULL;

    DPRINT ("CcMapData(FileObject 0x%p, FileOffset %I64x, Length %d,"
            " pBcb 0x%p, pBuffer 0x%p)\n", FileObject, FileOffset->QuadPart, Length, piBcb, pBuffer);

    ASSERT (FileObject);
    ASSERT (FileObject->SectionObjectPointer);
    ASSERT (FileObject->SectionObjectPointer->SharedCacheMap);
    ASSERT (FileOffset);
    ASSERT (piBcb);
    ASSERT (pBuffer);
	//ASSERT (Flags & MAP_WAIT);

    if (!(Flags & MAP_WAIT))
    {
		DPRINT1("Reading without MAP_WAIT flag!\n");
        //*piBcb = NULL;
        //*pBuffer = NULL;
        //return FALSE;
	}

    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;

    if (FileOffset->QuadPart + Length > Bcb->FileSizes.AllocationSize.QuadPart)
    {
        DPRINT ("%d %I64d %I64d\n", Length, FileOffset->QuadPart + Length, Bcb->FileSizes.AllocationSize.QuadPart);
//      KEBUGCHECK(0);
    }

    if (FileOffset->QuadPart + Length - ROUND_DOWN (FileOffset->QuadPart, CACHE_VIEW_SIZE) > CACHE_VIEW_SIZE)
    {
        /* not implemented */
        KeBugCheck(CACHE_MANAGER);
    }



    if (Bcb->FileSizes.AllocationSize.QuadPart > sizeof (Bcb->CacheView) / sizeof (Bcb->CacheView[0]) * CACHE_VIEW_SIZE)
    {
        /* not implemented */
        KeBugCheck(CACHE_MANAGER);
    }

    ExAcquireFastMutex (&CcCacheViewLock);

    Index = FileOffset->QuadPart / CACHE_VIEW_SIZE;
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

    iBcb = ExAllocateFromNPagedLookasideList (&iBcbLookasideList);
    if (iBcb == NULL)
    {
        KeBugCheck(CACHE_MANAGER);
    }
    memset (iBcb, 0, sizeof (INTERNAL_BCB));

    iBcb->Bcb = Bcb;
    iBcb->Index = Index;

    *piBcb = iBcb;
    *pBuffer = (PVOID) ((ULONG_PTR) Bcb->CacheView[Index]->BaseAddress +
                (ULONG_PTR) (FileOffset->QuadPart - Bcb->CacheView[Index]->SectionData.ViewOffset));

    DPRINT ("CcMapData() done\n");

    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN NTAPI
CcPinMappedData (IN PFILE_OBJECT FileObject, 
                 IN PLARGE_INTEGER FileOffset, 
                 IN ULONG Length, 
                 IN ULONG Flags, 
                 OUT PVOID * Bcb)
{
    /* no-op for current implementation. */
    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN NTAPI
CcPinRead (IN PFILE_OBJECT FileObject,
           IN PLARGE_INTEGER FileOffset, 
           IN ULONG Length, 
           IN ULONG Flags, 
           OUT PVOID * Bcb, 
           OUT PVOID * Buffer)
{
    if (CcMapData (FileObject, FileOffset, Length, Flags, Bcb, Buffer))
    {
        if (CcPinMappedData (FileObject, FileOffset, Length, Flags, Bcb))
            return TRUE;
        else
            CcUnpinData (Bcb);
    }
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN NTAPI
CcPreparePinWrite (IN PFILE_OBJECT FileObject,
                   IN PLARGE_INTEGER FileOffset,
                   IN ULONG Length, 
                   IN BOOLEAN Zero, 
                   IN ULONG Flags, 
                   OUT PVOID * Bcb, 
                   OUT PVOID * Buffer)
{
    /*
     * FIXME: This is function is similar to CcPinRead, but doesn't
     * read the data if they're not present. Instead it should just
     * prepare the cache segments and zero them out if Zero == TRUE.
     *
     * For now calling CcPinRead is better than returning error or
     * just having UNIMPLEMENTED here.
     */
    return CcPinRead (FileObject, FileOffset, Length, Flags, Bcb, Buffer);
}

/*
 * @implemented
 */
VOID NTAPI
CcSetDirtyPinnedData (IN PVOID Bcb, 
                      IN PLARGE_INTEGER Lsn)
{
//   PINTERNAL_BCB iBcb = Bcb;
//   iBcb->Dirty = TRUE;
//   UNIMPLEMENTED;
}


/*
 * @implemented
 */
VOID NTAPI
CcUnpinData (IN PVOID _iBcb)
{
    PINTERNAL_BCB iBcb = _iBcb;

    DPRINT ("CcUnpinData(%x)\n", _iBcb);

    ExAcquireFastMutex (&CcCacheViewLock);
    iBcb->Bcb->CacheView[iBcb->Index]->RefCount--;
    if (iBcb->Bcb->CacheView[iBcb->Index]->RefCount == 0)
    {
        RemoveEntryList (&iBcb->Bcb->CacheView[iBcb->Index]->ListEntry);
        InsertHeadList (&CcFreeCacheViewListHead, &iBcb->Bcb->CacheView[iBcb->Index]->ListEntry);
    }
    ExReleaseFastMutex (&CcCacheViewLock);
    ExFreeToNPagedLookasideList (&iBcbLookasideList, iBcb);

    DPRINT ("CcUnpinData done\n");
}

/*
 * @unimplemented
 */
VOID NTAPI
CcUnpinDataForThread (IN PVOID Bcb,
                      IN ERESOURCE_THREAD ResourceThreadId)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID NTAPI
CcRepinBcb (IN PVOID Bcb)
{
#if 0
    PINTERNAL_BCB iBcb = Bcb;
//  iBcb->RefCount++;
    ExAcquireFastMutex (&CcCacheViewLock);
    iBcb->Bcb->CacheView[iBcb->Index]->RefCount++;
    ExReleaseFastMutex (&CcCacheViewLock);
#endif
}

/*
 * @implemented
 */
VOID NTAPI
CcUnpinRepinnedBcb (IN PVOID Bcb,
                    IN BOOLEAN WriteThrough,
                    OUT PIO_STATUS_BLOCK IoStatus)
{
#if 0
    PINTERNAL_BCB iBcb = Bcb;

    DPRINT ("CcUnpinRepinnedBcb(%x)\n", Bcb);

    ExAcquireFastMutex (&CcCacheViewLock);
    iBcb->Bcb->CacheView[iBcb->Index]->RefCount--;
    if (iBcb->Bcb->CacheView[iBcb->Index]->RefCount == 0)
    {
        RemoveEntryList (&iBcb->Bcb->CacheView[iBcb->Index]->ListEntry);
        InsertHeadList (&CcFreeCacheViewListHead, &iBcb->Bcb->CacheView[iBcb->Index]->ListEntry);
    }
    ExReleaseFastMutex (&CcCacheViewLock);
    ExFreeToNPagedLookasideList (&iBcbLookasideList, iBcb);
#endif
    IoStatus->Status = STATUS_SUCCESS;

    DPRINT ("CcUnpinRepinnedBcb done\n");
}
