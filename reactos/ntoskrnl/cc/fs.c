/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/fs.c
 * PURPOSE:         Implements cache managers functions useful for File Systems
 *
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#ifndef VACB_MAPPING_GRANULARITY
#define VACB_MAPPING_GRANULARITY (256 * 1024)
#endif

/* GLOBALS   *****************************************************************/

extern FAST_MUTEX ViewLock;
extern ULONG DirtyPageCount;

NTSTATUS CcRosInternalFreeCacheSegment(PCACHE_SEGMENT CacheSeg);

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
LARGE_INTEGER
STDCALL
CcGetDirtyPages (
	IN	PVOID			LogHandle,
	IN	PDIRTY_PAGE_ROUTINE	DirtyPageRoutine,
	IN	PVOID			Context1,
	IN	PVOID			Context2
	)
{
	LARGE_INTEGER i;
	UNIMPLEMENTED;
	i.QuadPart = 0;
	return i;
}

/*
 * @implemented
 */
PFILE_OBJECT
STDCALL
CcGetFileObjectFromBcb (
	IN	PVOID	Bcb
	)
{
	PINTERNAL_BCB iBcb = (PINTERNAL_BCB)Bcb;
	return iBcb->CacheSegment->Bcb->FileObject;
}

/*
 * @unimplemented
 */
LARGE_INTEGER
STDCALL
CcGetLsnForFileObject (
	IN	PFILE_OBJECT	FileObject,
	OUT	PLARGE_INTEGER	OldestLsn OPTIONAL
	)
{
	LARGE_INTEGER i;
	UNIMPLEMENTED;
	i.QuadPart = 0;
	return i;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcInitializeCacheMap (
	IN	PFILE_OBJECT			FileObject,
	IN	PCC_FILE_SIZES			FileSizes,
	IN	BOOLEAN				PinAccess,
	IN	PCACHE_MANAGER_CALLBACKS	CallBacks,
	IN	PVOID				LazyWriterContext
	)
{
    CcRosInitializeFileCache(FileObject, VACB_MAPPING_GRANULARITY);
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
CcIsThereDirtyData (
	IN	PVPB	Vpb
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
CcPurgeCacheSection (
	IN	PSECTION_OBJECT_POINTERS	SectionObjectPointer,
	IN	PLARGE_INTEGER			FileOffset OPTIONAL,
	IN	ULONG				Length,
	IN	BOOLEAN				UninitializeCacheMaps
	)
{
	UNIMPLEMENTED;
	return FALSE;
}


/*
 * @implemented
 */
VOID STDCALL
CcSetFileSizes (IN PFILE_OBJECT FileObject,
		IN PCC_FILE_SIZES FileSizes)
{
  KIRQL oldirql;
  PBCB Bcb;
  PLIST_ENTRY current_entry;
  PCACHE_SEGMENT current;
  LIST_ENTRY FreeListHead;
  NTSTATUS Status;

  DPRINT("CcSetFileSizes(FileObject 0x%p, FileSizes 0x%p)\n",
	 FileObject, FileSizes);
  DPRINT("AllocationSize %d, FileSize %d, ValidDataLength %d\n",
         (ULONG)FileSizes->AllocationSize.QuadPart,
         (ULONG)FileSizes->FileSize.QuadPart,
         (ULONG)FileSizes->ValidDataLength.QuadPart);

  Bcb = FileObject->SectionObjectPointer->SharedCacheMap;

  /*
   * It is valid to call this function on file objects that weren't
   * initialized for caching. In this case it's simple no-op.
   */
  if (Bcb == NULL)
     return;

  if (FileSizes->AllocationSize.QuadPart < Bcb->AllocationSize.QuadPart)
  {
     InitializeListHead(&FreeListHead);
     ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&ViewLock);
     KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);

     current_entry = Bcb->BcbSegmentListHead.Flink;
     while (current_entry != &Bcb->BcbSegmentListHead)
     {
	current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
	current_entry = current_entry->Flink;
	if (current->FileOffset > FileSizes->AllocationSize.QuadPart)
	{
           if (current->ReferenceCount == 0 || (current->ReferenceCount == 1 && current->Dirty))
	   {
              RemoveEntryList(&current->BcbSegmentListEntry);
              RemoveEntryList(&current->CacheSegmentListEntry);
              RemoveEntryList(&current->CacheSegmentLRUListEntry);
              if (current->Dirty)
              {
                 RemoveEntryList(&current->DirtySegmentListEntry);
                 DirtyPageCount -= Bcb->CacheSegmentSize / PAGE_SIZE;
              }
	      InsertHeadList(&FreeListHead, &current->BcbSegmentListEntry);
	   }
	   else
	   {
	      DPRINT1("Anyone has referenced a cache segment behind the new size.\n");
	      KEBUGCHECKCC;
	   }
	}
     }

     Bcb->AllocationSize = FileSizes->AllocationSize;
     Bcb->FileSize = FileSizes->FileSize;
     KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
     ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&ViewLock);

     current_entry = FreeListHead.Flink;
     while(current_entry != &FreeListHead)
     {
        current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
        current_entry = current_entry->Flink;
        Status = CcRosInternalFreeCacheSegment(current);
        if (!NT_SUCCESS(Status))
        {
           DPRINT1("CcRosInternalFreeCacheSegment failed, status = %x\n", Status);
	   KEBUGCHECK(0);
        }
     }
  }
  else
  {
     KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
     Bcb->AllocationSize = FileSizes->AllocationSize;
     Bcb->FileSize = FileSizes->FileSize;
     KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
  }
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcSetLogHandleForFile (
	IN	PFILE_OBJECT	FileObject,
	IN	PVOID		LogHandle,
	IN	PFLUSH_TO_LSN	FlushToLsnRoutine
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
CcUninitializeCacheMap (
	IN	PFILE_OBJECT			FileObject,
	IN	PLARGE_INTEGER			TruncateSize OPTIONAL,
	IN	PCACHE_UNINITIALIZE_EVENT	UninitializeCompleteEvent OPTIONAL
	)
{
#if 0
	UNIMPLEMENTED;
	return FALSE;
#else
    return NT_SUCCESS(CcRosReleaseFileCache(FileObject));
#endif
}
