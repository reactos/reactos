/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/excutive/resource.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

BOOLEAN ExAcquireResourceExclusive(PERESOURCE Resource, BOOLEAN Wait)
{
   UNIMPLEMENTED;
}

BOOLEAN ExAcquireResourceExclusiveLite(PERESOURCE Resource, BOOLEAN Wait)
{   
   UNIMPLEMENTED;
}

BOOLEAN ExAcquireResourceSharedLite(PERESOURCE Resource, BOOLEAN Wait)
{
   UNIMPLEMENTED;
}

VOID ExConvertExclusiveToSharedLite(PERESOURCE Resource)
{
   UNIMPLEMENTED;
}

ULONG ExGetExclusiveWaiterCount(PERESOURCE Resource)
{
   UNIMPLEMENTED;
}

BOOLEAN ExAcquireSharedStarveExclusive(PERESOURCE Resource, BOOLEAN Wait)
{
   UNIMPLEMENTED;
}

BOOLEAN ExAcquireSharedWaitForExclusive(PERESOURCE Resource, BOOLEAN Wait)
{
   UNIMPLEMENTED;
}

NTSTATUS ExDeleteResource(PERESOURCE Resource)
{
   UNIMPLEMENTED;
}

NTSTATUS ExDeleteResourceLite(PERESOURCE Resource)
{
   UNIMPLEMENTED;
}

ERESOURCE_THREAD ExGetCurrentResourceThread()
{
   UNIMPLEMENTED;
}

ULONG ExGetSharedWaiterCount(PERESOURCE Resource)
{
   UNIMPLEMENTED;
}

NTSTATUS ExInitializeResource(PERESOURCE Resource)
{
}

NTSTATUS ExInitializeResourceLite(PERESOURCE Resource)
{
   Resource->NumberOfSharedWaiters = 0;
   Resource->NumberOfExclusiveWaiters = 0;
   KeInitializeSpinLock(&Resource->SpinLock);
   Resource->Flag=0;
   KeInitializeEvent(&Resource->ExclusiveWaiters,SynchronizationEvent,
		     FALSE);
   KeInitializeSemaphore(&Resource->SharedWaiters,5,0);
   Resource->ActiveCount = 0;
}

BOOLEAN ExIsResourceAcquiredExclusiveLite(PERESOURCE Resource)
{
   return(Resource->NumberOfExclusiveWaiters!=0);
}

BOOLEAN ExIsResourceAcquiredSharedLite(PERESOURCE Resource)
{
   return(Resource->NumberOfSharedWaiters!=0);
}

VOID ExReinitializeResourceLite(PERESOURCE Resource)
{
   UNIMPLEMENTED;
}

VOID ExReleaseResource(PERESOURCE Resource)
{
   UNIMPLEMENTED;
}

VOID ExReleaseResourceForThread(PERESOURCE Resource, 
				ERESOURCE_THREAD ResourceThreadId)
{
   UNIMPLEMENTED;
}

VOID ExReleaseResourceForThreadLite(PERESOURCE Resource,
				    ERESOURCE_THREAD ResourceThreadId)
{
   UNIMPLEMENTED;
}

BOOLEAN ExTryToAcquireResourceExclusiveLite(PERESOURCE Resource)
{
   LARGE_INTEGER timeout;

  SET_LARGE_INTEGER_HIGH_PART(timeout, 0);
  SET_LARGE_INTEGER_LOW_PART(timeout, 0);
  return KeWaitForSingleObject(&Resource->ExclusiveWaiters,
                               Executive,
                               KernelMode,
                               FALSE, 
                               &timeout) == STATUS_SUCCESS;
}


