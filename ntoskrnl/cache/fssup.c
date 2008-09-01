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
//#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PFSN_PREFETCHER_GLOBALS CcPfGlobals;
extern LONG CcOutstandingDeletes;
extern KEVENT CcpLazyWriteEvent;
extern VOID STDCALL CcpUnmapThread(PVOID Unused);
extern VOID STDCALL CcpLazyWriteThread(PVOID Unused);
HANDLE CcUnmapThreadHandle, CcLazyWriteThreadHandle;
CLIENT_ID CcUnmapThreadId, CcLazyWriteThreadId;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
CcInitializeCacheManager(VOID)
{
	int i;
	NTSTATUS Status;

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
	Status = PsCreateSystemThread
		(&CcUnmapThreadHandle,
		 THREAD_ALL_ACCESS,
		 NULL,
		 NULL,
		 &CcUnmapThreadId,
		 (PKSTART_ROUTINE) CcpUnmapThread,
		 NULL);

	if (!NT_SUCCESS(Status))
	{
		KEBUGCHECK(0);
	}

#if 0
	Status = PsCreateSystemThread
	    (&CcLazyWriteThreadHandle,
	     THREAD_ALL_ACCESS,
	     NULL,
	     NULL,
	     &CcLazyWriteThreadId,
	     (PKSTART_ROUTINE) CcpLazyWriteThread,
	     NULL);
#endif

	if (!NT_SUCCESS(Status))
	{
		KEBUGCHECK(0);
	}

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
    DPRINT("Initializing file object for %wZ\n", &FileObject->FileName);
    CcpLock();
    if (FileObject->SectionObjectPointer->SharedCacheMap)
    {
	PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
	InterlockedIncrement((PLONG)&Map->RefCount);
    }
    else
    {
	PNOCC_CACHE_MAP Map = ExAllocatePool(NonPagedPool, sizeof(NOCC_CACHE_MAP));
	FileObject->SectionObjectPointer->SharedCacheMap = Map;
	Map->RefCount = 1;
	ObReferenceObject(FileObject);
	Map->FileObject = FileObject;
	Map->NumberOfMaps = 0;
	Map->FileSizes = *FileSizes;
	DPRINT("FileSizes->ValidDataLength %x\n", FileSizes->ValidDataLength.LowPart);
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
    IO_STATUS_BLOCK IOSB;
    
    DPRINT("Uninitializing file object for %wZ SectionObjectPointer %x\n", &FileObject->FileName, FileObject->SectionObjectPointer);
    
    ASSERT(UninitializeEvent == NULL);

    for (Entry = Map->AssociatedBcb.Flink;
	 Entry != &Map->AssociatedBcb;
	 Entry = Entry->Flink)
    {
	Bcb = CONTAINING_RECORD(Entry, NOCC_BCB, ThisFileList);

	if (!Bcb->Dirty) continue;

	if (Bcb->RefCount == 1)
	{
	    DPRINT("Flushing #%x\n", Bcb - CcCacheSections);
	    CcFlushCache
		(Bcb->FileObject->SectionObjectPointer,
		 &Bcb->FileOffset,
		 Bcb->Length,
		 &IOSB);
	}
    }
    
    CcpLock();

    if (InterlockedDecrement((PLONG)&Map->RefCount) == 1)
    {
	for (Entry = Map->AssociatedBcb.Flink;
	     Entry != &Map->AssociatedBcb;
	     Entry = Entry->Flink)
	{
	    Bcb = CONTAINING_RECORD(Entry, NOCC_BCB, ThisFileList);
	    if (Bcb->RefCount == 1)
	    {
		DPRINT("Unmapping #%x\n", Bcb - CcCacheSections);
		CcpDereferenceCache(Bcb - CcCacheSections);
	    }
	}
	
	ObDereferenceObject(Map->FileObject);

	ExFreePool(Map);
	
	/* Clear the cache map */
	FileObject->SectionObjectPointer->SharedCacheMap = NULL;
    }
    
    CcpUnlock();
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
