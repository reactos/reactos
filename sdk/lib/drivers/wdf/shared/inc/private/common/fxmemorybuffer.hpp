/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryBuffer.hpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:


--*/

#ifndef _FXMEMORYBUFFER_H_
#define _FXMEMORYBUFFER_H_

class FxMemoryBuffer : public FxMemoryObject {

public:

    // Factory function
    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in  PFX_DRIVER_GLOBALS DriverGlobals,
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
        __in  ULONG PoolTag,
        __in  size_t BufferSize,
        __in POOL_TYPE PoolType,
        __out FxMemoryObject** Object
        );

    FxMemoryBuffer(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in size_t BufferSize
        );

    PVOID
    GetBuffer(
        VOID
        );

    PVOID
    __inline
    operator new(
        __in size_t Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
        __in USHORT ExtraSize,
        __in ULONG Tag,
        __in POOL_TYPE PoolType
        )
    {
        //
        // PoolType must be non paged pool but it can be NonPagedPool
        // or NonPagedPoolNx or their variants
        //
        ASSERT(!FxIsPagedPoolType(PoolType));

        //
        // Specialize operator new so that we can use the caller's tag when
        // making the object allocation vs using the default driver-wide tag.
        //
        return FxObjectHandleAlloc(FxDriverGlobals,
                                   PoolType,
                                   Size,
                                   Tag,
                                   Attributes,
                                   ExtraSize,
                                   FxObjectTypeExternal);
    }

protected:

    FxMemoryBuffer(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in size_t BufferSize
        );

    ~FxMemoryBuffer();
};

#endif // _FXMEMORYBUFFER_H_
