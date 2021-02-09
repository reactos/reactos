/*++

Copyright (c) Microsoft Corporation

Module Name:

    UsbStubUm.cpp

Abstract:

Author:

Environment:


Revision History:

--*/

extern "C" {
#define INITGUID
#include <initguid.h>

// {B1A96A13-3DE0-4574-9B01-C08FEAB318D6}
DEFINE_GUID(USB_BUS_INTERFACE_USBDI_GUID,
0xb1a96a13, 0x3de0, 0x4574, 0x9b, 0x1, 0xc0, 0x8f, 0xea, 0xb3, 0x18, 0xd6);

}

#include "fxusbpch.hpp"

VOID
USBD_UrbFree(
    _In_ USBD_HANDLE USBDHandle,
    _In_ PURB        Urb
)
{
    UNREFERENCED_PARAMETER(USBDHandle);
    UNREFERENCED_PARAMETER(Urb);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}


NTSTATUS
USBD_UrbAllocate(
    _In_                            USBD_HANDLE  USBDHandle,
    _Outptr_result_bytebuffer_(sizeof(URB)) PURB        *Urb
)
{
    UNREFERENCED_PARAMETER(USBDHandle);
    UNREFERENCED_PARAMETER(Urb);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
USBD_IsochUrbAllocate(
    _In_      USBD_HANDLE  USBDHandle,
    _In_      ULONG        NumberOfIsochPacket,
    _Outptr_result_bytebuffer_(sizeof(struct _URB_ISOCH_TRANSFER)
                               + (NumberOfIsochPackets * sizeof(USBD_ISO_PACKET_DESCRIPTOR))
                               - sizeof(USBD_ISO_PACKET_DESCRIPTOR))
              PURB        *Urb
)
{
    UNREFERENCED_PARAMETER(USBDHandle);
    UNREFERENCED_PARAMETER(NumberOfIsochPacket);
    UNREFERENCED_PARAMETER(Urb);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_SUCCESS;
}


VOID
USBD_CloseHandle(
    _In_      USBD_HANDLE USBDHandle
)
{
    UNREFERENCED_PARAMETER(USBDHandle);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

NTSTATUS
USBD_CreateHandle(
    _In_      PDEVICE_OBJECT DeviceObject,
    _In_      PDEVICE_OBJECT TargetDeviceObject,
    _In_      ULONG          USBDClientContractVersion,
    _In_      ULONG          PoolTag,
    _Out_     USBD_HANDLE   *USBDHandle
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(TargetDeviceObject);
    UNREFERENCED_PARAMETER(USBDClientContractVersion);
    UNREFERENCED_PARAMETER(PoolTag);
    UNREFERENCED_PARAMETER(USBDHandle);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_SUCCESS;

}

NTSTATUS
USBD_QueryUsbCapability(
    _In_ USBD_HANDLE USBDHandle,
    _In_ const GUID* CapabilityType,
    _In_ ULONG       OutputBufferLength,
    _When_(OutputBufferLength == 0, _Pre_null_)
    _When_(OutputBufferLength != 0 && ResultLength == NULL, _Out_writes_bytes_(OutputBufferLength))
    _When_(OutputBufferLength != 0 && ResultLength != NULL, _Out_writes_bytes_to_opt_(OutputBufferLength, *ResultLength))
        PUCHAR                        OutputBuffer,
    _Out_opt_
    _When_(ResultLength != NULL, _Deref_out_range_(<=,OutputBufferLength))
        PULONG                        ResultLength
)
{
    UNREFERENCED_PARAMETER(USBDHandle);
    UNREFERENCED_PARAMETER(CapabilityType);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(ResultLength);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_SUCCESS;
}

VOID
USBD_AssignUrbToIoStackLocation(
    _In_ USBD_HANDLE        USBDHandle,
    _In_ PIO_STACK_LOCATION IoStackLocation,
    _In_ PURB               Urb
)
{
    UNREFERENCED_PARAMETER(USBDHandle);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(Urb);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}
