/* $Id: pager.c,v 1.1 2000/06/25 03:59:15 dwelch Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/pager.c
 * PURPOSE:      Moves infrequently used data out of memory
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY: 
 *               27/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static HANDLE PagerThreadHandle;
static CLIENT_ID PagerThreadId;
static KEVENT PagerThreadEvent;
static PVOID LastAddress;
static PEPROCESS LastProcess;
static BOOLEAN PagerThreadShouldTerminate;
static volatile ULONG PageCount;

/* FUNCTIONS *****************************************************************/

VOID MmPageOutPage(PEPROCESS Process,
		   PMEMORY_AREA marea,
		   PVOID Address)
{
   ULONG Count;
   
   Count = 0;
   
   switch(marea->Type)
     {
      case MEMORY_AREA_SYSTEM:
	break;
	
      case MEMORY_AREA_SECTION_VIEW_COMMIT:
	Count = MmPageOutSectionView(&Process->Pcb.AddressSpace,
				     marea,
				     Address);
	break;
	
      case MEMORY_AREA_COMMIT:
	Count = MmPageOutVirtualMemory(&Process->Pcb.AddressSpace,
				       marea,
				       Address);
	break;
	
      default:
	break;
     }
   PageCount = PageCount - Count;
}

VOID MmTryPageOutFromArea(PEPROCESS Process,				  
			  PMEMORY_AREA marea)
{
   ULONG i;
   ULONG j;
   
   for (i = 0; i < marea->Length; i = i + 0x400000)
     {
	if (MmIsPageTablePresent(marea->BaseAddress + i))
	  {
	     for (j = 0; j < marea->Length; j = j + 4096)
	       {
		  if (MmIsPagePresent(NULL, marea->BaseAddress + i + j))
		    {
		       MmPageOutPage(Process, 
				     marea,
				     marea->BaseAddress + i + j);
		       if (PageCount == 0)
			 {
			    return;
			 }
		    }
	       }
	  }
     }
}

VOID MmTryPageOutFromProcess(PEPROCESS Process)
{
   PMEMORY_AREA marea;
   
   MmLockAddressSpace(&Process->Pcb.AddressSpace);
   while ((ULONG)LastAddress < 0xc0000000)
     {
	marea = MmOpenMemoryAreaByRegion(&Process->Pcb.AddressSpace,
					 LastAddress,
					 0xc0000000 - (ULONG)LastAddress);
	if (marea == NULL)
	  {
	     return;
	  }
	MmTryPageOutFromArea(Process,
			     marea);
	LastAddress = LastAddress + marea->Length;
	if (PageCount == 0)
	  {
	     MmUnlockAddressSpace(&Process->Pcb.AddressSpace);
	     return;
	  }
     }
   MmUnlockAddressSpace(&Process->Pcb.AddressSpace);
}

NTSTATUS MmPagerThreadMain(PVOID Ignored)
{
   NTSTATUS Status;
   
   PageCount = 0;
   LastAddress = 0;
   LastProcess = PsInitialSystemProcess;
   PagerThreadShouldTerminate = FALSE;
   KeInitializeEvent(&PagerThreadEvent,
		     SynchronizationEvent,
		     FALSE);
   
   for(;;)
     {
	Status = KeWaitForSingleObject(&PagerThreadEvent,
				       0,
				       KernelMode,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("PagerThread: Wait failed\n");
	     KeBugCheck(0);
	  }
	if (PagerThreadShouldTerminate)
	  {
	     DbgPrint("PagerThread: Terminating\n");
	     return(STATUS_SUCCESS);
	  }
	
	while (PageCount > 0)
	  {
	     KeAttachProcess(LastProcess);
	     MmTryPageOutFromProcess(LastProcess);
	     KeDetachProcess();
	     if (PageCount != 0)
	       {
		  LastProcess = PsGetNextProcess(LastProcess);
		  LastAddress = 0;
	       }
	  }
     }
}

NTSTATUS MmInitPager(VOID)
{
   NTSTATUS Status;
   
   Status = PsCreateSystemThread(&PagerThreadHandle,
				 THREAD_ALL_ACCESS,
				 NULL,
				 NULL,
				 &PagerThreadId,
				 MmPagerThreadMain,
				 NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   return(STATUS_SUCCESS);
}
