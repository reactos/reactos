/* $Id: mpw.c,v 1.7 2001/12/31 01:53:45 dwelch Exp $
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
static volatile BOOLEAN MpwThreadShouldTerminate;

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
MmMpwThreadMain(PVOID Ignored)
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
     }
}

NTSTATUS MmInitMpwThread(VOID)
{
   NTSTATUS Status;
   
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
