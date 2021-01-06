/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryObject.hpp

Abstract:
    This is the base class for a whole host of memory objects which serve
    different purposes:  The hierarchy is this

    FxMemoryObject : public FxObject, public IFxMemory
        FxMemoryBuffer
        FxMemoryBufferFromPool
            FxMemoryPagedBufferFromPool
        FxMemoryBufferFromLookaside
            FxMemoryBufferFromPoolLookaside
                FxMemoryPagedBufferFromPoolLookaside
        FxMemoryBufferPreallocated
            FxRequestMemory

    Here is quick overview of what each object does/brings to the table:

    FxMemoryObject:  provides an implementation for almost all of IFxMemory's
        abstract functions.  It also provides a static _Create function which
        can create non lookaside based memory objects

    FxMemoryBuffer:  memory object which can hold the external buffer within its
        own object allocation.  This means that the sizeof(FxMemoryBuffer) +
        external bufer size + extra computations < PAGE_SIZE

    FxMemoryBufferFromPool:  memory object whose external buffer is a separate
        allocation.  The external buffer can be either paged or non paged.

    FxMemoryPagedBufferFromPool:  same as FxMemoryBufferFromPool.  In addition,
        it has a back pointer to the owning FxDeviceBase so that the object
        can be disposed on the correct dispose list.

    FxMemoryBufferFromLookaside:  same as FxMemoryBuffer. In addition, the
        object knows how to return itself to the lookaside which created it.

    FxMemoryBufferFromPoolLookaside:  same as FxMemoryBufferFromPool.  In
        addtion, the object knows how to return itself to the lookaside which
        created it.

    FxMemoryPagedBufferFromPoolLookaside:  same as
        FxMemoryBufferFromPoolLookaside.  In addtion, it has a back pointer to
        the owning FxDeviceBase so that the object can be disposed on the
        correct dispose list.

    FxMemoryBufferPreallocated:  a memory object which does not own the external
        buffer's lifetime.  It is a wrapper object to allow existing buffers to
        be used as a WDFMEMORY.

    FxRequestMemory:  same as FxMemoryBufferPreallocated.  In addtion, it knows
        about PMDLs and is tied to the lifetime of a WDFREQUEST.

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXMEMORYOBJECT_H_
#define _FXMEMORYOBJECT_H_

class FxMemoryObject : public FxObject, public IFxMemory {
public:
    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS DriverGlobals,
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
        __in POOL_TYPE PoolType,
        __in ULONG PoolTag,
        __in size_t BufferSize,
        __out FxMemoryObject** Object
        );

    // begin IFxMemory derivations
    virtual
    size_t
    GetBufferSize(
        VOID
        )
    {
        return m_BufferSize;
    }

    virtual
    PMDL
    GetMdl(
        VOID
        )
    {
        //
        // This function or its derivatives do not allocate the PMDL, they just
        // return one if there is one in the object already.
        //
        return NULL;
    }

    virtual
    WDFMEMORY
    GetHandle(
        VOID
        )
    {
        return (WDFMEMORY) GetObjectHandle();
    }

    virtual
    PFX_DRIVER_GLOBALS
    GetDriverGlobals(
        VOID
        )
    {
        return FxObject::GetDriverGlobals();
    }

    virtual
    ULONG
    AddRef(
        __in PVOID Tag,
        __in LONG Line,
        __in_opt PSTR File
        )
    {
        return FxObject::AddRef(Tag, Line, File);
    }

    virtual
    ULONG
    Release(
        __in PVOID Tag,
        __in LONG Line,
        __in_opt PSTR File
        )
    {
        return FxObject::Release(Tag, Line, File);
    }

    virtual
    VOID
    Delete(
        VOID
        )
    {
        DeleteObject();
    }

    virtual
    USHORT
    GetFlags(
        VOID
        )
    {
        //
        // By default, memory is readable and writable
        return 0;
    }
    // end IFxMemory derivations

protected:

    FxMemoryObject(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in size_t BufferSize
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    QueryInterface(
        __inout FxQueryInterfaceParams* Params
        )
    {
        switch (Params->Type) {
        case IFX_TYPE_MEMORY:
            // cast is necessary for necessary this pointer offset to be computed
            *Params->Object = (IFxMemory*) this;
            return STATUS_SUCCESS;

        default:
            return FxObject::QueryInterface(Params); // __super call
        }
    }

    size_t m_BufferSize;
};

#endif // _FXMEMORYOBJECT_H_
