/* $Id: pager.c,v 1.13 2003/07/13 14:36:32 dwelch Exp $
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
static ULONG PagerThreadWorkCount;

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

  WasWorking = InterlockedIncrement(&PagerThreadWorkCount);
  if (WasWorking == 1)
    {
      KeSetEvent(&PagerThreadEvent, IO_NO_INCREMENT, FALSE);
    }
}

VOID
MiStopPagerThread(VOID)
{
  (VOID)InterlockedDecrement(&PagerThreadWorkCount);
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
       do
	 {
	   /* Try and make some memory available to the system. */
	   MmRebalanceMemoryConsumers();
	 } while(PagerThreadWorkCount > 0);
     }
}

NTSTATUS MmInitPagerThread(VOID)
{
   NTSTATUS Status;
   
   PagerThreadShouldTerminate = FALSE;
   PagerThreadWorkCount = 0;
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
