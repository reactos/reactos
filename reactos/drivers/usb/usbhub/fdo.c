/*
 * PROJECT:         ReactOS Universal Serial Bus Hub Driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/usb/usbhub/fdo.c
 * PURPOSE:         Handle FDO
 * PROGRAMMERS:
 *                  Michael Martin (michael.martin@reactos.org)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbhub.h"

#include <stdio.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
QueryStatusChangeEndpoint(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
CreateUsbChildDeviceObject(
    IN PDEVICE_OBJECT UsbHubDeviceObject,
    IN LONG PortId,
    OUT PDEVICE_OBJECT *UsbChildDeviceObject,
    IN ULONG PortStatus);

NTSTATUS
DestroyUsbChildDeviceObject(
    IN PDEVICE_OBJECT UsbHubDeviceObject,
    IN LONG PortId);


NTSTATUS
GetPortStatusAndChange(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG PortId,
    OUT PPORT_STATUS_CHANGE StatusChange)
{
    NTSTATUS Status;
    PURB Urb;

    //
    // Allocate URB
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);
    if (!Urb)
    {
        DPRINT1("Failed to allocate memory for URB!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero it
    //
    RtlZeroMemory(Urb, sizeof(URB));

    //
    // Initialize URB for getting Port Status
    //
    UsbBuildVendorRequest(Urb,
                          URB_FUNCTION_CLASS_OTHER,
                          sizeof(Urb->UrbControlVendorClassRequest),
                          USBD_TRANSFER_DIRECTION_OUT,
                          0,
                          USB_REQUEST_GET_STATUS,
                          0,
                          PortId,
                          StatusChange,
                          0,
                          sizeof(PORT_STATUS_CHANGE),
                          0);

    // FIXME: support usb hubs
    Urb->UrbHeader.UsbdDeviceHandle = NULL;


    //
    // Query the Root Hub
    //
    Status = SubmitRequestToRootHub(RootHubDeviceObject, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

    //
    // Free URB
    //
    ExFreePool(Urb);

    return Status;
}

NTSTATUS
SetPortFeature(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG PortId,
    IN ULONG Feature)
{
    NTSTATUS Status;
    PURB Urb;

    //
    // Allocate URB
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);
    if (!Urb)
    {
        DPRINT1("Failed to allocate memory for URB!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero it
    //
    RtlZeroMemory(Urb, sizeof(URB));

    //
    // Initialize URB for Clearing Port Reset
    //
    UsbBuildVendorRequest(Urb,
                          URB_FUNCTION_CLASS_OTHER,
                          sizeof(Urb->UrbControlVendorClassRequest),
                          USBD_TRANSFER_DIRECTION_IN,
                          0,
                          USB_REQUEST_SET_FEATURE,
                          Feature,
                          PortId,
                          NULL,
                          0,
                          0,
                          0);

    // FIXME support usbhubs
    Urb->UrbHeader.UsbdDeviceHandle = NULL;

    //
    // Query the Root Hub
    //
    Status = SubmitRequestToRootHub(RootHubDeviceObject, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

    //
    // Free URB
    //
    ExFreePool(Urb);

    return Status;
}

NTSTATUS
ClearPortFeature(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG PortId,
    IN ULONG Feature)
{
    NTSTATUS Status;
    PURB Urb;

    //
    // Allocate a URB
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);
    if (!Urb)
    {
        DPRINT1("Failed to allocate memory for URB!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero it
    //
    RtlZeroMemory(Urb, sizeof(URB));

    //
    // Initialize URB for Clearing Port Reset
    //
    UsbBuildVendorRequest(Urb,
                          URB_FUNCTION_CLASS_OTHER,
                          sizeof(Urb->UrbControlVendorClassRequest),
                          USBD_TRANSFER_DIRECTION_IN,
                          0,
                          USB_REQUEST_CLEAR_FEATURE,
                          Feature,
                          PortId,
                          NULL,
                          0,
                          0,
                          0);

    // FIXME: support usb hubs
    Urb->UrbHeader.UsbdDeviceHandle = NULL;

    //
    // Query the Root Hub
    //
    Status = SubmitRequestToRootHub(RootHubDeviceObject, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

    //
    // Free URB
    //
    ExFreePool(Urb);

    return Status;
}

VOID NTAPI
DeviceStatusChangeThread(
    IN PVOID Context)
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject, RootHubDeviceObject;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PWORK_ITEM_DATA WorkItemData;
    PORT_STATUS_CHANGE PortStatus;
    ULONG PortId;
    BOOLEAN SignalResetComplete = FALSE;

    DPRINT("Entered DeviceStatusChangeThread, Context %x\n", Context);

    WorkItemData = (PWORK_ITEM_DATA)Context;
    DeviceObject = (PDEVICE_OBJECT)WorkItemData->Context;
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;
    //
    // Loop all ports
    //
    for (PortId = 1; PortId <= HubDeviceExtension->UsbExtHubInfo.NumberOfPorts; PortId++)
    {
        //
        // Get Port Status
        //
        Status = GetPortStatusAndChange(RootHubDeviceObject, PortId, &PortStatus);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get port status for port %d, Status %x\n", PortId, Status);
            // FIXME: Do we really want to halt further SCE requests?
            return;
        }

        DPRINT("Port %d Status %x\n", PortId, PortStatus.Status);
        DPRINT("Port %d Change %x\n", PortId, PortStatus.Change);


        //
        // Check for new device connection
        //
        if (PortStatus.Change & USB_PORT_STATUS_CONNECT)
        {
            //
            // Clear Port Connect
            //
            Status = ClearPortFeature(RootHubDeviceObject, PortId, C_PORT_CONNECTION);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to clear connection change for port %d\n", PortId);
                continue;
            }

            //
            // Is this a connect or disconnect?
            //
            if (!(PortStatus.Status & USB_PORT_STATUS_CONNECT))
            {
                DPRINT1("Device disconnected from port %d\n", PortId);

                Status = DestroyUsbChildDeviceObject(DeviceObject, PortId);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to delete child device object after disconnect\n");
                    continue;
                }
            }
            else
            {
                DPRINT1("Device connected from port %d\n", PortId);

                // No SCE completion done for clearing C_PORT_CONNECT

                //
                // Reset Port
                //
                Status = SetPortFeature(RootHubDeviceObject, PortId, PORT_RESET);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to reset port %d\n", PortId);
                    SignalResetComplete = TRUE;
                    continue;
                }
            }
        }
        else if (PortStatus.Change & USB_PORT_STATUS_ENABLE)
        {
            //
            // Clear Enable
            //
            Status = ClearPortFeature(RootHubDeviceObject, PortId, C_PORT_ENABLE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to clear enable change on port %d\n", PortId);
                continue;
            }
        }
        else if (PortStatus.Change & USB_PORT_STATUS_RESET)
        {
            //
            // Request event signalling later
            //
            SignalResetComplete = TRUE;

            //
            // Clear Reset
            //
            Status = ClearPortFeature(RootHubDeviceObject, PortId, C_PORT_RESET);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to clear reset change on port %d\n", PortId);
                continue;
            }

            //
            // Get Port Status
            //
            Status = GetPortStatusAndChange(RootHubDeviceObject, PortId, &PortStatus);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get port status for port %d, Status %x\n", PortId, Status);
                // FIXME: Do we really want to halt further SCE requests?
                return;
            }

            DPRINT("Port %d Status %x\n", PortId, PortStatus.Status);
            DPRINT("Port %d Change %x\n", PortId, PortStatus.Change);

            //
            // Check that reset was cleared
            //
            if(PortStatus.Change & USB_PORT_STATUS_RESET)
            {
                DPRINT1("Port did not clear reset! Possible Hardware problem!\n");
                continue;
            }

            //
            // Check if the device is still connected
            //
            if (!(PortStatus.Status & USB_PORT_STATUS_CONNECT))
            {
                DPRINT1("Device has been disconnected\n");
                continue;
            }

            //
            // Make sure its Connected and Enabled
            //
            if (!(PortStatus.Status & (USB_PORT_STATUS_CONNECT | USB_PORT_STATUS_ENABLE)))
            {
                DPRINT1("Usb Device is not connected and enabled!\n");
                //
                // Attempt another reset
                //
                Status = SetPortFeature(RootHubDeviceObject, PortId, PORT_RESET);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to reset port %d\n", PortId);
                }
                continue;
            }

            //
            // This is a new device
            //
            Status = CreateUsbChildDeviceObject(DeviceObject, PortId, NULL, PortStatus.Status);
        }
    }

    ExFreePool(WorkItemData);

    //
    // Send another SCE Request
    //
    DPRINT("Sending another SCE!\n");
    QueryStatusChangeEndpoint(DeviceObject);

    //
    // Check if a reset event was satisfied
    //
    if (SignalResetComplete)
    {
        //
        // Signal anyone waiting on it
        //
        KeSetEvent(&HubDeviceExtension->ResetComplete, IO_NO_INCREMENT, FALSE);
    }
}

NTSTATUS
NTAPI
StatusChangeEndpointCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PDEVICE_OBJECT RealDeviceObject;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PWORK_ITEM_DATA WorkItemData;

    RealDeviceObject = (PDEVICE_OBJECT)Context;
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)RealDeviceObject->DeviceExtension;

    //
    // NOTE: USBPORT frees this IRP
    //
    DPRINT("Received Irp %x, HubDeviceExtension->PendingSCEIrp %x\n", Irp, HubDeviceExtension->PendingSCEIrp);
    //IoFreeIrp(Irp);

    //
    // Create and initialize work item data
    //
    WorkItemData = ExAllocatePoolWithTag(NonPagedPool, sizeof(WORK_ITEM_DATA), USB_HUB_TAG);
    if (!WorkItemData)
    {
        DPRINT1("Failed to allocate memory!n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    WorkItemData->Context = RealDeviceObject;

    DPRINT("Queuing work item\n");

    //
    // Queue the work item to handle initializing the device
    //
    ExInitializeWorkItem(&WorkItemData->WorkItem, DeviceStatusChangeThread, (PVOID)WorkItemData);
    ExQueueWorkItem(&WorkItemData->WorkItem, DelayedWorkQueue);

    //
    // Return more processing required so the IO Manger doesn’t try to mess with IRP just freed
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
QueryStatusChangeEndpoint(
    IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT RootHubDeviceObject;
    PIO_STACK_LOCATION Stack;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PURB PendingSCEUrb;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;

    //
    // Allocate a URB
    //
    PendingSCEUrb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);

    //
    // Initialize URB for Status Change Endpoint request
    //
    UsbBuildInterruptOrBulkTransferRequest(PendingSCEUrb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           HubDeviceExtension->PipeHandle,
                                           HubDeviceExtension->PortStatusChange,
                                           NULL,
                                           sizeof(USHORT) * 2 * HubDeviceExtension->UsbExtHubInfo.NumberOfPorts,
                                           USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                           NULL);

    // Set the device handle
    PendingSCEUrb->UrbHeader.UsbdDeviceHandle = HubDeviceExtension->RootHubHandle;

    //
    // Allocate an Irp
    //
    HubDeviceExtension->PendingSCEIrp = ExAllocatePoolWithTag(NonPagedPool,
                                                              IoSizeOfIrp(RootHubDeviceObject->StackSize),
                                                              USB_HUB_TAG);
/*
    HubDeviceExtension->PendingSCEIrp = IoAllocateIrp(RootHubDeviceObject->StackSize,
                                  FALSE);
*/
    DPRINT("Allocated IRP %x\n", HubDeviceExtension->PendingSCEIrp);

    if (!HubDeviceExtension->PendingSCEIrp)
    {
        DPRINT1("USBHUB: Failed to allocate IRP for SCE request!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the IRP
    //
    IoInitializeIrp(HubDeviceExtension->PendingSCEIrp,
                    IoSizeOfIrp(RootHubDeviceObject->StackSize),
                    RootHubDeviceObject->StackSize);

    HubDeviceExtension->PendingSCEIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    HubDeviceExtension->PendingSCEIrp->IoStatus.Information = 0;
    HubDeviceExtension->PendingSCEIrp->Flags = 0;
    HubDeviceExtension->PendingSCEIrp->UserBuffer = NULL;

    //
    // Get the Next Stack Location and Initialize it
    //
    Stack = IoGetNextIrpStackLocation(HubDeviceExtension->PendingSCEIrp);
    Stack->DeviceObject = DeviceObject;
    Stack->Parameters.Others.Argument1 = PendingSCEUrb;
    Stack->Parameters.Others.Argument2 = NULL;
    Stack->MajorFunction =  IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    //
    // Set the completion routine for when device is connected to root hub
    //
    IoSetCompletionRoutine(HubDeviceExtension->PendingSCEIrp,
                           StatusChangeEndpointCompletion,
                           DeviceObject,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Send to RootHub
    //
    DPRINT("DeviceObject is %x\n", DeviceObject);
    DPRINT("Iocalldriver %x with irp %x\n", RootHubDeviceObject, HubDeviceExtension->PendingSCEIrp);
    IoCallDriver(RootHubDeviceObject, HubDeviceExtension->PendingSCEIrp);

    return STATUS_PENDING;
}

NTSTATUS
QueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN CONST GUID InterfaceType,
    IN LONG Size,
    IN LONG Version,
    OUT PVOID Interface)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;

    //
    // Initialize the Event used to wait for Irp completion
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build Control Request
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);

    //
    // Get Next Stack Location and Initialize it.
    //
    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.InterfaceType= &InterfaceType;//USB_BUS_INTERFACE_HUB_GUID;
    Stack->Parameters.QueryInterface.Size = Size;
    Stack->Parameters.QueryInterface.Version = Version;
    Stack->Parameters.QueryInterface.Interface = Interface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Initialize the status block before sending the IRP
    //
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        DPRINT("Operation pending\n");
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    return Status;
}

NTSTATUS
GetUsbDeviceDescriptor(
    IN PDEVICE_OBJECT ChildDeviceObject,
    IN UCHAR DescriptorType,
    IN UCHAR Index,
    IN USHORT LangId,
    OUT PVOID TransferBuffer,
    IN ULONG TransferBufferLength)
{
    NTSTATUS Status;
    PDEVICE_OBJECT RootHubDeviceObject;
    PURB Urb;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;

    //
    // Get the Hubs Device Extension
    //
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)ChildDeviceObject->DeviceExtension;
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) ChildDeviceExtension->ParentDeviceObject->DeviceExtension;
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;

    //
    // Allocate a URB
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);
    if (!Urb)
    {
        DPRINT1("Failed to allocate memory for URB!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero it
    //
    RtlZeroMemory(Urb, sizeof(URB));

    //
    // Initialize URB for getting device descriptor
    //
    UsbBuildGetDescriptorRequest(Urb,
                                 sizeof(Urb->UrbControlDescriptorRequest),
                                 DescriptorType,
                                 Index,
                                 LangId,
                                 TransferBuffer,
                                 NULL,
                                 TransferBufferLength,
                                 NULL);

    //
    // Set the device handle
    //
    Urb->UrbHeader.UsbdDeviceHandle = (PVOID)ChildDeviceExtension->UsbDeviceHandle;

    //
    // Query the Root Hub
    //
    Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    Urb,
                                    NULL);

    return Status;
}

NTSTATUS
GetUsbStringDescriptor(
    IN PDEVICE_OBJECT ChildDeviceObject,
    IN UCHAR Index,
    IN USHORT LangId,
    OUT PVOID *TransferBuffer,
    OUT USHORT *Size)
{
    NTSTATUS Status;
    PUSB_STRING_DESCRIPTOR StringDesc = NULL;
    ULONG SizeNeeded;
    LPWSTR Buffer;

    StringDesc = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(USB_STRING_DESCRIPTOR),
                                       USB_HUB_TAG);
    if (!StringDesc)
    {
        DPRINT1("Failed to allocate buffer for string!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the index string descriptor length
    // FIXME: Implement LangIds
    //
    Status = GetUsbDeviceDescriptor(ChildDeviceObject,
                                    USB_STRING_DESCRIPTOR_TYPE,
                                    Index,
                                    0x0409,
                                    StringDesc,
                                    sizeof(USB_STRING_DESCRIPTOR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetUsbDeviceDescriptor failed with status %x\n", Status);
        ExFreePool(StringDesc);
        return Status;
    }
    DPRINT1("StringDesc->bLength %d\n", StringDesc->bLength);

    //
    // Did we get something more than the length of the first two fields of structure?
    //
    if (StringDesc->bLength == 2)
    {
        DPRINT1("USB Device Error!\n");
        ExFreePool(StringDesc);
        return STATUS_DEVICE_DATA_ERROR;
    }
    SizeNeeded = StringDesc->bLength + sizeof(WCHAR);

    //
    // Free String
    //
    ExFreePool(StringDesc);

    //
    // Recreate with appropriate size
    //
    StringDesc = ExAllocatePoolWithTag(NonPagedPool,
                                       SizeNeeded,
                                       USB_HUB_TAG);
    if (!StringDesc)
    {
        DPRINT1("Failed to allocate buffer for string!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(StringDesc, SizeNeeded);

    //
    // Get the string
    //
    Status = GetUsbDeviceDescriptor(ChildDeviceObject,
                                    USB_STRING_DESCRIPTOR_TYPE,
                                    Index,
                                    0x0409,
                                    StringDesc,
                                    SizeNeeded);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetUsbDeviceDescriptor failed with status %x\n", Status);
        ExFreePool(StringDesc);
        return Status;
    }

    //
    // Allocate Buffer to return
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   SizeNeeded,
                                   USB_HUB_TAG);
    if (!Buffer)
    {
        DPRINT1("Failed to allocate buffer for string!\n");
        ExFreePool(StringDesc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Buffer, SizeNeeded);

    //
    // Copy the string to destination
    //
    RtlCopyMemory(Buffer, StringDesc->bString, SizeNeeded - FIELD_OFFSET(USB_STRING_DESCRIPTOR, bString));
    *Size = SizeNeeded;
    *TransferBuffer = Buffer;

    ExFreePool(StringDesc);

    return STATUS_SUCCESS;
}

ULONG
IsCompositeDevice(
    IN PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    if (DeviceDescriptor->bNumConfigurations != 1)
    {
         //
         // composite device must have only one configuration
         //
         DPRINT1("IsCompositeDevice bNumConfigurations %x\n", DeviceDescriptor->bNumConfigurations);
         return FALSE;
    }

    if (ConfigurationDescriptor->bNumInterfaces < 2)
    {
        //
        // composite device must have multiple interfaces
        //
        DPRINT1("IsCompositeDevice bNumInterfaces %x\n", ConfigurationDescriptor->bNumInterfaces);
        return FALSE;
    }

    if (DeviceDescriptor->bDeviceClass == 0)
    {
        //
        // composite device
        //
        ASSERT(DeviceDescriptor->bDeviceSubClass == 0);
        ASSERT(DeviceDescriptor->bDeviceProtocol == 0);
        DPRINT1("IsCompositeDevice: TRUE\n");
        return TRUE;
    }

    if (DeviceDescriptor->bDeviceClass == 0xEF &&
        DeviceDescriptor->bDeviceSubClass == 0x02 &&
        DeviceDescriptor->bDeviceProtocol == 0x01)
    {
        //
        // USB-IF association descriptor
        //
        DPRINT1("IsCompositeDevice: TRUE\n");
        return TRUE;
    }

    DPRINT1("DeviceDescriptor bDeviceClass %x bDeviceSubClass %x bDeviceProtocol %x\n", DeviceDescriptor->bDeviceClass, DeviceDescriptor->bDeviceSubClass, DeviceDescriptor->bDeviceProtocol);

    //
    // not a composite device
    //
    return FALSE;
}

NTSTATUS
CreateDeviceIds(
    PDEVICE_OBJECT UsbChildDeviceObject)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index = 0;
    LPWSTR DeviceString;
    WCHAR Buffer[200];
    PHUB_CHILDDEVICE_EXTENSION UsbChildExtension;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;

    //
    // get child device extension
    //
    UsbChildExtension = (PHUB_CHILDDEVICE_EXTENSION)UsbChildDeviceObject->DeviceExtension;

    // get hub device extension
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) UsbChildExtension->ParentDeviceObject->DeviceExtension;

    //
    // get device descriptor
    //
    DeviceDescriptor = &UsbChildExtension->DeviceDesc;

    //
    // get configuration descriptor
    //
    ConfigurationDescriptor = UsbChildExtension->FullConfigDesc;

    //
    // use first interface descriptor available
    //
    InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, ConfigurationDescriptor, 0, -1, -1, -1, -1);
    if (InterfaceDescriptor == NULL)
    {
         DPRINT1("Error USBD_ParseConfigurationDescriptorEx failed to parse interface descriptor\n");
         return STATUS_INVALID_PARAMETER;
    }

    ASSERT(InterfaceDescriptor);

    //
    // Construct the CompatibleIds
    //
    if (IsCompositeDevice(DeviceDescriptor, ConfigurationDescriptor))
    {
        //
        // sanity checks
        //
        ASSERT(DeviceDescriptor->bNumConfigurations == 1);
        ASSERT(ConfigurationDescriptor->bNumInterfaces > 1);
        Index += swprintf(&Buffer[Index],
                          L"USB\\DevClass_%02x&SubClass_%02x&Prot_%02x",
                          DeviceDescriptor->bDeviceClass, DeviceDescriptor->bDeviceSubClass, DeviceDescriptor->bDeviceProtocol) + 1;
        Index += swprintf(&Buffer[Index],
                          L"USB\\DevClass_%02x&SubClass_%02x",
                          DeviceDescriptor->bDeviceClass, DeviceDescriptor->bDeviceSubClass) + 1;
        Index += swprintf(&Buffer[Index],
                          L"USB\\DevClass_%02x",
                          DeviceDescriptor->bDeviceClass) + 1;
        Index += swprintf(&Buffer[Index],
                          L"USB\\COMPOSITE") + 1;
    }
    else
    {
        //
        // FIXME: support multiple configurations
        //
        ASSERT(DeviceDescriptor->bNumConfigurations == 1);

        if (DeviceDescriptor->bDeviceClass == 0)
        {
            Index += swprintf(&Buffer[Index],
                          L"USB\\Class_%02x&SubClass_%02x&Prot_%02x",
                          InterfaceDescriptor->bInterfaceClass, InterfaceDescriptor->bInterfaceSubClass, InterfaceDescriptor->bInterfaceProtocol) + 1;
            Index += swprintf(&Buffer[Index],
                          L"USB\\Class_%02x&SubClass_%02x",
                          InterfaceDescriptor->bInterfaceClass, InterfaceDescriptor->bInterfaceSubClass) + 1;
            Index += swprintf(&Buffer[Index],
                          L"USB\\Class_%02x",
                          InterfaceDescriptor->bInterfaceClass) + 1;
        }
        else
        {
            Index += swprintf(&Buffer[Index],
                          L"USB\\Class_%02x&SubClass_%02x&Prot_%02x",
                          DeviceDescriptor->bDeviceClass, DeviceDescriptor->bDeviceSubClass, DeviceDescriptor->bDeviceProtocol) + 1;
            Index += swprintf(&Buffer[Index],
                          L"USB\\Class_%02x&SubClass_%02x",
                          DeviceDescriptor->bDeviceClass, DeviceDescriptor->bDeviceSubClass) + 1;
            Index += swprintf(&Buffer[Index],
                          L"USB\\Class_%02x",
                          DeviceDescriptor->bDeviceClass) + 1;
        }
    }

    //
    // now allocate the buffer
    //
    DeviceString = ExAllocatePool(NonPagedPool, (Index + 1) * sizeof(WCHAR));
    if (!DeviceString)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy buffer
    //
    RtlCopyMemory(DeviceString, Buffer, Index * sizeof(WCHAR));
    DeviceString[Index] = UNICODE_NULL;
    UsbChildExtension->usCompatibleIds.Buffer = DeviceString;
    UsbChildExtension->usCompatibleIds.Length = Index * sizeof(WCHAR);
    UsbChildExtension->usCompatibleIds.MaximumLength = (Index + 1) * sizeof(WCHAR);
    DPRINT("usCompatibleIds %wZ\n", &UsbChildExtension->usCompatibleIds);

    //
    // Construct DeviceId string
    //
    Index = swprintf(Buffer, L"USB\\Vid_%04x&Pid_%04x", UsbChildExtension->DeviceDesc.idVendor, UsbChildExtension->DeviceDesc.idProduct) + 1;

    //
    // now allocate the buffer
    //
    DeviceString = ExAllocatePool(NonPagedPool, Index * sizeof(WCHAR));
    if (!DeviceString)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy buffer
    //
    RtlCopyMemory(DeviceString, Buffer, Index * sizeof(WCHAR));
    UsbChildExtension->usDeviceId.Buffer = DeviceString;
    UsbChildExtension->usDeviceId.Length = (Index-1) * sizeof(WCHAR);
    UsbChildExtension->usDeviceId.MaximumLength = Index * sizeof(WCHAR);
    DPRINT("usDeviceId %wZ\n", &UsbChildExtension->usDeviceId);

    //
    // Construct HardwareIds
    //
    Index = 0;
    Index += swprintf(&Buffer[Index],
                      L"USB\\Vid_%04x&Pid_%04x&Rev_%04x",
                      UsbChildExtension->DeviceDesc.idVendor, UsbChildExtension->DeviceDesc.idProduct, UsbChildExtension->DeviceDesc.bcdDevice) + 1;
    Index += swprintf(&Buffer[Index],
                      L"USB\\Vid_%04x&Pid_%04x",
                      UsbChildExtension->DeviceDesc.idVendor, UsbChildExtension->DeviceDesc.idProduct) + 1;

    //
    // now allocate the buffer
    //
    DeviceString = ExAllocatePool(NonPagedPool, (Index + 1) * sizeof(WCHAR));
    if (!DeviceString)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy buffer
    //
    RtlCopyMemory(DeviceString, Buffer, Index * sizeof(WCHAR));
    DeviceString[Index] = UNICODE_NULL;
    UsbChildExtension->usHardwareIds.Buffer = DeviceString;
    UsbChildExtension->usHardwareIds.Length = (Index + 1) * sizeof(WCHAR);
    UsbChildExtension->usHardwareIds.MaximumLength = (Index + 1) * sizeof(WCHAR);
    DPRINT("usHardWareIds %wZ\n", &UsbChildExtension->usHardwareIds);

    //
    // FIXME: Handle Lang ids
    //

    //
    // Get the product string if obe provided
    //
    if (UsbChildExtension->DeviceDesc.iProduct)
    {
        Status = GetUsbStringDescriptor(UsbChildDeviceObject,
                                        UsbChildExtension->DeviceDesc.iProduct,
                                        0,
                                        (PVOID*)&UsbChildExtension->usTextDescription.Buffer,
                                        &UsbChildExtension->usTextDescription.Length);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("USBHUB: GetUsbStringDescriptor failed with status %x\n", Status);
            RtlInitUnicodeString(&UsbChildExtension->usTextDescription, L"USB Device"); // FIXME NON-NLS
        }
        else
        {
            UsbChildExtension->usTextDescription.MaximumLength = UsbChildExtension->usTextDescription.Length;
            DPRINT("Usb TextDescription %wZ\n", &UsbChildExtension->usTextDescription);
        }
    }

    //
    // Get the Serial Number string if obe provided
    //
    if (UsbChildExtension->DeviceDesc.iSerialNumber)
    {
       LPWSTR SerialBuffer = NULL;

        Status = GetUsbStringDescriptor(UsbChildDeviceObject,
                                        UsbChildExtension->DeviceDesc.iSerialNumber,
                                        0,
                                        (PVOID*)&SerialBuffer,
                                        &UsbChildExtension->usInstanceId.Length);
        if (NT_SUCCESS(Status))
        {
            // construct instance id buffer
            Index = swprintf(Buffer, L"%04d&%s", HubDeviceExtension->InstanceCount, SerialBuffer) + 1;

            ExFreePool(SerialBuffer);

            UsbChildExtension->usInstanceId.Buffer = (LPWSTR)ExAllocatePool(NonPagedPool, Index * sizeof(WCHAR));
            if (UsbChildExtension->usInstanceId.Buffer == NULL)
            {
                DPRINT1("Error: failed to allocate %lu bytes\n", Index * sizeof(WCHAR));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // copy instance id
            //
            RtlCopyMemory(UsbChildExtension->usInstanceId.Buffer, Buffer, Index * sizeof(WCHAR));
            UsbChildExtension->usInstanceId.Length = UsbChildExtension->usInstanceId.MaximumLength = Index * sizeof(WCHAR);

            DPRINT("Usb InstanceId %wZ InstanceCount %x\n", &UsbChildExtension->usInstanceId, HubDeviceExtension->InstanceCount);
            return Status;
        }
    }

    //
    // the device did not provide a serial number, or failed to retrieve the serial number
    // lets create a pseudo instance id
    //
    Index = swprintf(Buffer, L"%04d&%04d", HubDeviceExtension->InstanceCount, UsbChildExtension->PortNumber) + 1;
    UsbChildExtension->usInstanceId.Buffer = (LPWSTR)ExAllocatePool(NonPagedPool, Index * sizeof(WCHAR));
    if (UsbChildExtension->usInstanceId.Buffer == NULL)
    {
       DPRINT1("Error: failed to allocate %lu bytes\n", Index * sizeof(WCHAR));
       Status = STATUS_INSUFFICIENT_RESOURCES;
       return Status;
    }

    //
    // copy instance id
    //
    RtlCopyMemory(UsbChildExtension->usInstanceId.Buffer, Buffer, Index * sizeof(WCHAR));
    UsbChildExtension->usInstanceId.Length = UsbChildExtension->usInstanceId.MaximumLength = Index * sizeof(WCHAR);

    DPRINT("usDeviceId %wZ\n", &UsbChildExtension->usInstanceId);
    return STATUS_SUCCESS;
}

NTSTATUS
DestroyUsbChildDeviceObject(
    IN PDEVICE_OBJECT UsbHubDeviceObject,
    IN LONG PortId)
{
    PHUB_DEVICE_EXTENSION HubDeviceExtension = (PHUB_DEVICE_EXTENSION)UsbHubDeviceObject->DeviceExtension;
    PHUB_CHILDDEVICE_EXTENSION UsbChildExtension = NULL;
    PDEVICE_OBJECT ChildDeviceObject = NULL;
    ULONG Index = 0;

    KeAcquireGuardedMutex(&HubDeviceExtension->HubMutexLock);
    for (Index = 0; Index < USB_MAXCHILDREN; Index++)
    {
        if (HubDeviceExtension->ChildDeviceObject[Index])
        {
            UsbChildExtension = (PHUB_CHILDDEVICE_EXTENSION)HubDeviceExtension->ChildDeviceObject[Index]->DeviceExtension;

            /* Check if it matches the port ID */
            if (UsbChildExtension->PortNumber == PortId)
            {
                /* We found it */
                ChildDeviceObject = HubDeviceExtension->ChildDeviceObject[Index];
                break;
            }
        }
    }

    /* Fail the request if the device doesn't exist */
    if (!ChildDeviceObject)
    {
        DPRINT1("Removal request for non-existant device!\n");
        KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);
        return STATUS_UNSUCCESSFUL;
    }

    DPRINT("Removing device on port %d (Child index: %d)\n", PortId, Index);

    /* Remove the device from the table */
    HubDeviceExtension->ChildDeviceObject[Index] = NULL;

    KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);

    /* Invalidate device relations for the root hub */
    IoInvalidateDeviceRelations(HubDeviceExtension->RootHubPhysicalDeviceObject, BusRelations);

    /* The rest of the removal process takes place in IRP_MN_REMOVE_DEVICE handling for the PDO */
    return STATUS_SUCCESS;
}

NTSTATUS
CreateUsbChildDeviceObject(
    IN PDEVICE_OBJECT UsbHubDeviceObject,
    IN LONG PortId,
    OUT PDEVICE_OBJECT *UsbChildDeviceObject,
    IN ULONG PortStatus)
{
    NTSTATUS Status;
    PDEVICE_OBJECT RootHubDeviceObject, NewChildDeviceObject;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PHUB_CHILDDEVICE_EXTENSION UsbChildExtension;
    PUSB_BUS_INTERFACE_HUB_V5 HubInterface;
    ULONG ChildDeviceCount, UsbDeviceNumber = 0;
    WCHAR CharDeviceName[64];
    UNICODE_STRING DeviceName;
    ULONG ConfigDescSize, DeviceDescSize, DeviceInfoSize;
    PVOID HubInterfaceBusContext;
    USB_CONFIGURATION_DESCRIPTOR ConfigDesc;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) UsbHubDeviceObject->DeviceExtension;
    HubInterface = &HubDeviceExtension->HubInterface;
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;
    HubInterfaceBusContext = HubDeviceExtension->UsbDInterface.BusContext;

    while (TRUE)
    {
        //
        // Create a Device Name
        //
        swprintf(CharDeviceName, L"\\Device\\USBPDO-%d", UsbDeviceNumber);

        //
        // Initialize UnicodeString
        //
        RtlInitUnicodeString(&DeviceName, CharDeviceName);

        //
        // Create a DeviceObject
        //
        Status = IoCreateDevice(UsbHubDeviceObject->DriverObject,
                                sizeof(HUB_CHILDDEVICE_EXTENSION),
                                NULL,
                                FILE_DEVICE_CONTROLLER,
                                FILE_AUTOGENERATED_DEVICE_NAME,
                                FALSE,
                                &NewChildDeviceObject);

        //
        // Check if the name is already in use
        //
        if ((Status == STATUS_OBJECT_NAME_EXISTS) || (Status == STATUS_OBJECT_NAME_COLLISION))
        {
            //
            // Try next name
            //
            UsbDeviceNumber++;
            continue;
        }

        //
        // Check for other errors
        //
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("USBHUB: IoCreateDevice failed with status %x\n", Status);
            return Status;
        }

        DPRINT("USBHUB: Created Device %x\n", NewChildDeviceObject);
        break;
    }

    NewChildDeviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;

    //
    // Assign the device extensions
    //
    UsbChildExtension = (PHUB_CHILDDEVICE_EXTENSION)NewChildDeviceObject->DeviceExtension;
    RtlZeroMemory(UsbChildExtension, sizeof(HUB_CHILDDEVICE_EXTENSION));
    UsbChildExtension->ParentDeviceObject = UsbHubDeviceObject;
    UsbChildExtension->PortNumber = PortId;

    //
    // Create the UsbDeviceObject
    //
    Status = HubInterface->CreateUsbDevice(HubInterfaceBusContext,
                                           (PVOID)&UsbChildExtension->UsbDeviceHandle,
                                           HubDeviceExtension->RootHubHandle,
                                           PortStatus,
                                           PortId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBHUB: CreateUsbDevice failed with status %x\n", Status);
        goto Cleanup;
    }

    //
    // Initialize UsbDevice
    //
    Status = HubInterface->InitializeUsbDevice(HubInterfaceBusContext, UsbChildExtension->UsbDeviceHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBHUB: InitializeUsbDevice failed with status %x\n", Status);
        goto Cleanup;
    }

    DPRINT("Usb Device Handle %x\n", UsbChildExtension->UsbDeviceHandle);

    ConfigDescSize = sizeof(USB_CONFIGURATION_DESCRIPTOR);
    DeviceDescSize = sizeof(USB_DEVICE_DESCRIPTOR);

    //
    // Get the descriptors
    //
    Status = HubInterface->GetUsbDescriptors(HubInterfaceBusContext,
                                             UsbChildExtension->UsbDeviceHandle,
                                             (PUCHAR)&UsbChildExtension->DeviceDesc,
                                             &DeviceDescSize,
                                             (PUCHAR)&ConfigDesc,
                                             &ConfigDescSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBHUB: GetUsbDescriptors failed with status %x\n", Status);
        goto Cleanup;
    }

    //DumpDeviceDescriptor(&UsbChildExtension->DeviceDesc);
    //DumpConfigurationDescriptor(&ConfigDesc);

    //
    // FIXME: Support more than one configuration and one interface?
    //
    if (UsbChildExtension->DeviceDesc.bNumConfigurations > 1)
    {
        DPRINT1("Warning: Device has more than one configuration. Only one configuration (the first) is supported!\n");
    }

    if (ConfigDesc.bNumInterfaces > 1)
    {
        DPRINT1("Warning: Device has more than one interface. Only one interface (the first) is currently supported\n");
    }

    ConfigDescSize = ConfigDesc.wTotalLength;

    //
    // Allocate memory for the first full descriptor, including interfaces and endpoints.
    //
    UsbChildExtension->FullConfigDesc = ExAllocatePoolWithTag(PagedPool, ConfigDescSize, USB_HUB_TAG);

    //
    // Retrieve the full configuration descriptor
    //
    Status = GetUsbDeviceDescriptor(NewChildDeviceObject,
                                    USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                    0,
                                    0,
                                    UsbChildExtension->FullConfigDesc,
                                    ConfigDescSize);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBHUB: GetUsbDeviceDescriptor failed with status %x\n", Status);
        goto Cleanup;
    }

    // query device details
    Status = HubInterface->QueryDeviceInformation(HubInterfaceBusContext,
                                         UsbChildExtension->UsbDeviceHandle,
                                         &UsbChildExtension->DeviceInformation,
                                         sizeof(USB_DEVICE_INFORMATION_0),
                                         &DeviceInfoSize);


    //DumpFullConfigurationDescriptor(UsbChildExtension->FullConfigDesc);

    //
    // Construct all the strings that will describe the device to PNP
    //
    Status = CreateDeviceIds(NewChildDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create strings needed to describe device to PNP.\n");
        goto Cleanup;
    }

    // copy device interface
    RtlCopyMemory(&UsbChildExtension->DeviceInterface, &HubDeviceExtension->UsbDInterface, sizeof(USB_BUS_INTERFACE_USBDI_V2));
    UsbChildExtension->DeviceInterface.InterfaceReference(UsbChildExtension->DeviceInterface.BusContext);

    INITIALIZE_PNP_STATE(UsbChildExtension->Common);

    IoInitializeRemoveLock(&UsbChildExtension->Common.RemoveLock, 'pbuH', 0, 0);

    KeAcquireGuardedMutex(&HubDeviceExtension->HubMutexLock);

    //
    // Find an empty slot in the child device array
    //
    for (ChildDeviceCount = 0; ChildDeviceCount < USB_MAXCHILDREN; ChildDeviceCount++)
    {
        if (HubDeviceExtension->ChildDeviceObject[ChildDeviceCount] == NULL)
        {
            DPRINT("Found unused entry at %d\n", ChildDeviceCount);
            break;
        }
    }

    //
    // Check if the limit has been reached for maximum usb devices
    //
    if (ChildDeviceCount == USB_MAXCHILDREN)
    {
        DPRINT1("USBHUB: Too many child devices!\n");
        Status = STATUS_UNSUCCESSFUL;
        KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);
        UsbChildExtension->DeviceInterface.InterfaceDereference(UsbChildExtension->DeviceInterface.BusContext);
        goto Cleanup;
    }

    HubDeviceExtension->ChildDeviceObject[ChildDeviceCount] = NewChildDeviceObject;
    HubDeviceExtension->InstanceCount++;
    KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);

    IoInvalidateDeviceRelations(RootHubDeviceObject, BusRelations);
    return STATUS_SUCCESS;

Cleanup:

    //
    // Remove the usb device if it was created
    //
    if (UsbChildExtension->UsbDeviceHandle)
        HubInterface->RemoveUsbDevice(HubInterfaceBusContext, UsbChildExtension->UsbDeviceHandle, 0);

    //
    // Free full configuration descriptor if one was allocated
    //
    if (UsbChildExtension->FullConfigDesc)
        ExFreePool(UsbChildExtension->FullConfigDesc);

    //
    // Free ID buffers if they were allocated in CreateDeviceIds()
    //
    if (UsbChildExtension->usCompatibleIds.Buffer)
        ExFreePool(UsbChildExtension->usCompatibleIds.Buffer);

    if (UsbChildExtension->usDeviceId.Buffer)
        ExFreePool(UsbChildExtension->usDeviceId.Buffer);

    if (UsbChildExtension->usHardwareIds.Buffer)
        ExFreePool(UsbChildExtension->usHardwareIds.Buffer);

    if (UsbChildExtension->usInstanceId.Buffer)
        ExFreePool(UsbChildExtension->usInstanceId.Buffer);

    //
    // Delete the device object
    //
    IoDeleteDevice(NewChildDeviceObject);
    return Status;
}

NTSTATUS
USBHUB_FdoQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_RELATIONS RelationsFromTop,
    OUT PDEVICE_RELATIONS* pDeviceRelations)
{
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG i;
    ULONG ChildrenFromTop = 0;
    ULONG Children = 0;
    ULONG NeededSize;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    KeAcquireGuardedMutex(&HubDeviceExtension->HubMutexLock);

    //
    // Count the number of children
    //
    for (i = 0; i < USB_MAXCHILDREN; i++)
    {

        if (HubDeviceExtension->ChildDeviceObject[i] == NULL)
        {
            continue;
        }
        Children++;
    }

    if (RelationsFromTop)
    {
        ChildrenFromTop = RelationsFromTop->Count;
        if (!Children)
        {
            // We have nothing to add
            *pDeviceRelations = RelationsFromTop;
            KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);
            return STATUS_SUCCESS;
        }
    }

    NeededSize = sizeof(DEVICE_RELATIONS) + (Children + ChildrenFromTop - 1) * sizeof(PDEVICE_OBJECT);

    //
    // Allocate DeviceRelations
    //
    DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool,
                                                        NeededSize);

    if (!DeviceRelations)
    {
        KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);
        if (!RelationsFromTop)
            return STATUS_INSUFFICIENT_RESOURCES;
        else
            return STATUS_NOT_SUPPORTED;
    }
    // Copy the objects coming from top
    if (ChildrenFromTop)
    {
        RtlCopyMemory(DeviceRelations->Objects, RelationsFromTop->Objects,
                      ChildrenFromTop * sizeof(PDEVICE_OBJECT));
    }

    DeviceRelations->Count = Children + ChildrenFromTop;
    Children = ChildrenFromTop;

    //
    // Fill in return structure
    //
    for (i = 0; i < USB_MAXCHILDREN; i++)
    {
        if (HubDeviceExtension->ChildDeviceObject[i])
        {
            // The PnP Manager removes the reference when appropriate.
            ObReferenceObject(HubDeviceExtension->ChildDeviceObject[i]);
            HubDeviceExtension->ChildDeviceObject[i]->Flags &= ~DO_DEVICE_INITIALIZING;
            DeviceRelations->Objects[Children++] = HubDeviceExtension->ChildDeviceObject[i];
        }
    }

    KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);

    // We should do this, because replaced this with our's one
    if (RelationsFromTop)
        ExFreePool(RelationsFromTop);

    ASSERT(Children == DeviceRelations->Count);
    *pDeviceRelations = DeviceRelations;

    return STATUS_SUCCESS;
}

VOID
NTAPI
RootHubInitCallbackFunction(
    PVOID Context)
{
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;
    NTSTATUS Status;
    ULONG PortId;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PORT_STATUS_CHANGE StatusChange;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    DPRINT("RootHubInitCallbackFunction Sending the initial SCE Request %x\n", DeviceObject);

    //
    // Send the first SCE Request
    //
    QueryStatusChangeEndpoint(DeviceObject);

    for (PortId = 1; PortId <= HubDeviceExtension->HubDescriptor.bNumberOfPorts; PortId++)
    {
        //
        // get port status
        //
        Status = GetPortStatusAndChange(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, &StatusChange);
        if (NT_SUCCESS(Status))
        {
            //
            // is there a device connected
            //
            if (StatusChange.Status & USB_PORT_STATUS_CONNECT)
            {
                //
                // reset port
                //
                Status = SetPortFeature(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, PORT_RESET);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to reset on port %d\n", PortId);
                }
                else
                {
                    //
                    // wait for the reset to be handled since we want to enumerate synchronously
                    //
                    KeWaitForSingleObject(&HubDeviceExtension->ResetComplete,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                    KeClearEvent(&HubDeviceExtension->ResetComplete);
                }
            }
        }
    }
}

BOOLEAN
USBHUB_IsRootHubFDO(
   IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT RootHubPhysicalDeviceObject = NULL;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;

    // get hub device extension
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // Get the Root Hub Pdo
    Status = SubmitRequestToRootHub(HubDeviceExtension->LowerDeviceObject,
                                    IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO,
                                    &RootHubPhysicalDeviceObject,
                                    NULL);

    // FIXME handle error
    ASSERT(NT_SUCCESS(Status));

    // physical device object is only obtained for root hubs
    return (RootHubPhysicalDeviceObject != NULL);
}


NTSTATUS
USBHUB_FdoStartDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
    PURB Urb;
    PUSB_INTERFACE_DESCRIPTOR Pid;
    ULONG Result = 0, PortId;
    USBD_INTERFACE_LIST_ENTRY InterfaceList[2] = {{NULL, NULL}, {NULL, NULL}};
    PURB ConfigUrb = NULL;
    ULONG HubStatus;
    NTSTATUS Status = STATUS_SUCCESS;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PDEVICE_OBJECT RootHubDeviceObject;
    PVOID HubInterfaceBusContext;
    PORT_STATUS_CHANGE StatusChange;

    // get hub device extension
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");

    // Allocated size including the sizeof USBD_INTERFACE_LIST_ENTRY
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB) + sizeof(USBD_INTERFACE_LIST_ENTRY), USB_HUB_TAG);
    if (!Urb)
    {
         // no memory
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    // zero urb
    RtlZeroMemory(Urb, sizeof(URB) + sizeof(USBD_INTERFACE_LIST_ENTRY));

    // Get the Root Hub Pdo
    Status = SubmitRequestToRootHub(HubDeviceExtension->LowerDeviceObject,
                                    IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO,
                                    &HubDeviceExtension->RootHubPhysicalDeviceObject,
                                    &HubDeviceExtension->RootHubFunctionalDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        // failed to obtain hub pdo
        DPRINT1("IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO failed with %x\n", Status);
        goto cleanup;
    }

    // sanity checks
    ASSERT(HubDeviceExtension->RootHubPhysicalDeviceObject);
    ASSERT(HubDeviceExtension->RootHubFunctionalDeviceObject);

    // get roothub
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;

    // Send the StartDevice to RootHub
    Status = ForwardIrpAndWait(HubDeviceExtension->LowerDeviceObject, Irp);

    if (!NT_SUCCESS(Status))
    {
        // failed to start pdo
        DPRINT1("Failed to start the RootHub PDO\n");
        goto cleanup;
    }

    // Get the current number of hubs
    Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                    IOCTL_INTERNAL_USB_GET_HUB_COUNT,
                                    &HubDeviceExtension->NumberOfHubs, NULL);
    if (!NT_SUCCESS(Status))
    {
        // failed to get number of hubs
        DPRINT1("IOCTL_INTERNAL_USB_GET_HUB_COUNT failed with %x\n", Status);
        goto cleanup;
    }

    // Get the Hub Interface
    Status = QueryInterface(RootHubDeviceObject,
                            USB_BUS_INTERFACE_HUB_GUID,
                            sizeof(USB_BUS_INTERFACE_HUB_V5),
                            USB_BUSIF_HUB_VERSION_5,
                            (PVOID)&HubDeviceExtension->HubInterface);

    if (!NT_SUCCESS(Status))
    {
        // failed to get root hub interface
        DPRINT1("Failed to get HUB_GUID interface with status 0x%08lx\n", Status);
        goto cleanup;
    }

    HubInterfaceBusContext = HubDeviceExtension->HubInterface.BusContext;

    // Get the USBDI Interface
    Status = QueryInterface(RootHubDeviceObject,
                            USB_BUS_INTERFACE_USBDI_GUID,
                            sizeof(USB_BUS_INTERFACE_USBDI_V2),
                            USB_BUSIF_USBDI_VERSION_2,
                            (PVOID)&HubDeviceExtension->UsbDInterface);

    if (!NT_SUCCESS(Status))
    {
        // failed to get usbdi interface
        DPRINT1("Failed to get USBDI_GUID interface with status 0x%08lx\n", Status);
        goto cleanup;
    }

    // Get Root Hub Device Handle
    Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                    IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE,
                                    &HubDeviceExtension->RootHubHandle,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        // failed
        DPRINT1("IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE failed with status 0x%08lx\n", Status);
        goto cleanup;
    }

    //
    // Get Hub Device Information
    //
    Status = HubDeviceExtension->HubInterface.QueryDeviceInformation(HubInterfaceBusContext,
                                                                        HubDeviceExtension->RootHubHandle,
                                                                        &HubDeviceExtension->DeviceInformation,
                                                                        sizeof(USB_DEVICE_INFORMATION_0),
                                                                        &Result);

    DPRINT("Status %x, Result 0x%08lx\n", Status, Result);
    DPRINT("InformationLevel %x\n", HubDeviceExtension->DeviceInformation.InformationLevel);
    DPRINT("ActualLength %x\n", HubDeviceExtension->DeviceInformation.ActualLength);
    DPRINT("PortNumber %x\n", HubDeviceExtension->DeviceInformation.PortNumber);
    DPRINT("DeviceDescriptor %x\n", HubDeviceExtension->DeviceInformation.DeviceDescriptor);
    DPRINT("HubAddress %x\n", HubDeviceExtension->DeviceInformation.HubAddress);
    DPRINT("NumberofPipes %x\n", HubDeviceExtension->DeviceInformation.NumberOfOpenPipes);

    // Get Root Hubs Device Descriptor
    UsbBuildGetDescriptorRequest(Urb,
                                    sizeof(Urb->UrbControlDescriptorRequest),
                                    USB_DEVICE_DESCRIPTOR_TYPE,
                                    0,
                                    0,
                                    &HubDeviceExtension->HubDeviceDescriptor,
                                    NULL,
                                    sizeof(USB_DEVICE_DESCRIPTOR),
                                    NULL);

    // set device handle
    Urb->UrbHeader.UsbdDeviceHandle = HubDeviceExtension->RootHubHandle;

    // get hub device descriptor
    Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    Urb,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        // failed to get device descriptor of hub
        DPRINT1("Failed to get HubDeviceDescriptor!\n");
        goto cleanup;
    }

    // build configuration request
    UsbBuildGetDescriptorRequest(Urb,
                                    sizeof(Urb->UrbControlDescriptorRequest),
                                    USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                    0,
                                    0,
                                    &HubDeviceExtension->HubConfigDescriptor,
                                    NULL,
                                    sizeof(USB_CONFIGURATION_DESCRIPTOR) + sizeof(USB_INTERFACE_DESCRIPTOR) + sizeof(USB_ENDPOINT_DESCRIPTOR),
                                    NULL);

    // set device handle
    Urb->UrbHeader.UsbdDeviceHandle = HubDeviceExtension->RootHubHandle;

    // request configuration descriptor
    Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    Urb,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        // failed to get configuration descriptor
        DPRINT1("Failed to get RootHub Configuration with status %x\n", Status);
        goto cleanup;
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

    // get hub information
    Status = HubDeviceExtension->HubInterface.GetExtendedHubInformation(HubInterfaceBusContext,
                                                                        RootHubDeviceObject,
                                                                        &HubDeviceExtension->UsbExtHubInfo,
                                                                        sizeof(USB_EXTHUB_INFORMATION_0),
                                                                        &Result);
    if (!NT_SUCCESS(Status))
    {
        // failed to get hub information
        DPRINT1("Failed to extended hub information. Unable to determine the number of ports!\n");
        goto cleanup;
    }

    if (!HubDeviceExtension->UsbExtHubInfo.NumberOfPorts)
    {
        // bogus port driver
        DPRINT1("Failed to retrieve the number of ports\n");
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    DPRINT("HubDeviceExtension->UsbExtHubInfo.NumberOfPorts %x\n", HubDeviceExtension->UsbExtHubInfo.NumberOfPorts);

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

    // set device handle
    Urb->UrbHeader.UsbdDeviceHandle = HubDeviceExtension->RootHubHandle;

    // send request
    Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    Urb,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get Hub Descriptor!\n");
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    // sanity checks
    ASSERT(HubDeviceExtension->HubDescriptor.bDescriptorLength == sizeof(USB_HUB_DESCRIPTOR));
    ASSERT(HubDeviceExtension->HubDescriptor.bNumberOfPorts == HubDeviceExtension->UsbExtHubInfo.NumberOfPorts);
    ASSERT(HubDeviceExtension->HubDescriptor.bDescriptorType == 0x29);

    // build get status request
    HubStatus = 0;
    UsbBuildGetStatusRequest(Urb,
                                URB_FUNCTION_GET_STATUS_FROM_DEVICE,
                                0,
                                &HubStatus,
                                0,
                                NULL);
    // set device handle
    Urb->UrbHeader.UsbdDeviceHandle = HubDeviceExtension->RootHubHandle;

    // send request
    Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    Urb,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        // failed to get hub status
        DPRINT1("Failed to get Hub Status!\n");
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    // Allocate memory for PortStatusChange to hold 2 USHORTs for each port on hub
    HubDeviceExtension->PortStatusChange = ExAllocatePoolWithTag(NonPagedPool,
                                                                    sizeof(ULONG) * HubDeviceExtension->UsbExtHubInfo.NumberOfPorts,
                                                                    USB_HUB_TAG);

    if (!HubDeviceExtension->PortStatusChange)
    {
        DPRINT1("Failed to allocate pool for PortStatusChange!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    // Get the first Configuration Descriptor
    Pid = USBD_ParseConfigurationDescriptorEx(&HubDeviceExtension->HubConfigDescriptor,
                                                &HubDeviceExtension->HubConfigDescriptor,
                                                -1, -1, -1, -1, -1);
    if (Pid == NULL)
    {
        // failed parse hub descriptor
        DPRINT1("Failed to parse configuration descriptor\n");
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    // create configuration request
    InterfaceList[0].InterfaceDescriptor = Pid;
    ConfigUrb = USBD_CreateConfigurationRequestEx(&HubDeviceExtension->HubConfigDescriptor,
                                                    (PUSBD_INTERFACE_LIST_ENTRY)&InterfaceList);
    if (ConfigUrb == NULL)
    {
        // failed to build urb
        DPRINT1("Failed to allocate urb\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    // send request
    Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    ConfigUrb,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        // failed to select configuration
        DPRINT1("Failed to select configuration with %x\n", Status);
        goto cleanup;
    }

    // store configuration & pipe handle
    HubDeviceExtension->ConfigurationHandle = ConfigUrb->UrbSelectConfiguration.ConfigurationHandle;
    HubDeviceExtension->PipeHandle = ConfigUrb->UrbSelectConfiguration.Interface.Pipes[0].PipeHandle;
    DPRINT("Configuration Handle %x\n", HubDeviceExtension->ConfigurationHandle);

    // check if function is available
    if (HubDeviceExtension->UsbDInterface.IsDeviceHighSpeed)
    {
        // is it high speed bus
        if (HubDeviceExtension->UsbDInterface.IsDeviceHighSpeed(HubInterfaceBusContext))
        {
            // initialize usb 2.0 hub
            Status = HubDeviceExtension->HubInterface.Initialize20Hub(HubInterfaceBusContext,
                                                                        HubDeviceExtension->RootHubHandle, 1);
            DPRINT("Status %x\n", Status);

            // FIXME handle error
            ASSERT(Status == STATUS_SUCCESS);
        }
    }


    // Enable power on all ports
    DPRINT("Enabling PortPower on all ports!\n");
    for (PortId = 1; PortId <= HubDeviceExtension->HubDescriptor.bNumberOfPorts; PortId++)
    {
        Status = SetPortFeature(RootHubDeviceObject, PortId, PORT_POWER);
        if (!NT_SUCCESS(Status))
            DPRINT1("Failed to power on port %d\n", PortId);

        Status = ClearPortFeature(RootHubDeviceObject, PortId, C_PORT_CONNECTION);
        if (!NT_SUCCESS(Status))
            DPRINT1("Failed to power on port %d\n", PortId);
    }

    // init root hub notification
    if (HubDeviceExtension->HubInterface.RootHubInitNotification)
    {
        Status = HubDeviceExtension->HubInterface.RootHubInitNotification(HubInterfaceBusContext,
                                                                            DeviceObject,
                                                                            RootHubInitCallbackFunction);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to set callback\n");
            goto cleanup;
        }
    }
    else
    {
        // Send the first SCE Request
        QueryStatusChangeEndpoint(DeviceObject);

        //
        // reset ports
        //
        for (PortId = 1; PortId <= HubDeviceExtension->HubDescriptor.bNumberOfPorts; PortId++)
        {
            //
            // get port status
            //
            Status = GetPortStatusAndChange(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, &StatusChange);
            if (NT_SUCCESS(Status))
            {
                //
                // is there a device connected
                //
                if (StatusChange.Status & USB_PORT_STATUS_CONNECT)
                {
                    //
                    // reset port
                    //
                    Status = SetPortFeature(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, PORT_RESET);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("Failed to reset on port %d\n", PortId);
                    }
                    else
                    {
                        //
                        // wait for the reset to be handled since we want to enumerate synchronously
                        //
                        KeWaitForSingleObject(&HubDeviceExtension->ResetComplete,
                                                Executive,
                                                KernelMode,
                                                FALSE,
                                                NULL);
                        KeClearEvent(&HubDeviceExtension->ResetComplete);
                    }
                }
            }
        }
    }

    // free urb
    ExFreePool(Urb);

    // free ConfigUrb
    ExFreePool(ConfigUrb);

    // done
    return Status;

cleanup:
    if (Urb)
        ExFreePool(Urb);

    // Dereference interfaces
    if (HubDeviceExtension->HubInterface.Size)
        HubDeviceExtension->HubInterface.InterfaceDereference(HubDeviceExtension->HubInterface.BusContext);

    if (HubDeviceExtension->UsbDInterface.Size)
        HubDeviceExtension->UsbDInterface.InterfaceDereference(HubDeviceExtension->UsbDInterface.BusContext);

    if (HubDeviceExtension->PortStatusChange)
        ExFreePool(HubDeviceExtension->PortStatusChange);

    if (ConfigUrb)
        ExFreePool(ConfigUrb);

    return Status;
}

NTSTATUS
USBHUB_FdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT ChildDeviceObject;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PUSB_BUS_INTERFACE_HUB_V5 HubInterface;
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    HubInterface = &HubDeviceExtension->HubInterface;
    Stack = IoGetCurrentIrpStackLocation(Irp);

    Status = IoAcquireRemoveLock(&HubDeviceExtension->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    switch (Stack->MinorFunction)
    {
        int i;

        case IRP_MN_START_DEVICE:
        {
            DPRINT("IRP_MN_START_DEVICE\n");
            if (USBHUB_IsRootHubFDO(DeviceObject))
            {
                // start root hub fdo
                Status = USBHUB_FdoStartDevice(DeviceObject, Irp);
            }
            else
            {
                Status = USBHUB_ParentFDOStartDevice(DeviceObject, Irp);
            }

            SET_NEW_PNP_STATE(HubDeviceExtension->Common, Started);

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            IoReleaseRemoveLock(&HubDeviceExtension->Common.RemoveLock, Irp);
            return Status;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            switch (Stack->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                {
                    PDEVICE_RELATIONS DeviceRelations = NULL;
                    PDEVICE_RELATIONS RelationsFromTop = (PDEVICE_RELATIONS)Irp->IoStatus.Information;
                    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");

                    Status = USBHUB_FdoQueryBusRelations(DeviceObject, RelationsFromTop, &DeviceRelations);

                    if (!NT_SUCCESS(Status))
                    {
                        if (Status == STATUS_NOT_SUPPORTED)
                        {
                            // We should process this to not lose relations from top.
                            Irp->IoStatus.Status = STATUS_SUCCESS;
                            break;
                        }
                        // We should fail an IRP
                        Irp->IoStatus.Status = Status;
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        IoReleaseRemoveLock(&HubDeviceExtension->Common.RemoveLock, Irp);
                        return Status;
                    }

                    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
                    Irp->IoStatus.Status = Status;
                    break;
                }
                case RemovalRelations:
                {
                    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
                    break;
                }
                default:
                    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
                            Stack->Parameters.QueryDeviceRelations.Type);
                    break;
            }
            break;
        }
        case IRP_MN_QUERY_STOP_DEVICE:
        {
            //
            // We should fail this request, because we're not handling
            // IRP_MN_STOP_DEVICE for now.We'll receive this IRP ONLY when
            // PnP manager rebalances resources.
            //
            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_SUPPORTED;
        }
        case IRP_MN_QUERY_REMOVE_DEVICE:
        {
            // No action is required from FDO because it have nothing to free.
            DPRINT("IRP_MN_QUERY_REMOVE_DEVICE\n");

            SET_NEW_PNP_STATE(HubDeviceExtension->Common, RemovePending);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        {
            DPRINT("IRP_MN_CANCEL_REMOVE_DEVICE\n");

            if (HubDeviceExtension->Common.PnPState == RemovePending)
                RESTORE_PREVIOUS_PNP_STATE(HubDeviceExtension->Common);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_SURPRISE_REMOVAL:
        {
            //
            // We'll receive this IRP on HUB unexpected removal, or on USB
            // controller removal from PCI port. Here we should "let know" all
            // our children that their parent is removed and on next removal
            // they also can be removed.
            //
            SET_NEW_PNP_STATE(HubDeviceExtension->Common, SurpriseRemovePending);

            KeAcquireGuardedMutex(&HubDeviceExtension->HubMutexLock);

            for (i = 0; i < USB_MAXCHILDREN; i++)
            {
                ChildDeviceObject = HubDeviceExtension->ChildDeviceObject[i];
                if (ChildDeviceObject)
                {
                    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)ChildDeviceObject->DeviceObjectExtension;
                    ChildDeviceExtension->ParentDeviceObject = NULL;
                }
            }

            KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);

            // This IRP can't be failed
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            DPRINT("IRP_MN_REMOVE_DEVICE\n");

            SET_NEW_PNP_STATE(HubDeviceExtension->Common, Deleted);

            IoReleaseRemoveLockAndWait(&HubDeviceExtension->Common.RemoveLock, Irp);

            //
            // Here we should remove all child PDOs. At this point all children
            // received and returned from IRP_MN_REMOVE so remove synchronization
            // isn't needed here
            //

            KeAcquireGuardedMutex(&HubDeviceExtension->HubMutexLock);

            for (i = 0; i < USB_MAXCHILDREN; i++)
            {
                ChildDeviceObject = HubDeviceExtension->ChildDeviceObject[i];
                if (ChildDeviceObject)
                {
                    PHUB_CHILDDEVICE_EXTENSION UsbChildExtension = (PHUB_CHILDDEVICE_EXTENSION)ChildDeviceObject->DeviceExtension;

                    SET_NEW_PNP_STATE(UsbChildExtension->Common, Deleted);

                    // Remove the usb device
                    if (UsbChildExtension->UsbDeviceHandle)
                    {
                        Status = HubInterface->RemoveUsbDevice(HubInterface->BusContext, UsbChildExtension->UsbDeviceHandle, 0);
                        ASSERT(Status == STATUS_SUCCESS);
                    }

                    // Free full configuration descriptor
                    if (UsbChildExtension->FullConfigDesc)
                        ExFreePool(UsbChildExtension->FullConfigDesc);

                    // Free ID buffers
                    if (UsbChildExtension->usCompatibleIds.Buffer)
                        ExFreePool(UsbChildExtension->usCompatibleIds.Buffer);

                    if (UsbChildExtension->usDeviceId.Buffer)
                        ExFreePool(UsbChildExtension->usDeviceId.Buffer);

                    if (UsbChildExtension->usHardwareIds.Buffer)
                        ExFreePool(UsbChildExtension->usHardwareIds.Buffer);

                    if (UsbChildExtension->usInstanceId.Buffer)
                        ExFreePool(UsbChildExtension->usInstanceId.Buffer);

                    DPRINT("Deleting child PDO\n");
                    IoDeleteDevice(DeviceObject);
                    ChildDeviceObject = NULL;
                }
            }

            KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = ForwardIrpAndForget(DeviceObject, Irp);

            IoDetachDevice(HubDeviceExtension->LowerDeviceObject);
            DPRINT("Deleting FDO 0x%p\n", DeviceObject);
            IoDeleteDevice(DeviceObject);

            return Status;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            // Function drivers and filter drivers do not handle this IRP.
            DPRINT("IRP_MN_QUERY_BUS_INFORMATION\n");
            break;
        }
        case IRP_MN_QUERY_ID:
        {
            DPRINT("IRP_MN_QUERY_ID\n");
            // Function drivers and filter drivers do not handle this IRP.
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            //
            // If a function or filter driver does not handle this IRP, it
            // should pass that down.
            //
            DPRINT("IRP_MN_QUERY_CAPABILITIES\n");
            break;
        }
        default:
        {
            DPRINT(" IRP_MJ_PNP / unknown minor function 0x%lx\n", Stack->MinorFunction);
            break;
        }
    }

    Status = ForwardIrpAndForget(DeviceObject, Irp);
    IoReleaseRemoveLock(&HubDeviceExtension->Common.RemoveLock, Irp);
    return Status;
}

NTSTATUS
USBHUB_FdoHandleDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PUSB_NODE_INFORMATION NodeInformation;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PUSB_NODE_CONNECTION_INFORMATION NodeConnectionInfo;
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;
    PUSB_NODE_CONNECTION_DRIVERKEY_NAME NodeKey;
    PUSB_NODE_CONNECTION_NAME ConnectionName;
    ULONG Index, Length;

    // get stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get device extension
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    Status = IoAcquireRemoveLock(&HubDeviceExtension->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    // Prevent handling of control requests in remove pending state
    if (HubDeviceExtension->Common.PnPState == RemovePending)
    {
        DPRINT1("[USBHUB] Request for removed device object %p\n", DeviceObject);
        Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        IoReleaseRemoveLock(&HubDeviceExtension->Common.RemoveLock, Irp);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_USB_GET_NODE_INFORMATION)
    {
        // is the buffer big enough
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(USB_NODE_INFORMATION))
        {
            // buffer too small
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            // get buffer
            NodeInformation = (PUSB_NODE_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

            // sanity check
            ASSERT(NodeInformation);

            // init buffer
            NodeInformation->NodeType = UsbHub;
            RtlCopyMemory(&NodeInformation->u.HubInformation.HubDescriptor, &HubDeviceExtension->HubDescriptor, sizeof(USB_HUB_DESCRIPTOR));

            // FIXME is hub powered
            NodeInformation->u.HubInformation.HubIsBusPowered = TRUE;

            // done
            Irp->IoStatus.Information = sizeof(USB_NODE_INFORMATION);
            Status = STATUS_SUCCESS;
        }


    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_USB_GET_NODE_CONNECTION_INFORMATION)
    {
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(USB_NODE_CONNECTION_INFORMATION))
        {
            // buffer too small
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            // get node connection info
            NodeConnectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

            // sanity checks
            ASSERT(NodeConnectionInfo);

            KeAcquireGuardedMutex(&HubDeviceExtension->HubMutexLock);
            for(Index = 0; Index < USB_MAXCHILDREN; Index++)
            {
                if (HubDeviceExtension->ChildDeviceObject[Index] == NULL)
                    continue;

                // get child device extension
                ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)HubDeviceExtension->ChildDeviceObject[Index]->DeviceExtension;

                if (ChildDeviceExtension->PortNumber != NodeConnectionInfo->ConnectionIndex)
                   continue;

                // init node connection info
                RtlCopyMemory(&NodeConnectionInfo->DeviceDescriptor, &ChildDeviceExtension->DeviceDesc, sizeof(USB_DEVICE_DESCRIPTOR));
                NodeConnectionInfo->CurrentConfigurationValue = ChildDeviceExtension->FullConfigDesc->bConfigurationValue;
                NodeConnectionInfo->DeviceIsHub = FALSE; //FIXME support hubs
                NodeConnectionInfo->LowSpeed = ChildDeviceExtension->DeviceInformation.DeviceSpeed == UsbLowSpeed;
                NodeConnectionInfo->DeviceAddress = ChildDeviceExtension->DeviceInformation.DeviceAddress;
                NodeConnectionInfo->NumberOfOpenPipes = ChildDeviceExtension->DeviceInformation.NumberOfOpenPipes;
                NodeConnectionInfo->ConnectionStatus = DeviceConnected; //FIXME

                if (NodeConnectionInfo->NumberOfOpenPipes)
                {
                    DPRINT1("Need to copy pipe information\n");
                }
                break;
            }
            KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);
            // done
            Irp->IoStatus.Information = sizeof(USB_NODE_INFORMATION);
            Status = STATUS_SUCCESS;
        }
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME)
    {
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(USB_NODE_CONNECTION_INFORMATION))
        {
            // buffer too small
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            // get node connection info
            NodeKey = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)Irp->AssociatedIrp.SystemBuffer;

            // sanity checks
            ASSERT(NodeKey);

            KeAcquireGuardedMutex(&HubDeviceExtension->HubMutexLock);
            for(Index = 0; Index < USB_MAXCHILDREN; Index++)
            {
                if (HubDeviceExtension->ChildDeviceObject[Index] == NULL)
                    continue;

                // get child device extension
                ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)HubDeviceExtension->ChildDeviceObject[Index]->DeviceExtension;

                if (ChildDeviceExtension->PortNumber != NodeKey->ConnectionIndex)
                   continue;

                // get driver key
                Status = IoGetDeviceProperty(HubDeviceExtension->ChildDeviceObject[Index], DevicePropertyDriverKeyName,
                                             IoStack->Parameters.DeviceIoControl.OutputBufferLength - sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME),
                                             NodeKey->DriverKeyName,
                                             &Length);

                if (Status == STATUS_BUFFER_TOO_SMALL)
                {
                    // normalize status
                    Status = STATUS_SUCCESS;
                }

                if (Length + sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME) > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
                {
                    // terminate node key name
                    NodeKey->DriverKeyName[0] = UNICODE_NULL;
                    Irp->IoStatus.Information = sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME);
                }
                else
                {
                    // result size
                    Irp->IoStatus.Information = Length + sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME);
                }

                // length of driver name
                NodeKey->ActualLength = Length + sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME);
                break;
            }
            KeReleaseGuardedMutex(&HubDeviceExtension->HubMutexLock);
        }
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_USB_GET_NODE_CONNECTION_NAME)
    {
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(USB_NODE_CONNECTION_NAME))
        {
            // buffer too small
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            // FIXME support hubs
            ConnectionName = (PUSB_NODE_CONNECTION_NAME)Irp->AssociatedIrp.SystemBuffer;
            ConnectionName->ActualLength = 0;
            ConnectionName->NodeName[0] = UNICODE_NULL;

            // done
            Irp->IoStatus.Information = sizeof(USB_NODE_CONNECTION_NAME);
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        DPRINT1("UNIMPLEMENTED FdoHandleDeviceControl IoCtl %x InputBufferLength %x OutputBufferLength %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode,
           IoStack->Parameters.DeviceIoControl.InputBufferLength, IoStack->Parameters.DeviceIoControl.OutputBufferLength);
    }

    // finish irp
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&HubDeviceExtension->Common.RemoveLock, Irp);
    return Status;
}

