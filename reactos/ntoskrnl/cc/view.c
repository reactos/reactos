/* $Id: view.c,v 1.8 2000/03/05 19:17:40 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/view.c
 * PURPOSE:         Cache manager
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <internal/mm.h>
#include <internal/cc.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL CcFlushCachePage(PCACHE_SEGMENT CacheSeg)
/*
 * FUNCTION: Asks the FSD to flush the contents of the page back to disk
 */
{
   KeWaitForSingleObject(&CacheSeg->Lock,
			 Executive,
			 KernelMode,
			 FALSE,
			 NULL);
   /* Build an IRP_MJ_WRITE and send it to the filesystem */
   KeSetEvent(&CacheSeg->Lock, IO_NO_INCREMENT, 0);
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS STDCALL CcReleaseCachePage(PBCB Bcb,
			    PCACHE_SEGMENT CacheSeg,
			    BOOLEAN Valid)
{
   DPRINT("CcReleaseCachePage(Bcb %x, CacheSeg %x, Valid %d)\n",
	  Bcb, CacheSeg, Valid);
   
   CacheSeg->ReferenceCount--;
   CacheSeg->Valid = Valid;
   KeSetEvent(&CacheSeg->Lock, IO_NO_INCREMENT, FALSE);
   
   DPRINT("CcReleaseCachePage() finished\n");
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL CcRequestCachePage(PBCB Bcb,
			    ULONG FileOffset,
			    PVOID* BaseAddress,
			    PBOOLEAN UptoDate,
			    PCACHE_SEGMENT* CacheSeg)
{
   KIRQL oldirql;
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;
   
   DPRINT("CcRequestCachePage(Bcb %x, FileOffset %x, BaseAddress %x, "
	  "UptoDate %x, CacheSeg %x)\n", Bcb, FileOffset, BaseAddress,
	  UptoDate, CacheSeg);
   
   KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
   
   current_entry = Bcb->CacheSegmentListHead.Flink;
   while (current_entry != &Bcb->CacheSegmentListHead)
     {
	current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, ListEntry);
	if (current->FileOffset == PAGE_ROUND_DOWN(FileOffset))
	  {
	     DPRINT("Found existing segment at %x\n", current);
	     current->ReferenceCount++;
	     KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
	     DPRINT("Waiting for segment\n");
	     KeWaitForSingleObject(&current->Lock,
				   Executive,
				   KernelMode,
				   FALSE,
				   NULL);
	     *UptoDate = current->Valid;
	     *BaseAddress = current->BaseAddress;
	     *CacheSeg = current;
	     DPRINT("Returning %x (UptoDate %d)\n", current, current->Valid);
	     return(STATUS_SUCCESS);
	  }
	current_entry = current_entry->Flink;
     }

   DPRINT("Creating new segment\n");
   
   KeReleaseSpinLock(&Bcb->BcbLock, oldirql);

   current = ExAllocatePool(NonPagedPool, sizeof(CACHE_SEGMENT));
   current->BaseAddress = NULL;
   MmCreateMemoryArea(KernelMode,
		      NULL,
		      MEMORY_AREA_CACHE_SEGMENT,
		      &current->BaseAddress,
		      CACHE_SEGMENT_SIZE,
		      PAGE_READWRITE,
		      (PMEMORY_AREA*)&current->MemoryArea);
   CHECKPOINT;
   current->Valid = FALSE;
   current->FileOffset = PAGE_ROUND_DOWN(FileOffset);
   current->Bcb = Bcb;
   CHECKPOINT;
   KeInitializeEvent(&current->Lock, SynchronizationEvent, FALSE);
   current->ReferenceCount = 1;
   CHECKPOINT;
   InsertTailList(&Bcb->CacheSegmentListHead, &current->ListEntry);
   CHECKPOINT;
   *UptoDate = current->Valid;
   *BaseAddress = current->BaseAddress;
   *CacheSeg = current;
   CHECKPOINT;
   MmSetPage(NULL,
	     current->BaseAddress,
	     PAGE_READWRITE,
	     (ULONG)MmAllocPage());
   
   
   DPRINT("Returning %x (BaseAddress %x)\n", current, *BaseAddress);
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL CcFreeCacheSegment(PFILE_OBJECT FileObject,
			    PBCB Bcb,
			    PCACHE_SEGMENT CacheSeg)
{
   MmFreeMemoryArea(NULL,
		    CacheSeg->BaseAddress,
		    CACHE_SEGMENT_SIZE,
		    TRUE);
   ExFreePool(CacheSeg);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL CcReleaseFileCache(PFILE_OBJECT FileObject,
			    PBCB Bcb)
{
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;
   
   DPRINT("CcReleaseFileCache(FileObject %x, Bcb %x)\n",
	  FileObject, Bcb);
   
   current_entry = Bcb->CacheSegmentListHead.Flink;
   while (current_entry != (&Bcb->CacheSegmentListHead))
     {
	current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, ListEntry);
	current_entry = current_entry->Flink;
	CcFreeCacheSegment(FileObject,
			   Bcb,
			   current);
     }
   
   ExFreePool(Bcb);
   
   DPRINT("CcReleaseFileCache() finished\n");
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL CcInitializeFileCache(PFILE_OBJECT FileObject,
			       PBCB* Bcb)
{
   DPRINT("CcInitializeFileCache(FileObject %x)\n",FileObject);
   
   (*Bcb) = ExAllocatePool(NonPagedPool, sizeof(BCB));
   if ((*Bcb) == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   (*Bcb)->FileObject = FileObject;
   InitializeListHead(&(*Bcb)->CacheSegmentListHead);
   KeInitializeSpinLock(&(*Bcb)->BcbLock);
   
   DPRINT("Finished CcInitializeFileCache() = %x\n", *Bcb);
   
   return(STATUS_SUCCESS);
}


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
VOID
STDCALL
CcMdlReadCompleteDev (
	IN	PMDL		MdlChain,
	IN	PDEVICE_OBJECT	DeviceObject
	)
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
VOID
STDCALL
CcMdlReadComplete (
	IN	PFILE_OBJECT	FileObject,
	IN	PMDL		MdlChain
	)
{
	PDEVICE_OBJECT	DeviceObject = NULL;

	DeviceObject = IoGetRelatedDeviceObject (FileObject);
	/* FIXME: try fast I/O first */
	CcMdlReadCompleteDev (
		MdlChain,
		DeviceObject
		);
}


/* EOF */
