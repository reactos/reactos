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
/* $Id: slab.c,v 1.3 2002/05/13 18:10:41 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/slab.c
 * PURPOSE:     Slab allocator.
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              Created 27/12/01
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES ********************************************************************/

typedef VOID (*SLAB_CACHE_CONSTRUCTOR)(VOID*, ULONG);
typedef VOID (*SLAB_CACHE_DESTRUCTOR)(VOID*, ULONG);

struct _SLAB_CACHE_PAGE;

typedef struct _SLAB_CACHE
{
  SLAB_CACHE_CONSTRUCTOR Constructor;
  SLAB_CACHE_DESTRUCTOR Destructor;
  ULONG BaseSize;
  ULONG ObjectSize;
  ULONG ObjectsPerPage;
  LIST_ENTRY PageListHead;
  struct _SLAB_CACHE_PAGE* FirstFreePage;
  KSPIN_LOCK SlabLock;
} SLAB_CACHE, *PSLAB_CACHE;

typedef struct _SLAB_CACHE_BUFCTL
{
  struct _SLAB_CACHE_BUFCTL* NextFree;
} SLAB_CACHE_BUFCTL, *PSLAB_CACHE_BUFCTL;

typedef struct _SLAB_CACHE_PAGE
{
  LIST_ENTRY PageListEntry;
  PSLAB_CACHE_BUFCTL FirstFreeBuffer;
  ULONG ReferenceCount;
} SLAB_CACHE_PAGE, *PSLAB_CACHE_PAGE;

/* GLOBALS ******************************************************************/

/* FUNCTIONS ****************************************************************/

PSLAB_CACHE
ExCreateSlabCache(PUNICODE_STRING Name, ULONG Size, ULONG Align,
		  SLAB_CACHE_CONSTRUCTOR Constructor,
		  SLAB_CACHE_DESTRUCTOR Destructor)
{
  PSLAB_CACHE Slab;
  ULONG ObjectSize;
  ULONG AlignSize;

  Slab = ExAllocatePool(NonPagedPool, sizeof(SLAB_CACHE));
  if (Slab == NULL)
    {
      return(NULL);
    }

  Slab->Constructor = Constructor;
  Slab->Destructor = Destructor;
  Slab->BaseSize = Size;
  ObjectSize = Size + sizeof(SLAB_CACHE_BUFCTL);
  AlignSize = Align - (ObjectSize % Align);
  Slab->ObjectSize = ObjectSize + AlignSize;
  Slab->ObjectsPerPage = 
    (PAGESIZE - sizeof(SLAB_CACHE_PAGE)) / Slab->ObjectSize;
  InitializeListHead(&Slab->PageListHead);
  KeInitializeSpinLock(&Slab->SlabLock);
  
  return(Slab);
}

PSLAB_CACHE_PAGE
ExGrowSlabCache(PSLAB_CACHE Slab)
{
  PSLAB_CACHE_PAGE SlabPage;
  ULONG_PTR PhysicalPage;
  PVOID Page;
  NTSTATUS Status;
  ULONG i;
  PSLAB_CACHE_BUFCTL BufCtl;
  PVOID Object;

  Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &PhysicalPage);
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }

  Page = ExAllocatePageWithPhysPage(PhysicalPage);
  if (Page == NULL)
    {
      MmReleasePageMemoryConsumer(MC_NPPOOL, PhysicalPage);
      return(NULL);
    }

  SlabPage = (PSLAB_CACHE_PAGE)(Page + PAGESIZE - sizeof(SLAB_CACHE_PAGE));
  SlabPage->ReferenceCount = 0;
  SlabPage->FirstFreeBuffer = (PSLAB_CACHE_BUFCTL)Page;
  for (i = 0; i < Slab->ObjectsPerPage; i++)
    {
      BufCtl = (PSLAB_CACHE_BUFCTL)(Page + (i * Slab->ObjectSize));
      Object = (PVOID)(BufCtl + 1);
      if (Slab->Constructor != NULL)
	{
	  Slab->Constructor(Object, Slab->BaseSize);
	}
      if (i == (Slab->ObjectsPerPage - 1))
	{
	  BufCtl->NextFree = 
	    (PSLAB_CACHE_BUFCTL)(Page + ((i + 1) * Slab->ObjectSize));
	}
      else
	{
	  BufCtl->NextFree = NULL;
	}
    }

  return(SlabPage);
}

PVOID
ExAllocateSlabCache(PSLAB_CACHE Slab, BOOLEAN MayWait)
{
  KIRQL oldIrql;
  PSLAB_CACHE_PAGE Page;
  PVOID Object;
  BOOLEAN NewPage;

  KeAcquireSpinLock(&Slab->SlabLock, &oldIrql);
  
  /*
   * Check if there is a page with free objects
   * present, if so allocate from it, if
   * not grow the slab.
   */
  if (Slab->FirstFreePage == NULL)
    {
      KeReleaseSpinLock(&Slab->SlabLock, oldIrql);
      Page = ExGrowSlabCache(Slab);
      NewPage = TRUE;
      KeAcquireSpinLock(&Slab->SlabLock, &oldIrql);
    }
  else
    {
      Page = Slab->FirstFreePage;
      NewPage = FALSE;
    }
  
  /*
   * We shouldn't have got a page without free buffers.
   */
  if (Page->FirstFreeBuffer == NULL)
    {
      DPRINT1("First free page had no free buffers.\n");
      KeBugCheck(0);
    }

  /*
   * Allocate the first free object from the page.
   */
  Object = (PVOID)Page->FirstFreeBuffer + sizeof(SLAB_CACHE_BUFCTL);
  Page->FirstFreeBuffer = Page->FirstFreeBuffer->NextFree;
  Page->ReferenceCount++;

  /*
   * If we just allocated all the objects from this page
   * and it was the first free page then adjust the
   * first free page pointer and move the page to the head
   * of the list.
   */
  if (Page->ReferenceCount == Slab->ObjectsPerPage && !NewPage)
    {
      if (Page->PageListEntry.Flink == &Slab->PageListHead)
	{
	  Slab->FirstFreePage = NULL;
	}
      else
	{
	  PSLAB_CACHE_PAGE NextPage;
	  
	  NextPage = CONTAINING_RECORD(Page->PageListEntry.Flink,
				       SLAB_CACHE_PAGE,
				       PageListEntry);
	  Slab->FirstFreePage = NextPage;
	}
      RemoveEntryList(&Page->PageListEntry);
      InsertHeadList(&Slab->PageListHead, &Page->PageListEntry);
    }
  /*
   * Otherwise if we created a new page then add it to the end of
   * the page list.
   */
  else if (NewPage)
    {
      InsertTailList(&Slab->PageListHead, &Page->PageListEntry);
      if (Slab->FirstFreePage == NULL)
	{
	  Slab->FirstFreePage = Page;
	}
    }
  KeReleaseSpinLock(&Slab->SlabLock, oldIrql);
  return(Object);
}

VOID
ExFreeFromPageSlabCache(PSLAB_CACHE Slab,
			PSLAB_CACHE_PAGE Page,
			PVOID Object)
{
  PSLAB_CACHE_BUFCTL BufCtl;

  BufCtl = (PSLAB_CACHE_BUFCTL)(Object - sizeof(SLAB_CACHE_BUFCTL));
  BufCtl->NextFree = Page->FirstFreeBuffer;
  Page->FirstFreeBuffer = BufCtl;
  Page->ReferenceCount--;
}

VOID
ExFreeSlabCache(PSLAB_CACHE Slab, PVOID Object)
{
  KIRQL oldIrql;
  PLIST_ENTRY current_entry;
  PSLAB_CACHE_PAGE current;

  KeAcquireSpinLock(&Slab->SlabLock, &oldIrql);
  current_entry = Slab->PageListHead.Flink;
  while (current_entry != &Slab->PageListHead)
    {
      PVOID Base;

      current = CONTAINING_RECORD(current_entry,
				  SLAB_CACHE_PAGE,
				  PageListEntry);
      Base = (PVOID)current + sizeof(SLAB_CACHE_PAGE) - PAGESIZE;
      if (Base >= Object && 
	  (Base + PAGESIZE - sizeof(SLAB_CACHE_PAGE)) >= 
	   (Object + Slab->ObjectSize))
	{
	  ExFreeFromPageSlabCache(Slab, current, Object);
	  /*
	   * If the page just become free then rearrange things.
	   */
	  if (current->ReferenceCount == 0)
	    {
	      RemoveEntryList(&current->PageListEntry);
	      InsertTailList(&Slab->PageListHead, &current->PageListEntry);
	      if (Slab->FirstFreePage == NULL)
		{
		  Slab->FirstFreePage = current;
		}
	    }
	  KeReleaseSpinLock(&Slab->SlabLock, oldIrql);
	  return;
	}
    }
  DPRINT1("Tried to free object not in cache.\n");
  KeBugCheck(0);
}

VOID
ExDestroySlabCache(PSLAB_CACHE Slab)
{
  PLIST_ENTRY current_entry;
  PSLAB_CACHE_PAGE current;
  ULONG i;
  PVOID Object;

  current_entry = Slab->PageListHead.Flink;
  while (current_entry != &Slab->PageListHead)
    {
      PVOID Base;
      ULONG_PTR PhysicalPage;

      current = CONTAINING_RECORD(current_entry,
				  SLAB_CACHE_PAGE,
				  PageListEntry);
      Base = (PVOID)(current + sizeof(SLAB_CACHE_PAGE) - PAGESIZE);
      if (Slab->Destructor != NULL)
	{
	  for (i = 0; i < Slab->ObjectsPerPage; i++)
	    {
	      Object = (PVOID)(Base + (i * Slab->ObjectSize) +
		sizeof(SLAB_CACHE_BUFCTL));
	      Slab->Destructor(Object, Slab->BaseSize);
	    }
	}
      PhysicalPage = MmGetPhysicalAddressForProcess(NULL, Base);
      ExUnmapPage(Base);
      MmReleasePageMemoryConsumer(MC_NPPOOL, PhysicalPage);
    }
  ExFreePool(Slab);
}
