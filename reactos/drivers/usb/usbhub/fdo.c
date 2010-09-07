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
        DPRINT1("bLength %x\n", InterfaceDescriptor->bLength);
        DPRINT1("bDescriptorType %x\n", InterfaceDescriptor->bDescriptorType);
        DPRINT1("bInterfaceNumber %x\n", InterfaceDescriptor->bInterfaceNumber);
        DPRINT1("bAlternateSetting %x\n", InterfaceDescriptor->bAlternateSetting);
        DPRINT1("bNumEndpoints %x\n", InterfaceDescriptor->bNumEndpoints);
        DPRINT1("bInterfaceClass %x\n", InterfaceDescriptor->bInterfaceClass);
        DPRINT1("bInterfaceSubClass %x\n", InterfaceDescriptor->bInterfaceSubClass);
        DPRINT1("bInterfaceProtocol %x\n", InterfaceDescriptor->bInterfaceProtocol);
        DPRINT1("iInterface %x\n", InterfaceDescriptor->iInterface);

        EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) ((ULONG_PTR)InterfaceDescriptor + sizeof(USB_INTERFACE_DESCRIPTOR));

        for (j=0; j < InterfaceDescriptor->bNumEndpoints; j++)
        {
            DPRINT1("bLength %x\n", EndpointDescriptor->bLength);
            DPRINT1("bDescriptorType %x\n", EndpointDescriptor->bDescriptorType);
            DPRINT1("bEndpointAddress %x\n", EndpointDescriptor->bEndpointAddress);
            DPRINT1("bmAttributes %x\n", EndpointDescriptor->bmAttributes);
            DPRINT1("wMaxPacketSize %x\n", EndpointDescriptor->wMaxPacketSize);
            DPRINT1("bInterval %x\n", EndpointDescriptor->bInterval);
        }

        InterfaceDescriptor += sizeof(USB_ENDPOINT_DESCRIPTOR);
    }

}
NTSTATUS
DeviceArrivalCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{

    PHUB_DEVICE_EXTENSION DeviceExtension;
    LONG i;

    DeviceExtension = (PHUB_DEVICE_EXTENSION)((PDEVICE_OBJECT)Context)->DeviceExtension;

    /* FIXME: Need to use the number of ports returned from miniport at device start */
    for (i=0; i < 8; i++)
        DPRINT1("Port %x DeviceExtension->PortStatus %x\n",i, DeviceExtension->PortStatus[i]);

    return STATUS_SUCCESS;
}


NTSTATUS
WaitForUsbDeviceArrivalNotification(PDEVICE_OBJECT DeviceObject)
{
    PURB Urb;
    PIRP Irp;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;
    IO_STATUS_BLOCK IoStatus;
    PHUB_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);

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

    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
                                        DeviceExtension->RootHubPdo,
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

    IoSetCompletionRoutineEx(DeviceExtension->RootHubPdo, Irp, (PIO_COMPLETION_ROUTINE)DeviceArrivalCompletion, DeviceObject, TRUE, TRUE, TRUE);

    Status = IoCallDriver(DeviceExtension->RootHubPdo, Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;

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
        DPRINT("Usbhub: Operation pending\n");
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
    PDEVICE_OBJECT Pdo;
    PHUB_DEVICE_EXTENSION PdoExtension;
    ULONG i;
    ULONG Children = 0;
    ULONG NeededSize;

    DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DPRINT1("USBHUB: Query Bus Relations\n");
    /* Create PDOs that are missing */
    for (i = 0; i < USB_MAXCHILDREN; i++)
    {

        if (DeviceExtension->Children[i] == NULL)
        {
            continue;
        }
        Children++;
        Pdo = DeviceExtension->Children[i];
        Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

        PdoExtension = Pdo->DeviceExtension;

        RtlZeroMemory(PdoExtension, sizeof(HUB_DEVICE_EXTENSION));

        PdoExtension->IsFDO = FALSE;

        Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
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

            /* We differ from windows on hubpdo because we dont have usbport.sys which manages all usb device objects */
            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_START_DEVICE\n");

            /* Allocating size including the sizeof USBD_INTERFACE_LIST_ENTRY */
            Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB) + sizeof(USBD_INTERFACE_LIST_ENTRY), USB_HUB_TAG);
            RtlZeroMemory(Urb, sizeof(URB) + sizeof(USBD_INTERFACE_LIST_ENTRY));

            /* Get the hubs PDO */
            QueryRootHub(DeviceObject, IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO, &DeviceExtension->RootHubPdo, &DeviceExtension->RootHubFdo);
            ASSERT(DeviceExtension->RootHubPdo);
            ASSERT(DeviceExtension->RootHubFdo);

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

            /* FIXME: This gets nothing from MS miniport */
            Status = DeviceExtension->HubInterface.QueryDeviceInformation(DeviceExtension->RootHubPdo,
                                                                 DeviceExtension->RootHubUsbDevice,
                                                                 &DeviceExtension->DeviceInformation,
                                                                 sizeof(USB_DEVICE_INFORMATION_0),
                                                                 &Result);


            DPRINT1("Status %x, Result %x\n", Status, Result);
            DPRINT1("InformationLevel %x\n", DeviceExtension->DeviceInformation.InformationLevel);
            DPRINT1("ActualLength %x\n", DeviceExtension->DeviceInformation.ActualLength);
            DPRINT1("PortNumber %x\n", DeviceExtension->DeviceInformation.PortNumber);
            DPRINT1("DeviceDescriptor %x\n", DeviceExtension->DeviceInformation.DeviceDescriptor);
            DPRINT1("HubAddress %x\n", DeviceExtension->DeviceInformation.HubAddress);
            DPRINT1("NumberofPipes %x\n", DeviceExtension->DeviceInformation.NumberOfOpenPipes);

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
            /* FIXME: Use USBD_CreateConfigurationRequestEx instead */
            RtlZeroMemory(Urb, sizeof(URB));
            UsbBuildSelectConfigurationRequest(Urb,
                                               sizeof(Urb->UrbSelectConfiguration),
                                               &DeviceExtension->HubConfigDescriptor);

            Urb->UrbSelectConfiguration.Interface.Length = sizeof(USBD_INTERFACE_INFORMATION);
            Urb->UrbSelectConfiguration.Interface.NumberOfPipes = 1;
            Urb->UrbSelectConfiguration.Interface.Pipes[0].MaximumTransferSize = 4096;

            Status = QueryRootHub(DeviceExtension->RootHubPdo, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

            DPRINT1("Status %x\n", Status);

            DeviceExtension->ConfigurationHandle = Urb->UrbSelectConfiguration.ConfigurationHandle;
            DeviceExtension->PipeHandle = Urb->UrbSelectConfiguration.Interface.Pipes[0].PipeHandle;
            DPRINT1("Configuration Handle %x\n", DeviceExtension->ConfigurationHandle);

            Status = DeviceExtension->HubInterface.Initialize20Hub(DeviceExtension->RootHubPdo, DeviceExtension->RootHubUsbDevice, 1);

            DPRINT1("Status %x\n", Status);

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
