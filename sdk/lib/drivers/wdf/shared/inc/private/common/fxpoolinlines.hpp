/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPoolInlines.h

Abstract:

    This module contains inline functions for pool

Environment:

    kernel/user mode

Revision History:

        Made it mode agnostic

--*/

#ifndef __FX_POOL_INLINES_HPP__
#define __FX_POOL_INLINES_HPP__

extern "C" {

#if defined(EVENT_TRACING)
#include "FxPoolInlines.hpp.tmh"
#endif

}

_Must_inspect_result_
NTSTATUS
__inline
FxPoolAddHeaderSize(
    __in    PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in    size_t AllocationSize,
    __out   size_t* NewSize
    )
{
    NTSTATUS status;
    size_t total;

    total = AllocationSize;

    //
    // sizeof(FX_POOL_HEADER) is too large since it will contain enough memory
    // for AllocationStart which we compute on our own.
    //
    status = RtlSizeTAdd(total, FX_POOL_HEADER_SIZE, &total);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Size overflow, could not add pool header, %!STATUS!", status);

        return status;
    }

    if (FxDriverGlobals->IsPoolTrackingOn())  {
        status = RtlSizeTAdd(total, sizeof(FX_POOL_TRACKER), &total);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Size overflow, could not add pool tracker, %!STATUS!", status);
            return status;
        }
    }

    *NewSize = total;

    return STATUS_SUCCESS;
}

VOID
__inline
FxPoolInsertNonPagedAllocateTracker(
    __in PFX_POOL           Pool,
    __in PFX_POOL_TRACKER   Tracker,
    __in SIZE_T             Size,
    __in ULONG              Tag,
    __in PVOID              Caller
    )
/*++

Routine Description:

    Format and insert a Tracker for a NonPaged allocation.

Arguments:

    Pool    - Pointer to FX_POOL structure

    Tracker - Pointer to raw FX_POOL_TRACKER structure.

    Size    - Size in bytes of the allocation

    Tag     - Caller specified additional tag value for debugging/tracing

    Caller  - Caller's address

Returns:

    VOID

--*/
{
    KIRQL  irql;

    Tracker->Tag            = Tag;
    Tracker->PoolType       = NonPagedPool;
    Tracker->Pool           = Pool;
    Tracker->Size           = Size;
    Tracker->CallersAddress = Caller;

    Pool->NonPagedLock.Acquire(&irql);

    InsertTailList(&Pool->NonPagedHead, &Tracker->Link);

    Pool->NonPagedBytes += Size;
    Pool->NonPagedAllocations++;

    if( Pool->NonPagedBytes > Pool->PeakNonPagedBytes ) {
        Pool->PeakNonPagedBytes = Pool->NonPagedBytes;
    }

    if( Pool->NonPagedAllocations > Pool->PeakNonPagedAllocations ) {
        Pool->PeakNonPagedAllocations = Pool->NonPagedAllocations;
    }

    Pool->NonPagedLock.Release(irql);
}

VOID
__inline
FxPoolRemoveNonPagedAllocateTracker(
    __in PFX_POOL_TRACKER  Tracker
    )
/*++

Routine Description:

    Decommission a Tracker for a NonPaged allocation.

Arguments:

    Tracker - Pointer to the formatted FX_POOL_TRACKER structure.

Returns:

    VOID

--*/
{
    KIRQL irql;

    Tracker->Pool->NonPagedLock.Acquire(&irql);

    RemoveEntryList(&Tracker->Link);

    Tracker->Pool->NonPagedBytes -= Tracker->Size;
    Tracker->Pool->NonPagedAllocations--;

    Tracker->Pool->NonPagedLock.Release(irql);
}

VOID
__inline
FxPoolInsertPagedAllocateTracker(
    __in PFX_POOL           Pool,
    __in PFX_POOL_TRACKER    Tracker,
    __in SIZE_T             Size,
    __in ULONG              Tag,
    __in PVOID              Caller
    )
/*++

Routine Description:

    Format and insert a Tracker for a Paged allocation.

Arguments:

    Pool    - Pointer to FX_POOL structure

    Tracker - Pointer to raw FX_POOL_TRACKER structure.

    Size    - Size in bytes of the allocation

    Tag     - Caller specified additional tag value for debugging/tracing

    Caller  - Caller's address

Returns:

    VOID

--*/
{
    Tracker->Tag            = Tag;
    Tracker->PoolType       = PagedPool;
    Tracker->Pool           = Pool;
    Tracker->Size           = Size;
    Tracker->CallersAddress = Caller;

    Pool->PagedLock.Acquire();

    InsertTailList(&Pool->PagedHead, &Tracker->Link);

    Pool->PagedBytes += Size;
    Pool->PagedAllocations++;

    if( Pool->PagedBytes > Pool->PeakPagedBytes ) {
        Pool->PeakPagedBytes = Pool->PagedBytes;
    }

    if( Pool->PagedAllocations > Pool->PeakPagedAllocations ) {
        Pool->PeakPagedAllocations = Pool->PagedAllocations;
    }

    Pool->PagedLock.Release();
}

VOID
__inline
FxPoolRemovePagedAllocateTracker(
    __in PFX_POOL_TRACKER  Tracker
    )
/*++

Routine Description:

    Decommission a Tracker for a Paged allocation.

Arguments:

    Tracker - Pointer to the formatted FX_POOL_TRACKER structure.

Returns:

    VOID

--*/
{
    Tracker->Pool->PagedLock.Acquire();

    RemoveEntryList(&Tracker->Link);

    Tracker->Pool->PagedBytes -= Tracker->Size;
    Tracker->Pool->PagedAllocations--;

    Tracker->Pool->PagedLock.Release();
}

#endif // __FX_POOL_INLINES_HPP__
