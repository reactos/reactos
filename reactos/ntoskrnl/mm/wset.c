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
/* $Id: wset.c,v 1.10 2001/08/03 09:36:19 ei Exp $
 * 
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/wset.c
 * PURPOSE:         Manages working sets
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <ntos/minmax.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
KiInitializeCircularQueue(PKCIRCULAR_QUEUE Queue, ULONG MaximumSize, 
			  PVOID* Mem)
{
  Queue->MaximumSize = MaximumSize;
  Queue->CurrentSize = 0;
  Queue->First = Queue->Last = 0;
  Queue->Mem = Mem;
}

VOID
KiInsertItemCircularQueue(PKCIRCULAR_QUEUE Queue, PVOID Item)
{
  Queue->Mem[Queue->Last] = Item;
  Queue->Last = (Queue->Last + 1) % Queue->MaximumSize;
  Queue->CurrentSize++;
}

VOID
KiRemoveItemCircularQueue(PKCIRCULAR_QUEUE Queue, PVOID Item)
{
  ULONG i, j;

  j = Queue->First;
  for (i = 0; i < Queue->CurrentSize; i++)
    {
      if (Queue->Mem[j] == Item)
	{
	  if (j != Queue->First)
	    {
	      if (j > 0 && Queue->First <= j)
		{
		  memmove(&Queue->Mem[Queue->First + 1], 
			  &Queue->Mem[Queue->First],
			  sizeof(PVOID) * (j - Queue->First));
		}
	      else if (j > 0 && Queue->First > j)
		{
		  memmove(&Queue->Mem[1], &Queue->Mem[0],
			  sizeof(PVOID) * j);
		  Queue->Mem[0] = Queue->Mem[Queue->MaximumSize - 1];
		  memmove(&Queue[Queue->First + 1], &Queue[Queue->First],
			  sizeof(PVOID) * 
			  ((Queue->MaximumSize - 1) - Queue->First));
		}
	      else if (j == 0)
		{
		  Queue->Mem[0] = Queue->Mem[Queue->MaximumSize];
		  memmove(&Queue[Queue->First + 1], &Queue[Queue->First],
			  sizeof(PVOID) * 
			  ((Queue->MaximumSize - 1) - Queue->First));
		}
	    }
	  Queue->First = (Queue->First + 1) % Queue->MaximumSize;
	  Queue->CurrentSize--;
	  return;
	}
      j = (j + 1) % Queue->MaximumSize;
    }
  KeBugCheck(0);
}

PVOID 
MmGetDirtyPagesFromWorkingSet(struct _EPROCESS* Process)
{
   return(NULL);
}

VOID 
MmLockWorkingSet(PEPROCESS Process)
{
   (VOID)KeWaitForMutexObject(&Process->WorkingSetLock,
			      0,
			      KernelMode,
			      FALSE,
			      NULL);   
}

VOID MmUnlockWorkingSet(PEPROCESS Process)
{
   KeReleaseMutex(&Process->WorkingSetLock, FALSE);
}

VOID 
MmInitializeWorkingSet(PEPROCESS Process, PMADDRESS_SPACE AddressSpace)
{
  PVOID BaseAddress;
  ULONG MaximumLength;
  PVOID FirstPage;
  NTSTATUS Status;

  /*
   * The maximum number of pages in the working set is the maximum
   * of the size of physical memory and the size of the user address space.
   * In either case the maximum size is 3Mb.
   */
  MaximumLength = MmStats.NrTotalPages - MmStats.NrReservedPages;
  MaximumLength = min(MaximumLength, KERNEL_BASE / PAGESIZE);
  MaximumLength = PAGE_ROUND_UP(MaximumLength * sizeof(ULONG));
  
  FirstPage = MmAllocPageMaybeSwap(0);
  if (FirstPage == NULL)
    {
      KeBugCheck(0);
    }
  
  MmLockAddressSpace(MmGetKernelAddressSpace());
  BaseAddress = NULL;
  Status = MmCreateMemoryArea(NULL,
			      MmGetKernelAddressSpace(),
			      MEMORY_AREA_WORKING_SET,
			      &BaseAddress,
			      MaximumLength,
			      0,
			      &AddressSpace->WorkingSetArea,
			      FALSE);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
  if (!NT_SUCCESS(Status))
    {
      KeBugCheck(0);
    }
  KiInitializeCircularQueue(&Process->AddressSpace.WSQueue,
			    MaximumLength,
			    (PVOID*)BaseAddress);
  KeInitializeMutex(&Process->WorkingSetLock, 1);
  Process->WorkingSetPage = BaseAddress;
  Status = MmCreateVirtualMapping(NULL,
				  Process->WorkingSetPage,
				  PAGE_READWRITE,
				  (ULONG)FirstPage);
  if (!NT_SUCCESS(Status))
    {
      KeBugCheck(0);
    }
  memset(Process->WorkingSetPage, 0, 4096);
}

ULONG 
MmPageOutPage(PMADDRESS_SPACE AddressSpace,
	      PMEMORY_AREA MArea,
	      PVOID Address,
	      PBOOLEAN Ul)
{
   ULONG Count;
   
   switch(MArea->Type)
     {
      case MEMORY_AREA_SYSTEM:
	*Ul = FALSE;
	return(0);
	     
      case MEMORY_AREA_SECTION_VIEW_COMMIT:
	Count = MmPageOutSectionView(AddressSpace,
				     MArea,
				     Address,
				     Ul);
	return(Count);
		  
      case MEMORY_AREA_VIRTUAL_MEMORY:
	Count = MmPageOutVirtualMemory(AddressSpace,
				       MArea,
				       Address,
				       Ul);
	return(Count);
	
     }
   *Ul = FALSE;
   return(0);
}

VOID
MmLruAdjustWorkingSet(PEPROCESS Process)
{
  ULONG i, j;
  PVOID CurrentAddress;

  MmLockWorkingSet(Process);

  j = Process->AddressSpace.WSQueue.First;
  for (i = 0; i < Process->AddressSpace.WSQueue.CurrentSize; i++)
    {
      CurrentAddress = Process->AddressSpace.WSQueue.Mem[j];
      if (MmIsAccessedAndResetAccessPage(Process, CurrentAddress))
	{
	  DbgPrint("L");
	  KiRemoveItemCircularQueue(&Process->AddressSpace.WSQueue,
				    CurrentAddress);
	  KiInsertItemCircularQueue(&Process->AddressSpace.WSQueue,
				    CurrentAddress);
	}
      j = (j + 1) % Process->AddressSpace.WSQueue.MaximumSize;
    }

  MmUnlockWorkingSet(Process);
}

ULONG 
MmTrimWorkingSet(PEPROCESS Process, ULONG ReduceHint)
     /*
      * Reduce the size of the working set of a process
      */
{
   ULONG i, j;
   PMADDRESS_SPACE AddressSpace;
   ULONG Count;
   BOOLEAN Ul;
   
   MmLockWorkingSet(Process);
   
   AddressSpace = &Process->AddressSpace;
   
   Count = 0;
   j = AddressSpace->WSQueue.First;
   
   for (i = 0; i < AddressSpace->WSQueue.CurrentSize; )
     {
	PVOID Address;
	PMEMORY_AREA MArea;
	
	Address = AddressSpace->WSQueue.Mem[j];
		
	MArea = MmOpenMemoryAreaByAddress(AddressSpace, Address);
	if (MArea == NULL)
	  {
	    KeBugCheck(0);
	  }
	
	Count = Count + MmPageOutPage(AddressSpace, MArea, Address, &Ul);
	
	if (Ul)
	  {
	     MmLockWorkingSet(Process);
	     
	     j = AddressSpace->WSQueue.First;
	     i = 0;
	  }
	else
	  {
	     j = (j + 1) % AddressSpace->WSQueue.MaximumSize;
	     i++;
	  }
			
	if (Count == ReduceHint)
	  {
	     MmUnlockWorkingSet(Process);
	     return(Count);
	  }
     }
   MmUnlockWorkingSet(Process);
   return(Count);
}

VOID
MmRemovePageFromWorkingSet(PEPROCESS Process, PVOID Address)
     /*
      * Remove a page from a process's working set.
      */
{
   MmLockWorkingSet(Process);

   KiRemoveItemCircularQueue(&Process->AddressSpace.WSQueue, Address);

   MmUnlockWorkingSet(Process);
}

VOID
MmAddPageToWorkingSet(PEPROCESS Process, PVOID Address)
     /*
      * insert a page into a process's working set 
      */
{
   PMADDRESS_SPACE AddressSpace;
   PVOID NextAddress;

   AddressSpace = &Process->AddressSpace;
   
   /*
    * This can't happen unless there is a bug.
    */
   if (AddressSpace->WSQueue.CurrentSize == AddressSpace->WSQueue.MaximumSize)
     {
       KeBugCheck(0);
     }

   /*
    * lock the working set
    */
   MmLockWorkingSet(Process);

   /*
    * if we are growing the working set then check to see if we need
    * to allocate a page
    */   
   NextAddress = 
     (PVOID)PAGE_ROUND_DOWN((PVOID)&
		AddressSpace->WSQueue.Mem[AddressSpace->WSQueue.Last]);
   if (!MmIsPagePresent(NULL, NextAddress))
     {
       PVOID Page;
       NTSTATUS Status;

       /* FIXME: This isn't correct */
       Page = MmAllocPageMaybeSwap(0);
       if (Page == 0)
	 {
	   KeBugCheck(0);
	 }

       Status = MmCreateVirtualMapping(NULL,
				       NextAddress,
				       PAGE_READWRITE,
				       (ULONG)Page);
       if (!NT_SUCCESS(Status))
	 {
	   KeBugCheck(0);
	 }
     }

   /*
    * Insert the page in the working set
    */
   KiInsertItemCircularQueue(&AddressSpace->WSQueue, Address);

   /*
    * And unlock
    */
   MmUnlockWorkingSet(Process);
}



