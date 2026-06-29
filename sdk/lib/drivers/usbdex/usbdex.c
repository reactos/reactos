
#include <ntddk.h>
#include <usb.h>
#include <usbdlib.h>
#include <debug.h>

DECLARE_HANDLE(USBD_HANDLE);

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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
USBD_AssignUrbToIoStackLocation(
    _In_ USBD_HANDLE USBDHandle,
    _In_ PIO_STACK_LOCATION IoStackLocation,
    _In_ PURB Urb
)
{
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
USBD_CloseHandle(
    _In_ USBD_HANDLE USBDHandle
)
{
    UNIMPLEMENTED;
}

NTSTATUS
USBD_UrbAllocate(
    _In_ USBD_HANDLE USBDHandle,
    _Outptr_result_bytebuffer_(sizeof(URB)) PURB *Urb
)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
USBD_IsochUrbAllocate(
    _In_ USBD_HANDLE USBDHandle,
    _In_ ULONG NumberOfIsochPacket,
    _Outptr_result_bytebuffer_(sizeof(struct _URB_ISOCH_TRANSFER) 
                               + (NumberOfIsochPackets * sizeof(USBD_ISO_PACKET_DESCRIPTOR))
                               - sizeof(USBD_ISO_PACKET_DESCRIPTOR)) 
    PURB *Urb
)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
USBD_UrbFree(
    _In_ USBD_HANDLE USBDHandle,
    _In_ PURB Urb
)
{
    UNIMPLEMENTED;
}
