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
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
UsbhubPnpFdo(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
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
            URB Urb;
            ULONG Result = 0;
            USB_DEVICE_INFORMATION_0 DeviceInformation;

            /* We differ from windows on hubpdo because we dont have usbport.sys which manages all usb device objects */
            DPRINT("Usbhub: IRP_MJ_PNP / IRP_MN_START_DEVICE\n");

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
            QueryRootHub(DeviceObject,IOCTL_INTERNAL_USB_GET_HUB_COUNT, &DeviceExtension->HubCount, NULL);

            /* Get the Direct Call Interfaces */
            Status = QueryInterface(DeviceObject, USB_BUS_INTERFACE_HUB_GUID, sizeof(USB_BUS_INTERFACE_HUB_V5), 5, (PVOID)&DeviceExtension->HubInterface);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("UsbhubM Failed to get HUB_GUID interface with status 0x%08lx\n", Status);
                return STATUS_UNSUCCESSFUL;
            }

            Status = QueryInterface(DeviceObject, USB_BUS_INTERFACE_USBDI_GUID, sizeof(USB_BUS_INTERFACE_USBDI_V2), 2, (PVOID)&DeviceExtension->UsbDInterface);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("UsbhubM Failed to get USBDI_GUID interface with status 0x%08lx\n", Status);
                return STATUS_UNSUCCESSFUL;
            }

            /* Get roothub device handle */
            Status = QueryRootHub(DeviceObject, IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE, &DeviceExtension->RootHubUsbDevice, NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Usbhub: GetRootHubDeviceHandle failed with status 0x%08lx\n", Status);
                return Status;
            }

            RtlZeroMemory(&DeviceInformation, sizeof(USB_DEVICE_INFORMATION_0));

            Status = DeviceExtension->HubInterface.QueryDeviceInformation(DeviceExtension->RootHubPdo,
                                                                 DeviceExtension->RootHubUsbDevice,
                                                                 &DeviceInformation,
                                                                 sizeof(USB_DEVICE_INFORMATION_0),
                                                                 &Result);

            DPRINT1("Status %x, Result %x\n", Status, Result);
            DPRINT1("InformationLevel %x\n", DeviceInformation.InformationLevel);
            DPRINT1("ActualLength %x\n", DeviceInformation.ActualLength);
            DPRINT1("PortNumber %x\n", DeviceInformation.PortNumber);
            DPRINT1("DeviceDescriptor %x\n", DeviceInformation.DeviceDescriptor);
            DPRINT1("HubAddress %x\n", DeviceInformation.HubAddress);
            DPRINT1("NumberofPipes %x\n", DeviceInformation.NumberOfOpenPipes);



            UsbBuildGetDescriptorRequest(&Urb,
                                         sizeof(Urb.UrbControlDescriptorRequest),
                                         USB_DEVICE_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         &DeviceExtension->HubDeviceDescriptor,
                                         NULL,
                                         sizeof(USB_DEVICE_DESCRIPTOR),
                                         NULL);

            Urb.UrbHeader.UsbdDeviceHandle = DeviceExtension->RootHubUsbDevice;

            Status = QueryRootHub(DeviceObject, IOCTL_INTERNAL_USB_SUBMIT_URB, &Urb, NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Usbhub: Failed to get HubDeviceDescriptor!\n");
            }

            UsbBuildGetDescriptorRequest(&Urb,
                                         sizeof(Urb.UrbControlDescriptorRequest),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         &DeviceExtension->HubConfig,
                                         NULL,
                                         sizeof(USB_CONFIGURATION_DESCRIPTOR),
                                         NULL);
            Urb.UrbHeader.UsbdDeviceHandle = DeviceExtension->RootHubUsbDevice;

            Status = QueryRootHub(DeviceObject, IOCTL_INTERNAL_USB_SUBMIT_URB, &Urb, NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Usbhub: Failed to get HubConfig!\n");
            }

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

            /* FIXME: HubDescriptor is empty and shouldnt be but Status is success */
            UsbBuildVendorRequest(&Urb,
                                  URB_FUNCTION_CLASS_DEVICE,
                                  sizeof(Urb.UrbControlVendorClassRequest),
                                  USBD_TRANSFER_DIRECTION_IN,
                                  0,
                                  USB_DEVICE_CLASS_RESERVED,
                                  0,
                                  0,
                                  &DeviceExtension->HubDescriptor,
                                  NULL,
                                  sizeof(USB_HUB_DESCRIPTOR),
                                  NULL);

            Urb.UrbHeader.UsbdDeviceHandle = DeviceExtension->RootHubUsbDevice;

            Status = QueryRootHub(DeviceObject, IOCTL_INTERNAL_USB_SUBMIT_URB, &Urb, NULL);

            DPRINT1("bDescriptorType %x\n", DeviceExtension->HubDescriptor.bDescriptorType);

            Status = DeviceExtension->HubInterface.Initialize20Hub(DeviceExtension->RootHubPdo, DeviceExtension->RootHubUsbDevice, 1);

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
