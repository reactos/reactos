/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/excutive/resource.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */
/*
 * Usage of ERESOURCE members is not documented.
 * From names of members and functionnalities, we can assume :
 * ActiveCount = number of threads who have access to the resource
 * OwnerTable = list of threads who have shared access
 * OwnerThreads[0]= thread who have exclusive access
 * OwnerThreads[1]= thread who have shared access if only one thread
 *                  else
 *                     OwnerThread=0
 *                      and TableSize = number of entries in the owner table
 * Flag = bits : ResourceOwnedExclusive=0x80
 *               ResourceNeverExclusive=0x10
 *               ResourceReleaseByOtherThread=0x20
 *
 */

#define ResourceOwnedExclusive 0x80

/* INCLUDES *****************************************************************/


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
   UNIMPLEMENTED;
}

NTSTATUS ExInitializeResourceLite(PERESOURCE Resource)
{
   memset(Resource,0,sizeof(ERESOURCE));
//   Resource->NumberOfSharedWaiters = 0;
//   Resource->NumberOfExclusiveWaiters = 0;
   KeInitializeSpinLock(&Resource->SpinLock);
//   Resource->Flag=0;
   Resource->ExclusiveWaiters = ExAllocatePool(NonPagedPool,
					       sizeof(KEVENT));					       
   KeInitializeEvent(Resource->ExclusiveWaiters,SynchronizationEvent,
		     FALSE);
   Resource->SharedWaiters = ExAllocatePool(NonPagedPool,
					    sizeof(KSEMAPHORE));
   KeInitializeSemaphore(Resource->SharedWaiters,5,0);
//   Resource->ActiveCount = 0;
   return(STATUS_SUCCESS);
}

BOOLEAN ExIsResourceAcquiredExclusiveLite(PERESOURCE Resource)
{
 return((Resource->Flag & ResourceOwnedExclusive)
       && Resource->OwnerThreads[0].OwnerThread==ExGetCurrentResourceThread());
}

/* note : this function is defined USHORT in nt4, but ULONG in nt5
 * ULONG is more logical, since return value is number of times the resource
 * is acquired by the caller, and this value is defined ULONG in
 * PERESOURCE struct
 */
ULONG ExIsResourceAcquiredSharedLite(PERESOURCE Resource)
{
 long i;
  if(Resource->OwnerThreads[0].OwnerThread==ExGetCurrentResourceThread())
    return Resource->OwnerThreads[0].a.OwnerCount;
  if(Resource->OwnerThreads[1].OwnerThread==ExGetCurrentResourceThread())
    return Resource->OwnerThreads[1].a.OwnerCount;
  if(!Resource->OwnerThreads[1].a.TableSize) return 0;
  for(i=0;i<Resource->OwnerThreads[1].a.TableSize;i++)
  {
    if(Resource->OwnerTable[i].OwnerThread==ExGetCurrentResourceThread())
      return Resource->OwnerTable[i].a.OwnerCount;
  }
  return 0;
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


