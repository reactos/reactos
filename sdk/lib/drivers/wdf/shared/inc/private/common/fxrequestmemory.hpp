
/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxRequestMemory.hpp

Abstract:

    This is the memory object for FxRequest that is sized, and
    allows checking for read/write access.

    It's reference lifetime is tied with IRP completion in
    FxRequest.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXREQUESTMEMORY_H_
#define _FXREQUESTMEMORY_H_

class FxRequestMemory : public FxMemoryBufferPreallocated {
public:

    // Factory function
    static
    NTSTATUS
    Create(
        __in PFX_DRIVER_GLOBALS DriverGlobals,
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
        __out FxRequestMemory** Object
        );

    FxRequestMemory(
        __in PFX_DRIVER_GLOBALS Globals
        );

    // begin end FxMemoryObject
    virtual
    PVOID
    GetBuffer(
        VOID
        );

    _Must_inspect_result_
    virtual
    PMDL
    GetMdl(
        VOID
        );

    virtual
    USHORT
    GetFlags(
        VOID
        )
    {
        return m_Flags;
    }
    // end FxMemoryObject overrides

    VOID
    SetBuffer(
        _In_ FxRequest* Request,
        _Pre_notnull_ _Pre_writable_byte_size_(BufferSize) PVOID      Buffer,
        _In_ PMDL       BackingMdl,
        _In_ size_t     BufferSize,
        _In_ BOOLEAN    ReadOnly
        );

    VOID
    SetMdl(
        __in FxRequest* Request,
        __in PMDL       Mdl,
        __in PVOID      MdlBuffer,
        __in size_t     BufferSize,
        __in BOOLEAN    ReadOnly
        );

    _Must_inspect_result_
    NTSTATUS
    QueryInterface(
        __in FxQueryInterfaceParams* Params
        );

    ~FxRequestMemory(
        VOID
        );

protected:
    VOID
    SetFlags(
        __in USHORT Flags
        )
    {
        m_Flags = Flags;
    }

protected:

    FxRequest* m_Request;

    //
    // The m_Mdl is owned by this object and it must be freed by the this object.
    //
    PMDL       m_Mdl;

    USHORT m_Flags;
};

#endif //  _FXREQUESTMEMORY_H_


