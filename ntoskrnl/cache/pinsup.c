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
//#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define TAG_MAP_SEC    TAG('C', 'c', 'S', 'x')
#define TAG_MAP_READ   TAG('M', 'c', 'p', 'y')
#define TAG_MAP_BCB    TAG('B', 'c', 'b', ' ')

NOCC_BCB CcCacheSections[CACHE_NUM_SECTIONS];
CHAR CcpBitmapBuffer[sizeof(RTL_BITMAP) + ROUND_UP((CACHE_NUM_SECTIONS), 32) / 8];
PRTL_BITMAP CcCacheBitmap = (PRTL_BITMAP)&CcpBitmapBuffer;
KGUARDED_MUTEX CcMutex;
KEVENT CcDeleteEvent;
ULONG CcCacheClockHand;
LONG CcOutstandingDeletes;

typedef struct _NOCC_UNMAP_CHAIN
{
    PVOID Buffer;
    PSECTION_OBJECT SectionObject;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER FileOffset;
    ULONG Length;
} NOCC_UNMAP_CHAIN, *PNOCC_UNMAP_CHAIN;
NOCC_UNMAP_CHAIN CcUnmapChain[CACHE_NUM_SECTIONS];

/* FUNCTIONS ******************************************************************/

VOID CcpUnlinkedFromFile
(PFILE_OBJECT FileObject, 
 PNOCC_BCB Bcb)
{
	PLIST_ENTRY Entry;

	for (Entry = ((PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap)->AssociatedBcb.Flink;
		 Entry != &((PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap)->AssociatedBcb;
		 Entry = Entry->Flink)
	{
		ASSERT(CONTAINING_RECORD(Entry, NOCC_BCB, ThisFileList) != Bcb);
	}
}

VOID CcpLock()
{
	KeAcquireGuardedMutex(&CcMutex);
	//DPRINT("<<<---<<< CC In Mutex!\n");
}

VOID CcpPerformUnmapWork()
{
	NOCC_UNMAP_CHAIN WorkingOn;
	IO_STATUS_BLOCK IoStatus;
	ULONG NumElements;

	KeAcquireGuardedMutex(&CcMutex);
	while (CcOutstandingDeletes > 0)
	{
		NumElements = InterlockedDecrement(&CcOutstandingDeletes);
		DPRINT1("Unmapping %d ...\n", NumElements);
		WorkingOn = CcUnmapChain[0];
		RtlMoveMemory(&CcUnmapChain[0], &CcUnmapChain[1], NumElements * sizeof(NOCC_UNMAP_CHAIN));
		KeReleaseGuardedMutex(&CcMutex);
		CcFlushCache
		    (WorkingOn.FileObject->SectionObjectPointer,
		     &WorkingOn.FileOffset,
		     WorkingOn.Length,
		     &IoStatus);
		DPRINT1("Status result from flush: %08x\n", IoStatus.Status);
		MmUnmapViewInSystemSpace(WorkingOn.Buffer);
		ObDereferenceObject(WorkingOn.SectionObject);
		DPRINT1("Done unmapping\n");
		KeAcquireGuardedMutex(&CcMutex);
	}
	KeReleaseGuardedMutex(&CcMutex);
}

VOID CcpUnlock()
{
	//DPRINT(">>>--->>> CC Exit Mutex!\n");
	KeReleaseGuardedMutex(&CcMutex);
}

VOID STDCALL
CcpUnmapThread(PVOID Unused)
{
	while (TRUE)
	{
		KeWaitForSingleObject(&CcDeleteEvent, UserRequest, KernelMode, FALSE, NULL);
		CcpPerformUnmapWork();
	}
}

PDEVICE_OBJECT
NTAPI
MmGetDeviceObjectForFile(IN PFILE_OBJECT FileObject);

VOID CcpUnmapSegment(ULONG Segment)
{
	PNOCC_BCB Bcb = &CcCacheSections[Segment];
	PNOCC_UNMAP_CHAIN UnmapWork = &CcUnmapChain[CcOutstandingDeletes];

	ASSERT(Bcb->RefCount > 0);
	DPRINT("CcpUnmapSegment(#%x)\n", Segment);

	InterlockedIncrement(&CcOutstandingDeletes);
	UnmapWork->Buffer = Bcb->BaseAddress;
	UnmapWork->SectionObject = Bcb->SectionObject;
	UnmapWork->FileObject = Bcb->FileObject;
	UnmapWork->FileOffset = Bcb->FileOffset;
	UnmapWork->Length = Bcb->Length;
	Bcb->BaseAddress = NULL;
	Bcb->Length = 0;
	Bcb->FileOffset.QuadPart = 0;

	KeSetEvent(&CcDeleteEvent, IO_DISK_INCREMENT, FALSE);
}

NTSTATUS CcpMapSegment(ULONG Start)
{
	PNOCC_BCB Bcb = &CcCacheSections[Start];
	ULONG ViewSize = CACHE_STRIPE;
	NTSTATUS Status;

	ASSERT(RtlTestBit(CcCacheBitmap, Start));
	DPRINT("CcpMapSegment(#%x)\n", Start);

	Status = MmMapViewInSystemSpaceAtOffset
		(Bcb->SectionObject,
		 &Bcb->BaseAddress,
		 &Bcb->FileOffset,
		 &ViewSize);

	if (!NT_SUCCESS(Status))
	{
		DPRINT("Failed to map view in system space: %x\n", Status);
		return Status;
	}

	DPRINT("System view is at %x\n", Bcb->BaseAddress);
	Bcb->Length = ViewSize;

	return TRUE;
}

NTSTATUS CcpAllocateSection
(PFILE_OBJECT FileObject, 
 ULONG Length,
 ULONG Protect, 
 PSECTION_OBJECT *Result)
{
	NTSTATUS Status;
	LARGE_INTEGER MaxSize;

	MaxSize.QuadPart = Length;

	Status = MmCreateSection
		((PVOID*)Result,
		 STANDARD_RIGHTS_REQUIRED,
		 NULL,
		 &MaxSize,
		 Protect,
		 SEC_RESERVE,
		 NULL,
		 FileObject);
		
	return Status;
}

/* Must have acquired the mutex */
VOID CcpDereferenceCache(ULONG Start)
{
	PNOCC_BCB Bcb;
	PNOCC_CACHE_MAP Map;

	DPRINT("CcpDereferenceCache(#%x)\n", Start);

	Bcb = &CcCacheSections[Start];
	DPRINT("Dereference #%x (count %d)\n", Start, Bcb->RefCount);
	ASSERT(Bcb->FileObject);
	ASSERT(Bcb->SectionObject);
	ASSERT(Bcb->RefCount == 1);
	
	Map = (PNOCC_CACHE_MAP)Bcb->FileObject->SectionObjectPointer->SharedCacheMap;
	Map->NumberOfMaps--;

	DPRINT("Fully unreferencing Bcb #%x\n", Start);
	CcpUnmapSegment(Start);
	
	RemoveEntryList(&Bcb->ThisFileList);
	
	Bcb->FileObject = NULL;
	Bcb->SectionObject = NULL;
	Bcb->BaseAddress = NULL;
	Bcb->FileOffset.QuadPart = 0;
	Bcb->Length = 0;
	Bcb->RefCount = 0;
}

/* Needs mutex */
ULONG CcpAllocateCacheSections
(PFILE_OBJECT FileObject, 
 PSECTION_OBJECT SectionObject, 
 PLARGE_INTEGER FileOffset)
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

	DPRINT("Allocating Cache Section (File already has %d sections)\n", Map->NumberOfMaps);

	i = RtlFindClearBitsAndSet
		(CcCacheBitmap, 1, CcCacheClockHand);
	CcCacheClockHand = i + 1;

	if (i != INVALID_CACHE)
	{
		DPRINT("Setting up Bcb #%x\n", i);

		Bcb = &CcCacheSections[i];
		
		ASSERT(Bcb->RefCount < 2);

		if (Bcb->RefCount > 0)
		{
			CcpDereferenceCache(i);
		}

		ASSERT(!Bcb->RefCount);

		Bcb->RefCount = 1;
		DPRINT("Bcb #%x RefCount %d\n", Bcb - CcCacheSections, Bcb->RefCount);
		ObReferenceObject(SectionObject);
		
		Bcb->FileObject = FileObject;
		Bcb->SectionObject = SectionObject;
		Bcb->FileOffset = *FileOffset;
		Bcb->Length = CACHE_STRIPE;
		Map->NumberOfMaps++;

		InsertTailList(&Map->AssociatedBcb, &Bcb->ThisFileList);

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

/*
 * Must have mutex
 */
ULONG CcpFindCacheFor(PNOCC_CACHE_MAP Map)
{
	if (!Map)
		return INVALID_CACHE;
	else 
	{
		if (IsListEmpty(&Map->AssociatedBcb)) 
			return INVALID_CACHE;
		PNOCC_BCB Bcb = CONTAINING_RECORD
			(Map->AssociatedBcb.Flink,
			 NOCC_BCB,
			 ThisFileList);
		return Bcb - CcCacheSections;
	}
}

/* Must have acquired the mutex */
VOID CcpReferenceCache(ULONG Start)
{
	PNOCC_BCB Bcb;
	Bcb = &CcCacheSections[Start];
	ASSERT(Bcb->FileObject);
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
	ASSERT(Bcb->FileObject);
	ASSERT(Bcb->SectionObject);
	Bcb->RefCount++;
	Bcb->Exclusive = TRUE;
	Bcb->ExclusiveWaiter--;
	RtlSetBit(CcCacheBitmap, Start);
	CcpUnlock();
}

/* Find a map that encompasses the target range */
/* Must have the mutex */
ULONG CcpFindMatchingMap(PNOCC_BCB Head, PLARGE_INTEGER FileOffset, ULONG Length)
{
	PLIST_ENTRY Entry;

	//DPRINT("Find Matching Map: %x:%x\n", FileOffset->LowPart, Length);

	if (FileOffset->QuadPart >= Head->FileOffset.QuadPart &&
		FileOffset->QuadPart + Length <= Head->FileOffset.QuadPart + Head->Length)
	{
		//DPRINT("Head matched\n");
		return Head - CcCacheSections;
	}

	for (Entry = Head->ThisFileList.Flink; Entry != &Head->ThisFileList; Entry = Entry->Flink)
	{
		PNOCC_BCB Bcb = CONTAINING_RECORD(Entry, NOCC_BCB, ThisFileList);
		//DPRINT("This File: %x:%x\n", Bcb->FileOffset.LowPart, Bcb->Length);
		if (FileOffset->QuadPart >= Bcb->FileOffset.QuadPart &&
			FileOffset->QuadPart + Length <= Bcb->FileOffset.QuadPart + Bcb->Length)
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
(IN PNOCC_CACHE_MAP Map,
 IN PLARGE_INTEGER FileOffset,
 IN ULONG Length,
 IN ULONG Flags,
 OUT PVOID *BcbResult,
 OUT PVOID *Buffer)
{
	BOOLEAN Success = FALSE;
	PFILE_OBJECT FileObject = Map->FileObject;
	/* Note: windows 2000 drivers treat this as a bool */
	BOOLEAN Wait = (Flags & MAP_WAIT) || (Flags == TRUE);
	LARGE_INTEGER Target;
	ULONG BcbHead;
	PNOCC_BCB Bcb = NULL;
	PSECTION_OBJECT SectionObject = NULL;
	NTSTATUS Status;

	DPRINT("CcMapData(F->%x,%x:%d)\n", Map, FileOffset->LowPart, Length);

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	Target.QuadPart = CACHE_ROUND_DOWN(FileOffset->QuadPart);

	CcpLock();

	/* Find out if any range is a superset of what we want */
	BcbHead = CcpFindCacheFor(Map);
	
	/* No bcbs for this file */
	if (BcbHead != INVALID_CACHE)
	{
		/* Find an accomodating section */
		//DPRINT("Selected BCB #%x\n", BcbHead);
		Bcb = &CcCacheSections[BcbHead];
		SectionObject = Bcb->SectionObject;
		BcbHead = CcpFindMatchingMap(Bcb, FileOffset, Length);
	
		if (BcbHead == INVALID_CACHE)
		{
			if (!Wait)
			{
				CcpUnlock();
				//DPRINT("End\n");
				goto cleanup;
			}
		}
		else
		{
			Bcb = &CcCacheSections[BcbHead];
			Success = TRUE;
			*BcbResult = Bcb;
			*Buffer = ((PCHAR)Bcb->BaseAddress) + (int)(FileOffset->QuadPart - Bcb->FileOffset.QuadPart);
			DPRINT
				("Bcb #%x Buffer maps (%x) At %x Length %x (Getting %x:%x)\n", 
				 Bcb - CcCacheSections,
				 Bcb->FileOffset.LowPart, 
				 Bcb->BaseAddress,
				 Bcb->Length,
				 *Buffer, 
				 Length);
			CcpUnlock();
			//DPRINT("w1n\n");
			goto cleanup;
		}
	}

	CcpUnlock();

	if (!SectionObject)
	{
		Status = CcpAllocateSection
			(FileObject,
			 CACHE_STRIPE,
			 PAGE_READWRITE,
			 &SectionObject);
		
		if (!NT_SUCCESS(Status))
		{
			//DPRINT("End %08x\n", Status);
			goto cleanup;
		}
	}

	/* Returns a reference */
	do 
	{
		CcpLock();
		
		BcbHead = CcpAllocateCacheSections
			(FileObject,
			 SectionObject, 
			 &Target);
		
		CcpUnlock();
	}
	while (BcbHead == INVALID_CACHE && 
		   NT_SUCCESS
		   (KeWaitForSingleObject
			(&CcDeleteEvent, UserRequest, KernelMode, FALSE, NULL)));

	if (BcbHead == INVALID_CACHE)
	{
		//DPRINT("End\n");
		goto cleanup;
	}

	//DPRINT("Selected BCB #%x\n", BcbHead);
	Bcb = &CcCacheSections[BcbHead];
	Status = CcpMapSegment(BcbHead);

	if (NT_SUCCESS(Status))
	{
		Success = TRUE;
		//DPRINT("w1n\n");
		*BcbResult = &CcCacheSections[BcbHead];
		*Buffer = ((PCHAR)Bcb->BaseAddress) + (int)(FileOffset->QuadPart - Bcb->FileOffset.QuadPart);
		DPRINT
			("Bcb #%x Buffer maps (%x) At %x Length %x (Getting %x:%x)\n", 
			 Bcb - CcCacheSections,
			 Bcb->FileOffset.LowPart, 
			 Bcb->BaseAddress,
			 Bcb->Length,
			 *Buffer, 
			 Length);
		goto cleanup;
	}

	//DPRINT("TERM!\n");

cleanup:
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
	if (!(PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap)
	{
		PNOCC_CACHE_MAP Map = ExAllocatePool(NonPagedPool, sizeof(NOCC_CACHE_MAP));
		FileObject->SectionObjectPointer->SharedCacheMap = Map;
		Map->FileObject = FileObject;
		InitializeListHead(&Map->AssociatedBcb);
	}

	return CcpMapData
		((PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap,
		 FileOffset,
		 Length,
		 Flags,
		 BcbResult,
		 Buffer);
}

BOOLEAN
NTAPI
CcPinMappedData(IN PFILE_OBJECT FileObject,
                IN PLARGE_INTEGER FileOffset,
                IN ULONG Length,
                IN ULONG Flags,
                IN OUT PVOID *Bcb)
{
	BOOLEAN Wait = Flags & PIN_WAIT;
	BOOLEAN Exclusive = Flags & PIN_EXCLUSIVE;
	BOOLEAN Result;
	ULONG BcbHead;
	PVOID Buffer;

	Result = CcMapData(FileObject, FileOffset, Length, Wait ? MAP_WAIT : 0, Bcb, &Buffer);
	BcbHead = ((PNOCC_BCB)*Bcb) - CcCacheSections;

	if (!Result) return FALSE;

	CcpLock();
	if (Exclusive)
	{
		DPRINT("Requesting #%x Exclusive\n", BcbHead);
		CcpMarkForExclusive(BcbHead);
	}
	else
		CcpReferenceCache(BcbHead);
	CcpUnlock();

	if (Exclusive)
		CcpReferenceCacheExclusive(BcbHead);

	return TRUE;
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
	return CcMapData(FileObject, FileOffset, Length, Flags, Bcb, Buffer);
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
	BOOLEAN GotIt = CcPinMappedData
		(FileObject,
		 FileOffset,
		 Length,
		 Flags,
		 Bcb);

	PNOCC_BCB TheBcb = (PNOCC_BCB)*Bcb;
	ULONG Start = TheBcb - CcCacheSections;
	
	DPRINT("CcPreparePinWrite(#%x)\n", Start);
	
	if (GotIt)
	{
		CcCacheSections[Start].Dirty = TRUE;
		*Buffer = (PVOID)((PCHAR)TheBcb->BaseAddress + (FileOffset->QuadPart - TheBcb->FileOffset.QuadPart));
		DPRINT("Returning Buffer: %x\n", *Buffer);
	}

	DPRINT("Done\n");
	return GotIt;
}

VOID
NTAPI
CcUnpinData(IN PVOID Bcb)
{
	PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;
	ULONG Selected = RealBcb - CcCacheSections;
	DPRINT("CcUnpinData Bcb #%x (RefCount %d)\n", Selected, RealBcb->RefCount);
	CcpLock();
	if (RealBcb->RefCount <= 2)
	{
		DPRINT("Unset allocation bit #%x\n", Selected);
		RtlClearBit(CcCacheBitmap, Selected);
		RealBcb->Exclusive = FALSE;
		if (RealBcb->ExclusiveWaiter)
		{
			DPRINT("Triggering exclusive waiter\n");
			KeSetEvent(&RealBcb->ExclusiveWait, IO_NO_INCREMENT, FALSE);
		}
	}
	if (RealBcb->RefCount > 1)
	{
		DPRINT("Removing one reference #%x\n", Selected);
		RealBcb->RefCount--;
		ASSERT(RealBcb->RefCount);
	}
	CcpUnlock();
	KeSetEvent(&CcDeleteEvent, IO_DISK_INCREMENT, FALSE);
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
