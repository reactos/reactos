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
#include "newcc.h"
#include "section/newmm.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PFSN_PREFETCHER_GLOBALS CcPfGlobals;
extern LONG CcOutstandingDeletes;
extern KEVENT CcpLazyWriteEvent;
extern KEVENT CcFinalizeEvent;
extern VOID NTAPI CcpUnmapThread(PVOID Unused);
extern VOID NTAPI CcpLazyWriteThread(PVOID Unused);
HANDLE CcUnmapThreadHandle, CcLazyWriteThreadHandle;
CLIENT_ID CcUnmapThreadId, CcLazyWriteThreadId;

typedef struct _NOCC_PRIVATE_CACHE_MAP
{
	LIST_ENTRY ListEntry;
	PFILE_OBJECT FileObject;
	PNOCC_CACHE_MAP Map;
} NOCC_PRIVATE_CACHE_MAP, *PNOCC_PRIVATE_CACHE_MAP;

LIST_ENTRY CcpAllSharedCacheMaps;

/* FUNCTIONS ******************************************************************/

// Interact with legacy balance manager for now
// This can fall away when our section implementation supports
// demand paging properly
NTSTATUS
CcRosTrimCache(ULONG Target, ULONG Priority, PULONG NrFreed)
{
	ULONG i, Freed, BcbHead;

	*NrFreed = 0;

	for (i = 0; i < CACHE_NUM_SECTIONS; i++) {
		BcbHead = (i+CcCacheClockHand) % CACHE_NUM_SECTIONS;

		// Reference a cache stripe so it won't go away
		CcpLock();
		if (CcCacheSections[BcbHead].BaseAddress) {
			CcpReferenceCache(BcbHead);
			CcpUnlock();
		} else {
			CcpUnlock();
			continue;
		}
		
		// Defer to MM to try recovering pages from it
		Freed = MiCacheEvictPages
			(CcCacheSections[BcbHead].BaseAddress, Target);

		Target -= Freed;
		*NrFreed += Freed;

		CcpLock();
		CcpUnpinData(&CcCacheSections[BcbHead], TRUE);
		CcpUnlock();
	}

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

	InitializeListHead(&CcpAllSharedCacheMaps);

	KeInitializeEvent(&CcDeleteEvent, SynchronizationEvent, FALSE);
	KeInitializeEvent(&CcFinalizeEvent, SynchronizationEvent, FALSE);
	KeInitializeEvent(&CcpLazyWriteEvent, SynchronizationEvent, FALSE);

	CcCacheBitmap->Buffer = ((PULONG)&CcCacheBitmap[1]);
	CcCacheBitmap->SizeOfBitMap = ROUND_UP(CACHE_NUM_SECTIONS, 32);
	DPRINT("Cache has %d entries\n", CcCacheBitmap->SizeOfBitMap);
	ExInitializeFastMutex(&CcMutex);

	// MM stub
	KeInitializeEvent(&MmWaitPageEvent, SynchronizationEvent, FALSE);

	// Until we're fully demand paged, we can do things the old way through
	// the balance manager
	MmInitializeMemoryConsumer(MC_CACHE, CcRosTrimCache);

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

BOOLEAN
NTAPI
CcpAcquireFileLock(PNOCC_CACHE_MAP Map)
{
	DPRINT("Calling AcquireForLazyWrite: %x\n", Map->LazyContext);
	return Map->Callbacks.AcquireForLazyWrite(Map->LazyContext, TRUE);
}

VOID
NTAPI
CcpReleaseFileLock(PNOCC_CACHE_MAP Map)
{
	DPRINT("Releasing Lazy Write %x\n", Map->LazyContext);
	Map->Callbacks.ReleaseFromLazyWrite(Map->LazyContext);
}

// Must have CcpLock()
PFILE_OBJECT CcpFindOtherStreamFileObject(PFILE_OBJECT FileObject)
{
	PLIST_ENTRY Entry, Private;
	for (Entry = CcpAllSharedCacheMaps.Flink;
		 Entry != &CcpAllSharedCacheMaps;
		 Entry = Entry->Flink)
	{
		// 'Identical' test for other stream file object
		PNOCC_CACHE_MAP Map = CONTAINING_RECORD(Entry, NOCC_CACHE_MAP, Entry);
		for (Private = Map->PrivateCacheMaps.Flink;
			 Private != &Map->PrivateCacheMaps;
			 Private = Private->Flink)
		{
			PNOCC_PRIVATE_CACHE_MAP PrivateMap = CONTAINING_RECORD(Private, NOCC_PRIVATE_CACHE_MAP, ListEntry);
			if (PrivateMap->FileObject->Flags & FO_STREAM_FILE &&
				PrivateMap->FileObject->DeviceObject == FileObject->DeviceObject &&
				PrivateMap->FileObject->Vpb == FileObject->Vpb &&
				PrivateMap->FileObject->FsContext == FileObject->FsContext &&
				PrivateMap->FileObject->FsContext2 == FileObject->FsContext2 && 
				1)
			{
				return PrivateMap->FileObject;
			}
		}
	}
	return 0;
}

// Thanks: http://windowsitpro.com/Windows/Articles/ArticleID/3864/pg/2/2.html

VOID
NTAPI
CcInitializeCacheMap(IN PFILE_OBJECT FileObject,
                     IN PCC_FILE_SIZES FileSizes,
                     IN BOOLEAN PinAccess,
                     IN PCACHE_MANAGER_CALLBACKS Callbacks,
                     IN PVOID LazyWriteContext)
{
	PNOCC_CACHE_MAP Map = FileObject->SectionObjectPointer->SharedCacheMap;
	PNOCC_PRIVATE_CACHE_MAP PrivateCacheMap = FileObject->PrivateCacheMap;

    CcpLock();
	if (!Map && FileObject->Flags & FO_STREAM_FILE)
	{
		PFILE_OBJECT IdenticalStreamFileObject = 
			CcpFindOtherStreamFileObject(FileObject);
		if (IdenticalStreamFileObject)
			Map = IdenticalStreamFileObject->SectionObjectPointer->SharedCacheMap;
		if (Map)
		{
			DPRINT1
				("Linking SFO %x to previous SFO %x through cache map %x #\n",
				 FileObject, IdenticalStreamFileObject, Map);
		}
	}
    if (!Map)
    {
		DPRINT("Initializing file object for (%p) %wZ\n", FileObject, &FileObject->FileName);
		Map = ExAllocatePool(NonPagedPool, sizeof(NOCC_CACHE_MAP));
		FileObject->SectionObjectPointer->SharedCacheMap = Map;
		Map->FileSizes = *FileSizes;
		Map->LazyContext = LazyWriteContext;
		Map->ReadAheadGranularity = PAGE_SIZE;
		RtlCopyMemory(&Map->Callbacks, Callbacks, sizeof(*Callbacks));
		// For now ...
		DPRINT("FileSizes->ValidDataLength %08x%08x\n", FileSizes->ValidDataLength.HighPart, FileSizes->ValidDataLength.LowPart);
		InitializeListHead(&Map->AssociatedBcb);
		InitializeListHead(&Map->PrivateCacheMaps);
		InsertTailList(&CcpAllSharedCacheMaps, &Map->Entry);
		DPRINT("New Map %x\n", Map);
    }
	if (!PrivateCacheMap)
	{
		PrivateCacheMap = ExAllocatePool(NonPagedPool, sizeof(*PrivateCacheMap));
		FileObject->PrivateCacheMap = PrivateCacheMap;
		PrivateCacheMap->FileObject = FileObject;
		ObReferenceObject(PrivateCacheMap->FileObject);
	}

	PrivateCacheMap->Map = Map;
	InsertTailList(&Map->PrivateCacheMaps, &PrivateCacheMap->ListEntry);

    CcpUnlock();
}

ULONG
NTAPI
CcpCountCacheSections(IN PNOCC_CACHE_MAP Map)
{
	PLIST_ENTRY Entry;
	ULONG Count;

	for (Count = 0, Entry = Map->AssociatedBcb.Flink; Entry != &Map->AssociatedBcb; Entry = Entry->Flink, Count++);

	return Count;
}

BOOLEAN
NTAPI
CcUninitializeCacheMap(IN PFILE_OBJECT FileObject,
                       IN OPTIONAL PLARGE_INTEGER TruncateSize,
                       IN OPTIONAL PCACHE_UNINITIALIZE_EVENT UninitializeEvent)
{
	BOOLEAN LastMap = FALSE;
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
	PNOCC_PRIVATE_CACHE_MAP PrivateCacheMap = FileObject->PrivateCacheMap;
    
    DPRINT("Uninitializing file object for %wZ SectionObjectPointer %x\n", &FileObject->FileName, FileObject->SectionObjectPointer);

    ASSERT(UninitializeEvent == NULL);

	if (Map)
		CcpFlushCache(Map, NULL, 0, NULL, FALSE);

	CcpLock();
	if (PrivateCacheMap)
	{
		ASSERT(!Map || Map == PrivateCacheMap->Map);
		ASSERT(PrivateCacheMap->FileObject == FileObject);

		RemoveEntryList(&PrivateCacheMap->ListEntry);
		if (IsListEmpty(&PrivateCacheMap->Map->PrivateCacheMaps))
		{
			while (!IsListEmpty(&Map->AssociatedBcb))
			{
				PNOCC_BCB Bcb = CONTAINING_RECORD(Map->AssociatedBcb.Flink, NOCC_BCB, ThisFileList);
				DPRINT("Evicting cache stripe #%x\n", Bcb - CcCacheSections);
				Bcb->RefCount = 1;
				CcpDereferenceCache(Bcb - CcCacheSections, TRUE);
			}
			RemoveEntryList(&PrivateCacheMap->Map->Entry);
			ExFreePool(PrivateCacheMap->Map);
			FileObject->SectionObjectPointer->SharedCacheMap = NULL;
			LastMap = TRUE;
		}
		ObDereferenceObject(PrivateCacheMap->FileObject);
		FileObject->PrivateCacheMap = NULL;
		ExFreePool(PrivateCacheMap);
	}
	CcpUnlock();

	DPRINT("Uninit complete\n");

    return LastMap;
}

VOID
NTAPI
CcSetFileSizes(IN PFILE_OBJECT FileObject,
               IN PCC_FILE_SIZES FileSizes)
{
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
    if (!Map) return;
    Map->FileSizes = *FileSizes;
	PNOCC_BCB Bcb = Map->AssociatedBcb.Flink == &Map->AssociatedBcb ? 
		NULL : CONTAINING_RECORD(Map->AssociatedBcb.Flink, NOCC_BCB, ThisFileList);
	if (!Bcb) return;
	MmExtendCacheSection(Bcb->SectionObject, &FileSizes->FileSize, FALSE);
	DPRINT("FileSizes->FileSize %x\n", FileSizes->FileSize.LowPart);
	DPRINT("FileSizes->AllocationSize %x\n", FileSizes->AllocationSize.LowPart);
    DPRINT("FileSizes->ValidDataLength %x\n", FileSizes->ValidDataLength.LowPart);
}

BOOLEAN
NTAPI
CcGetFileSizes
(IN PFILE_OBJECT FileObject,
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
	PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap;
	if (!Map) return TRUE;
	CcpFlushCache(Map, NULL, 0, NULL, TRUE);
	return TRUE;
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
    PNOCC_BCB Bcb = NULL;
	PLIST_ENTRY ListEntry = NULL;
	LARGE_INTEGER LowerBound = *StartOffset;
	LARGE_INTEGER UpperBound = *EndOffset;
	LARGE_INTEGER Target, End;
	PVOID PinnedBcb, PinnedBuffer;
	PNOCC_CACHE_MAP Map = FileObject->SectionObjectPointer->SharedCacheMap;

	DPRINT
		("S %08x%08x E %08x%08x\n",
		 StartOffset->u.HighPart, StartOffset->u.LowPart,
		 EndOffset->u.HighPart, EndOffset->u.LowPart);

	if (!Map)
	{
		NTSTATUS Status;
		IO_STATUS_BLOCK IOSB;
		PCHAR ZeroBuf = ExAllocatePool(PagedPool, PAGE_SIZE);
		ULONG ToWrite;

		if (!ZeroBuf) RtlRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
		DPRINT1("RtlZeroMemory(%x,%x)\n", ZeroBuf, PAGE_SIZE);
		RtlZeroMemory(ZeroBuf, PAGE_SIZE);

		Target.QuadPart = PAGE_ROUND_DOWN(LowerBound.QuadPart);
		End.QuadPart = PAGE_ROUND_UP(UpperBound.QuadPart);

		// Handle leading page
		if (LowerBound.QuadPart != Target.QuadPart)
		{
			ToWrite = MIN(UpperBound.QuadPart - LowerBound.QuadPart, (PAGE_SIZE - LowerBound.QuadPart) & (PAGE_SIZE - 1));
			DPRINT("Zero last half %08x%08x %x\n", Target.u.HighPart, Target.u.LowPart, ToWrite);
			Status = MiSimpleRead(FileObject, &Target, ZeroBuf, PAGE_SIZE, &IOSB);
			if (!NT_SUCCESS(Status)) 
			{
				ExFreePool(ZeroBuf);
				RtlRaiseStatus(Status);
			}
			DPRINT1("RtlZeroMemory(%x,%x)\n", ZeroBuf + LowerBound.QuadPart - Target.QuadPart, ToWrite);
			RtlZeroMemory(ZeroBuf + LowerBound.QuadPart - Target.QuadPart, ToWrite);
			Status = MiSimpleWrite(FileObject, &Target, ZeroBuf, MIN(PAGE_SIZE,UpperBound.QuadPart-Target.QuadPart), &IOSB);
			if (!NT_SUCCESS(Status))
			{
				ExFreePool(ZeroBuf);
				RtlRaiseStatus(Status);
			}
			Target.QuadPart += PAGE_SIZE;
		}

		DPRINT1("RtlZeroMemory(%x,%x)\n", ZeroBuf, PAGE_SIZE);
		RtlZeroMemory(ZeroBuf, PAGE_SIZE);
		
		while (UpperBound.QuadPart - Target.QuadPart > PAGE_SIZE)
		{
			DPRINT("Zero full page %08x%08x\n", Target.u.HighPart, Target.u.LowPart);
			Status = MiSimpleWrite(FileObject, &Target, ZeroBuf, PAGE_SIZE, &IOSB);
			if (!NT_SUCCESS(Status))
			{
				ExFreePool(ZeroBuf);
				RtlRaiseStatus(Status);
			}
			Target.QuadPart += PAGE_SIZE;
		}

		if (UpperBound.QuadPart > Target.QuadPart)
		{
			ToWrite = UpperBound.QuadPart - Target.QuadPart;
			DPRINT("Zero first half %08x%08x %x\n", Target.u.HighPart, Target.u.LowPart, ToWrite);
			Status = MiSimpleRead(FileObject, &Target, ZeroBuf, PAGE_SIZE, &IOSB);
			if (!NT_SUCCESS(Status)) 
			{
				ExFreePool(ZeroBuf);
				RtlRaiseStatus(Status);
			}
			DPRINT1("RtlZeroMemory(%x,%x)\n", ZeroBuf, ToWrite);
			RtlZeroMemory(ZeroBuf, ToWrite);
			Status = MiSimpleWrite(FileObject, &Target, ZeroBuf, MIN(PAGE_SIZE, UpperBound.QuadPart-Target.QuadPart), &IOSB);
			if (!NT_SUCCESS(Status))
			{
				ExFreePool(ZeroBuf);
				RtlRaiseStatus(Status);
			}
			Target.QuadPart += PAGE_SIZE;
		}

		ExFreePool(ZeroBuf);
		return TRUE;
	}
	
	CcpLock();
	ListEntry = Map->AssociatedBcb.Flink;

	while (ListEntry != &Map->AssociatedBcb)
	{
		Bcb = CONTAINING_RECORD(ListEntry, NOCC_BCB, ThisFileList);
		CcpReferenceCache(Bcb - CcCacheSections);

		if (Bcb->FileOffset.QuadPart + Bcb->Length >= LowerBound.QuadPart &&
			Bcb->FileOffset.QuadPart < UpperBound.QuadPart)
		{
			DPRINT
				("Bcb #%x (@%08x%08x)\n", 
				 Bcb - CcCacheSections, 
				 Bcb->FileOffset.u.HighPart, Bcb->FileOffset.u.LowPart);

			Target.QuadPart = MAX(Bcb->FileOffset.QuadPart, LowerBound.QuadPart);
			End.QuadPart = MIN(Map->FileSizes.ValidDataLength.QuadPart, UpperBound.QuadPart);
			End.QuadPart = MIN(End.QuadPart, Bcb->FileOffset.QuadPart + Bcb->Length);
			CcpUnlock();

			if (!CcPreparePinWrite
				(FileObject, 
				 &Target,
				 End.QuadPart - Target.QuadPart,
				 TRUE, 
				 Wait,
				 &PinnedBcb,
				 &PinnedBuffer))
			{
				return FALSE;
			}

			ASSERT(PinnedBcb == Bcb);

			CcpLock();
			ListEntry = ListEntry->Flink;
			// Return from pin state
			CcpUnpinData(PinnedBcb, TRUE);
		}

		CcpUnpinData(Bcb, TRUE);
	}

	CcpUnlock();

	return TRUE;
}

PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrs(IN PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
	PFILE_OBJECT Result = NULL;
	PNOCC_CACHE_MAP Map = SectionObjectPointer->SharedCacheMap;
	CcpLock();
	if (!IsListEmpty(&Map->AssociatedBcb))
	{
		PNOCC_BCB Bcb = CONTAINING_RECORD(Map->AssociatedBcb.Flink, NOCC_BCB, ThisFileList);
		Result = MmGetFileObjectForSection((PROS_SECTION_OBJECT)Bcb->SectionObject);
	}
	CcpUnlock();
    return Result;
}

PFILE_OBJECT
NTAPI
CcGetFileObjectFromBcb(PVOID Bcb)
{
	PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;
	DPRINT("BCB #%x\n", RealBcb - CcCacheSections);
	return MmGetFileObjectForSection((PROS_SECTION_OBJECT)RealBcb->SectionObject);
}

/* EOF */
