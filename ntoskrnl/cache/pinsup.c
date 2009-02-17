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
FAST_MUTEX CcMutex;
KEVENT CcDeleteEvent;
ULONG CcCacheClockHand;

typedef struct _NOCC_UNMAP_CHAIN
{
    PVOID Buffer;
	PMEMORY_AREA MemoryArea;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER FileOffset;
    ULONG Length;
} NOCC_UNMAP_CHAIN, *PNOCC_UNMAP_CHAIN;
NOCC_UNMAP_CHAIN CcUnmapChain[CACHE_NUM_SECTIONS];

/* FUNCTIONS ******************************************************************/

VOID MmPrintMemoryStatistic(VOID);

VOID CcpLock()
{
    //DPRINT("<<<---<<< CC In Mutex! (from %x)\n", __builtin_return_address(0));
    ExAcquireFastMutex(&CcMutex);
}

VOID CcpUnlock()
{
    ExReleaseFastMutex(&CcMutex);
    //DPRINT(">>>--->>> CC Exit Mutex!\n");
}

PDEVICE_OBJECT
NTAPI
MmGetDeviceObjectForFile(IN PFILE_OBJECT FileObject);

VOID CcpFreeAreaPage
(PVOID Context, 
 PMEMORY_AREA MemoryArea,
 PVOID Address,
 PFN_TYPE Page,
 SWAPENTRY SwapEntry,
 BOOLEAN Dirty)
{
	NTSTATUS Status;
	PNOCC_BCB Bcb = (PNOCC_BCB)Context;
	LARGE_INTEGER ReadOffset;
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)Bcb->FileObject->SectionObjectPointer->SharedCacheMap;

	if (!Page) return;

	ReadOffset.QuadPart = 
		Bcb->FileOffset.QuadPart + 
		((PCHAR)Address - (PCHAR)Bcb->BaseAddress);
		
	if (Dirty && ReadOffset.QuadPart < Map->FileSizes.FileSize.QuadPart)
	{
		Status = MiScheduleForWrite
			(Bcb->FileObject,
			 &ReadOffset,
			 Page,
			 (ULONG)
			 (Map->FileSizes.FileSize.QuadPart - 
			  ReadOffset.QuadPart));
		if (NT_SUCCESS(Status))
		{
			DPRINT1("We failed to write a page!\n");
		}
	}
	MmReleasePageMemoryConsumer(MC_CACHE, Page);
}

VOID CcpUnmapSegment(ULONG Segment)
{
    PNOCC_BCB Bcb = &CcCacheSections[Segment];

	MmLockAddressSpace(MmGetKernelAddressSpace());
	MmFreeMemoryArea
		(MmGetKernelAddressSpace(),
		 Bcb->MemoryArea,
		 CcpFreeAreaPage,
		 Bcb);
	MmUnlockAddressSpace(MmGetKernelAddressSpace());
}

NTSTATUS CcpMapSegment(ULONG Start)
{
    PNOCC_BCB Bcb = &CcCacheSections[Start];
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(RtlTestBit(CcCacheBitmap, Start));
    DPRINT("CcpMapSegment(#%x)\n", Start);

    DPRINT("System view is at %x\n", Bcb->BaseAddress);

	return Status;
}

NTSTATUS CcpAllocateSection
(ULONG Protect, 
 PVOID *BaseAddress,
 PMEMORY_AREA *Result)
{
    NTSTATUS Status;
	LARGE_INTEGER BoundaryAddressMultiple;
	BoundaryAddressMultiple.QuadPart = 0;
	MmLockAddressSpace(MmGetKernelAddressSpace());
	*BaseAddress = 0;
	Status = MmCreateMemoryArea
		(MmGetKernelAddressSpace(),
		 MEMORY_AREA_CACHE_SEGMENT,
		 BaseAddress,
		 CACHE_STRIPE + 2 * PAGE_SIZE,
		 Protect,
		 Result,
		 FALSE,
		 0,
		 BoundaryAddressMultiple);
	MmUnlockAddressSpace(MmGetKernelAddressSpace());
	*BaseAddress = ((PCHAR)*BaseAddress) + PAGE_SIZE;
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
    ASSERT(Bcb->RefCount == 1);
	
    Map = (PNOCC_CACHE_MAP)Bcb->FileObject->SectionObjectPointer->SharedCacheMap;
    Map->NumberOfMaps--;

    DPRINT("Fully unreferencing Bcb #%x\n", Start);
    CcpUnmapSegment(Start);
	
    RemoveEntryList(&Bcb->ThisFileList);
	
	ObDereferenceObject(Bcb->FileObject);
    Bcb->FileObject = NULL;
    Bcb->BaseAddress = NULL;
    Bcb->FileOffset.QuadPart = 0;
    Bcb->Length = 0;
    Bcb->RefCount = 0;

	MmPrintMemoryStatistic();
}

/* Needs mutex */
ULONG CcpAllocateCacheSections
(PFILE_OBJECT FileObject, 
 PLARGE_INTEGER FileOffset)
{
    ULONG i = INVALID_CACHE;
	NTSTATUS Status;
    PNOCC_CACHE_MAP Map;
    PNOCC_BCB Bcb;
	PVOID BaseAddress;
	PMEMORY_AREA Area;
	
    DPRINT("AllocateCacheSections: FileObject %x\n", FileObject);
	
    if (!FileObject->SectionObjectPointer)
		return INVALID_CACHE;

    Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;

    if (!Map)
		return INVALID_CACHE;

    DPRINT("Allocating Cache Section (File already has %d sections)\n", Map->NumberOfMaps);

	Status = CcpAllocateSection(PAGE_READWRITE, &BaseAddress, &Area);

	if (!NT_SUCCESS(Status)) return INVALID_CACHE;

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

		ObReferenceObject(FileObject);
		Bcb->FileObject = FileObject;
		Bcb->FileOffset = *FileOffset;
		Bcb->BaseAddress = BaseAddress;
		Bcb->MemoryArea = Area;
		Bcb->Length = CACHE_STRIPE;
		Map->NumberOfMaps++;
		Area->Data.CacheData.CacheRegion = Bcb - CcCacheSections;

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
		MmLockAddressSpace(MmGetKernelAddressSpace());
		MmFreeMemoryArea(MmGetKernelAddressSpace(), Area, NULL, NULL);
		MmUnlockAddressSpace(MmGetKernelAddressSpace());
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

    DPRINT("Find Matching Map: %x:%x\n", FileOffset->LowPart, Length);

    if (FileOffset->QuadPart >= Head->FileOffset.QuadPart &&
		FileOffset->QuadPart + Length <= Head->FileOffset.QuadPart + Head->Length)
    {
		DPRINT("Head matched\n");
		return Head - CcCacheSections;
    }

    for (Entry = Head->ThisFileList.Flink; Entry != &Head->ThisFileList; Entry = Entry->Flink)
    {
		PNOCC_BCB Bcb = CONTAINING_RECORD(Entry, NOCC_BCB, ThisFileList);
		DPRINT("This File: %x:%x\n", Bcb->FileOffset.LowPart, Bcb->Length);
		if (FileOffset->QuadPart >= Bcb->FileOffset.QuadPart &&
			FileOffset->QuadPart + Length <= Bcb->FileOffset.QuadPart + Bcb->Length)
		{
			//DPRINT("Found match at #%x\n", Bcb - CcCacheSections);
			return Bcb - CcCacheSections;
		}
    }

    DPRINT("This region isn't mapped\n");

    return INVALID_CACHE;
}

BOOLEAN
NTAPI
CcpStartCaching(PFILE_OBJECT FileObject)
{
    FILE_STANDARD_INFORMATION FileInfo;
    ULONG Information;
    NTSTATUS Status;
    CC_FILE_SIZES FileSizes;

    if (!FileObject->SectionObjectPointer->SharedCacheMap)
    {
		DPRINT("CcpStartCaching %p\n", FileObject);
		Status = IoQueryFileInformation
			(FileObject,
			 FileStandardInformation,
			 sizeof(FILE_STANDARD_INFORMATION),
			 &FileInfo,
			 &Information);
	
		if (!NT_SUCCESS(Status))
		{
			return FALSE;
		}
	
		FileSizes.ValidDataLength = FileInfo.EndOfFile;
		FileSizes.FileSize = FileInfo.EndOfFile;
		FileSizes.AllocationSize = FileInfo.AllocationSize;

		DPRINT1("Initializing -> File Sizes: VALID %08x%08x FILESIZE %08x%08x ALLOC %08x%08x\n",
			   FileSizes.ValidDataLength.HighPart,
			   FileSizes.ValidDataLength.LowPart,
			   FileSizes.FileSize.HighPart,
			   FileSizes.FileSize.LowPart,
			   FileSizes.AllocationSize.HighPart,
			   FileSizes.AllocationSize.LowPart);

		CcInitializeCacheMap
			(FileObject,
			 &FileSizes,
			 FALSE,
			 NULL,
			 NULL);
    }

    return TRUE;
}

BOOLEAN
NTAPI
CcpMapData
(IN PNOCC_CACHE_MAP Map,
 IN PLARGE_INTEGER FileOffset,
 IN ULONG Length,
 IN ULONG Flags,
 IN BOOLEAN Zero,
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
			Bcb->Zero = Zero;
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

    /* Returns a reference */
    do 
    {
		CcpLock();
		
		BcbHead = CcpAllocateCacheSections(FileObject, &Target);
		
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

    DPRINT("Selected BCB #%x\n", BcbHead);
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
    if (!CcpStartCaching(FileObject)) return FALSE;
    return CcpMapData
		((PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap,
		 FileOffset,
		 Length,
		 Flags,
		 FALSE,
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
    BOOLEAN Wait = Flags == TRUE ? PIN_WAIT : Flags & PIN_WAIT;
    BOOLEAN Exclusive = Flags & PIN_EXCLUSIVE;
    BOOLEAN Result;
    ULONG BcbHead;
    PNOCC_BCB TheBcb;
    PVOID Buffer;

    if (!CcpStartCaching(FileObject)) return FALSE;
    Result = CcMapData(FileObject, FileOffset, Length, Wait ? MAP_WAIT : 0, Bcb, &Buffer);

    if (!Result) return FALSE;

    TheBcb = (PNOCC_BCB)*Bcb;
    BcbHead = TheBcb - CcCacheSections;

    CcpLock();
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
    CcpUnlock();

    if (Exclusive)
		CcpReferenceCacheExclusive(BcbHead);

    if (!TheBcb->Pinned)
    {
		PCHAR Buffer;
		ULONG Length;
		PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)
			FileObject->SectionObjectPointer->SharedCacheMap;

		TheBcb->Pinned = TRUE;

		Length = 
			min(Map->FileSizes.FileSize.QuadPart - TheBcb->FileOffset.QuadPart,
				CACHE_STRIPE);

		for (Buffer = TheBcb->BaseAddress;
			 Buffer < (PCHAR)TheBcb->BaseAddress + Length;
			 Buffer += PAGE_SIZE)
		{
			MmLockAddressSpace(MmGetKernelAddressSpace());
			CcReplaceCachePage(TheBcb->MemoryArea, Buffer);
			MmUnlockAddressSpace(MmGetKernelAddressSpace());
		}
	}

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
    PNOCC_BCB RealBcb;
    BOOLEAN Result;

    if (!CcpStartCaching(FileObject)) return FALSE;

    Result = CcPinMappedData
		(FileObject, 
		 FileOffset,
		 Length,
		 Flags,
		 Bcb);

    if (Result)
    {
		CcpLock();
		/* Find out if any range is a superset of what we want */
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

	if (!Zero)
	{
		Result = CcPinRead
			(FileObject,
			 FileOffset,
			 Length,
			 Flags,
			 Bcb,
			 Buffer);
	}
	else
	{
		PNOCC_CACHE_MAP Map;
		ASSERT(FileObject->SectionObjectPointer->SharedCacheMap);
		Map = (PNOCC_CACHE_MAP)
			FileObject->SectionObjectPointer->SharedCacheMap;
		Result = CcpMapData
			(Map,
			 FileOffset,
			 Length,
			 Flags,
			 TRUE,
			 Bcb,
			 Buffer);
		if (Result)
		{
			Result = CcPinMappedData
				(FileObject,
				 FileOffset,
				 Length,
				 Flags,
				 Bcb);
		}
	}

    if (Result)
    {
		CcpLock();
		RealBcb = *Bcb;
		RealBcb->Dirty = TRUE;
		CcpUnlock();
    }

    return Result;
}

VOID
NTAPI
CcUnpinData(IN PVOID Bcb)
{
    PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;
    ULONG Selected = RealBcb - CcCacheSections;

    DPRINT("CcUnpinData Bcb #%x (RefCount %d) (ExclusiveWaiter %d)\n", Selected, RealBcb->RefCount, RealBcb->ExclusiveWaiter);

    CcpLock();
    if (RealBcb->RefCount <= 2)
    {
		if (RealBcb->Pinned)
		{
			DPRINT("Unpin (actually) the memory\n");
			RealBcb->Pinned = FALSE;
		}
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

VOID
NTAPI
CcFlushCache(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
             IN OPTIONAL PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             OUT OPTIONAL PIO_STATUS_BLOCK IoStatus)
{
	NTSTATUS Status;
	ULONG Target;
	ULONG PageLength;
    PCHAR BufPage;
    PNOCC_BCB Bcb;
    LARGE_INTEGER ToWrite = *FileOffset;
    IO_STATUS_BLOCK IOSB;
    PNOCC_CACHE_MAP Map;

	DPRINT("CcFlushCache\n");
    
    if (IoStatus)
    {
		IoStatus->Status = STATUS_SUCCESS;
		IoStatus->Information = 0;
    }
	
    if (!SectionObjectPointer->SharedCacheMap)
    {
		DPRINT("Attempt to flush a non-cached file section\n");
		return;
    }
    
	Map = (PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap;

	CcpLock();
	Target = CcpFindCacheFor(Map);
	if (Target == INVALID_CACHE)
	{
		CcpUnlock();
		return;
	}
	Target = CcpFindMatchingMap(&CcCacheSections[Target], FileOffset, Length);
	if (Target == INVALID_CACHE)
	{
		CcpUnlock();
		return;
	}

	Bcb = &CcCacheSections[Target];

    /* Don't flush a pinned bcb, because we'll disturb the locked-ness
     * of the pages.  Figured out how to do this right. */
    if (Bcb->Pinned || !Bcb->Dirty) 
    {
		CcpUnlock();
		return;
    }
    
	CcpReferenceCache(Bcb - CcCacheSections);

    DPRINT
		("CcpSimpleWrite: [%wZ] %x:%d\n", 
		 &Bcb->FileObject->FileName,
		 Bcb->BaseAddress,
		 Bcb->Length);
    
	ToWrite = *FileOffset;

    for (BufPage = (PCHAR)Bcb->BaseAddress + 
			 PAGE_ROUND_DOWN((ULONG)(FileOffset->QuadPart - Bcb->FileOffset.QuadPart));
		 BufPage < (PCHAR)Bcb->BaseAddress + PAGE_ROUND_UP(Length);
		 BufPage += PAGE_SIZE)
    {
		PageLength = Length - ((PCHAR)BufPage - (PCHAR)Bcb->BaseAddress);
		if (PageLength > PAGE_SIZE)
			PageLength = PAGE_SIZE;
		if (MmIsPagePresent(NULL, BufPage) && MmIsDirtyPage(NULL, BufPage))
		{
			Status = MiScheduleForWrite
				(Bcb->FileObject,
				 &ToWrite,
				 MmGetPfnForProcess(NULL, BufPage),
				 PageLength);

			if (NT_SUCCESS(Status))
				MmSetCleanPage(NULL, BufPage);
		}
		ToWrite.QuadPart += PAGE_SIZE;
    }
    
    Bcb->Dirty = FALSE;
	CcpUnlock();

	CcUnpinData(Bcb);

    DPRINT("Page Write: %08x\n", IOSB.Status);
	
    if (IoStatus)
    {
		*IoStatus = IOSB;
    }

	DPRINT("CcFlushCache Done\n");
}

/* EOF */
