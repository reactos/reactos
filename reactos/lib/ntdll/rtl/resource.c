/* $Id: resource.c,v 1.2 2002/09/07 15:12:40 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/resource.c
 * PURPOSE:         Resource (multiple-reader-single-writer lock) functions
 * PROGRAMMER:      
 * UPDATE HISTORY:
 *                  Created 24/05/2001
 *
 * NOTES:           Partially take from Wine:
 *                  Copyright 1996-1998 Marcus Meissner
 *                            1999 Alex Korobka
 */

/*
 * xxxResource() functions implement multiple-reader-single-writer lock.
 * The code is based on information published in WDJ January 1999 issue.
 */

#define NTOS_USER_MODE
#include <ntos.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

VOID STDCALL
RtlInitializeResource(PRTL_RESOURCE Resource)
{
   NTSTATUS Status;
   
   Status = RtlInitializeCriticalSection(&Resource->Lock);
   if (!NT_SUCCESS(Status))
     {
	RtlRaiseStatus(Status);
     }
   
   Status = NtCreateSemaphore(&Resource->SharedSemaphore,
			      SEMAPHORE_ALL_ACCESS,
			      NULL,
			      0,
			      65535);
   if (!NT_SUCCESS(Status))
     {
	RtlRaiseStatus(Status);
     }
   Resource->SharedWaiters = 0;
   
   Status = NtCreateSemaphore(&Resource->ExclusiveSemaphore,
			      SEMAPHORE_ALL_ACCESS,
			      NULL,
			      0,
			      65535);
   if (!NT_SUCCESS(Status))
     {
	RtlRaiseStatus(Status);
     }
   Resource->ExclusiveWaiters = 0;
   
   Resource->NumberActive = 0;
   Resource->OwningThread = NULL;
   Resource->TimeoutBoost = 0; /* no info on this one, default value is 0 */
}


VOID STDCALL
RtlDeleteResource(PRTL_RESOURCE Resource)
{
   RtlDeleteCriticalSection(&Resource->Lock);
   NtClose(Resource->ExclusiveSemaphore);
   NtClose(Resource->SharedSemaphore);
   Resource->OwningThread = NULL;
   Resource->ExclusiveWaiters = 0;
   Resource->SharedWaiters = 0;
   Resource->NumberActive = 0;
}


BOOLEAN STDCALL
RtlAcquireResourceExclusive(PRTL_RESOURCE Resource,
			    BOOLEAN Wait)
{
   NTSTATUS Status;
   BOOLEAN retVal = FALSE;

start:
    RtlEnterCriticalSection(&Resource->Lock);
    if (Resource->NumberActive == 0) /* lock is free */
      {
	Resource->NumberActive = -1;
	retVal = TRUE;
      }
    else if (Resource->NumberActive < 0) /* exclusive lock in progress */
      {
	if (Resource->OwningThread == NtCurrentTeb()->Cid.UniqueThread)
	  {
	     retVal = TRUE;
	     Resource->NumberActive--;
	     goto done;
	  }
wait:
	if (Wait == TRUE)
	  {
	     Resource->ExclusiveWaiters++;

	     RtlLeaveCriticalSection(&Resource->Lock);
	     Status = NtWaitForSingleObject(Resource->ExclusiveSemaphore,
					    FALSE,
					    NULL);
	     if (!NT_SUCCESS(Status))
	       goto done;
	     goto start; /* restart the acquisition to avoid deadlocks */
	  }
     }
   else  /* one or more shared locks are in progress */
     {
	if (Wait == TRUE)
	  goto wait;
     }
   if (retVal == TRUE)
     Resource->OwningThread = NtCurrentTeb()->Cid.UniqueThread;
done:
    RtlLeaveCriticalSection(&Resource->Lock);
    return retVal;
}


BOOLEAN STDCALL
RtlAcquireResourceShared(PRTL_RESOURCE Resource,
			 BOOLEAN Wait)
{
   NTSTATUS Status = STATUS_UNSUCCESSFUL;
   BOOLEAN retVal = FALSE;

start:
   RtlEnterCriticalSection(&Resource->Lock);
   if (Resource->NumberActive < 0)
     {
	if (Resource->OwningThread == NtCurrentTeb()->Cid.UniqueThread)
	  {
	     Resource->NumberActive--;
	     retVal = TRUE;
	     goto done;
	  }
	
	if (Wait == TRUE)
	  {
	     Resource->SharedWaiters++;
	     RtlLeaveCriticalSection(&Resource->Lock);
	     Status = NtWaitForSingleObject(Resource->SharedSemaphore,
					    FALSE,
					    NULL);
	     if (!NT_SUCCESS(Status))
	       goto done;
	     goto start;
	  }
     }
   else
     {
	if (Status != STATUS_WAIT_0) /* otherwise RtlReleaseResource() has already done it */
	  Resource->NumberActive++;
	retVal = TRUE;
     }
done:
   RtlLeaveCriticalSection(&Resource->Lock);
   return retVal;
}


VOID STDCALL
RtlConvertExclusiveToShared(PRTL_RESOURCE Resource)
{
   RtlEnterCriticalSection(&Resource->Lock);
   
   if (Resource->NumberActive == -1)
     {
	Resource->OwningThread = NULL;
   
	if (Resource->SharedWaiters > 0)
	  {
	     ULONG n;
	     /* prevent new writers from joining until
	      * all queued readers have done their thing */
	     n = Resource->SharedWaiters;
	     Resource->NumberActive = Resource->SharedWaiters + 1;
	     Resource->SharedWaiters = 0;
	     NtReleaseSemaphore(Resource->SharedSemaphore,
				n,
				NULL);
	  }
	else
	  {
	     Resource->NumberActive = 1;
	  }
     }
   
   RtlLeaveCriticalSection(&Resource->Lock);
}


VOID STDCALL
RtlConvertSharedToExclusive(PRTL_RESOURCE Resource)
{
   NTSTATUS Status;
   
   RtlEnterCriticalSection(&Resource->Lock);
   
   if (Resource->NumberActive == 1)
     {
	Resource->OwningThread = NtCurrentTeb()->Cid.UniqueThread;
	Resource->NumberActive = -1;
     }
   else
     {
	Resource->ExclusiveWaiters++;
   
	RtlLeaveCriticalSection(&Resource->Lock);
	Status = NtWaitForSingleObject(Resource->ExclusiveSemaphore,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	   return;
   
	RtlEnterCriticalSection(&Resource->Lock);
	Resource->OwningThread = NtCurrentTeb()->Cid.UniqueThread;
	Resource->NumberActive = -1;
     }
   RtlLeaveCriticalSection(&Resource->Lock);
}


VOID STDCALL
RtlReleaseResource(PRTL_RESOURCE Resource)
{
   RtlEnterCriticalSection(&Resource->Lock);

   if (Resource->NumberActive > 0) /* have one or more readers */
     {
	Resource->NumberActive--;
	if (Resource->NumberActive == 0)
	  {
	     if (Resource->ExclusiveWaiters > 0)
	       {
wake_exclusive:
		  Resource->ExclusiveWaiters--;
		  NtReleaseSemaphore(Resource->ExclusiveSemaphore,
				     1,
				     NULL);
	       }
	  }
     }
   else if (Resource->NumberActive < 0) /* have a writer, possibly recursive */
     {
	Resource->NumberActive++;
	if (Resource->NumberActive == 0)
	  {
	     Resource->OwningThread = 0;
	     if (Resource->ExclusiveWaiters > 0)
	       {
		  goto wake_exclusive;
	       }
	     else
	       {
		  if (Resource->SharedWaiters > 0)
		    {
		       ULONG n;
		       /* prevent new writers from joining until
		        * all queued readers have done their thing */
		       n = Resource->SharedWaiters;
		       Resource->NumberActive = Resource->SharedWaiters;
		       Resource->SharedWaiters = 0;
		       NtReleaseSemaphore(Resource->SharedSemaphore,
					  n,
					  NULL);
		    }
	       }
	  }
     }
   RtlLeaveCriticalSection(&Resource->Lock);
}


VOID STDCALL
RtlDumpResource(PRTL_RESOURCE Resource)
{
   DbgPrint("RtlDumpResource(%p):\n\tactive count = %i\n\twaiting readers = %i\n\twaiting writers = %i\n",
	    Resource,
	    Resource->NumberActive,
	    Resource->SharedWaiters,
	    Resource->ExclusiveWaiters);
   if (Resource->NumberActive != 0)
     {
	DbgPrint("\towner thread = %08x\n",
		 Resource->OwningThread);
     }
}

/* EOF */
