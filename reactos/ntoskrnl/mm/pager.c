/* $Id: pager.c,v 1.3 2000/07/06 14:34:51 dwelch Exp $
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
#include <internal/ke.h>
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
static PEPROCESS LastProcess;
static volatile BOOLEAN PagerThreadShouldTerminate;
static volatile ULONG PageCount;
static volatile ULONG WaiterCount;
static KEVENT FreedMemEvent;

/* FUNCTIONS *****************************************************************/

VOID MmWaitForFreePages(VOID)
{
   InterlockedIncrement((PULONG)&PageCount);
   KeClearEvent(&FreedMemEvent);
   KeSetEvent(&PagerThreadEvent,
	      IO_NO_INCREMENT,
	      FALSE);
   InterlockedIncrement((PULONG)&WaiterCount);
   KeWaitForSingleObject(&FreedMemEvent,
			 0,
			 KernelMode,
			 FALSE,
			 NULL);
   InterlockedDecrement((PULONG)&WaiterCount);
}

static VOID MmTryPageOutFromProcess(PEPROCESS Process)
{
   ULONG P;
   
   MmLockAddressSpace(&Process->AddressSpace);
   P = MmTrimWorkingSet(Process, PageCount);
   if (P > 0)
     {
	InterlockedExchangeAdd((PULONG)&PageCount, -P);
	KeSetEvent(&FreedMemEvent, IO_NO_INCREMENT, FALSE);
     }
   MmUnlockAddressSpace(&Process->AddressSpace);
}

static NTSTATUS MmPagerThreadMain(PVOID Ignored)
{
   NTSTATUS Status;
      
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
	
	while (WaiterCount > 0)
	  {
	     while (PageCount > 0)
	       {
		  KeAttachProcess(LastProcess);
		  MmTryPageOutFromProcess(LastProcess);
		  KeDetachProcess();
		  if (PageCount != 0)
		    {
		       LastProcess = PsGetNextProcess(LastProcess);
		    }
	       }
	     KeSetEvent(&FreedMemEvent, IO_NO_INCREMENT, FALSE);
	  }
     }
}

NTSTATUS MmInitPagerThread(VOID)
{
   NTSTATUS Status;
   
   PageCount = 0;
   WaiterCount = 0;
   LastProcess = PsInitialSystemProcess;
   PagerThreadShouldTerminate = FALSE;
   KeInitializeEvent(&PagerThreadEvent,
		     SynchronizationEvent,
		     FALSE);
   KeInitializeEvent(&FreedMemEvent,
		     NotificationEvent,
		     FALSE);
   
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
