/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/fssup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Art Yerkes
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PFSN_PREFETCHER_GLOBALS CcPfGlobals;
extern LONG CcOutstandingDeletes;
extern ULONG CcCacheClockHand;

/* FUNCTIONS ******************************************************************/

VOID MmPrintMemoryStatistic(VOID);

NTSTATUS
CcTrimPages
(ULONG Target,
 ULONG Priority,
 PULONG NrFreed)
{
	PCHAR PageAddress;
	PNOCC_BCB Bcb;
	PNOCC_CACHE_MAP Map;
	PFN_TYPE Page;
	ULONG ClockHand;
	BOOLEAN WasDirty;

	*NrFreed = 0;
	DPRINT1("Balance: Free cache pages\n");
	CcpLock();
	ClockHand = CcCacheClockHand;
	do
	{
		Bcb = &CcCacheSections[ClockHand];
		ClockHand = (ClockHand+1) % CACHE_NUM_SECTIONS;
		DPRINT1("Bcb #%x -> Base %x RefCount %d Pinned %x\n", 
				Bcb - CcCacheSections,
				Bcb->BaseAddress,
				Bcb->RefCount,
				Bcb->Pinned);
		if (Bcb->RefCount && Bcb->BaseAddress && !Bcb->Pinned)
		{
			Map = (PNOCC_CACHE_MAP)
				Bcb->FileObject->SectionObjectPointer->SharedCacheMap;
			for (PageAddress = Bcb->BaseAddress;
				 *NrFreed < Target && 
				 PageAddress < ((PCHAR)Bcb->BaseAddress) + CACHE_STRIPE;
				 PageAddress += PAGE_SIZE)
			{
				MmLockAddressSpace(MmGetKernelAddressSpace());
				if (MmIsPagePresent(NULL, PageAddress) && 
					!MmIsDirtyPage(NULL, PageAddress))
				{
					MmDeleteVirtualMapping
						(NULL, PageAddress, FALSE, &WasDirty, &Page);
				}
				MmUnlockAddressSpace(MmGetKernelAddressSpace());
				MmReleasePageMemoryConsumer(MC_CACHE, Page);
				(*NrFreed)++;
			}
		}
	}
	while (*NrFreed < Target && ClockHand != CcCacheClockHand);
	CcpUnlock();
	DPRINT1("Balance: Freed %d cache pages\n", *NrFreed);
	MmPrintMemoryStatistic();

	return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
CcInitializeCacheManager(VOID)
{
	int i;

	DPRINT("Initialize\n");
	for (i = 0; i < CACHE_NUM_SECTIONS; i++)
	{
		KeInitializeEvent(&CcCacheSections[i].ExclusiveWait, SynchronizationEvent, FALSE);
		InitializeListHead(&CcCacheSections[i].ThisFileList);
	}

	KeInitializeEvent(&CcDeleteEvent, SynchronizationEvent, FALSE);
	KeInitializeEvent(&CcpLazyWriteEvent, SynchronizationEvent, FALSE);
	CcCacheBitmap->Buffer = ((PULONG)&CcCacheBitmap[1]);
	CcCacheBitmap->SizeOfBitMap = ROUND_UP(CACHE_NUM_SECTIONS, 32);
	DPRINT("Cache has %d entries\n", CcCacheBitmap->SizeOfBitMap);
	ExInitializeFastMutex(&CcMutex);
	MmInitializeMemoryConsumer(MC_CACHE, CcTrimPages);

    return TRUE;
}

VOID
NTAPI
CcPfInitializePrefetcher(VOID)
{
    /* Notify debugger */
    DbgPrintEx(DPFLTR_PREFETCHER_ID,
               DPFLTR_TRACE_LEVEL,
               "CCPF: InitializePrefetecher()\n");

    /* Setup the Prefetcher Data */
    InitializeListHead(&CcPfGlobals.ActiveTraces);
    InitializeListHead(&CcPfGlobals.CompletedTraces);
    ExInitializeFastMutex(&CcPfGlobals.CompletedTracesLock);

    /* FIXME: Setup the rest of the prefetecher */
}

VOID
NTAPI
CcInitializeCacheMap(IN PFILE_OBJECT FileObject,
                     IN PCC_FILE_SIZES FileSizes,
                     IN BOOLEAN PinAccess,
                     IN PCACHE_MANAGER_CALLBACKS Callbacks,
                     IN PVOID LazyWriteContext)
{
    CcpLock();
    if (FileObject->SectionObjectPointer->SharedCacheMap)
    {
		PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
		InterlockedIncrement((PLONG)&Map->RefCount);
    }
    else
    {
		PNOCC_CACHE_MAP Map = ExAllocatePool(NonPagedPool, sizeof(NOCC_CACHE_MAP));
		DPRINT1("Initializing file object for (%p) %wZ\n", FileObject, &FileObject->FileName);
		ASSERT(FileObject);
		FileObject->SectionObjectPointer->SharedCacheMap = Map;
		Map->RefCount = 1;
		ObReferenceObject(FileObject);
		Map->FileObject = FileObject;
		Map->NumberOfMaps = 0;
		Map->FileSizes = *FileSizes;
		InitializeListHead(&Map->AssociatedBcb);
    }
    CcpUnlock();
}

BOOLEAN
NTAPI
CcUninitializeCacheMap(IN PFILE_OBJECT FileObject,
                       IN OPTIONAL PLARGE_INTEGER TruncateSize,
                       IN OPTIONAL PCACHE_UNINITIALIZE_EVENT UninitializeEvent)
{
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
    PNOCC_BCB Bcb;
    PLIST_ENTRY Entry;
    
    DPRINT("Uninitializing file object for %wZ SectionObjectPointer %x\n", &FileObject->FileName, FileObject->SectionObjectPointer);
	
    ASSERT(UninitializeEvent == NULL);
	
    if (!Map) return TRUE;
	    
	CcpLock();
    if (InterlockedDecrement((PLONG)&Map->RefCount) == 1)
    {
		for (Entry = Map->AssociatedBcb.Flink;
			 Entry != &Map->AssociatedBcb;
			 Entry = Entry->Flink)
		{
			Bcb = CONTAINING_RECORD(Entry, NOCC_BCB, ThisFileList);
			DPRINT("Unmapping #%x\n", Bcb - CcCacheSections);
			CcpDereferenceCache(Bcb - CcCacheSections);
		}
		
		ObDereferenceObject(Map->FileObject);
		ExFreePool(Map);	
	
		/* Clear the cache map */
		FileObject->SectionObjectPointer->SharedCacheMap = NULL;
    }
	CcpUnlock();

	MmPrintMemoryStatistic();
    return TRUE;
}

VOID
NTAPI
CcSetFileSizes(IN PFILE_OBJECT FileObject,
               IN PCC_FILE_SIZES FileSizes)
{
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
    if (!Map) return;
    Map->FileSizes = *FileSizes;
    DPRINT("FileSizes->ValidDataLength %x\n", FileSizes->ValidDataLength.LowPart);
}

BOOLEAN
NTAPI
CcGetFileSizes(IN PFILE_OBJECT FileObject,
	       IN PCC_FILE_SIZES FileSizes)
{
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
    if (!Map) return FALSE;
    *FileSizes = Map->FileSizes;
    return TRUE;
}

BOOLEAN
NTAPI
CcPurgeCacheSection(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                    IN OPTIONAL PLARGE_INTEGER FileOffset,
                    IN ULONG Length,
                    IN BOOLEAN UninitializeCacheMaps)
{
	IO_STATUS_BLOCK IOSB;
	PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap;
	CcFlushCache(SectionObjectPointer, FileOffset, Length, &IOSB);
	if (UninitializeCacheMaps)
	    CcUninitializeCacheMap(Map->FileObject, NULL, NULL);
	return NT_SUCCESS(IOSB.Status);
}

VOID
NTAPI
CcSetDirtyPageThreshold(IN PFILE_OBJECT FileObject,
                        IN ULONG DirtyPageThreshold)
{
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
CcZeroData(IN PFILE_OBJECT FileObject,
           IN PLARGE_INTEGER StartOffset,
           IN PLARGE_INTEGER EndOffset,
           IN BOOLEAN Wait)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrs(IN PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
    return ((PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap)->FileObject;
}

PFILE_OBJECT
NTAPI
CcGetFileObjectFromBcb(IN PVOID Bcb)
{
    PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;
    return RealBcb->FileObject;
}

/* EOF */
