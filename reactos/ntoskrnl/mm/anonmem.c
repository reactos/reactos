/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
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
/* $Id: anonmem.c,v 1.17 2003/07/11 01:23:15 royce Exp $
 *
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/anonmem.c
 * PURPOSE:     Implementing anonymous memory.
 * PROGRAMMER:  David Welch
 */

/* INCLUDE *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS 
MmWritePageVirtualMemory(PMADDRESS_SPACE AddressSpace,
			 PMEMORY_AREA MemoryArea,
			 PVOID Address,
			 PMM_PAGEOP PageOp)
{
  SWAPENTRY SwapEntry;
  LARGE_INTEGER PhysicalAddress;
  PMDL Mdl;
  NTSTATUS Status;

  /*
   * Check for paging out from a deleted virtual memory area.
   */
  if (MemoryArea->DeleteInProgress)
    {
      PageOp->Status = STATUS_UNSUCCESSFUL;
      KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp(PageOp);
      return(STATUS_UNSUCCESSFUL);
    }

  PhysicalAddress = 
    MmGetPhysicalAddressForProcess(AddressSpace->Process, Address);

  /*
   * Get that the page actually is dirty.
   */
  if (!MmIsDirtyPage(MemoryArea->Process, Address))
    {
      PageOp->Status = STATUS_SUCCESS;
      KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
    }

  /*
   * Speculatively set the mapping to clean.
   */
  MmSetCleanPage(MemoryArea->Process, Address);
  
  /*
   * If necessary, allocate an entry in the paging file for this page
   */
  SwapEntry = MmGetSavedSwapEntryPage(PhysicalAddress);
  if (SwapEntry == 0)
    {
      SwapEntry = MmAllocSwapPage();
      if (SwapEntry == 0)
	{
	  MmSetDirtyPage(MemoryArea->Process, Address);
	  PageOp->Status = STATUS_PAGEFILE_QUOTA_EXCEEDED;
	  KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	  MmReleasePageOp(PageOp);
	  return(STATUS_PAGEFILE_QUOTA_EXCEEDED);
	}
    }

  /*
   * Write the page to the pagefile
   */
  Mdl = MmCreateMdl(NULL, NULL, PAGE_SIZE);
  MmBuildMdlFromPages(Mdl, (PULONG)&PhysicalAddress);
  Status = MmWriteToSwapPage(SwapEntry, Mdl);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n", 
	      Status);
      MmSetDirtyPage(MemoryArea->Process, Address);
      PageOp->Status = STATUS_UNSUCCESSFUL;
      KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp(PageOp);
      return(STATUS_UNSUCCESSFUL);
    }
  
  /*
   * Otherwise we have succeeded.
   */
  MmSetSavedSwapEntryPage(PhysicalAddress, SwapEntry);
  PageOp->Status = STATUS_SUCCESS;
  KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
  MmReleasePageOp(PageOp);
  return(STATUS_SUCCESS);
}

NTSTATUS 
MmPageOutVirtualMemory(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MemoryArea,
		       PVOID Address,
		       PMM_PAGEOP PageOp)
{
   PHYSICAL_ADDRESS PhysicalAddress;
   BOOL WasDirty;
   SWAPENTRY SwapEntry;
   NTSTATUS Status;
   PMDL Mdl;

   DPRINT("MmPageOutVirtualMemory(Address 0x%.8X) PID %d\n",
	   Address, MemoryArea->Process->UniqueProcessId);

   /*
    * Check for paging out from a deleted virtual memory area.
    */
   if (MemoryArea->DeleteInProgress)
     {
       PageOp->Status = STATUS_UNSUCCESSFUL;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       return(STATUS_UNSUCCESSFUL);
     }

   /*
    * Disable the virtual mapping.
    */
   MmDisableVirtualMapping(MemoryArea->Process, Address,
			   &WasDirty, &PhysicalAddress);

   if (PhysicalAddress.QuadPart == 0)
     {
       KeBugCheck(0);
     }

   /*
    * Paging out non-dirty data is easy. 
    */
   if (!WasDirty)
     {
       MmDeleteVirtualMapping(MemoryArea->Process, Address, FALSE, NULL, NULL);
       MmDeleteAllRmaps(PhysicalAddress, NULL, NULL);
       if ((SwapEntry = MmGetSavedSwapEntryPage(PhysicalAddress)) != 0)
	 {
	   MmCreatePageFileMapping(MemoryArea->Process, Address, SwapEntry);
	   MmSetSavedSwapEntryPage(PhysicalAddress, 0);
	 }
       MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
       PageOp->Status = STATUS_SUCCESS;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       return(STATUS_SUCCESS);
     }

   /*
    * If necessary, allocate an entry in the paging file for this page
    */
   SwapEntry = MmGetSavedSwapEntryPage(PhysicalAddress);
   if (SwapEntry == 0)
     {
       SwapEntry = MmAllocSwapPage();
       if (SwapEntry == 0)
	 {
	   MmShowOutOfSpaceMessagePagingFile();
	   MmEnableVirtualMapping(MemoryArea->Process, Address);
	   PageOp->Status = STATUS_UNSUCCESSFUL;
	   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	   MmReleasePageOp(PageOp);
	   return(STATUS_UNSUCCESSFUL);
	 }
     }
   
   /*
    * Write the page to the pagefile
    */
   Mdl = MmCreateMdl(NULL, NULL, PAGE_SIZE);
   MmBuildMdlFromPages(Mdl, (ULONG *)&PhysicalAddress.u.LowPart);
   Status = MmWriteToSwapPage(SwapEntry, Mdl);
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n", 
	       Status);
       MmEnableVirtualMapping(MemoryArea->Process, Address);
       PageOp->Status = STATUS_UNSUCCESSFUL;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       return(STATUS_UNSUCCESSFUL);
     }

   /*
    * Otherwise we have succeeded, free the page
    */
   DPRINT("MM: Swapped out virtual memory page 0x%.8X!\n", PhysicalAddress);
   MmDeleteVirtualMapping(MemoryArea->Process, Address, FALSE, NULL, NULL);
   MmCreatePageFileMapping(MemoryArea->Process, Address, SwapEntry);
   MmDeleteAllRmaps(PhysicalAddress, NULL, NULL);
   MmSetSavedSwapEntryPage(PhysicalAddress, 0);
   MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
   PageOp->Status = STATUS_SUCCESS;
   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
   MmReleasePageOp(PageOp);
   return(STATUS_SUCCESS);
}

NTSTATUS
MmNotPresentFaultVirtualMemory(PMADDRESS_SPACE AddressSpace,
			       MEMORY_AREA* MemoryArea, 
			       PVOID Address,
			       BOOLEAN Locked)
/*
 * FUNCTION: Move data into memory to satisfy a page not present fault
 * ARGUMENTS:
 *      AddressSpace = Address space within which the fault occurred
 *      MemoryArea = The memory area within which the fault occurred
 *      Address = The absolute address of fault
 * RETURNS: Status
 * NOTES: This function is called with the address space lock held.
 */
{
   PHYSICAL_ADDRESS Page;
   NTSTATUS Status;
   PMM_REGION Region;
   PMM_PAGEOP PageOp;
   
   /*
    * There is a window between taking the page fault and locking the
    * address space when another thread could load the page so we check
    * that.
    */
   if (MmIsPagePresent(NULL, Address))
     {
	if (Locked)
	  {
	    MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
	  }  
	return(STATUS_SUCCESS);
     }

   /*
    * Check for the virtual memory area being deleted.
    */
   if (MemoryArea->DeleteInProgress)
     {
       return(STATUS_UNSUCCESSFUL);
     }

   /*
    * Get the segment corresponding to the virtual address
    */
   Region = MmFindRegion(MemoryArea->BaseAddress, 
			 &MemoryArea->Data.VirtualMemoryData.RegionListHead,
			 Address, NULL);
   if (Region->Type == MEM_RESERVE)
     {
	return(STATUS_UNSUCCESSFUL);
     }

   /*
    * Get or create a page operation
    */
   PageOp = MmGetPageOp(MemoryArea, (ULONG)MemoryArea->Process->UniqueProcessId, 
			(PVOID)PAGE_ROUND_DOWN(Address), NULL, 0,
			MM_PAGEOP_PAGEIN);
   if (PageOp == NULL)
     {
       DPRINT1("MmGetPageOp failed");
       KeBugCheck(0);
     }

   /*
    * Check if someone else is already handling this fault, if so wait
    * for them
    */
   if (PageOp->Thread != PsGetCurrentThread())
     {
       MmUnlockAddressSpace(AddressSpace);
       Status = KeWaitForSingleObject(&PageOp->CompletionEvent,
				      0,
				      KernelMode,
				      FALSE,
				      NULL);
       /*
	* Check for various strange conditions
	*/
       if (Status != STATUS_SUCCESS)
	 {
	   DPRINT1("Failed to wait for page op\n");
	   KeBugCheck(0);
	 }
       if (PageOp->Status == STATUS_PENDING)
	 {
	   DPRINT1("Woke for page op before completion\n");
	   KeBugCheck(0);
	 }
       /*
	* If this wasn't a pagein then we need to restart the handling
	*/
       if (PageOp->OpType != MM_PAGEOP_PAGEIN)
	 {
           MmLockAddressSpace(AddressSpace);
           KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	   MmReleasePageOp(PageOp);
	   return(STATUS_MM_RESTART_OPERATION);
	 }
       /*
	* If the thread handling this fault has failed then we don't retry
	*/
       if (!NT_SUCCESS(PageOp->Status))
	 {
           MmLockAddressSpace(AddressSpace);
           KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
           Status = PageOp->Status;
	   MmReleasePageOp(PageOp);
	   return(Status);
	 }
       MmLockAddressSpace(AddressSpace);
       if (Locked)
	 {
	   MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
	 }
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       return(STATUS_SUCCESS);
     }
   
   /*
    * Try to allocate a page
    */
   Status = MmRequestPageMemoryConsumer(MC_USER, FALSE, &Page);
   if (Status == STATUS_NO_MEMORY)
     {
       MmUnlockAddressSpace(AddressSpace);
       Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
       MmLockAddressSpace(AddressSpace);
     }
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("MmRequestPageMemoryConsumer failed, status = %x\n", Status);
      KeBugCheck(0);
   }

   /*
    * Handle swapped out pages.
    */
   if (MmIsPageSwapEntry(NULL, Address))
     {
       SWAPENTRY SwapEntry;
       PMDL Mdl;

       MmDeletePageFileMapping(NULL, Address, &SwapEntry);
       Mdl = MmCreateMdl(NULL, NULL, PAGE_SIZE);
       MmBuildMdlFromPages(Mdl, (PULONG)&Page);
       Status = MmReadFromSwapPage(SwapEntry, Mdl);
       if (!NT_SUCCESS(Status))
	 {
	   KeBugCheck(0);
	 }
       MmSetSavedSwapEntryPage(Page, SwapEntry);
     }
   
   /*
    * Set the page. If we fail because we are out of memory then
    * try again
    */
   Status = MmCreateVirtualMapping(MemoryArea->Process,		      
				   (PVOID)PAGE_ROUND_DOWN(Address),
				   MemoryArea->Attributes,
				   Page,
				   FALSE);
   while (Status == STATUS_NO_MEMORY)
     {
	MmUnlockAddressSpace(AddressSpace);
	Status = MmCreateVirtualMapping(MemoryArea->Process,	      
					Address,
					MemoryArea->Attributes,
					Page,
					TRUE);
	MmLockAddressSpace(AddressSpace);
     }  
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("MmCreateVirtualMapping failed, not out of memory\n");
       KeBugCheck(0);
       return(Status);
     }

   /*
    * Add the page to the process's working set
    */
   MmInsertRmap(Page, MemoryArea->Process, (PVOID)PAGE_ROUND_DOWN(Address));

   /*
    * Finish the operation
    */
   if (Locked)
     {
       MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
     }  
   PageOp->Status = STATUS_SUCCESS;
   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
   MmReleasePageOp(PageOp);
   return(STATUS_SUCCESS);
}

VOID STATIC
MmModifyAttributes(PMADDRESS_SPACE AddressSpace,
		   PVOID BaseAddress,
		   ULONG RegionSize,
		   ULONG OldType,
		   ULONG OldProtect,
		   ULONG NewType,
		   ULONG NewProtect)
/*
 * FUNCTION: Modify the attributes of a memory region
 */
{      
  /*
   * If we are switching a previously committed region to reserved then
   * free any allocated pages within the region
   */
  if (NewType == MEM_RESERVE && OldType == MEM_COMMIT)
    {
      ULONG i;
      
      for (i=0; i < PAGE_ROUND_UP(RegionSize)/PAGE_SIZE; i++)
	{
	  LARGE_INTEGER PhysicalAddr;

	  if (MmIsPageSwapEntry(AddressSpace->Process,
				BaseAddress + (i * PAGE_SIZE)))
	    {
	      SWAPENTRY SwapEntry;
	      
	      MmDeletePageFileMapping(AddressSpace->Process,
				      BaseAddress + (i * PAGE_SIZE),
				      &SwapEntry);
	      MmFreeSwapPage(SwapEntry);
	    }
	  else
	    {
	      PhysicalAddr = MmGetPhysicalAddress(BaseAddress + (i*PAGE_SIZE));
	      MmDeleteVirtualMapping(AddressSpace->Process,
				     BaseAddress + (i*PAGE_SIZE),
				     FALSE, NULL, NULL);
	      if (PhysicalAddr.QuadPart != 0)
		{
		  SWAPENTRY SavedSwapEntry;
		  SavedSwapEntry = MmGetSavedSwapEntryPage(PhysicalAddr);
		  if (SavedSwapEntry != 0)
		    {
		      MmFreeSwapPage(SavedSwapEntry);
		      MmSetSavedSwapEntryPage(PhysicalAddr, 0);
		    }
		  MmDeleteRmap(PhysicalAddr, AddressSpace->Process,
			       BaseAddress + (i * PAGE_SIZE));
		  MmReleasePageMemoryConsumer(MC_USER, PhysicalAddr);
		}
	    }
	}
    }

  /*
   * If we are changing the protection attributes of a committed region then
   * alter the attributes for any allocated pages within the region
   */
  if (NewType == MEM_COMMIT && OldType == MEM_COMMIT && 
      OldProtect != NewProtect)
    {
      ULONG i;
   
      for (i=0; i < PAGE_ROUND_UP(RegionSize)/PAGE_SIZE; i++)
	{
	  if (MmIsPagePresent(AddressSpace->Process, 
			      BaseAddress + (i*PAGE_SIZE)))
	    {
	      MmSetPageProtect(AddressSpace->Process,
			       BaseAddress + (i*PAGE_SIZE), 
			       NewProtect);
	    }
	}
    }
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtAllocateVirtualMemory(IN	HANDLE	ProcessHandle,
			IN OUT	PVOID*  UBaseAddress,
			IN	ULONG	ZeroBits,
			IN OUT	PULONG	URegionSize,
			IN	ULONG	AllocationType,
			IN	ULONG	Protect)
/*
 * FUNCTION: Allocates a block of virtual memory in the process address space
 * ARGUMENTS:
 *      ProcessHandle = The handle of the process which owns the virtual memory
 *      BaseAddress   = A pointer to the virtual memory allocated. If you 
 *                      supply a non zero value the system will try to 
 *                      allocate the memory at the address supplied. It round 
 *                      it down to a multiple  of the page size.
 *      ZeroBits  = (OPTIONAL) You can specify the number of high order bits 
 *                      that must be zero, ensuring that the memory will be 
 *                      allocated at a address below a certain value.
 *      RegionSize = The number of bytes to allocate
 *      AllocationType = Indicates the type of virtual memory you like to 
 *                       allocated, can be a combination of MEM_COMMIT, 
 *                       MEM_RESERVE, MEM_RESET, MEM_TOP_DOWN.
 *      Protect = Indicates the protection type of the pages allocated, can be
 *                a combination of PAGE_READONLY, PAGE_READWRITE, 
 *                PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE, PAGE_GUARD, 
 *                PAGE_NOACCESS
 * RETURNS: Status
 */
{
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;
   ULONG Type;
   NTSTATUS Status;
   PMADDRESS_SPACE AddressSpace;
   PVOID BaseAddress;
   ULONG RegionSize;
   PVOID PBaseAddress;
   ULONG PRegionSize;

   DPRINT("NtAllocateVirtualMemory(*UBaseAddress %x, "
	  "ZeroBits %d, *URegionSize %x, AllocationType %x, Protect %x)\n",
	  *UBaseAddress,ZeroBits,*URegionSize,AllocationType,
	  Protect);
   
   /*
    * Check the validity of the parameters
    */
   if ((Protect & PAGE_FLAGS_VALID_FROM_USER_MODE) != Protect)
     {
       return(STATUS_INVALID_PAGE_PROTECTION);
     }
   if ((AllocationType & (MEM_COMMIT | MEM_RESERVE)) == 0)
     {
       return(STATUS_INVALID_PARAMETER);
     }
   
   PBaseAddress = *UBaseAddress;
   PRegionSize = *URegionSize;
   
   BaseAddress = (PVOID)PAGE_ROUND_DOWN(PBaseAddress);
   RegionSize = PAGE_ROUND_UP(PBaseAddress + PRegionSize) -
     PAGE_ROUND_DOWN(PBaseAddress);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      NULL,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtAllocateVirtualMemory() = %x\n",Status);
	return(Status);
     }
   
   Type = (AllocationType & MEM_COMMIT) ? MEM_COMMIT : MEM_RESERVE;
   DPRINT("Type %x\n", Type);
   
   AddressSpace = &Process->AddressSpace;
   MmLockAddressSpace(AddressSpace);
   
   if (PBaseAddress != 0)
     {
	MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					       BaseAddress);
	
	if (MemoryArea != NULL &&
	    MemoryArea->Type == MEMORY_AREA_VIRTUAL_MEMORY &&
	    MemoryArea->Length >= RegionSize)
	  {
	    Status = 
	      MmAlterRegion(AddressSpace, 
			    MemoryArea->BaseAddress, 
			    &MemoryArea->Data.VirtualMemoryData.RegionListHead,
			    BaseAddress, RegionSize,
			    Type, Protect, MmModifyAttributes);
	    MmUnlockAddressSpace(AddressSpace);
	    ObDereferenceObject(Process);
	    DPRINT("NtAllocateVirtualMemory() = %x\n",Status);
	    return(Status);
	  }
	else if (MemoryArea != NULL)
	  {
	    MmUnlockAddressSpace(AddressSpace);
	    ObDereferenceObject(Process);
	    return(STATUS_UNSUCCESSFUL);
	  }
     }
   
   Status = MmCreateMemoryArea(Process,
			       AddressSpace,
			       MEMORY_AREA_VIRTUAL_MEMORY,
			       &BaseAddress,
			       RegionSize,
			       Protect,
			       &MemoryArea,
			       PBaseAddress != 0,
			       (AllocationType & MEM_TOP_DOWN));
   if (!NT_SUCCESS(Status))
     {
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	DPRINT("NtAllocateVirtualMemory() = %x\n",Status);
	return(Status);
     }
   MmInitialiseRegion(&MemoryArea->Data.VirtualMemoryData.RegionListHead,
		      RegionSize, Type, Protect);
   
   if ((AllocationType & MEM_COMMIT) &&
       ((Protect & PAGE_READWRITE) ||
	(Protect & PAGE_EXECUTE_READWRITE)))
     {
	MmReserveSwapPages(RegionSize);
     }
   
   *UBaseAddress = BaseAddress;
   *URegionSize = RegionSize;
   DPRINT("*UBaseAddress %x  *URegionSize %x\n", BaseAddress, RegionSize);
   
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}

VOID STATIC
MmFreeVirtualMemoryPage(PVOID Context,
			MEMORY_AREA* MemoryArea,
			PVOID Address,
			PHYSICAL_ADDRESS PhysicalAddr,
			SWAPENTRY SwapEntry,
			BOOLEAN Dirty)
{
  PEPROCESS Process = (PEPROCESS)Context;
  
  if (PhysicalAddr.QuadPart != 0)
    {
      SWAPENTRY SavedSwapEntry;
      SavedSwapEntry = MmGetSavedSwapEntryPage(PhysicalAddr);
      if (SavedSwapEntry != 0)
	{
	  MmFreeSwapPage(SavedSwapEntry);
	  MmSetSavedSwapEntryPage(PhysicalAddr, 0);
	}
      MmDeleteRmap(PhysicalAddr, Process, Address);
      MmReleasePageMemoryConsumer(MC_USER, PhysicalAddr);
    }
  else if (SwapEntry != 0)
    {
      MmFreeSwapPage(SwapEntry);
    }
}

VOID
MmFreeVirtualMemory(PEPROCESS Process,
		    PMEMORY_AREA MemoryArea)
{
  PLIST_ENTRY current_entry;
  PMM_REGION current;
  ULONG i;
  
  DPRINT("MmFreeVirtualMemory(Process %p  MemoryArea %p)\n", Process, 
	 MemoryArea);
  
  /* Mark this memory area as about to be deleted. */
  MemoryArea->DeleteInProgress = TRUE;

  /* 
   * Wait for any ongoing paging operations. Notice that since we have
   * flagged this memory area as deleted no more page ops will be added.
   */
  if (MemoryArea->PageOpCount > 0)
    {
      for (i = 0; i < PAGE_ROUND_UP(MemoryArea->Length) / PAGE_SIZE; i++)
	{
	  PMM_PAGEOP PageOp;

	  if (MemoryArea->PageOpCount == 0)
	    {
	      break;
	    }
	  
	  PageOp = MmCheckForPageOp(MemoryArea, Process->UniqueProcessId,
				    MemoryArea->BaseAddress + (i * PAGE_SIZE),
				    NULL, 0);
	  if (PageOp != NULL)
	    {
	      NTSTATUS Status;
	      MmUnlockAddressSpace(&Process->AddressSpace);
	      Status = KeWaitForSingleObject(&PageOp->CompletionEvent,
					     0,
					     KernelMode,
					     FALSE,
					     NULL);
	      if (Status != STATUS_SUCCESS)
		{
		  DPRINT1("Failed to wait for page op\n");
		  KeBugCheck(0);
		}
	      MmLockAddressSpace(&Process->AddressSpace);
	      MmReleasePageOp(PageOp);
	    }
	}
    }

  /* Free all the individual segments. */
  current_entry = MemoryArea->Data.VirtualMemoryData.RegionListHead.Flink;
  while (current_entry != &MemoryArea->Data.VirtualMemoryData.RegionListHead)
    {
      current = CONTAINING_RECORD(current_entry, MM_REGION, RegionListEntry);
      current_entry = current_entry->Flink;
      ExFreePool(current);
    }

  /* Actually free the memory area. */
  MmFreeMemoryArea(&Process->AddressSpace,
		   MemoryArea->BaseAddress,
		   0,
		   MmFreeVirtualMemoryPage,
		   (PVOID)Process);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL 
NtFreeVirtualMemory(IN	HANDLE	ProcessHandle,
		    IN	PVOID*  PBaseAddress,	
		    IN	PULONG	PRegionSize,	
		    IN	ULONG	FreeType)
/*
 * FUNCTION: Frees a range of virtual memory
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual 
 *                        memory
 *        BaseAddress = Points to the memory address, rounded down to a 
 *                      multiple of the pagesize
 *        RegionSize = Limits the range to free, rounded up to a multiple of 
 *                     the paging size
 *        FreeType = Can be one of the values:  MEM_DECOMMIT, or MEM_RELEASE
 * RETURNS: Status 
 */
{
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   PEPROCESS Process;
   PMADDRESS_SPACE AddressSpace;
   PVOID BaseAddress;
   ULONG RegionSize;
   
   DPRINT("NtFreeVirtualMemory(ProcessHandle %x, *PBaseAddress %x, "
	  "*PRegionSize %x, FreeType %x)\n",ProcessHandle,*PBaseAddress,
	  *PRegionSize,FreeType);
				 
   BaseAddress = (PVOID)PAGE_ROUND_DOWN((*PBaseAddress));
   RegionSize = PAGE_ROUND_UP((*PBaseAddress) + (*PRegionSize)) -
     PAGE_ROUND_DOWN((*PBaseAddress));
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   AddressSpace = &Process->AddressSpace;
   
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  BaseAddress);
   if (MemoryArea == NULL)
     {
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_UNSUCCESSFUL);
     }
   
   switch (FreeType)
     {
      case MEM_RELEASE:
	/* We can only free a memory area in one step. */
	if (MemoryArea->BaseAddress != BaseAddress)
	  {
	     MmUnlockAddressSpace(AddressSpace);
	     ObDereferenceObject(Process);
	     return(STATUS_UNSUCCESSFUL);
	  }	
	MmFreeVirtualMemory(Process, MemoryArea);
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_SUCCESS);
	
      case MEM_DECOMMIT:
	Status = 
	  MmAlterRegion(AddressSpace,
			MemoryArea->BaseAddress,
			&MemoryArea->Data.VirtualMemoryData.RegionListHead,
			BaseAddress,
			RegionSize,
			MEM_RESERVE,
			PAGE_NOACCESS,
			MmModifyAttributes);
	MmUnlockAddressSpace(AddressSpace);	
	ObDereferenceObject(Process);
	return(Status);
     }
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS
MmProtectAnonMem(PMADDRESS_SPACE AddressSpace,
		 PMEMORY_AREA MemoryArea,
		 PVOID BaseAddress,
		 ULONG Length,
		 ULONG Protect,
		 PULONG OldProtect)
{
  PMM_REGION Region;
  NTSTATUS Status;

  Region = MmFindRegion(MemoryArea->BaseAddress, 
			&MemoryArea->Data.VirtualMemoryData.RegionListHead,
			BaseAddress, NULL);
  *OldProtect = Region->Protect;
  Status = MmAlterRegion(AddressSpace, MemoryArea->BaseAddress,
			 &MemoryArea->Data.VirtualMemoryData.RegionListHead,
			 BaseAddress, Length, Region->Type, Protect, 
			 MmModifyAttributes);
  return(Status);
}

NTSTATUS STDCALL
MmQueryAnonMem(PMEMORY_AREA MemoryArea,
	       PVOID Address,
	       PMEMORY_BASIC_INFORMATION Info,
	       PULONG ResultLength)
{
  PMM_REGION Region;
  PVOID RegionBase;

  Info->BaseAddress = (PVOID)PAGE_ROUND_DOWN(Address);

  Region = MmFindRegion(MemoryArea->BaseAddress,
			&MemoryArea->Data.VirtualMemoryData.RegionListHead,
			Address, &RegionBase);
  Info->AllocationBase = RegionBase;
  Info->AllocationProtect = Region->Protect;  /* FIXME */
  Info->RegionSize = RegionBase + Region->Length - Info->BaseAddress;
  Info->State = Region->Type;
  Info->Protect = Region->Protect;
  Info->Type = MEM_PRIVATE;

  *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
  return(STATUS_SUCCESS);
}

/* EOF */
