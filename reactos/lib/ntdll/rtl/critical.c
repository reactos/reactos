/* $Id: critical.c,v 1.13 2003/07/11 13:50:23 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/critical.c
 * PURPOSE:         Critical sections
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntos/synch.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
RtlDeleteCriticalSection(PCRITICAL_SECTION CriticalSection)
{
   NtClose(CriticalSection->LockSemaphore);
   CriticalSection->Reserved = -1;
}

/*
 * @implemented
 */
VOID STDCALL
RtlEnterCriticalSection(PCRITICAL_SECTION CriticalSection)
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

/*
 * @implemented
 */
NTSTATUS STDCALL
RtlInitializeCriticalSection(PCRITICAL_SECTION CriticalSection)
{
   NTSTATUS Status;
   
   CriticalSection->LockCount = -1;
   CriticalSection->RecursionCount = 0;
   CriticalSection->OwningThread = (HANDLE)0;
   CriticalSection->Reserved = 0;
   
   Status = NtCreateSemaphore(&CriticalSection->LockSemaphore,
			      SEMAPHORE_ALL_ACCESS,
			      NULL,
			      0,
			      1);
   return Status;
}

/*
 * @implemented
 */
VOID STDCALL
RtlLeaveCriticalSection(PCRITICAL_SECTION CriticalSection)
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

/*
 * @implemented
 */
BOOLEAN STDCALL
RtlTryEnterCriticalSection(PCRITICAL_SECTION CriticalSection)
{
   if (InterlockedCompareExchange((PVOID*)&CriticalSection->LockCount, 
				  (PVOID)1, (PVOID)0 ) == 0)
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
