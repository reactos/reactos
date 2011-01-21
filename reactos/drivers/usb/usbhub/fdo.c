/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         USB hub driver
 * FILE:            drivers/usb/cromwell/hub/fdo.c
 * PURPOSE:         IRP_MJ_PNP operations for FDOs
 *
 * PROGRAMMERS:     Herv� Poussineau (hpoussin@reactos.com)
 *                  Michael Martin (michael.martin@reactos.org)
 */

#define INITGUID
#include <stdio.h>
#define NDEBUG
#include "usbhub.h"
#include "usbdlib.h"

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

typedef struct _PORTSTATUSANDCHANGE
{
    USHORT Status;
    USHORT Change;
} PORTSTATUSANDCHANGE, *PPORTSTATUSANDCHANGE;

NTSTATUS
QueryRootHub(IN PDEVICE_OBJECT Pdo, IN ULONG IoControlCode, OUT PVOID OutParameter1, OUT PVOID OutParameter2);
NTSTATUS
WaitForUsbDeviceArrivalNotification(PDEVICE_OBJECT DeviceObject);
NTSTATUS
SubmitUrbToRootHub(IN PDEVICE_OBJECT Pdo, IN ULONG IoControlCode, IN PURB Urb);

VOID DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    DPRINT1("Dumping Device Descriptor %x\n", DeviceDescriptor);
    DPRINT1("bLength %x\n", DeviceDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", DeviceDescriptor->bDescriptorType);
    DPRINT1("bcdUSB %x\n", DeviceDescriptor->bcdUSB);
    DPRINT1("bDeviceClass %x\n", DeviceDescriptor->bDeviceClass);
    DPRINT1("bDeviceSubClass %x\n", DeviceDescriptor->bDeviceSubClass);
    DPRINT1("bDeviceProtocol %x\n", DeviceDescriptor->bDeviceProtocol);
    DPRINT1("bMaxPacketSize0 %x\n", DeviceDescriptor->bMaxPacketSize0);
    DPRINT1("idVendor %x\n", DeviceDescriptor->idVendor);
    DPRINT1("idProduct %x\n", DeviceDescriptor->idProduct);
    DPRINT1("bcdDevice %x\n", DeviceDescriptor->bcdDevice);
    DPRINT1("iManufacturer %x\n", DeviceDescriptor->iManufacturer);
    DPRINT1("iProduct %x\n", DeviceDescriptor->iProduct);
    DPRINT1("iSerialNumber %x\n", DeviceDescriptor->iSerialNumber);
    DPRINT1("bNumConfigurations %x\n", DeviceDescriptor->bNumConfigurations);
}

VOID DumpFullConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    LONG i, j;

    DPRINT1("Duming ConfigurationDescriptor %x\n", ConfigurationDescriptor);
    DPRINT1("bLength %x\n", ConfigurationDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", ConfigurationDescriptor->bDescriptorType);
    DPRINT1("wTotalLength %x\n", ConfigurationDescriptor->wTotalLength);
    DPRINT1("bNumInterfaces %x\n", ConfigurationDescriptor->bNumInterfaces);
    DPRINT1("bConfigurationValue %x\n", ConfigurationDescriptor->bConfigurationValue);
    DPRINT1("iConfiguration %x\n", ConfigurationDescriptor->iConfiguration);
    DPRINT1("bmAttributes %x\n", ConfigurationDescriptor->bmAttributes);
    DPRINT1("MaxPower %x\n", ConfigurationDescriptor->MaxPower);

    InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) ((ULONG_PTR)ConfigurationDescriptor + sizeof(USB_CONFIGURATION_DESCRIPTOR));

    for (i=0; i < ConfigurationDescriptor->bNumInterfaces; i++)
    {
        DPRINT1("- Dumping InterfaceDescriptor %x\n", InterfaceDescriptor);
        DPRINT1("  bLength %x\n", InterfaceDescriptor->bLength);
        DPRINT1("  bDescriptorType %x\n", InterfaceDescriptor->bDescriptorType);
        DPRINT1("  bInterfaceNumber %x\n", InterfaceDescriptor->bInterfaceNumber);
        DPRINT1("  bAlternateSetting %x\n", InterfaceDescriptor->bAlternateSetting);
        DPRINT1("  bNumEndpoints %x\n", InterfaceDescriptor->bNumEndpoints);
        DPRINT1("  bInterfaceClass %x\n", InterfaceDescriptor->bInterfaceClass);
        DPRINT1("  bInterfaceSubClass %x\n", InterfaceDescriptor->bInterfaceSubClass);
        DPRINT1("  bInterfaceProtocol %x\n", InterfaceDescriptor->bInterfaceProtocol);
        DPRINT1("  iInterface %x\n", InterfaceDescriptor->iInterface);

        EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) ((ULONG_PTR)InterfaceDescriptor + sizeof(USB_INTERFACE_DESCRIPTOR));

        for (j=0; j < InterfaceDescriptor->bNumEndpoints; j++)
        {
            DPRINT1("   bLength %x\n", EndpointDescriptor->bLength);
            DPRINT1("   bDescriptorType %x\n", EndpointDescriptor->bDescriptorType);
            DPRINT1("   bEndpointAddress %x\n", EndpointDescriptor->bEndpointAddress);
            DPRINT1("   bmAttributes %x\n", EndpointDescriptor->bmAttributes);
            DPRINT1("   wMaxPacketSize %x\n", EndpointDescriptor->wMaxPacketSize);
            DPRINT1("   bInterval %x\n", EndpointDescriptor->bInterval);
            EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) ((ULONG_PTR)EndpointDescriptor + sizeof(USB_ENDPOINT_DESCRIPTOR));
        }
        InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)(ULONG_PTR)EndpointDescriptor;
    }

}

VOID DumpInterfaceInfo(PUSBD_INTERFACE_INFORMATION InterfaceInformation)
{
    PUSBD_PIPE_INFORMATION PipeInformation;
    ULONG i;

    DPRINT1("IntefaceLenth %x\n",InterfaceInformation->Length);
    DPRINT1("InterfaceNumber %x\n",InterfaceInformation->InterfaceNumber);
    DPRINT1("AlternateSetting %x\n",InterfaceInformation->AlternateSetting);
    DPRINT1("Class %x\n",InterfaceInformation->Class);
    DPRINT1("SubClass %x\n",InterfaceInformation->SubClass);
    DPRINT1("Protocol %x\n",InterfaceInformation->Protocol);
    DPRINT1("Reserved %x\n",InterfaceInformation->Reserved);
    DPRINT1("InterfaceHandle %x\n",InterfaceInformation->InterfaceHandle);
    DPRINT1("NumberOfPipes %x\n", InterfaceInformation->NumberOfPipes);

    PipeInformation = &InterfaceInformation->Pipes[0];

    for (i = 0; i < InterfaceInformation->NumberOfPipes; i++)
    {

        DPRINT1("MaximumPacketSize %x\n", PipeInformation->MaximumPacketSize);
        DPRINT1("EndpointAddress %x\n", PipeInformation->EndpointAddress);
        DPRINT1("Interval %x\n", PipeInformation->Interval);
        DPRINT1("PipeType %x\n", PipeInformation->PipeType);
        DPRINT1("PipeHandle %x\n", PipeInformation->PipeHandle);
        DPRINT1("PipeFlags %x\n", PipeInformation->PipeFlags);
        DPRINT1("MaximumTransferSize %x\n", PipeInformation->MaximumTransferSize);
        PipeInformation = (PUSBD_PIPE_INFORMATION)((ULONG_PTR)PipeInformation + sizeof(USBD_PIPE_INFORMATION));
    }
}


VOID NTAPI
WorkerThread(IN PVOID Context)
{
    PHUB_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT DeviceObject, Pdo;
    PHUB_CHILDDEVICE_EXTENSION PdoExtension;
    PURB Urb = NULL;
    PORTSTATUSANDCHANGE PortStatusAndChange;
    int PortLoop, DeviceCount;
    NTSTATUS Status;
    USB_DEVICE_DESCRIPTOR DevDesc;
    USB_CONFIGURATION_DESCRIPTOR ConfigDesc;
    ULONG DevDescSize, ConfigDescSize;
    PUSB_STRING_DESCRIPTOR StringDesc;
    USB_STRING_DESCRIPTOR LanguageIdDescriptor;
    PWORKITEMDATA WorkItemData = (PWORKITEMDATA)Context;

    DeviceObject = (PDEVICE_OBJECT)WorkItemData->Context;

    DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Determine where in the children array to store this device info */
    for (DeviceCount = 0; DeviceCount < USB_MAXCHILDREN; DeviceCount++)
    {
        if (DeviceExtension->UsbChildren[DeviceCount] == NULL)
        {
            break;
        }
    }

    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);
    if (!Urb)
    {
        DPRINT1("Failed to allocate memory for URB!\n");
        ASSERT(FALSE);
    }

    RtlZeroMemory(Urb, sizeof(URB));

    for (PortLoop = 0; PortLoop < DeviceExtension->UsbExtHubInfo.NumberOfPorts; PortLoop++)
    {
        /* Get the port status */
        UsbBuildVendorRequest(Urb,
                            URB_FUNCTION_CLASS_OTHER,
                            sizeof(Urb->UrbControlVendorClassRequest),
                            USBD_TRANSFER_DIRECTION_OUT,
                            0,
                            USB_REQUEST_GET_STATUS,
                            0,
                            PortLoop + 1,
                            &PortStatusAndChange,
                            0,
                            sizeof(PORTSTATUSANDCHANGE),
                            0);

        Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get PortStatus!\n");
            goto CleanUp;
        }

        DPRINT("Notification Port %x:\n", PortLoop + 1);
        DPRINT("Status %x\n", PortStatusAndChange.Status);
        DPRINT("Change %x\n", PortStatusAndChange.Change);

        if (PortStatusAndChange.Change == USB_PORT_STATUS_RESET)
        {
            /* Clear the Reset */
            UsbBuildVendorRequest(Urb,
                                  URB_FUNCTION_CLASS_OTHER,
                                  sizeof(Urb->UrbControlVendorClassRequest),
                                  USBD_TRANSFER_DIRECTION_IN,
                                  0,
                                  USB_REQUEST_CLEAR_FEATURE,
                                  C_PORT_RESET,
                                  PortLoop + 1,
                                  &PortStatusAndChange,
                                  0,
                                  sizeof(PORTSTATUSANDCHANGE),
                                  0);

            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to Clear the Port Reset with Status %x!\n", Status);
                goto CleanUp;
            }

            UsbBuildVendorRequest(Urb, URB_FUNCTION_CLASS_OTHER,
                                  sizeof(Urb->UrbControlVendorClassRequest),
                                  USBD_TRANSFER_DIRECTION_OUT,
                                  0,
                                  USB_REQUEST_GET_STATUS,
                                  0,
                                  PortLoop + 1,
                                  &PortStatusAndChange,
                                  0,
                                  sizeof(PORTSTATUSANDCHANGE),
                                  0);

            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

            DPRINT("Status %x\n", PortStatusAndChange.Status);
            DPRINT("Change %x\n", PortStatusAndChange.Change);

            /* Create the UsbDevice */
            Status = DeviceExtension->HubInterface.CreateUsbDevice(DeviceExtension->RootHubPdo,
                                                                   (PVOID)&DeviceExtension->UsbChildren[DeviceCount],
                                                                   DeviceExtension->RootHubUsbDevice,
                                                                   PortStatusAndChange.Status,
                                                                   PortLoop + 1);
            DPRINT1("CreateUsbDevice Status %x\n", Status);

            Status = DeviceExtension->HubInterface.InitializeUsbDevice(DeviceExtension->RootHubPdo, DeviceExtension->UsbChildren[DeviceCount]);
            DPRINT1("InitializeUsbDevice Status %x\n", Status);

            DevDescSize = sizeof(USB_DEVICE_DESCRIPTOR);
            ConfigDescSize = sizeof(USB_CONFIGURATION_DESCRIPTOR);
            Status = DeviceExtension->HubInterface.GetUsbDescriptors(DeviceExtension->RootHubPdo,
                                                                     DeviceExtension->UsbChildren[DeviceCount],
                                                                     (PUCHAR)&DevDesc,
                                                                     &DevDescSize,
                                                                     (PUCHAR)&ConfigDesc,
                                                                     &ConfigDescSize);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to Get Usb Deccriptors %x!\n", Status);
            }

            DumpDeviceDescriptor(&DevDesc);

            Status = IoCreateDevice(DeviceObject->DriverObject,
                                    sizeof(HUB_CHILDDEVICE_EXTENSION),
                                    NULL,
                                    FILE_DEVICE_CONTROLLER,
                                    FILE_AUTOGENERATED_DEVICE_NAME,
                                    FALSE,
                                    &DeviceExtension->Children[DeviceCount]);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("UsbHub; IoCreateDevice failed with status %x\n",Status);
                goto CleanUp;
            }

            Pdo = DeviceExtension->Children[DeviceCount];
            DPRINT1("Created New Device with %x\n", Pdo);
            Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

            PdoExtension = Pdo->DeviceExtension;

            RtlZeroMemory(PdoExtension, sizeof(HUB_CHILDDEVICE_EXTENSION));

            PdoExtension->DeviceId = ExAllocatePoolWithTag(NonPagedPool, 32 * sizeof(WCHAR), USB_HUB_TAG);
            RtlZeroMemory(PdoExtension->DeviceId, 32 * sizeof(WCHAR));
            swprintf(PdoExtension->DeviceId, L"USB\\Vid_%04x&Pid_%04x", DevDesc.idVendor, DevDesc.idProduct);


            /* Get the LANGids */
            RtlZeroMemory(&LanguageIdDescriptor, sizeof(USB_STRING_DESCRIPTOR));
            UsbBuildGetDescriptorRequest(Urb,
                                         sizeof(Urb->UrbControlDescriptorRequest),
                                         USB_STRING_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         &LanguageIdDescriptor,
                                         NULL,
                                         sizeof(USB_STRING_DESCRIPTOR),
                                         NULL);

            Urb->UrbHeader.UsbdDeviceHandle = DeviceExtension->UsbChildren[DeviceCount];
            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

            /* Get the length of the SerialNumber */
            StringDesc = ExAllocatePoolWithTag(PagedPool, 64, USB_HUB_TAG);
            RtlZeroMemory(StringDesc, 64);
            StringDesc->bLength = 0;
            StringDesc->bDescriptorType = 0;

            UsbBuildGetDescriptorRequest(Urb,
                                         sizeof(Urb->UrbControlDescriptorRequest),
                                         USB_STRING_DESCRIPTOR_TYPE,
                                         DevDesc.iSerialNumber,
                                         LanguageIdDescriptor.bString[0],
                                         StringDesc,
                                         NULL,
                                         64,
                                         NULL);

            Urb->UrbHeader.UsbdDeviceHandle = DeviceExtension->UsbChildren[DeviceCount];

            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

            PdoExtension->InstanceId = ExAllocatePoolWithTag(NonPagedPool, (StringDesc->bLength + 1) * sizeof(WCHAR), USB_HUB_TAG);
            DPRINT1("PdoExtension->InstanceId %x\n",PdoExtension->InstanceId);

            RtlZeroMemory(PdoExtension->InstanceId, (StringDesc->bLength + 1) * sizeof(WCHAR));
            RtlCopyMemory(PdoExtension->InstanceId, &StringDesc->bString[0], StringDesc->bLength);
            DPRINT1("------>SerialNumber %S\n", PdoExtension->InstanceId);



            RtlZeroMemory(StringDesc, 64);
            StringDesc->bLength = 0;
            StringDesc->bDescriptorType = 0;

            UsbBuildGetDescriptorRequest(Urb,
                                         sizeof(Urb->UrbControlDescriptorRequest),
                                         USB_STRING_DESCRIPTOR_TYPE,
                                         DevDesc.iProduct,
                                         LanguageIdDescriptor.bString[0],
                                         StringDesc,
                                         NULL,
                                         64,
                                         NULL);

            Urb->UrbHeader.UsbdDeviceHandle = DeviceExtension->UsbChildren[DeviceCount];

            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

            PdoExtension->TextDescription = ExAllocatePoolWithTag(NonPagedPool, (StringDesc->bLength + 1) * sizeof(WCHAR), USB_HUB_TAG);

            RtlZeroMemory(PdoExtension->TextDescription, (StringDesc->bLength + 1) * sizeof(WCHAR));
            RtlCopyMemory(PdoExtension->TextDescription, &StringDesc->bString[0], StringDesc->bLength);
            ExFreePool(StringDesc);
            DPRINT1("------>TextDescription %S\n", PdoExtension->TextDescription);

            PdoExtension->IsFDO = FALSE;
            PdoExtension->Parent = DeviceObject;
            Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

            ExFreePool(WorkItemData);
            ExFreePool(Urb);

            IoInvalidateDeviceRelations(DeviceExtension->RootHubPdo, BusRelations);
            return;
        }

        /* Is a device connected to this port */
        if (PortStatusAndChange.Change == USB_PORT_STATUS_CONNECT)
        {
            /* Clear the Connect from ProtChange */
            UsbBuildVendorRequest(Urb,
                                URB_FUNCTION_CLASS_OTHER,
                                sizeof(Urb->UrbControlVendorClassRequest),
                                USBD_TRANSFER_DIRECTION_IN,
                                0,
                                USB_REQUEST_CLEAR_FEATURE,
                                C_PORT_CONNECTION,
                                PortLoop + 1,
                                &PortStatusAndChange,
                                0,
                                sizeof(PORTSTATUSANDCHANGE),
                                0);

            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to Clear the Port Connect!\n");
                goto CleanUp;
            }

            /* Send the miniport controller a SCE request so when the port resets we can be informed */
            WaitForUsbDeviceArrivalNotification(DeviceObject);

            UsbBuildVendorRequest(Urb,
                                URB_FUNCTION_CLASS_OTHER,
                                sizeof(Urb->UrbControlVendorClassRequest),
                                USBD_TRANSFER_DIRECTION_IN,
                                0,
                                USB_REQUEST_SET_FEATURE,
                                PORT_RESET,
                                PortLoop + 1,
                                &PortStatusAndChange,
                                0,
                                sizeof(PORTSTATUSANDCHANGE),
                                0);

            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to Reset the port!\n");
                goto CleanUp;
            }
            /* At this point the miniport will complete another SCE to inform of Reset completed */
        }
    }

CleanUp:
    ExFreePool(WorkItemData);
    ExFreePool(Urb);
}

NTSTATUS
DeviceArrivalCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    PHUB_DEVICE_EXTENSION DeviceExtension;
    LONG i;
    PWORKITEMDATA WorkItemData;

    DeviceExtension = (PHUB_DEVICE_EXTENSION)((PDEVICE_OBJECT)Context)->DeviceExtension;

    for (i=0; i < DeviceExtension->UsbExtHubInfo.NumberOfPorts; i++)
        DPRINT1("Port %x DeviceExtension->PortStatus %x\n",i+1, DeviceExtension->PortStatus[i]);

    IoFreeIrp(Irp);

    WorkItemData = ExAllocatePool(NonPagedPool, sizeof(WORKITEMDATA));
    if (!WorkItemData)
    {
        DPRINT1("Failed to allocate memory\n");
        return STATUS_NO_MEMORY;
    }


    RtlZeroMemory(WorkItemData, sizeof(WORKITEMDATA));
    WorkItemData->Context = Context;

    ExInitializeWorkItem(&WorkItemData->WorkItem, (PWORKER_THREAD_ROUTINE)WorkerThread, (PVOID)WorkItemData);
    ExQueueWorkItem(&WorkItemData->WorkItem, DelayedWorkQueue);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
WaitForUsbDeviceArrivalNotification(PDEVICE_OBJECT DeviceObject)
{
    PURB Urb;
    PIRP Irp;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;
    PHUB_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    Urb = &DeviceExtension->Urb;

    RtlZeroMemory(Urb, sizeof(URB));

    /* Send URB to the miniports Status Change Endpoint SCE */
    UsbBuildInterruptOrBulkTransferRequest(Urb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           DeviceExtension->PipeHandle,
                                           &DeviceExtension->PortStatus,
                                           NULL,
                                           sizeof(ULONG) * 2,
                                           USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                           NULL);

    Urb->UrbHeader.UsbdDeviceHandle = DeviceExtension->RootHubUsbDevice;

    Irp = IoAllocateIrp(DeviceExtension->RootHubPdo->StackSize, FALSE);

    if (Irp == NULL)
    {
        DPRINT("Usbhub: IoBuildDeviceIoControlRequest() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    Irp->Flags = 0;
    Irp->UserBuffer = NULL;

    Stack = IoGetCurrentIrpStackLocation(Irp);
    Stack->DeviceObject = DeviceExtension->RootHubPdo;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->DeviceObject = DeviceExtension->RootHubPdo;
    Stack->Parameters.Others.Argument1 = Urb;
    Stack->Parameters.Others.Argument2 = NULL;
    Stack->MajorFunction =  IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    //IoSetCompletionRoutineEx(DeviceExtension->RootHubPdo, Irp, (PIO_COMPLETION_ROUTINE)DeviceArrivalCompletion, DeviceObject, TRUE, TRUE, TRUE);
    IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE)DeviceArrivalCompletion, DeviceObject, TRUE, TRUE, TRUE);

    Status = IoCallDriver(DeviceExtension->RootHubPdo, Irp);
    DPRINT1("SCE request status %x\n", Status);

    return STATUS_PENDING;
}

NTSTATUS
SubmitUrbToRootHub(IN PDEVICE_OBJECT Pdo, IN ULONG IoControlCode, IN PURB Urb)
{
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;

    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        Pdo,
                                        NULL, 0,
                                        NULL, 0,
                                        TRUE,
                                        NULL,
                                        &IoStatus);

    if (Irp == NULL)
    {
        DPRINT("Usbhub: IoBuildDeviceIoControlRequest() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize the status block before sending the IRP */
    IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoStatus.Information = 0;

    Stack = IoGetNextIrpStackLocation(Irp);

    Stack->Parameters.Others.Argument1 = Urb;
    Stack->Parameters.Others.Argument2 = NULL;

    Status = IoCallDriver(Pdo, Irp);

    return Status;
}

NTSTATUS
QueryRootHub(IN PDEVICE_OBJECT Pdo, IN ULONG IoControlCode, OUT PVOID OutParameter1, OUT PVOID OutParameter2)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        Pdo,
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

    /* Initialize the status block before sending the IRP */
    IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoStatus.Information = 0;

    Stack = IoGetNextIrpStackLocation(Irp);

    Stack->Parameters.Others.Argument1 = OutParameter1;
    Stack->Parameters.Others.Argument2 = OutParameter2;

    Status = IoCallDriver(Pdo, Irp);

    if (Status == STATUS_PENDING)
    {
        DPRINT1("Usbhub: Operation pending\n");
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    return Status;
}

NTSTATUS QueryInterface(IN PDEVICE_OBJECT Pdo, IN CONST GUID InterfaceType, IN LONG Size, IN LONG Version, OUT PVOID Interface)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       Pdo,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.InterfaceType= &InterfaceType;//USB_BUS_INTERFACE_HUB_GUID;
    Stack->Parameters.QueryInterface.Size = Size;
    Stack->Parameters.QueryInterface.Version = Version;
    Stack->Parameters.QueryInterface.Interface = Interface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(Pdo, Irp);

    if (Status == STATUS_PENDING)
    {
        DPRINT("Usbhub: Operation pending\n");
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    return Status;
}

static VOID
UsbhubGetUserBuffers(IN PIRP Irp, IN ULONG IoControlCode, OUT PVOID* BufferIn, OUT PVOID* BufferOut)
{
    ASSERT(Irp);
    ASSERT(BufferIn);
    ASSERT(BufferOut);

    switch (IO_METHOD_FROM_CTL_CODE(IoControlCode))
    {
        case METHOD_BUFFERED:
            *BufferIn = *BufferOut = Irp->AssociatedIrp.SystemBuffer;
            break;
        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:
            *BufferIn = Irp->AssociatedIrp.SystemBuffer;
            *BufferOut = MmGetSystemAddressForMdl(Irp->MdlAddress);
            break;
        case METHOD_NEITHER:
            *BufferIn = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.Type3InputBuffer;
            *BufferOut = Irp->UserBuffer;
            break;
        default:
            /* Should never happen */
            *BufferIn = NULL;
            *BufferOut = NULL;
            break;
    }
}

NTSTATUS
UsbhubFdoQueryBusRelations(IN PDEVICE_OBJECT DeviceObject, OUT PDEVICE_RELATIONS* pDeviceRelations)
{
    PHUB_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG i;
    ULONG Children = 0;
    ULONG NeededSize;

    DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DPRINT1("USBHUB: Query Bus Relations\n");

    /* Create PDOs that are missing */
    for (i = 0; i < USB_MAXCHILDREN; i++)
    {

        if (DeviceExtension->UsbChildren[i] == NULL)
        {
            continue;
        }
        Children++;
    }

    /* Fill returned structure */
    NeededSize = sizeof(DEVICE_RELATIONS);
    if (Children > 1)
        NeededSize += (Children - 1) * sizeof(PDEVICE_OBJECT);

    DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool,
                                                        NeededSize);

    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;
    DeviceRelations->Count = Children;
    Children = 0;

    for (i = 0; i < USB_MAXCHILDREN; i++)
    {
        if (DeviceExtension->Children[i])
        {
            ObReferenceObject(DeviceExtension->Children[i]);
            DeviceRelations->Objects[Children++] = DeviceExtension->Children[i];
        }
    }

    ASSERT(Children == DeviceRelations->Count);
    *pDeviceRelations = DeviceRelations;

    WaitForUsbDeviceArrivalNotification(DeviceObject);

    return STATUS_SUCCESS;
}

VOID CallBackRoutine(IN PVOID Argument1)
{
    DPRINT1("RH_INIT_CALLBACK %x\n", Argument1);
    ASSERT(FALSE);
}

NTSTATUS NTAPI
UsbhubPnpFdo(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG MinorFunction;
    ULONG_PTR Information = 0;
    PHUB_DEVICE_EXTENSION DeviceExtension;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    MinorFunction = IrpSp->MinorFunction;

    DeviceExtension = (PHUB_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE: /* 0x0 */
        {
            PURB Urb;
            ULONG Result = 0;
            PUSB_INTERFACE_DESCRIPTOR Pid;
            /* Theres only one descriptor on hub */
            USBD_INTERFACE_LIST_ENTRY InterfaceList[2] = {{NULL, NULL}, {NULL, NULL}};
            PURB ConfigUrb = NULL;

            /* We differ from windows on hubpdo because we dont have usbport.sys which manages all usb device objects */
            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_START_DEVICE\n");

            /* Allocating size including the sizeof USBD_INTERFACE_LIST_ENTRY */
            Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB) + sizeof(USBD_INTERFACE_LIST_ENTRY), USB_HUB_TAG);
            RtlZeroMemory(Urb, sizeof(URB) + sizeof(USBD_INTERFACE_LIST_ENTRY));

            /* Get the hubs PDO */
            QueryRootHub(DeviceExtension->LowerDevice, IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO, &DeviceExtension->RootHubPdo, &DeviceExtension->RootHubFdo);
            ASSERT(DeviceExtension->RootHubPdo);
            ASSERT(DeviceExtension->RootHubFdo);
            DPRINT1("RootPdo %x, RootFdo %x\n", DeviceExtension->RootHubPdo, DeviceExtension->RootHubFdo);

            /* Send the START_DEVICE irp down to the PDO of RootHub */
            Status = ForwardIrpAndWait(DeviceExtension->RootHubPdo, Irp);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to start the RootHub PDO\n");
                ASSERT(FALSE);
            }

            /* Get the current number of hubs */
            QueryRootHub(DeviceExtension->RootHubPdo,IOCTL_INTERNAL_USB_GET_HUB_COUNT, &DeviceExtension->HubCount, NULL);

            /* Get the Direct Call Interfaces */
            Status = QueryInterface(DeviceExtension->RootHubPdo, 
                                    USB_BUS_INTERFACE_HUB_GUID,
                                    sizeof(USB_BUS_INTERFACE_HUB_V5),
                                    5,
                                    (PVOID)&DeviceExtension->HubInterface);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("UsbhubM Failed to get HUB_GUID interface with status 0x%08lx\n", Status);
                return STATUS_UNSUCCESSFUL;
            }

            Status = QueryInterface(DeviceExtension->RootHubPdo,
                                    USB_BUS_INTERFACE_USBDI_GUID,
                                    sizeof(USB_BUS_INTERFACE_USBDI_V2),
                                    2,
                                    (PVOID)&DeviceExtension->UsbDInterface);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("UsbhubM Failed to get USBDI_GUID interface with status 0x%08lx\n", Status);
                return STATUS_UNSUCCESSFUL;
            }

            /* Get roothub device handle */
            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE, &DeviceExtension->RootHubUsbDevice, NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Usbhub: GetRootHubDeviceHandle failed with status 0x%08lx\n", Status);
                return Status;
            }

            Status = DeviceExtension->HubInterface.QueryDeviceInformation(DeviceExtension->RootHubPdo,
                                                                 DeviceExtension->RootHubUsbDevice,
                                                                 &DeviceExtension->DeviceInformation,
                                                                 sizeof(USB_DEVICE_INFORMATION_0),
                                                                 &Result);


            DPRINT("Status %x, Result %x\n", Status, Result);
            DPRINT("InformationLevel %x\n", DeviceExtension->DeviceInformation.InformationLevel);
            DPRINT("ActualLength %x\n", DeviceExtension->DeviceInformation.ActualLength);
            DPRINT("PortNumber %x\n", DeviceExtension->DeviceInformation.PortNumber);
            DPRINT("DeviceDescriptor %x\n", DeviceExtension->DeviceInformation.DeviceDescriptor);
            DPRINT("HubAddress %x\n", DeviceExtension->DeviceInformation.HubAddress);
            DPRINT("NumberofPipes %x\n", DeviceExtension->DeviceInformation.NumberOfOpenPipes);

            /* Get roothubs device descriptor */
            UsbBuildGetDescriptorRequest(Urb,
                                         sizeof(Urb->UrbControlDescriptorRequest),
                                         USB_DEVICE_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         &DeviceExtension->HubDeviceDescriptor,
                                         NULL,
                                         sizeof(USB_DEVICE_DESCRIPTOR),
                                         NULL);

            Urb->UrbHeader.UsbdDeviceHandle = DeviceExtension->RootHubUsbDevice;

            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Usbhub: Failed to get HubDeviceDescriptor!\n");
            }

            DumpDeviceDescriptor(&DeviceExtension->HubDeviceDescriptor);

            /* Get roothubs configuration descriptor */
            UsbBuildGetDescriptorRequest(Urb,
                                         sizeof(Urb->UrbControlDescriptorRequest),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         &DeviceExtension->HubConfigDescriptor,
                                         NULL,
                                         sizeof(USB_CONFIGURATION_DESCRIPTOR) + sizeof(USB_INTERFACE_DESCRIPTOR) + sizeof(USB_ENDPOINT_DESCRIPTOR),
                                         NULL);
            Urb->UrbHeader.UsbdDeviceHandle = DeviceExtension->RootHubUsbDevice;

            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Usbhub: Failed to get RootHub Configuration with status %x\n", Status);
                ASSERT(FALSE);
            }
            ASSERT(DeviceExtension->HubConfigDescriptor.wTotalLength);

            DumpFullConfigurationDescriptor(&DeviceExtension->HubConfigDescriptor);
            //DPRINT1("DeviceExtension->HubConfigDescriptor.wTotalLength %x\n", DeviceExtension->HubConfigDescriptor.wTotalLength);

            Status = DeviceExtension->HubInterface.GetExtendedHubInformation(DeviceExtension->RootHubPdo,
                                                                    DeviceExtension->RootHubPdo,
                                                                    &DeviceExtension->UsbExtHubInfo,
                                                                    sizeof(USB_EXTHUB_INFORMATION_0),
                                                                    &Result);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Usbhub: Failed to extended hub information. Unable to determine the number of ports!\n");
                ASSERT(FALSE);
            }

            DPRINT1("DeviceExtension->UsbExtHubInfo.NumberOfPorts %x\n", DeviceExtension->UsbExtHubInfo.NumberOfPorts);

            UsbBuildVendorRequest(Urb,
                                  URB_FUNCTION_CLASS_DEVICE,
                                  sizeof(Urb->UrbControlVendorClassRequest),
                                  USBD_TRANSFER_DIRECTION_IN,
                                  0,
                                  USB_DEVICE_CLASS_RESERVED,
                                  0,
                                  0,
                                  &DeviceExtension->HubDescriptor,
                                  NULL,
                                  sizeof(USB_HUB_DESCRIPTOR),
                                  NULL);

            Urb->UrbHeader.UsbdDeviceHandle = DeviceExtension->RootHubUsbDevice;

            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

            DPRINT1("bDescriptorType %x\n", DeviceExtension->HubDescriptor.bDescriptorType);

            /* Select the configuration */

            /* Get the first one */
            Pid = USBD_ParseConfigurationDescriptorEx(&DeviceExtension->HubConfigDescriptor,
                                                      &DeviceExtension->HubConfigDescriptor,
                                                      -1, -1, -1, -1, -1);
            ASSERT(Pid != NULL);
            InterfaceList[0].InterfaceDescriptor = Pid;
            ConfigUrb = USBD_CreateConfigurationRequestEx(&DeviceExtension->HubConfigDescriptor, (PUSBD_INTERFACE_LIST_ENTRY)&InterfaceList);
            ASSERT(ConfigUrb != NULL);
            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, ConfigUrb, NULL);

            DeviceExtension->ConfigurationHandle = ConfigUrb->UrbSelectConfiguration.ConfigurationHandle;
            DeviceExtension->PipeHandle = ConfigUrb->UrbSelectConfiguration.Interface.Pipes[0].PipeHandle;
            DPRINT1("Configuration Handle %x\n", DeviceExtension->ConfigurationHandle);

            ExFreePool(ConfigUrb);

            Status = DeviceExtension->HubInterface.Initialize20Hub(DeviceExtension->RootHubPdo, DeviceExtension->RootHubUsbDevice, 1);
            DPRINT1("Status %x\n", Status);

            {
                int PortLoop;
                USHORT PortStatusAndChange[2];

                for (PortLoop=0; PortLoop< DeviceExtension->UsbExtHubInfo.NumberOfPorts; PortLoop++)
                {
                    DPRINT1("Port %x\n", PortLoop);
                    UsbBuildVendorRequest(Urb,
                                          URB_FUNCTION_CLASS_OTHER,
                                          sizeof(Urb->UrbControlVendorClassRequest),
                                          USBD_TRANSFER_DIRECTION_IN,
                                          0,
                                          USB_REQUEST_SET_FEATURE,
                                          PORT_POWER,
                                          1,
                                          0,
                                          0,
                                          0,
                                          0);

                    Urb->UrbOSFeatureDescriptorRequest.MS_FeatureDescriptorIndex = PortLoop + 1;
                    Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

                    DPRINT1("Status %x\n", Status);

                    UsbBuildVendorRequest(Urb,
                                          URB_FUNCTION_CLASS_OTHER,
                                          sizeof(Urb->UrbControlVendorClassRequest),
                                          USBD_TRANSFER_DIRECTION_OUT,
                                          0,
                                          USB_REQUEST_GET_STATUS,
                                          0,
                                          PortLoop + 1,
                                          &PortStatusAndChange,
                                          0,
                                        sizeof(PortStatusAndChange),
                                        0);
                    Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

                    DPRINT1("Status %x\n", Status);
                    DPRINT1("PortStatus = %x\n", PortStatusAndChange[0]);
                    DPRINT1("PortChange = %x\n", PortStatusAndChange[1]);
                }
            }

            ExFreePool(Urb);
            break;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS: /* (optional) 0x7 */
        {
            switch (IrpSp->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                {
                    PDEVICE_RELATIONS DeviceRelations = NULL;
                    DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");

                    Status = UsbhubFdoQueryBusRelations(DeviceObject, &DeviceRelations);

                    Information = (ULONG_PTR)DeviceRelations;
                    break;
                }
                case RemovalRelations:
                {
                    DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
                    return ForwardIrpAndForget(DeviceObject, Irp);
                }
                default:
                    DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
                        IrpSp->Parameters.QueryDeviceRelations.Type);
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
            DPRINT1("Usbhub: IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
            return ForwardIrpAndForget(DeviceObject, Irp);
        }
    }
    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
UsbhubDeviceControlFdo(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    ULONG IoControlCode;
    PHUB_DEVICE_EXTENSION DeviceExtension;
    ULONG LengthIn, LengthOut;
    ULONG_PTR Information = 0;
    PVOID BufferIn, BufferOut;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Stack = IoGetCurrentIrpStackLocation(Irp);
    LengthIn = Stack->Parameters.DeviceIoControl.InputBufferLength;
    LengthOut = Stack->Parameters.DeviceIoControl.OutputBufferLength;
    DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    IoControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
    UsbhubGetUserBuffers(Irp, IoControlCode, &BufferIn, &BufferOut);

    switch (IoControlCode)
    {
        case IOCTL_USB_GET_NODE_INFORMATION:
        {
            //PUSB_NODE_INFORMATION NodeInformation;

            DPRINT1("Usbhub: IOCTL_USB_GET_NODE_INFORMATION\n");
            if (LengthOut < sizeof(USB_NODE_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else if (BufferOut == NULL)
                Status = STATUS_INVALID_PARAMETER;
            else
            {
                /*NodeInformation = (PUSB_NODE_INFORMATION)BufferOut;
                dev = ((PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->dev;
                NodeInformation->NodeType = UsbHub;
                RtlCopyMemory(
                    &NodeInformation->u.HubInformation.HubDescriptor,
                    ((struct usb_hub *)usb_get_intfdata(to_usb_interface(&dev->actconfig->interface[0].dev)))->descriptor,
                    sizeof(USB_HUB_DESCRIPTOR));
                NodeInformation->u.HubInformation.HubIsBusPowered = dev->actconfig->desc.bmAttributes & 0x80;
                Information = sizeof(USB_NODE_INFORMATION);*/
                Status = STATUS_SUCCESS;
            }
            break;
        }
        case IOCTL_USB_GET_NODE_CONNECTION_NAME:
        {
            PHUB_DEVICE_EXTENSION DeviceExtension;
            PUSB_NODE_CONNECTION_NAME ConnectionName;
            DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
            ConnectionName = (PUSB_NODE_CONNECTION_NAME)BufferOut;

            DPRINT1("Usbhub: IOCTL_USB_GET_NODE_CONNECTION_NAME\n");
            if (LengthOut < sizeof(USB_NODE_CONNECTION_NAME))
                Status = STATUS_BUFFER_TOO_SMALL;
            else if (BufferOut == NULL)
                Status = STATUS_INVALID_PARAMETER;
            else if (ConnectionName->ConnectionIndex < 1
                || ConnectionName->ConnectionIndex > USB_MAXCHILDREN)
                Status = STATUS_INVALID_PARAMETER;
            else if (DeviceExtension->Children[ConnectionName->ConnectionIndex - 1] == NULL)
                Status = STATUS_INVALID_PARAMETER;
            else
            {
                ULONG NeededStructureSize;
                DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceExtension->Children[ConnectionName->ConnectionIndex - 1]->DeviceExtension;
                NeededStructureSize = DeviceExtension->SymbolicLinkName.Length + sizeof(UNICODE_NULL) + FIELD_OFFSET(USB_NODE_CONNECTION_NAME, NodeName);
                if (ConnectionName->ActualLength < NeededStructureSize / sizeof(WCHAR)
                    || LengthOut < NeededStructureSize)
                {
                    /* Buffer too small */
                    ConnectionName->ActualLength = NeededStructureSize / sizeof(WCHAR);
                    Information = sizeof(USB_NODE_CONNECTION_NAME);
                    Status = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    RtlCopyMemory(
                        ConnectionName->NodeName,
                        DeviceExtension->SymbolicLinkName.Buffer,
                        DeviceExtension->SymbolicLinkName.Length);
                    ConnectionName->NodeName[DeviceExtension->SymbolicLinkName.Length / sizeof(WCHAR)] = UNICODE_NULL;
                    DPRINT1("Usbhub: IOCTL_USB_GET_NODE_CONNECTION_NAME returns '%S'\n", ConnectionName->NodeName);
                    ConnectionName->ActualLength = NeededStructureSize / sizeof(WCHAR);
                    Information = NeededStructureSize;
                    Status = STATUS_SUCCESS;
                }
                Information = LengthOut;
            }
            break;
        }
        case IOCTL_USB_GET_NODE_CONNECTION_INFORMATION:
        {
            PUSB_NODE_CONNECTION_INFORMATION ConnectionInformation;
/*
            ULONG i, j, k;
            struct usb_device* dev;
            ULONG NumberOfOpenPipes = 0;
            ULONG SizeOfOpenPipesArray;
*/
            ConnectionInformation = (PUSB_NODE_CONNECTION_INFORMATION)BufferOut;

            DPRINT1("Usbhub: IOCTL_USB_GET_NODE_CONNECTION_INFORMATION\n");
            if (LengthOut < sizeof(USB_NODE_CONNECTION_INFORMATION))
                Status = STATUS_BUFFER_TOO_SMALL;
            else if (BufferOut == NULL)
                Status = STATUS_INVALID_PARAMETER;
            else if (ConnectionInformation->ConnectionIndex < 1
                || ConnectionInformation->ConnectionIndex > USB_MAXCHILDREN)
                Status = STATUS_INVALID_PARAMETER;
            else
            {
                DPRINT1("Usbhub: We should succeed\n");
            }
            break;
        }
        case IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION:
        {
            //PUSB_DESCRIPTOR_REQUEST Descriptor;
            DPRINT1("Usbhub: IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION\n");
            Information = 0;
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        case IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME:
        {
            PHUB_DEVICE_EXTENSION DeviceExtension;
            PUSB_NODE_CONNECTION_DRIVERKEY_NAME StringDescriptor;
            DPRINT1("Usbhub: IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME\n");
            DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
            StringDescriptor = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)BufferOut;
            if (LengthOut < sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME))
                Status = STATUS_BUFFER_TOO_SMALL;
            else if (StringDescriptor == NULL)
                Status = STATUS_INVALID_PARAMETER;
            else if (StringDescriptor->ConnectionIndex < 1
                || StringDescriptor->ConnectionIndex > USB_MAXCHILDREN)
                Status = STATUS_INVALID_PARAMETER;
            else if (DeviceExtension->Children[StringDescriptor->ConnectionIndex - 1] == NULL)
                Status = STATUS_INVALID_PARAMETER;
            else
            {
                ULONG StringSize;
                Status = IoGetDeviceProperty(
                    DeviceExtension->Children[StringDescriptor->ConnectionIndex - 1],
                    DevicePropertyDriverKeyName,
                    LengthOut - FIELD_OFFSET(USB_NODE_CONNECTION_DRIVERKEY_NAME, DriverKeyName),
                    StringDescriptor->DriverKeyName,
                    &StringSize);
                if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_TOO_SMALL)
                {
                    StringDescriptor->ActualLength = StringSize + FIELD_OFFSET(USB_NODE_CONNECTION_DRIVERKEY_NAME, DriverKeyName);
                    Information = LengthOut;
                    Status = STATUS_SUCCESS;
                }
            }
            break;
        }
        default:
        {
            /* Pass Irp to lower driver */
            DPRINT1("Usbhub: Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
            return ForwardIrpAndForget(DeviceObject, Irp);
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
