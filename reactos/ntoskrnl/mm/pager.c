/* $Id: pager.c,v 1.12 2003/07/12 01:52:10 dwelch Exp $
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
static ULONG PagerThreadWorking;

/* FUNCTIONS *****************************************************************/

BOOLEAN
MiIsPagerThread(VOID)
{
  return(PsGetCurrentThreadId() == PagerThreadId.UniqueThread);
}

VOID
MiStartPagerThread(VOID)
{
  ULONG WasWorking;

  WasWorking = InterlockedExchange(&PagerThreadWorking, 1);
  if (WasWorking == 0)
    {
      KeSetEvent(&PagerThreadEvent, IO_NO_INCREMENT, FALSE);
    }
}

static NTSTATUS STDCALL
MmPagerThreadMain(PVOID Ignored)
{
   NTSTATUS Status;

   for(;;)
     {
       /* Wake for a low memory situation or a terminate request. */
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
       /* Try and make some memory available to the system. */
       MmRebalanceMemoryConsumers();
       /* Let the rest of the system know we finished this run. */
       (VOID)InterlockedExchange(&PagerThreadWorking, 0);
     }
}

NTSTATUS MmInitPagerThread(VOID)
{
   NTSTATUS Status;
   
   PagerThreadShouldTerminate = FALSE;
   PagerThreadWorking = 0;
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
