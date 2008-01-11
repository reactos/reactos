/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/view.c
 * PURPOSE:         Cache manager
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* NOTES **********************************************************************
 *
 * This is not the NT implementation of a file cache nor anything much like
 * it.
 *
 * The general procedure for a filesystem to implement a read or write
 * dispatch routine is as follows
 *
 * (1) If caching for the FCB hasn't been initiated then so do by calling
 * CcInitializeFileCache.
 *
 * (2) For each 4k region which is being read or written obtain a cache page
 * by calling CcRequestCachePage.
 *
 * (3) If either the page is being read or not completely written, and it is
 * not up to date then read its data from the underlying medium. If the read
 * fails then call CcReleaseCachePage with VALID as FALSE and return a error.
 *
 * (4) Copy the data into or out of the page as necessary.
 *
 * (5) Release the cache page
 */
/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, CcInitView)
#endif

/* GLOBALS *******************************************************************/

/*
 * If CACHE_BITMAP is defined, the cache manager uses one large memory region
 * within the kernel address space and allocate/deallocate space from this block
 * over a bitmap. If CACHE_BITMAP is used, the size of the mdl mapping region
 * must be reduced (ntoskrnl\mm\mdl.c, MI_MDLMAPPING_REGION_SIZE).
 */
//#define CACHE_BITMAP

static LIST_ENTRY DirtySegmentListHead;
static LIST_ENTRY CacheSegmentListHead;
static LIST_ENTRY CacheSegmentLRUListHead;
static LIST_ENTRY ClosedListHead;
ULONG DirtyPageCount=0;

KGUARDED_MUTEX ViewLock;

#ifdef CACHE_BITMAP
#define	CI_CACHESEG_MAPPING_REGION_SIZE	(128*1024*1024)

static PVOID CiCacheSegMappingRegionBase = NULL;
static RTL_BITMAP CiCacheSegMappingRegionAllocMap;
static ULONG CiCacheSegMappingRegionHint;
static KSPIN_LOCK CiCacheSegMappingRegionLock;
#endif

NPAGED_LOOKASIDE_LIST iBcbLookasideList;
static NPAGED_LOOKASIDE_LIST BcbLookasideList;
static NPAGED_LOOKASIDE_LIST CacheSegLookasideList;

static ULONG CcTimeStamp;
static KEVENT LazyCloseThreadEvent;
static HANDLE LazyCloseThreadHandle;
static CLIENT_ID LazyCloseThreadId;
static volatile BOOLEAN LazyCloseThreadShouldTerminate;

#if defined(__GNUC__)
/* void * alloca(size_t size); */
#elif defined(_MSC_VER)
void* _alloca(size_t size);
#else
#error Unknown compiler for alloca intrinsic stack allocation "function"
#endif

#if defined(DBG) || defined(KDBG)
static void CcRosCacheSegmentIncRefCount_ ( PCACHE_SEGMENT cs, const char* file, int line )
{
	++cs->ReferenceCount;
	if ( cs->Bcb->Trace )
	{
		DbgPrint("(%s:%i) CacheSegment %p ++RefCount=%d, Dirty %d, PageOut %d\n",
			file, line, cs, cs->ReferenceCount, cs->Dirty, cs->PageOut );
	}
}
static void CcRosCacheSegmentDecRefCount_ ( PCACHE_SEGMENT cs, const char* file, int line )
{
	--cs->ReferenceCount;
	if ( cs->Bcb->Trace )
	{
		DbgPrint("(%s:%i) CacheSegment %p --RefCount=%d, Dirty %d, PageOut %d\n",
			file, line, cs, cs->ReferenceCount, cs->Dirty, cs->PageOut );
	}
}
#define CcRosCacheSegmentIncRefCount(cs) CcRosCacheSegmentIncRefCount_(cs,__FILE__,__LINE__)
#define CcRosCacheSegmentDecRefCount(cs) CcRosCacheSegmentDecRefCount_(cs,__FILE__,__LINE__)
#else
#define CcRosCacheSegmentIncRefCount(cs) (++((cs)->ReferenceCount))
#define CcRosCacheSegmentDecRefCount(cs) (--((cs)->ReferenceCount))
#endif

NTSTATUS
CcRosInternalFreeCacheSegment(PCACHE_SEGMENT CacheSeg);


/* FUNCTIONS *****************************************************************/

VOID
STDCALL
CcRosTraceCacheMap (
	PBCB Bcb,
	BOOLEAN Trace )
{
#if defined(DBG) || defined(KDBG)
	KIRQL oldirql;
	PLIST_ENTRY current_entry;
	PCACHE_SEGMENT current;

	if ( !Bcb )
		return;

	Bcb->Trace = Trace;

	if ( Trace )
	{
		DPRINT1("Enabling Tracing for CacheMap 0x%p:\n", Bcb );

		KeAcquireGuardedMutex(&ViewLock);
		KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);

		current_entry = Bcb->BcbSegmentListHead.Flink;
		while (current_entry != &Bcb->BcbSegmentListHead)
		{
			current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
			current_entry = current_entry->Flink;

			DPRINT1("  CacheSegment 0x%p enabled, RefCount %d, Dirty %d, PageOut %d\n",
				current, current->ReferenceCount, current->Dirty, current->PageOut );
		}
		KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
		KeReleaseGuardedMutex(&ViewLock);
	}
	else
	{
		DPRINT1("Disabling Tracing for CacheMap 0x%p:\n", Bcb );
	}

#else
	Bcb = Bcb;
	Trace = Trace;
#endif
}

NTSTATUS
NTAPI
CcRosFlushCacheSegment(PCACHE_SEGMENT CacheSegment)
{
    NTSTATUS Status;
    KIRQL oldIrql;
    
    Status = WriteCacheSegment(CacheSegment);
    if (NT_SUCCESS(Status))
    {
        KeAcquireGuardedMutex(&ViewLock);
        KeAcquireSpinLock(&CacheSegment->Bcb->BcbLock, &oldIrql);
        
        CacheSegment->Dirty = FALSE;
        RemoveEntryList(&CacheSegment->DirtySegmentListEntry);
        DirtyPageCount -= CacheSegment->Bcb->CacheSegmentSize / PAGE_SIZE;
        CcRosCacheSegmentDecRefCount ( CacheSegment );
        
        KeReleaseSpinLock(&CacheSegment->Bcb->BcbLock, oldIrql);
        KeReleaseGuardedMutex(&ViewLock);
    }
    
    return(Status);
}

NTSTATUS
NTAPI
CcRosFlushDirtyPages(ULONG Target, PULONG Count)
{
    PLIST_ENTRY current_entry;
    PCACHE_SEGMENT current;
    ULONG PagesPerSegment;
    BOOLEAN Locked;
    NTSTATUS Status;
    static ULONG WriteCount[4] = {0, 0, 0, 0};
    ULONG NewTarget;
    
    DPRINT("CcRosFlushDirtyPages(Target %d)\n", Target);
    
    (*Count) = 0;
    
    KeAcquireGuardedMutex(&ViewLock);
    
    WriteCount[0] = WriteCount[1];
    WriteCount[1] = WriteCount[2];
    WriteCount[2] = WriteCount[3];
    WriteCount[3] = 0;
    
    NewTarget = WriteCount[0] + WriteCount[1] + WriteCount[2];
    
    if (NewTarget < DirtyPageCount)
    {
        NewTarget = (DirtyPageCount - NewTarget + 3) / 4;
        WriteCount[0] += NewTarget;
        WriteCount[1] += NewTarget;
        WriteCount[2] += NewTarget;
        WriteCount[3] += NewTarget;
    }
    
    NewTarget = WriteCount[0];
    
    Target = max(NewTarget, Target);
    
    current_entry = DirtySegmentListHead.Flink;
    if (current_entry == &DirtySegmentListHead)
    {
        DPRINT("No Dirty pages\n");
    }
    
    while (current_entry != &DirtySegmentListHead && Target > 0)
    {
        current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT,
                                    DirtySegmentListEntry);
        current_entry = current_entry->Flink;

        /* This Ros-specific function needs FileObject to be referenced,
           ohterwise it may be deleted while this function still works
           with it */
        ObReferenceObject(current->Bcb->FileObject);

        Locked = ExTryToAcquireResourceExclusiveLite(((FSRTL_COMMON_FCB_HEADER*)(current->Bcb->FileObject->FsContext))->Resource);
        if (!Locked)
        {
            ObDereferenceObject(current->Bcb->FileObject);
            continue;
        }
        
        Locked = ExTryToAcquirePushLockExclusive(&current->Lock);
        if (!Locked)
        {
            ExReleaseResourceLite(((FSRTL_COMMON_FCB_HEADER*)(current->Bcb->FileObject->FsContext))->Resource);
            ObDereferenceObject(current->Bcb->FileObject);
            continue;
        }
        
        ASSERT(current->Dirty);
        if (current->ReferenceCount > 1)
        {
            ExReleasePushLock(&current->Lock);
            ExReleaseResourceLite(((FSRTL_COMMON_FCB_HEADER*)(current->Bcb->FileObject->FsContext))->Resource);
            ObDereferenceObject(current->Bcb->FileObject);
            continue;
        }
        
        PagesPerSegment = current->Bcb->CacheSegmentSize / PAGE_SIZE;

        KeReleaseGuardedMutex(&ViewLock);
        
        Status = CcRosFlushCacheSegment(current);

        ExReleasePushLock(&current->Lock);
        ExReleaseResourceLite(((FSRTL_COMMON_FCB_HEADER*)(current->Bcb->FileObject->FsContext))->Resource);
        ObDereferenceObject(current->Bcb->FileObject);

        if (!NT_SUCCESS(Status) &&  (Status != STATUS_END_OF_FILE))
        {
            DPRINT1("CC: Failed to flush cache segment.\n");
        }
        else
        {
            (*Count) += PagesPerSegment;
            Target -= PagesPerSegment;
        }
        
        KeAcquireGuardedMutex(&ViewLock);
        current_entry = DirtySegmentListHead.Flink;
    }
    
    if (*Count < NewTarget)
    {
        WriteCount[1] += (NewTarget - *Count);
    }
    
    KeReleaseGuardedMutex(&ViewLock);
    
    DPRINT("CcRosFlushDirtyPages() finished\n");
    return(STATUS_SUCCESS);
}

NTSTATUS
CcRosTrimCache(ULONG Target, ULONG Priority, PULONG NrFreed)
/*
 * FUNCTION: Try to free some memory from the file cache.
 * ARGUMENTS:
 *       Target - The number of pages to be freed.
 *       Priority - The priority of free (currently unused).
 *       NrFreed - Points to a variable where the number of pages
 *                 actually freed is returned.
 */
{
  PLIST_ENTRY current_entry;
  PCACHE_SEGMENT current, last = NULL;
  ULONG PagesPerSegment;
  ULONG PagesFreed;
  KIRQL oldIrql;
  LIST_ENTRY FreeList;

  DPRINT("CcRosTrimCache(Target %d)\n", Target);

  *NrFreed = 0;

  InitializeListHead(&FreeList);

  KeAcquireGuardedMutex(&ViewLock);
  current_entry = CacheSegmentLRUListHead.Flink;
  while (current_entry != &CacheSegmentLRUListHead && Target > 0)
    {
      current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT,
				  CacheSegmentLRUListEntry);
      current_entry = current_entry->Flink;

      KeAcquireSpinLock(&current->Bcb->BcbLock, &oldIrql);
      if (current->ReferenceCount == 0)
      {
         RemoveEntryList(&current->BcbSegmentListEntry);
         KeReleaseSpinLock(&current->Bcb->BcbLock, oldIrql);
         RemoveEntryList(&current->CacheSegmentListEntry);
         RemoveEntryList(&current->CacheSegmentLRUListEntry);
	 InsertHeadList(&FreeList, &current->BcbSegmentListEntry);
         PagesPerSegment = current->Bcb->CacheSegmentSize / PAGE_SIZE;
         PagesFreed = min(PagesPerSegment, Target);
         Target -= PagesFreed;
         (*NrFreed) += PagesFreed;
      }
      else
      {
         if (last != current && current->MappedCount > 0 && !current->Dirty && !current->PageOut)
	   {
	     ULONG i;
	     NTSTATUS Status;

         CcRosCacheSegmentIncRefCount(current);
	     last = current;
	     current->PageOut = TRUE;
             KeReleaseSpinLock(&current->Bcb->BcbLock, oldIrql);
	     KeReleaseGuardedMutex(&ViewLock);
	     for (i = 0; i < current->Bcb->CacheSegmentSize / PAGE_SIZE; i++)
	       {
	         PFN_TYPE Page;
		 Page = (PFN_TYPE)(MmGetPhysicalAddress((char*)current->BaseAddress + i * PAGE_SIZE).QuadPart >> PAGE_SHIFT);
                 Status = MmPageOutPhysicalAddress(Page);
		 if (!NT_SUCCESS(Status))
		   {
		     break;
		   }
	       }
             KeAcquireGuardedMutex(&ViewLock);
             KeAcquireSpinLock(&current->Bcb->BcbLock, &oldIrql);
             CcRosCacheSegmentDecRefCount(current);
             current->PageOut = FALSE;
             KeReleaseSpinLock(&current->Bcb->BcbLock, oldIrql);
             current_entry = &current->CacheSegmentLRUListEntry;
	     continue;
	   }
	 KeReleaseSpinLock(&current->Bcb->BcbLock, oldIrql);
      }
  }
  KeReleaseGuardedMutex(&ViewLock);

  while (!IsListEmpty(&FreeList))
  {
     current_entry = RemoveHeadList(&FreeList);
     current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT,
				 BcbSegmentListEntry);
     CcRosInternalFreeCacheSegment(current);
  }

  DPRINT("CcRosTrimCache() finished\n");
  return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
CcRosReleaseCacheSegment(PBCB Bcb,
			 PCACHE_SEGMENT CacheSeg,
			 BOOLEAN Valid,
			 BOOLEAN Dirty,
			 BOOLEAN Mapped)
{
  BOOLEAN WasDirty = CacheSeg->Dirty;
  KIRQL oldIrql;

  ASSERT(Bcb);

  DPRINT("CcReleaseCacheSegment(Bcb 0x%p, CacheSeg 0x%p, Valid %d)\n",
	 Bcb, CacheSeg, Valid);

  CacheSeg->Valid = Valid;
  CacheSeg->Dirty = CacheSeg->Dirty || Dirty;

  KeAcquireGuardedMutex(&ViewLock);
  if (!WasDirty && CacheSeg->Dirty)
    {
      InsertTailList(&DirtySegmentListHead, &CacheSeg->DirtySegmentListEntry);
      DirtyPageCount += Bcb->CacheSegmentSize / PAGE_SIZE;
    }
  RemoveEntryList(&CacheSeg->CacheSegmentLRUListEntry);
  InsertTailList(&CacheSegmentLRUListHead, &CacheSeg->CacheSegmentLRUListEntry);

  if (Mapped)
  {
     CacheSeg->MappedCount++;
  }
  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  CcRosCacheSegmentDecRefCount(CacheSeg);
  if (Mapped && CacheSeg->MappedCount == 1)
  {
      CcRosCacheSegmentIncRefCount(CacheSeg);
  }
  if (!WasDirty && CacheSeg->Dirty)
  {
      CcRosCacheSegmentIncRefCount(CacheSeg);
  }
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  KeReleaseGuardedMutex(&ViewLock);
  ExReleasePushLock(&CacheSeg->Lock);

  return(STATUS_SUCCESS);
}

/* Returns with Cache Segment Lock Held! */
PCACHE_SEGMENT
NTAPI
CcRosLookupCacheSegment(PBCB Bcb, ULONG FileOffset)
{
    PLIST_ENTRY current_entry;
    PCACHE_SEGMENT current;
    KIRQL oldIrql;
    
    ASSERT(Bcb);
    
    DPRINT("CcRosLookupCacheSegment(Bcb -x%p, FileOffset %d)\n", Bcb, FileOffset);
    
    KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
    current_entry = Bcb->BcbSegmentListHead.Flink;
    while (current_entry != &Bcb->BcbSegmentListHead)
    {
        current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT,
                                    BcbSegmentListEntry);
        if (current->FileOffset <= FileOffset &&
            (current->FileOffset + Bcb->CacheSegmentSize) > FileOffset)
        {
            CcRosCacheSegmentIncRefCount(current);
            KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
            ExAcquirePushLockExclusive(&current->Lock);
            return(current);
        }
        current_entry = current_entry->Flink;
    }
    KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
    return(NULL);
}

NTSTATUS
NTAPI
CcRosMarkDirtyCacheSegment(PBCB Bcb, ULONG FileOffset)
{
  PCACHE_SEGMENT CacheSeg;
  KIRQL oldIrql;

  ASSERT(Bcb);

  DPRINT("CcRosMarkDirtyCacheSegment(Bcb 0x%p, FileOffset %d)\n", Bcb, FileOffset);

  CacheSeg = CcRosLookupCacheSegment(Bcb, FileOffset);
  if (CacheSeg == NULL)
    {
      KEBUGCHECKCC;
    }
  if (!CacheSeg->Dirty)
    {
      KeAcquireGuardedMutex(&ViewLock);
      InsertTailList(&DirtySegmentListHead, &CacheSeg->DirtySegmentListEntry);
      DirtyPageCount += Bcb->CacheSegmentSize / PAGE_SIZE;
      KeReleaseGuardedMutex(&ViewLock);
    }
  else
  {
     KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
     CcRosCacheSegmentDecRefCount(CacheSeg);
     KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  }


  CacheSeg->Dirty = TRUE;
  ExReleasePushLock(&CacheSeg->Lock);

  return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
CcRosUnmapCacheSegment(PBCB Bcb, ULONG FileOffset, BOOLEAN NowDirty)
{
  PCACHE_SEGMENT CacheSeg;
  BOOLEAN WasDirty;
  KIRQL oldIrql;

  ASSERT(Bcb);

  DPRINT("CcRosUnmapCacheSegment(Bcb 0x%p, FileOffset %d, NowDirty %d)\n",
          Bcb, FileOffset, NowDirty);

  CacheSeg = CcRosLookupCacheSegment(Bcb, FileOffset);
  if (CacheSeg == NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  WasDirty = CacheSeg->Dirty;
  CacheSeg->Dirty = CacheSeg->Dirty || NowDirty;

  CacheSeg->MappedCount--;

  if (!WasDirty && NowDirty)
  {
     KeAcquireGuardedMutex(&ViewLock);
     InsertTailList(&DirtySegmentListHead, &CacheSeg->DirtySegmentListEntry);
     DirtyPageCount += Bcb->CacheSegmentSize / PAGE_SIZE;
     KeReleaseGuardedMutex(&ViewLock);
  }

  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  CcRosCacheSegmentDecRefCount(CacheSeg);
  if (!WasDirty && NowDirty)
  {
     CcRosCacheSegmentIncRefCount(CacheSeg);
  }
  if (CacheSeg->MappedCount == 0)
  {
     CcRosCacheSegmentDecRefCount(CacheSeg);
  }
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);

  ExReleasePushLock(&CacheSeg->Lock);
  return(STATUS_SUCCESS);
}

NTSTATUS static
CcRosCreateCacheSegment(PBCB Bcb,
			ULONG FileOffset,
			PCACHE_SEGMENT* CacheSeg)
{
  ULONG i;
  PCACHE_SEGMENT current;
  PCACHE_SEGMENT previous;
  PLIST_ENTRY current_entry;
  NTSTATUS Status;
  KIRQL oldIrql;
  PPFN_TYPE Pfn;
#ifdef CACHE_BITMAP
  ULONG StartingOffset;
#else
#endif
  PHYSICAL_ADDRESS BoundaryAddressMultiple;

  ASSERT(Bcb);

  DPRINT("CcRosCreateCacheSegment()\n");

  BoundaryAddressMultiple.QuadPart = 0;
  if (FileOffset >= Bcb->FileSize.u.LowPart)
  {
     CacheSeg = NULL;
     return STATUS_INVALID_PARAMETER;
  }

  current = ExAllocateFromNPagedLookasideList(&CacheSegLookasideList);
  current->Valid = FALSE;
  current->Dirty = FALSE;
  current->PageOut = FALSE;
  current->FileOffset = ROUND_DOWN(FileOffset, Bcb->CacheSegmentSize);
  current->Bcb = Bcb;
#if defined(DBG) || defined(KDBG)
  if ( Bcb->Trace )
  {
    DPRINT1("CacheMap 0x%p: new Cache Segment: 0x%p\n", Bcb, current );
  }
#endif
  current->MappedCount = 0;
  current->DirtySegmentListEntry.Flink = NULL;
  current->DirtySegmentListEntry.Blink = NULL;
  current->ReferenceCount = 1;
  ExInitializePushLock((PULONG_PTR)&current->Lock);
  ExAcquirePushLockExclusive(&current->Lock);
  KeAcquireGuardedMutex(&ViewLock);

  *CacheSeg = current;
  /* There is window between the call to CcRosLookupCacheSegment
   * and CcRosCreateCacheSegment. We must check if a segment on
   * the fileoffset exist. If there exist a segment, we release
   * our new created segment and return the existing one.
   */
  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  current_entry = Bcb->BcbSegmentListHead.Flink;
  previous = NULL;
  while (current_entry != &Bcb->BcbSegmentListHead)
  {
     current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT,
				 BcbSegmentListEntry);
     if (current->FileOffset <= FileOffset &&
      	(current->FileOffset + Bcb->CacheSegmentSize) > FileOffset)
     {
	CcRosCacheSegmentIncRefCount(current);
	KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
#if defined(DBG) || defined(KDBG)
	if ( Bcb->Trace )
	{
		DPRINT1("CacheMap 0x%p: deleting newly created Cache Segment 0x%p ( found existing one 0x%p )\n",
			Bcb,
			(*CacheSeg),
			current );
	}
#endif
	ExReleasePushLock(&(*CacheSeg)->Lock);
	KeReleaseGuardedMutex(&ViewLock);
	ExFreeToNPagedLookasideList(&CacheSegLookasideList, *CacheSeg);
	*CacheSeg = current;
        ExAcquirePushLockExclusive(&current->Lock);
	return STATUS_SUCCESS;
     }
     if (current->FileOffset < FileOffset)
     {
        if (previous == NULL)
	{
	   previous = current;
	}
	else
	{
	   if (previous->FileOffset < current->FileOffset)
	   {
	      previous = current;
	   }
	}
     }
     current_entry = current_entry->Flink;
  }
  /* There was no existing segment. */
  current = *CacheSeg;
  if (previous)
  {
     InsertHeadList(&previous->BcbSegmentListEntry, &current->BcbSegmentListEntry);
  }
  else
  {
     InsertHeadList(&Bcb->BcbSegmentListHead, &current->BcbSegmentListEntry);
  }
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  InsertTailList(&CacheSegmentListHead, &current->CacheSegmentListEntry);
  InsertTailList(&CacheSegmentLRUListHead, &current->CacheSegmentLRUListEntry);
  KeReleaseGuardedMutex(&ViewLock);
#ifdef CACHE_BITMAP
  KeAcquireSpinLock(&CiCacheSegMappingRegionLock, &oldIrql);

  StartingOffset = RtlFindClearBitsAndSet(&CiCacheSegMappingRegionAllocMap, Bcb->CacheSegmentSize / PAGE_SIZE, CiCacheSegMappingRegionHint);

  if (StartingOffset == 0xffffffff)
  {
     DPRINT1("Out of CacheSeg mapping space\n");
     KEBUGCHECKCC;
  }

  current->BaseAddress = CiCacheSegMappingRegionBase + StartingOffset * PAGE_SIZE;

  if (CiCacheSegMappingRegionHint == StartingOffset)
  {
     CiCacheSegMappingRegionHint += Bcb->CacheSegmentSize / PAGE_SIZE;
  }

  KeReleaseSpinLock(&CiCacheSegMappingRegionLock, oldIrql);
#else
  MmLockAddressSpace(MmGetKernelAddressSpace());
  current->BaseAddress = NULL;
  Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
			      MEMORY_AREA_CACHE_SEGMENT,
			      &current->BaseAddress,
			      Bcb->CacheSegmentSize,
			      PAGE_READWRITE,
			      (PMEMORY_AREA*)&current->MemoryArea,
			      FALSE,
			      0,
			      BoundaryAddressMultiple);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
  if (!NT_SUCCESS(Status))
  {
     KEBUGCHECKCC;
  }
#endif
  Pfn = alloca(sizeof(PFN_TYPE) * (Bcb->CacheSegmentSize / PAGE_SIZE));
  for (i = 0; i < (Bcb->CacheSegmentSize / PAGE_SIZE); i++)
  {
     Status = MmRequestPageMemoryConsumer(MC_CACHE, TRUE, &Pfn[i]);
     if (!NT_SUCCESS(Status))
     {
	KEBUGCHECKCC;
     }
  }
  Status = MmCreateVirtualMapping(NULL,
				  current->BaseAddress,
				  PAGE_READWRITE,
				  Pfn,
				  Bcb->CacheSegmentSize / PAGE_SIZE);
  if (!NT_SUCCESS(Status))
  {
    KEBUGCHECKCC;
  }
  return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
CcRosGetCacheSegmentChain(PBCB Bcb,
			  ULONG FileOffset,
			  ULONG Length,
			  PCACHE_SEGMENT* CacheSeg)
{
  PCACHE_SEGMENT current;
  ULONG i;
  PCACHE_SEGMENT* CacheSegList;
  PCACHE_SEGMENT Previous = NULL;

  ASSERT(Bcb);

  DPRINT("CcRosGetCacheSegmentChain()\n");

  Length = ROUND_UP(Length, Bcb->CacheSegmentSize);

#if defined(__GNUC__)
  CacheSegList = alloca(sizeof(PCACHE_SEGMENT) *
			(Length / Bcb->CacheSegmentSize));
#elif defined(_MSC_VER)
  CacheSegList = _alloca(sizeof(PCACHE_SEGMENT) *
			(Length / Bcb->CacheSegmentSize));
#else
#error Unknown compiler for alloca intrinsic stack allocation "function"
#endif

  /*
   * Look for a cache segment already mapping the same data.
   */
  for (i = 0; i < (Length / Bcb->CacheSegmentSize); i++)
    {
      ULONG CurrentOffset = FileOffset + (i * Bcb->CacheSegmentSize);
      current = CcRosLookupCacheSegment(Bcb, CurrentOffset);
      if (current != NULL)
	{
	  CacheSegList[i] = current;
	}
      else
	{
	  CcRosCreateCacheSegment(Bcb, CurrentOffset, &current);
	  CacheSegList[i] = current;
	}
    }

  for (i = 0; i < (Length / Bcb->CacheSegmentSize); i++)
    {
      if (i == 0)
	{
	  *CacheSeg = CacheSegList[i];
	  Previous = CacheSegList[i];
	}
      else
	{
	  Previous->NextInChain = CacheSegList[i];
	  Previous = CacheSegList[i];
	}
    }
  ASSERT(Previous);
  Previous->NextInChain = NULL;

  return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
CcRosGetCacheSegment(PBCB Bcb,
		     ULONG FileOffset,
		     PULONG BaseOffset,
		     PVOID* BaseAddress,
		     PBOOLEAN UptoDate,
		     PCACHE_SEGMENT* CacheSeg)
{
   PCACHE_SEGMENT current;
   NTSTATUS Status;

   ASSERT(Bcb);

   DPRINT("CcRosGetCacheSegment()\n");

   /*
    * Look for a cache segment already mapping the same data.
    */
   current = CcRosLookupCacheSegment(Bcb, FileOffset);
   if (current == NULL)
   {
     /*
      * Otherwise create a new segment.
      */
      Status = CcRosCreateCacheSegment(Bcb, FileOffset, &current);
      if (!NT_SUCCESS(Status))
      {
	return Status;
      }
   }
   /*
    * Return information about the segment to the caller.
    */
   *UptoDate = current->Valid;
   *BaseAddress = current->BaseAddress;
   DPRINT("*BaseAddress 0x%.8X\n", *BaseAddress);
   *CacheSeg = current;
   *BaseOffset = current->FileOffset;
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
CcRosRequestCacheSegment(PBCB Bcb,
		      ULONG FileOffset,
		      PVOID* BaseAddress,
		      PBOOLEAN UptoDate,
		      PCACHE_SEGMENT* CacheSeg)
/*
 * FUNCTION: Request a page mapping for a BCB
 */
{
  ULONG BaseOffset;

  ASSERT(Bcb);

  if ((FileOffset % Bcb->CacheSegmentSize) != 0)
    {
      CPRINT("Bad fileoffset %x should be multiple of %x",
        FileOffset, Bcb->CacheSegmentSize);
      KEBUGCHECKCC;
    }

  return(CcRosGetCacheSegment(Bcb,
			   FileOffset,
			   &BaseOffset,
			   BaseAddress,
			   UptoDate,
			   CacheSeg));
}
#ifdef CACHE_BITMAP
#else
static VOID
CcFreeCachePage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
		PFN_TYPE Page, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
  ASSERT(SwapEntry == 0);
  if (Page != 0)
    {
      MmReleasePageMemoryConsumer(MC_CACHE, Page);
    }
}
#endif
NTSTATUS
CcRosInternalFreeCacheSegment(PCACHE_SEGMENT CacheSeg)
/*
 * FUNCTION: Releases a cache segment associated with a BCB
 */
{
#ifdef CACHE_BITMAP
  ULONG i;
  ULONG RegionSize;
  ULONG Base;
  PFN_TYPE Page;
  KIRQL oldIrql;
#endif
  DPRINT("Freeing cache segment 0x%p\n", CacheSeg);
#if defined(DBG) || defined(KDBG)
	if ( CacheSeg->Bcb->Trace )
	{
		DPRINT1("CacheMap 0x%p: deleting Cache Segment: 0x%p\n", CacheSeg->Bcb, CacheSeg );
	}
#endif
#ifdef CACHE_BITMAP
  RegionSize = CacheSeg->Bcb->CacheSegmentSize / PAGE_SIZE;

  /* Unmap all the pages. */
  for (i = 0; i < RegionSize; i++)
    {
      MmDeleteVirtualMapping(NULL,
			     CacheSeg->BaseAddress + (i * PAGE_SIZE),
			     FALSE,
			     NULL,
			     &Page);
      MmReleasePageMemoryConsumer(MC_CACHE, Page);
    }

  KeAcquireSpinLock(&CiCacheSegMappingRegionLock, &oldIrql);
  /* Deallocate all the pages used. */
  Base = (ULONG)(CacheSeg->BaseAddress - CiCacheSegMappingRegionBase) / PAGE_SIZE;

  RtlClearBits(&CiCacheSegMappingRegionAllocMap, Base, RegionSize);

  CiCacheSegMappingRegionHint = min (CiCacheSegMappingRegionHint, Base);

  KeReleaseSpinLock(&CiCacheSegMappingRegionLock, oldIrql);
#else
  MmLockAddressSpace(MmGetKernelAddressSpace());
  MmFreeMemoryArea(MmGetKernelAddressSpace(),
		   CacheSeg->MemoryArea,
		   CcFreeCachePage,
		   NULL);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
#endif
  ExFreeToNPagedLookasideList(&CacheSegLookasideList, CacheSeg);
  return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
CcRosFreeCacheSegment(PBCB Bcb, PCACHE_SEGMENT CacheSeg)
{
  NTSTATUS Status;
  KIRQL oldIrql;

  ASSERT(Bcb);

  DPRINT("CcRosFreeCacheSegment(Bcb 0x%p, CacheSeg 0x%p)\n",
         Bcb, CacheSeg);

  KeAcquireGuardedMutex(&ViewLock);
  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  RemoveEntryList(&CacheSeg->BcbSegmentListEntry);
  RemoveEntryList(&CacheSeg->CacheSegmentListEntry);
  RemoveEntryList(&CacheSeg->CacheSegmentLRUListEntry);
  if (CacheSeg->Dirty)
  {
     RemoveEntryList(&CacheSeg->DirtySegmentListEntry);
     DirtyPageCount -= Bcb->CacheSegmentSize / PAGE_SIZE;

  }
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  KeReleaseGuardedMutex(&ViewLock);

  Status = CcRosInternalFreeCacheSegment(CacheSeg);
  return(Status);
}

/*
 * @implemented
 */
VOID STDCALL
CcFlushCache(IN PSECTION_OBJECT_POINTERS SectionObjectPointers,
	     IN PLARGE_INTEGER FileOffset OPTIONAL,
	     IN ULONG Length,
	     OUT PIO_STATUS_BLOCK IoStatus)
{
   PBCB Bcb;
   LARGE_INTEGER Offset;
   PCACHE_SEGMENT current;
   NTSTATUS Status;
   KIRQL oldIrql;

   DPRINT("CcFlushCache(SectionObjectPointers 0x%p, FileOffset 0x%p, Length %d, IoStatus 0x%p)\n",
           SectionObjectPointers, FileOffset, Length, IoStatus);

   if (SectionObjectPointers && SectionObjectPointers->SharedCacheMap)
   {
      Bcb = (PBCB)SectionObjectPointers->SharedCacheMap;
      ASSERT(Bcb);
      if (FileOffset)
      {
	 Offset = *FileOffset;
      }
      else
      {
	 Offset.QuadPart = (LONGLONG)0;
	 Length = Bcb->FileSize.u.LowPart;
      }

      if (IoStatus)
      {
	 IoStatus->Status = STATUS_SUCCESS;
	 IoStatus->Information = 0;
      }

      while (Length > 0)
      {
	 current = CcRosLookupCacheSegment (Bcb, Offset.u.LowPart);
	 if (current != NULL)
	 {
	    if (current->Dirty)
	    {
	       Status = CcRosFlushCacheSegment(current);
	       if (!NT_SUCCESS(Status) && IoStatus != NULL)
	       {
		   IoStatus->Status = Status;
	       }
	    }
            KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
	    ExReleasePushLock(&current->Lock);
            CcRosCacheSegmentDecRefCount(current);
	    KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
	 }

	 Offset.QuadPart += Bcb->CacheSegmentSize;
	 if (Length > Bcb->CacheSegmentSize)
	 {
	    Length -= Bcb->CacheSegmentSize;
	 }
	 else
	 {
	    Length = 0;
	 }
      }
   }
   else
   {
      if (IoStatus)
      {
	 IoStatus->Status = STATUS_INVALID_PARAMETER;
      }
   }
}

NTSTATUS
NTAPI
CcRosDeleteFileCache(PFILE_OBJECT FileObject, PBCB Bcb)
/*
 * FUNCTION: Releases the BCB associated with a file object
 */
{
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;
   NTSTATUS Status;
   LIST_ENTRY FreeList;
   KIRQL oldIrql;

   ASSERT(Bcb);

   Bcb->RefCount++;
   KeReleaseGuardedMutex(&ViewLock);

   CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, NULL);

   KeAcquireGuardedMutex(&ViewLock);
   Bcb->RefCount--;
   if (Bcb->RefCount == 0)
   {
      if (Bcb->BcbRemoveListEntry.Flink != NULL)
      {
	 RemoveEntryList(&Bcb->BcbRemoveListEntry);
         Bcb->BcbRemoveListEntry.Flink = NULL;
      }

      FileObject->SectionObjectPointer->SharedCacheMap = NULL;

      /*
       * Release all cache segments.
       */
      InitializeListHead(&FreeList);
      KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
      current_entry = Bcb->BcbSegmentListHead.Flink;
      while (!IsListEmpty(&Bcb->BcbSegmentListHead))
      {
         current_entry = RemoveTailList(&Bcb->BcbSegmentListHead);
         current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
         RemoveEntryList(&current->CacheSegmentListEntry);
         RemoveEntryList(&current->CacheSegmentLRUListEntry);
         if (current->Dirty)
	 {
            RemoveEntryList(&current->DirtySegmentListEntry);
            DirtyPageCount -= Bcb->CacheSegmentSize / PAGE_SIZE;
	    DPRINT1("Freeing dirty segment\n");
	 }
         InsertHeadList(&FreeList, &current->BcbSegmentListEntry);
      }
#if defined(DBG) || defined(KDBG)
      Bcb->Trace = FALSE;
#endif
      KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);

      KeReleaseGuardedMutex(&ViewLock);
      ObDereferenceObject (Bcb->FileObject);

      while (!IsListEmpty(&FreeList))
      {
         current_entry = RemoveTailList(&FreeList);
         current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
         Status = CcRosInternalFreeCacheSegment(current);
      }
      ExFreeToNPagedLookasideList(&BcbLookasideList, Bcb);
      KeAcquireGuardedMutex(&ViewLock);
   }
   return(STATUS_SUCCESS);
}

VOID
NTAPI
CcRosReferenceCache(PFILE_OBJECT FileObject)
{
  PBCB Bcb;
  KeAcquireGuardedMutex(&ViewLock);
  Bcb = (PBCB)FileObject->SectionObjectPointer->SharedCacheMap;
  ASSERT(Bcb);
  if (Bcb->RefCount == 0)
  {
     ASSERT(Bcb->BcbRemoveListEntry.Flink != NULL);
     RemoveEntryList(&Bcb->BcbRemoveListEntry);
     Bcb->BcbRemoveListEntry.Flink = NULL;

  }
  else
  {
     ASSERT(Bcb->BcbRemoveListEntry.Flink == NULL);
  }
  Bcb->RefCount++;
  KeReleaseGuardedMutex(&ViewLock);
}

VOID
NTAPI
CcRosSetRemoveOnClose(PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
  PBCB Bcb;
  DPRINT("CcRosSetRemoveOnClose()\n");
  KeAcquireGuardedMutex(&ViewLock);
  Bcb = (PBCB)SectionObjectPointer->SharedCacheMap;
  if (Bcb)
  {
    Bcb->RemoveOnClose = TRUE;
    if (Bcb->RefCount == 0)
    {
      CcRosDeleteFileCache(Bcb->FileObject, Bcb);
    }
  }
  KeReleaseGuardedMutex(&ViewLock);
}


VOID
NTAPI
CcRosDereferenceCache(PFILE_OBJECT FileObject)
{
  PBCB Bcb;
  KeAcquireGuardedMutex(&ViewLock);
  Bcb = (PBCB)FileObject->SectionObjectPointer->SharedCacheMap;
  ASSERT(Bcb);
  if (Bcb->RefCount > 0)
  {
    Bcb->RefCount--;
    if (Bcb->RefCount == 0)
    {
       MmFreeSectionSegments(Bcb->FileObject);
       if (Bcb->RemoveOnClose)
       {
          CcRosDeleteFileCache(FileObject, Bcb);
       }
       else
       {
	  Bcb->TimeStamp = CcTimeStamp;
          InsertHeadList(&ClosedListHead, &Bcb->BcbRemoveListEntry);
       }
    }
  }
  KeReleaseGuardedMutex(&ViewLock);
}

NTSTATUS STDCALL
CcRosReleaseFileCache(PFILE_OBJECT FileObject)
/*
 * FUNCTION: Called by the file system when a handle to a file object
 * has been closed.
 */
{
  PBCB Bcb;

  KeAcquireGuardedMutex(&ViewLock);

  if (FileObject->SectionObjectPointer->SharedCacheMap != NULL)
  {
    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
    if (FileObject->PrivateCacheMap != NULL)
    {
      FileObject->PrivateCacheMap = NULL;
      if (Bcb->RefCount > 0)
      {
         Bcb->RefCount--;
	 if (Bcb->RefCount == 0)
	 {
            MmFreeSectionSegments(Bcb->FileObject);
	    if (Bcb->RemoveOnClose)
	    {
	       CcRosDeleteFileCache(FileObject, Bcb);
	    }
	    else
	    {
	       Bcb->TimeStamp = CcTimeStamp;
	       InsertHeadList(&ClosedListHead, &Bcb->BcbRemoveListEntry);
	    }
	 }
      }
    }
  }
  KeReleaseGuardedMutex(&ViewLock);
  return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
CcTryToInitializeFileCache(PFILE_OBJECT FileObject)
{
   PBCB Bcb;
   NTSTATUS Status;

   KeAcquireGuardedMutex(&ViewLock);

   Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
   if (Bcb == NULL)
   {
      Status = STATUS_UNSUCCESSFUL;
   }
   else
   {
      if (FileObject->PrivateCacheMap == NULL)
      {
         FileObject->PrivateCacheMap = Bcb;
         Bcb->RefCount++;
      }
      if (Bcb->BcbRemoveListEntry.Flink != NULL)
      {
         RemoveEntryList(&Bcb->BcbRemoveListEntry);
         Bcb->BcbRemoveListEntry.Flink = NULL;
      }
      Status = STATUS_SUCCESS;
   }
   KeReleaseGuardedMutex(&ViewLock);

   return Status;
}


NTSTATUS STDCALL
CcRosInitializeFileCache(PFILE_OBJECT FileObject,
			 ULONG CacheSegmentSize)
/*
 * FUNCTION: Initializes a BCB for a file object
 */
{
   PBCB Bcb;

   Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
   DPRINT("CcRosInitializeFileCache(FileObject 0x%p, Bcb 0x%p, CacheSegmentSize %d)\n",
           FileObject, Bcb, CacheSegmentSize);

   KeAcquireGuardedMutex(&ViewLock);
   if (Bcb == NULL)
   {
      Bcb = ExAllocateFromNPagedLookasideList(&BcbLookasideList);
      if (Bcb == NULL)
      {
        KeReleaseGuardedMutex(&ViewLock);
	return(STATUS_UNSUCCESSFUL);
      }
      memset(Bcb, 0, sizeof(BCB));
      ObReferenceObjectByPointer(FileObject,
			         FILE_ALL_ACCESS,
			         NULL,
			         KernelMode);
      Bcb->FileObject = FileObject;
      Bcb->CacheSegmentSize = CacheSegmentSize;
      if (FileObject->FsContext)
      {
         Bcb->AllocationSize =
	   ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->AllocationSize;
         Bcb->FileSize =
	   ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->FileSize;
      }
      KeInitializeSpinLock(&Bcb->BcbLock);
      InitializeListHead(&Bcb->BcbSegmentListHead);
      FileObject->SectionObjectPointer->SharedCacheMap = Bcb;
   }
   if (FileObject->PrivateCacheMap == NULL)
   {
      FileObject->PrivateCacheMap = Bcb;
      Bcb->RefCount++;
   }
   if (Bcb->BcbRemoveListEntry.Flink != NULL)
   {
      RemoveEntryList(&Bcb->BcbRemoveListEntry);
      Bcb->BcbRemoveListEntry.Flink = NULL;
   }
   KeReleaseGuardedMutex(&ViewLock);

   return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
PFILE_OBJECT STDCALL
CcGetFileObjectFromSectionPtrs(IN PSECTION_OBJECT_POINTERS SectionObjectPointers)
{
   PBCB Bcb;
   if (SectionObjectPointers && SectionObjectPointers->SharedCacheMap)
   {
      Bcb = (PBCB)SectionObjectPointers->SharedCacheMap;
      ASSERT(Bcb);
      return Bcb->FileObject;
   }
   return NULL;
}

VOID STDCALL
CmLazyCloseThreadMain(PVOID Ignored)
{
   LARGE_INTEGER Timeout;
   PLIST_ENTRY current_entry;
   PBCB current;
   ULONG RemoveTimeStamp;
   NTSTATUS Status;

   KeQuerySystemTime (&Timeout);

   while (1)
   {
      Timeout.QuadPart += (LONGLONG)100000000; // 10sec
      Status = KeWaitForSingleObject(&LazyCloseThreadEvent,
	                             0,
				     KernelMode,
				     FALSE,
				     &Timeout);

      DPRINT("LazyCloseThreadMain %d\n", CcTimeStamp);

      if (!NT_SUCCESS(Status))
      {
	  DbgPrint("LazyCloseThread: Wait failed\n");
	  KEBUGCHECKCC;
	  break;
      }
      if (LazyCloseThreadShouldTerminate)
      {
          DbgPrint("LazyCloseThread: Terminating\n");
	  break;
      }

      KeAcquireGuardedMutex(&ViewLock);
      CcTimeStamp++;
      if (CcTimeStamp >= 30)
      {
         RemoveTimeStamp = CcTimeStamp - 30; /* 5min = 10sec * 30 */
         while (!IsListEmpty(&ClosedListHead))
	 {
            current_entry = ClosedListHead.Blink;
            current = CONTAINING_RECORD(current_entry, BCB, BcbRemoveListEntry);
	    if (current->TimeStamp >= RemoveTimeStamp)
	    {
	       break;
	    }
            CcRosDeleteFileCache(current->FileObject, current);
	 }
      }
      KeReleaseGuardedMutex(&ViewLock);
   }
}

VOID
INIT_FUNCTION
NTAPI
CcInitView(VOID)
{
#ifdef CACHE_BITMAP
  PMEMORY_AREA marea;
  PVOID Buffer;
  PHYSICAL_ADDRESS BoundaryAddressMultiple;
#endif
  NTSTATUS Status;
  KPRIORITY Priority;

  DPRINT("CcInitView()\n");
#ifdef CACHE_BITMAP
  BoundaryAddressMultiple.QuadPart = 0;
  CiCacheSegMappingRegionHint = 0;
  CiCacheSegMappingRegionBase = NULL;

  MmLockAddressSpace(MmGetKernelAddressSpace());

  Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
			      MEMORY_AREA_CACHE_SEGMENT,
			      &CiCacheSegMappingRegionBase,
			      CI_CACHESEG_MAPPING_REGION_SIZE,
			      PAGE_READWRITE,
			      &marea,
			      FALSE,
			      0,
			      BoundaryAddressMultiple);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
  if (!NT_SUCCESS(Status))
    {
      KEBUGCHECKCC;
    }

  Buffer = ExAllocatePool(NonPagedPool, CI_CACHESEG_MAPPING_REGION_SIZE / (PAGE_SIZE * 8));

  RtlInitializeBitMap(&CiCacheSegMappingRegionAllocMap, Buffer, CI_CACHESEG_MAPPING_REGION_SIZE / PAGE_SIZE);
  RtlClearAllBits(&CiCacheSegMappingRegionAllocMap);

  KeInitializeSpinLock(&CiCacheSegMappingRegionLock);
#endif
  InitializeListHead(&CacheSegmentListHead);
  InitializeListHead(&DirtySegmentListHead);
  InitializeListHead(&CacheSegmentLRUListHead);
  InitializeListHead(&ClosedListHead);
  KeInitializeGuardedMutex(&ViewLock);
  ExInitializeNPagedLookasideList (&iBcbLookasideList,
	                           NULL,
				   NULL,
				   0,
				   sizeof(INTERNAL_BCB),
				   TAG_IBCB,
				   20);
  ExInitializeNPagedLookasideList (&BcbLookasideList,
	                           NULL,
				   NULL,
				   0,
				   sizeof(BCB),
				   TAG_BCB,
				   20);
  ExInitializeNPagedLookasideList (&CacheSegLookasideList,
	                           NULL,
				   NULL,
				   0,
				   sizeof(CACHE_SEGMENT),
				   TAG_CSEG,
				   20);

  MmInitializeMemoryConsumer(MC_CACHE, CcRosTrimCache);

  CcInitCacheZeroPage();

  CcTimeStamp = 0;
  LazyCloseThreadShouldTerminate = FALSE;
  KeInitializeEvent (&LazyCloseThreadEvent, SynchronizationEvent, FALSE);
  Status = PsCreateSystemThread(&LazyCloseThreadHandle,
				THREAD_ALL_ACCESS,
				NULL,
				NULL,
				&LazyCloseThreadId,
				(PKSTART_ROUTINE)CmLazyCloseThreadMain,
				NULL);
  if (NT_SUCCESS(Status))
  {
     Priority = LOW_REALTIME_PRIORITY;
     NtSetInformationThread(LazyCloseThreadHandle,
			    ThreadPriority,
			    &Priority,
			    sizeof(Priority));
  }

}

/* EOF */








