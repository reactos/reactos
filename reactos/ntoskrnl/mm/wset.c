/* $Id: wset.c,v 1.1 2000/07/04 08:52:45 dwelch Exp $
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

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

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

VOID MmInitializeWorkingSet(PEPROCESS Process,
			    PMADDRESS_SPACE AddressSpace)
{
   AddressSpace->WorkingSetSize = 0;
   AddressSpace->WorkingSetLruFirst = 0;
   AddressSpace->WorkingSetLruLast = 0;
   AddressSpace->WorkingSetPagesAllocated = 1;
   KeInitializeMutex(&Process->WorkingSetLock, 1);
   Process->WorkingSetPage = ExAllocatePage();
}

ULONG MmTrimWorkingSet(PEPROCESS Process,
		       ULONG ReduceHint)
{
   ULONG i;
   PMADDRESS_SPACE AddressSpace;
   PMWORKING_SET WSet;
   ULONG Count;
   
   MmLockWorkingSet(Process);
   
   WSet = (PMWORKING_SET)Process->WorkingSetPage;
   AddressSpace = &Process->AddressSpace;
   
   Count = 0;
   
   for (i = 0; i < AddressSpace->WorkingSetSize; i++)
     {
	PVOID Address;
	PMEMORY_AREA MArea;
	
	Address = WSet->Address[AddressSpace->WorkingSetLruFirst];
	
	MArea = MmOpenMemoryAreaByAddress(AddressSpace, Address);
	
	switch(MArea->Type)
	  {
	   case MEMORY_AREA_SYSTEM:
	     break;
	     
	   case MEMORY_AREA_SECTION_VIEW_COMMIT:
	     Count = Count + MmPageOutSectionView(AddressSpace,
						  MArea,
						  Address);
	     break;
	     
	   case MEMORY_AREA_COMMIT:
	     Count = Count + MmPageOutVirtualMemory(AddressSpace,
						    MArea,
						    Address);
	     break;
	     
	   default:
	     break;
	  }

	
	AddressSpace->WorkingSetLruFirst =
	  ((AddressSpace->WorkingSetLruFirst) + 1) % 1020;
	
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
   PMWORKING_SET WSet;
   ULONG j;
   
   WSet = (PMWORKING_SET)Process->WorkingSetPage;
   AddressSpace = &Process->AddressSpace;
   
   j = AddressSpace->WorkingSetLruFirst;
   for (i = 0; i < AddressSpace->WorkingSetSize; i++)
     {
	if (WSet->Address[j] == Address)
	  {
	     WSet->Address[j] = WSet->Address[AddressSpace->WorkingSetLruLast];
	     if (AddressSpace->WorkingSetLruLast != 0)
	       {
		  AddressSpace->WorkingSetLruLast =
		    AddressSpace->WorkingSetLruLast --;
	       }
	     else
	       {
		  AddressSpace->WorkingSetLruLast = 1020;
	       }
	     return;
	  }
	j = (j + 1) % 1020;
     }
   KeBugCheck(0);
}

BOOLEAN MmAddPageToWorkingSet(PEPROCESS Process,
			      PVOID Address)
{
   PMWORKING_SET WSet;
   PMADDRESS_SPACE AddressSpace;
   
   AddressSpace = &Process->AddressSpace;
   
   if (((AddressSpace->WorkingSetLruLast + 1) % 1020) ==
       AddressSpace->WorkingSetLruFirst)
     {
	return(FALSE);
     }
   
   WSet = (PMWORKING_SET)Process->WorkingSetPage;
   
   WSet->Address[AddressSpace->WorkingSetLruLast] = Address;
   
   AddressSpace->WorkingSetLruLast =
     (AddressSpace->WorkingSetLruLast + 1) % 1024;
   AddressSpace->WorkingSetSize++;
   
   return(TRUE);
}


