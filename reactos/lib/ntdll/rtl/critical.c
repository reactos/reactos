/* $Id: critical.c,v 1.11 2002/09/07 15:12:40 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/critical.c
 * PURPOSE:         Critical sections
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#define NTOS_USER_MODE
#include <ntos.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#define SPIN_COUNT(CriticalSection)((CriticalSection)->Reserved)

VOID STDCALL
RtlDeleteCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
   NtClose(CriticalSection->LockSemaphore);
   SPIN_COUNT(CriticalSection) = -1;
}

VOID STDCALL
RtlEnterCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
   HANDLE Thread = (HANDLE)NtCurrentTeb()->Cid.UniqueThread;
   ULONG ret;
 
   if (InterlockedIncrement(&CriticalSection->LockCount))
     {
	NTSTATUS Status;
	
	if (CriticalSection->OwningThread == Thread)
	  {
	     CriticalSection->RecursionCount++;
	     return;
	  }
	
//	DbgPrint("Entering wait for critical section\n");
	Status = NtWaitForSingleObject(CriticalSection->LockSemaphore, 
				       0, FALSE);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("RtlEnterCriticalSection: Failed to wait (Status %x)\n", 
		      Status);
	  }
//	DbgPrint("Left wait for critical section\n");
     }
   CriticalSection->OwningThread = Thread;
   CriticalSection->RecursionCount = 1;
   
#if 0
   if ((ret = InterlockedIncrement(&(CriticalSection->LockCount) )) != 1)
     {
	if (CriticalSection->OwningThread != Thread)
	  {
	     NtWaitForSingleObject(CriticalSection->LockSemaphore, 
				   0, 
				   FALSE);
	     CriticalSection->OwningThread = Thread;
	  }
     }
   else
     {
	CriticalSection->OwningThread = Thread;
     }
   
   CriticalSection->RecursionCount++;
#endif
}

NTSTATUS STDCALL
RtlInitializeCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
   NTSTATUS Status;
   
   CriticalSection->LockCount = -1;
   CriticalSection->RecursionCount = 0;
   CriticalSection->OwningThread = (HANDLE)0;
   SPIN_COUNT(CriticalSection) = 0;
   
   Status = NtCreateSemaphore(&CriticalSection->LockSemaphore,
			      SEMAPHORE_ALL_ACCESS,
			      NULL,
			      0,
			      1);
   return Status;
}

VOID STDCALL
RtlLeaveCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
   HANDLE Thread = (HANDLE)NtCurrentTeb()->Cid.UniqueThread;
   
   if (CriticalSection->OwningThread != Thread)
     {
	DbgPrint("Freeing critical section not owned\n");
	return;
     }
   
   CriticalSection->RecursionCount--;
   if (CriticalSection->RecursionCount > 0)
     {
	InterlockedDecrement(&CriticalSection->LockCount);
	return;
     }
   CriticalSection->OwningThread = 0;
   if (InterlockedIncrement(&CriticalSection->LockCount) >= 0)
     {
	NTSTATUS Status;
	
	Status = NtReleaseSemaphore(CriticalSection->LockSemaphore, 1, NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Failed to release semaphore (Status %x)\n",
		      Status);
	  }
     }
   
#if 0
   CriticalSection->RecursionCount--;
   if (CriticalSection->RecursionCount == 0)
     {
	CriticalSection->OwningThread = (HANDLE)-1;
	// if LockCount > 0 and RecursionCount == 0 there
	// is a waiting thread 
	// ReleaseSemaphore will fire up a waiting thread
	if (InterlockedDecrement(&CriticalSection->LockCount) > 0)
	  {
	     NtReleaseSemaphore(CriticalSection->LockSemaphore,1,NULL);
	  }
     }
   else
     {
	InterlockedDecrement(&CriticalSection->LockCount);
     }
#endif
}

BOOLEAN STDCALL
RtlTryEnterCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
   if (InterlockedCompareExchange((PLONG)&CriticalSection->LockCount, 
				  1, 0 ) == 0)
     {
	CriticalSection->OwningThread = 
	  (HANDLE) NtCurrentTeb()->Cid.UniqueThread;
	CriticalSection->RecursionCount++;
	return TRUE;
     }
   if (CriticalSection->OwningThread == 
       (HANDLE)NtCurrentTeb()->Cid.UniqueThread)
     {
	CriticalSection->RecursionCount++;
	return TRUE;
     }
   return FALSE;
}


/* EOF */





