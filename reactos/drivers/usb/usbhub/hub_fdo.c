/*
 * PROJECT:         ReactOS Universal Serial Bus Hub Driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/usb/usbhub/hub_fdo.c
 * PURPOSE:         Hub FDO
 * PROGRAMMERS:
 *                  Michael Martin (michael.martin@reactos.org)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbhub.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
USBHUB_ParentFDOStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PURB Urb, ConfigurationUrb;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSBD_INTERFACE_LIST_ENTRY InterfaceList;
    ULONG Index;
    NTSTATUS Status;

    // get hub device extension
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // Send the StartDevice to lower device object
    Status = ForwardIrpAndWait(HubDeviceExtension->LowerDeviceObject, Irp);

    if (!NT_SUCCESS(Status))
    {
        // failed to start pdo
        DPRINT1("Failed to start the RootHub PDO\n");
        return Status;
    }

    // FIXME get capabilities

    Urb = ExAllocatePool(NonPagedPool, sizeof(URB));
    if (!Urb)
    {
        // no memory
        DPRINT1("No memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    // lets get device descriptor
    UsbBuildGetDescriptorRequest(Urb,
                                    sizeof(Urb->UrbControlDescriptorRequest),
                                    USB_DEVICE_DESCRIPTOR_TYPE,
                                    0,
                                    0,
                                    &HubDeviceExtension->HubDeviceDescriptor,
                                    NULL,
                                    sizeof(USB_DEVICE_DESCRIPTOR),
                                    NULL);


    // get hub device descriptor
    Status = SubmitRequestToRootHub(HubDeviceExtension->LowerDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    Urb,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        // failed to get device descriptor of hub
        DPRINT1("Failed to get hub device descriptor with Status %x!\n", Status);
        ExFreePool(Urb);
        return Status;
    }

    // now get configuration descriptor
    UsbBuildGetDescriptorRequest(Urb,
                                    sizeof(Urb->UrbControlDescriptorRequest),
                                    USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                    0,
                                    0,
                                    &HubDeviceExtension->HubConfigDescriptor,
                                    NULL,
                                    sizeof(USB_CONFIGURATION_DESCRIPTOR) + sizeof(USB_INTERFACE_DESCRIPTOR) + sizeof(USB_ENDPOINT_DESCRIPTOR),
                                    NULL);

    // request configuration descriptor
    Status = SubmitRequestToRootHub(HubDeviceExtension->LowerDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    Urb,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        // failed to get configuration descriptor
        DPRINT1("Failed to get hub configuration descriptor with status %x\n", Status);
        ExFreePool(Urb);
        return Status;
    }

    // sanity checks
    ASSERT(HubDeviceExtension->HubConfigDescriptor.wTotalLength == sizeof(USB_CONFIGURATION_DESCRIPTOR) + sizeof(USB_INTERFACE_DESCRIPTOR) + sizeof(USB_ENDPOINT_DESCRIPTOR));
    ASSERT(HubDeviceExtension->HubConfigDescriptor.bDescriptorType == USB_CONFIGURATION_DESCRIPTOR_TYPE);
    ASSERT(HubDeviceExtension->HubConfigDescriptor.bLength == sizeof(USB_CONFIGURATION_DESCRIPTOR));
    ASSERT(HubDeviceExtension->HubConfigDescriptor.bNumInterfaces == 1);
    ASSERT(HubDeviceExtension->HubInterfaceDescriptor.bLength == sizeof(USB_INTERFACE_DESCRIPTOR));
    ASSERT(HubDeviceExtension->HubInterfaceDescriptor.bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE);
    ASSERT(HubDeviceExtension->HubInterfaceDescriptor.bNumEndpoints == 1);
    ASSERT(HubDeviceExtension->HubEndPointDescriptor.bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE);
    ASSERT(HubDeviceExtension->HubEndPointDescriptor.bLength == sizeof(USB_ENDPOINT_DESCRIPTOR));
    ASSERT(HubDeviceExtension->HubEndPointDescriptor.bmAttributes == USB_ENDPOINT_TYPE_INTERRUPT);
    ASSERT(HubDeviceExtension->HubEndPointDescriptor.bEndpointAddress == 0x81); // interrupt in

    // Build hub descriptor request
    UsbBuildVendorRequest(Urb,
                            URB_FUNCTION_CLASS_DEVICE,
                            sizeof(Urb->UrbControlVendorClassRequest),
                            USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                            0,
                            USB_REQUEST_GET_DESCRIPTOR,
                            USB_DEVICE_CLASS_RESERVED,
                            0,
                            &HubDeviceExtension->HubDescriptor,
                            NULL,
                            sizeof(USB_HUB_DESCRIPTOR),
                            NULL);

    // send request
    Status = SubmitRequestToRootHub(HubDeviceExtension->LowerDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    Urb,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get Hub Descriptor Status %x!\n", Status);
        ExFreePool(Urb);
        return STATUS_UNSUCCESSFUL;
    }

    // sanity checks
    ASSERT(HubDeviceExtension->HubDescriptor.bDescriptorLength == sizeof(USB_HUB_DESCRIPTOR));
    ASSERT(HubDeviceExtension->HubDescriptor.bNumberOfPorts);
    ASSERT(HubDeviceExtension->HubDescriptor.bDescriptorType == 0x29);

    // store number of ports
    DPRINT1("NumberOfPorts %lu\n", HubDeviceExtension->HubDescriptor.bNumberOfPorts);
    HubDeviceExtension->UsbExtHubInfo.NumberOfPorts = HubDeviceExtension->HubDescriptor.bNumberOfPorts;

    // allocate interface list
    InterfaceList = ExAllocatePool(NonPagedPool, sizeof(USBD_INTERFACE_LIST_ENTRY) * (HubDeviceExtension->HubConfigDescriptor.bNumInterfaces + 1));
    if (!InterfaceList)
    {
        // no memory
        DPRINT1("No memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // zero list
    RtlZeroMemory(InterfaceList, sizeof(USBD_INTERFACE_LIST_ENTRY) * (HubDeviceExtension->HubConfigDescriptor.bNumInterfaces + 1));

    // grab all interface descriptors
    for(Index = 0; Index < HubDeviceExtension->HubConfigDescriptor.bNumInterfaces; Index++)
    {
        // Get the first Configuration Descriptor
        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(&HubDeviceExtension->HubConfigDescriptor,
                                                                  &HubDeviceExtension->HubConfigDescriptor,
                                                                  Index, 0, -1, -1, -1);

        // store in list
        InterfaceList[Index].InterfaceDescriptor = InterfaceDescriptor;
    }

    // now create configuration request
    ConfigurationUrb = USBD_CreateConfigurationRequestEx(&HubDeviceExtension->HubConfigDescriptor,
                                                    (PUSBD_INTERFACE_LIST_ENTRY)&InterfaceList);
    if (ConfigurationUrb == NULL)
    {
        // failed to build urb
        DPRINT1("Failed to build configuration urb\n");
        ExFreePool(Urb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // send request
    Status = SubmitRequestToRootHub(HubDeviceExtension->LowerDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    ConfigurationUrb,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get Hub Descriptor Status %x!\n", Status);
        ExFreePool(Urb);
        ExFreePool(ConfigurationUrb);
        return STATUS_UNSUCCESSFUL;
    }

    // store configuration & pipe handle
    HubDeviceExtension->ConfigurationHandle = ConfigurationUrb->UrbSelectConfiguration.ConfigurationHandle;
    HubDeviceExtension->PipeHandle = ConfigurationUrb->UrbSelectConfiguration.Interface.Pipes[0].PipeHandle;
    DPRINT("Hub Configuration Handle %x\n", HubDeviceExtension->ConfigurationHandle);

    // free urb
    ExFreePool(ConfigurationUrb);
    ExFreePool(Urb);

    // FIXME build SCE interrupt request

    // FIXME create pdos

    return Status;
}

