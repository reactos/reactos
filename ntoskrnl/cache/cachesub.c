/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/cachesup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Art Yerkes
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
CcSetReadAheadGranularity(IN PFILE_OBJECT FileObject,
                          IN ULONG Granularity)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcScheduleReadAhead(IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN ULONG Length)
{
    UNIMPLEMENTED;
    while (TRUE);  
}

VOID
NTAPI
CcSetDirtyPinnedData(IN PVOID BcbVoid,
                     IN OPTIONAL PLARGE_INTEGER Lsn)
{
	PCHAR Buffer;
	PNOCC_BCB Bcb = (PNOCC_BCB)BcbVoid;
	Bcb->Dirty = TRUE;
	for (Buffer = Bcb->BaseAddress; 
	     Buffer < ((PCHAR)Bcb->BaseAddress) + Bcb->Length;
	     Buffer += PAGE_SIZE)
		MmSetDirtyPage(NULL, Buffer);
	Bcb->Dirty = TRUE;
}

LARGE_INTEGER
NTAPI
CcGetFlushedValidData(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                      IN BOOLEAN CcInternalCaller)
{
    LARGE_INTEGER Result = {{0}};
    UNIMPLEMENTED;
    while (TRUE);
    return Result;
}


BOOLEAN
NTAPI
CcFlushImageSection
(PSECTION_OBJECT_POINTERS SectionObjectPointer,
 MMFLUSH_TYPE FlushType)
{
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap;
    PNOCC_BCB Bcb;
    PLIST_ENTRY Entry;
    IO_STATUS_BLOCK IOSB;
    BOOLEAN Result = TRUE;
    
    for (Entry = Map->AssociatedBcb.Flink;
	 Entry != &Map->AssociatedBcb;
	 Entry = Entry->Flink)
    {
	Bcb = CONTAINING_RECORD(Entry, NOCC_BCB, ThisFileList);

	if (!Bcb->Dirty) continue;

	switch (FlushType)
	{
	case MmFlushForDelete:
	    CcPurgeCacheSection
		(Bcb->FileObject->SectionObjectPointer,
		 &Bcb->FileOffset,
		 Bcb->Length,
		 FALSE);
	    break;
	case MmFlushForWrite:
	    CcFlushCache
		(Bcb->FileObject->SectionObjectPointer,
		 &Bcb->FileOffset,
		 Bcb->Length,
		 &IOSB);
	    break;
	}
    }

    return Result;
}

// Always succeeds for us
PVOID
NTAPI
CcRemapBcb(IN PVOID Bcb)
{
    return Bcb;
}


VOID
NTAPI
CcRepinBcb(IN PVOID Bcb)
{
    PVOID TheBcb;
    PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;
    CcPinMappedData
	(RealBcb->FileObject, 
	 &RealBcb->FileOffset,
	 RealBcb->Length,
	 PIN_WAIT,
	 &TheBcb);
}

VOID
NTAPI
CcUnpinRepinnedBcb(IN PVOID Bcb,
                   IN BOOLEAN WriteThrough,
                   OUT PIO_STATUS_BLOCK IoStatus)
{
    PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;

    if (WriteThrough)
    {
		CcFlushCache
			(RealBcb->FileObject->SectionObjectPointer,
			 &RealBcb->FileOffset,
			 RealBcb->Length,
			 IoStatus);
    }

    CcUnpinData(Bcb);
}

NTSTATUS
NTAPI
CcReplaceCachePage(PMEMORY_AREA MemoryArea, PVOID Address)
{
	NTSTATUS Status;
	PFN_TYPE Page;
	ULONG CacheRegion = MemoryArea->Data.CacheData.CacheRegion;
	PVOID HyperspaceMapping;
	LARGE_INTEGER ReadOffset;
	IO_STATUS_BLOCK Iosb;
    PNOCC_BCB Bcb = &CcCacheSections[CacheRegion];

	ASSERT(Address >= Bcb->BaseAddress);
	ASSERT(((PCHAR)Address - (PCHAR)Bcb->BaseAddress) < CACHE_STRIPE);
	
	ReadOffset.QuadPart = 
		PAGE_ROUND_DOWN((PCHAR)Address - (PCHAR)Bcb->BaseAddress) +
		Bcb->FileOffset.QuadPart;

//#if 0
	DPRINT("Replacing page at offset %08x%08x in file\n",
			ReadOffset.u.HighPart, ReadOffset.u.LowPart);
//#endif

	Status = MmRequestPageMemoryConsumer(MC_CACHE, TRUE, &Page);
	
	if (!NT_SUCCESS(Status))
		return Status;

	ASSERT(Page);

	MmCreateVirtualMapping
		(NULL,
		 (PVOID)PAGE_ROUND_DOWN((ULONG_PTR)Address), 
		 PAGE_READWRITE, 
		 &Page,
		 1);
	
	MmUnlockAddressSpace(MmGetKernelAddressSpace());
	
	HyperspaceMapping = MmCreateHyperspaceMapping(Page);
	if (!Bcb->Zero)
	{
		Status = MiSimpleRead
			(Bcb->FileObject, 
			 &ReadOffset, 
			 HyperspaceMapping, 
			 PAGE_SIZE,
			 &Iosb);
	}
	MmDeleteHyperspaceMapping(HyperspaceMapping);

	MmLockAddressSpace(MmGetKernelAddressSpace());

	if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
	{
		MmReleasePageMemoryConsumer(MC_CACHE, Page);
		return Status;
	}
	else
		Status = STATUS_SUCCESS;
	
	return Status;
}

/* EOF */
