/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/resource.c
 * PURPOSE:         Resource synchronization construct
 * PROGRAMMER:      Unknown 
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

	      
BOOLEAN ExTryToAcquireResourceExclusiveLite(PERESOURCE Resource)
/*
 * FUNCTION: Attempts to require the resource for exclusive access
 * ARGUMENTS:
 *         Resource = Points to the resource of be acquired
 * RETURNS: TRUE if the resource was acquired for the caller
 * NOTES: Must be acquired at IRQL < DISPATCH_LEVEL
 */
{
  return(ExAcquireResourceExclusiveLite(Resource,FALSE));
}

BOOLEAN ExAcquireResourceExclusive(PERESOURCE Resource, BOOLEAN Wait)
{
   return(ExAcquireResourceExclusiveLite(Resource,Wait));
}

BOOLEAN ExAcquireResourceExclusiveLite(PERESOURCE Resource, BOOLEAN Wait)
/*
 * FUNCTION: Acquires a resource exclusively for the calling thread
 * ARGUMENTS:
 *       Resource = Points to the resource to acquire
 *       Wait = Is set to TRUE if the caller should wait to acquire the
 *              resource if it can't be acquired immediately
 * RETURNS: TRUE if the resource was acquired,
 *          FALSE otherwise
 * NOTES: Must be called at IRQL < DISPATCH_LEVEL
 */
{
   KIRQL oldIrql;
   
   DPRINT("ExAcquireResourceExclusiveLite(Resource %x, Wait %d)\n",
	  Resource, Wait);
   
   KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);

   /* resource already locked */
   if((Resource->Flag & ResourceOwnedExclusive)
      && Resource->OwnerThreads[0].OwnerThread == ExGetCurrentResourceThread())
     { 
	/* it's ok : same lock for same thread */
	Resource->OwnerThreads[0].a.OwnerCount++;
	KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	DPRINT("ExAcquireResourceExclusiveLite() = TRUE\n");
	return(TRUE);
     }

   if (Resource->ActiveCount && !Wait)
     {
	KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	DPRINT("ExAcquireResourceExclusiveLite() = FALSE\n");
	return(FALSE);
     }
   
   /* 
    * This is slightly better than it looks because other exclusive
    * threads who are waiting won't be woken up but there is a race
    * with new threads trying to grab the resource so we must have
    * the spinlock, still normally this loop will only be executed
    * once
    * NOTE: We might want to set a timeout to detect deadlock 
    * (10 minutes?)
    */
   while (Resource->ActiveCount) 
     {
	Resource->NumberOfExclusiveWaiters++;
	KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	KeWaitForSingleObject(Resource->ExclusiveWaiters,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
	KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);
	Resource->NumberOfExclusiveWaiters--;
     }
   Resource->Flag |= ResourceOwnedExclusive;
   Resource->ActiveCount = 1;
   Resource->OwnerThreads[0].OwnerThread = ExGetCurrentResourceThread();
   Resource->OwnerThreads[0].a.OwnerCount = 1;
   KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
   DPRINT("ExAcquireResourceExclusiveLite() = TRUE\n");
   return(TRUE);
}

static BOOLEAN EiRemoveSharedOwner(PERESOURCE Resource,
				   ERESOURCE_THREAD ResourceThreadId)
/*
 * FUNCTION: Removes the current thread from the shared owners of the resource
 * ARGUMENTS:
 *      Resource = Pointer to the resource for which the thread is to be
 *                 added
 * NOTE: Must be called with the resource spinlock held
 */
{
   ULONG i;
   
   if (Resource->OwnerThreads[1].OwnerThread == ResourceThreadId)
     {
	Resource->OwnerThreads[1].a.OwnerCount--;
	Resource->ActiveCount--;
	if (Resource->OwnerThreads[1].a.OwnerCount == 0)
	  {
	     Resource->OwnerThreads[1].OwnerThread = 0;
	  }
	return(TRUE);
     }
   
   if (Resource->OwnerThreads[1].OwnerThread) 
     {
	/* Oh dear, the caller didn't own the resource after all */
	return(FALSE);;
     }
   
   for (i=0; i<Resource->OwnerThreads[1].a.TableSize; i++)
     {
	if (Resource->OwnerTable[i].OwnerThread == ResourceThreadId)
	  {
	     Resource->OwnerTable[1].a.OwnerCount--;
	     Resource->ActiveCount--;
	     if (Resource->OwnerTable[1].a.OwnerCount == 0)
	       {
		  Resource->OwnerTable[i].OwnerThread = 0;
	       }
	  }
	return(TRUE);
     }
   return(FALSE);
}

static BOOLEAN EiAddSharedOwner(PERESOURCE Resource)
/*
 * FUNCTION: Adds the current thread to the shared owners of the resource
 * ARGUMENTS:
 *         Resource = Pointer to the resource for which the thread is to be
 *                    added
 * NOTE: Must be called with the resource spinlock held
 */
{
   ERESOURCE_THREAD CurrentThread = ExGetCurrentResourceThread();
   POWNER_ENTRY freeEntry;
   ULONG i = 0;
   
   DPRINT("EiAddSharedOwner(Resource %x)\n", Resource);
   
   if (Resource->ActiveCount == 0) 
     {
	/* no owner, it's easy */
	Resource->OwnerThreads[1].OwnerThread = ExGetCurrentResourceThread();
	Resource->OwnerThreads[1].a.OwnerCount = 1;
	if (Resource->OwnerTable != NULL)
	  {
	     ExFreePool(Resource->OwnerTable);
	  }
	Resource->OwnerTable = NULL;
	Resource->ActiveCount = 1;
	DPRINT("EiAddSharedOwner() = TRUE\n");
	return(TRUE);
     }
   
   /* 
    * now, we must search if this thread has already acquired this resource 
    * then increase ownercount if found, else create new entry or reuse free 
    * entry
    */
   if (Resource->OwnerTable == NULL)
     {
	DPRINT("Creating owner table\n");
	
	/* allocate ownertable,memset to 0, initialize first entry */
	Resource->OwnerTable = ExAllocatePool(NonPagedPool,
					      sizeof(OWNER_ENTRY)*3);
	if (Resource->OwnerTable == NULL)
	  {
	     KeBugCheck(0);
	     return(FALSE);
	  }
	memset(Resource->OwnerTable,sizeof(OWNER_ENTRY)*3,0);
	memcpy(&Resource->OwnerTable[0], &Resource->OwnerThreads[1],
	       sizeof(OWNER_ENTRY));
	
	Resource->OwnerThreads[1].OwnerThread = 0;
	Resource->OwnerThreads[1].a.TableSize = 3;
	
	Resource->OwnerTable[1].OwnerThread = CurrentThread;
	Resource->OwnerTable[1].a.OwnerCount = 1;
	
	return(TRUE);
     }
   
   DPRINT("Search free entries\n");
   
   DPRINT("Number of entries %d\n", 
	  Resource->OwnerThreads[1].a.TableSize);
   
   freeEntry = NULL;
   for (i=0; i<Resource->OwnerThreads[1].a.TableSize; i++)
     {
	if (Resource->OwnerTable[i].OwnerThread == CurrentThread)
	  {
	     DPRINT("Thread already owns resource\n");
	     Resource->OwnerTable[i].a.OwnerCount++;
	     return(TRUE);
	  }
	if (Resource->OwnerTable[i].OwnerThread == 0)
	  {
	     freeEntry = &Resource->OwnerTable[i];
	  }
     }
   
   DPRINT("Found free entry %x\n", freeEntry);
   
   if (!freeEntry)
     {
	DPRINT("Allocating new entry\n");
	
	/* reallocate ownertable with one more entry */
	freeEntry = ExAllocatePool(NonPagedPool,
				   sizeof(OWNER_ENTRY)*
				   (Resource->OwnerThreads[1].a.TableSize+1));
	if (freeEntry == NULL)
	  {
	     KeBugCheck(0);
	     return(FALSE);
	  }
	memcpy(freeEntry,Resource->OwnerTable,
	       sizeof(OWNER_ENTRY)*(Resource->OwnerThreads[1].a.TableSize));
	ExFreePool(Resource->OwnerTable);
	Resource->OwnerTable=freeEntry;
	freeEntry=&Resource->OwnerTable[Resource->OwnerThreads[1].a.TableSize];
	Resource->OwnerThreads[1].a.TableSize++;
     }
   DPRINT("Creating entry\n");
   freeEntry->OwnerThread=ExGetCurrentResourceThread();
   freeEntry->a.OwnerCount=1;
   Resource->ActiveCount++;
   return(TRUE);
}

BOOLEAN ExAcquireResourceSharedLite(PERESOURCE Resource, BOOLEAN Wait)
/*
 * FUNCTION: Acquires the given resource for shared access by the calling
 *           thread
 * ARGUMENTS:
 *       Resource = Points to the resource to acquire
 *       Wait = Is set to TRUE if the caller should be put into wait state
 *              until the resource can be acquired if it cannot be acquired
 *              immediately
 * RETURNS: TRUE, if the resource is acquire
 *          FALSE otherwise
 */
{
   KIRQL oldIrql;
   
   DPRINT("ExAcquireResourceSharedLite(Resource %x, Wait %d)\n",
	  Resource, Wait);
   
   KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);
   
   /* first, resolve trivial cases */
   if (Resource->ActiveCount == 0) 
     {
	EiAddSharedOwner(Resource);
	KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	DPRINT("ExAcquireResourceSharedLite() = TRUE\n");
	return(TRUE);
     }
   
   if ((Resource->Flag & ResourceOwnedExclusive)
       && Resource->OwnerThreads[0].OwnerThread==ExGetCurrentResourceThread())
     {
	/* exclusive, but by same thread : it's ok */
	/* 
	 * NOTE: Is this correct? Seems the same as ExConvertExclusiveToShared 
	 */
	Resource->OwnerThreads[0].a.OwnerCount++;
	KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	DPRINT("ExAcquireResourceSharedLite() = TRUE\n");
	return(TRUE);
     }
   
   if ((Resource->Flag & ResourceOwnedExclusive)
       || Resource->NumberOfExclusiveWaiters)
     { 
	/* exclusive by another thread , or thread waiting for exclusive */
	if (!Wait) 
	  {
	     KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	     DPRINT("ExAcquireResourceSharedLite() = FALSE\n");
	     return(FALSE);
	  }
	else
	  {
	     Resource->NumberOfSharedWaiters++;
	     /* wait for the semaphore */
	     KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	     KeWaitForSingleObject(Resource->SharedWaiters,0,0,0,0);
	     KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);
	     Resource->NumberOfSharedWaiters--;
	  }
     }
   
   EiAddSharedOwner(Resource);
   KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
   DPRINT("ExAcquireResourceSharedLite() = TRUE\n");
   return(TRUE);
}

VOID ExConvertExclusiveToSharedLite(PERESOURCE Resource)
/*
 * FUNCTION: Converts a given resource from acquired for exclusive access
 *           to acquire for shared access
 * ARGUMENTS:
 *      Resource = Points to the resource for which the access should be
 *                 converted
 * NOTES: Caller must be running at IRQL < DISPATCH_LEVEL
 */
{
   ULONG oldWaiters;
   KIRQL oldIrql;
   
   DPRINT("ExConvertExclusiveToSharedLite(Resource %x)\n", Resource);
   
   KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);
   
   oldWaiters = Resource->NumberOfSharedWaiters;
   
   if (!(Resource->Flag & ResourceOwnedExclusive))
     {
	/* Might not be what the caller expects, better bug check */
	KeBugCheck(0);
	KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	return;
     }
   
   //transfer infos from entry 0 to entry 1 and erase entry 0
   Resource->OwnerThreads[1].OwnerThread=Resource->OwnerThreads[0].OwnerThread;
   Resource->OwnerThreads[1].a.OwnerCount=Resource->OwnerThreads[0].a.OwnerCount;
   Resource->OwnerThreads[0].OwnerThread=0;
   Resource->OwnerThreads[0].a.OwnerCount=0;
   /* erase exclusive flag */
   Resource->Flag &= (~ResourceOwnedExclusive);
   /* if no shared waiters, that's all */
   if (!oldWaiters) 
     {
	KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	return;
     }
   /* else, awake the waiters */
   KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
   KeReleaseSemaphore(Resource->SharedWaiters,0,oldWaiters,0);
   DPRINT("ExConvertExclusiveToSharedLite() finished\n");
}

ULONG ExGetExclusiveWaiterCount(PERESOURCE Resource)
{
  return(Resource->NumberOfExclusiveWaiters);
}

BOOLEAN ExAcquireSharedStarveExclusive(PERESOURCE Resource, BOOLEAN Wait)
/*
 * FUNCTION: Acquires a given resource for shared access without waiting
 *           for any pending attempts to acquire exclusive access to the
 *           same resource
 * ARGUMENTS:
 *       Resource = Points to the resource to be acquired for shared access
 *       Wait = Is set to TRUE if the caller will wait until the resource
 *              becomes available when access can't be granted immediately
 * RETURNS: TRUE if the requested access is granted. The routine returns
 *          FALSE if the input Wait is FALSE and shared access can't be
 *          granted immediately
 */
{
   KIRQL oldIrql;
   
   DPRINT("ExAcquireSharedStarveExclusive(Resource %x, Wait %d)\n",
	  Resource, Wait);
   
   KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);
   
   /* no owner, it's easy */
   if (Resource->ActiveCount == 0) 
     {
	Resource->OwnerThreads[1].OwnerThread=ExGetCurrentResourceThread();
	Resource->OwnerThreads[1].a.OwnerCount=1;
	Resource->ActiveCount=1;
	KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	DPRINT("ExAcquireSharedStarveExclusive() = TRUE\n");
	return(TRUE);
     }
   
   if ((Resource->Flag & ResourceOwnedExclusive)
       &&  Resource->OwnerThreads[0].OwnerThread==ExGetCurrentResourceThread())
     { 
	/* exclusive, but by same thread : it's ok */
	Resource->OwnerThreads[0].a.OwnerCount++;
	KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	DPRINT("ExAcquireSharedStarveExclusive() = TRUE\n");
	return(TRUE);
     }
   
   if (Resource->Flag & ResourceOwnedExclusive)
     { 
	/* exclusive by another thread */
	if (!Wait) 
	  {
	     KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	     DPRINT("ExAcquireSharedStarveExclusive() = FALSE\n");
	     return(FALSE);
	  }
	else
	  {
	     Resource->NumberOfSharedWaiters++;
	     /* wait for the semaphore */
	     KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	     KeWaitForSingleObject(Resource->SharedWaiters,0,0,0,0);
	     KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);
	     Resource->NumberOfSharedWaiters--;
	  }
     }
   EiAddSharedOwner(Resource);
   KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
   DPRINT("ExAcquireSharedStarveExclusive() = TRUE\n");
   return(TRUE);
}

BOOLEAN ExAcquireSharedWaitForExclusive(PERESOURCE Resource, BOOLEAN Wait)
{
  return(ExAcquireResourceSharedLite(Resource,Wait));
}

NTSTATUS ExDeleteResource(PERESOURCE Resource)
{
   return(ExDeleteResourceLite(Resource));
}

NTSTATUS ExDeleteResourceLite(PERESOURCE Resource)
{
   DPRINT("ExDeleteResourceLite(Resource %x)\n", Resource);
   if (Resource->OwnerTable) ExFreePool(Resource->OwnerTable);
   if (Resource->SharedWaiters) ExFreePool(Resource->SharedWaiters);
   if (Resource->ExclusiveWaiters) ExFreePool(Resource->ExclusiveWaiters);
   return(STATUS_SUCCESS);;
}

ERESOURCE_THREAD ExGetCurrentResourceThread()
{
   return((ERESOURCE_THREAD)PsGetCurrentThread());
}

ULONG ExGetSharedWaiterCount(PERESOURCE Resource)
{
   return(Resource->NumberOfSharedWaiters);
}

NTSTATUS ExInitializeResource(PERESOURCE Resource)
{
   return(ExInitializeResourceLite(Resource));
}

NTSTATUS ExInitializeResourceLite(PERESOURCE Resource)
{
   DPRINT("ExInitializeResourceLite(Resource %x)\n", Resource);
   memset(Resource,0,sizeof(ERESOURCE));
   Resource->NumberOfSharedWaiters = 0;
   Resource->NumberOfExclusiveWaiters = 0;
   KeInitializeSpinLock(&Resource->SpinLock);
   Resource->Flag = 0;
   Resource->ExclusiveWaiters = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
   KeInitializeEvent(Resource->ExclusiveWaiters,
		     SynchronizationEvent,
		     FALSE);
   Resource->SharedWaiters = ExAllocatePool(NonPagedPool ,sizeof(KSEMAPHORE));
   KeInitializeSemaphore(Resource->SharedWaiters,0,0x7fffffff);
   Resource->ActiveCount = 0;
   return(0);
}

BOOLEAN ExIsResourceAcquiredExclusiveLite(PERESOURCE Resource)
/*
 * FUNCTION: Returns whether the current thread has exclusive access to
 * a given resource
 * ARGUMENTS:
 *        Resource = Points to the resource to be queried
 * RETURNS: TRUE if the caller has exclusive access to the resource,
 *          FALSE otherwise
 */
{
   return((Resource->Flag & ResourceOwnedExclusive)
	  && Resource->OwnerThreads[0].OwnerThread==ExGetCurrentResourceThread());
}

ULONG ExIsResourceAcquiredSharedLite(PERESOURCE Resource)
/*
 * FUNCTION: Returns whether the current thread has shared access to a given
 *           resource
 * ARGUMENTS:
 *      Resource = Points to the resource to be queried
 * RETURNS: The number of times the caller has acquired shared access to the
 *          given resource
 */ 
{
   ULONG i;
   if (Resource->OwnerThreads[0].OwnerThread == ExGetCurrentResourceThread())
     {
	return(Resource->OwnerThreads[0].a.OwnerCount);
     }
   if (Resource->OwnerThreads[1].OwnerThread == ExGetCurrentResourceThread())
     {
	return(Resource->OwnerThreads[1].a.OwnerCount);
     }
   if (!Resource->OwnerThreads[1].a.TableSize) 
     {
	return(0);
     }
   for (i=0; i<Resource->OwnerThreads[1].a.TableSize; i++)
     {
	if (Resource->OwnerTable[i].OwnerThread==ExGetCurrentResourceThread())
	  {
	     return Resource->OwnerTable[i].a.OwnerCount;
	  }
     }
   return(0);
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
   if (Resource->OwnerTable) 
     {
	ExFreePool(Resource->OwnerTable);
     }
   Resource->OwnerThreads[0].OwnerThread=0;
   Resource->OwnerThreads[0].a.OwnerCount=0;
   Resource->OwnerThreads[1].OwnerThread=0;
   Resource->OwnerThreads[1].a.OwnerCount=0;
}

VOID ExReleaseResourceLite(PERESOURCE Resource)
{
  return(ExReleaseResourceForThreadLite(Resource,
					ExGetCurrentResourceThread()));
}

VOID ExReleaseResource(PERESOURCE Resource)
{
  return ExReleaseResourceForThreadLite(Resource,ExGetCurrentResourceThread());
}

VOID ExReleaseResourceForThread(PERESOURCE Resource, 
				ERESOURCE_THREAD ResourceThreadId)
{
  return(ExReleaseResourceForThreadLite(Resource,ResourceThreadId));
}

VOID ExReleaseResourceForThreadLite(PERESOURCE Resource,
				    ERESOURCE_THREAD ResourceThreadId)
/*
 * FUNCTION: Releases a resource for the given thread
 * ARGUMENTS:
 *         Resource = Points to the release to release
 *         ResourceThreadId = Identifies the thread that originally acquired
 *                            the resource
 * NOTES: Must be running at IRQL < DISPATCH_LEVEL
 * BUG: We don't support starving exclusive waiters
 */
{
   KIRQL oldIrql;
   
   DPRINT("ExReleaseResourceForThreadLite(Resource %x, ResourceThreadId %x)\n",
	  Resource, ResourceThreadId);
   
   KeAcquireSpinLock(&Resource->SpinLock, &oldIrql);
   
   if (Resource->Flag & ResourceOwnedExclusive)
     {
	DPRINT("Releasing from exclusive access\n");
	
	Resource->OwnerThreads[0].a.OwnerCount--;
	if (Resource->OwnerThreads[0].a.OwnerCount > 0)
	  {
	     KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	     DPRINT("ExReleaseResourceForThreadLite() finished\n");
	     return;
	  }
	
	Resource->OwnerThreads[0].OwnerThread = 0;
	Resource->ActiveCount--;
	Resource->Flag &=(~ResourceOwnedExclusive);
	assert(Resource->ActiveCount == 0);
	DPRINT("Resource->NumberOfExclusiveWaiters %d\n",
	       Resource->NumberOfExclusiveWaiters);
	if (Resource->NumberOfExclusiveWaiters)
	  {
	     /* get resource to first exclusive waiter */
	     KeSetEvent(Resource->ExclusiveWaiters, 
			IO_NO_INCREMENT, 
			FALSE);
	     KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	     DPRINT("ExReleaseResourceForThreadLite() finished\n");
	     return;
	  }
	DPRINT("Resource->NumberOfSharedWaiters %d\n",
	       Resource->NumberOfSharedWaiters);
	if (Resource->NumberOfSharedWaiters)
	  {
	     DPRINT("Releasing semaphore\n");
	     KeReleaseSemaphore(Resource->SharedWaiters,
				IO_NO_INCREMENT,
				Resource->NumberOfSharedWaiters,
				FALSE);
	  }
	KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
	DPRINT("ExReleaseResourceForThreadLite() finished\n");
	return;
     }
  
   EiRemoveSharedOwner(Resource, ResourceThreadId);
   
   if (Resource->ActiveCount == 0)
     {
	if (Resource->NumberOfExclusiveWaiters)
	  { 
	     /* get resource to first exclusive waiter */
	     KeSetEvent(Resource->ExclusiveWaiters,
			IO_NO_INCREMENT,
			FALSE);
	  }
     }
     
   KeReleaseSpinLock(&Resource->SpinLock, oldIrql);
   DPRINT("ExReleaseResourceForThreadLite() finished\n");
}


