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

PDEVICE_OBJECT
NTAPI
MmGetDeviceObjectForFile(IN PFILE_OBJECT FileObject);

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
	PNOCC_BCB Bcb = (PNOCC_BCB)BcbVoid;
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


VOID
NTAPI
CcpFlushCache(IN PNOCC_CACHE_MAP Map,
			  IN OPTIONAL PLARGE_INTEGER FileOffset,
			  IN ULONG Length,
			  OUT OPTIONAL PIO_STATUS_BLOCK IoStatus,
			  BOOLEAN Delete)
{
    PNOCC_BCB Bcb = NULL;
	LARGE_INTEGER LowerBound, UpperBound;
	PLIST_ENTRY ListEntry;
    IO_STATUS_BLOCK IOSB = { };

	DPRINT("CcFlushCache (while file)\n");

	if (FileOffset && Length)
	{
		LowerBound.QuadPart = FileOffset->QuadPart;
		UpperBound.QuadPart = LowerBound.QuadPart + Length;
	}
	else
	{
		LowerBound.QuadPart = 0;
		UpperBound.QuadPart = 0x7fffffffffffffffull;
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

			CcpUnlock();
			MiFlushMappedSection(Bcb->BaseAddress, &Map->FileSizes.FileSize);
			CcpLock();

			Bcb->Dirty = FALSE;
						
			ListEntry = ListEntry->Flink;
			if (Delete && Bcb->RefCount == 2)
			{
				Bcb->RefCount = 1;
				CcpDereferenceCache(Bcb - CcCacheSections, FALSE);
			}
			else
				CcpUnpinData(Bcb);
		}
		else
		{
			ListEntry = ListEntry->Flink;
			CcpUnpinData(Bcb);
		}

		DPRINT("End loop\n");
	}
	CcpUnlock();
	
    if (IoStatus) *IoStatus = IOSB;
}

VOID
NTAPI
CcFlushCache(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
             IN OPTIONAL PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             OUT OPTIONAL PIO_STATUS_BLOCK IoStatus)
{
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap;

	// Not cached
	if (!Map) 
	{
		if (IoStatus)
		{
			IoStatus->Status = STATUS_SUCCESS;
			IoStatus->Information = 0;
		}
		return;
	}

	CcpFlushCache(Map, FileOffset, Length, IoStatus, FALSE);
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

	if (!Map) return TRUE;
    
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
			(SectionObjectPointer,
			 &Bcb->FileOffset,
			 Bcb->Length,
			 FALSE);
	    break;
	case MmFlushForWrite:
	    CcFlushCache
			(SectionObjectPointer,
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
	CcpLock();
	ASSERT(RtlTestBit(CcCacheBitmap, ((PNOCC_BCB)Bcb) - CcCacheSections));
	CcpReferenceCache(((PNOCC_BCB)Bcb) - CcCacheSections);
	CcpUnlock();
    return Bcb;
}

VOID
NTAPI
CcShutdownSystem()
{
	ULONG i;

	DPRINT1("CC: Shutdown\n");

	for (i = 0; i < CACHE_NUM_SECTIONS; i++)
	{
		PNOCC_BCB Bcb = &CcCacheSections[i];
		if (Bcb->SectionObject)
		{
			DPRINT1
				("Evicting #%02x %08x%08x %wZ\n", 
				 i, 
				 Bcb->FileOffset.u.HighPart, Bcb->FileOffset.u.LowPart,
				 &MmGetFileObjectForSection
				 ((PROS_SECTION_OBJECT)Bcb->SectionObject)->FileName);
			CcpFlushCache(Bcb->Map, NULL, 0, NULL, FALSE);
			Bcb->Dirty = FALSE;
		}
	}

	DPRINT1("Done\n");
}


VOID
NTAPI
CcRepinBcb(IN PVOID Bcb)
{
	CcpLock();
	ASSERT(RtlTestBit(CcCacheBitmap, ((PNOCC_BCB)Bcb) - CcCacheSections));
	DPRINT("CcRepinBcb(#%x)\n", ((PNOCC_BCB)Bcb) - CcCacheSections);
	CcpReferenceCache(((PNOCC_BCB)Bcb) - CcCacheSections);
	CcpUnlock();
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
		DPRINT("BCB #%x\n", RealBcb - CcCacheSections);

		CcpFlushCache
			(RealBcb->Map,
			 &RealBcb->FileOffset,
			 RealBcb->Length,
			 IoStatus, FALSE);
    }

    CcUnpinData(Bcb);
}

/* EOF */
