/* $Id: wset.c,v 1.7 2001/02/06 00:11:19 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
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

PVOID MmGetDirtyPagesFromWorkingSet(struct _EPROCESS* Process)
{
   return(NULL);
}

VOID MmLockWorkingSet(PEPROCESS Process)
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
   * of the size of physical memory and the size of the user address space
   */
  MaximumLength = MmStats.NrTotalPages - MmStats.NrReservedPages;
  MaximumLength = min(MaximumLength, KERNEL_BASE / PAGESIZE);
  MaximumLength = PAGE_ROUND_UP(MaximumLength * sizeof(ULONG));
  
  FirstPage = MmAllocPageMaybeSwap(0);
  if (FirstPage == NULL)
    {
      KeBugCheck(0);
    }
  
  BaseAddress = 0;
  Status = MmCreateMemoryArea(NULL,
			      MmGetKernelAddressSpace(),
			      MEMORY_AREA_WORKING_SET,
			      &BaseAddress,
			      MaximumLength,
			      0,
			      &AddressSpace->WorkingSetArea);
  if (!NT_SUCCESS(Status))
    {
      KeBugCheck(0);
    }
  AddressSpace->WorkingSetSize = 0;
  AddressSpace->WorkingSetLruFirst = 0;
  AddressSpace->WorkingSetLruLast = 0;
  AddressSpace->WorkingSetMaximumLength = MaximumLength;
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

ULONG MmPageOutPage(PMADDRESS_SPACE AddressSpace,
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

ULONG MmTrimWorkingSet(PEPROCESS Process,
		       ULONG ReduceHint)
{
   ULONG i, j;
   PMADDRESS_SPACE AddressSpace;
   PVOID* WSet;
   ULONG Count;
   BOOLEAN Ul;
   
   MmLockWorkingSet(Process);
   
   WSet = (PVOID*)Process->WorkingSetPage;
   AddressSpace = &Process->AddressSpace;
   
   Count = 0;
   j = AddressSpace->WorkingSetLruFirst;
   
   for (i = 0; i < AddressSpace->WorkingSetSize; )
     {
	PVOID Address;
	PMEMORY_AREA MArea;
	
	Address = WSet[j];
		
	MArea = MmOpenMemoryAreaByAddress(AddressSpace, Address);
	
	if (MArea == NULL)
	  {
	     KeBugCheck(0);
	  }
	
	Count = Count + MmPageOutPage(AddressSpace, MArea, Address, &Ul);
	
	if (Ul)
	  {
	     MmLockWorkingSet(Process);
	     
	     j = AddressSpace->WorkingSetLruFirst;
	     i = 0;
	  }
	else
	  {
	     j = (j + 1) % AddressSpace->WorkingSetMaximumLength;
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

VOID MmRemovePageFromWorkingSet(PEPROCESS Process,
				PVOID Address)
{
   ULONG i;
   PMADDRESS_SPACE AddressSpace;
   PVOID* WSet;
   ULONG j;
   
   WSet = (PVOID*)Process->WorkingSetPage;
   AddressSpace = &Process->AddressSpace;
   
   j = AddressSpace->WorkingSetLruFirst;
   for (i = 0; i < AddressSpace->WorkingSetSize; i++)
     {
	if (WSet[j] == Address)
	  {
	     WSet[j] = WSet[AddressSpace->WorkingSetLruLast - 1];
	     if (AddressSpace->WorkingSetLruLast != 0)
	       {
		 AddressSpace->WorkingSetLruLast--;
	       }
	     else
	       {
		  AddressSpace->WorkingSetLruLast = 
		    AddressSpace->WorkingSetMaximumLength;
	       }
	     return;
	  }
	j = (j + 1) % AddressSpace->WorkingSetMaximumLength;
     }
   KeBugCheck(0);
}

VOID
MmAddPageToWorkingSet(PEPROCESS Process,
		      PVOID Address)
{
   PVOID* WSet;
   PMADDRESS_SPACE AddressSpace;
   BOOLEAN Present;
   ULONG Current;
   PVOID NextAddress;

   AddressSpace = &Process->AddressSpace;
   
   /*
    * This can't happen unless there is a bug 
    */
   if (AddressSpace->WorkingSetSize == AddressSpace->WorkingSetMaximumLength)
     {
       KeBugCheck(0);
     }
   
   WSet = (PVOID*)Process->WorkingSetPage;
   
   Current = AddressSpace->WorkingSetLruLast;
   
   /*
    * If we are growing the working set then check to see if we need
    * to allocate a page
    */
   NextAddress = (PVOID)PAGE_ROUND_DOWN((PVOID)&WSet[Current]);
   Present = MmIsPagePresent(NULL, NextAddress);
   if (!Present)
     {
       PVOID Page;
       NTSTATUS Status;

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
   WSet[Current] = Address;  

   AddressSpace->WorkingSetLruLast = 
     (Current + 1) % AddressSpace->WorkingSetMaximumLength;
   AddressSpace->WorkingSetSize++;
}



