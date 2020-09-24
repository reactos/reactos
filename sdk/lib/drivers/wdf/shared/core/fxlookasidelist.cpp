/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxLookasideList.cpp

Abstract:

    This module implements a frameworks managed FxLookasideList

Author:


Environment:

    kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"
#include "FxLookasideList.hpp"

FxLookasideList::FxLookasideList(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ObjectSize,
    __in ULONG PoolTag
    ) :
    FxObject(FX_TYPE_LOOKASIDE, ObjectSize, FxDriverGlobals),
    m_BufferSize(0), m_PoolTag(PoolTag), m_MemoryObjectSize(0)
/*++

Routine Description:
    Constructor for FxLookasideList

Arguments:
    ObjectSize - Size of the derived object.

    PoolTag - Tag to use when allocating memory.

Return Value:
    None

  --*/
{
}

FxLookasideList::~FxLookasideList()
/*++

Routine Description:
    Destructor for FxLookasideList.  Default implementation does nothing.
    Derived classes will call the appropriate NTOS export to remove itself from
    the list of lookaside lists.

Arguments:
    None

Return Value:
    None

  --*/
{
}

_Must_inspect_result_
NTSTATUS
FxLookasideList::InitializeLookaside(
    __in USHORT BufferSize,
    __in USHORT MemoryObjectSize,
    __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
    )
/*++

Routine Description:
    Computes the memory object size to be used by the derived class.  This
    function handles the overflow in computing the size and returns an error
    if that occurs.

Arguments:
    BufferSize - the length of the buffer being allocated alongside the object

    MemoryObjectSize - the raw size of the FxObject derived object, ie
        sizeof(FxMemoryBufferFromLookaside)

    MemoryAttributes - attributes to be associated for each memory object created

Return Value:
    None

  --*/
{
    size_t size;
    NTSTATUS status;

    if (MemoryAttributes != NULL) {
        RtlCopyMemory(&m_MemoryAttributes,
                      MemoryAttributes,
                      sizeof(m_MemoryAttributes));
    }
    else {
        RtlZeroMemory(&m_MemoryAttributes, sizeof(m_MemoryAttributes));
    }

    status = FxCalculateObjectTotalSize(GetDriverGlobals(),
                                        MemoryObjectSize,
                                        BufferSize,
                                        &m_MemoryAttributes,
                                        &size);

    if (!NT_SUCCESS(status)) {
        //
        // FxCalculateObjectTotalSize logs an error  to the IFR
        //
        return status;
    }

    status = FxPoolAddHeaderSize(GetDriverGlobals(), size, &size);
    if (!NT_SUCCESS(status)) {
        //
        // FxPoolAddHeaderSize logs to the IFR on error
        //
        return status;
    }

    //
    // It is *required* to set these values only once we know we are returning
    // success b/c the derived classes use == 0 as an indication that Initialization
    // failed and that any associated lookaside lists were not initialized.
    //
    m_MemoryObjectSize = size;
    m_BufferSize = BufferSize;

    return status;
}

#pragma prefast(push)



//This routine intentionally accesses the header of the allocated memory.
#pragma prefast(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY)
PVOID
FxLookasideList::InitObjectAlloc(
    __out_bcount(this->m_MemoryObjectSize) PVOID Alloc
    )
/*++

Routine Description:
    Initializes the object allocation so that it can be tracked and inserted
    in this drivers POOL.

Arguments:
    Alloc - the raw allocation

Return Value:
    the start of where the object memory should be, not necessarily == Alloc

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    PFX_POOL_HEADER pHeader;
    PFX_POOL_TRACKER tracker;

    pFxDriverGlobals = GetDriverGlobals();

    RtlZeroMemory(Alloc, m_MemoryObjectSize);

    if (pFxDriverGlobals->IsPoolTrackingOn())  {
        //
        // PoolTracking is active, so format and insert
        // a tracker in the NonPagedHeader list of the pool.
        //
        tracker = (PFX_POOL_TRACKER) Alloc;
        pHeader  = WDF_PTR_ADD_OFFSET_TYPE(Alloc,
                                           sizeof(FX_POOL_TRACKER),
                                           PFX_POOL_HEADER);

        pHeader->Base = Alloc;
        pHeader->FxDriverGlobals = pFxDriverGlobals;

        FxPoolInsertNonPagedAllocateTracker(
            &pFxDriverGlobals->FxPoolFrameworks,
            tracker,
            m_BufferSize,
            m_PoolTag,
            _ReturnAddress());
    }
    else {
        //
        // PoolTracking is inactive, only format FX_POOL_HEADER area.
        //
        pHeader = (PFX_POOL_HEADER) Alloc;
        pHeader->Base = Alloc;
        pHeader->FxDriverGlobals = pFxDriverGlobals;
    }

    return &pHeader->AllocationStart[0];
}
#pragma prefast(pop)

VOID
FxLookasideList::_Reclaim(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __inout PNPAGED_LOOKASIDE_LIST List,
    __in FxMemoryBufferFromLookaside* Memory
    )
{
    PFX_POOL_HEADER pHeader;

    pHeader = FxObject::_CleanupPointer(FxDriverGlobals, (FxObject*) Memory);

    FxFreeToNPagedLookasideList(List, pHeader->Base);
}
