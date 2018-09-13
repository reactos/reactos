#include "iop.h"

//
// This entire file is only present if NO_SPECIAL_IRP isn't defined
//
#ifndef NO_SPECIAL_IRP

//
// When enabled, everything is locked down on demand...
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,     IovpTrackingDataInit)
#pragma alloc_text(PAGEVRFY, IovpTrackingDataFindPointer)
#pragma alloc_text(PAGEVRFY, IovpTrackingDataFindAndLock)
#pragma alloc_text(PAGEVRFY, IovpTrackingDataCreateAndLock)
#pragma alloc_text(PAGEVRFY, IovpTrackingDataGetCurrentSessionData)
#pragma alloc_text(PAGEVRFY, IovpTrackingDataFree)
#pragma alloc_text(PAGEVRFY, IovpTrackingDataAcquireLock)
#pragma alloc_text(PAGEVRFY, IovpTrackingDataReleaseLock)
#pragma alloc_text(PAGEVRFY, IovpTrackingDataReference)
#pragma alloc_text(PAGEVRFY, IovpTrackingDataDereference)
#pragma alloc_text(PAGEVRFY, IovpWatermarkIrp)
#endif

#define POOL_TAG_TRACKING_DATA      'tprI'
#define POOL_TAG_PROTECTED_IRP      '+prI'

//
// Debug spew level
//
ULONG IovpIrpTrackingSpewLevel = 0 ;

//
// This is our IRP tracking table, a hash table that points to a block of
// data associated with each IRP.
//
LIST_ENTRY  IovpIrpTrackingTable[IRP_TRACKING_HASH_SIZE];
KSPIN_LOCK  IovpIrpHashLock;

/*
 * The 10 routines listed below -
 *   IovpTrackingDataInit
 *   IovpTrackingDataCreateAndLock
 *   IovpTrackingDataFindAndLock
 *   IovpTrackingDataGetCurrentSessionData
 *   IovpTrackingDataReference
 *   IovpTrackingDataDereference
 *   IovpTrackingDataAcquireLock
 *   IovpTrackingDataReleaseLock
 *   IovpTrackingDataFree                    - (internal)
 *   IovpTrackingDataFindPointer             - (internal)
 *
 * - handle the IRP tracking structures. This is an allocation of memory
 * that lives alongside the IRP. We need this as the IRP structure and
 * size are things that drivers must pass around, so we cannot add this
 * information into the IRP itself by expanding it's size. We use a hash
 * table setup to quickly find the IRPs in our table, so not too much time
 * is lost.
 *
 * Locking semantics: No IRP may be removed from or inserted into the table
 * without the HashLock being taken. Furthermore at some point in the lifetime
 * of the tracking data the pointer to the IRP may be zero'd, making it
 * "unreachable". This zeroing of the IRP pointer must also happen under the
 * hash lock.
 *
 * Three functions may be called to retrieve tracking data, the create and find
 * instructions. The acquire lock function can be used if you already have a
 * referenced pointer to the tracking data and merely need to lock it. None of
 * these functions add a reference to the object. Reference counts aren't
 * examined until the lock on the tracking data is dropped (ReleaseLock).
 * First the PointerCount member is first checked to see if the tracking
 * data should be marked "unreachable" (TrackedIrp = NULL). Then ReferenceCount
 * is examined to see if the tracking data can be freed.
 *
 * Note that find searches the table holding the hashlock. If an entry is found
 * a handoff to the IRP specific lock must be executed. This is done by
 * incrementing the ReferenceCount and then releasing the hash lock to ensure
 * the packet stays in the table. The hash lock is then dropped and the IRPlock
 * picked up. Note that ReferenceCount is only dropped under the held IRPlock
 * though, allowing us to avoid taking it to examining the count under the
 * hash lock as often as possible.
 *
 * Perf - The hashlock should be replaced with an array of
 *        IRP_TRACKING_HASH_SIZE hashlocks with little cost.
 */

VOID
FASTCALL
IovpTrackingDataInit(
    VOID
    )
/*++

  Description:

    This routine initializes all the important structures we use to track
    IRPs through the hash tables.

  Arguments:

    None

  Return Value:

    None

--*/
{
    ULONG i;

    PAGED_CODE();

    KeInitializeSpinLock( &IovpIrpHashLock );
    for(i=0; i<IRP_TRACKING_HASH_SIZE; i++) {

        InitializeListHead(IovpIrpTrackingTable+i);
    }
}

PIOV_REQUEST_PACKET
FASTCALL
IovpTrackingDataCreateAndLock(
    IN PIRP           Irp
    )
/*++

  Description:

    This routine creates a tracking packet for a new IRP. The IRP does not get
    an initial reference count however. IovpTrackingDataReleaseLock must be
    called to drop the lock.

  Arguments:

    Irp                    - Irp to begin tracking.

  Return Value:

    iovPacket block, NULL if no memory.

--*/
{
    KIRQL oldIrql;
    PIOV_REQUEST_PACKET iovPacket;
    PLIST_ENTRY hashHead;
    ULONG trackingDataSize;
    LONG newCount;

    ExAcquireSpinLock( &IovpIrpHashLock, &oldIrql );

    iovPacket = IovpTrackingDataFindPointer(Irp, &hashHead) ;

    ASSERT(!iovPacket) ;

    //
    // One extra stack location is allocated as the "zero'th" is used to
    // simplify some logic...
    //
    trackingDataSize = sizeof(IOV_REQUEST_PACKET)+Irp->StackCount*sizeof(IOV_STACK_LOCATION) ;

    iovPacket = ExAllocatePoolWithTag(
        NonPagedPool,
        trackingDataSize,
        POOL_TAG_TRACKING_DATA
        );

    if (!iovPacket) {

        ExReleaseSpinLock( &IovpIrpHashLock, oldIrql );
        return iovPacket;
    }

    //RtlZeroMemory(iovPacket, trackingDataSize) ;

    //
    // From top to bottom, initialize the fields. Note that there is not a
    // "surrogateHead". If any code needs to find out the first entry in the
    // circularly linked list of IRPs (the first is the only non-surrogate IRP),
    // then HeadPacket should be used. Note that the link to the session is
    // stored by the headPacket, more on this later.
    //
    iovPacket->TrackedIrp = Irp;
    KeInitializeSpinLock( &iovPacket->IrpLock );
    iovPacket->ReferenceCount = 1;
    iovPacket->PointerCount = 0;
    iovPacket->Flags = 0;
    InitializeListHead(&iovPacket->HashLink);
    InitializeListHead(&iovPacket->SurrogateLink);
    InitializeListHead(&iovPacket->SessionHead);
    iovPacket->HeadPacket = iovPacket;
    iovPacket->StackCount = Irp->StackCount;
    iovPacket->AssertFlags = IovpTrackingFlags;
    iovPacket->RealIrpCompletionRoutine = NULL;
    iovPacket->RealIrpControl = 0;
    iovPacket->RealIrpContext = NULL;
    iovPacket->TopStackLocation = 0;
    iovPacket->PriorityBoost = 0;
    iovPacket->LastLocation = 0;
    iovPacket->RefTrackingCount =0;
    iovPacket->RestoreHandle = NULL;
    iovPacket->pIovSessionData = NULL;

    //
    // Place into hash table under lock (with the initial reference count)
    //
    InsertHeadList(hashHead, &iovPacket->HashLink);

    ExReleaseSpinLock( &IovpIrpHashLock, oldIrql );

    ExAcquireSpinLock( &iovPacket->IrpLock, &iovPacket->CallerIrql );

    newCount = InterlockedDecrement(&iovPacket->ReferenceCount);

    //
    // If this assert gets hit it means somebody got hold of tracking data
    // at a very odd (and probably buggy) time. Actually, this might happen
    // if an IRP was cancelled right as it entered IoCallDriver...
    //
    //ASSERT(newCount == 0);

    TRACKIRP_DBGPRINT((
        "  VRP CREATE(%x)->%x\n",
        Irp,
        iovPacket
        ), 3) ;

    return iovPacket ;
}

VOID
FASTCALL
IovpTrackingDataFree(
    IN PIOV_REQUEST_PACKET IovPacket
    )
/*++

  Description:

    This routine free's the tracking data. The tracking data should already
    have been removed from the table by a call to IovpTrackingDataReleaseLock
    with the ReferenceCount at 0.

  Arguments:

    IovPacket        - Tracking data to free.

  Return Value:

    Nope.

--*/
{
    //
    // The list entry is inited to point back to itself when removed. The
    // pointer count should of course still be zero.
    //
    IovPacket->Flags|=TRACKFLAG_REMOVED_FROM_TABLE ;
    ASSERT(IsListEmpty(&IovPacket->HashLink)) ;

    //
    // with no reference counts...
    //
    ASSERT(!IovPacket->ReferenceCount) ;
    ASSERT(!IovPacket->PointerCount) ;

    TRACKIRP_DBGPRINT((
        "  VRP FREE(%x)x\n",
        IovPacket
        ), 3) ;

    ExFreePool(IovPacket) ;
}

PIOV_REQUEST_PACKET
FASTCALL
IovpTrackingDataFindAndLock(
    IN PIRP           Irp
    )
/*++

  Description:

    This routine will return the tracking data for an IRP that is
    being tracked without a surrogate or the tracking data for with
    a surrogate if the surrogate IRP is what was passed in.

  Arguments:

    Irp                    - Irp to find.

  Return Value:

    IovPacket block, iff above conditions are satified.

--*/
{
    KIRQL oldIrql ;
    PIOV_REQUEST_PACKET iovPacket ;
    PLIST_ENTRY listHead;

    ASSERT(Irp) ;
    ExAcquireSpinLock( &IovpIrpHashLock, &oldIrql );

    iovPacket = IovpTrackingDataFindPointer(Irp, &listHead) ;

    if (!iovPacket) {

        ExReleaseSpinLock( &IovpIrpHashLock, oldIrql );
        return NULL;
    }

    InterlockedIncrement(&iovPacket->ReferenceCount);

    ExReleaseSpinLock( &IovpIrpHashLock, oldIrql );

    IovpTrackingDataAcquireLock(iovPacket) ;
    iovPacket->CallerIrql = oldIrql;

    InterlockedDecrement(&iovPacket->ReferenceCount);

    if (iovPacket->TrackedIrp == NULL) {

        ASSERT(0);
        //
        // Someone IRP is being mishandled, we got in a race condition where
        // we got the packet but the pointer count decayed to zero. Therefore
        // we do not want this packet so we will return NULL after dropping
        // it's lock. This sort of thing really shouldn't happen ya know.
        //
        IovpTrackingDataReleaseLock(iovPacket);
        return NULL;
    }

    TRACKIRP_DBGPRINT((
        "  VRP FIND(%x)->%x\n",
        Irp,
        iovPacket
        ), 3) ;

    return iovPacket;
}

PIOV_REQUEST_PACKET
FASTCALL
IovpTrackingDataFindPointer(
    IN  PIRP               Irp,
    OUT PLIST_ENTRY        *HashHead
    )
/*++

  Description:

    This routine returns a pointer to a pointer to the Irp tracking data.
    This function is meant to be called by other routines in this file.

    N.B. The tracking lock is assumed to be held by the caller.

  Arguments:

    Irp                        - Irp to locate in the tracking table.

    HashHead                   - If return is non-null, points to the
                                 list head that should be used to insert
                                 the IRP.

  Return Value:

     IrpTrackingData iff found, NULL otherwise.

--*/
{
    KIRQL oldIrql ;
    PIOV_REQUEST_PACKET iovPacket ;
    PLIST_ENTRY listEntry, listHead;
    UINT_PTR hash ;

    ASSERT_SPINLOCK_HELD(&IovpIrpHashLock) ;

    hash = (((UINT_PTR) Irp)/PAGE_SIZE)*IRP_TRACKING_HASH_PRIME ;
    hash %= IRP_TRACKING_HASH_SIZE ;

    *HashHead = listHead = IovpIrpTrackingTable + hash ;

    for(listEntry = listHead;
        listEntry->Flink != listHead;
        listEntry = listEntry->Flink) {

        iovPacket = CONTAINING_RECORD(listEntry->Flink, IOV_REQUEST_PACKET, HashLink);

        if (iovPacket->TrackedIrp == Irp) {

            return iovPacket;
        }
    }

    return NULL ;
}

VOID
FASTCALL
IovpTrackingDataAcquireLock(
    IN  PIOV_REQUEST_PACKET IovPacket OPTIONAL
    )
/*++

  Description:

    This routine is called by to acquire the IRPs tracking data lock.

    Incoming IRQL must be the same as the callers (IoCallDriver, IoCompleteRequest)
    We may be at DPC level when we return. Callers *must* follow up with
    IovpTrackingDataReleaseLock.

  Arguments:

    IovPacket        - Pointer to the IRP tracking data (or NULL, in which
                       case this routine does nothing).

  Return Value:

     None.
--*/
{
    KIRQL oldIrql ;
    PIOV_REQUEST_PACKET iovCurPacket;

    if (!IovPacket) {

        return ;
    }

    iovCurPacket = IovPacket;
    ASSERT(iovCurPacket->ReferenceCount != 0);
    while(1) {

        ExAcquireSpinLock( &iovCurPacket->IrpLock, &oldIrql );
        iovCurPacket->CallerIrql = oldIrql ;

        if (iovCurPacket == IovPacket->HeadPacket) {

            break;
        }

        iovCurPacket = CONTAINING_RECORD(
            iovCurPacket->SurrogateLink.Blink,
            IOV_REQUEST_PACKET,
            SurrogateLink
            );
    }
}

VOID
FASTCALL
IovpTrackingDataReleaseLock(
    IN  PIOV_REQUEST_PACKET IovPacket
    )
/*++

  Description:

    This routine releases the IRPs tracking data lock and adjusts the ref count
    as appropriate. If the reference count drops to zero, the tracking data is
    freed.

  Arguments:

    IovPacket              - Pointer to the IRP tracking data.

  Return Value:

     None.

--*/
{
    BOOLEAN freeTrackingData;
    PIOV_REQUEST_PACKET iovCurPacket, iovHeadPacket, iovNextPacket;
    KIRQL oldIrql;

    //
    // Pass one, delink anyone from the tree who's leaving, and assert that
    // no surrogates are left after a freed one.
    //
    iovCurPacket = iovHeadPacket = IovPacket->HeadPacket;
    while(1) {

        ASSERT_SPINLOCK_HELD(&iovCurPacket->IrpLock);

        iovNextPacket = CONTAINING_RECORD(
            iovCurPacket->SurrogateLink.Flink,
            IOV_REQUEST_PACKET,
            SurrogateLink
            );

        //
        // PointerCount is always referenced under the IRP lock.
        //
        if (iovCurPacket->PointerCount == 0) {

            ExAcquireSpinLock( &IovpIrpHashLock, &oldIrql );

            //
            // This field may be examined only under the hash lock.
            //
            if (iovCurPacket->TrackedIrp) {

                iovCurPacket->TrackedIrp->Flags &=~ IRPFLAG_EXAMINE_MASK;
                iovCurPacket->TrackedIrp = NULL;
            }

            ExReleaseSpinLock( &IovpIrpHashLock, oldIrql );
        }

        //
        // We now remove any entries that will be leaving from the hash table.
        // Note that the ReferenceCount may be incremented outside the IRP lock
        // (but under the hash lock) but ReferenceCount can never be dropped
        // outside of the IRP lock. Therefore for performance we check once
        // and then take the lock to prevent anyone finding it and incrementing
        // it.
        //

        if (iovCurPacket->ReferenceCount == 0) {

            ExAcquireSpinLock( &IovpIrpHashLock, &oldIrql );

            if (iovCurPacket->ReferenceCount ==0) {

                ASSERT(iovCurPacket->PointerCount == 0);
                ASSERT((iovCurPacket->pIovSessionData == NULL) ||
                       (iovCurPacket != iovHeadPacket));
                ASSERT((iovNextPacket->ReferenceCount == 0) ||
                       (iovNextPacket == iovHeadPacket));

                RemoveEntryList(&iovCurPacket->HashLink);

                InitializeListHead(&iovCurPacket->HashLink);
            }
            ExReleaseSpinLock( &IovpIrpHashLock, oldIrql );
        }

        if (iovCurPacket == IovPacket) {

            break;
        }

        iovCurPacket = iovNextPacket;
    }

    //
    // Pass two, drop locks and free neccessary data.
    //
    iovCurPacket = iovHeadPacket;
    while(1) {

        freeTrackingData = IsListEmpty(&iovCurPacket->HashLink);

        iovNextPacket = CONTAINING_RECORD(
            iovCurPacket->SurrogateLink.Flink,
            IOV_REQUEST_PACKET,
            SurrogateLink
            );

        ExReleaseSpinLock(&iovCurPacket->IrpLock, iovCurPacket->CallerIrql) ;

        if (freeTrackingData) {

            RemoveEntryList(&iovCurPacket->SurrogateLink);
            InitializeListHead(&iovCurPacket->SurrogateLink);

            IovpTrackingDataFree(iovCurPacket) ;
        }

        if (iovCurPacket == IovPacket) {

            break;
        }

        iovCurPacket = iovNextPacket;
    }
}

VOID
FASTCALL
IovpTrackingDataReference(
    IN PIOV_REQUEST_PACKET IovPacket,
    IN IOV_REFERENCE_TYPE  IovRefType
    )
{
    ASSERT_SPINLOCK_HELD(&IovPacket->IrpLock);

    TRACKIRP_DBGPRINT((
        "  VRP REF(%x) %x++\n",
        IovPacket,
        IovPacket->ReferenceCount
        ), 3) ;

    InterlockedIncrement(&IovPacket->ReferenceCount);
    if (IovRefType == IOVREFTYPE_POINTER) {

        TRACKIRP_DBGPRINT((
            "  VRP REF2(%x) %x++\n",
            IovPacket,
            IovPacket->PointerCount
            ), 3) ;

        IovPacket->PointerCount++;
    }
}

VOID
FASTCALL
IovpTrackingDataDereference(
    IN PIOV_REQUEST_PACKET IovPacket,
    IN IOV_REFERENCE_TYPE  IovRefType
    )
{
    KIRQL oldIrql;

    ASSERT_SPINLOCK_HELD(&IovPacket->IrpLock);
    ASSERT(IovPacket->ReferenceCount > 0);

    TRACKIRP_DBGPRINT((
        "  VRP DEREF(%x) %x--\n",
        IovPacket,
        IovPacket->ReferenceCount
        ), 3) ;

    if (IovRefType == IOVREFTYPE_POINTER) {

        ASSERT(IovPacket->PointerCount > 0);

        TRACKIRP_DBGPRINT((
            "  VRP DEREF2(%x) %x--\n",
            IovPacket,
            IovPacket->PointerCount
            ), 3) ;

        IovPacket->PointerCount--;

        if (IovPacket->PointerCount == 0) {

            ExAcquireSpinLock( &IovpIrpHashLock, &oldIrql );

            IovPacket->TrackedIrp->Flags &=~ IRPFLAG_EXAMINE_MASK;
            IovPacket->TrackedIrp = NULL;

            ExReleaseSpinLock( &IovpIrpHashLock, oldIrql );
        }
    }
    InterlockedDecrement(&IovPacket->ReferenceCount);

    ASSERT(IovPacket->ReferenceCount >= IovPacket->PointerCount);
}

PIOV_SESSION_DATA
FASTCALL
IovpTrackingDataGetCurrentSessionData(
    IN PIOV_REQUEST_PACKET IovPacket
    )
{
    ASSERT_SPINLOCK_HELD(&IovPacket->IrpLock);
    ASSERT_SPINLOCK_HELD(&IovPacket->HeadPacket->IrpLock);
    ASSERT((IovPacket->HeadPacket->pIovSessionData == NULL)||
           (IovPacket->Flags&TRACKFLAG_ACTIVE)) ;

    return IovPacket->HeadPacket->pIovSessionData;
}

/*
 * The 4 routines listed below -
 *   IovpProtectedIrpAllocate
 *   IovpProtectedIrpMakeTouchable
 *   IovpProtectedIrpMakeUntouchable
 *   IovpProtectedIrpFree
 *
 * - handle management of the replacement IRP. Specifically, we want to be
 * able to allocate a set of non-paged bytes we can remove the backing
 * physical memory from, and release the virtual addresses for later (we
 * are essentially breaking free into it's two components). We do this with
 * help from the special pool.
 *
 */

PIRP
FASTCALL
IovpProtectedIrpAllocate(
    IN CCHAR    StackSize,
    IN BOOLEAN  ChargeQuota,
    IN PETHREAD QuotaThread OPTIONAL
    )
/*++

  Description:

    This routine allocates an IRP from the special pool using the
    "replacement IRP" tag.

  Arguments:

     StackSize   - Number of stack locations to give the new IRP

     ChargeQuota - TRUE iff quota should be charged against QuotaThread

     QuotaThread - See above

  Return Value:

     Pointer to the memory allocated.

--*/
{
    PIRP pSurrogateIrp;
    ULONG_PTR irpPtr;
    SIZE_T sizeOfAllocation;

    //
    // We are allocating an IRP from the special pool. Since IRPs may come from
    // lookaside lists they may be ULONG aligned. The memory manager on the
    // other hand gaurentees quad-aligned allocations. So to catch all special
    // pool overrun bugs we "skew" the IRP right up to the edge.
    //
    sizeOfAllocation = IoSizeOfIrp(StackSize);

    ASSERT((sizeOfAllocation % (sizeof(ULONG))) == 0);

    //
    // ADRIAO BUGBUG 08/16/98 - Use a quota'd alloc function if one is available
    // later...
    //
    irpPtr = (ULONG_PTR) ExAllocatePoolWithTagPriority(
        NonPagedPool,
        sizeOfAllocation,
        POOL_TAG_PROTECTED_IRP,
        HighPoolPrioritySpecialPoolOverrun
        );

    pSurrogateIrp = (PIRP) (irpPtr);

    return pSurrogateIrp;
}

PVOID
FASTCALL
IovpProtectedIrpMakeUntouchable(
    IN  PIRP    Irp OPTIONAL,
    IN  BOOLEAN Permanent
    )
/*++

  Description:

    This routine makes the surrogate IRP untouchable. Currently, this is
    done by freeing the IRP back to the special pool.

  Arguments:

    Irp        - Pointer to the Irp to make untouchable
    Permanent  - TRUE iff Irp should not be made touchable again


  Return Value:

     RestoreHandle to be passed to make the Irp touchable again, or to free it.

--*/
{
    ULONG howModified;

    if (!Irp) {
        return NULL ;
    }

    if (Permanent) {
        ExFreePool(Irp) ;
        return NULL;
    }

    howModified = (ULONG) MmProtectSpecialPool(Irp, PAGE_NOACCESS);

    switch(howModified) {

        case (ULONG) -1:

            //
            // Didn't come from special pool.
            //
            return NULL;

        case 0:

            //
            // Can't comply with request, ref counts, etc hold down page.
            //
            return NULL;

        default:

            //
            // Allocation has been successfully marked as untouchable.
            //
            return (PVOID) Irp;
    }
}

VOID
FASTCALL
IovpProtectedIrpMakeTouchable(
    IN  PIRP    Irp,
    IN  PVOID   *RestoreHandle
    )
/*++

  Description:

    This routine makes the an IRP touchable if previously untouchable.

  Arguments:

    Irp           - Pointer to the Irp to make untouchable
    RestoreHandle - Pointer to handle returned by IovpProtectedIrpMakeUntouchable


  Return Value:

     None.
--*/
{
    if (*RestoreHandle) {

        ASSERT(*RestoreHandle == Irp);
        MmProtectSpecialPool(Irp, PAGE_READWRITE) ;
        *RestoreHandle = NULL ;
    }
}

VOID
FASTCALL
IovpProtectedIrpFree(
    IN  PIRP  Irp OPTIONAL,
    IN  PVOID *RestoreHandle
    )
/*++

  Description:

    This routine is called when the call stack has entirely unwound
    and the IRP has completed. At this point it is no longer really
    useful to hold the surrogate IRP around. As we are using the
    special pool currently, this routine needs to do nothing.

  Arguments:

    IrpTrackingData        - Pointer to the IRP tracking data.

  Return Value:

     None.
--*/
{
    ASSERT((*RestoreHandle) == NULL);
}

VOID
FASTCALL
IovpWatermarkIrp(
    IN PIRP  Irp,
    IN ULONG Flags
    )
{
    PIOV_REQUEST_PACKET iovPacket;

    iovPacket = IovpTrackingDataFindAndLock(Irp);

    if (iovPacket == NULL) {

        return;
    }

    if (Flags & IRP_SYSTEM_RESTRICTED) {

        //
        // Note that calling this function is not in itself enough to get the
        // system to prevent drivers from sending restricted IRPs. Those IRPs to
        // be protected must also be added to IovpIsSystemRestrictedIrp in
        // flunkirp.c
        //
        iovPacket->Flags |= TRACKFLAG_WATERMARKED;
    }

    if (Flags & IRP_BOGUS) {

        iovPacket->Flags |= TRACKFLAG_BOGUS;
    }

    IovpTrackingDataReleaseLock(iovPacket);
}

#endif // NO_SPECIAL_IRP



