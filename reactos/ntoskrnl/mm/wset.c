/* $Id: wset.c,v 1.4 2000/07/08 16:53:33 dwelch Exp $
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

VOID MmInitializeWorkingSet(PEPROCESS Process,
			    PMADDRESS_SPACE AddressSpace)
{
   AddressSpace->WorkingSetSize = 0;
   AddressSpace->WorkingSetLruFirst = 0;
   AddressSpace->WorkingSetLruLast = 0;
   AddressSpace->WorkingSetPagesAllocated = 1;
   KeInitializeMutex(&Process->WorkingSetLock, 1);
   Process->WorkingSetPage = ExAllocatePage();
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
		  
      case MEMORY_AREA_COMMIT:
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
   PMWORKING_SET WSet;
   ULONG Count;
   BOOLEAN Ul;
   
   MmLockWorkingSet(Process);
   
   WSet = (PMWORKING_SET)Process->WorkingSetPage;
   AddressSpace = &Process->AddressSpace;
   
   Count = 0;
   j = AddressSpace->WorkingSetLruFirst;
   
   for (i = 0; i < AddressSpace->WorkingSetSize; )
     {
	PVOID Address;
	PMEMORY_AREA MArea;
	
	Address = WSet->Address[j];
		
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
	     j = (j + 1) % WSET_ADDRESSES_IN_PAGE;
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
   PMWORKING_SET WSet;
   ULONG j;
   
   WSet = (PMWORKING_SET)Process->WorkingSetPage;
   AddressSpace = &Process->AddressSpace;
   
   j = AddressSpace->WorkingSetLruFirst;
   for (i = 0; i < AddressSpace->WorkingSetSize; i++)
     {
	if (WSet->Address[j] == Address)
	  {
	     WSet->Address[j] = 
	       WSet->Address[AddressSpace->WorkingSetLruLast - 1];
	     if (AddressSpace->WorkingSetLruLast != 0)
	       {
		  AddressSpace->WorkingSetLruLast =
		    AddressSpace->WorkingSetLruLast --;
	       }
	     else
	       {
		  AddressSpace->WorkingSetLruLast = WSET_ADDRESSES_IN_PAGE;
	       }
	     return;
	  }
	j = (j + 1) % WSET_ADDRESSES_IN_PAGE;
     }
   KeBugCheck(0);
}

BOOLEAN MmAddPageToWorkingSet(PEPROCESS Process,
			      PVOID Address)
{
   PMWORKING_SET WSet;
   PMADDRESS_SPACE AddressSpace;
   
   AddressSpace = &Process->AddressSpace;
   
   if (((AddressSpace->WorkingSetLruLast + 1) % WSET_ADDRESSES_IN_PAGE) ==
       AddressSpace->WorkingSetLruFirst)
     {
	return(FALSE);
     }
   
   WSet = (PMWORKING_SET)Process->WorkingSetPage;
   
   WSet->Address[AddressSpace->WorkingSetLruLast] = Address;
   
   AddressSpace->WorkingSetLruLast =
     (AddressSpace->WorkingSetLruLast + 1) % WSET_ADDRESSES_IN_PAGE;
   AddressSpace->WorkingSetSize++;
   
   return(TRUE);
}


