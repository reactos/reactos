#include "common/fxrequest.h"
#include "common/fxdevice.h"



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

        if (pDevice->GetIoType() == WdfDeviceIoDirect)
        {
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