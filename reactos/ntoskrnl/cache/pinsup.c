/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/pinsup.c
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

/* The following is a test mode that only works with modified filesystems.
 * it maps the cache sections read only until they're pinned writable, and then
 * turns them readonly again when they're unpinned. 
 * This helped me determine that a certain bug was not a memory overwrite. */
//#define PIN_WRITE_ONLY

/* GLOBALS ********************************************************************/

#define TAG_MAP_SEC    TAG('C', 'c', 'S', 'x')
#define TAG_MAP_READ   TAG('M', 'c', 'p', 'y')
#define TAG_MAP_BCB    TAG('B', 'c', 'b', ' ')

NOCC_BCB CcCacheSections[CACHE_NUM_SECTIONS];
CHAR CcpBitmapBuffer[sizeof(RTL_BITMAP) + ROUND_UP((CACHE_NUM_SECTIONS), 32) / 8];
PRTL_BITMAP CcCacheBitmap = (PRTL_BITMAP)&CcpBitmapBuffer;
FAST_MUTEX CcMutex;
KEVENT CcDeleteEvent;
KEVENT CcFinalizeEvent;
ULONG CcCacheClockHand;
LONG CcOutstandingDeletes;

/* FUNCTIONS ******************************************************************/

PETHREAD LastThread;
VOID _CcpLock(const char *file, int line)
{
    //DPRINT("<<<---<<< CC In Mutex(%s:%d %x)!\n", file, line, PsGetCurrentThread());
    ExAcquireFastMutex(&CcMutex);
}

VOID _CcpUnlock(const char *file, int line)
{
    ExReleaseFastMutex(&CcMutex);
    //DPRINT(">>>--->>> CC Exit Mutex!\n", file, line);
}

PDEVICE_OBJECT
NTAPI
MmGetDeviceObjectForFile(IN PFILE_OBJECT FileObject);

NTSTATUS CcpAllocateSection
(PFILE_OBJECT FileObject, 
 ULONG Length,
 ULONG Protect, 
 PROS_SECTION_OBJECT *Result)
{
    NTSTATUS Status;
    LARGE_INTEGER MaxSize;

    MaxSize.QuadPart = Length;

	DPRINT("Making Section for File %x\n", FileObject);
	DPRINT("File name %wZ\n", &FileObject->FileName);
    Status = MmCreateSection
	 ((PVOID*)Result,
	 STANDARD_RIGHTS_REQUIRED,
	 NULL,
	 &MaxSize,
	 Protect,
	 SEC_RESERVE | SEC_CACHE,
	 NULL,
	 FileObject);
		
    return Status;
}

typedef struct _WORK_QUEUE_WITH_CONTEXT 
{ 
	WORK_QUEUE_ITEM WorkItem; 
	PVOID ToUnmap; 
	LARGE_INTEGER FileOffset;
	LARGE_INTEGER MapSize;
	PROS_SECTION_OBJECT ToDeref;
	PACQUIRE_FOR_LAZY_WRITE AcquireForLazyWrite;
	PRELEASE_FROM_LAZY_WRITE ReleaseFromLazyWrite;
	PVOID LazyContext;
	BOOLEAN Dirty;
} WORK_QUEUE_WITH_CONTEXT, *PWORK_QUEUE_WITH_CONTEXT;

VOID
CcpUnmapCache(PVOID Context)
{
	PWORK_QUEUE_WITH_CONTEXT WorkItem = (PWORK_QUEUE_WITH_CONTEXT)Context;
	DPRINT("Unmapping (finally) %x\n", WorkItem->ToUnmap);
	MmUnmapCacheViewInSystemSpace(WorkItem->ToUnmap);
	ObDereferenceObject(WorkItem->ToDeref);
	ExFreePool(WorkItem);
	DPRINT("Done\n");
}

/* Must have acquired the mutex */
VOID CcpDereferenceCache(ULONG Start, BOOLEAN Immediate)
{
	PVOID ToUnmap;
    PNOCC_BCB Bcb;
	BOOLEAN Dirty;
	LARGE_INTEGER MappedSize;
	LARGE_INTEGER BaseOffset;
	PWORK_QUEUE_WITH_CONTEXT WorkItem;

    DPRINT("CcpDereferenceCache(#%x)\n", Start);

    Bcb = &CcCacheSections[Start];

	Dirty = Bcb->Dirty;
	ToUnmap = Bcb->BaseAddress;
	BaseOffset = Bcb->FileOffset;
	MappedSize = Bcb->Map->FileSizes.ValidDataLength;

    DPRINT("Dereference #%x (count %d)\n", Start, Bcb->RefCount);
    ASSERT(Bcb->SectionObject);
    ASSERT(Bcb->RefCount == 1);
	
	DPRINT("Firing work item for %x\n", Bcb->BaseAddress);

	if (Immediate)
	{
		PROS_SECTION_OBJECT ToDeref = Bcb->SectionObject;
		Bcb->Map = NULL;
		Bcb->SectionObject = NULL;
		Bcb->BaseAddress = NULL;
		Bcb->FileOffset.QuadPart = 0;
		Bcb->Length = 0;
		Bcb->RefCount = 0;
		Bcb->Dirty = FALSE;
		RemoveEntryList(&Bcb->ThisFileList);

		CcpUnlock();
		if (Dirty)
			MiFlushMappedSection(ToUnmap, &BaseOffset, &MappedSize, Dirty);
		MmUnmapCacheViewInSystemSpace(ToUnmap);
		ObDereferenceObject(ToDeref);
		CcpLock();
	}
	else
	{
		WorkItem = ExAllocatePool(NonPagedPool, sizeof(*WorkItem));
		if (!WorkItem) KeBugCheck(0);
		WorkItem->ToUnmap = Bcb->BaseAddress;
		WorkItem->FileOffset = Bcb->FileOffset;
		WorkItem->Dirty = Bcb->Dirty;
		WorkItem->MapSize = MappedSize;
		WorkItem->ToDeref = Bcb->SectionObject;
		WorkItem->AcquireForLazyWrite = Bcb->Map->Callbacks.AcquireForLazyWrite;
		WorkItem->ReleaseFromLazyWrite = Bcb->Map->Callbacks.ReleaseFromLazyWrite;
		WorkItem->LazyContext = Bcb->Map->LazyContext;
		
		ExInitializeWorkItem(((PWORK_QUEUE_ITEM)WorkItem), (PWORKER_THREAD_ROUTINE)CcpUnmapCache, WorkItem);
	
		Bcb->Map = NULL;
		Bcb->SectionObject = NULL;
		Bcb->BaseAddress = NULL;
		Bcb->FileOffset.QuadPart = 0;
		Bcb->Length = 0;
		Bcb->RefCount = 0;
		Bcb->Dirty = FALSE;
		RemoveEntryList(&Bcb->ThisFileList);

		CcpUnlock();
		ExQueueWorkItem((PWORK_QUEUE_ITEM)WorkItem, DelayedWorkQueue);
		CcpLock();
	}
	DPRINT("Done\n");
}

/* Needs mutex */
ULONG CcpAllocateCacheSections
(PFILE_OBJECT FileObject, 
 PROS_SECTION_OBJECT SectionObject)
{
    ULONG i = INVALID_CACHE;
    PNOCC_CACHE_MAP Map;
    PNOCC_BCB Bcb;
	
    DPRINT("AllocateCacheSections: FileObject %x\n", FileObject);
	
    if (!FileObject->SectionObjectPointer)
	return INVALID_CACHE;

    Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;

    if (!Map)
	return INVALID_CACHE;

    DPRINT("Allocating Cache Section\n");

    i = RtlFindClearBitsAndSet(CcCacheBitmap, 1, CcCacheClockHand);
    CcCacheClockHand = (i + 1) % CACHE_NUM_SECTIONS;

    if (i != INVALID_CACHE)
    {
	DPRINT("Setting up Bcb #%x\n", i);

	Bcb = &CcCacheSections[i];
		
	ASSERT(Bcb->RefCount < 2);

	if (Bcb->RefCount > 0)
	{
	    CcpDereferenceCache(i, FALSE);
	}

	ASSERT(!Bcb->RefCount);
	Bcb->RefCount = 1;

	DPRINT("Bcb #%x RefCount %d\n", Bcb - CcCacheSections, Bcb->RefCount);
	
	if (!RtlTestBit(CcCacheBitmap, i))
	{
	    DPRINT("Somebody stoeled BCB #%x\n", i);
	}
	ASSERT(RtlTestBit(CcCacheBitmap, i));
	
	DPRINT("Allocated #%x\n", i);
	ASSERT(CcCacheSections[i].RefCount);
    }
    else
    {
	DPRINT("Failed to allocate cache segment\n");
    }
    return i;
}

/* Must have acquired the mutex */
VOID CcpReferenceCache(ULONG Start)
{
    PNOCC_BCB Bcb;
    Bcb = &CcCacheSections[Start];
    ASSERT(Bcb->SectionObject);
    Bcb->RefCount++;
    RtlSetBit(CcCacheBitmap, Start);

}

VOID CcpMarkForExclusive(ULONG Start)
{
    PNOCC_BCB Bcb;
    Bcb = &CcCacheSections[Start];
    Bcb->ExclusiveWaiter++;
}

/* Must not have the mutex */
VOID CcpReferenceCacheExclusive(ULONG Start)
{
    PNOCC_BCB Bcb = &CcCacheSections[Start];

    KeWaitForSingleObject(&Bcb->ExclusiveWait, Executive, KernelMode, FALSE, NULL);
    CcpLock();
    ASSERT(Bcb->ExclusiveWaiter);
    ASSERT(Bcb->SectionObject);
    Bcb->Exclusive = TRUE;
    Bcb->ExclusiveWaiter--;
    RtlSetBit(CcCacheBitmap, Start);
    CcpUnlock();
}

/* Find a map that encompasses the target range */
/* Must have the mutex */
ULONG CcpFindMatchingMap(PLIST_ENTRY Head, PLARGE_INTEGER FileOffset, ULONG Length)
{
	PLIST_ENTRY Entry;
    //DPRINT("Find Matching Map: (%x) %x:%x\n", FileOffset->LowPart, Length);
    for (Entry = Head->Flink; Entry != Head; Entry = Entry->Flink)
    {
		//DPRINT("Link @%x\n", Entry);
		PNOCC_BCB Bcb = CONTAINING_RECORD(Entry, NOCC_BCB, ThisFileList);
		//DPRINT("Selected BCB %x #%x\n", Bcb, Bcb - CcCacheSections);
		//DPRINT("This File: %x:%x\n", Bcb->FileOffset.LowPart, Bcb->Length);
		if (FileOffset->QuadPart >= Bcb->FileOffset.QuadPart &&
			FileOffset->QuadPart < Bcb->FileOffset.QuadPart + CACHE_STRIPE)
		{
			//DPRINT("Found match at #%x\n", Bcb - CcCacheSections);
			return Bcb - CcCacheSections;
		}
    }

    //DPRINT("This region isn't mapped\n");

    return INVALID_CACHE;
}

BOOLEAN
NTAPI
CcpMapData
(IN PFILE_OBJECT FileObject,
 IN PLARGE_INTEGER FileOffset,
 IN ULONG Length,
 IN ULONG Flags,
 OUT PVOID *BcbResult,
 OUT PVOID *Buffer)
{
    BOOLEAN Success = FALSE, FaultIn = FALSE;
    /* Note: windows 2000 drivers treat this as a bool */
    //BOOLEAN Wait = (Flags & MAP_WAIT) || (Flags == TRUE);
    LARGE_INTEGER Target, EndInterval;
	ULONG BcbHead, SectionSize, ViewSize;
    PNOCC_BCB Bcb = NULL;
    PROS_SECTION_OBJECT SectionObject = NULL;
    NTSTATUS Status;
	PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
	ViewSize = CACHE_STRIPE;

	if (!Map)
	{
		DPRINT1("File object was not mapped\n");
		return FALSE;
	}

    DPRINT("CcMapData(F->%x,%08x%08x:%d)\n", FileObject, FileOffset->HighPart, FileOffset->LowPart, Length);

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    Target.HighPart = FileOffset->HighPart;
	Target.LowPart = CACHE_ROUND_DOWN(FileOffset->LowPart);

    CcpLock();

    /* Find out if any range is a superset of what we want */
	/* Find an accomodating section */
	BcbHead = CcpFindMatchingMap(&Map->AssociatedBcb, FileOffset, Length);
	
	if (BcbHead != INVALID_CACHE)
	{
		Bcb = &CcCacheSections[BcbHead];
		Success = TRUE;
		*BcbResult = Bcb;
		*Buffer = ((PCHAR)Bcb->BaseAddress) + (int)(FileOffset->QuadPart - Bcb->FileOffset.QuadPart);
		DPRINT
			("Bcb #%x Buffer maps (%08x%08x) At %x Length %x (Getting %x:%x) %wZ\n", 
			 Bcb - CcCacheSections,
			 Bcb->FileOffset.HighPart,
			 Bcb->FileOffset.LowPart, 
			 Bcb->BaseAddress,
			 Bcb->Length,
			 *Buffer, 
			 Length,
			 &FileObject->FileName);
		DPRINT("w1n\n");
		goto cleanup;
	}

	DPRINT("File size %08x%08x\n", Map->FileSizes.ValidDataLength.HighPart, Map->FileSizes.ValidDataLength.LowPart);
	
	if (Map->FileSizes.ValidDataLength.QuadPart)
	{
		SectionSize = min(CACHE_STRIPE, Map->FileSizes.ValidDataLength.QuadPart - Target.QuadPart);
	}
	else
	{
		SectionSize = CACHE_STRIPE;
	}

	DPRINT("Allocating a cache stripe at %x:%d\n",
		   Target.LowPart, SectionSize);
	//ASSERT(SectionSize <= CACHE_STRIPE);

	CcpUnlock();
	Status = CcpAllocateSection
		(FileObject,
		 SectionSize,
#ifdef PIN_WRITE_ONLY
		 PAGE_READONLY,
#else
		 PAGE_READWRITE,
#endif
		 &SectionObject);
	CcpLock();

	if (!NT_SUCCESS(Status))
	{
		*BcbResult = NULL;
		*Buffer = NULL;
		DPRINT1("End %08x\n", Status);
		goto cleanup;
	}
	
retry:
    /* Returns a reference */
	DPRINT("Allocating cache sections: %wZ\n", &FileObject->FileName);	
	BcbHead = CcpAllocateCacheSections(FileObject, SectionObject);
	if (BcbHead == INVALID_CACHE)
	{
		ULONG i;
		DbgPrint("Cache Map:");
		for (i = 0; i < CACHE_NUM_SECTIONS; i++)
		{
			if (!(i % 64)) DbgPrint("\n");
			DbgPrint("%c", CcCacheSections[i].RefCount + (RtlTestBit(CcCacheBitmap, i) ? '@' : '`'));
		}
		DbgPrint("\n");
		KeWaitForSingleObject(&CcDeleteEvent, Executive, KernelMode, FALSE, NULL);
		goto retry;
	}

	DPRINT("BcbHead #%x (final)\n", BcbHead);

    if (BcbHead == INVALID_CACHE)
    {
		*BcbResult = NULL;
		*Buffer = NULL;
		DPRINT1("End\n");
		goto cleanup;
    }
	
    DPRINT("Selected BCB #%x\n", BcbHead);
	ViewSize = CACHE_STRIPE;

    Bcb = &CcCacheSections[BcbHead];
	Status = MmMapCacheViewInSystemSpaceAtOffset
		(SectionObject->Segment,
		 &Bcb->BaseAddress,
		 &Target,
		 &ViewSize);
	
    if (!NT_SUCCESS(Status))
    {
		*BcbResult = NULL;
		*Buffer = NULL;
		ObDereferenceObject(SectionObject);
		RemoveEntryList(&Bcb->ThisFileList);
		RtlZeroMemory(Bcb, sizeof(*Bcb));
		RtlClearBit(CcCacheBitmap, BcbHead);
		DPRINT1("Failed to map\n");
		goto cleanup;
	}
	
	Success = TRUE;
	//DPRINT("w1n\n");

	Bcb->Length = MIN(Map->FileSizes.ValidDataLength.QuadPart - Target.QuadPart, CACHE_STRIPE);
	Bcb->SectionObject = SectionObject;
	Bcb->Map = Map;
	Bcb->FileOffset = Target;
	InsertTailList(&Map->AssociatedBcb, &Bcb->ThisFileList);
	
	*BcbResult = &CcCacheSections[BcbHead];
	*Buffer = ((PCHAR)Bcb->BaseAddress) + (int)(FileOffset->QuadPart - Bcb->FileOffset.QuadPart);
	FaultIn = TRUE;

	DPRINT
		("Bcb #%x Buffer maps (%08x%08x) At %x Length %x (Getting %x:%x) %wZ\n", 
		 Bcb - CcCacheSections,
			 Bcb->FileOffset.HighPart,
		 Bcb->FileOffset.LowPart, 
		 Bcb->BaseAddress,
		 Bcb->Length,
		 *Buffer, 
		 Length,
		 &FileObject->FileName);

	EndInterval.QuadPart = Bcb->FileOffset.QuadPart + Bcb->Length - 1;
	ASSERT((EndInterval.QuadPart & ~(CACHE_STRIPE - 1)) == (Bcb->FileOffset.QuadPart & ~(CACHE_STRIPE - 1)));
	
    //DPRINT("TERM!\n");
	
cleanup:
	CcpUnlock();
	if (Success) 
	{
		if (FaultIn) 
		{
			// Fault in the pages.  This forces reads to happen now.
			ULONG i;
			PCHAR FaultIn = Bcb->BaseAddress;
			DPRINT
				("Faulting in pages at this point: file %wZ %08x%08x:%x\n",
				 &FileObject->FileName,
				 Bcb->FileOffset.HighPart,
				 Bcb->FileOffset.LowPart,
				 Bcb->Length);
			for (i = 0; i < Bcb->Length; i += PAGE_SIZE) 
			{
				FaultIn[i] ^= 0;
			}
		}
		ASSERT(Bcb >= CcCacheSections && Bcb < (CcCacheSections + CACHE_NUM_SECTIONS));
	}
	else
	{
		ASSERT(FALSE);
	}

    return Success;
}

BOOLEAN
NTAPI
CcMapData
(IN PFILE_OBJECT FileObject,
 IN PLARGE_INTEGER FileOffset,
 IN ULONG Length,
 IN ULONG Flags,
 OUT PVOID *BcbResult,
 OUT PVOID *Buffer)
{
    BOOLEAN Result;

	Result = CcpMapData
		(FileObject,
		 FileOffset,
		 Length,
		 Flags,
		 BcbResult,
		 Buffer);
	
	if (Result)
	{
		PNOCC_BCB Bcb = (PNOCC_BCB)*BcbResult;
		ASSERT(Bcb >= CcCacheSections && Bcb < CcCacheSections + CACHE_NUM_SECTIONS);
		ASSERT(Bcb->BaseAddress);
		CcpLock();
		CcpReferenceCache(Bcb - CcCacheSections);
		CcpUnlock();
	}

	return Result;
}

BOOLEAN
NTAPI
CcpPinMappedData(IN PNOCC_CACHE_MAP Map,
				 IN PLARGE_INTEGER FileOffset,
				 IN ULONG Length,
				 IN ULONG Flags,
				 IN OUT PVOID *Bcb)
{
    BOOLEAN Exclusive = Flags & PIN_EXCLUSIVE;
    ULONG BcbHead;
    PNOCC_BCB TheBcb;

    CcpLock();

	ASSERT(Map->AssociatedBcb.Flink == &Map->AssociatedBcb || (CONTAINING_RECORD(Map->AssociatedBcb.Flink, NOCC_BCB, ThisFileList) >= CcCacheSections && CONTAINING_RECORD(Map->AssociatedBcb.Flink, NOCC_BCB, ThisFileList) < CcCacheSections + CACHE_NUM_SECTIONS));
	BcbHead = CcpFindMatchingMap(&Map->AssociatedBcb, FileOffset, Length);
	if (BcbHead == INVALID_CACHE)
	{
		CcpUnlock();
		return FALSE;
	}

	TheBcb = &CcCacheSections[BcbHead];

    if (Exclusive)
    {
		DPRINT("Requesting #%x Exclusive\n", BcbHead);
		CcpMarkForExclusive(BcbHead);
    }
    else
    {
		DPRINT("Reference #%x\n", BcbHead);
		CcpReferenceCache(BcbHead);
    }

    if (Exclusive)
		CcpReferenceCacheExclusive(BcbHead);

	CcpUnlock();

	*Bcb = TheBcb;
    return TRUE;
}

BOOLEAN
NTAPI
CcPinMappedData(IN PFILE_OBJECT FileObject,
				IN PLARGE_INTEGER FileOffset,
				IN ULONG Length,
				IN ULONG Flags,
				IN OUT PVOID *Bcb)
{
	PVOID Buffer;
	PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;

	if (!Map)
	{
		DPRINT1("Not cached\n");
		return FALSE;
	}
	
	if (CcpMapData(FileObject, FileOffset, Length, Flags, Bcb, &Buffer))
	{
		return CcpPinMappedData(Map, FileOffset, Length, Flags, Bcb);
	}
	else
	{
		DPRINT1("could not map\n");
		return FALSE;
	}
}

BOOLEAN
NTAPI
CcPinRead(IN PFILE_OBJECT FileObject,
          IN PLARGE_INTEGER FileOffset,
          IN ULONG Length,
          IN ULONG Flags,
          OUT PVOID *Bcb,
          OUT PVOID *Buffer)
{
    PNOCC_BCB RealBcb;
    BOOLEAN Result;

    Result = CcPinMappedData
	(FileObject, 
	 FileOffset,
	 Length,
	 Flags,
	 Bcb);

    if (Result)
    {
		CcpLock();
		RealBcb = *Bcb;
		*Buffer = ((PCHAR)RealBcb->BaseAddress) + (int)(FileOffset->QuadPart - RealBcb->FileOffset.QuadPart);
		CcpUnlock();
    }

    return Result;
}

BOOLEAN
NTAPI
CcPreparePinWrite(IN PFILE_OBJECT FileObject,
                  IN PLARGE_INTEGER FileOffset,
                  IN ULONG Length,
                  IN BOOLEAN Zero,
                  IN ULONG Flags,
                  OUT PVOID *Bcb,
                  OUT PVOID *Buffer)
{
    BOOLEAN Result;
    PNOCC_BCB RealBcb;
#ifdef PIN_WRITE_ONLY
	PVOID BaseAddress;
	SIZE_T NumberOfBytes;
	ULONG OldProtect;
#endif

	DPRINT("CcPreparePinWrite(%x:%x)\n", Buffer, Length);

    Result = CcPinRead
		(FileObject,
		 FileOffset,
		 Length,
		 Flags,
		 Bcb,
		 Buffer);
	
    if (Result)
    {
		CcpLock();
		RealBcb = *Bcb;

#ifdef PIN_WRITE_ONLY
		BaseAddress = RealBcb->BaseAddress;
		NumberOfBytes = RealBcb->Length;

		MiProtectVirtualMemory
			(NULL, 
			 &BaseAddress,
			 &NumberOfBytes,
			 PAGE_READWRITE,
			 &OldProtect);
#endif

		CcpUnlock();
		RealBcb->Dirty = TRUE;

		if (Zero)
		{
			DPRINT
				("Zero fill #%x %08x%08x:%x Buffer %x %wZ\n",
				 RealBcb - CcCacheSections,
				 FileOffset->u.HighPart,
				 FileOffset->u.LowPart,
				 Length,
				 *Buffer,
				 &FileObject->FileName);

			DPRINT1("RtlZeroMemory(%x,%x)\n", *Buffer, Length);
			RtlZeroMemory(*Buffer, Length);
		}
	}

    return Result;
}

BOOLEAN
NTAPI
CcpUnpinData(IN PNOCC_BCB RealBcb, BOOLEAN ReleaseBit)
{
    if (RealBcb->RefCount <= 2)
    {
		RealBcb->Exclusive = FALSE;
		if (RealBcb->ExclusiveWaiter)
		{
			DPRINT("Triggering exclusive waiter\n");
			KeSetEvent(&RealBcb->ExclusiveWait, IO_NO_INCREMENT, FALSE);
			return TRUE;
		}
    }
	if (RealBcb->RefCount == 2 && !ReleaseBit)
		return FALSE;
    if (RealBcb->RefCount > 1)
    {
		DPRINT("Removing one reference #%x\n", RealBcb - CcCacheSections);
		RealBcb->RefCount--;
		KeSetEvent(&CcDeleteEvent, IO_DISK_INCREMENT, FALSE);
    }
	if (RealBcb->RefCount == 1)
	{
		DPRINT("Clearing allocation bit #%x\n", RealBcb - CcCacheSections);

		RtlClearBit(CcCacheBitmap, RealBcb - CcCacheSections);

#ifdef PIN_WRITE_ONLY
		PVOID BaseAddress = RealBcb->BaseAddress;
		SIZE_T NumberOfBytes = RealBcb->Length;
		ULONG OldProtect;

		MiProtectVirtualMemory
			(NULL,
			 &BaseAddress,
			 &NumberOfBytes,
			 PAGE_READONLY,
			 &OldProtect);
#endif
	}

	return TRUE;
}

VOID
NTAPI
CcUnpinData(IN PVOID Bcb)
{
    PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;
    ULONG Selected = RealBcb - CcCacheSections;
	BOOLEAN Released;

	ASSERT(RealBcb >= CcCacheSections && RealBcb - CcCacheSections < CACHE_NUM_SECTIONS);
    DPRINT("CcUnpinData Bcb #%x (RefCount %d)\n", Selected, RealBcb->RefCount);

    CcpLock();
	Released = CcpUnpinData(RealBcb, FALSE);
    CcpUnlock();

	if (!Released) {
		MiFlushMappedSection(RealBcb->BaseAddress, &RealBcb->FileOffset, &RealBcb->Map->FileSizes.FileSize, RealBcb->Dirty);
		CcpLock();
		CcpUnpinData(RealBcb, TRUE);
		CcpUnlock();
	}
}

VOID
NTAPI
CcSetBcbOwnerPointer(IN PVOID Bcb,
                     IN PVOID OwnerPointer)
{
    PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;
    CcpLock();
    CcpReferenceCache(RealBcb - CcCacheSections);
    RealBcb->OwnerPointer = OwnerPointer;
    CcpUnlock();	
}

VOID
NTAPI
CcUnpinDataForThread(IN PVOID Bcb,
                     IN ERESOURCE_THREAD ResourceThreadId)
{
    CcUnpinData(Bcb);
}

/* EOF */
