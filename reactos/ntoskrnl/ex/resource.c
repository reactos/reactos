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
 *
 * OwnerTable = list of threads who have shared access(if more than one)
 * ActiveCount = number of threads who have access to the resource
 * Flag = bits : ResourceOwnedExclusive=0x80
 *               ResourceNeverExclusive=0x10
 *               ResourceReleaseByOtherThread=0x20
 * SharedWaiters = semaphore, used to manage wait list of shared waiters.
 * ExclusiveWaiters = event, used to manage wait list of exclusive waiters.
 * OwnerThreads[0]= thread who have exclusive access
 * OwnerThreads[1]= if only one thread own the resource
 *                     thread who have shared access
 *                  else
 *                     OwnerThread=0
 *                     and TableSize = number of entries in the owner table
 * NumberOfExclusiveWaiters = number of threads waiting for exclusive access.
 * NumberOfSharedWaiters = number of threads waiting for exclusive access.
 *
 */

#define ResourceOwnedExclusive 0x80

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <stddef.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

BOOLEAN ExAcquireResourceExclusive(PERESOURCE Resource, BOOLEAN Wait)
{
   return ExAcquireResourceExclusiveLite(Resource,Wait);
}

BOOLEAN ExAcquireResourceExclusiveLite(PERESOURCE Resource, BOOLEAN Wait)
{   
// FIXME : protect essential tasks of this function from interrupts
// perhaps with KeRaiseIrql(SYNCH_LEVEL);
   if(Resource->ActiveCount) // resource already locked
   {
      if((Resource->Flag & ResourceOwnedExclusive)
          && Resource->OwnerThreads[0].OwnerThread==ExGetCurrentResourceThread())
       { // it's ok : same lock for same thread
        Resource->OwnerThreads[0].a.OwnerCount++;
        return TRUE;
       }
      if( ! Wait)  return FALSE;
      Resource->NumberOfExclusiveWaiters++;
      if(KeWaitForSingleObject(Resource->ExclusiveWaiters,Executive,KernelMode,FALSE,NULL)
          ==STATUS_TIMEOUT)
      {
         //FIXME : what to do if timeout ?
         Resource->NumberOfExclusiveWaiters--;
         return FALSE;
      }
        Resource->NumberOfExclusiveWaiters--;
   }
   Resource->OwnerThreads[0].a.OwnerCount=1;
   Resource->Flag |= ResourceOwnedExclusive;
   Resource->ActiveCount=1;
   Resource->OwnerThreads[0].OwnerThread=ExGetCurrentResourceThread();
   return TRUE;
}

BOOLEAN ExAcquireResourceSharedLite(PERESOURCE Resource, BOOLEAN Wait)
{
 POWNER_ENTRY freeEntry;
 unsigned long i;
// FIXME : protect from interrupts
  // first, resolve trivial cases
  if(Resource->ActiveCount==0) // no owner, it's easy
  {
    Resource->OwnerThreads[1].OwnerThread=ExGetCurrentResourceThread();
    Resource->OwnerThreads[1].a.OwnerCount=1;
    Resource->ActiveCount=1;
    return TRUE;
  }
  if((Resource->Flag & ResourceOwnedExclusive)
     &&  Resource->OwnerThreads[0].OwnerThread==ExGetCurrentResourceThread())
  {// exclusive, but by same thread : it's ok
      Resource->OwnerThreads[0].a.OwnerCount++;
      return TRUE;
  }
  if((Resource->Flag & ResourceOwnedExclusive)
       || Resource->NumberOfExclusiveWaiters)
  { //exclusive by another thread , or thread waiting for exclusive
    if(!Wait) return FALSE;
    else
    {
      Resource->NumberOfSharedWaiters++;
      // wait for the semaphore
      if(KeWaitForSingleObject(Resource->SharedWaiters,0,0,0,0)==STATUS_TIMEOUT)
      {
         //FIXME : what to do ?
         return FALSE;
      }
    }
  }
  //now, we must search if this thread has already acquired this resource
  //then increase ownercount if found, else create new entry or reuse free entry
  if(!Resource->OwnerThreads[1].a.TableSize)
  {
     // allocate ownertable,memset to 0, initialize first entry
     Resource->OwnerTable=ExAllocatePool(NonPagedPool,sizeof(OWNER_ENTRY)*3);
     memset(Resource->OwnerTable,sizeof(OWNER_ENTRY)*3,0);
     Resource->OwnerTable[0].OwnerThread = Resource->OwnerThreads[1].OwnerThread;
     Resource->OwnerTable[0].a.OwnerCount=Resource->OwnerThreads[1].a.OwnerCount;
     Resource->OwnerThreads[1].OwnerThread=0;;
     Resource->OwnerThreads[1].a.TableSize=3;
     Resource->OwnerTable[1].OwnerThread=ExGetCurrentResourceThread();
     Resource->OwnerTable[1].a.OwnerCount=1;
     return TRUE;
  }
  freeEntry=NULL;
  for(i=0;i<Resource->OwnerThreads[1].a.TableSize;i++)
  {
    if(Resource->OwnerTable[i].OwnerThread==ExGetCurrentResourceThread())
    {// old entry for this thread found
      Resource->OwnerTable[i].a.OwnerCount++;
      return TRUE;
    }
    if(Resource->OwnerTable[i].OwnerThread==0)
       freeEntry = &Resource->OwnerTable[i];
  }
  if(! freeEntry)
  {
    // reallocate ownertable with one more entry
    freeEntry=ExAllocatePool(NonPagedPool
                        ,sizeof(OWNER_ENTRY)*(Resource->OwnerThreads[1].a.TableSize+1));
    memcpy(freeEntry,Resource->OwnerTable
                    ,sizeof(OWNER_ENTRY)*(Resource->OwnerThreads[1].a.TableSize));
    ExFreePool(Resource->OwnerTable);
    Resource->OwnerTable=freeEntry;
    freeEntry=&Resource->OwnerTable[Resource->OwnerThreads[1].a.TableSize];
    Resource->OwnerThreads[1].a.TableSize ++;
  }
  freeEntry->OwnerThread=ExGetCurrentResourceThread();
  freeEntry->a.OwnerCount=1;
  Resource->ActiveCount++;
  return TRUE;
}

VOID ExConvertExclusiveToSharedLite(PERESOURCE Resource)
{
 int oldWaiters=Resource->NumberOfSharedWaiters;
  if(!Resource->Flag & ResourceOwnedExclusive) return;
  //transfer infos from entry 0 to entry 1 and erase entry 0
  Resource->OwnerThreads[1].OwnerThread=Resource->OwnerThreads[0].OwnerThread;
  Resource->OwnerThreads[1].a.OwnerCount=Resource->OwnerThreads[0].a.OwnerCount;
  Resource->OwnerThreads[0].OwnerThread=0;
  Resource->OwnerThreads[0].a.OwnerCount=0;
  //erase exclusive flag
  Resource->Flag &= (~ResourceOwnedExclusive);
  //if no shared waiters, that's all :
  if(!oldWaiters) return;
  //else, awake the waiters :
  Resource->ActiveCount += oldWaiters;
  Resource->NumberOfSharedWaiters=0;
  KeReleaseSemaphore(Resource->SharedWaiters,0,oldWaiters,0);
}

ULONG ExGetExclusiveWaiterCount(PERESOURCE Resource)
{
  return Resource->NumberOfExclusiveWaiters;
}

BOOLEAN ExAcquireSharedStarveExclusive(PERESOURCE Resource, BOOLEAN Wait)
{
 POWNER_ENTRY freeEntry;
 unsigned long i;
// FIXME : protect from interrupts
  // first, resolve trivial cases
  if(Resource->ActiveCount==0) // no owner, it's easy
  {
    Resource->OwnerThreads[1].OwnerThread=ExGetCurrentResourceThread();
    Resource->OwnerThreads[1].a.OwnerCount=1;
    Resource->ActiveCount=1;
    return TRUE;
  }
  if((Resource->Flag & ResourceOwnedExclusive)
     &&  Resource->OwnerThreads[0].OwnerThread==ExGetCurrentResourceThread())
  {// exclusive, but by same thread : it's ok
      Resource->OwnerThreads[0].a.OwnerCount++;
      return TRUE;
  }
  if(Resource->Flag & ResourceOwnedExclusive)
  { //exclusive by another thread , or thread waiting for exclusive
    if(!Wait) return FALSE;
    else
    {
      Resource->NumberOfSharedWaiters++;
      // wait for the semaphore
      if(KeWaitForSingleObject(Resource->SharedWaiters,0,0,0,0)==STATUS_TIMEOUT)
      {
         //FIXME : what to do ?
         return FALSE;
      }
    }
  }
  //now, we must search if this thread has already acquired this resource
  //then increase ownercount if found, else create new entry or reuse free entry
  if(!Resource->OwnerThreads[1].a.TableSize)
  {
     // allocate ownertable,memset to 0, initialize first entry
     Resource->OwnerTable=ExAllocatePool(NonPagedPool,sizeof(OWNER_ENTRY)*3);
     memset(Resource->OwnerTable,sizeof(OWNER_ENTRY)*3,0);
     Resource->OwnerTable[0].OwnerThread = Resource->OwnerThreads[1].OwnerThread;
     Resource->OwnerTable[0].a.OwnerCount=Resource->OwnerThreads[1].a.OwnerCount;
     Resource->OwnerThreads[1].OwnerThread=0;;
     Resource->OwnerThreads[1].a.TableSize=3;
     Resource->OwnerTable[1].OwnerThread=ExGetCurrentResourceThread();
     Resource->OwnerTable[1].a.OwnerCount=1;
     return TRUE;
  }
  freeEntry=NULL;
  for(i=0;i<Resource->OwnerThreads[1].a.TableSize;i++)
  {
    if(Resource->OwnerTable[i].OwnerThread==ExGetCurrentResourceThread())
    {// old entry for this thread found
      Resource->OwnerTable[i].a.OwnerCount++;
      return TRUE;
    }
    if(Resource->OwnerTable[i].OwnerThread==0)
       freeEntry = &Resource->OwnerTable[i];
  }
  if(! freeEntry)
  {
    // reallocate ownertable with one more entry
    freeEntry=ExAllocatePool(NonPagedPool
                        ,sizeof(OWNER_ENTRY)*(Resource->OwnerThreads[1].a.TableSize+1));
    memcpy(freeEntry,Resource->OwnerTable
                    ,sizeof(OWNER_ENTRY)*(Resource->OwnerThreads[1].a.TableSize));
    ExFreePool(Resource->OwnerTable);
    Resource->OwnerTable=freeEntry;
    freeEntry=&Resource->OwnerTable[Resource->OwnerThreads[1].a.TableSize];
    Resource->OwnerThreads[1].a.TableSize ++;
  }
  freeEntry->OwnerThread=ExGetCurrentResourceThread();
  freeEntry->a.OwnerCount=1;
  Resource->ActiveCount++;
  return TRUE;
}

BOOLEAN ExAcquireSharedWaitForExclusive(PERESOURCE Resource, BOOLEAN Wait)
{
  return ExAcquireResourceSharedLite(Resource,Wait);
}

NTSTATUS ExDeleteResource(PERESOURCE Resource)
{
   if(Resource->OwnerTable) ExFreePool(Resource->OwnerTable);
   if(Resource->SharedWaiters) ExFreePool(Resource->SharedWaiters);
   if(Resource->ExclusiveWaiters) ExFreePool(Resource->ExclusiveWaiters);
   return 0;
}

NTSTATUS ExDeleteResourceLite(PERESOURCE Resource)
{
   if(Resource->OwnerTable) ExFreePool(Resource->OwnerTable);
   if(Resource->SharedWaiters) ExFreePool(Resource->SharedWaiters);
   if(Resource->ExclusiveWaiters) ExFreePool(Resource->ExclusiveWaiters);
   return 0;
}

ERESOURCE_THREAD ExGetCurrentResourceThread()
{
   return ((ERESOURCE_THREAD)PsGetCurrentThread() );
}

ULONG ExGetSharedWaiterCount(PERESOURCE Resource)
{
   return Resource->NumberOfSharedWaiters;
}

NTSTATUS ExInitializeResource(PERESOURCE Resource)
{
   return ExInitializeResourceLite(Resource);
}

NTSTATUS ExInitializeResourceLite(PERESOURCE Resource)
{
   memset(Resource,0,sizeof(ERESOURCE));
//   Resource->NumberOfSharedWaiters = 0;
//   Resource->NumberOfExclusiveWaiters = 0;
   KeInitializeSpinLock(&Resource->SpinLock);
//   Resource->Flag=0;
   // FIXME : I'm not sure it's a good idea to allocate and initialize
   // immediately Waiters.
   Resource->ExclusiveWaiters=ExAllocatePool(NonPagedPool , sizeof(KEVENT));
   KeInitializeEvent(Resource->ExclusiveWaiters,SynchronizationEvent,
		     FALSE);
   Resource->SharedWaiters=ExAllocatePool(NonPagedPool ,sizeof(KSEMAPHORE));
   KeInitializeSemaphore(Resource->SharedWaiters,0,0x7fffffff);
//   Resource->ActiveCount = 0;
   return 0;
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
   Resource->NumberOfSharedWaiters = 0;
   Resource->NumberOfExclusiveWaiters = 0;
   KeInitializeSpinLock(&Resource->SpinLock);
   Resource->Flag=0;
   KeInitializeEvent(Resource->ExclusiveWaiters,SynchronizationEvent,
		     FALSE);
   KeInitializeSemaphore(Resource->SharedWaiters,0,0x7fffffff);
   Resource->ActiveCount = 0;
   if(Resource->OwnerTable) ExFreePool(Resource->OwnerTable);
   Resource->OwnerThreads[0].OwnerThread=0;
   Resource->OwnerThreads[0].a.OwnerCount=0;
   Resource->OwnerThreads[1].OwnerThread=0;
   Resource->OwnerThreads[1].a.OwnerCount=0;
}

VOID ExReleaseResourceLite(PERESOURCE Resource)
{
  return ExReleaseResourceForThreadLite(Resource,ExGetCurrentResourceThread());
}

VOID ExReleaseResource(PERESOURCE Resource)
{
  return ExReleaseResourceForThreadLite(Resource,ExGetCurrentResourceThread());
}

VOID ExReleaseResourceForThread(PERESOURCE Resource, 
				ERESOURCE_THREAD ResourceThreadId)
{
  return ExReleaseResourceForThreadLite(Resource,ResourceThreadId);
}

VOID ExReleaseResourceForThreadLite(PERESOURCE Resource,
				    ERESOURCE_THREAD ResourceThreadId)
{
 long i;
  //FIXME : protect this function from interrupts
  // perhaps with KeRaiseIrql(SYNCH_LEVEL);
  if (Resource->Flag & ResourceOwnedExclusive)
  {
    if(--Resource->OwnerThreads[0].a.OwnerCount == 0)
    {//release the resource
      Resource->OwnerThreads[0].OwnerThread=0;
      assert(--Resource->ActiveCount == 0);
      if(Resource->NumberOfExclusiveWaiters)
      {// get resource to first exclusive waiter
        KeDispatcherObjectWake(&Resource->ExclusiveWaiters->Header);
        return;
      }
      else
        Resource->Flag &=(~ResourceOwnedExclusive);
      if(Resource->NumberOfSharedWaiters)
      {// get resource to waiters
        Resource->ActiveCount=Resource->NumberOfSharedWaiters;
        Resource->NumberOfSharedWaiters=0;
        KeReleaseSemaphore(Resource->SharedWaiters,0,Resource->ActiveCount,0);
        return;
      }
    }
    return;
  }
  //search the entry for this thread
  if(Resource->OwnerThreads[1].OwnerThread==ResourceThreadId)
  {
    if( --Resource->OwnerThreads[1].a.OwnerCount == 0)
    {
      Resource->OwnerThreads[1].OwnerThread=0;
      if( --Resource->ActiveCount ==0) //normally always true
      {
        if(Resource->NumberOfExclusiveWaiters)
        {// get resource to first exclusive waiter
          KeDispatcherObjectWake(&Resource->ExclusiveWaiters->Header);
        }
      }
    }
    return;
  }
  if(Resource->OwnerThreads[1].OwnerThread) return ;
  for(i=0;i<Resource->OwnerThreads[1].a.TableSize;i++)
  {
    if(Resource->OwnerTable[i].OwnerThread==ResourceThreadId)
    {
      if( --Resource->OwnerTable[1].a.OwnerCount == 0)
      {
        Resource->OwnerTable[i].OwnerThread=0;
        if( --Resource->ActiveCount ==0)
        {
          ExFreePool(Resource->OwnerTable);
          Resource->OwnerTable=NULL;
          Resource->OwnerThreads[1].a.TableSize=0;
          if(Resource->NumberOfExclusiveWaiters)
          {// get resource to first exclusive waiter
            KeDispatcherObjectWake(&Resource->ExclusiveWaiters->Header);
          }
        }
      }
      return;
    }
  }
}

BOOLEAN ExTryToAcquireResourceExclusiveLite(PERESOURCE Resource)
{
  return ExAcquireResourceExclusiveLite(Resource,FALSE);
}


