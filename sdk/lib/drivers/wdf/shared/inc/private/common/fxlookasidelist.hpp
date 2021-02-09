/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxLookasideList.hpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:


--*/

#ifndef _FXLOOKASIDELIST_H_
#define _FXLOOKASIDELIST_H_

class FxLookasideList : public FxObject {

    friend FxMemoryBufferFromLookaside;

public:
    FxLookasideList(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in ULONG PoolTag
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in size_t BufferSize,
        __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
        ) =0;

    virtual
    _Must_inspect_result_
    NTSTATUS
    Allocate(
        __out FxMemoryObject** PPMemory
        ) =0;

    size_t
    GetBufferSize(
        VOID
        )
    {
        return m_BufferSize;
    }

protected:
    virtual
    ~FxLookasideList(
        );

    //
    // Function used by IFxMemoryBuffer to return itself to the lookaside list
    //
    virtual
    VOID
    Reclaim(
        __in FxMemoryBufferFromLookaside* Memory
        ) =0;

    _Must_inspect_result_
    NTSTATUS
    InitializeLookaside(
        __in USHORT BufferSize,
        __in USHORT MemoryObjectSize,
        __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
        );

    PVOID
    InitObjectAlloc(
        __out_bcount(this->m_MemoryObjectSize) PVOID Alloc
        );

    static
    VOID
    _Reclaim(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __inout PNPAGED_LOOKASIDE_LIST List,
        __in FxMemoryBufferFromLookaside* Memory
        );

public:
    WDF_OBJECT_ATTRIBUTES m_MemoryAttributes;

protected:
    size_t m_BufferSize;

    size_t m_MemoryObjectSize;

    ULONG  m_PoolTag;
};

class FxLookasideListFromPool : public FxLookasideList {
    friend FxMemoryBufferFromPoolLookaside;

public:
    FxLookasideListFromPool(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in ULONG PoolTag
        ) : FxLookasideList(FxDriverGlobals, ObjectSize, PoolTag)
    {
    }

protected:
    virtual
    VOID
    ReclaimPool(
        __inout PVOID Pool
        ) =0;
};


#endif // _FXLOOKASIDELIST_H_
