/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxNPagedLookasideList.hpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXNPAGEDLOOKASIDELIST_H_
#define _FXNPAGEDLOOKASIDELIST_H_

class FxNPagedLookasideList : public FxLookasideList {
public:
    FxNPagedLookasideList(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in ULONG PoolTag
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in size_t BufferSize,
        __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    Allocate(
        __out FxMemoryObject** PPMemory
        );

protected:
    ~FxNPagedLookasideList();

    virtual
    VOID
    Reclaim(
        __in FxMemoryBufferFromLookaside* Memory
        );

protected:
    NPAGED_LOOKASIDE_LIST m_ObjectLookaside;
};

class FxNPagedLookasideListFromPool : public FxLookasideListFromPool {

    friend FxMemoryBufferFromPoolLookaside;

public:
    FxNPagedLookasideListFromPool(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in ULONG PoolTag
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in size_t BufferSize,
        __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    Allocate(
        __out FxMemoryObject** PPMemory
        );

protected:
    ~FxNPagedLookasideListFromPool(
        VOID
        );

    virtual
    VOID
    Reclaim(
        __in FxMemoryBufferFromLookaside* Memory
        );

    virtual
    VOID
    ReclaimPool(
        __inout PVOID Pool
        )
    {
        FxFreeToNPagedLookasideList(&m_PoolLookaside, Pool);
    }

    NPAGED_LOOKASIDE_LIST m_ObjectLookaside;

    NPAGED_LOOKASIDE_LIST m_PoolLookaside;
};


#endif // __FX_NPAGED_LOOKASIDE_LIST_H__
