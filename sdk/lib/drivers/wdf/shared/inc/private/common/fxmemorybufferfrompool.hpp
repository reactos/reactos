
/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryBufferBif.hpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXMEMORYBUFFERFROMPOOL_H_
#define _FXMEMORYBUFFERFROMPOOL_H_

class FxMemoryBufferFromPool : public FxMemoryObject {

public:
    FxMemoryBufferFromPool(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in size_t BufferSize
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
        __in POOL_TYPE PoolType,
        __in ULONG PoolTag,
        __in size_t BufferSize,
        __out FxMemoryObject** Buffer
        );

    virtual
    PVOID
    GetBuffer(
        VOID
        )
    {
        return m_Pool;
    }

    BOOLEAN
    AllocateBuffer(
        __in POOL_TYPE Type,
        __in ULONG Tag
        )
    {
        m_Pool = MxMemory::MxAllocatePoolWithTag(Type, GetBufferSize(), Tag);
        return  m_Pool != NULL ? TRUE : FALSE;
    }

protected:
    FxMemoryBufferFromPool(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in size_t BufferSize,
        __in USHORT ObjectSize
        );

    ~FxMemoryBufferFromPool();

    PVOID m_Pool;
};

class FxMemoryPagedBufferFromPool : public FxMemoryBufferFromPool {
public:
    FxMemoryPagedBufferFromPool(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in size_t BufferSize,
        __in CfxDeviceBase* DeviceBase
        ) : FxMemoryBufferFromPool(FxDriverGlobals, BufferSize, sizeof(*this))
    {
        SetDeviceBase(DeviceBase);
    }
};

#endif //  _FXMEMORYBUFFERFROMPOOL_H_
