/* $Id: critical.c,v 1.20 2004/03/24 23:43:52 gdalsnes Exp $
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

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
RtlDeleteCriticalSection(PCRITICAL_SECTION CriticalSection)
{
   NtClose(CriticalSection->LockSemaphore);
   CriticalSection->LockCount = -1;
}

/*
 * @implemented
 */
DWORD STDCALL
RtlSetCriticalSectionSpinCount(
   LPCRITICAL_SECTION CriticalSection,
   DWORD SpinCount
   )
{
   
#ifdef MP
   DWORD PrevSpinCount = CriticalSection->SpinCount; 
   CriticalSection->SpinCount = SpinCount;
   return PrevSpinCount;
#else
   return 0;
#endif

}


/*
 * @implemented
 */
VOID STDCALL
RtlEnterCriticalSection(PCRITICAL_SECTION CriticalSection)
{
   HANDLE Thread = (HANDLE)NtCurrentTeb()->Cid.UniqueThread;
 
   if (InterlockedIncrement(&CriticalSection->LockCount))
     {
	NTSTATUS Status;
	
	if (CriticalSection->OwningThread == Thread)
	  {
	     CriticalSection->RecursionCount++;
	     return;
	  }
	
	DPRINT("Entering wait for critical section\n");
	Status = NtWaitForSingleObject(CriticalSection->LockSemaphore, 
				       0, FALSE);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT1("RtlEnterCriticalSection: Failed to wait (Status %x)\n", 
		     Status);
	  }
	DPRINT("Left wait for critical section\n");
     }
   CriticalSection->OwningThread = Thread;
   CriticalSection->RecursionCount = 1;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlInitializeCriticalSection(PCRITICAL_SECTION CriticalSection)
{
  return RtlInitializeCriticalSectionAndSpinCount (CriticalSection,
						   0);
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
	DPRINT1("Freeing critical section not owned\n");
     }
   
   CriticalSection->RecursionCount--;
   if (CriticalSection->RecursionCount > 0)
     {
	InterlockedDecrement(&CriticalSection->LockCount);
	return;
     }
   CriticalSection->OwningThread = 0;
   if (InterlockedDecrement(&CriticalSection->LockCount) >= 0)
     {
	NTSTATUS Status;
	
	Status = NtReleaseSemaphore(CriticalSection->LockSemaphore, 1, NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT1("Failed to release semaphore (Status %x)\n",
		     Status);
	  }
     }
}

/*
 * @implemented
 */
BOOLEAN STDCALL
RtlTryEnterCriticalSection(PCRITICAL_SECTION CriticalSection)
{
   if (InterlockedCompareExchange((PVOID*)&CriticalSection->LockCount,
				  (PVOID)0, (PVOID)-1 ) == (PVOID)-1)
     {
	CriticalSection->OwningThread = 
	  (HANDLE) NtCurrentTeb()->Cid.UniqueThread;
	CriticalSection->RecursionCount = 1;
	return TRUE;
     }
   if (CriticalSection->OwningThread == 
       (HANDLE)NtCurrentTeb()->Cid.UniqueThread)
     {
        InterlockedIncrement(&CriticalSection->LockCount);
	CriticalSection->RecursionCount++;
	return TRUE;
     }
   return FALSE;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlInitializeCriticalSectionAndSpinCount (PCRITICAL_SECTION CriticalSection,
					  ULONG SpinCount)
{
  CriticalSection->LockCount = -1;
  CriticalSection->RecursionCount = 0;
  CriticalSection->OwningThread = (HANDLE)0;
#ifdef MP  
  CriticalSection->SpinCount = SpinCount;
#else
  CriticalSection->SpinCount = 0;
#endif

  return NtCreateSemaphore (&CriticalSection->LockSemaphore,
			    SEMAPHORE_ALL_ACCESS,
			    NULL,
			    0,
			    1);
}

/* EOF */
