/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequestSystemBuffer.cpp

Abstract:

    This module implements class representing the system buffer in an FxRequest

Author:




Environment:

    Both kernel and user mode

Revision History:




--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
// #include "FxRequestSystemBuffer.tmh"
}

size_t
FxRequestSystemBuffer::GetBufferSize(
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

    switch (irp->GetMajorFunction()) {
    case IRP_MJ_READ:
        return irp->GetParameterReadLength();

    case IRP_MJ_WRITE:
        return irp->GetParameterWriteLength();

    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        return irp->GetParameterIoctlInputBufferLength();

    default:
        // should not get here
        ASSERT(FALSE);
        return 0;
    }
}

_Must_inspect_result_
PMDL
FxRequestSystemBuffer::GetMdl(
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
    FxDevice* pDevice;
    FxIrp* irp = GetRequest()->GetFxIrp();

    switch (irp->GetMajorFunction()) {
    case IRP_MJ_READ:
    case IRP_MJ_WRITE:
        pDevice = FxDevice::GetFxDevice(irp->GetDeviceObject());

        if (pDevice->GetIoType() == WdfDeviceIoDirect) {
            return m_Mdl;
        }
        //    ||  ||       Fall through  ||   ||
        //    \/  \/                     \/   \/

    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        //
        // For IOCLs, the outbuffer will return the PMDL
        //
        //    ||  ||       Fall through  ||   ||
        //    \/  \/                     \/   \/

    default:
        return NULL;
    }
}

WDFMEMORY
FxRequestSystemBuffer::GetHandle(
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
        FIELD_OFFSET(FxRequest, m_SystemBufferOffset));
}

USHORT
FxRequestSystemBuffer::GetFlags(
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

    switch (irp->GetMajorFunction()) {
    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        switch (irp->GetParameterIoctlCodeBufferMethod()) {
        case METHOD_BUFFERED:
        case METHOD_NEITHER:
            return 0;

        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:
            return IFxMemoryFlagReadOnly;
        }

    case IRP_MJ_READ:
        return 0;

    case IRP_MJ_WRITE:
        return IFxMemoryFlagReadOnly;

    default:
        ASSERT(FALSE);
        return 0;
    }
}

PFX_DRIVER_GLOBALS
FxRequestSystemBuffer::GetDriverGlobals(
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
FxRequestSystemBuffer::AddRef(
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

    //
    // This value should never be used by the caller
    //
    return 2;
}

ULONG
FxRequestSystemBuffer::Release(
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
FxRequestSystemBuffer::GetRequest(
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
    return CONTAINING_RECORD(this, FxRequest, m_SystemBuffer);
}

VOID
FxRequestSystemBuffer::Delete(
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
