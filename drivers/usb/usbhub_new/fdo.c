/*
 * PROJECT:         ReactOS Universal Serial Bus Hub Driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/usb/usbhub/fdo.c
 * PURPOSE:         Handle FDO
 * PROGRAMMERS:
 *                  Michael Martin (michael.martin@reactos.org)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define INITGUID

#define NDEBUG
#include "usbhub.h"

NTSTATUS
QueryStatusChangeEndpoint(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
CreateUsbChildDeviceObject(
    IN PDEVICE_OBJECT UsbHubDeviceObject,
    IN LONG PortId,
    OUT PDEVICE_OBJECT *UsbChildDeviceObject);

NTSTATUS
SubmitRequestToRootHub(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG IoControlCode,
    OUT PVOID OutParameter1,
    OUT PVOID OutParameter2)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build Control Request
    //
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        RootHubDeviceObject,
                                        NULL, 0,
                                        NULL, 0,
                                        TRUE,
                                        &Event,
                                        &IoStatus);

    if (Irp == NULL)
    {
        DPRINT("Usbhub: IoBuildDeviceIoControlRequest() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the status block before sending the IRP
    //
    IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoStatus.Information = 0;

    //
    // Get Next Stack Location and Initialize it
    //
    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->Parameters.Others.Argument1 = OutParameter1;
    Stack->Parameters.Others.Argument2 = OutParameter2;

    //
    // Call RootHub
    //
    Status = IoCallDriver(RootHubDeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    return Status;
}

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
    // Create URB for getting Port Status
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
    // Create URB for Clearing Port Reset
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
    // Create URB for Clearing Port Reset
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
    PDEVICE_OBJECT DeviceObject;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PWORK_ITEM_DATA WorkItemData;
    PORT_STATUS_CHANGE PortStatus;
    LONG PortId;

    WorkItemData = (PWORK_ITEM_DATA)Context;
    DeviceObject = (PDEVICE_OBJECT)WorkItemData->Context;
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // Loop all ports
    //
    for (PortId = 1; PortId <= HubDeviceExtension->UsbExtHubInfo.NumberOfPorts; PortId++)
    {
        //
        // Get Port Status
        //
        Status = GetPortStatusAndChange(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, &PortStatus);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get port status for port %d, Status %x\n", PortId, Status);
            // FIXME: Do we really want to halt further SCE requests?
            return;
        }

        DPRINT1("Port %d Status %x\n", PortId, PortStatus.Status);
        DPRINT1("Port %d Change %x\n", PortId, PortStatus.Change);

        //
        // Check for new device connection
        //
        if ((PortStatus.Change == USB_PORT_STATUS_CONNECT) && (PortStatus.Status & USB_PORT_STATUS_CONNECT))
        {
            //
            // Clear Connection Status
            //
            Status = ClearPortFeature(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, C_PORT_CONNECTION);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to clear connection change for port %d\n", PortId);
            }

            // No SCE completion done for clearing C_PORT_CONNECT

            //
            // Reset Port
            //
            Status = SetPortFeature(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, PORT_RESET);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to reset port %d\n", PortId);
            }
        }
        else if ((PortStatus.Change == USB_PORT_STATUS_ENABLE) &&
            (PortStatus.Status & (USB_PORT_STATUS_CONNECT | USB_PORT_STATUS_ENABLE)))
        {
            //
            // Clear Enable
            //
            Status = ClearPortFeature(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, C_PORT_ENABLE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to clear enable change on port %d\n", PortId);
            }
        }
        else if ((PortStatus.Change == USB_PORT_STATUS_RESET) &&
            (PortStatus.Status & (USB_PORT_STATUS_CONNECT | USB_PORT_STATUS_ENABLE)))
        {
            //
            // Clear Reset
            //
            Status = ClearPortFeature(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, C_PORT_RESET);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to clear reset change on port %d\n", PortId);
            }

            // FIXME: Double Check Port Status

            //
            // Create the device object only if the port manipulation was started by a device connect
            //
            if (HubDeviceExtension->PortStatusChange[PortId-1].Status)
            {
                HubDeviceExtension->PortStatusChange[PortId-1].Status = 0;
                Status = CreateUsbChildDeviceObject(DeviceObject, PortId, NULL);
            }
        }
    }

    ExFreePool(WorkItemData);

    //
    // Send another SCE Request
    //
    QueryStatusChangeEndpoint(DeviceObject);


}

NTSTATUS
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
    // Free the Irp
    //
    IoFreeIrp(Irp);

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
    ExInitializeWorkItem(&WorkItemData->WorkItem, (PWORKER_THREAD_ROUTINE)DeviceStatusChangeThread, (PVOID)WorkItemData);

    //
    // Queue the work item to handle initializing the device
    //
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
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // Initialize URB for Status Change Endpoint request
    //
    UsbBuildInterruptOrBulkTransferRequest(&HubDeviceExtension->PendingSCEUrb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           HubDeviceExtension->PipeHandle,
                                           HubDeviceExtension->PortStatusChange,
                                           NULL,
                                           sizeof(USHORT) * 2 * HubDeviceExtension->UsbExtHubInfo.NumberOfPorts,
                                           USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                           NULL);

    //
    // Set the device handle to null for roothub
    //
    HubDeviceExtension->PendingSCEUrb.UrbHeader.UsbdDeviceHandle = NULL;

    //
    // Allocate an Irp
    //
    HubDeviceExtension->PendingSCEIrp = IoAllocateIrp(HubDeviceExtension->RootHubPhysicalDeviceObject->StackSize,
                                        FALSE);

    if (!HubDeviceExtension->PendingSCEIrp)
    {
        DPRINT1("USBHUB: Failed to allocate IRP for SCE request!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the IRP
    //
    HubDeviceExtension->PendingSCEIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    HubDeviceExtension->PendingSCEIrp->IoStatus.Information = 0;
    HubDeviceExtension->PendingSCEIrp->Flags = 0;
    HubDeviceExtension->PendingSCEIrp->UserBuffer = NULL;

    //
    // Get the Next Stack Location and Initialize it
    //
    Stack = IoGetNextIrpStackLocation(HubDeviceExtension->PendingSCEIrp);
    Stack->DeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;
    Stack->Parameters.Others.Argument1 = &HubDeviceExtension->PendingSCEUrb;
    Stack->Parameters.Others.Argument2 = NULL;
    Stack->MajorFunction =  IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    //
    // Set the completion routine for when device is connected to root hub
    //
    IoSetCompletionRoutine(HubDeviceExtension->PendingSCEIrp,
                           (PIO_COMPLETION_ROUTINE) StatusChangeEndpointCompletion,
                           DeviceObject,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Send to RootHub
    //
    Status = IoCallDriver(HubDeviceExtension->RootHubPhysicalDeviceObject, HubDeviceExtension->PendingSCEIrp);

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
    PURB Urb;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;
    PMDL BufferMdl;

    //
    // Get the Hubs Device Extension
    //
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)ChildDeviceObject->DeviceExtension;
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) ChildDeviceExtension->ParentDeviceObject->DeviceExtension;

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
    // Create a MDL for buffer
    //
    BufferMdl = IoAllocateMdl(TransferBuffer,
                              TransferBufferLength,
                              FALSE,
                              FALSE,
                              NULL);

    //
    // Update Physical Pages
    //
    _SEH2_TRY
    {
        MmBuildMdlForNonPagedPool(BufferMdl);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("MmBuildMdlForNonPagedPool Failed!\n");
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    //
    // Create URB for getting device descriptor
    //
    UsbBuildGetDescriptorRequest(Urb,
                                 sizeof(Urb->UrbControlDescriptorRequest),
                                 DescriptorType,
                                 Index,
                                 LangId,
                                 NULL,
                                 BufferMdl,
                                 TransferBufferLength,
                                 NULL);

    //
    // Set the device handle
    //
    Urb->UrbHeader.UsbdDeviceHandle = (PVOID)ChildDeviceExtension->UsbDeviceHandle;

    //
    // Query the Root Hub
    //
    Status = SubmitRequestToRootHub(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    Urb,
                                    NULL);

    //
    // Free Mdl
    //
    IoFreeMdl(BufferMdl);

    return Status;
}

NTSTATUS
GetUsbStringDescriptor(
    IN PDEVICE_OBJECT ChildDeviceObject,
    IN UCHAR Index,
    IN USHORT LangId,
    OUT PVOID *TransferBuffer)
{
    NTSTATUS Status;
    PUSB_STRING_DESCRIPTOR StringDesc = NULL;
    ULONG SizeNeeded;

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
    //
    Status = GetUsbDeviceDescriptor(ChildDeviceObject,
                                    USB_STRING_DESCRIPTOR_TYPE,
                                    Index,
                                    0,
                                    StringDesc,
                                    sizeof(USB_STRING_DESCRIPTOR));
    SizeNeeded = StringDesc->bLength;

    //
    // Free String
    //
    ExFreePool(StringDesc);

    //
    // Recreate with appropriate size
    //
    StringDesc = ExAllocatePoolWithTag(NonPagedPool,
                                       SizeNeeded + sizeof(USB_STRING_DESCRIPTOR),
                                       USB_HUB_TAG);
    if (!StringDesc)
    {
        DPRINT1("Failed to allocate buffer for string!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the string
    //
    Status = GetUsbDeviceDescriptor(ChildDeviceObject,
                                    USB_STRING_DESCRIPTOR_TYPE,
                                    Index,
                                    0,
                                    StringDesc,
                                    SizeNeeded + sizeof(USB_STRING_DESCRIPTOR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get string from device\n");
        ExFreePool(StringDesc);
        return Status;
    }

    //
    // Allocate Buffer and Save it
    //
    *TransferBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                           SizeNeeded,
                                           USB_HUB_TAG);
    if (!*TransferBuffer)
    {
        DPRINT1("Failed to allocate buffer for string!\n");
        ExFreePool(StringDesc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(*TransferBuffer, SizeNeeded);

    //
    // Copy the string to destination
    //
    RtlCopyMemory(*TransferBuffer, StringDesc->bString, SizeNeeded);

    ExFreePool(StringDesc);

    return STATUS_SUCCESS;
}

NTSTATUS
CreateUsbChildDeviceObject(
    IN PDEVICE_OBJECT UsbHubDeviceObject,
    IN LONG PortId,
    OUT PDEVICE_OBJECT *UsbChildDeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT RootHubDeviceObject, NewChildDeviceObject;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PHUB_CHILDDEVICE_EXTENSION UsbChildExtension;
    PUSB_BUS_INTERFACE_HUB_V5 HubInterface;
    ULONG ChildDeviceCount;
    WCHAR CharDeviceName[64];
    ULONG UsbDeviceNumber = 0;
    UNICODE_STRING DeviceName;
    USB_DEVICE_DESCRIPTOR DeviceDesc;
    USB_CONFIGURATION_DESCRIPTOR ConfigDesc;
    ULONG DeviceDescSize, ConfigDescSize;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) UsbHubDeviceObject->DeviceExtension;
    HubInterface = &HubDeviceExtension->HubInterface;
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;

    //
    // Find an empty slot in the child device array 
    //
    for (ChildDeviceCount = 0; ChildDeviceCount < USB_MAXCHILDREN; ChildDeviceCount++)
    {
        if (HubDeviceExtension->ChildDeviceObject[ChildDeviceCount] == NULL)
        {
            break;
        }
    }

    //
    // Check if the limit has been reached for maximum usb devices
    //
    if (ChildDeviceCount == USB_MAXCHILDREN)
    {
        DPRINT1("Too many child devices!\n");
        return STATUS_UNSUCCESSFUL;
    }

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
            DPRINT1("IoCreateDevice failed with status %x\n", Status);
            return Status;
        }

        DPRINT1("USBHUB: Created Device %x\n", NewChildDeviceObject);
        break;
    }

    NewChildDeviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;

    //
    // Assign the device extensions
    //
    UsbChildExtension = (PHUB_CHILDDEVICE_EXTENSION)NewChildDeviceObject->DeviceExtension;
    RtlZeroMemory(UsbChildExtension, sizeof(HUB_CHILDDEVICE_EXTENSION));
    UsbChildExtension->ParentDeviceObject = UsbHubDeviceObject;

    //
    // Create the UsbDeviceObject
    //
    Status = HubInterface->CreateUsbDevice(RootHubDeviceObject,
                                           (PVOID)&UsbChildExtension->UsbDeviceHandle,
                                           HubDeviceExtension->RootHubHandle,
                                           0x503, //hack
                                           PortId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateUsbDevice failed with status %x\n", Status);
        goto Cleanup;
    }

    //
    // Initialize UsbDevice
    //
    Status = HubInterface->InitializeUsbDevice(RootHubDeviceObject, UsbChildExtension->UsbDeviceHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InitializeUsbDevice failed with status %x\n", Status);
        goto Cleanup;
    }

    DeviceDescSize = sizeof(USB_DEVICE_DESCRIPTOR);
    ConfigDescSize = sizeof(USB_CONFIGURATION_DESCRIPTOR);

    //
    // Get the descriptors
    //
    Status = HubInterface->GetUsbDescriptors(RootHubDeviceObject,
                                             UsbChildExtension->UsbDeviceHandle,
                                             (PUCHAR)&DeviceDesc,
                                             &DeviceDescSize,
                                             (PUCHAR)&ConfigDesc,
                                             &ConfigDescSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetUsbDescriptors failed with status %x\n", Status);
        goto Cleanup;
    }

    //
    // Allocate memory for DeviceId
    //
    UsbChildExtension->DeviceId = ExAllocatePoolWithTag(NonPagedPool, 32 * sizeof(WCHAR), USB_HUB_TAG);

    //
    // Construct DeviceId from vendor and product values
    //
    swprintf(UsbChildExtension->DeviceId, L"USB\\Vid_%04x&Pid_%04x", DeviceDesc.idVendor, DeviceDesc.idProduct);

    DPRINT1("Usb Device Id %S\n", UsbChildExtension->DeviceId);

    //
    // FIXME: Handle Lang ids, will use default for now
    //

    //
    // Get the product string
    //
    Status = GetUsbStringDescriptor(NewChildDeviceObject,
                                    DeviceDesc.iProduct,
                                    0,
                                    (PVOID*)&UsbChildExtension->TextDescription);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetUsbStringDescriptor failed with status %x\n", Status);
        goto Cleanup;
    }

    DPRINT1("Usb TextDescription %S\n", UsbChildExtension->TextDescription);
    
    Status = GetUsbStringDescriptor(NewChildDeviceObject,
                                    DeviceDesc.iSerialNumber,
                                    0,
                                    (PVOID*)&UsbChildExtension->InstanceId);

    DPRINT1("Usb InstanceId %S\n", UsbChildExtension->InstanceId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetUsbStringDescriptor failed with status %x\n", Status);
        goto Cleanup;
    }

    HubDeviceExtension->ChildDeviceObject[ChildDeviceCount] = NewChildDeviceObject;

    IoInvalidateDeviceRelations(RootHubDeviceObject, BusRelations);
    return STATUS_SUCCESS;

Cleanup:

    IoDeleteDevice(NewChildDeviceObject);
    return Status;
    
}

NTSTATUS
USBHUB_FdoQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_RELATIONS* pDeviceRelations)
{
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG i;
    ULONG Children = 0;
    ULONG NeededSize;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

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

    NeededSize = sizeof(DEVICE_RELATIONS);
    if (Children > 1)
        NeededSize += (Children - 1) * sizeof(PDEVICE_OBJECT);

    //
    // Allocate DeviceRelations
    //
    DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool,
                                                        NeededSize);

    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;
    DeviceRelations->Count = Children;
    Children = 0;

    //
    // Fill in return structure
    //
    for (i = 0; i < USB_MAXCHILDREN; i++)
    {
        if (HubDeviceExtension->ChildDeviceObject[i])
        {
            ObReferenceObject(HubDeviceExtension->ChildDeviceObject[i]);
            HubDeviceExtension->ChildDeviceObject[i]->Flags &= ~DO_DEVICE_INITIALIZING;
            DeviceRelations->Objects[Children++] = HubDeviceExtension->ChildDeviceObject[i];
        }
    }

    ASSERT(Children == DeviceRelations->Count);
    *pDeviceRelations = DeviceRelations;

    return STATUS_SUCCESS;
}

NTSTATUS
USBHUB_FdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    switch (Stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            PURB Urb;
            ULONG Result = 0;
            PUSB_INTERFACE_DESCRIPTOR Pid;
            ULONG PortId;
            USBD_INTERFACE_LIST_ENTRY InterfaceList[2] = {{NULL, NULL}, {NULL, NULL}};
            PURB ConfigUrb = NULL;

            DPRINT1("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");

            //
            // Allocated size including the sizeof USBD_INTERFACE_LIST_ENTRY
            //
            Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB) + sizeof(USBD_INTERFACE_LIST_ENTRY), USB_HUB_TAG);
            RtlZeroMemory(Urb, sizeof(URB) + sizeof(USBD_INTERFACE_LIST_ENTRY));

            //
            // Get the Root Hub Pdo
            //
            SubmitRequestToRootHub(HubDeviceExtension->LowerDeviceObject,
                                   IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO,
                                   &HubDeviceExtension->RootHubPhysicalDeviceObject,
                                   &HubDeviceExtension->RootHubFunctionalDeviceObject);

            ASSERT(HubDeviceExtension->RootHubPhysicalDeviceObject);
            ASSERT(HubDeviceExtension->RootHubFunctionalDeviceObject);
            DPRINT1("RootPdo %x, RootFdo %x\n",
                    HubDeviceExtension->RootHubPhysicalDeviceObject,
                    HubDeviceExtension->RootHubFunctionalDeviceObject);

            //
            // Send the StartDevice to RootHub
            //
            Status = ForwardIrpAndWait(HubDeviceExtension->RootHubPhysicalDeviceObject, Irp);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to start the RootHub PDO\n");
                ASSERT(FALSE);
            }

            //
            // Get the current number of hubs
            //
            Status = SubmitRequestToRootHub(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                            IOCTL_INTERNAL_USB_GET_HUB_COUNT,
                                            &HubDeviceExtension->NumberOfHubs, NULL);

            //
            // Get the Hub Interface
            //
            Status = QueryInterface(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                    USB_BUS_INTERFACE_HUB_GUID,
                                    sizeof(USB_BUS_INTERFACE_HUB_V5),
                                    5,
                                    (PVOID)&HubDeviceExtension->HubInterface);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get HUB_GUID interface with status 0x%08lx\n", Status);
                return STATUS_UNSUCCESSFUL;
            }

            //
            // Get the USBDI Interface
            //
            Status = QueryInterface(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                    USB_BUS_INTERFACE_USBDI_GUID,
                                    sizeof(USB_BUS_INTERFACE_USBDI_V2),
                                    2,
                                    (PVOID)&HubDeviceExtension->UsbDInterface);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get USBDI_GUID interface with status 0x%08lx\n", Status);
                return Status;
            }

            //
            // Get Root Hub Device Handle
            //
            Status = SubmitRequestToRootHub(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                            IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE,
                                            &HubDeviceExtension->RootHubHandle,
                                            NULL);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("GetRootHubDeviceHandle failed with status 0x%08lx\n", Status);
                return Status;
            }

            //
            // Get Hub Device Information
            //
            Status = HubDeviceExtension->HubInterface.QueryDeviceInformation(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                                                             HubDeviceExtension->RootHubHandle,
                                                                             &HubDeviceExtension->DeviceInformation,
                                                                             sizeof(USB_DEVICE_INFORMATION_0),
                                                                             &Result);

            DPRINT1("Status %x, Result 0x%08lx\n", Status, Result);
            DPRINT1("InformationLevel %x\n", HubDeviceExtension->DeviceInformation.InformationLevel);
            DPRINT1("ActualLength %x\n", HubDeviceExtension->DeviceInformation.ActualLength);
            DPRINT1("PortNumber %x\n", HubDeviceExtension->DeviceInformation.PortNumber);
            DPRINT1("DeviceDescriptor %x\n", HubDeviceExtension->DeviceInformation.DeviceDescriptor);
            DPRINT1("HubAddress %x\n", HubDeviceExtension->DeviceInformation.HubAddress);
            DPRINT1("NumberofPipes %x\n", HubDeviceExtension->DeviceInformation.NumberOfOpenPipes);

            //
            // Get Root Hubs Device Descriptor
            //
            UsbBuildGetDescriptorRequest(Urb,
                                         sizeof(Urb->UrbControlDescriptorRequest),
                                         USB_DEVICE_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         &HubDeviceExtension->HubDeviceDescriptor,
                                         NULL,
                                         sizeof(USB_DEVICE_DESCRIPTOR),
                                         NULL);

            Urb->UrbHeader.UsbdDeviceHandle = HubDeviceExtension->RootHubHandle;

            Status = SubmitRequestToRootHub(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                            IOCTL_INTERNAL_USB_SUBMIT_URB,
                                            Urb,
                                            NULL);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get HubDeviceDescriptor!\n");
            }

            DumpDeviceDescriptor(&HubDeviceExtension->HubDeviceDescriptor);

            //
            // Get Root Hubs Configuration Descriptor
            //
            UsbBuildGetDescriptorRequest(Urb,
                                         sizeof(Urb->UrbControlDescriptorRequest),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         &HubDeviceExtension->HubConfigDescriptor,
                                         NULL,
                                         sizeof(USB_CONFIGURATION_DESCRIPTOR) + sizeof(USB_INTERFACE_DESCRIPTOR) + sizeof(USB_ENDPOINT_DESCRIPTOR),
                                         NULL);

            Urb->UrbHeader.UsbdDeviceHandle = HubDeviceExtension->RootHubHandle;

            Status = SubmitRequestToRootHub(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                            IOCTL_INTERNAL_USB_SUBMIT_URB,
                                            Urb,
                                            NULL);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get RootHub Configuration with status %x\n", Status);
                ASSERT(FALSE);
            }
            ASSERT(HubDeviceExtension->HubConfigDescriptor.wTotalLength);

            DumpConfigurationDescriptor(&HubDeviceExtension->HubConfigDescriptor);

            Status = HubDeviceExtension->HubInterface.GetExtendedHubInformation(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                                                                HubDeviceExtension->RootHubPhysicalDeviceObject,
                                                                                &HubDeviceExtension->UsbExtHubInfo,
                                                                                sizeof(USB_EXTHUB_INFORMATION_0),
                                                                                &Result);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to extended hub information. Unable to determine the number of ports!\n");
                ASSERT(FALSE);
            }

            DPRINT1("HubDeviceExtension->UsbExtHubInfo.NumberOfPorts %x\n", HubDeviceExtension->UsbExtHubInfo.NumberOfPorts);

            //
            // Get the Hub Descriptor
            //
            UsbBuildVendorRequest(Urb,
                                  URB_FUNCTION_CLASS_DEVICE,
                                  sizeof(Urb->UrbControlVendorClassRequest),
                                  USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                  0,
                                  USB_DEVICE_CLASS_RESERVED,
                                  0,
                                  0,
                                  &HubDeviceExtension->HubDescriptor,
                                  NULL,
                                  sizeof(USB_HUB_DESCRIPTOR),
                                  NULL);

            Urb->UrbHeader.UsbdDeviceHandle = HubDeviceExtension->RootHubHandle;

            Status = SubmitRequestToRootHub(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                            IOCTL_INTERNAL_USB_SUBMIT_URB,
                                            Urb,
                                            NULL);

            //
            // FIXME: This is returning nothing.
            //
            DPRINT1("bDescriptorType %x\n", HubDeviceExtension->HubDescriptor.bDescriptorType);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get Hub Descriptor!\n");
                ExFreePool(Urb);
                return STATUS_UNSUCCESSFUL;
            }

            //
            // Allocate memory for PortStatusChange to hold 2 USHORTs for each port on hub
            //
            HubDeviceExtension->PortStatusChange = ExAllocatePoolWithTag(NonPagedPool,
                                                                         sizeof(ULONG) * HubDeviceExtension->UsbExtHubInfo.NumberOfPorts,
                                                                         USB_HUB_TAG);

            //
            // Initialize the Hub
            //
            Status = HubDeviceExtension->HubInterface.Initialize20Hub(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                                                      HubDeviceExtension->RootHubHandle, 1);
            DPRINT1("Status %x\n", Status);

            //
            // Get the first Configuration Descriptor
            //
            Pid = USBD_ParseConfigurationDescriptorEx(&HubDeviceExtension->HubConfigDescriptor,
                                                      &HubDeviceExtension->HubConfigDescriptor,
                                                     -1, -1, -1, -1, -1);

            ASSERT(Pid != NULL);

            InterfaceList[0].InterfaceDescriptor = Pid;
            ConfigUrb = USBD_CreateConfigurationRequestEx(&HubDeviceExtension->HubConfigDescriptor,
                                                          (PUSBD_INTERFACE_LIST_ENTRY)&InterfaceList);
            ASSERT(ConfigUrb != NULL);

            Status = SubmitRequestToRootHub(HubDeviceExtension->RootHubPhysicalDeviceObject,
                                            IOCTL_INTERNAL_USB_SUBMIT_URB,
                                            ConfigUrb,
                                            NULL);

            HubDeviceExtension->ConfigurationHandle = ConfigUrb->UrbSelectConfiguration.ConfigurationHandle;
            HubDeviceExtension->PipeHandle = ConfigUrb->UrbSelectConfiguration.Interface.Pipes[0].PipeHandle;
            DPRINT1("Configuration Handle %x\n", HubDeviceExtension->ConfigurationHandle);

            ExFreePool(ConfigUrb);

            //
            // Enable power on all ports
            //
            for (PortId = 1; PortId <= HubDeviceExtension->HubDescriptor.bNumberOfPorts; PortId++)
            {
                Status = SetPortFeature(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, PORT_POWER);
                if (!NT_SUCCESS(Status))
                    DPRINT1("Failed to power on port %d\n", PortId);
            }

            //
            // Send the first SCE Request
            //
            QueryStatusChangeEndpoint(DeviceObject);

            ExFreePool(Urb);
            break;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            switch (Stack->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                {
                    PDEVICE_RELATIONS DeviceRelations = NULL;
                    DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");

                    Status = USBHUB_FdoQueryBusRelations(DeviceObject, &DeviceRelations);

                    Information = (ULONG_PTR)DeviceRelations;
                    break;
                }
                case RemovalRelations:
                {
                    DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
                    return ForwardIrpAndForget(DeviceObject, Irp);
                }
                default:
                    DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
                            Stack->Parameters.QueryDeviceRelations.Type);
                    return ForwardIrpAndForget(DeviceObject, Irp);
            }
            break;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            DPRINT1("IRP_MN_QUERY_BUS_INFORMATION\n");
            break;
        }
        case IRP_MN_QUERY_ID:
        {
            DPRINT1("IRP_MN_QUERY_ID\n");
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            DPRINT1("IRP_MN_QUERY_CAPABILITIES\n");
            break;
        }
        default:
        {
            DPRINT1(" IRP_MJ_PNP / unknown minor function 0x%lx\n", Stack->MinorFunction);
            return ForwardIrpAndForget(DeviceObject, Irp);
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
USBHUB_FdoHandleDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DPRINT1("FdoHandleDeviceControl\n");
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

