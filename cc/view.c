/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/view.c
 * PURPOSE:         Cache manager
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#ifdef ROUND_UP
#undef ROUND_UP
#endif
#ifdef ROUND_DOWN
#undef ROUND_DOWN
#endif

#define ROUND_UP(N, S)        (((N) + (S) - 1) & ~((S) - 1))
#define ROUND_DOWN(N, S)    ((N) & ~((S) - 1))

NPAGED_LOOKASIDE_LIST iBcbLookasideList;
static NPAGED_LOOKASIDE_LIST BcbLookasideList;

PVOID CcCacheViewBase;
ULONG CcCacheViewArrayCount;
PCACHE_VIEW CcCacheViewArray;
FAST_MUTEX CcCacheViewLock;
LIST_ENTRY CcFreeCacheViewListHead;
LIST_ENTRY CcInUseCacheViewListHead;
PMEMORY_AREA CcCacheViewMemoryArea;

NTSTATUS NTAPI MmCreateDataFileSection (PSECTION_OBJECT * SectionObject,
                                        ACCESS_MASK DesiredAccess,
                                        POBJECT_ATTRIBUTES ObjectAttributes,
                                        PLARGE_INTEGER UMaximumSize,
                                        ULONG SectionPageProtection,
                                        ULONG AllocationAttributes, 
                                        PFILE_OBJECT FileObject, 
                                        BOOLEAN CacheManager);

NTSTATUS NTAPI MmUnmapViewInSystemCache (PCACHE_VIEW);

NTSTATUS MmFlushDataFileSection (PSECTION_OBJECT Section, PLARGE_INTEGER StartOffset, ULONG Length);

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID NTAPI
CcFlushCache (IN PSECTION_OBJECT_POINTERS SectionObjectPointers,
              IN PLARGE_INTEGER FileOffset OPTIONAL, 
              IN ULONG Length, 
              OUT PIO_STATUS_BLOCK IoStatus)
{
    PBCB Bcb;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT ("CcFlushCache(SectionObjectPointers 0x%p, FileOffset 0x%p, Length %d, IoStatus 0x%p)\n",
            SectionObjectPointers, FileOffset, Length, IoStatus);

    if (SectionObjectPointers && SectionObjectPointers->SharedCacheMap)
    {
        Bcb = (PBCB) SectionObjectPointers->SharedCacheMap;
        ASSERT (Bcb);

        Status = MmFlushDataFileSection ((PSECTION_OBJECT)Bcb->Section, FileOffset, Length);
    }
    if (IoStatus)
    {
        IoStatus->Status = Status;
        IoStatus->Status = STATUS_SUCCESS;
    }
}

/*
 * @implemented
 */
PFILE_OBJECT NTAPI
CcGetFileObjectFromSectionPtrs (IN PSECTION_OBJECT_POINTERS SectionObjectPointers)
{
    PBCB Bcb;
    if (SectionObjectPointers && SectionObjectPointers->SharedCacheMap)
    {
        Bcb = (PBCB) SectionObjectPointers->SharedCacheMap;
        ASSERT (Bcb);
        return Bcb->FileObject;
    }
    return NULL;
}

NTSTATUS
CcTrimMemory (ULONG Target, ULONG Priority, PULONG NrFreedPages)
{   
   DPRINT1("Trim function for cache memory is not implemented yet.\n");

   (*NrFreedPages) = 0;
   
   return STATUS_SUCCESS;
}


VOID INIT_FUNCTION NTAPI
CcInitView (VOID)
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    ULONG i;
    ULONG Size;
    PVOID Base;

    DPRINT ("CcInitView()\n");

    ExInitializeFastMutex (&CcCacheViewLock);

    ExInitializeNPagedLookasideList (&iBcbLookasideList, NULL, NULL, 0, sizeof (INTERNAL_BCB), TAG_IBCB, 20);
    ExInitializeNPagedLookasideList (&BcbLookasideList, NULL, NULL, 0, sizeof (BCB), TAG_BCB, 20);

    InitializeListHead (&CcFreeCacheViewListHead);
    InitializeListHead (&CcInUseCacheViewListHead);

    BoundaryAddressMultiple.QuadPart = 0LL;

    Size = MmSystemRangeStart >= (PVOID) 0xC0000000 ? 0x18000000 : 0x20000000;
    CcCacheViewBase = (PVOID) (0xF0000000 - Size);
    MmLockAddressSpace (MmGetKernelAddressSpace ());

    Status = MmCreateMemoryArea (MmGetKernelAddressSpace (),
                                 MEMORY_AREA_CACHE_SEGMENT,
                                 &CcCacheViewBase, Size, 0, &CcCacheViewMemoryArea, FALSE, FALSE, BoundaryAddressMultiple);
    MmUnlockAddressSpace (MmGetKernelAddressSpace ());
    DPRINT ("CcCacheViewBase: %x\n", CcCacheViewBase);
    if (!NT_SUCCESS (Status))
    {
        KeBugCheck(CACHE_MANAGER);
    }
    CcCacheViewArray = ExAllocatePool (NonPagedPool, sizeof (CACHE_VIEW) * (Size / CACHE_VIEW_SIZE));
    if (CcCacheViewArray == NULL)
    {
        KeBugCheck(CACHE_MANAGER);
    }

    Base = CcCacheViewBase;
    CcCacheViewArrayCount = Size / CACHE_VIEW_SIZE;
    for (i = 0; i < CcCacheViewArrayCount; i++)
    {
        CcCacheViewArray[i].BaseAddress = Base;
        CcCacheViewArray[i].RefCount = 0;
        CcCacheViewArray[i].Bcb = NULL;
        CcCacheViewArray[i].SectionData.ViewOffset = 0;
        InsertTailList (&CcFreeCacheViewListHead, &CcCacheViewArray[i].ListEntry);
        Base = (PVOID) ((ULONG_PTR) Base + CACHE_VIEW_SIZE);
    }
    
    MmInitializeMemoryConsumer(MC_CACHE, CcTrimMemory);
    CcInitCacheZeroPage ();

}

VOID NTAPI
CcInitializeCacheMap (IN PFILE_OBJECT FileObject,
                      IN PCC_FILE_SIZES FileSizes,
                      IN BOOLEAN PinAccess, 
                      IN PCACHE_MANAGER_CALLBACKS CallBacks, 
                      IN PVOID LazyWriterContext)
{
    PBCB Bcb;
    NTSTATUS Status;

    DPRINT ("CcInitializeCacheMap(), %wZ\n", &FileObject->FileName);
    DPRINT ("%I64x (%I64d)\n", FileSizes->FileSize.QuadPart, FileSizes->FileSize.QuadPart);

    ASSERT (FileObject);
    ASSERT (FileSizes);

    ExAcquireFastMutex (&CcCacheViewLock);

    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
    if (Bcb == NULL)
    {
        Bcb = ExAllocateFromNPagedLookasideList (&BcbLookasideList);
        if (Bcb == NULL)
        {
            KeBugCheck(CACHE_MANAGER);
        }
        memset (Bcb, 0, sizeof (BCB));

        Bcb->FileObject = FileObject;
        Bcb->FileSizes = *FileSizes;
        Bcb->PinAccess = PinAccess;
        Bcb->CallBacks = CallBacks;
        Bcb->LazyWriterContext = LazyWriterContext;
        Bcb->RefCount = 0;

        DPRINT ("%x %x\n", FileObject, FileSizes->FileSize.QuadPart);

        Status = MmCreateDataFileSection ((PSECTION_OBJECT*)&Bcb->Section,
                                          STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE,
                                          NULL, &Bcb->FileSizes.FileSize, PAGE_READWRITE, SEC_COMMIT, Bcb->FileObject, TRUE);
        if (!NT_SUCCESS (Status))
        {
            DPRINT1 ("%x\n", Status);
            KeBugCheck(CACHE_MANAGER);
        }

        FileObject->SectionObjectPointer->SharedCacheMap = Bcb;
    }

    if (FileObject->PrivateCacheMap == NULL)
    {
        FileObject->PrivateCacheMap = Bcb;
        Bcb->RefCount++;
    }

    ExReleaseFastMutex (&CcCacheViewLock);
    DPRINT ("CcInitializeCacheMap() done\n");
}

BOOLEAN NTAPI
CcUninitializeCacheMap (IN PFILE_OBJECT FileObject,
                        IN PLARGE_INTEGER TruncateSize OPTIONAL, 
                        IN PCACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent OPTIONAL)
{
    PBCB Bcb;
    ULONG i;
    NTSTATUS Status;

    DPRINT ("CcUninitializeCacheMap(), %wZ\n", &FileObject->FileName);
    ExAcquireFastMutex (&CcCacheViewLock);
    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
    if (Bcb)
    {
        if (FileObject->PrivateCacheMap == Bcb)
        {
            Bcb->RefCount--;
            FileObject->PrivateCacheMap = NULL;
        }
        if (Bcb->RefCount == 0)
        {
            Bcb->RefCount++;
            ExReleaseFastMutex (&CcCacheViewLock);
            MmFlushDataFileSection ((PSECTION_OBJECT)Bcb->Section, NULL, 0);
            ExAcquireFastMutex (&CcCacheViewLock);
            Bcb->RefCount--;
            if (Bcb->RefCount == 0)
            {
                for (i = 0; i < ROUND_UP (Bcb->FileSizes.AllocationSize.QuadPart, CACHE_VIEW_SIZE) / CACHE_VIEW_SIZE; i++)
                {
                    if (Bcb->CacheView[i] && Bcb->CacheView[i]->Bcb == Bcb)
                    {
                        if (Bcb->CacheView[i]->RefCount > 0)
                        {
                            KeBugCheck(CACHE_MANAGER);
                        }
                        Status = MmUnmapViewInSystemCache (Bcb->CacheView[i]);
                        if (!NT_SUCCESS (Status))
                        {
                            KeBugCheck(CACHE_MANAGER);
                        }
                        Bcb->CacheView[i]->RefCount = 0;
                        Bcb->CacheView[i]->Bcb = NULL;
                        Bcb->CacheView[i] = NULL;
                    }
                }

                DPRINT ("%x\n", Bcb->Section);
                ObDereferenceObject (Bcb->Section);
                FileObject->SectionObjectPointer->SharedCacheMap = NULL;
                ExFreeToNPagedLookasideList (&BcbLookasideList, Bcb);
            }
        }
    }
    DPRINT ("CcUninitializeCacheMap() done, %wZ\n", &FileObject->FileName);
    ExReleaseFastMutex (&CcCacheViewLock);
    return TRUE;
}



/* EOF */
