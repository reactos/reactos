/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryBufferPreallocated.hpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXMEMORYBUFFERPREALLOCATED_H_
#define _FXMEMORYBUFFERPREALLOCATED_H_

class FxMemoryBufferPreallocated : public FxMemoryObject {
public:

    FxMemoryBufferPreallocated(
        _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
        _Pre_notnull_ _Pre_writable_byte_size_(BufferSize) PVOID Buffer,
        _In_ size_t BufferSize
        );

    virtual
    PVOID
    GetBuffer(
        VOID
        )
    {
        return m_pBuffer;
    }

    VOID
    UpdateBuffer(
        _Pre_notnull_ _Pre_writable_byte_size_(BufferSize) PVOID Buffer,
        _In_ size_t BufferSize
        );

    _Must_inspect_result_
    NTSTATUS
    QueryInterface(
        __inout FxQueryInterfaceParams* Params
        );

protected:
    // for derived classes
    FxMemoryBufferPreallocated(
        __in USHORT ObjectSize,
        __in PFX_DRIVER_GLOBALS Globals
        );

    FxMemoryBufferPreallocated(
        _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
        _In_ USHORT ObjectSize,
        _Pre_notnull_ _Pre_writable_byte_size_(BufferSize) PVOID Buffer,
        _In_ size_t BufferSize
        );

    ~FxMemoryBufferPreallocated();

    PVOID m_pBuffer;
};
#endif //  _FXMEMORYBLOCKPREALLOCATED_H_
