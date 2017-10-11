/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort URB functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbport.h"

#define NDEBUG
#include <debug.h>

#define NDEBUG_USBPORT_URB
#include "usbdebug.h"

NTSTATUS
NTAPI
USBPORT_HandleGetConfiguration(IN PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket;

    DPRINT_URB("USBPORT_HandleGetConfiguration: Urb - %p\n", Urb);

    SetupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)
                   &Urb->UrbControlGetConfigurationRequest.Reserved1;

    SetupPacket->bmRequestType.Dir = BMREQUEST_DEVICE_TO_HOST;
    SetupPacket->bRequest = USB_REQUEST_GET_CONFIGURATION;
    SetupPacket->wValue.W = 0;
    SetupPacket->wIndex.W = 0;
    SetupPacket->wLength = Urb->UrbControlGetConfigurationRequest.TransferBufferLength;

    Urb->UrbControlGetConfigurationRequest.Reserved0 |= USBD_TRANSFER_DIRECTION_IN; // 1;
    Urb->UrbControlGetConfigurationRequest.Reserved0 |= USBD_SHORT_TRANSFER_OK; // 2

    USBPORT_DumpingSetupPacket(SetupPacket);

    USBPORT_QueueTransferUrb(Urb);

    return STATUS_PENDING;
}

NTSTATUS
NTAPI
USBPORT_HandleGetCurrentFrame(IN PDEVICE_OBJECT FdoDevice,
                              IN PIRP Irp,
                              IN PURB Urb)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    ULONG FrameNumber;
    KIRQL OldIrql;

    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);
    FrameNumber = Packet->Get32BitFrameNumber(FdoExtension->MiniPortExt);
    KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);

    Urb->UrbGetCurrentFrameNumber.FrameNumber = FrameNumber;

    DPRINT_URB("USBPORT_HandleGetCurrentFrame: FrameNumber - %p\n",
               FrameNumber);

    return USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_SUCCESS);
}

NTSTATUS
NTAPI
USBPORT_AbortPipe(IN PDEVICE_OBJECT FdoDevice,
                  IN PIRP Irp,
                  IN PURB Urb)
{
    PUSBPORT_ENDPOINT Endpoint;
    PUSBPORT_PIPE_HANDLE PipeHandle;
    PUSBPORT_DEVICE_HANDLE DeviceHandle;
    NTSTATUS Status;

    DPRINT_URB("USBPORT_AbortPipe: ... \n");

    PipeHandle = Urb->UrbPipeRequest.PipeHandle;
    DeviceHandle = Urb->UrbHeader.UsbdDeviceHandle;

    if (USBPORT_ValidatePipeHandle(DeviceHandle, PipeHandle))
    {
        if (!(PipeHandle->Flags & PIPE_HANDLE_FLAG_NULL_PACKET_SIZE))
        {
            Endpoint = PipeHandle->Endpoint;

            Status = STATUS_PENDING;

            Irp->IoStatus.Status = Status;
            IoMarkIrpPending(Irp);

            USBPORT_AbortEndpoint(FdoDevice, Endpoint, Irp);

            return Status;
        }

        Status = USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_SUCCESS);
    }
    else
    {
        Status = USBPORT_USBDStatusToNtStatus(Urb,
                                              USBD_STATUS_INVALID_PIPE_HANDLE);
    }

    return Status;
}

NTSTATUS
NTAPI
USBPORT_ResetPipe(IN PDEVICE_OBJECT FdoDevice,
                  IN PIRP Irp,
                  IN PURB Urb)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    PUSBPORT_PIPE_HANDLE PipeHandle;
    PUSBPORT_ENDPOINT Endpoint;
    NTSTATUS Status;

    DPRINT_URB("USBPORT_ResetPipe: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    PipeHandle = Urb->UrbPipeRequest.PipeHandle;

    if (!USBPORT_ValidatePipeHandle((PUSBPORT_DEVICE_HANDLE)Urb->UrbHeader.UsbdDeviceHandle,
                                    PipeHandle))
    {
        return USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_INVALID_PIPE_HANDLE);
    }

    Endpoint = PipeHandle->Endpoint;

    KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);

    if (IsListEmpty(&Endpoint->TransferList))
    {
        if (Urb->UrbHeader.UsbdFlags & USBD_FLAG_NOT_ISO_TRANSFER)
        {
            KeAcquireSpinLockAtDpcLevel(&FdoExtension->MiniportSpinLock);

            Packet->SetEndpointDataToggle(FdoExtension->MiniPortExt,
                                          Endpoint + 1,
                                          0);

            KeReleaseSpinLockFromDpcLevel(&FdoExtension->MiniportSpinLock);
        }

        Status = USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_SUCCESS);
    }
    else
    {
        Status = USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_ERROR_BUSY);
    }

    Endpoint->Flags |= ENDPOINT_FLAG_QUEUENE_EMPTY;

    KeAcquireSpinLockAtDpcLevel(&FdoExtension->MiniportSpinLock);

    Packet->SetEndpointStatus(FdoExtension->MiniPortExt,
                              Endpoint + 1,
                              USBPORT_ENDPOINT_RUN);

    KeReleaseSpinLockFromDpcLevel(&FdoExtension->MiniportSpinLock);
    KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

    return Status;
}

NTSTATUS
NTAPI
USBPORT_ClearStall(IN PDEVICE_OBJECT FdoDevice,
                   IN PIRP Irp,
                   IN PURB Urb)
{
    PUSBPORT_DEVICE_HANDLE DeviceHandle;
    PUSBPORT_PIPE_HANDLE PipeHandle;
    USBD_STATUS USBDStatus;
    PUSBPORT_ENDPOINT Endpoint;
    NTSTATUS Status;
    USB_DEFAULT_PIPE_SETUP_PACKET SetupPacket;

    DPRINT_URB("USBPORT_ClearStall: ... \n");

    PipeHandle = Urb->UrbPipeRequest.PipeHandle;
    DeviceHandle = Urb->UrbHeader.UsbdDeviceHandle;

    if (!USBPORT_ValidatePipeHandle(DeviceHandle, PipeHandle))
    {
        return USBPORT_USBDStatusToNtStatus(Urb,
                                            USBD_STATUS_INVALID_PIPE_HANDLE);
    }

    Endpoint = PipeHandle->Endpoint;

    RtlZeroMemory(&SetupPacket, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    SetupPacket.bmRequestType.Recipient = BMREQUEST_TO_ENDPOINT;
    SetupPacket.bRequest = USB_REQUEST_CLEAR_FEATURE;
    SetupPacket.wValue.W = 0;
    SetupPacket.wIndex.W = Endpoint->EndpointProperties.EndpointAddress;
    SetupPacket.wLength = 0;

    USBPORT_SendSetupPacket(DeviceHandle,
                            FdoDevice,
                            &SetupPacket,
                            NULL,
                            0,
                            NULL,
                            &USBDStatus);

    Status = USBPORT_USBDStatusToNtStatus(Urb, USBDStatus);

    return Status;
}

NTSTATUS
NTAPI
USBPORT_SyncResetPipeAndClearStall(IN PDEVICE_OBJECT FdoDevice,
                                   IN PIRP Irp,
                                   IN PURB Urb)
{
    PUSBPORT_DEVICE_HANDLE DeviceHandle;
    PUSBPORT_PIPE_HANDLE PipeHandle;
    PUSBPORT_ENDPOINT Endpoint;
    ULONG EndpointState;
    NTSTATUS Status;

    DPRINT_URB("USBPORT_SyncResetPipeAndClearStall: ... \n");

    ASSERT(Urb->UrbHeader.UsbdDeviceHandle);
    ASSERT(Urb->UrbHeader.Length == sizeof(struct _URB_PIPE_REQUEST));
    ASSERT(Urb->UrbPipeRequest.PipeHandle);

    DeviceHandle = Urb->UrbHeader.UsbdDeviceHandle;
    PipeHandle = Urb->UrbPipeRequest.PipeHandle;

    if (!USBPORT_ValidatePipeHandle(DeviceHandle, PipeHandle))
    {
        return USBPORT_USBDStatusToNtStatus(Urb,
                                            USBD_STATUS_INVALID_PIPE_HANDLE);
    }

    if (PipeHandle->Flags & PIPE_HANDLE_FLAG_NULL_PACKET_SIZE)
    {
        return USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_SUCCESS);
    }

    Endpoint = PipeHandle->Endpoint;
    InterlockedIncrement(&DeviceHandle->DeviceHandleLock);

    if (Endpoint->EndpointProperties.TransferType != USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        Urb->UrbHeader.UsbdFlags |= USBD_FLAG_NOT_ISO_TRANSFER;
        Status = USBPORT_ClearStall(FdoDevice, Irp, Urb);
    }
    else
    {
        Status = USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_SUCCESS);
    }

    if (NT_SUCCESS(Status))
    {
        Status = USBPORT_ResetPipe(FdoDevice, Irp, Urb);

        if (Endpoint->EndpointProperties.TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
        {
            while (TRUE)
            {
                KeAcquireSpinLock(&Endpoint->EndpointSpinLock,
                                  &Endpoint->EndpointOldIrql);

                EndpointState = USBPORT_GetEndpointState(Endpoint);

                if (EndpointState == USBPORT_ENDPOINT_PAUSED &&
                    IsListEmpty(&Endpoint->TransferList))
                {
                    USBPORT_SetEndpointState(Endpoint,
                                             USBPORT_ENDPOINT_ACTIVE);
                }

                KeReleaseSpinLock(&Endpoint->EndpointSpinLock,
                                  Endpoint->EndpointOldIrql);

                if (EndpointState == USBPORT_ENDPOINT_ACTIVE)
                {
                    break;
                }

                USBPORT_Wait(FdoDevice, 1);
            }
        }
    }

    InterlockedDecrement(&DeviceHandle->DeviceHandleLock);

    return Status;
}

NTSTATUS
NTAPI
USBPORT_HandleSetOrClearFeature(IN PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket;

    DPRINT_URB("USBPORT_HandleSetOrClearFeature: Urb - %p\n", Urb);

    SetupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)
                  &Urb->UrbControlFeatureRequest.Reserved0;

    SetupPacket->wLength = 0;
    Urb->UrbControlFeatureRequest.Reserved3 = 0; // TransferBufferLength

    SetupPacket->bmRequestType.Dir = BMREQUEST_HOST_TO_DEVICE;

    switch (Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE\n");
            SetupPacket->bRequest = USB_REQUEST_SET_FEATURE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_DEVICE;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE\n");
            SetupPacket->bRequest = USB_REQUEST_SET_FEATURE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_INTERFACE;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE\n");
            SetupPacket->bRequest = USB_REQUEST_SET_FEATURE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_ENDPOINT;
            break;

        case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT\n");
            SetupPacket->bRequest = USB_REQUEST_CLEAR_FEATURE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_DEVICE;
            break;

        case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT\n");
            SetupPacket->bRequest = USB_REQUEST_CLEAR_FEATURE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_INTERFACE;
            break;

        case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE\n");
            SetupPacket->bRequest = USB_REQUEST_CLEAR_FEATURE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_ENDPOINT;
            break;

        case URB_FUNCTION_CLEAR_FEATURE_TO_OTHER:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE\n");
            SetupPacket->bRequest = USB_REQUEST_CLEAR_FEATURE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_OTHER;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_OTHER:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE\n");
            SetupPacket->bRequest = USB_REQUEST_SET_FEATURE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_OTHER;
            break;
    }

    Urb->UrbControlFeatureRequest.Reserved2 &= ~USBD_TRANSFER_DIRECTION_IN;
    Urb->UrbControlFeatureRequest.Reserved2 |= USBD_SHORT_TRANSFER_OK;

    USBPORT_DumpingSetupPacket(SetupPacket);

    USBPORT_QueueTransferUrb(Urb);

    return STATUS_PENDING;
}

NTSTATUS
NTAPI
USBPORT_HandleDataTransfers(IN PURB Urb)
{
    PUSBPORT_ENDPOINT Endpoint;

    DPRINT_URB("USBPORT_HandleDataTransfers: Urb - %p\n", Urb);

    Endpoint = ((PUSBPORT_PIPE_HANDLE)
                (Urb->UrbBulkOrInterruptTransfer.PipeHandle))->Endpoint;

    if (Endpoint->EndpointProperties.TransferType != USBPORT_TRANSFER_TYPE_CONTROL)
    {
        if (Endpoint->EndpointProperties.Direction == USBPORT_TRANSFER_DIRECTION_OUT)
        {
            Urb->UrbBulkOrInterruptTransfer.TransferFlags &= ~USBD_TRANSFER_DIRECTION_IN;
        }
        else
        {
            Urb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_TRANSFER_DIRECTION_IN;
        }
    }

    USBPORT_QueueTransferUrb(Urb);

    return STATUS_PENDING;
}

NTSTATUS
NTAPI
USBPORT_HandleGetStatus(IN PIRP Irp,
                        IN PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket;
    NTSTATUS Status;

    SetupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)
                   &Urb->UrbControlDescriptorRequest.Reserved1;

    SetupPacket->bmRequestType.B = 0;
    SetupPacket->bmRequestType.Dir = BMREQUEST_DEVICE_TO_HOST;
    SetupPacket->bRequest = USB_REQUEST_GET_STATUS;
    SetupPacket->wLength = Urb->UrbControlDescriptorRequest.TransferBufferLength;
    SetupPacket->wValue.W = 0;

    switch (Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
            DPRINT_URB("USBPORT_HandleGetStatus: URB_FUNCTION_GET_STATUS_FROM_DEVICE\n");
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_DEVICE;
            break;

        case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
            DPRINT_URB("USBPORT_HandleGetStatus: URB_FUNCTION_GET_STATUS_FROM_INTERFACE\n");
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_INTERFACE;
            break;

        case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
            DPRINT_URB("USBPORT_HandleGetStatus: URB_FUNCTION_GET_STATUS_FROM_ENDPOINT\n");
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_ENDPOINT;
            break;

        case URB_FUNCTION_GET_STATUS_FROM_OTHER:
            DPRINT_URB("USBPORT_HandleGetStatus: URB_FUNCTION_GET_STATUS_FROM_OTHER\n");
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_OTHER;
            break;
    }

    if (SetupPacket->wLength == 2)
    {
        Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;

        if (SetupPacket->bmRequestType.Dir)
            Urb->UrbControlTransfer.TransferFlags |= USBD_TRANSFER_DIRECTION_IN;
        else
            Urb->UrbControlTransfer.TransferFlags &= ~USBD_TRANSFER_DIRECTION_IN;

        //USBPORT_DumpingSetupPacket(SetupPacket);

        USBPORT_QueueTransferUrb(Urb);

        Status = STATUS_PENDING;
    }
    else
    {
        Status = USBPORT_USBDStatusToNtStatus(Urb,
                                              USBD_STATUS_INVALID_PARAMETER);

        DPRINT1("USBPORT_HandleGetStatus: Bad wLength\n");
        USBPORT_DumpingSetupPacket(SetupPacket);
    }

    return Status;
}

NTSTATUS
NTAPI
USBPORT_HandleVendorOrClass(IN PIRP Irp,
                            IN PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket;

    /*
        Specifies a value, from 4 to 31 inclusive,
        that becomes part of the request type code in the USB-defined setup packet.
        This value is defined by USB for a class request or the vendor for a vendor request.
    */

    SetupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)
                   &Urb->UrbControlDescriptorRequest.Reserved1;

    SetupPacket->bmRequestType.Dir = USBD_TRANSFER_DIRECTION_FLAG
                                     (Urb->UrbControlTransfer.TransferFlags);

    SetupPacket->wLength = Urb->UrbControlDescriptorRequest.TransferBufferLength;

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;

    switch (Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_VENDOR_DEVICE:
            DPRINT_URB("USBPORT_HandleVendorOrClass: URB_FUNCTION_VENDOR_DEVICE\n");
            SetupPacket->bmRequestType.Type = BMREQUEST_VENDOR;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_DEVICE;
            break;

        case URB_FUNCTION_VENDOR_INTERFACE:
            DPRINT_URB("USBPORT_HandleVendorOrClass: URB_FUNCTION_VENDOR_INTERFACE\n");
            SetupPacket->bmRequestType.Type = BMREQUEST_VENDOR;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_INTERFACE;
            break;

        case URB_FUNCTION_VENDOR_ENDPOINT:
            DPRINT_URB("USBPORT_HandleVendorOrClass: URB_FUNCTION_VENDOR_ENDPOINT\n");
            SetupPacket->bmRequestType.Type = BMREQUEST_VENDOR;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_ENDPOINT;
            break;

        case URB_FUNCTION_CLASS_DEVICE:
            DPRINT_URB("USBPORT_HandleVendorOrClass: URB_FUNCTION_CLASS_DEVICE\n");
            SetupPacket->bmRequestType.Type = BMREQUEST_CLASS;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_DEVICE;
            break;

        case URB_FUNCTION_CLASS_INTERFACE:
            DPRINT_URB("USBPORT_HandleVendorOrClass: URB_FUNCTION_CLASS_INTERFACE\n");
            SetupPacket->bmRequestType.Type = BMREQUEST_CLASS;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_INTERFACE;
            break;

        case URB_FUNCTION_CLASS_ENDPOINT:
            DPRINT_URB("USBPORT_HandleVendorOrClass: URB_FUNCTION_CLASS_ENDPOINT\n");
            SetupPacket->bmRequestType.Type = BMREQUEST_CLASS;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_ENDPOINT;
            break;

        case URB_FUNCTION_CLASS_OTHER:
            DPRINT_URB("USBPORT_HandleVendorOrClass: URB_FUNCTION_CLASS_OTHER\n");
            SetupPacket->bmRequestType.Type = BMREQUEST_CLASS;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_OTHER;
            break;

        case URB_FUNCTION_VENDOR_OTHER:
            DPRINT_URB("USBPORT_HandleVendorOrClass: URB_FUNCTION_VENDOR_OTHER\n");
            SetupPacket->bmRequestType.Type = BMREQUEST_VENDOR;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_OTHER;
            break;
    }

    USBPORT_DumpingSetupPacket(SetupPacket);

    USBPORT_QueueTransferUrb(Urb);

    return STATUS_PENDING;
}

NTSTATUS
NTAPI
USBPORT_HandleGetSetDescriptor(IN PIRP Irp,
                               IN PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket;

    SetupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)
                   &Urb->UrbControlDescriptorRequest.Reserved1;

    SetupPacket->wLength = Urb->UrbControlDescriptorRequest.TransferBufferLength;
    SetupPacket->bmRequestType.B = 0; // Clear bmRequestType
    SetupPacket->bmRequestType.Type = BMREQUEST_STANDARD;

    switch (Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE\n");
            SetupPacket->bRequest = USB_REQUEST_GET_DESCRIPTOR;
            SetupPacket->bmRequestType.Dir = BMREQUEST_DEVICE_TO_HOST;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_DEVICE;
            break;

        case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE\n");
            SetupPacket->bRequest = USB_REQUEST_SET_DESCRIPTOR;
            SetupPacket->bmRequestType.Dir = BMREQUEST_HOST_TO_DEVICE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_DEVICE;
            break;

        case URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT\n");
            SetupPacket->bRequest = USB_REQUEST_GET_DESCRIPTOR;
            SetupPacket->bmRequestType.Dir = BMREQUEST_DEVICE_TO_HOST;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_ENDPOINT;
            break;

        case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT\n");
            SetupPacket->bRequest = USB_REQUEST_SET_DESCRIPTOR;
            SetupPacket->bmRequestType.Dir = BMREQUEST_HOST_TO_DEVICE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_ENDPOINT;
            break;

        case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE\n");
            SetupPacket->bRequest = USB_REQUEST_GET_DESCRIPTOR;
            SetupPacket->bmRequestType.Dir = BMREQUEST_DEVICE_TO_HOST;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_INTERFACE;
            break;

        case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
            DPRINT_URB("USBPORT_HandleGetSetDescriptor: URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE\n");
            SetupPacket->bRequest = USB_REQUEST_SET_DESCRIPTOR;
            SetupPacket->bmRequestType.Dir = BMREQUEST_HOST_TO_DEVICE;
            SetupPacket->bmRequestType.Recipient = BMREQUEST_TO_INTERFACE;
            break;
    }

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;

    if (SetupPacket->bmRequestType.Dir)
        Urb->UrbControlTransfer.TransferFlags |= USBD_TRANSFER_DIRECTION_IN;
    else
        Urb->UrbControlTransfer.TransferFlags &= ~USBD_TRANSFER_DIRECTION_IN;

    USBPORT_DumpingSetupPacket(SetupPacket);

    USBPORT_QueueTransferUrb(Urb);

    return STATUS_PENDING;
}

NTSTATUS
NTAPI
USBPORT_ValidateTransferParametersURB(IN PURB Urb)
{
    struct _URB_CONTROL_TRANSFER *UrbRequest;
    PMDL Mdl;

    DPRINT_URB("USBPORT_ValidateTransferParametersURB: Urb - %p\n", Urb);

    UrbRequest = &Urb->UrbControlTransfer;

    if (UrbRequest->TransferBuffer == NULL &&
        UrbRequest->TransferBufferMDL == NULL &&
        UrbRequest->TransferBufferLength > 0)
    {
        DPRINT1("USBPORT_ValidateTransferParametersURB: Not valid parameter\n");
        USBPORT_DumpingURB(Urb);
        return STATUS_INVALID_PARAMETER;
    }

    if ((UrbRequest->TransferBuffer != NULL) &&
        (UrbRequest->TransferBufferMDL != NULL) &&
        UrbRequest->TransferBufferLength == 0)
    {
        DPRINT1("USBPORT_ValidateTransferParametersURB: Not valid parameter\n");
        USBPORT_DumpingURB(Urb);
        return STATUS_INVALID_PARAMETER;
    }

    if (UrbRequest->TransferBuffer != NULL &&
        UrbRequest->TransferBufferMDL == NULL &&
        UrbRequest->TransferBufferLength != 0)
    {
        DPRINT_URB("USBPORT_ValidateTransferParametersURB: TransferBuffer - %p, TransferBufferLength - %x\n",
                   UrbRequest->TransferBuffer,
                   UrbRequest->TransferBufferLength);

        Mdl = IoAllocateMdl(UrbRequest->TransferBuffer,
                            UrbRequest->TransferBufferLength,
                            FALSE,
                            FALSE,
                            NULL);

        if (!Mdl)
        {
            DPRINT1("USBPORT_ValidateTransferParametersURB: Not allocated Mdl\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool(Mdl);

        UrbRequest->TransferBufferMDL = Mdl;
        Urb->UrbHeader.UsbdFlags |= USBD_FLAG_ALLOCATED_MDL;

        DPRINT_URB("USBPORT_ValidateTransferParametersURB: Mdl - %p\n", Mdl);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBPORT_ValidateURB(IN PDEVICE_OBJECT FdoDevice,
                    IN PIRP Irp,
                    IN PURB Urb,
                    IN BOOLEAN IsControlTransfer,
                    IN BOOLEAN IsNullTransfer)
{
    struct _URB_CONTROL_TRANSFER *UrbRequest;
    PUSBPORT_DEVICE_HANDLE DeviceHandle;
    NTSTATUS Status;
    USBD_STATUS USBDStatus;

    UrbRequest = &Urb->UrbControlTransfer;

    if (UrbRequest->UrbLink)
    {
        Status = USBPORT_USBDStatusToNtStatus(Urb,
                                              USBD_STATUS_INVALID_PARAMETER);

        DPRINT1("USBPORT_ValidateURB: Not valid parameter\n");

        USBPORT_DumpingURB(Urb);
        return Status;
    }

    DeviceHandle = Urb->UrbHeader.UsbdDeviceHandle;

    if (IsControlTransfer)
    {
        UrbRequest->TransferFlags |= USBD_DEFAULT_PIPE_TRANSFER;
        UrbRequest->PipeHandle = &DeviceHandle->PipeHandle;
    }

    if (UrbRequest->TransferFlags & USBD_DEFAULT_PIPE_TRANSFER)
    {
        if (UrbRequest->TransferBufferLength > 0x1000)
        {
            Status = USBPORT_USBDStatusToNtStatus(Urb,
                                                  USBD_STATUS_INVALID_PARAMETER);

            DPRINT1("USBPORT_ValidateURB: Not valid parameter\n");

            USBPORT_DumpingURB(Urb);
            return Status;
        }

        if (Urb->UrbHeader.Function == URB_FUNCTION_CONTROL_TRANSFER)
        {
            UrbRequest->PipeHandle = &DeviceHandle->PipeHandle;
        }
    }

    if (!USBPORT_ValidatePipeHandle(DeviceHandle, UrbRequest->PipeHandle))
    {
        Status = USBPORT_USBDStatusToNtStatus(Urb,
                                              USBD_STATUS_INVALID_PIPE_HANDLE);

        DPRINT1("USBPORT_ValidateURB: Not valid pipe handle\n");

        USBPORT_DumpingURB(Urb);
        return Status;
    }

    UrbRequest->hca.Reserved8[0] = NULL; // Transfer

    if (IsNullTransfer)
    {
        UrbRequest->TransferBuffer = 0;
        UrbRequest->TransferBufferMDL = NULL;
        UrbRequest->TransferBufferLength = 0;
    }
    else
    {
        Status = USBPORT_ValidateTransferParametersURB(Urb);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    USBDStatus = USBPORT_AllocateTransfer(FdoDevice,
                                          Urb,
                                          DeviceHandle,
                                          Irp,
                                          NULL);

    Status = USBPORT_USBDStatusToNtStatus(Urb, USBDStatus);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBPORT_ValidateURB: Not allocated transfer\n");
    }

    return Status;
}

NTSTATUS
NTAPI
USBPORT_HandleSubmitURB(IN PDEVICE_OBJECT PdoDevice,
                        IN PIRP Irp,
                        IN PURB Urb)
{
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    USHORT Function;
    PUSBPORT_DEVICE_HANDLE DeviceHandle;
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    ASSERT(Urb);

    PdoExtension = PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
    Urb->UrbHeader.UsbdFlags = 0;

    Function = Urb->UrbHeader.Function;

    if (Function > URB_FUNCTION_MAX)
    {
        Status = USBPORT_USBDStatusToNtStatus(Urb,
                                              USBD_STATUS_INVALID_URB_FUNCTION);

        DPRINT1("USBPORT_HandleSubmitURB: Unknown URB function - %x !!!\n",
               Function);

        return Status;
    }

    if (FdoExtension->TimerFlags & USBPORT_TMFLAG_RH_SUSPENDED)
    {
        DPRINT1("USBPORT_HandleSubmitURB: Bad Request\n");

        USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_DEVICE_GONE);

        Irp->IoStatus.Status = STATUS_PENDING;
        IoMarkIrpPending(Irp);
        IoCsqInsertIrp(&FdoExtension->BadRequestIoCsq, Irp, NULL);

        return STATUS_PENDING;
    }

    DeviceHandle = Urb->UrbHeader.UsbdDeviceHandle;

    if (!DeviceHandle)
    {
        DeviceHandle = &PdoExtension->DeviceHandle;
        Urb->UrbHeader.UsbdDeviceHandle = DeviceHandle;
    }

    if (!USBPORT_ValidateDeviceHandle(PdoExtension->FdoDevice,
                                      DeviceHandle))
    {
        DPRINT1("USBPORT_HandleSubmitURB: Not valid device handle\n");

        Irp->IoStatus.Status = STATUS_PENDING;
        IoMarkIrpPending(Irp);
        IoCsqInsertIrp(&FdoExtension->BadRequestIoCsq, Irp, NULL);

        return STATUS_PENDING;
    }

    InterlockedIncrement(&DeviceHandle->DeviceHandleLock);

    DPRINT_URB("USBPORT_HandleSubmitURB: Function - 0x%02X, DeviceHandle - %p\n",
               Function,
               Urb->UrbHeader.UsbdDeviceHandle);

    switch (Function)
    {
        case URB_FUNCTION_ISOCH_TRANSFER:
            DPRINT1("USBPORT_HandleSubmitURB: URB_FUNCTION_ISOCH_TRANSFER UNIMPLEMENTED. FIXME. \n");
            break;

        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        case URB_FUNCTION_CONTROL_TRANSFER:
            Status = USBPORT_ValidateURB(FdoDevice, Irp, Urb, FALSE, FALSE);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("USBPORT_HandleSubmitURB: Not valid URB\n");
                break;
            }

            Status = USBPORT_HandleDataTransfers(Urb);
            break;

        case URB_FUNCTION_VENDOR_DEVICE:
        case URB_FUNCTION_VENDOR_INTERFACE:
        case URB_FUNCTION_VENDOR_ENDPOINT:
        case URB_FUNCTION_CLASS_DEVICE:
        case URB_FUNCTION_CLASS_INTERFACE:
        case URB_FUNCTION_CLASS_ENDPOINT:
        case URB_FUNCTION_CLASS_OTHER:
        case URB_FUNCTION_VENDOR_OTHER:
            Status = USBPORT_ValidateURB(FdoDevice, Irp, Urb, TRUE, FALSE);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("USBPORT_HandleSubmitURB: Not valid URB\n");
                break;
            }

            Status = USBPORT_HandleVendorOrClass(Irp, Urb);
            break;

        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
        case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
        case URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT:
        case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
        case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
        case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
            Status = USBPORT_ValidateURB(FdoDevice, Irp, Urb, TRUE, FALSE);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("USBPORT_HandleSubmitURB: Not valid URB\n");
                break;
            }

            Status = USBPORT_HandleGetSetDescriptor(Irp, Urb);
            break;

        case URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR:
            DPRINT1("USBPORT_HandleSubmitURB: URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR (0x2A) NOT_SUPPORTED\n");
            return USBPORT_USBDStatusToNtStatus(Urb,
                                                USBD_STATUS_INVALID_URB_FUNCTION);

        case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
        case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
        case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
        case URB_FUNCTION_GET_STATUS_FROM_OTHER:
            Status = USBPORT_ValidateURB(FdoDevice, Irp, Urb, TRUE, FALSE);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("USBPORT_HandleSubmitURB: Not valid URB\n");
                break;
            }

            Status = USBPORT_HandleGetStatus(Irp, Urb);
            break;

        case URB_FUNCTION_SELECT_CONFIGURATION:
            Status = USBPORT_HandleSelectConfiguration(PdoExtension->FdoDevice,
                                                       Irp,
                                                       Urb);
            break;

        case URB_FUNCTION_SELECT_INTERFACE:
            Status = USBPORT_HandleSelectInterface(PdoExtension->FdoDevice,
                                                   Irp,
                                                   Urb);
            break;

        case URB_FUNCTION_GET_CONFIGURATION:
            Status = USBPORT_ValidateURB(FdoDevice, Irp, Urb, TRUE, FALSE);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("USBPORT_HandleSubmitURB: Not valid URB\n");
                break;
            }

            Status = USBPORT_HandleGetConfiguration(Urb);
            break;

        case URB_FUNCTION_GET_INTERFACE:
            DPRINT1("USBPORT_HandleSubmitURB: URB_FUNCTION_GET_INTERFACE (0x27) NOT_SUPPORTED\n");
            return USBPORT_USBDStatusToNtStatus(Urb,
                                                USBD_STATUS_INVALID_URB_FUNCTION);

        case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
            Status = USBPORT_SyncResetPipeAndClearStall(PdoExtension->FdoDevice,
                                                        Irp,
                                                        Urb);
            break;

        case URB_FUNCTION_SYNC_RESET_PIPE:
            Status = USBPORT_ResetPipe(PdoExtension->FdoDevice,
                                       Irp,
                                       Urb);
            break;

        case URB_FUNCTION_SYNC_CLEAR_STALL:
            Status = USBPORT_ClearStall(PdoExtension->FdoDevice,
                                        Irp,
                                        Urb);
            break;

        case URB_FUNCTION_ABORT_PIPE:
            Status = USBPORT_AbortPipe(PdoExtension->FdoDevice,
                                       Irp,
                                       Urb);
            break;

        case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
        case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
        case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
        case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
        case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
        case URB_FUNCTION_CLEAR_FEATURE_TO_OTHER:
        case URB_FUNCTION_SET_FEATURE_TO_OTHER:
            Status = USBPORT_ValidateURB(FdoDevice, Irp, Urb, TRUE, TRUE);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("USBPORT_HandleSubmitURB: Not valid URB\n");
                break;
            }

            Status = USBPORT_HandleSetOrClearFeature(Urb);
            break;

        case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER:
            Status = USBPORT_HandleGetCurrentFrame(PdoExtension->FdoDevice,
                                                   Irp,
                                                   Urb);
            break;

        case URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL:
            DPRINT1("USBPORT_HandleSubmitURB: URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL (0x03) NOT_SUPPORTED\n");
            return USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_NOT_SUPPORTED);

        case URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL:
            DPRINT1("USBPORT_HandleSubmitURB: URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL (0x04) NOT_SUPPORTED\n");
            return USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_NOT_SUPPORTED);

        case URB_FUNCTION_GET_FRAME_LENGTH:
            DPRINT1("USBPORT_HandleSubmitURB: URB_FUNCTION_GET_FRAME_LENGTH (0x05) NOT_SUPPORTED\n");
            return USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_NOT_SUPPORTED);

        case URB_FUNCTION_SET_FRAME_LENGTH:
            DPRINT1("USBPORT_HandleSubmitURB: URB_FUNCTION_SET_FRAME_LENGTH (0x06) NOT_SUPPORTED\n");
            return USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_NOT_SUPPORTED);

        default:
            DPRINT1("USBPORT_HandleSubmitURB: Unknown URB Function - %x\n",
                    Function);
            //URB_FUNCTION_RESERVED_0X0016
            //URB_FUNCTION_RESERVE_0X001D
            //URB_FUNCTION_RESERVE_0X002B
            //URB_FUNCTION_RESERVE_0X002C
            //URB_FUNCTION_RESERVE_0X002D
            //URB_FUNCTION_RESERVE_0X002E
            //URB_FUNCTION_RESERVE_0X002F
            break;
    }

    if (Status == STATUS_PENDING)
    {
        return Status;
    }

    if (Urb->UrbHeader.UsbdFlags & USBD_FLAG_ALLOCATED_TRANSFER)
    {
        PUSBPORT_TRANSFER Transfer;

        Transfer = Urb->UrbControlTransfer.hca.Reserved8[0];
        Urb->UrbControlTransfer.hca.Reserved8[0] = NULL;
        Urb->UrbHeader.UsbdFlags |= ~USBD_FLAG_ALLOCATED_TRANSFER;
        ExFreePoolWithTag(Transfer, USB_PORT_TAG);
    }

    InterlockedDecrement(&DeviceHandle->DeviceHandleLock);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
