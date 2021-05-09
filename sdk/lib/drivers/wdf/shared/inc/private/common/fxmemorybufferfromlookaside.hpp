/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryBufferFromLookaside.hpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXMEMORYBUFFERFROMLOOKASIDE_H_
#define _FXMEMORYBUFFERFROMLOOKASIDE_H_

class FxMemoryBufferFromLookaside : public FxMemoryObject {

public:
    FxMemoryBufferFromLookaside(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __inout FxLookasideList* Lookaside,
        __in size_t BufferSize
        );

    _Must_inspect_result_
    PVOID
    operator new(
        __in size_t Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __inout PVOID ValidMemory,
        __in size_t BufferSize,
        __in PWDF_OBJECT_ATTRIBUTES Attributes
        );

    virtual
    PVOID
    GetBuffer(
        VOID
        );

protected:
    FxMemoryBufferFromLookaside(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __inout FxLookasideList* Lookaside,
        __in size_t BufferSize,
        __in USHORT ObjectSize
        );

    ~FxMemoryBufferFromLookaside(
        );

    VOID
    Init(
        VOID
        );

    virtual
    VOID
    SelfDestruct(
        VOID
        );

    FxLookasideList* m_pLookaside;
};

class FxMemoryBufferFromPoolLookaside : public FxMemoryBufferFromLookaside {
public:
    FxMemoryBufferFromPoolLookaside(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __inout FxLookasideList* Lookaside,
        __in size_t BufferSize,
        __in_bcount(BufferSize) PVOID Buffer
        );

    _Must_inspect_result_
    PVOID
    operator new(
        __in size_t Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __inout PVOID ValidMemory,
        __in PWDF_OBJECT_ATTRIBUTES Attributes
        );

    virtual
    PVOID
    GetBuffer(
        VOID
        )
    {
        return m_Pool;
    }

protected:
    FxMemoryBufferFromPoolLookaside(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __inout FxLookasideList* Lookaside,
        __in size_t BufferSize,
        __in_bcount(BufferSize) PVOID Buffer,
        __in USHORT ObjectSize
        );

    virtual
    VOID
    SelfDestruct(
        VOID
        );

    PVOID m_Pool;
};

class FxMemoryPagedBufferFromPoolLookaside : public FxMemoryBufferFromPoolLookaside {
public:
    FxMemoryPagedBufferFromPoolLookaside(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __inout FxLookasideList* Lookaside,
        __in size_t BufferSize,
        __in_bcount(BufferSize) PVOID Buffer,
        __in FxDeviceBase* DeviceBase
        ) :  FxMemoryBufferFromPoolLookaside(FxDriverGlobals,
                                             Lookaside,
                                             BufferSize,
                                             Buffer,
                                             sizeof(*this))
    {
        SetDeviceBase(DeviceBase);
    }
};
#endif // _FXMEMORYBUFFERFROMLOOKASIDE_H_
