/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: view.c,v 1.25 2001/05/01 23:08:18 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/view.c
 * PURPOSE:         Cache manager
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * PORTABILITY:     Checked
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
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) (((N) % (S)) ? ROUND_UP(N, S) - S : N)

#define TAG_CSEG  TAG('C', 'S', 'E', 'G')
#define TAG_BCB   TAG('B', 'C', 'B', ' ')

static LIST_ENTRY BcbListHead;
static KSPIN_LOCK BcbListLock;

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL 
CcFlushCacheSegment(PCACHE_SEGMENT CacheSeg)
/*
 * FUNCTION: Asks the FSD to flush the contents of the page back to disk
 */
{
   KeWaitForSingleObject(&CacheSeg->Lock,
			 Executive,
			 KernelMode,
			 FALSE,
			 NULL);
   /* FIXME: Build an IRP_MJ_WRITE and send it to the filesystem */
   KeSetEvent(&CacheSeg->Lock, IO_NO_INCREMENT, 0);
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS STDCALL 
CcReleaseCacheSegment(PBCB Bcb,
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

NTSTATUS
CcGetCacheSegment(PBCB Bcb,
		  ULONG FileOffset,
		  PULONG BaseOffset,
		  PVOID* BaseAddress,
		  PBOOLEAN UptoDate,
		  PCACHE_SEGMENT* CacheSeg)
{
   KIRQL oldirql;
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;
   ULONG i;
   
   KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
   
   current_entry = Bcb->CacheSegmentListHead.Flink;
   while (current_entry != &Bcb->CacheSegmentListHead)
     {
	current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbListEntry);
	if (current->FileOffset <= FileOffset &&
	    (current->FileOffset + Bcb->CacheSegmentSize) > FileOffset)
	  {
	     current->ReferenceCount++;
	     KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
	     KeWaitForSingleObject(&current->Lock,
				   Executive,
				   KernelMode,
				   FALSE,
				   NULL);
	     *UptoDate = current->Valid;
	     *BaseAddress = current->BaseAddress;
	     *CacheSeg = current;
	     *BaseOffset = current->FileOffset;
	     return(STATUS_SUCCESS);
	  }
	current_entry = current_entry->Flink;
     }

   DPRINT("Creating new segment\n");
   
   KeReleaseSpinLock(&Bcb->BcbLock, oldirql);

   current = ExAllocatePoolWithTag(NonPagedPool, sizeof(CACHE_SEGMENT), 
				   TAG_CSEG);
   current->BaseAddress = NULL;
   MmCreateMemoryArea(KernelMode,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_CACHE_SEGMENT,
		      &current->BaseAddress,
		      Bcb->CacheSegmentSize,
		      PAGE_READWRITE,
		      (PMEMORY_AREA*)&current->MemoryArea,
		      FALSE);
   current->Valid = FALSE;
   current->FileOffset = ROUND_DOWN(FileOffset, Bcb->CacheSegmentSize);
   current->Bcb = Bcb;
   KeInitializeEvent(&current->Lock, SynchronizationEvent, FALSE);
   current->ReferenceCount = 1;
   InsertTailList(&Bcb->CacheSegmentListHead, &current->BcbListEntry);
   *UptoDate = current->Valid;
   *BaseAddress = current->BaseAddress;
   *CacheSeg = current;
   *BaseOffset = current->FileOffset;
   for (i = 0; i < (Bcb->CacheSegmentSize / PAGESIZE); i++)
     {
       MmCreateVirtualMapping(NULL,
			      current->BaseAddress + (i * PAGESIZE),
			      PAGE_READWRITE,
			      (ULONG)MmAllocPage(0));
     }
   
   
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcRequestCacheSegment(PBCB Bcb,
		      ULONG FileOffset,
		      PVOID* BaseAddress,
		      PBOOLEAN UptoDate,
		      PCACHE_SEGMENT* CacheSeg)
/*
 * FUNCTION: Request a page mapping for a BCB
 */
{
  ULONG BaseOffset;
   
  if ((FileOffset % Bcb->CacheSegmentSize) != 0)
    {
      CPRINT("Bad fileoffset %x should be multiple of %x",
        FileOffset, Bcb->CacheSegmentSize);
      KeBugCheck(0);
    }

  return(CcGetCacheSegment(Bcb,
			   FileOffset,
			   &BaseOffset,
			   BaseAddress,
			   UptoDate,
			   CacheSeg));
}

STATIC VOID 
CcFreeCachePage(PVOID Context, PVOID Address, ULONG PhysAddr)
{
  if (PhysAddr != 0)
    {
      MmDereferencePage((PVOID)PhysAddr);
    }
}

NTSTATUS STDCALL 
CcFreeCacheSegment(PBCB Bcb, PCACHE_SEGMENT CacheSeg)
/*
 * FUNCTION: Releases a cache segment associated with a BCB
 */
{
  DPRINT("Freeing cache segment %x\n", CacheSeg);
  MmFreeMemoryArea(MmGetKernelAddressSpace(),
		   CacheSeg->BaseAddress,
		   Bcb->CacheSegmentSize,
		   CcFreeCachePage,
		   NULL);
  ExFreePool(CacheSeg);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcReleaseFileCache(PFILE_OBJECT FileObject, PBCB Bcb)
/*
 * FUNCTION: Releases the BCB associated with a file object
 */
{
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;
   
   DPRINT("CcReleaseFileCache(FileObject %x, Bcb %x)\n", FileObject, Bcb);

   MmFreeSectionSegments(FileObject);
   
   current_entry = Bcb->CacheSegmentListHead.Flink;
   while (current_entry != &Bcb->CacheSegmentListHead)
     {
	current = 
	  CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbListEntry);
	current_entry = current_entry->Flink;
	CcFreeCacheSegment(Bcb, current);
     }
   
   ExFreePool(Bcb);
   
   DPRINT("CcReleaseFileCache() finished\n");
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcInitializeFileCache(PFILE_OBJECT FileObject,
		      PBCB* Bcb,
		      ULONG CacheSegmentSize)
/*
 * FUNCTION: Initializes a BCB for a file object
 */
{
   DPRINT("CcInitializeFileCache(FileObject %x)\n",FileObject);
   
   (*Bcb) = ExAllocatePoolWithTag(NonPagedPool, sizeof(BCB), TAG_BCB);
   if ((*Bcb) == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   ObReferenceObjectByPointer(FileObject,
			      FILE_ALL_ACCESS,
			      NULL,
			      KernelMode);
   (*Bcb)->FileObject = FileObject;
   InitializeListHead(&(*Bcb)->CacheSegmentListHead);
   KeInitializeSpinLock(&(*Bcb)->BcbLock);
   (*Bcb)->CacheSegmentSize = CacheSegmentSize;

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

VOID
CcInitView(VOID)
{
  InitializeListHead(&BcbListHead);
  KeInitializeSpinLock(&BcbListLock);
}

/* EOF */
