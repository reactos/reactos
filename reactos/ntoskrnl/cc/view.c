/*
 *  ReactOS kernel
 *  Copyright (C) 2000, 1999, 1998 David Welch <welch@cwcom.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: view.c,v 1.11 2000/12/10 23:42:00 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/view.c
 * PURPOSE:         Cache manager
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
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

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <internal/mm.h>
#include <internal/cc.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL 
CcFlushCachePage(PCACHE_SEGMENT CacheSeg)
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

NTSTATUS STDCALL 
CcReleaseCachePage(PBCB Bcb,
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

NTSTATUS STDCALL 
CcRequestCachePage(PBCB Bcb,
		   ULONG FileOffset,
		   PVOID* BaseAddress,
		   PBOOLEAN UptoDate,
		   PCACHE_SEGMENT* CacheSeg)
/*
 * FUNCTION: Request a page mapping for a BCB
 */
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
   current->Valid = FALSE;
   current->FileOffset = PAGE_ROUND_DOWN(FileOffset);
   current->Bcb = Bcb;
   KeInitializeEvent(&current->Lock, SynchronizationEvent, FALSE);
   current->ReferenceCount = 1;
   InsertTailList(&Bcb->CacheSegmentListHead, &current->ListEntry);
   *UptoDate = current->Valid;
   *BaseAddress = current->BaseAddress;
   *CacheSeg = current;
   MmCreateVirtualMapping(NULL,
			  current->BaseAddress,
			  PAGE_READWRITE,
			  (ULONG)MmAllocPage(0));
   
   
   DPRINT("Returning %x (BaseAddress %x)\n", current, *BaseAddress);
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcFreeCacheSegment(PBCB Bcb,
		   PCACHE_SEGMENT CacheSeg)
/*
 * FUNCTION: Releases a cache segment associated with a BCB
 */
{
   MmFreeMemoryArea(NULL,
		    CacheSeg->BaseAddress,
		    CACHE_SEGMENT_SIZE,
		    TRUE);
   ExFreePool(CacheSeg);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcReleaseFileCache(PFILE_OBJECT FileObject,
		   PBCB Bcb)
/*
 * FUNCTION: Releases the BCB associated with a file object
 */
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
	CcFreeCacheSegment(Bcb,
			   current);
     }
   
   ExFreePool(Bcb);
   
   DPRINT("CcReleaseFileCache() finished\n");
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcInitializeFileCache(PFILE_OBJECT FileObject,
		      PBCB* Bcb)
/*
 * FUNCTION: Initializes a BCB for a file object
 */
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
