/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <internal/mm.h>
#include <internal/cc.h>
#include <internal/pool.h>
#include <internal/io.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) (((N) % (S)) ? ROUND_UP(N, S) - S : N)

extern FAST_MUTEX ViewLock;
extern ULONG DirtyPageCount;

NTSTATUS CcRosInternalFreeCacheSegment(PCACHE_SEGMENT CacheSeg);

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME							INTERNAL
 * 	CcMdlReadCompleteDev@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	MdlChain
 *	DeviceObject
 *	
 * RETURN VALUE
 * 	None.
 *
 * NOTE
 * 	Used by CcMdlReadComplete@8 and FsRtl
 */
VOID STDCALL
CcMdlReadCompleteDev (IN	PMDL		MdlChain,
		      IN	PDEVICE_OBJECT	DeviceObject)
{
  UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	CcMdlReadComplete@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 	None.
 *
 * NOTE
 * 	From Bo Branten's ntifs.h v13.
 */
VOID STDCALL
CcMdlReadComplete (IN	PFILE_OBJECT	FileObject,
		   IN	PMDL		MdlChain)
{
   PDEVICE_OBJECT	DeviceObject = NULL;
   
   DeviceObject = IoGetRelatedDeviceObject (FileObject);
   /* FIXME: try fast I/O first */
   CcMdlReadCompleteDev (MdlChain,
			 DeviceObject);
}

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

  DPRINT("CcSetFileSizes(FileObject %x, FileSizes %x)\n", 
	 FileObject, FileSizes);
  DPRINT("AllocationSize %d, FileSize %d, ValidDataLength %d\n",
         (ULONG)FileSizes->AllocationSize.QuadPart,
         (ULONG)FileSizes->FileSize.QuadPart,
         (ULONG)FileSizes->ValidDataLength.QuadPart);

  Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
  assert(Bcb);
 
  if (FileSizes->AllocationSize.QuadPart < Bcb->AllocationSize.QuadPart)
  {
     InitializeListHead(&FreeListHead);
     ExAcquireFastMutex(&ViewLock);
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
	      KeBugCheck(0);
	   }
	}
     }
     
     Bcb->AllocationSize = FileSizes->AllocationSize;
     Bcb->FileSize = FileSizes->FileSize;
     KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
     ExReleaseFastMutex(&ViewLock);

     current_entry = FreeListHead.Flink;
     while(current_entry != &FreeListHead)
     {
        current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
        current_entry = current_entry->Flink;
        Status = CcRosInternalFreeCacheSegment(current);
        if (!NT_SUCCESS(Status))
        {
           DPRINT1("CcRosInternalFreeCacheSegment failed, status = %x\n");
	   KeBugCheck(0);
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

