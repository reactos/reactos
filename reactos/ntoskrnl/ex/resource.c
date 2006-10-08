/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/resource.c
 * PURPOSE:         ERESOURCE Implementation
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* WARNING:
 * This implementation is the Windows NT 5.x one.
 * NT 6.0 beta has optimized the OwnerThread entry array
 * and the internals of ExpFindEntryForThread and ExpFindFreeEntry
 * need to be modified accordingly in order to support the WDK.
 * These changes will not be made here until NT 6.0 reaches RTM status since
 * there is a possibility that they will be removed; as such, do NOT
 * update ERESOURCE/relevant code to the WDK definition.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, ExpResourceInitialization)
#endif

/* Macros for reading resource flags */
#define IsExclusiveWaiting(r)   (r->NumberOfExclusiveWaiters)
#define IsSharedWaiting(r)      (r->NumberOfSharedWaiters)
#define IsOwnedExclusive(r)     (r->Flag & ResourceOwnedExclusive)

/* DATA***********************************************************************/

LARGE_INTEGER ExpTimeout;
ULONG ExpResourceTimeoutCount = 90 * 3600 / 2;
KSPIN_LOCK ExpResourceSpinLock;
LIST_ENTRY ExpSystemResourcesList;
BOOLEAN ExResourceStrict = FALSE; /* FIXME */

/* PRIVATE FUNCTIONS *********************************************************/

#if DBG
/*++
 * @name ExpVerifyResource
 *
 *     The ExpVerifyResource routine verifies the correctness of an ERESOURCE
 *
 * @param Resource
 *        Pointer to the resource being verified.
 *
 * @return None.
 *
 * @remarks Only present on DBG builds.
 *
 *--*/
VOID
NTAPI
ExpVerifyResource(IN PERESOURCE_XP Resource)
{
    /* Verify the resource data */
    ASSERT((((ULONG_PTR)Resource) & (sizeof(ULONG_PTR) - 1)) == 0);
    ASSERT(!Resource->SharedWaiters ||
            Resource->SharedWaiters->Header.Type == SemaphoreObject);
    ASSERT(!Resource->SharedWaiters ||
            Resource->SharedWaiters->Header.Size == (sizeof(KSEMAPHORE) / sizeof(ULONG)));
    ASSERT(!Resource->ExclusiveWaiters ||
            Resource->ExclusiveWaiters->Header.Type == SynchronizationEvent);
    ASSERT(!Resource->ExclusiveWaiters ||
            Resource->ExclusiveWaiters->Header.Size == (sizeof(KEVENT) / sizeof(ULONG)));
}

/*++
 * @name ExpCheckForApcsDisabled
 *
 *     The ExpCheckForApcsDisabled routine checks if Kernel APCs are still
 *     enabled when they should be disabled, and optionally breakpoints.
 *
 * @param BreakIfTrue
 *        Specifies if we should break if an invalid APC State is detected.
 *
 * @param Resource
 *        Pointer to the resource being checked.
 *
 * @param Thread
 *        Pointer to the thread being checked.
 *
 * @return None.
 *
 * @remarks Only present on DBG builds. Depends on ExResourceStrict value.
 *
 *--*/
VOID
NTAPI
ExpCheckForApcsDisabled(IN BOOLEAN BreakIfTrue,
                        IN PERESOURCE_XP Resource,
                        IN PETHREAD Thread)
{
    /* Check if we should care and check if we should break */
    if ((ExResourceStrict) &&
        (BreakIfTrue) &&
        !(Thread->SystemThread) &&
        !(Thread->Tcb.CombinedApcDisable))
    {
        /* Bad! */
        DPRINT1("EX: resource: APCs still enabled before resource %p acquire "
                "!!!\n", Resource);
        DbgBreakPoint();
    }
}
#else
#define ExpVerifyResource(r)
#define ExpCheckForApcsDisabled(b,r,t)
#endif

/*++
 * @name ExpResourceInitialization
 *
 *     The ExpResourceInitialization routine initializes resources for use.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks This routine should only be called once, during system startup.
 *
 *--*/
VOID
NTAPI
INIT_FUNCTION
ExpResourceInitialization(VOID)
{
    /* Setup the timeout */
    ExpTimeout.QuadPart = Int32x32To64(4, -10000000);
    InitializeListHead(&ExpSystemResourcesList);
    KeInitializeSpinLock(&ExpResourceSpinLock);
}

/*++
 * @name ExpAllocateExclusiveWaiterEvent
 *
 *     The ExpAllocateExclusiveWaiterEvent routine creates the event that will
 *     be used by exclusive waiters on the resource.
 *
 * @param Resource
 *        Pointer to the resource.
 *
 * @param OldIrql
 *        Pointer to current IRQL. TBC: Pointer to in-stack queued spinlock.
 *
 * @return None.
 *
 * @remarks The pointer to the event must be atomically set.
 *
 *--*/
VOID
NTAPI
ExpAllocateExclusiveWaiterEvent(IN PERESOURCE_XP Resource,
                                IN PKIRQL OldIrql)
{
    PKEVENT Event;

    /* Release the lock */
    ExReleaseResourceLock(&Resource->SpinLock, *OldIrql);

    /* Allocate the event */
    Event = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(KEVENT),
                                  TAG_RESOURCE_EVENT);

    /* Initialize it */
    KeInitializeEvent(Event, SynchronizationEvent, FALSE);

    /* Set it */
    if (InterlockedCompareExchangePointer(&Resource->ExclusiveWaiters,
                                          Event,
                                          NULL))
    {
        /* Someone already set it, free our event */
        DPRINT1("WARNING: Handling race condition\n");
        ExFreePool(Event);
    }

    /* Re-acquire the lock */
    ExAcquireResourceLock(&Resource->SpinLock, OldIrql);
}

/*++
 * @name ExpAllocateSharedWaiterSemaphore
 *
 *     The ExpAllocateSharedWaiterSemaphore routine creates the semaphore that
 *     will be used by shared waiters on the resource.
 *
 * @param Resource
 *        Pointer to the resource.
 *
 * @param OldIrql
 *        Pointer to current IRQL. TBC: Pointer to in-stack queued spinlock.
 *
 * @return None.
 *
 * @remarks The pointer to the semaphore must be atomically set.
 *
 *--*/
VOID
NTAPI
ExpAllocateSharedWaiterSemaphore(IN PERESOURCE_XP Resource,
                                 IN PKIRQL OldIrql)
{
    PKSEMAPHORE Semaphore;

    /* Release the lock */
    ExReleaseResourceLock(&Resource->SpinLock, *OldIrql);

    /* Allocate the semaphore */
    Semaphore = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(KSEMAPHORE),
                                      TAG_RESOURCE_SEMAPHORE);

    /* Initialize it */
    KeInitializeSemaphore(Semaphore, 0, MAXLONG);

    /* Set it */
    if (InterlockedCompareExchangePointer(&Resource->SharedWaiters,
                                          Semaphore,
                                          NULL))
    {
        /* Someone already set it, free our semaphore */
        DPRINT1("WARNING: Handling race condition\n");
        ExFreePool(Semaphore);
    }

    /* Re-acquire the lock */
    ExAcquireResourceLock(&Resource->SpinLock, OldIrql);
}

/*++
 * @name ExpExpandResourceOwnerTable
 *
 *     The ExpExpandResourceOwnerTable routine expands the owner table of the
 *     specified resource.
 *
 * @param Resource
 *        Pointer to the resource.
 *
 * @param OldIrql
 *        Pointer to current IRQL. TBC: Pointer to in-stack queued spinlock.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
ExpExpandResourceOwnerTable(IN PERESOURCE_XP Resource,
                            IN PKIRQL OldIrql)
{
    POWNER_ENTRY Owner, Table;
    ULONG NewSize, OldSize;
    DPRINT("ExpExpandResourceOwnerTable: %p\n", Resource);

    /* Get the owner table */
    Owner = Resource->OwnerTable;
    if (!Owner)
    {
        /* Start with the default size of 3 */
        OldSize = 0;
        NewSize = 3;
    }
    else
    {
        /* Add 4 more entries */
        OldSize = Owner->TableSize;
        NewSize = OldSize + 4;
    }

    /* Release the lock */
    ExReleaseResourceLock(&Resource->SpinLock, *OldIrql);

    /* Allocate memory for the table */
    Table = ExAllocatePoolWithTag(NonPagedPool,
                                  NewSize * sizeof(OWNER_ENTRY),
                                  TAG_RESOURCE_TABLE);

    /* Zero the table */
    RtlZeroMemory((PVOID)(Table + OldSize),
                  (NewSize - OldSize) * sizeof(OWNER_ENTRY));

    /* Lock the resource */
    ExAcquireResourceLock(&Resource->SpinLock, OldIrql);

    /* Make sure nothing has changed */
    if ((Owner != Resource->OwnerTable) ||
        ((Owner) && (OldSize != Resource->OwnerTable->TableSize)))
    {
        /* Resource changed while we weren't holding the lock; bail out */
        ExReleaseResourceLock(&Resource->SpinLock, *OldIrql);
        ExFreePool(Table);
    }
    else
    {
        /* Copy the table */
        RtlCopyMemory((PVOID)Table,
                      Owner,
                      OldSize * sizeof(OWNER_ENTRY));

        /* Acquire dispatcher lock to prevent thread boosting */
        KiAcquireDispatcherLockAtDpcLevel();

        /* Set the new table data */
        Table->TableSize = NewSize;
        Resource->OwnerTable = Table;

        /* Sanity check */
        ExpVerifyResource(Resource);

        /* Release locks */
        KiReleaseDispatcherLockFromDpcLevel();
        ExReleaseResourceLock(&Resource->SpinLock, *OldIrql);

        /* Free the old table */
        if (Owner) ExFreePool(Owner);

        /* Set the resource index */
        if (!OldSize) OldSize = 1;
    }

    /* Set the resource index */
    KeGetCurrentThread()->ResourceIndex = (UCHAR)OldSize;

    /* Lock the resource again */
    ExAcquireResourceLock(&Resource->SpinLock, OldIrql);
}

/*++
 * @name ExpFindFreeEntry
 *
 *     The ExpFindFreeEntry routine locates an empty owner entry in the
 *     specified resource. If none was found, then the owner table is
 *     expanded.
 *
 * @param Resource
 *        Pointer to the resource.
 *
 * @param OldIrql
 *        Pointer to current IRQL. TBC: Pointer to in-stack queued spinlock.
 *
 * @return Pointer to an empty OWNER_ENTRY structure.
 *
 * @remarks None.
 *
 *--*/
POWNER_ENTRY
FASTCALL
ExpFindFreeEntry(IN PERESOURCE_XP Resource,
                 IN PKIRQL OldIrql)
{
    POWNER_ENTRY Owner, Limit;
    ULONG Size;
    POWNER_ENTRY FreeEntry = NULL;
    DPRINT("ExpFindFreeEntry: %p\n", Resource);

    /* Sanity check */
    ASSERT(OldIrql != 0);
    ASSERT(Resource->OwnerThreads[0].OwnerThread != 0);

    /* Check if the next built-in entry is free */
    if (!Resource->OwnerThreads[1].OwnerThread)
    {
        /* Return it */
        FreeEntry = &Resource->OwnerThreads[1];
    }
    else
    {
        /* Get the current table pointer */
        Owner = Resource->OwnerTable;
        if (Owner)
        {
            /* Loop every entry */
            Size = Owner->TableSize;
            Limit = &Owner[Size];

            /* Go to the next entry and loop */
            Owner++;
            do
            {
                /* Check for a free entry */
                if (!Owner->OwnerThread)
                {
                    /* Found one, return it!*/
                    FreeEntry = Owner;
                    break;
                }

                /* Move on */
                Owner++;
            } while (Owner != Limit);

            /* If we found a free entry by now, return it */
            if (FreeEntry)
            {
                /* Set the resource index */
                KeGetCurrentThread()->ResourceIndex = 
                    (UCHAR)(Owner - Resource->OwnerTable);
                return FreeEntry;
            }
        }

        /* No free entry, expand the table */
        ExpExpandResourceOwnerTable(Resource, OldIrql);
        FreeEntry = NULL;
    }

    /* Return the entry found */
    return FreeEntry;
}

/*++
 * @name ExpFindEntryForThread
 *
 *     The ExpFindEntryForThread routine locates the owner entry associated with
 *     the specified thread in the given resource. If none was found, then the
 *     owner table is expanded.
 *
 * @param Resource
 *        Pointer to the resource.
 *
 * @param Thread
 *        Pointer to the thread to find.
 *
 * @param OldIrql
 *        Pointer to current IRQL. TBC: Pointer to in-stack queued spinlock.
 *
 * @return Pointer to an empty OWNER_ENTRY structure.
 *
 * @remarks None.
 *
 *--*/
POWNER_ENTRY
FASTCALL
ExpFindEntryForThread(IN PERESOURCE_XP Resource,
                      IN ERESOURCE_THREAD Thread,
                      IN PKIRQL OldIrql)
{
    POWNER_ENTRY FreeEntry, Owner, Limit;
    ULONG Size;
    DPRINT("ExpFindEntryForThread: %p\n", Resource);

    /* Start by looking in the static array */
    if (Resource->OwnerThreads[0].OwnerThread == Thread)
    {
        /* Found it, return it! */
        return &Resource->OwnerThreads[0];
    }
    else if (Resource->OwnerThreads[1].OwnerThread == Thread)
    {
        /* Return it */
        return &Resource->OwnerThreads[1];
    }
    else
    {
        /* Check if the first array is empty for our use */
        FreeEntry = NULL;
        if (!Resource->OwnerThreads[1].OwnerThread)
        {
            /* Use this as the first free entry */
            FreeEntry = &Resource->OwnerThreads[1];
        }

        /* Get the current table pointer */
        Owner = Resource->OwnerTable;
        if (!Owner)
        {
            /* The current table is empty, so no size */
            Size = 0;
        }
        else
        {
            /* We have a table, get it's size and limit */
            Size = Owner->TableSize;
            Limit = &Owner[Size];

            /* Go to the next entry and loop */
            Owner++;
            do
            {
                /* Check for a match */
                if (Owner->OwnerThread == Thread)
                {
                    /* Match found! Set the resource index */
                    KeGetCurrentThread()->ResourceIndex = 
                        (UCHAR)(Owner - Resource->OwnerTable);
                    return Owner;
                }

                /* If we don't have a free entry yet, make this one free */
                if (!(FreeEntry) && !(Owner->OwnerThread)) FreeEntry = Owner;

                /* Move on */
                Owner++;
            } while (Owner != Limit);
        }
    }

    /* Check if it's OK to do an expansion */
    if (!OldIrql) return NULL;

    /* If we found a free entry by now, return it */
    if (FreeEntry)
    {
        /* Set the resource index */
        KeGetCurrentThread()->ResourceIndex = (UCHAR)
                                              (Owner - Resource->OwnerTable);
        return FreeEntry;
    }

    /* No free entry, expand the table */
    ExpExpandResourceOwnerTable(Resource, OldIrql);
    return NULL;
}

/*++
 * @name ExpBoostOwnerThread
 *
 *     The ExpBoostOwnerThread routine increases the priority of a waiting
 *     thread in an attempt to fight a possible deadlock.
 *
 * @param Thread
 *        Pointer to the current thread.
 *
 * @param OwnerThread
 *        Pointer to thread that owns the resource.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
#if 0

/*
 * Disabled, read the comments bellow.
 */
VOID
FASTCALL
ExpBoostOwnerThread(IN PKTHREAD Thread,
                    IN PKTHREAD OwnerThread)
{
    BOOLEAN Released = FALSE;
    DPRINT("ExpBoostOwnerThread: %p\n", Thread);

    /* Make sure the owner thread is a pointer, not an ID */
    if (!((ULONG_PTR)OwnerThread & 0x3))
    {
        /* Check if we can actually boost it */
        if ((OwnerThread->Priority < Thread->Priority) &&
            (OwnerThread->Priority < 14))
        {
            /* Make sure we're at dispatch */
            ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

            /* Set the new priority */
            OwnerThread->PriorityDecrement += 14 - OwnerThread->Priority;

            /* Update quantum */
            OwnerThread->Quantum = OwnerThread->QuantumReset;

            /* Update the kernel state */
            KiSetPriorityThread(OwnerThread, 14, &Released);

            /* Reacquire lock if it got releases */
            if (Released) KiAcquireDispatcherLockFromDpcLevel();

            /* Make sure we're still at dispatch */
            ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
        }
    }
}
#endif

/*++
 * @name ExpWaitForResource
 *
 *     The ExpWaitForResource routine performs a wait on the specified resource.
 *
 * @param Resource
 *        Pointer to the resource to wait on.
 *
 * @param OwnerThread
 *        Pointer to object (exclusive event or shared semaphore) to wait on.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
FASTCALL
ExpWaitForResource(IN PERESOURCE_XP Resource,
                   IN PVOID Object)
{
#if DBG
    ULONG i;
    ULONG Size;
    KIRQL OldIrql;
    POWNER_ENTRY Owner;
#endif
    ULONG WaitCount = 0;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;

    /* Increase contention count and use a 5 second timeout */
    Resource->ContentionCount++;
    Timeout.QuadPart = 500 * -10000;
    for(;;)
    {
        /* Wait for ownership */
        Status = KeWaitForSingleObject(Object,
                                       WrResource,
                                       KernelMode,
                                       FALSE,
                                       &Timeout);
        if (Status != STATUS_TIMEOUT) break;

        /* Increase wait count */
        WaitCount++;
        Timeout = ExpTimeout;

        /* Check if we've exceeded the limit */
        if (WaitCount > ExpResourceTimeoutCount)
        {
            /* Reset wait count */
            WaitCount = 0;
#if DBG
            /* Lock the resource */
            ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);

            /* Dump debug information */
            DPRINT1("Resource @ %lx\n", Resource);
            DPRINT1(" ActiveCount = %04lx  Flags = %s%s%s\n",
                    Resource->ActiveCount,
                    IsOwnedExclusive(Resource) ? "IsOwnedExclusive " : "",
                    IsSharedWaiting(Resource) ? "SharedWaiter "     : "",
                    IsExclusiveWaiting(Resource) ? "ExclusiveWaiter "  : "");
            DPRINT1(" NumberOfExclusiveWaiters = %04lx\n",
                    Resource->NumberOfExclusiveWaiters);
            DPRINT1("   Thread = %08lx, Count = %02x\n",
                    Resource->OwnerThreads[0].OwnerThread,
                    Resource->OwnerThreads[0].OwnerCount);
            DPRINT1("   Thread = %08lx, Count = %02x\n",
                    Resource->OwnerThreads[1].OwnerThread,
                    Resource->OwnerThreads[1].OwnerCount);

            /* Dump out the table too */
            Owner = Resource->OwnerTable;
            if (Owner)
            {
                /* Loop every entry */
                Size = Owner->TableSize;
                for (i = 1; i < Size; i++)
                {
                    /* Print the data */
                    Owner++;
                    DPRINT1("   Thread = %08lx, Count = %02x\n",
                            Owner->OwnerThread,
                            Owner->OwnerCount);
                }
            }

            /* Break */
            DbgBreakPoint();
            DPRINT1("EX - Rewaiting\n");
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
#endif
        }
#if 0
/* 
 * Disabled because:
 * - We cannot access the OwnerTable without locking the resource.  
 * - The shared waiters may wait also on the semaphore. It makes no sense to boost a waiting thread.  
 * - The thread header is initialized like KeWaitForSingleObject (?, ?, ?, TRUE, ?). 
 *   During the boost, possible the dispatcher lock is released but the thread block (WaitNext) isn't changed. 
 */

        /* Check if we can boost */
        if (!(Resource->Flag & ResourceHasDisabledPriorityBoost))
        {
            PKTHREAD Thread, OwnerThread;

            /* Get the current kernel thread and lock the dispatcher */
            Thread = KeGetCurrentThread();
            Thread->WaitIrql = KiAcquireDispatcherLock();
            Thread->WaitNext = TRUE;

            /* Get the owner thread and boost it */
            OwnerThread = (PKTHREAD)Resource->OwnerThreads[0].OwnerThread;
            if (OwnerThread) ExpBoostOwnerThread(Thread, OwnerThread);

            /* If it's a shared resource */
            if (!IsOwnedExclusive(Resource))
            {
                /* Boost the other owner thread too */
                OwnerThread = (PKTHREAD)Resource->OwnerThreads[1].OwnerThread;
                if (OwnerThread) ExpBoostOwnerThread(Thread, OwnerThread);

                /* Get the table */
                Owner = Resource->OwnerTable;
                if (Owner)
                {
                    /* Loop every entry */
                    Size = Owner->TableSize;
                    for (i = 1; i < Size; i++)
                    {
                        /* Move to next entry */
                        Owner++;

                        /* Get the thread */
                        OwnerThread = (PKTHREAD)Owner->OwnerThread;

                        /* Boost it */
                        if (OwnerThread) ExpBoostOwnerThread(Thread, OwnerThread);
                    }
                }
            }
        }
#endif
    }
}

/* FUNCTIONS *****************************************************************/

/*++
 * @name ExAcquireResourceExclusiveLite
 * @implemented NT4
 *
 *     The ExAcquireResourceExclusiveLite routine acquires the given resource
 *     for exclusive access by the calling thread.
 *
 * @param Resource
 *        Pointer to the resource to acquire.
 *
 * @param Wait
 *        Specifies the routine's behavior whenever the resource cannot be
 *        acquired immediately.
 *
 * @return TRUE if the resource is acquired. FALSE if the input Wait is FALSE
 *         and exclusive access cannot be granted immediately.
 *
 * @remarks The caller can release the resource by calling either
 *          ExReleaseResourceLite or ExReleaseResourceForThreadLite.
 *
 *          Normal kernel APC delivery must be disabled before calling this
 *          routine. Disable normal kernel APC delivery by calling
 *          KeEnterCriticalRegion. Delivery must remain disabled until the
 *          resource is released, at which point it can be reenabled by calling
 *          KeLeaveCriticalRegion.
 *
 *          For better performance, call ExTryToAcquireResourceExclusiveLite,
 *          rather than calling ExAcquireResourceExclusiveLite with Wait set
 *          to FALSE.
 *
 *          Callers of ExAcquireResourceExclusiveLite must be running at IRQL <
 *          DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN
NTAPI
ExAcquireResourceExclusiveLite(PERESOURCE resource,
                               BOOLEAN Wait)
{
    KIRQL OldIrql = PASSIVE_LEVEL;
    ERESOURCE_THREAD Thread;
    BOOLEAN Success;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Sanity check */
    ASSERT((Resource->Flag & ResourceNeverExclusive) == 0);

    /* Get the thread */
    Thread = ExGetCurrentResourceThread();

    /* Sanity check and validation */
    ASSERT(KeIsExecutingDpc() == FALSE);
    ExpVerifyResource(Resource);

    /* Acquire the lock */
    ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);
    ExpCheckForApcsDisabled(TRUE, Resource, (PETHREAD)Thread);

    /* Check if there is a shared owner or exclusive owner */
TryAcquire:
    DPRINT("ExAcquireResourceExclusiveLite(Resource 0x%p, Wait %d)\n",
           Resource, Wait);
    if (Resource->ActiveCount)
    {
        /* Check if it's exclusively owned, and we own it */
        if ((IsOwnedExclusive(Resource)) &&
            (Resource->OwnerThreads[0].OwnerThread == Thread))
        {
            /* Increase the owning count */
            Resource->OwnerThreads[0].OwnerCount++;
            Success = TRUE;
        }
        else
        {
            /*
             * If the caller doesn't want us to wait, we can't acquire the
             * resource because someone else then us owns it. If we can wait,
             * then we'll wait.
             */
            if (!Wait)
            {
                Success = FALSE;
            }
            else
            {
                /* Check if it has exclusive waiters */
                if (!Resource->ExclusiveWaiters)
                {
                    /* It doesn't, allocate the event and try acquiring again */
                    ExpAllocateExclusiveWaiterEvent(Resource, &OldIrql);
                    goto TryAcquire;
                }
                else
                {
                    /* Has exclusive waiters, wait on it */
                    Resource->NumberOfExclusiveWaiters++;
                    ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
                    ExpWaitForResource(Resource, Resource->ExclusiveWaiters);

                    /* Set owner and return success */
                    Resource->OwnerThreads[0].OwnerThread = Thread;
                    return TRUE;
                }
            }
        }
    }
    else
    {
        /* Nobody owns it, so let's! */
        Resource->Flag |= ResourceOwnedExclusive;
        Resource->ActiveCount = 1;
        Resource->OwnerThreads[0].OwnerThread = Thread;
        Resource->OwnerThreads[0].OwnerCount = 1;
        Success = TRUE;
    }

    /* Release the lock and return */
    ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
    return Success;
}

/*++
 * @name ExAcquireResourceSharedLite
 * @implemented NT4
 *
 *     The ExAcquireResourceSharedLite routine acquires the given resource
 *     for shared access by the calling thread.
 *
 * @param Resource
 *        Pointer to the resource to acquire.
 *
 * @param Wait
 *        Specifies the routine's behavior whenever the resource cannot be
 *        acquired immediately.
 *
 * @return TRUE if the resource is acquired. FALSE if the input Wait is FALSE
 *         and exclusive access cannot be granted immediately.
 *
 * @remarks The caller can release the resource by calling either
 *          ExReleaseResourceLite or ExReleaseResourceForThreadLite.
 *
 *          Normal kernel APC delivery must be disabled before calling this
 *          routine. Disable normal kernel APC delivery by calling
 *          KeEnterCriticalRegion. Delivery must remain disabled until the
 *          resource is released, at which point it can be reenabled by calling
 *          KeLeaveCriticalRegion.
 *
 *          Callers of ExAcquireResourceExclusiveLite must be running at IRQL <
 *          DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN
NTAPI
ExAcquireResourceSharedLite(PERESOURCE resource,
                            BOOLEAN Wait)
{
    KIRQL OldIrql;
    ERESOURCE_THREAD Thread;
    POWNER_ENTRY Owner;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Get the thread */
    Thread = ExGetCurrentResourceThread();

    /* Sanity check and validation */
    ASSERT(KeIsExecutingDpc() == FALSE);
    ExpVerifyResource(Resource);

    /* Acquire the lock */
    ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);
    ExpCheckForApcsDisabled(TRUE, Resource, (PETHREAD)Thread);

    /* See if nobody owns us */
TryAcquire:
    DPRINT("ExAcquireResourceSharedLite(Resource 0x%p, Wait %d)\n",
            Resource, Wait);
    if (!Resource->ActiveCount)
    {
        if (Resource->NumberOfSharedWaiters == 0)
        {
           Owner = &Resource->OwnerThreads[1];
        }
        else
        {
           /* Find a free entry */
           Owner = ExpFindFreeEntry(Resource, &OldIrql);
           if (!Owner) goto TryAcquire;
        }

        Owner->OwnerThread = Thread;
        Owner->OwnerCount = 1;
        Resource->ActiveCount = 1;

        /* Release the lock and return */
        ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
        return TRUE;
    }

    /* Check if it's exclusively owned */
    if (IsOwnedExclusive(Resource))
    {
        /* Check if we own it */
        if (Resource->OwnerThreads[0].OwnerThread == Thread)
        {
            /* Increase the owning count */
            Resource->OwnerThreads[0].OwnerCount++;

            /* Release the lock and return */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return TRUE;
        }

        /* Find a free entry */
        Owner = ExpFindFreeEntry(Resource, &OldIrql);
        if (!Owner) goto TryAcquire;
    }
    else
    {
        /* Resource is shared, find who owns it */
        Owner = ExpFindEntryForThread(Resource, Thread, &OldIrql);
        if (!Owner) goto TryAcquire;

        /* Is it us? */
        if (Owner->OwnerThread == Thread)
        {
            /* Increase acquire count and return */
            Owner->OwnerCount++;
            ASSERT(Owner->OwnerCount != 0);

            /* Release the lock and return */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return TRUE;
        }

        /* Try to find if there are exclusive waiters */
        if (!IsExclusiveWaiting(Resource))
        {
            /* There are none, so acquire it */
            Owner->OwnerThread = Thread;
            Owner->OwnerCount = 1;
            Resource->ActiveCount++;

            /* Release the lock and return */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return TRUE;
        }
    }

    /* If we got here, then we need to wait. Are we allowed? */
    if (!Wait)
    {
        /* Release the lock and return */
        ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
        return FALSE;
    }

    /* Check if we have a shared waiters semaphore */
    if (!Resource->SharedWaiters)
    {
        /* Allocate it and try another acquire */
        ExpAllocateSharedWaiterSemaphore(Resource, &OldIrql);
        goto TryAcquire;
    }

    /* Now wait for the resource */
    Owner->OwnerThread = Thread;
    Owner->OwnerCount = 1;
    Resource->NumberOfSharedWaiters++;

    /* Release the lock and return */
    ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
    ExpWaitForResource(Resource, Resource->SharedWaiters);
    return TRUE;
}

/*++
 * @name ExAcquireSharedStarveExclusive
 * @implemented NT4
 *
 *     The ExAcquireSharedStarveExclusive routine acquires the given resource
 *     shared access without waiting for any pending attempts to acquire
 *     exclusive access to the same resource.
 *
 * @param Resource
 *        Pointer to the resource to acquire.
 *
 * @param Wait
 *        Specifies the routine's behavior whenever the resource cannot be
 *        acquired immediately.
 *
 * @return TRUE if the resource is acquired. FALSE if the input Wait is FALSE
 *         and exclusive access cannot be granted immediately.
 *
 * @remarks The caller can release the resource by calling either
 *          ExReleaseResourceLite or ExReleaseResourceForThreadLite.
 *
 *          Normal kernel APC delivery must be disabled before calling this
 *          routine. Disable normal kernel APC delivery by calling
 *          KeEnterCriticalRegion. Delivery must remain disabled until the
 *          resource is released, at which point it can be reenabled by calling
 *          KeLeaveCriticalRegion.
 *
 *          Callers of ExAcquireSharedStarveExclusive usually need quick access
 *          to a shared resource in order to save an exclusive accessor from
 *          doing redundant work. For example, a file system might call this
 *          routine to modify a cached resource, such as a BCB pinned in the
 *          cache, before the Cache Manager can acquire exclusive access to the
 *          resource and write the cache out to disk.
 *
 *          Callers of ExAcquireResourceExclusiveLite must be running at IRQL <
 *          DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN
NTAPI
ExAcquireSharedStarveExclusive(PERESOURCE resource,
                               BOOLEAN Wait)
{
    KIRQL OldIrql;
    ERESOURCE_THREAD Thread;
    POWNER_ENTRY Owner;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Get the thread */
    Thread = ExGetCurrentResourceThread();

    /* Sanity check and validation */
    ASSERT(KeIsExecutingDpc() == FALSE);
    ExpVerifyResource(Resource);

    /* Acquire the lock */
    ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);

    /* See if nobody owns us */
TryAcquire:
    DPRINT("ExAcquireSharedStarveExclusive(Resource 0x%p, Wait %d)\n",
            Resource, Wait);
    if (!Resource->ActiveCount)
    {
        /* Nobody owns it, so let's take control */
        Resource->ActiveCount = 1;
        Resource->OwnerThreads[1].OwnerThread = Thread;
        Resource->OwnerThreads[1].OwnerCount = 1;

        /* Release the lock and return */
        ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
        return TRUE;
    }

    /* Check if it's exclusively owned */
    if (IsOwnedExclusive(Resource))
    {
        /* Check if we own it */
        if (Resource->OwnerThreads[0].OwnerThread == Thread)
        {
            /* Increase the owning count */
            Resource->OwnerThreads[0].OwnerCount++;

            /* Release the lock and return */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return TRUE;
        }

        /* Find a free entry */
        Owner = ExpFindFreeEntry(Resource, &OldIrql);
        if (!Owner) goto TryAcquire;

        /* If we got here, then we need to wait. Are we allowed? */
        if (!Wait)
        {
            /* Release the lock and return */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return TRUE;
        }

        /* Check if we have a shared waiters semaphore */
        if (!Resource->SharedWaiters)
        {
            /* Allocate one and try again */
            ExpAllocateSharedWaiterSemaphore(Resource, &OldIrql);
            goto TryAcquire;
        }
    }
    else
    {
        /* Resource is shared, find who owns it */
        Owner = ExpFindEntryForThread(Resource, Thread, &OldIrql);
        if (!Owner) goto TryAcquire;

        /* Is it us? */
        if (Owner->OwnerThread == Thread)
        {
            /* Increase acquire count and return */
            Owner->OwnerCount++;
            ASSERT(Owner->OwnerCount != 0);

            /* Release the lock and return */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return TRUE;
        }

        /* Acquire it */
        Owner->OwnerThread = Thread;
        Owner->OwnerCount = 1;
        Resource->ActiveCount++;

        /* Release the lock and return */
        ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
        return TRUE;
    }

    /* If we got here, then we need to wait. Are we allowed? */
    if (!Wait)
    {
        /* Release the lock and return */
        ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
        return TRUE;
    }

    /* Now wait for the resource */
    Owner->OwnerThread = Thread;
    Owner->OwnerCount = 1;
    Resource->NumberOfSharedWaiters++;

    /* Release the lock and return */
    ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
    ExpWaitForResource(Resource, Resource->SharedWaiters);
    return TRUE;
}

/*++
 * @name ExAcquireSharedWaitForExclusive
 * @implemented NT4
 *
 *     The ExAcquireSharedWaitForExclusive routine acquires the given resource
 *     for shared access if shared access can be granted and there are no
 *     exclusive waiters.
 *
 * @param Resource
 *        Pointer to the resource to acquire.
 *
 * @param Wait
 *        Specifies the routine's behavior whenever the resource cannot be
 *        acquired immediately.
 *
 * @return TRUE if the resource is acquired. FALSE if the input Wait is FALSE
 *         and exclusive access cannot be granted immediately.
 *
 * @remarks The caller can release the resource by calling either
 *          ExReleaseResourceLite or ExReleaseResourceForThreadLite.
 *
 *          Normal kernel APC delivery must be disabled before calling this
 *          routine. Disable normal kernel APC delivery by calling
 *          KeEnterCriticalRegion. Delivery must remain disabled until the
 *          resource is released, at which point it can be reenabled by calling
 *          KeLeaveCriticalRegion.
 *
 *          Callers of ExAcquireResourceExclusiveLite must be running at IRQL <
 *          DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN
NTAPI
ExAcquireSharedWaitForExclusive(PERESOURCE resource,
                                BOOLEAN Wait)
{
    KIRQL OldIrql;
    ERESOURCE_THREAD Thread;
    POWNER_ENTRY Owner;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Get the thread */
    Thread = ExGetCurrentResourceThread();

    /* Sanity check and validation */
    ASSERT(KeIsExecutingDpc() == FALSE);
    ExpVerifyResource(Resource);

    /* Acquire the lock */
    ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);

    /* See if nobody owns us */
TryAcquire:
    DPRINT("ExAcquireSharedWaitForExclusive(Resource 0x%p, Wait %d)\n",
            Resource, Wait);
    if (!Resource->ActiveCount)
    {
        /* Nobody owns it, so let's take control */
        Resource->ActiveCount = 1;
        Resource->OwnerThreads[1].OwnerThread = Thread;
        Resource->OwnerThreads[1].OwnerCount = 1;

        /* Release the lock and return */
        ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
        return TRUE;
    }

    /* Check if it's exclusively owned */
    if (IsOwnedExclusive(Resource))
    {
        /* Check if we own it */
        if (Resource->OwnerThreads[0].OwnerThread == Thread)
        {
            /* Increase the owning count */
            Resource->OwnerThreads[0].OwnerCount++;

            /* Release the lock and return */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return TRUE;
        }
        else
        {
            /* Find a free entry */
            Owner = ExpFindFreeEntry(Resource, &OldIrql);
            if (!Owner) goto TryAcquire;

            /* If we got here, then we need to wait. Are we allowed? */
            if (!Wait)
            {
                /* Release the lock and return */
                ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
                return TRUE;
            }

            /* Check if we have a shared waiters semaphore */
            if (!Resource->SharedWaiters)
            {
                /* Allocate one and try again */
                ExpAllocateSharedWaiterSemaphore(Resource, &OldIrql);
                goto TryAcquire;
            }

            /* Now take control of the resource */
            Owner->OwnerThread = Thread;
            Owner->OwnerCount = 1;
            Resource->NumberOfSharedWaiters++;

            /* Release the lock and wait for it to be ours */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            ExpWaitForResource(Resource, Resource->SharedWaiters);
            return TRUE;
        }
    }
    else
    {
        /* Try to find if there are exclusive waiters */
        if (IsExclusiveWaiting(Resource))
        {
            /* We have to wait for the exclusive waiter to be done */
            if (!Wait)
            {
                /* So bail out if we're not allowed */
                ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
                return TRUE;
            }

            /* Check if we have a shared waiters semaphore */
            if (!Resource->SharedWaiters)
            {
                /* Allocate one and try again */
                ExpAllocateSharedWaiterSemaphore(Resource, &OldIrql);
                goto TryAcquire;
            }

            /* Now wait for the resource */
            Resource->NumberOfSharedWaiters++;
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            ExpWaitForResource(Resource, Resource->SharedWaiters);

            /* Get the lock back */
            ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);

            /* Find who owns it now */
            Owner = ExpFindEntryForThread(Resource, Thread, &OldIrql);

            /* Sanity checks */
            ASSERT(IsOwnedExclusive(Resource) == FALSE);
            ASSERT(Resource->ActiveCount > 0);
            ASSERT(Owner->OwnerThread != Thread);

            /* Take control */
            Owner->OwnerThread = Thread;
            Owner->OwnerCount = 1;

            /* Release the lock and return */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return TRUE;
        }
        else
        {
            /* Resource is shared, find who owns it */
            Owner = ExpFindEntryForThread(Resource, Thread, &OldIrql);
            if (!Owner) goto TryAcquire;

            /* Is it us? */
            if (Owner->OwnerThread == Thread)
            {
                /* Increase acquire count and return */
                Owner->OwnerCount++;
                ASSERT(Owner->OwnerCount != 0);

                /* Release the lock and return */
                ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
                return TRUE;
            }

            /* No exclusive waiters, so acquire it */
            Owner->OwnerThread = Thread;
            Owner->OwnerCount = 1;
            Resource->ActiveCount++;

            /* Release the lock and return */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return TRUE;
        }
    }
}

/*++
 * @name ExConvertExclusiveToSharedLite
 * @implemented NT4
 *
 *     The ExConvertExclusiveToSharedLite routine converts an exclusively
 *     acquired resource into a resource that can be acquired shared.
 *
 * @param Resource
 *        Pointer to the resource to convert.
 *
 * @return None.
 *
 * @remarks Callers of ExConvertExclusiveToSharedLite must be running at IRQL <
 *          DISPATCH_LEVEL.
 *
 *--*/
VOID
NTAPI
ExConvertExclusiveToSharedLite(PERESOURCE resource)
{
    ULONG OldWaiters;
    KIRQL OldIrql;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;
    DPRINT("ExConvertExclusiveToSharedLite(Resource 0x%p)\n", Resource);

    /* Lock the resource */
    ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);

    /* Sanity checks */
    ASSERT(KeIsExecutingDpc() == FALSE);
    ExpVerifyResource(Resource);
    ASSERT(IsOwnedExclusive(Resource));
    ASSERT(Resource->OwnerThreads[0].OwnerThread == (ERESOURCE_THREAD)PsGetCurrentThread());

    /* Erase the exclusive flag */
    Resource->Flag &= ~ResourceOwnedExclusive;

    /* Check if we have shared waiters */
    OldWaiters = Resource->NumberOfSharedWaiters;
    if (OldWaiters)
    {
        /* Make the waiters active owners */
        Resource->ActiveCount = Resource->ActiveCount + (USHORT)OldWaiters;
        Resource->NumberOfSharedWaiters = 0;

        /* Release lock and wake the waiters */
        ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
        KeReleaseSemaphore(Resource->SharedWaiters, 0, OldWaiters, FALSE);
    }
    else
    {
        /* Release lock */
        ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
    }
}

/*++
 * @name ExDeleteResourceLite
 * @implemented NT4
 *
 *     The ExConvertExclusiveToSharedLite routine deletes a given resource
 *     from the system’s resource list.
 *
 * @param Resource
 *        Pointer to the resource to delete.
 *
 * @return STATUS_SUCCESS if the resource was deleted.
 *
 * @remarks Callers of ExDeleteResourceLite must be running at IRQL <
 *          DISPATCH_LEVEL.
 *
 *--*/
NTSTATUS
NTAPI
ExDeleteResourceLite(PERESOURCE resource)
{
    KIRQL OldIrql;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;
    DPRINT("ExDeleteResourceLite(Resource 0x%p)\n", Resource);

    /* Sanity checks */
    ASSERT(IsSharedWaiting(Resource) == FALSE);
    ASSERT(IsExclusiveWaiting(Resource) == FALSE);
    ASSERT(KeIsExecutingDpc() == FALSE);
    ExpVerifyResource(Resource);

    /* Lock the resource */
    KeAcquireSpinLock(&ExpResourceSpinLock, &OldIrql);

    /* Remove the resource */
    RemoveEntryList(&Resource->SystemResourcesList);

    /* Release the lock */
    KeReleaseSpinLock(&ExpResourceSpinLock, OldIrql);

    /* Free every  structure */
    if (Resource->OwnerTable) ExFreePool(Resource->OwnerTable);
    if (Resource->SharedWaiters) ExFreePool(Resource->SharedWaiters);
    if (Resource->ExclusiveWaiters) ExFreePool(Resource->ExclusiveWaiters);

    /* Return success */
    return STATUS_SUCCESS;
}

/*++
 * @name ExDisableResourceBoostLite
 * @implemented NT4
 *
 *     The ExDisableResourceBoostLite routine disables thread boosting for
 *     the given resource.
 *
 * @param Resource
 *        Pointer to the resource whose thread boosting will be disabled.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
ExDisableResourceBoostLite(PERESOURCE resource)
{
    KIRQL OldIrql;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Sanity check */
    ExpVerifyResource(Resource);

    /* Lock the resource */
    ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);

    /* Remove the flag */
    Resource->Flag |= ResourceHasDisabledPriorityBoost;

    /* Release the lock */
    ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
}

/*++
 * @name ExGetExclusiveWaiterCount
 * @implemented NT4
 *
 *     The ExGetExclusiveWaiterCount routine returns the number of exclusive
 *     waiters for the given resource.
 *
 * @param Resource
 *        Pointer to the resource to check.
 *
 * @return The number of exclusive waiters.
 *
 * @remarks None.
 *
 *--*/
ULONG
NTAPI
ExGetExclusiveWaiterCount(PERESOURCE resource)
{
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Return the count */
    return Resource->NumberOfExclusiveWaiters;
}

/*++
 * @name ExGetSharedWaiterCount
 * @implemented NT4
 *
 *     The ExGetSharedWaiterCount routine returns the number of shared
 *     waiters for the given resource.
 *
 * @param Resource
 *        Pointer to the resource to check.
 *
 * @return The number of shared waiters.
 *
 * @remarks None.
 *
 *--*/
ULONG
NTAPI
ExGetSharedWaiterCount(PERESOURCE resource)
{
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Return the count */
    return Resource->NumberOfSharedWaiters;
}

/*++
 * @name ExInitializeResourceLite
 * @implemented NT4
 *
 *     The ExInitializeResourceLite routine initializes a resource variable.
 *
 * @param Resource
 *        Pointer to the resource to check.
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks The storage for ERESOURCE must not be allocated from paged pool.
 *
 *          The storage must be 8-byte aligned.
 *
 *--*/
NTSTATUS
NTAPI
ExInitializeResourceLite(PERESOURCE resource)
{
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;
    DPRINT("ExInitializeResourceLite(Resource 0x%p)\n", Resource);

    /* Clear the structure */
    RtlZeroMemory(Resource, sizeof(ERESOURCE_XP));

    /* Initialize the lock */
    KeInitializeSpinLock(&Resource->SpinLock);

    /* Add it into the system list */
    ExInterlockedInsertTailList(&ExpSystemResourcesList,
                                &Resource->SystemResourcesList,
                                &ExpResourceSpinLock);

    /* Return success */
    return STATUS_SUCCESS;
}

/*++
 * @name ExIsResourceAcquiredExclusiveLite
 * @implemented NT4
 *
 *     The ExIsResourceAcquiredExclusiveLite routine returns whether the
 *     current thread has exclusive access to a given resource.
 *
 * @param Resource
 *        Pointer to the resource to check.
 *
 * @return TRUE if the caller already has exclusive access to the given resource.
 *
 * @remarks Callers of ExIsResourceAcquiredExclusiveLite must be running at
 *          IRQL <= DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN
NTAPI
ExIsResourceAcquiredExclusiveLite(PERESOURCE resource)
{
    ERESOURCE_THREAD Thread = ExGetCurrentResourceThread();
    BOOLEAN IsAcquired = FALSE;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Sanity check */
    ExpVerifyResource(Resource);

    /* Check if it's exclusively acquired */
    if ((IsOwnedExclusive(Resource)) &&
        (Resource->OwnerThreads[0].OwnerThread == Thread))
    {
        /* It is acquired */
        IsAcquired = TRUE;
    }

    /* Return if it's acquired */
    return IsAcquired;
}

/*++
 * @name ExIsResourceAcquiredSharedLite
 * @implemented NT4
 *
 *     The ExIsResourceAcquiredSharedLite routine returns whether the
 *     current thread has has access (either shared or exclusive) to a
 *     given resource.
 *
 * @param Resource
 *        Pointer to the resource to check.
 *
 * @return Number of times the caller has acquired the given resource for
 *         shared or exclusive access.
 *
 * @remarks Callers of ExIsResourceAcquiredExclusiveLite must be running at
 *          IRQL <= DISPATCH_LEVEL.
 *
 *--*/
ULONG
NTAPI
ExIsResourceAcquiredSharedLite(IN PERESOURCE resource)
{
    ERESOURCE_THREAD Thread;
    ULONG i, Size;
    ULONG Count = 0;
    KIRQL OldIrql;
    POWNER_ENTRY Owner;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Sanity check */
    ExpVerifyResource(Resource);

    /* Get the thread */
    Thread = ExGetCurrentResourceThread();

    /* Check if we are in the thread list */
    if (Resource->OwnerThreads[0].OwnerThread == Thread)
    {
        /* Found it, return count */
        Count = Resource->OwnerThreads[0].OwnerCount;
    }
    else if (Resource->OwnerThreads[1].OwnerThread == Thread)
    {
        /* Found it on the second list, return count */
        Count = Resource->OwnerThreads[1].OwnerCount;
    }
    else
    {
        /* Not in the list, do a full table look up */
        Owner = Resource->OwnerTable;
        if (Owner)
        {
            /* Lock the resource */
            ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);

            /* Get the resource index */
            i = ((PKTHREAD)Thread)->ResourceIndex;
            Size = Owner->TableSize;

            /* Check if the index is valid and check if we don't match */
            if ((i >= Size) || (Owner[i].OwnerThread != Thread))
            {
                /* Sh*t! We need to do a full search */
                for (i = 1; i < Size; i++)
                {
                    /* Move to next owner */
                    Owner++;

                    /* Try to find a match */
                    if (Owner->OwnerThread == Thread)
                    {
                        /* Finally! */
                        Count = Owner->OwnerCount;
                        break;
                    }
                }
            }
            else
            {
                /* We found the match directlry */
                Count = Owner[i].OwnerCount;
            }

            /* Release the lock */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
        }
    }

    /* Return count */
    return Count;
}

/*++
 * @name ExReinitializeResourceLite
 * @implemented NT4
 *
 *     The ExReinitializeResourceLite routine routine reinitializes
 *     an existing resource variable.
 *
 * @param Resource
 *        Pointer to the resource to be reinitialized.
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks With a single call to ExReinitializeResource, a driver writer can
 *          replace three calls: one to ExDeleteResourceLite, another to
 *          ExAllocatePool, and a third to ExInitializeResourceLite. As
 *          contention for a resource variable increases, memory is dynamically
 *          allocated and attached to the resource in order to track this
 *          contention. As an optimization, ExReinitializeResourceLite retains
 *          and zeroes this previously allocated memory.
 *
 *          Callers of ExReinitializeResourceLite must be running at
 *          IRQL <= DISPATCH_LEVEL.
 *
 *--*/
NTSTATUS
NTAPI
ExReinitializeResourceLite(PERESOURCE resource)
{
    PKEVENT Event;
    PKSEMAPHORE Semaphore;
    ULONG i, Size;
    POWNER_ENTRY Owner;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Get the owner table */
    Owner = Resource->OwnerTable;
    if (Owner)
    {
        /* Get the size and loop it */
        Size = Owner->TableSize;
        for (i = 0; i < Size; i++)
        {
            /* Zero the table */
            Owner[i].OwnerThread = 0;
            Owner[i].OwnerCount = 0;
        }
    }

    /* Zero the flags and count */
    Resource->Flag = 0;
    Resource->ActiveCount = 0;

    /* Reset the semaphore */
    Semaphore = Resource->SharedWaiters;
    if (Semaphore) KeInitializeSemaphore(Semaphore, 0, MAXLONG);

    /* Reset the event */
    Event = Resource->ExclusiveWaiters;
    if (Event) KeInitializeEvent(Event, SynchronizationEvent, FALSE);

    /* Clear the resource data */
    Resource->OwnerThreads[0].OwnerThread = 0;
    Resource->OwnerThreads[1].OwnerThread = 0;
    Resource->OwnerThreads[0].OwnerCount = 0;
    Resource->OwnerThreads[1].OwnerCount = 0;
    Resource->ContentionCount = 0;
    Resource->NumberOfSharedWaiters = 0;
    Resource->NumberOfExclusiveWaiters = 0;

    /* Reset the spinlock */
    KeInitializeSpinLock(&Resource->SpinLock);
    return STATUS_SUCCESS;
}

/*++
 * @name ExReleaseResourceLite
 * @implemented NT4
 *
 *     The ExReleaseResourceLite routine routine releases
 *     a specified executive resource owned by the current thread.
 *
 * @param Resource
 *        Pointer to the resource to be released.
 *
 * @return None.
 *
 * @remarks Callers of ExReleaseResourceLite must be running at
 *          IRQL <= DISPATCH_LEVEL.
 *
 *--*/
VOID
FASTCALL
ExReleaseResourceLite(PERESOURCE resource)
{
    ERESOURCE_THREAD Thread;
    ULONG Count, i;
    KIRQL OldIrql;
    POWNER_ENTRY Owner, Limit;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;
    DPRINT("ExReleaseResourceLite: %p\n", Resource);

    /* Sanity check */
    ExpVerifyResource(Resource);

    /* Get the thread and lock the resource */
    Thread = ExGetCurrentResourceThread();
    ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);
    ExpCheckForApcsDisabled(TRUE, Resource, (PETHREAD)Thread);

    /* Check if it's exclusively owned */
    if (IsOwnedExclusive(Resource))
    {
        /* Decrement owner count and check if we're done */
        ASSERT(Resource->OwnerThreads[0].OwnerCount > 0);
        ASSERT(Resource->ActiveCount == 1);
        if (--Resource->OwnerThreads[0].OwnerCount)
        {
            /* Done, release lock! */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return;
        }

        /* Clear the owner */
        Resource->OwnerThreads[0].OwnerThread = 0;

        /* Check if there are shared waiters */
        if (IsSharedWaiting(Resource))
        {
            /* Remove the exclusive flag */
            Resource->Flag &= ~ResourceOwnedExclusive;

            /* Give ownage to another thread */
            Count = Resource->NumberOfSharedWaiters;
            Resource->ActiveCount = (USHORT)Count;
            Resource->NumberOfSharedWaiters = 0;

            /* Release lock and let someone else have it */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            KeReleaseSemaphore(Resource->SharedWaiters, 0, Count, FALSE);
            return;
         }
         else if (IsExclusiveWaiting(Resource))
         {
            /* Give exclusive access */
            Resource->OwnerThreads[0].OwnerThread = 1;
            Resource->OwnerThreads[0].OwnerCount = 1;
            Resource->ActiveCount = 1;
            Resource->NumberOfExclusiveWaiters--;

            /* Release the lock and give it away */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            KeSetEventBoostPriority(Resource->ExclusiveWaiters,
                                    (PKTHREAD*)
                                    &Resource->OwnerThreads[0].OwnerThread);
            return;
         }

         /* Remove the exclusive flag */
         Resource->Flag &= ~ResourceOwnedExclusive;

         Resource->ActiveCount = 0;
    }
    else
    {
        /* Check if we are in the thread list */
        if (Resource->OwnerThreads[1].OwnerThread == Thread)
        {
            /* Found it, get owner */
            Owner = &Resource->OwnerThreads[1];
        }
        else
        {
            /* Not in the list, do a full table look up */
            i = ((PKTHREAD)Thread)->ResourceIndex;
            Owner = Resource->OwnerTable;
            if (!Owner)
            {
                /* Nobody owns us, bugcheck! */
                KEBUGCHECKEX(RESOURCE_NOT_OWNED,
                             (ULONG_PTR)Resource,
                             Thread,
                             (ULONG_PTR)Resource->OwnerTable,
                             (ULONG_PTR)2);
            }

            /* Check if we're out of the size and don't match */
            if ((i >= Owner->TableSize) || (Owner[i].OwnerThread != Thread))
            {
                /* Get the last entry */
                Limit = &Owner[Owner->TableSize];
                for (;;)
                {
                    /* Move to the next entry */
                    Owner++;

                    /* Check if we don't match */
                    if (Owner >= Limit)
                    {
                        /* Nobody owns us, bugcheck! */
                        KEBUGCHECKEX(RESOURCE_NOT_OWNED,
                                     (ULONG_PTR)Resource,
                                     Thread,
                                     (ULONG_PTR)Resource->OwnerTable,
                                     2);
                    }

                    /* Check for a match */
                    if (Owner->OwnerThread == Thread) break;
                }
            }
            else
            {
                /* Get the entry directly */
                Owner = &Owner[i];
            }
        }

        /* Sanity checks */
        ASSERT(Owner->OwnerThread == Thread);
        ASSERT(Owner->OwnerCount > 0);

        /* Check if we are the last owner */
        if (--Owner->OwnerCount)
        {
            /* Release lock */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return;
        }

        /* Clear owner */
        Owner->OwnerThread = 0;

        /* See if the resource isn't being owned anymore */
        ASSERT(Resource->ActiveCount > 0);
        if (!(--Resource->ActiveCount))
        {
            /* Check if there's an exclusive waiter */
            if (IsExclusiveWaiting(Resource))
            {
                /* Give exclusive access */
                Resource->Flag |= ResourceOwnedExclusive;
                Resource->OwnerThreads[0].OwnerThread = 1;
                Resource->OwnerThreads[0].OwnerCount = 1;
                Resource->ActiveCount = 1;
                Resource->NumberOfExclusiveWaiters--;

                /* Release the lock and give it away */
                ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
                KeSetEventBoostPriority(Resource->ExclusiveWaiters,
                                        (PKTHREAD*)
                                        &Resource->OwnerThreads[0].OwnerThread);
                return;
            }
        }
    }

    /* Release lock */
    ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
    return;
}

/*++
 * @name ExReleaseResourceForThreadLite
 * @implemented NT4
 *
 *     The ExReleaseResourceForThreadLite routine routine releases
 *     the input resource of the indicated thread.
 *
 * @param Resource
 *        Pointer to the resource to be released.
 *
 * @param Thread
 *        Identifies the thread that originally acquired the resource.
  *
 * @return None.
 *
 * @remarks Callers of ExReleaseResourceForThreadLite must be running at
 *          IRQL <= DISPATCH_LEVEL.
 *
 *--*/
VOID
NTAPI
ExReleaseResourceForThreadLite(PERESOURCE resource,
                               ERESOURCE_THREAD Thread)
{
    ULONG i;
    ULONG Count;
    KIRQL OldIrql;
    POWNER_ENTRY Owner;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;
    ASSERT(Thread != 0);
    DPRINT("ExReleaseResourceForThreadLite: %p\n", Resource);

    /* Get the thread and lock the resource */
    ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);

    /* Sanity check */
    ExpVerifyResource(Resource);

    /* Check if it's exclusively owned */
    if (IsOwnedExclusive(Resource))
    {
        /* Decrement owner count and check if we're done */
        ASSERT(Resource->OwnerThreads[0].OwnerThread == Thread);
        ASSERT(Resource->OwnerThreads[0].OwnerCount > 0);
        if (--Resource->OwnerThreads[0].OwnerCount)
        {
            /* Done, release lock! */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return;
        }

        /* Clear the owner */
        Resource->OwnerThreads[0].OwnerThread = 0;

        /* See if the resource isn't being owned anymore */
        ASSERT(Resource->ActiveCount > 0);
        if (!(--Resource->ActiveCount))
        {
            /* Check if there are shared waiters */
            if (IsSharedWaiting(Resource))
            {
                /* Remove the exclusive flag */
                Resource->Flag &= ~ResourceOwnedExclusive;

                /* Give ownage to another thread */
                Count = Resource->NumberOfSharedWaiters;
                Resource->ActiveCount = (USHORT)Count;
                Resource->NumberOfSharedWaiters = 0;

                /* Release lock and let someone else have it */
                ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
                KeReleaseSemaphore(Resource->SharedWaiters, 0, Count, FALSE);
                return;
            }
            else if (IsExclusiveWaiting(Resource))
            {
                /* Give exclusive access */
                Resource->OwnerThreads[0].OwnerThread = 1;
                Resource->OwnerThreads[0].OwnerCount = 1;
                Resource->ActiveCount = 1;
                Resource->NumberOfExclusiveWaiters--;

                /* Release the lock and give it away */
                ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
                KeSetEventBoostPriority(Resource->ExclusiveWaiters,
                                        (PKTHREAD*)
                                        &Resource->OwnerThreads[0].OwnerThread);
                return;
            }

            /* Remove the exclusive flag */
            Resource->Flag &= ~ResourceOwnedExclusive;
        }
    }
    else
    {
        /* Check if we are in the thread list */
        if (Resource->OwnerThreads[0].OwnerThread == Thread)
        {
            /* Found it, get owner */
            Owner = &Resource->OwnerThreads[0];
        }
        else if (Resource->OwnerThreads[1].OwnerThread == Thread)
        {
            /* Found it on the second list, get owner */
            Owner = &Resource->OwnerThreads[1];
        }
        else
        {
            /* Assume no valid index */
            i = 1;

            /* If we got a valid pointer, try to get the resource index */
            if (!((ULONG)Thread & 3)) i = ((PKTHREAD)Thread)->ResourceIndex;

            /* Do a table lookup */
            Owner = Resource->OwnerTable;
            ASSERT(Owner != NULL);

            /* Check if we're out of the size and don't match */
            if ((i >= Owner->TableSize) || (Owner[i].OwnerThread != Thread))
            {
                /* Get the last entry */
                for (;;)
                {
                    /* Move to the next entry */
                    Owner++;

                    /* Check for a match */
                    if (Owner->OwnerThread == Thread) break;
                }
            }
            else
            {
                /* Get the entry directly */
                Owner = &Owner[i];
            }
        }

        /* Sanity checks */
        ASSERT(Owner->OwnerThread == Thread);
        ASSERT(Owner->OwnerCount > 0);

        /* Check if we are the last owner */
        if (!(--Owner->OwnerCount))
        {
            /* Release lock */
            ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
            return;
        }

        /* Clear owner */
        Owner->OwnerThread = 0;

        /* See if the resource isn't being owned anymore */
        ASSERT(Resource->ActiveCount > 0);
        if (!(--Resource->ActiveCount))
        {
            /* Check if there's an exclusive waiter */
            if (IsExclusiveWaiting(Resource))
            {
                /* Give exclusive access */
                Resource->OwnerThreads[0].OwnerThread = 1;
                Resource->OwnerThreads[0].OwnerCount = 1;
                Resource->ActiveCount = 1;
                Resource->NumberOfExclusiveWaiters--;

                /* Release the lock and give it away */
                ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
                KeSetEventBoostPriority(Resource->ExclusiveWaiters,
                                        (PKTHREAD*)
                                        &Resource->OwnerThreads[0].OwnerThread);
                return;
            }
        }
    }

    /* Release lock */
    ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
    return;
}

/*++
 * @name ExSetResourceOwnerPointer
 * @implemented NT4
 *
 *     The ExSetResourceOwnerPointer routine routine sets the owner thread
 *     thread pointer for an executive resource.
 *
 * @param Resource
 *        Pointer to the resource whose owner to change.
 *
 * @param OwnerPointer
 *        Pointer to an owner thread pointer of type ERESOURCE_THREAD.
  *
 * @return None.
 *
 * @remarks ExSetResourceOwnerPointer, used in conjunction with
 *          ExReleaseResourceForThreadLite, provides a means for one thread
 *          (acting as an resource manager thread) to acquire and release
 *          resources for use by another thread (acting as a resource user
 *          thread).
 *
 *          After calling ExSetResourceOwnerPointer for a specific resource,
 *          the only other routine that can be called for that resource is
 *          ExReleaseResourceForThreadLite.
 *
 *          Callers of ExSetResourceOwnerPointer must be running at
 *          IRQL <= DISPATCH_LEVEL.
 *
 *--*/
VOID
NTAPI
ExSetResourceOwnerPointer(IN PERESOURCE resource,
                          IN PVOID OwnerPointer)
{
    ERESOURCE_THREAD Thread;
    KIRQL OldIrql;
    POWNER_ENTRY Owner, ThisOwner;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;

    /* Sanity check */
    ASSERT((OwnerPointer != 0) && (((ULONG_PTR)OwnerPointer & 3) == 3));

    /* Get the thread */
    Thread = ExGetCurrentResourceThread();

    /* Sanity check */
    ExpVerifyResource(Resource);

    /* Lock the resource */
    ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);

    /* Check if it's exclusive */
    if (IsOwnedExclusive(Resource))
    {
        /* If it's exclusive, set the first entry no matter what */
        ASSERT(Resource->OwnerThreads[0].OwnerThread == Thread);
        ASSERT(Resource->OwnerThreads[0].OwnerCount > 0);
        Resource->OwnerThreads[0].OwnerThread = (ULONG_PTR)OwnerPointer;
    }
    else
    {
        /* Set the thread in both entries */
        ThisOwner = ExpFindEntryForThread(Resource,
                                         (ERESOURCE_THREAD)OwnerPointer,
                                         0);
        Owner = ExpFindEntryForThread(Resource, Thread, 0);
        if (!Owner)
        {
            /* Nobody owns it, crash */
            KEBUGCHECKEX(RESOURCE_NOT_OWNED,
                         (ULONG_PTR)Resource,
                         Thread,
                         (ULONG_PTR)Resource->OwnerTable,
                         3);
        }

        /* Set if we are the owner */
        if (ThisOwner)
        {
            /* Update data */
            ThisOwner->OwnerCount += Owner->OwnerCount;
            Owner->OwnerCount = 0;
            Owner->OwnerThread = 0;
            ASSERT(Resource->ActiveCount >= 2);
            Resource->ActiveCount--;
        }
    }

    /* Release the resource */
    ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
}

/*++
 * @name ExTryToAcquireResourceExclusiveLite
 * @implemented NT4
 *
 *     The ExTryToAcquireResourceExclusiveLite routine routine attemps to
 *     acquire the given resource for exclusive access.
 *
 * @param Resource
 *        Pointer to the resource to be acquired.
 *
 * @return TRUE if the given resource has been acquired for the caller.
 *
 * @remarks Callers of ExTryToAcquireResourceExclusiveLite must be running at
 *          IRQL < DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN
NTAPI
ExTryToAcquireResourceExclusiveLite(PERESOURCE resource)
{
    ERESOURCE_THREAD Thread;
    KIRQL OldIrql;
    BOOLEAN Acquired = FALSE;
    PERESOURCE_XP Resource = (PERESOURCE_XP)resource;
    DPRINT("ExTryToAcquireResourceExclusiveLite: %p\n", Resource);

    /* Sanity check */
    ASSERT((Resource->Flag & ResourceNeverExclusive) == 0);

    /* Get the thread */
    Thread = ExGetCurrentResourceThread();

    /* Sanity check and validation */
    ASSERT(KeIsExecutingDpc() == FALSE);
    ExpVerifyResource(Resource);

    /* Acquire the lock */
    ExAcquireResourceLock(&Resource->SpinLock, &OldIrql);

    /* Check if there is an owner */
    if (!Resource->ActiveCount)
    {
        /* No owner, give exclusive access */
        Resource->Flag |= ResourceOwnedExclusive;
        Resource->OwnerThreads[0].OwnerThread = Thread;
        Resource->OwnerThreads[0].OwnerCount = 1;
        Resource->ActiveCount = 1;
        Acquired = TRUE;
    }
    else if ((IsOwnedExclusive(Resource)) &&
             (Resource->OwnerThreads[0].OwnerThread == Thread))
    {
        /* Do a recursive acquire */
        Resource->OwnerThreads[0].OwnerCount++;
        Acquired = TRUE;
    }

    /* Release the resource */
    ExReleaseResourceLock(&Resource->SpinLock, OldIrql);
    return Acquired;
}

/*++
 * @name ExEnterCriticalRegionAndAcquireResourceExclusive
 * @implemented NT5.1
 *
 *     The ExEnterCriticalRegionAndAcquireResourceExclusive enters a critical
 *     region and then exclusively acquires a resource.
 *
 * @param Resource
 *        Pointer to the resource to acquire.
 *
 * @return Pointer to the Win32K thread pointer of the current thread.
 *
 * @remarks See ExAcquireResourceExclusiveLite.
 *
 *--*/
PVOID
NTAPI
ExEnterCriticalRegionAndAcquireResourceExclusive(PERESOURCE Resource)
{
    /* Enter critical region */
    KeEnterCriticalRegion();

    /* Acquire the resource */
    ExAcquireResourceExclusiveLite(Resource, TRUE);

    /* Return the Win32 Thread */
    return KeGetCurrentThread()->Win32Thread;
}

/*++
 * @name ExReleaseResourceAndLeaveCriticalRegion
 * @implemented NT5.1
 *
 *     The ExReleaseResourceAndLeaveCriticalRegion release a resource and
 *     then leaves a critical region.
 *
 * @param Resource
 *        Pointer to the resource to release.
 *
 * @return None
 *
 * @remarks See ExReleaseResourceLite.
 *
 *--*/
VOID
FASTCALL
ExReleaseResourceAndLeaveCriticalRegion(PERESOURCE Resource)
{
    /* Release the resource */
    ExReleaseResourceLite(Resource);

    /* Leave critical region */
    KeLeaveCriticalRegion();
}

/* EOF */

