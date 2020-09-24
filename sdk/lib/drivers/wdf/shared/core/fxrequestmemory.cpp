
/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxRequestMemory.hpp

Abstract:

    This is the memory object for FxRequest that is sized, and
    allows checking for read/write access.

    Its lifetime reference is tied with IRP completion in
    FxRequest.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "coreprivshared.hpp"

//
// FxRequestMemory can be an embedded object inside of FxRequest,
// in which it represents the standard WDM IRP buffers.
//
// In addition, FxRequestMemory may be allocated by the device
// driver by probing and locking pages for dealing with direct
// and method_neither I/O types.
//
// In both cases, the lifetime of a WDFMEMORY handle returned from
// FxRequestMemory becomes invalid once FxRequest::Complete is called.
//
// Code exists to assist FxRequest in verifying there are no out
// standing references on the WDFMEMORY handles when a request is
// completed.
//
// Rundown and disposing of FxRequestMemory occur during FxRequest::Complete,
// and not when FxRequest is actually destroyed. This is because they represent
// system buffers and MDLs which must be released before the IRP inside
// the FxRequest is completed. Otherwise an I/O manager bugcheck will occur.
//

NTSTATUS
FxRequestMemory::Create(
    __in PFX_DRIVER_GLOBALS DriverGlobals,
    __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
    __out FxRequestMemory** Object
    )

/*++

Routine Description:

    Class Factory for FxRequestMemory

--*/

{
    FxRequestMemory* pMemory;

    // Build an FxRequestMemory object now
    pMemory = new(DriverGlobals, Attributes) FxRequestMemory(DriverGlobals);

    if (pMemory == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Object = pMemory;

    return STATUS_SUCCESS;
}

FxRequestMemory::FxRequestMemory(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxMemoryBufferPreallocated(sizeof(*this), FxDriverGlobals)
/*++

Routine Description:

    Default constructor for this object.

  --*/
{

    //
    // Our constructor chain:
    //
    // FxRequestMemory(ObjectSize, PFX_DRIVER_GLOBALS),
    //     : FxMemoryBufferPreallocated(ObjectSize, PFX_DRIVER_GLOBALS),
    //     : FxMemoryObject(PFX_DRIVER_GLOBALS, ObjectSize, ObjectSize),
    //     : FxObject(FX_TYPE_MEMORY, ObjectSize, PFX_DRIVER_GLOBALS)
    //
    m_Request = NULL;

    m_Mdl = NULL;

    m_Flags = 0;
}


FxRequestMemory::~FxRequestMemory(
    VOID
    )
/*++

Routine Description:
    Destructor for this object.  Does nothing with the client memory since
    the client owns it.

Arguments:
    None

Return Value:
    None

  --*/
{
    //
    // Non-embedded case releases resources in the destructor
    // rather than Dispose to ensure all outstanding device driver
    // references are gone.
    //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    if( m_Mdl != NULL ) {
        Mx::MxUnlockPages(m_Mdl);
        FxMdlFree(GetDriverGlobals(), m_Mdl);
        m_Mdl = NULL;
    }
#endif

    if( m_Request != NULL ) {
        m_Request->ReleaseIrpReference();
        m_Request = NULL;
    }
}

_Must_inspect_result_
NTSTATUS
FxRequestMemory::QueryInterface(
    __in FxQueryInterfaceParams* Params
    )
{
    if (Params->Type == FX_TYPE_REQUEST_MEMORY) {
        *Params->Object = (FxRequestMemory*) this;
        return STATUS_SUCCESS;
    }
    else {
        return __super::QueryInterface(Params);
    }
}

PVOID
FxRequestMemory::GetBuffer(
    VOID
    )
/*++

Routine Description:
    GetBuffer overload.

    Returns pointer into buffer memory.

Arguments:
    None

Return Value:
    Client supplied buffer from the constructor

  --*/
{
    return m_pBuffer;
}

_Must_inspect_result_
PMDL
FxRequestMemory::GetMdl(
    VOID
    )
/*++

Routine Description:
    GetMdl overload.  Returns the embedded MDL

Arguments:
    None

Return Value:
    valid MDL or NULL

  --*/
{
    return m_Mdl;
}

VOID
FxRequestMemory::SetBuffer(
    _In_ FxRequest* Request,
    _Pre_notnull_ _Pre_writable_byte_size_(BufferSize) PVOID      Buffer,
    _In_ PMDL       BackingMdl,
    _In_ size_t     BufferSize,
    _In_ BOOLEAN    ReadOnly
    )
/*++

Routine Description:
    Updates the internal pointer to a new value.


Arguments:
    Request - new Request.

    Buffer - new buffer

    BackingMdl - associated MDL.

    BufferSize - length of Buffer in bytes

    ReadOnly - TRUE if read only buffer.

Return Value:
    None.

  --*/

{
    ASSERT(m_pBuffer == NULL);
    ASSERT(m_Request == NULL);
    ASSERT(m_Mdl == NULL);

    ASSERT(Request != NULL);

    m_pBuffer = Buffer;
    m_Mdl = BackingMdl;
    m_BufferSize = BufferSize;

    m_Request = Request;

    //
    // A single FxRequest IRP reference
    // is outstanding until destroy
    //
    m_Request->AddIrpReference();

    // Set access checking if its a readonly buffer
    if (ReadOnly) {
        SetFlags(IFxMemoryFlagReadOnly);
    }
}

VOID
FxRequestMemory::SetMdl(
    __in FxRequest* Request,
    __in PMDL       Mdl,
    __in PVOID      MdlBuffer,
    __in size_t     BufferSize,
    __in BOOLEAN    ReadOnly
    )
/*++

Routine Description:

    Sets an MDL on the WDFMEMORY object.

    It can't already have a buffer set.

    We are expected to free the MDL when Disposed.

    This is not used by embedded WDFMEMORY objects whose
    MDL comes from the IRP.

Arguments:
    Request - new Request.

    Mdl - new MDL.

    MdlBuffer - associated buffer

    BufferSize - length of Buffer in bytes

    ReadOnly - TRUE if read only buffer.

Return Value:
    None.

  --*/

{

    ASSERT(m_pBuffer == NULL);
    ASSERT(m_Mdl == NULL);
    ASSERT(m_Request == NULL);
    ASSERT(Request != NULL);

    m_Mdl = Mdl;
    m_pBuffer = MdlBuffer;
    m_BufferSize = BufferSize;

    m_Request = Request;

    //
    // A single FxRequest IRP reference is outstanding until destroy
    //
    m_Request->AddIrpReference();

    // Set access checking if it's a readonly buffer
    if (ReadOnly) {
        SetFlags(IFxMemoryFlagReadOnly);
    }

    return;
}


