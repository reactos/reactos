/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Resource.c

Abstract:

    This module implements the executive functions to acquire and release
    a shared resource, that was unfortunately export to ntddk in release 1.

Author:

    Gary D. Kimura      [GaryKi]    25-Jun-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"
#pragma hdrstop

#include <nturtl.h>

//
//  The following variable, macros, and procedure are only for debug purposes.
//

extern LARGE_INTEGER ExpTimeout;

//
//  thirty days worth
//

extern ULONG ExpResourceTimeoutCount;

//
//  Avoid the aliasing done in ex.h
//

#undef ExInitializeResource
#undef ExAcquireResourceExclusive
#undef ExReleaseResourceForThread
#undef ExDeleteResource

#if !DBG && NT_UP && defined(i386)
#define ExReleaseResourceForThread ExpReleaseResourceForThread
VOID
ExReleaseResourceForThread(
    IN PNTDDK_ERESOURCE Resource,
    IN ERESOURCE_THREAD OurThread
    );
#endif

#if DBG

VOID
ExpAssertResourceDdk (
    IN PNTDDK_ERESOURCE   Resource
    );

#define ASSERT_RESOURCE(_Resource)  ExpAssertResourceDdk(_Resource)
#else
#define ASSERT_RESOURCE(_Resource)
#endif

//
// bit value for ERESOURCE.Flag
//

#define ExclusiveWaiter             0x01    // ** Also in i386\resoura.asm **
#define SharedWaiter                0x02    // ** Also in i386\resoura.asm **
//      CounterShiftBit             0x04    - see below
#define DisablePriorityBoost        0x08
//      ResourceOwnedExclusive      0x80    - from ex.h

#if NT_UP
#define CounterShiftBit     0x00
#else
#define CounterShiftBit     0x04        // Must be 0x04!
#endif

#define IsExclusiveWaiting(a)   (((a)->Flag & ExclusiveWaiter) != 0)
#define IsOwnedExclusive(a)     (((a)->Flag & ResourceOwnedExclusive) != 0)
#define IsOwnedExclusive(a)     (((a)->Flag & ResourceOwnedExclusive) != 0)
#define IsBoostAllowed(a)       (((a)->Flag & DisablePriorityBoost) == 0)

//
//  The following static data and two macros are used to control entering and
//  exiting the resource critical section.
//

static KSPIN_LOCK ExpResourceSpinLock;

//++
//
//  Macros:
//      AcquireResourceLock - Obtains the Resource's spinlock
//      ReleaseResourceLock - Releases the Resource's spinlock
//
//      WaitForResourceExclusive(Resource, OldIrql)
//          Block on resource until WakeSharedWaiters
//
//      WakeExclusiveWaiters
//          Wakes threads on resource which are WaitForResourceShared
//
//--

#define AcquireResourceLock(_Resource,_Irql) {              \
    ExAcquireSpinLock( &_Resource->SpinLock, _Irql );       \
    ASSERT_RESOURCE( _Resource );                           \
}

#define ReleaseResourceLock(_Resource,_Irql) {              \
    ExReleaseSpinLock( &_Resource->SpinLock, _Irql );       \
}

#define INITIAL_TABLE_SIZE  4

#define WaitForResourceExclusive(_Resource, _OldIrql)   {               \
    _Resource->Flag |= ExclusiveWaiter;                                 \
    _Resource->NumberOfExclusiveWaiters += 1;                           \
    ReleaseResourceLock ( _Resource, _OldIrql );                        \
    ExpWaitForResourceDdk  ( _Resource, &_Resource->ExclusiveWaiters ); \
    AcquireResourceLock ( _Resource, &_OldIrql);                        \
    if (--_Resource->NumberOfExclusiveWaiters != 0) {                   \
        _Resource->Flag |= ExclusiveWaiter;                             \
    }                                                                   \
}

#define WakeExclusiveWaiters(_Resource)    {                            \
    _Resource->Flag &= ~ExclusiveWaiter;                                \
    KeSetEvent(&_Resource->ExclusiveWaiters, 0, FALSE);                 \
}

//
//  Local procedure prototypes
//

VOID
ExpWaitForResourceDdk (
    IN PNTDDK_ERESOURCE   Resource,
    IN PVOID        Object
    );

//
// The following list head points to a list of all currently
// initialized Executive Resources in the system.
//

extern LIST_ENTRY ExpSystemResourcesList;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,ExpResourceInitialization)
#pragma alloc_text(PAGELK,ExQuerySystemLockInformation)
#endif

//
//
//
//


NTSTATUS
ExInitializeResource (
    IN PNTDDK_ERESOURCE Resource
    )

/*++

Routine Description:

    This routine initializes the input resource variable

Arguments:

    Resource - Supplies the resource variable being initialized

Return Value:

    Status of the operation.

--*/

{
    ULONG   i;

    ASSERTMSG("A resource cannot be in paged pool ", MmDeterminePoolType(Resource) == NonPagedPool);
    //
    //  Initialize the shared and exclusive waiting counters and semaphore.
    //  The counters indicate how many are waiting for access to the resource
    //  and the semaphores are used to wait on the resource.  Note that
    //  the semaphores can also indicate the number waiting for a resource
    //  however there is a race condition in the algorithm on the acquire
    //  side if count if not updated before the critical section is exited.
    //  So we need to have an outside counter.
    //

    Resource->NumberOfSharedWaiters    = 0;
    Resource->NumberOfExclusiveWaiters = 0;

    KeInitializeSemaphore ( &Resource->SharedWaiters, 0, MAXLONG );
    KeInitializeEvent     ( &Resource->ExclusiveWaiters, SynchronizationEvent, FALSE );
    KeInitializeSpinLock  ( &Resource->SpinLock );

    Resource->OwnerThreads = Resource->InitialOwnerThreads;
    Resource->OwnerCounts  = Resource->InitialOwnerCounts;

    Resource->TableSize    = INITIAL_TABLE_SIZE;
    Resource->ActiveCount  = 0;
    Resource->TableRover   = 1;
    Resource->Flag         = 0;

    for(i=0; i < INITIAL_TABLE_SIZE; i++) {
        Resource->OwnerThreads[i] = 0;
        Resource->OwnerCounts[i] = 0;
    }

    Resource->ContentionCount = 0;
    InitializeListHead( &Resource->SystemResourcesList );

#if i386 && !FPO
    if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {
        Resource->CreatorBackTraceIndex = RtlLogStackBackTrace();
        }
    else {
        Resource->CreatorBackTraceIndex = 0;
        }
#endif // i386 && !FPO
    if (Resource >= (PNTDDK_ERESOURCE)MM_USER_PROBE_ADDRESS) {
        ExInterlockedInsertTailList (
                &ExpSystemResourcesList,
                &Resource->SystemResourcesList,
                &ExpResourceSpinLock );
    }

    return STATUS_SUCCESS;
}


BOOLEAN
ExAcquireResourceExclusive(
    IN PNTDDK_ERESOURCE Resource,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The routine acquires the resource for exclusive access.  Upon return from
    the procedure the resource is acquired for exclusive access.

Arguments:

    Resource - Supplies the resource to acquire

    Wait - Indicates if the call is allowed to wait for the resource
        to become available for must return immediately

Return Value:

    BOOLEAN - TRUE if the resource is acquired and FALSE otherwise

--*/

{
    KIRQL OldIrql;
    ERESOURCE_THREAD  OurThread;

    ASSERTMSG("Routine cannot be called at DPC ", !KeIsExecutingDpc() );

    ASSERT((Resource->Flag & ResourceNeverExclusive) == 0);

    OurThread = (ULONG_PTR)ExGetCurrentResourceThread();

    //
    //  Get exclusive access to this resource structure
    //

    AcquireResourceLock( Resource, &OldIrql );

    //
    //  Loop until the resource is ours or exit if we cannot wait.
    //

    while (TRUE) {
        if (Resource->ActiveCount == 0) {

            //
            // Resource is un-owned, obtain it exclusive
            //

            Resource->InitialOwnerThreads[0] = OurThread;
            Resource->OwnerThreads[0] = OurThread;
            Resource->OwnerCounts[0]  = 1;
            Resource->ActiveCount     = 1;
            Resource->Flag           |= ResourceOwnedExclusive;
            ReleaseResourceLock ( Resource, OldIrql );
            return TRUE;
        }

        if (IsOwnedExclusive(Resource) &&
            Resource->OwnerThreads[0] == OurThread) {

            //
            // Our thread is already the exclusive resource owner
            //

            Resource->OwnerCounts[0] += 1;
            ReleaseResourceLock( Resource, OldIrql );
            return TRUE;
        }

        //
        //  Check if we are allowed to wait or must return immediately, and
        //  indicate that we didn't acquire the resource
        //

        if (!Wait) {
            ReleaseResourceLock( Resource, OldIrql );
            return FALSE;
        }

        //
        //  Otherwise we need to wait to acquire the resource.
        //

        WaitForResourceExclusive ( Resource, OldIrql );
    }
}


VOID
ExReleaseResourceForThread(
    IN PNTDDK_ERESOURCE Resource,
    IN ERESOURCE_THREAD OurThread
    )

/*++

Routine Description:

    This routine release the input resource for the indicated thread.  The
    resource could have been acquired for either shared or exclusive access.

Arguments:

    Resource - Supplies the resource to release

    Thread - Supplies the thread that originally acquired the resource

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    ASSERT( OurThread != 0 );

    //
    //  Get exclusive access to this resource structure
    //

    AcquireResourceLock( Resource, &OldIrql );

    ASSERT( Resource->OwnerThreads[0] == OurThread );

    //
    // OwnerThread[0] is optimized for resources which get
    // single users.  We check it first, plus we clear the
    // threadid if the counts goes to zero
    //

    if (--Resource->OwnerCounts[0] != 0) {
        ReleaseResourceLock( Resource, OldIrql );
        return;
    }

    Resource->OwnerThreads[0] = 0;
    Resource->InitialOwnerThreads[0] = 0;

    //
    // Thread's count went to zero, lower the resource's active count.
    //

    Resource->ActiveCount -= 1;

    ASSERT( Resource->ActiveCount == 0 );

    //
    // Resource's activecount went to zero - clear possible exclusive
    // owner bit, and wake any waiters
    //

    if (IsExclusiveWaiting(Resource)) {

        WakeExclusiveWaiters ( Resource );
    }

    //
    //  No longer owned exclusive
    //

    Resource->Flag &= ~ResourceOwnedExclusive;

    ReleaseResourceLock( Resource, OldIrql );
    return;
}

NTSTATUS
ExDeleteResource (
    IN PNTDDK_ERESOURCE Resource
    )

/*++

Routine Description:

    This routine deletes (i.e., uninitializes) the input resource variable


Arguments:

    Resource - Supplies the resource variable being deleted

Return Value:

    None

--*/

{
    ASSERTMSG("Routine cannot be called at DPC ", !KeIsExecutingDpc() );

    ASSERT_RESOURCE( Resource );
    ASSERT( !IsExclusiveWaiting(Resource) );


    if (Resource >= (PNTDDK_ERESOURCE)MM_USER_PROBE_ADDRESS) {
        KIRQL OldIrql;

        ExAcquireSpinLock( &ExpResourceSpinLock, &OldIrql );
        RemoveEntryList( &Resource->SystemResourcesList );
        ExReleaseSpinLock( &ExpResourceSpinLock, OldIrql );

    }

    return STATUS_SUCCESS;
}

VOID
ExpWaitForResourceDdk (
    IN PNTDDK_ERESOURCE   Resource,
    IN PVOID        Object
    )
/*++

Routine Description:

    The routine waits on the Resource's object to be set.  If the
    wait is too long the current owners of the resource are boosted
    in priority.

Arguments:

    Resource - Supplies the resource

    Object - Event or Semaphore to wait on

Return Value:

    None

--*/
{
    KIRQL       OldIrql;
    NTSTATUS    Status;
    ULONG       i;


    Resource->ContentionCount += 1;

    i = 0;
    for (; ;) {
        Status = KeWaitForSingleObject (
                        Object,
                        Executive,
                        KernelMode,
                        FALSE,
                        &ExpTimeout );

        if (Status != STATUS_TIMEOUT) {
            break;
        }

        if (i++ >= ExpResourceTimeoutCount) {
            i = 0;

            DbgPrint("Resource @ %lx\n", Resource);
            DbgPrint(" ActiveCount = %04lx  Flags = %s%s%s\n",
                Resource->ActiveCount,
                IsOwnedExclusive(Resource)   ? "IsOwnedExclusive " : "",
                "",
                IsExclusiveWaiting(Resource) ? "ExclusiveWaiter "  : ""
            );

            DbgPrint(" NumberOfExclusiveWaiters = %04lx\n", Resource->NumberOfExclusiveWaiters);

            DbgPrint("  Thread = %08lx, Count = %02x\n",
                Resource->OwnerThreads[0],
                Resource->OwnerCounts[0] );

            DbgBreakPoint();
            DbgPrint("EX - Rewaiting\n");
        }

        //
        //  Give owning thread(s) a priority boost
        //

        AcquireResourceLock( Resource, &OldIrql );

        if (IsBoostAllowed(Resource) && IsOwnedExclusive(Resource)) {

            //
            //  Only one thread, boost it
            //

            KeBoostPriorityThread((PKTHREAD) Resource->OwnerThreads[0],
                                  ERESOURCE_INCREMENT );
        }

        ReleaseResourceLock( Resource, OldIrql );

        //
        //  Loop and wait some more
        //
    }

    //
    //  Wait was satisfied
    //

    ASSERT(NT_SUCCESS(Status));
    return ;
}

#if DBG
VOID
ExpAssertResourceDdk (
    IN PNTDDK_ERESOURCE   Resource
)
{
    USHORT  Sum;

    //
    //  Assert internal structures headers are correct
    //

    ASSERT(Resource->SharedWaiters.Header.Type == SemaphoreObject);
    ASSERT(Resource->SharedWaiters.Header.Size == sizeof(KSEMAPHORE));
    ASSERT(Resource->ExclusiveWaiters.Header.Type == SynchronizationEvent);
    ASSERT(Resource->ExclusiveWaiters.Header.Size == sizeof(KEVENT));

    //
    //  Count number of currently owned threads
    //

    Sum = Resource->OwnerCounts[0];

    //
    //  Verify sum is consistent with what's in the resource
    //

    ASSERT(Resource->ActiveCount == Sum);

    if (Sum == 0) {
        ASSERT(!IsOwnedExclusive(Resource));
        ASSERT(Resource->OwnerThreads[0] == 0);
        ASSERT(Resource->InitialOwnerThreads[0] == 0);
    }

    if (Sum > 1) {
        ASSERT(!IsOwnedExclusive(Resource));
    }

    //
    //  Verify resource flags appear correct
    //

    if (IsOwnedExclusive(Resource)) {
        ASSERT (Sum == 1);
        ASSERT (Resource->OwnerCounts[0] != 0);
        ASSERT (Resource->OwnerThreads[0] == Resource->InitialOwnerThreads[0]);
    }
}
#endif  // dbg

