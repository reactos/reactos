/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequestOutputBuffer.cpp

Abstract:

    This module implements class representing the output buffer in an FxRequest

Author:



Environment:

    Both kernel and user mode

Revision History:




--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
// #include "FxRequestOutputBuffer.tmh"
}

PVOID
FxRequestOutputBuffer::GetBuffer(
    VOID
    )
/*++

Routine Description:
    Returns the output buffer that has been cached away by the call to SetBuffer()

Arguments:
    None

Return Value:
    Valid memory or NULL on error

  --*/
{
    FxIrp* irp = GetRequest()->GetFxIrp();

    ASSERT(irp->GetMajorFunction() == IRP_MJ_DEVICE_CONTROL ||
           irp->GetMajorFunction() == IRP_MJ_INTERNAL_DEVICE_CONTROL);

    switch (irp->GetParameterIoctlCodeBufferMethod()) {
    case METHOD_BUFFERED:
        //
        // For buffered ioctls, input and output buffer pointers are same.
        //
        return m_Buffer;

    case METHOD_IN_DIRECT:
    case METHOD_OUT_DIRECT:
        //
        // FxRequest::GetDeviceControlOutputMemoryObject has already called
        // MmGetSystemAddressForMdlSafe and returned success, so we know that
        // we can safely call MmGetSystemAddressForMdlSafe again to get a
        // valid VA pointer.
        //
        return Mx::MxGetSystemAddressForMdlSafe(m_Mdl, NormalPagePriority);

    case METHOD_NEITHER:
        return m_Buffer;

    default:
        ASSERT(FALSE);
        return NULL;
    }
}

size_t
FxRequestOutputBuffer::GetBufferSize(
    VOID
    )
/*++

Routine Description:
    Returns the size of the buffer returned by GetBuffer()

Arguments:
    None

Return Value:
    Buffer length or 0 on error

  --*/
{
    FxIrp* irp = GetRequest()->GetFxIrp();

    ASSERT(irp->GetMajorFunction() == IRP_MJ_DEVICE_CONTROL ||
           irp->GetMajorFunction() == IRP_MJ_INTERNAL_DEVICE_CONTROL);

    return irp->GetParameterIoctlOutputBufferLength();
}

_Must_inspect_result_
PMDL
FxRequestOutputBuffer::GetMdl(
    VOID
    )
/*++

Routine Description:
    Returns the PMDL from the irp if one exists, otherwise NULL

Arguments:
    None

Return Value:
    a valid PMDL or NULL (not an error condition)

  --*/
{
    FxIrp* irp = GetRequest()->GetFxIrp();

    ASSERT(irp->GetMajorFunction() == IRP_MJ_DEVICE_CONTROL ||
           irp->GetMajorFunction() == IRP_MJ_INTERNAL_DEVICE_CONTROL);

    switch (irp->GetParameterIoctlCodeBufferMethod()) {
    case METHOD_IN_DIRECT:
    case METHOD_OUT_DIRECT:
        return m_Mdl;

    default:
        return NULL;
    }
}

WDFMEMORY
FxRequestOutputBuffer::GetHandle(
    VOID
    )
/*++

Routine Description:
    Returns the handle that will represent this object to the driver writer.

Arguments:
    None

Return Value:
    Valid WDF handle

  --*/
{
    return GetRequest()->GetMemoryHandle(
        FIELD_OFFSET(FxRequest, m_OutputBufferOffset));
}

USHORT
FxRequestOutputBuffer::GetFlags(
    VOID
    )
/*++

Routine Description:
    Returns the flags associated with this buffer.  This currently only includes
    whether the buffer is read only or not

Arguments:
    None

Return Value:
    flags from IFxMemoryFlags

  --*/
{
    FxIrp* irp = GetRequest()->GetFxIrp();

    ASSERT(irp->GetMajorFunction() == IRP_MJ_DEVICE_CONTROL ||
           irp->GetMajorFunction() == IRP_MJ_INTERNAL_DEVICE_CONTROL);

    switch (irp->GetParameterIoctlCodeBufferMethod()) {
    case METHOD_IN_DIRECT:
        return IFxMemoryFlagReadOnly;

    case METHOD_BUFFERED:
    case METHOD_OUT_DIRECT:
    case METHOD_NEITHER: // since it is neither, we can't tell, so leave it as 0
    default:
        return 0;
    }
}

PFX_DRIVER_GLOBALS
FxRequestOutputBuffer::GetDriverGlobals(
    VOID
    )
/*++

Routine Description:
    Returns the driver globals

Arguments:
    none

Return Value:
    Driver globals pointer

  --*/
{
    return GetRequest()->GetDriverGlobals();
}

ULONG
FxRequestOutputBuffer::AddRef(
    __in PVOID Tag,
    __in LONG Line,
    __in_opt PSTR File
    )
/*++

Routine Description:
    Adds an irp reference to the owning FxRequest.  This object does not maintain
    its own reference count.  A request cannot be completed with outstanding
    irp references.

Arguments:
    Tag - the tag to use to track the reference

    Line - The line number of the caller

    File - the file name of the caller

Return Value:
    current reference count

  --*/
{
    UNREFERENCED_PARAMETER(Tag);
    UNREFERENCED_PARAMETER(Line);
    UNREFERENCED_PARAMETER(File);

    GetRequest()->AddIrpReference();

    return 2;
}

ULONG
FxRequestOutputBuffer::Release(
    __in PVOID Tag,
    __in LONG Line,
    __in_opt PSTR File
    )
/*++

Routine Description:
    Removes an irp reference to the owning FxRequest.  This object does not maintain
    its own reference count.  A request cannot be completed with outstanding
    irp references.

Arguments:
    Tag - the tag to use to track the release

    Line - The line number of the caller

    File - the file name of the caller

Return Value:
    current reference count

  --*/
{
    UNREFERENCED_PARAMETER(Tag);
    UNREFERENCED_PARAMETER(Line);
    UNREFERENCED_PARAMETER(File);

    GetRequest()->ReleaseIrpReference();

    return 1;
}

FxRequest*
FxRequestOutputBuffer::GetRequest(
    VOID
    )
/*++

Routine Description:
    Return the owning FxRequest based on this object's address

Arguments:
    None

Return Value:
    owning FxRequest

  --*/
{
    return CONTAINING_RECORD(this, FxRequest, m_OutputBuffer);
}

VOID
FxRequestOutputBuffer::Delete(
    VOID
    )
/*++

Routine Description:
    Attempt to delete this interface.  Since this is an embedded object, it
    cannot be deleted.  Since this function is only called internally, the
    internal caller knows if the IFxMemory is deletable because the internal
    caller allocated the IFxMemory to begin with.

Arguments:
    None

Return Value:
    None

  --*/
{
    // this function should never be called
    ASSERT(FALSE);
}
