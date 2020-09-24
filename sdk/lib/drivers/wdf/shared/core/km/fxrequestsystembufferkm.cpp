/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequestSystemBuffer.cpp

Abstract:

    This module implements class representing the system buffer in an FxRequest

Author:




Environment:

    Kernel mode only

Revision History:




--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
#include "FxRequestSystemBufferKm.tmh"
}

_Must_inspect_result_
PVOID
FxRequestSystemBuffer::GetBuffer(
    VOID
    )
/*++

Routine Description:
    Returns the system buffer that has been cached away by the call to SetBuffer()

Arguments:
    None

Return Value:
    Valid memory or NULL on error

  --*/
{
    FxDevice* pDevice;
    FxIrp* irp = GetRequest()->GetFxIrp();
    WDF_DEVICE_IO_TYPE ioType;

    switch (irp->GetMajorFunction()) {
    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        return m_Buffer;

    case IRP_MJ_READ:
    case IRP_MJ_WRITE:
        pDevice = FxDevice::GetFxDevice(irp->GetDeviceObject());
        ioType = pDevice->GetIoType();

        switch (ioType) {
        case WdfDeviceIoBuffered:
            return m_Buffer;

        case WdfDeviceIoDirect:
            //
            // FxRequest::GetMemoryObject has already called MmGetSystemAddressForMdlSafe
            // and returned success, so we know that we can safely call
            // MmGetSystemAddressForMdlSafe again to get a valid VA pointer.
            //
            return Mx::MxGetSystemAddressForMdlSafe(m_Mdl, NormalPagePriority);

        case WdfDeviceIoNeither:
            return m_Buffer;

        default:
            ASSERT(FALSE);
            return NULL;
        }

    default:
        ASSERT(FALSE);
        return NULL;
    }
}

