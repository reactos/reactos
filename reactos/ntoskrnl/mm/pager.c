/* $Id: pager.c,v 1.9 2001/12/31 01:53:45 dwelch Exp $
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

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static HANDLE PagerThreadHandle;
static CLIENT_ID PagerThreadId;
static KEVENT PagerThreadEvent;
static BOOLEAN PagerThreadShouldTerminate;

/* FUNCTIONS *****************************************************************/

static NTSTATUS STDCALL
MmPagerThreadMain(PVOID Ignored)
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
     }
}

NTSTATUS MmInitPagerThread(VOID)
{
   NTSTATUS Status;
   
   PagerThreadShouldTerminate = FALSE;
   KeInitializeEvent(&PagerThreadEvent,
		     SynchronizationEvent,
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
