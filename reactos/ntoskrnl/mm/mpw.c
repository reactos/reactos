/* $Id: mpw.c,v 1.5 2001/03/16 18:11:23 dwelch Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/mpw.c
 * PURPOSE:      Writes data that has been modified in memory but not on
 *               the disk
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY: 
 *               27/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <internal/mm.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static HANDLE MpwThreadHandle;
static CLIENT_ID MpwThreadId;
static KEVENT MpwThreadEvent;
static PEPROCESS LastProcess;
static volatile BOOLEAN MpwThreadShouldTerminate;
static ULONG CountToWrite;

/* FUNCTIONS *****************************************************************/

VOID MmStartWritingPages(VOID)
{
   CountToWrite = CountToWrite + MmStats.NrDirtyPages;
}

ULONG MmWritePage(PMADDRESS_SPACE AddressSpace,
		  PVOID Address)
{
   PMEMORY_AREA MArea;
   NTSTATUS Status;
   
   MArea = MmOpenMemoryAreaByAddress(AddressSpace, Address);
   
   switch(MArea->Type)
     {
      case MEMORY_AREA_SYSTEM:
	return(STATUS_UNSUCCESSFUL);
	     
      case MEMORY_AREA_SECTION_VIEW_COMMIT:
	Status = MmWritePageSectionView(AddressSpace,
					MArea,
					Address);
	return(Status);
		  
      case MEMORY_AREA_VIRTUAL_MEMORY:
	Status = MmWritePageVirtualMemory(AddressSpace,
					  MArea,
					  Address);
	return(Status);
	
     }
   return(STATUS_UNSUCCESSFUL);
}

VOID MmWritePagesInProcess(PEPROCESS Process)
{
   PVOID Address;
   NTSTATUS Status;
   
   MmLockAddressSpace(&Process->AddressSpace);
   
   while ((Address = MmGetDirtyPagesFromWorkingSet(Process)) != NULL)
     {
	Status = MmWritePage(&Process->AddressSpace, Address);
	if (NT_SUCCESS(Status))
	  {
	     CountToWrite = CountToWrite - 1;
	     if (CountToWrite == 0)
	       {
		  MmUnlockAddressSpace(&Process->AddressSpace);
		  return;
	       }	     
	  }
     }
   
   MmUnlockAddressSpace(&Process->AddressSpace);
}

NTSTATUS MmMpwThreadMain(PVOID Ignored)
{
   NTSTATUS Status;
      
   for(;;)
     {
	Status = KeWaitForSingleObject(&MpwThreadEvent,
				       0,
				       KernelMode,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("MpwThread: Wait failed\n");
	     KeBugCheck(0);
	     return(STATUS_UNSUCCESSFUL);
	  }
	if (MpwThreadShouldTerminate)
	  {
	     DbgPrint("MpwThread: Terminating\n");
	     return(STATUS_SUCCESS);
	  }
	
	do
	  {
	     KeAttachProcess(LastProcess);
	     MmWritePagesInProcess(LastProcess);
	     KeDetachProcess();
	     if (CountToWrite != 0)
	       {
		  LastProcess = PsGetNextProcess(LastProcess);
	       }
	  } while (CountToWrite > 0 &&
		   LastProcess != PsInitialSystemProcess);
     }
}

NTSTATUS MmInitMpwThread(VOID)
{
   NTSTATUS Status;
   
   CountToWrite = 0;
   LastProcess = PsInitialSystemProcess;
   MpwThreadShouldTerminate = FALSE;
   KeInitializeEvent(&MpwThreadEvent,
		     SynchronizationEvent,
		     FALSE);
   
   Status = PsCreateSystemThread(&MpwThreadHandle,
				 THREAD_ALL_ACCESS,
				 NULL,
				 NULL,
				 &MpwThreadId,
				 MmMpwThreadMain,
				 NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   return(STATUS_SUCCESS);
}
