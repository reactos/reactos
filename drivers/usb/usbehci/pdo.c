/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/pdo.c
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

#define INITGUID

#include "usbehci.h"
#include <hubbusif.h>
#include <usbbusif.h>
#include "usbiffn.h"
#include <wdmguid.h>
#include <stdio.h>
#include <debug.h>

/* Lifted from Linux with slight changes */
const UCHAR ROOTHUB2_DEVICE_DESCRIPTOR [] =
{
    0x12,       /*  bLength; */
    USB_DEVICE_DESCRIPTOR_TYPE,       /*  bDescriptorType; Device */
    0x00, 0x20, /*  bcdUSB; v1.1 */
    USB_DEVICE_CLASS_HUB,       /*  bDeviceClass; HUB_CLASSCODE */
    0x01,       /*  bDeviceSubClass; */
    0x00,       /*  bDeviceProtocol; [ low/full speeds only ] */
    0x08,       /*  bMaxPacketSize0; 8 Bytes */
    /* Fill Vendor and Product in when init root hub */
    0x00, 0x00, /*  idVendor; */
    0x00, 0x00, /*  idProduct; */
    0x00, 0x00, /*  bcdDevice */
    0x00,       /*  iManufacturer; */
    0x00,       /*  iProduct; */
    0x00,       /*  iSerialNumber; */
    0x01        /*  bNumConfigurations; */

};

const UCHAR ROOTHUB2_CONFIGURATION_DESCRIPTOR [] =
{
    /* one configuration */
    0x09,       /* bLength; */
    0x02,       /* bDescriptorType; Configuration */
    0x19, 0x00, /* wTotalLength; */
    0x01,       /* bNumInterfaces; (1) */
    0x23,       /* bConfigurationValue; */
    0x00,       /* iConfiguration; */
    0x40,       /* bmAttributes; */
    0x00        /* MaxPower; */
};

const UCHAR ROOTHUB2_INTERFACE_DESCRIPTOR [] =
{
    /* one interface */
    0x09,       /* bLength: Interface; */
    0x04,       /* bDescriptorType; Interface */
    0x00,       /* bInterfaceNumber; */
    0x00,       /* bAlternateSetting; */
    0x01,       /* bNumEndpoints; */
    0x09,       /* bInterfaceClass; HUB_CLASSCODE */
    0x01,       /* bInterfaceSubClass; */
    0x00,       /* bInterfaceProtocol: */
    0x00        /* iInterface; */
};

const UCHAR ROOTHUB2_ENDPOINT_DESCRIPTOR [] =
{
    /* one endpoint (status change endpoint) */
    0x07,       /* bLength; */
    0x05,       /* bDescriptorType; Endpoint */
    0x81,       /* bEndpointAddress; IN Endpoint 1 */
    0x03,       /* bmAttributes; Interrupt */
    0x08, 0x00, /* wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8) */
    0xFF        /* bInterval; (255ms -- usb 2.0 spec) */
};

NTSTATUS NTAPI
PdoDispatchInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PIO_STACK_LOCATION Stack = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG_PTR Information = 0;
    PEHCI_HOST_CONTROLLER hcd;

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) PdoDeviceExtension->ControllerFdo->DeviceExtension;

    ASSERT(PdoDeviceExtension->Common.IsFdo == FALSE);

    hcd = &FdoDeviceExtension->hcd;
    Stack =  IoGetCurrentIrpStackLocation(Irp);

    switch(Stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_INTERNAL_USB_SUBMIT_URB:
        {
            PUSB_DEVICE UsbDevice = NULL;
            URB *Urb;
            ULONG i;

            Urb = (PURB) Stack->Parameters.Others.Argument1;

            UsbDevice = Urb->UrbHeader.UsbdDeviceHandle;

            /* If there was no device passed then this URB is for the RootHub */
            if (UsbDevice == NULL)
                UsbDevice = PdoDeviceExtension->UsbDevices[0];

            /* Check if it is a Status Change Endpoint (SCE). The Hub Driver sends this request and miniports mark the IRP pending
               if there is no changes on any of the ports. When the DPC of miniport routine detects changes this IRP will be completed.
               Based on XEN PV Usb Drivers */

            if ((Urb->UrbHeader.Function == URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER) &&
                (UsbDevice == PdoDeviceExtension->UsbDevices[0]))
            {
                DPRINT("URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER on SCE\n");
                if (Urb->UrbBulkOrInterruptTransfer.PipeHandle != &UsbDevice->ActiveInterface->EndPoints[0]->EndPointDescriptor)
                {
                    DPRINT1("PipeHandle doesnt match SCE PipeHandle\n");
                }

                /* Queue the Irp first */
                QueueURBRequest(PdoDeviceExtension, Irp);

                /* Check for port changes */
                if (EnumControllerPorts(hcd) == FALSE)
                {
                    DPRINT("No port change\n");
                    Status = STATUS_PENDING;
                    IoMarkIrpPending(Irp);
                    break;
                }

                /* If we reached this point then port status has changed, so check
                   which port */
                for (i = 0; i < hcd->ECHICaps.HCSParams.PortCount; i++)
                {
                    if (hcd->Ports[i].PortChange == 0x01)
                    {
                        DPRINT1("On SCE request: Inform hub driver that port %d has changed\n", i+1);
                        ASSERT(FALSE);
                        ((PUCHAR)Urb->UrbBulkOrInterruptTransfer.TransferBuffer)[0] = 1 << ((i + 1) & 7);
                        Information = 0;
                        Status = STATUS_SUCCESS;
                        /* Assume URB success */
                        Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
                        /* Set the DeviceHandle to the Internal Device */
                        Urb->UrbHeader.UsbdDeviceHandle = UsbDevice;

                        /* Request handled, Remove it from the queue */
                        RemoveUrbRequest(PdoDeviceExtension, Irp);
                        break;
                    }
                }
                if (Status == STATUS_SUCCESS) break;

                IoMarkIrpPending(Irp);
                Status = STATUS_PENDING;
                break;
            }

            Status = HandleUrbRequest(PdoDeviceExtension, Irp);

            break;
        }
        case IOCTL_INTERNAL_USB_CYCLE_PORT:
        {
            DPRINT1("IOCTL_INTERNAL_USB_CYCLE_PORT\n");
            break;
        }
        case IOCTL_INTERNAL_USB_ENABLE_PORT:
        {
            DPRINT1("IOCTL_INTERNAL_USB_ENABLE_PORT\n");
            Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_INTERNAL_USB_GET_BUS_INFO:
        {
            DPRINT1("IOCTL_INTERNAL_USB_GET_BUS_INFO\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_BUSGUID_INFO:
        {
            DPRINT1("IOCTL_INTERNAL_USB_GET_BUSGUID_INFO\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME:
        {
            DPRINT1("IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE:
        {
            DPRINT1("Ehci: IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE %x\n", Stack->Parameters.Others.Argument2);
            if (Stack->Parameters.Others.Argument1)
            {
                /* Return the root hubs devicehandle */
                DPRINT("Returning RootHub Handle %x\n", PdoDeviceExtension->UsbDevices[0]);
                *(PVOID *)Stack->Parameters.Others.Argument1 = (PVOID)PdoDeviceExtension->UsbDevices[0];
                Status = STATUS_SUCCESS;
            }
            else
                Status = STATUS_INVALID_DEVICE_REQUEST;

            break;

        }
        case IOCTL_INTERNAL_USB_GET_HUB_COUNT:
        {
            DPRINT1("Ehci: IOCTL_INTERNAL_USB_GET_HUB_COUNT %x\n", IOCTL_INTERNAL_USB_GET_HUB_COUNT);
            ASSERT(Stack->Parameters.Others.Argument1 != NULL);
            if (Stack->Parameters.Others.Argument1)
            {
                /* FIXME: Determine the number of hubs between the usb device and root hub. 
                   For now we have at least one. */
                *(PULONG)Stack->Parameters.Others.Argument1 = 1;
            }
            Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_INTERNAL_USB_GET_HUB_NAME:
        {
            DPRINT1("IOCTL_INTERNAL_USB_GET_HUB_NAME\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO:
        {
            DPRINT1("IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_PORT_STATUS:
        {
            DPRINT1("IOCTL_INTERNAL_USB_GET_PORT_STATUS\n");
            break;
        }
        case IOCTL_INTERNAL_USB_RESET_PORT:
        {
            DPRINT1("IOCTL_INTERNAL_USB_RESET_PORT\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO\n");
            /* DDK documents that both the PDO and FDO are returned. However, while writing the UsbHub driver it was determine
               that this was not the case. Windows usbehci driver gives the Pdo in both Arguments. Which makes sense as upper level
               drivers should not be communicating with FDO. */
            if (Stack->Parameters.Others.Argument1)
                *(PVOID *)Stack->Parameters.Others.Argument1 = FdoDeviceExtension->Pdo;
            if (Stack->Parameters.Others.Argument2)
                *(PVOID *)Stack->Parameters.Others.Argument2 = FdoDeviceExtension->Pdo;
            Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION:
        {
            PUSB_IDLE_CALLBACK_INFO CallBackInfo;
            DPRINT1("Ehci: IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION\n");
            /* FIXME: Set Callback for safe power down */
            CallBackInfo = Stack->Parameters.DeviceIoControl.Type3InputBuffer;

            PdoDeviceExtension->IdleCallback = CallBackInfo->IdleCallback;
            PdoDeviceExtension->IdleContext = CallBackInfo->IdleContext;

            Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }
        default:
        {
            DPRINT1("Unhandled IoControlCode %x\n", Stack->Parameters.DeviceIoControl.IoControlCode);
            break;
        }
    }

    Irp->IoStatus.Information = Information;

    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
PdoQueryId(PDEVICE_OBJECT DeviceObject, PIRP Irp, ULONG_PTR* Information)
{
    WCHAR Buffer[256];
    ULONG Index = 0;
    ULONG IdType;
    UNICODE_STRING SourceString;
    UNICODE_STRING String;
    NTSTATUS Status;

    IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;

    switch (IdType)
    {
        case BusQueryDeviceID:
        {
            RtlInitUnicodeString(&SourceString, L"USB\\ROOT_HUB20");
            break;
        }
        case BusQueryHardwareIDs:
        {
            /* FIXME: Build from Device Vendor and Device ID */
            Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB20&VID8086&PID265C&REV0000") + 1;
            Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB20&VID8086&PID265C") + 1;
            Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB20") + 1;

            Buffer[Index] = UNICODE_NULL;
            SourceString.Length = SourceString.MaximumLength = Index * sizeof(WCHAR);
            SourceString.Buffer = Buffer;
            break;

        }
        case BusQueryCompatibleIDs:
        {
            /* We have none */
            return STATUS_SUCCESS;
        }
        case BusQueryInstanceID:
        {
            return STATUS_SUCCESS;
        }
        default:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
            return STATUS_NOT_SUPPORTED;
        }
    }

    Status = DuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                    &SourceString,
                                    &String);

    *Information = (ULONG_PTR)String.Buffer;
    return Status;
}

NTSTATUS
PdoQueryDeviceRelations(PDEVICE_OBJECT DeviceObject, PDEVICE_RELATIONS* pDeviceRelations)
{
    PDEVICE_RELATIONS DeviceRelations;

    DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = DeviceObject;
    ObReferenceObject(DeviceObject);

    *pDeviceRelations = DeviceRelations;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
PdoDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    ULONG MinorFunction;
    PIO_STACK_LOCATION Stack;
    ULONG_PTR Information = Irp->IoStatus.Information;
    NTSTATUS Status = Irp->IoStatus.Status;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    ULONG i;

    Stack = IoGetCurrentIrpStackLocation(Irp);
    MinorFunction = Stack->MinorFunction;

    switch (MinorFunction)
    {
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_QUERY_DEVICE_TEXT:
        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        {
            Status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_START_DEVICE:
        {
            PUSB_DEVICE RootHubDevice;
            PPDO_DEVICE_EXTENSION PdoDeviceExtension;
            PFDO_DEVICE_EXTENSION FdoDeviceExtension;
            UNICODE_STRING InterfaceSymLinkName;

            DPRINT1("Ehci: PDO StartDevice\n");
            PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
            FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->ControllerFdo->DeviceExtension;

            /* Create the root hub */
            RootHubDevice = InternalCreateUsbDevice(1, 0, NULL, TRUE);

            RtlCopyMemory(&RootHubDevice->DeviceDescriptor,
                          ROOTHUB2_DEVICE_DESCRIPTOR,
                          sizeof(ROOTHUB2_DEVICE_DESCRIPTOR));

            RootHubDevice->DeviceDescriptor.idVendor = FdoDeviceExtension->VendorId;
            RootHubDevice->DeviceDescriptor.idProduct = FdoDeviceExtension->DeviceId;

            /* Here config, interfaces and descriptors are stored. This was duplicated from XEN PV Usb Drivers implementation.
               Not sure that it is really needed as the information can be queueried from the device. */

            RootHubDevice->Configs = ExAllocatePoolWithTag(NonPagedPool,
                                                            sizeof(PVOID) * RootHubDevice->DeviceDescriptor.bNumConfigurations,
                                                            USB_POOL_TAG);

            RootHubDevice->Configs[0] = ExAllocatePoolWithTag(NonPagedPool,
                                                            sizeof(USB_CONFIGURATION) + sizeof(PVOID) * ROOTHUB2_CONFIGURATION_DESCRIPTOR[4],
                                                            USB_POOL_TAG);

            RootHubDevice->Configs[0]->Interfaces[0] = ExAllocatePoolWithTag(NonPagedPool,
                                                            sizeof(USB_INTERFACE) + sizeof(PVOID) * ROOTHUB2_INTERFACE_DESCRIPTOR[4],
                                                            USB_POOL_TAG);

            RootHubDevice->Configs[0]->Interfaces[0]->EndPoints[0] = ExAllocatePoolWithTag(NonPagedPool,
                                                            sizeof(USB_ENDPOINT),
                                                            USB_POOL_TAG);

            RootHubDevice->ActiveConfig = RootHubDevice->Configs[0];
            RootHubDevice->ActiveInterface = RootHubDevice->ActiveConfig->Interfaces[0];

            RtlCopyMemory(&RootHubDevice->ActiveConfig->ConfigurationDescriptor,
                          ROOTHUB2_CONFIGURATION_DESCRIPTOR,
                          sizeof(ROOTHUB2_CONFIGURATION_DESCRIPTOR));

            RtlCopyMemory(&RootHubDevice->ActiveConfig->Interfaces[0]->InterfaceDescriptor,
                         ROOTHUB2_INTERFACE_DESCRIPTOR,
                         sizeof(ROOTHUB2_INTERFACE_DESCRIPTOR));

            RtlCopyMemory(&RootHubDevice->ActiveConfig->Interfaces[0]->EndPoints[0]->EndPointDescriptor,
                         ROOTHUB2_ENDPOINT_DESCRIPTOR,
                         sizeof(ROOTHUB2_ENDPOINT_DESCRIPTOR));
            RootHubDevice->DeviceSpeed = UsbHighSpeed;
            RootHubDevice->DeviceType = Usb20Device;

            PdoDeviceExtension->UsbDevices[0] = RootHubDevice;

            Status = IoRegisterDeviceInterface(DeviceObject, &GUID_DEVINTERFACE_USB_HUB, NULL, &InterfaceSymLinkName);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to register interface\n");
                return Status;
            }
            else
            {
                Status = IoSetDeviceInterfaceState(&InterfaceSymLinkName, TRUE);
                if (!NT_SUCCESS(Status)) 
                    return Status;
            }

            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            DPRINT1("Ehci: PDO QueryDeviceRelations\n");
            switch (Stack->Parameters.QueryDeviceRelations.Type)
            {
                case TargetDeviceRelation:
                {
                    PDEVICE_RELATIONS DeviceRelations = NULL;
                    Status = PdoQueryDeviceRelations(DeviceObject, &DeviceRelations);
                    Information = (ULONG_PTR)DeviceRelations;
                    break;
                }
                case BusRelations:
                {
                    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
                    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

                    DPRINT("BusRelations!!!!!\n");

                    /* The hub driver has created the new device object and reported to pnp, as a result the pnp manager
                       has sent this IRP and type, so leave the next SCE request pending until a new device arrives.
                       Is there a better way to do this? */
                    ExAcquireFastMutex(&PdoDeviceExtension->ListLock);
                    PdoDeviceExtension->HaltQueue = TRUE;
                    ExReleaseFastMutex(&PdoDeviceExtension->ListLock);
                }
                case RemovalRelations:
                case EjectionRelations:
                {
                    /* Ignore the request */
                    Information = Irp->IoStatus.Information;
                    Status = Irp->IoStatus.Status;
                    break;

                }
                default:
                {
                    DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unhandled type 0x%lx\n",
                        Stack->Parameters.QueryDeviceRelations.Type);
                    Status = STATUS_NOT_SUPPORTED;
                    break;
                }
            }
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            DPRINT("Ehci: PDO Query Capabilities\n");

            DeviceCapabilities = (PDEVICE_CAPABILITIES)Stack->Parameters.DeviceCapabilities.Capabilities;

            DeviceCapabilities->LockSupported = FALSE;
            DeviceCapabilities->EjectSupported = FALSE;
            DeviceCapabilities->Removable = FALSE;
            DeviceCapabilities->DockDevice = FALSE;
            DeviceCapabilities->UniqueID = FALSE;
            DeviceCapabilities->SilentInstall = FALSE;
            DeviceCapabilities->RawDeviceOK = FALSE;
            DeviceCapabilities->SurpriseRemovalOK = FALSE;
            DeviceCapabilities->Address = 0;
            DeviceCapabilities->UINumber = 0;
            DeviceCapabilities->DeviceD2 = 1;

             /* FIXME: Verify these settings are correct */
            DeviceCapabilities->HardwareDisabled = FALSE;
            //DeviceCapabilities->NoDisplayInUI = FALSE;
            DeviceCapabilities->DeviceState[0] = PowerDeviceD0;
            for (i = 0; i < PowerSystemMaximum; i++)
                DeviceCapabilities->DeviceState[i] = PowerDeviceD3;
            DeviceCapabilities->DeviceWake = 0;
            DeviceCapabilities->D1Latency = 0;
            DeviceCapabilities->D2Latency = 0;
            DeviceCapabilities->D3Latency = 0;
            Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }
        /*case IRP_MN_QUERY_DEVICE_TEXT:
        {
            Status = STATUS_NOT_SUPPORTED;
            break;
        }*/

        case IRP_MN_QUERY_ID:
        {
            DPRINT("Ehci: PDO Query ID\n");
            Status = PdoQueryId(DeviceObject, Irp, &Information);
            break;
        }
        case IRP_MN_QUERY_INTERFACE:
        {
            UNICODE_STRING GuidString;
            PUSB_BUS_INTERFACE_HUB_V5 InterfaceHub;
            PUSB_BUS_INTERFACE_USBDI_V2 InterfaceDI;
            PPDO_DEVICE_EXTENSION PdoDeviceExtension;
            PFDO_DEVICE_EXTENSION FdoDeviceExtension;

            DPRINT("Ehci: PDO Query Interface\n");

            PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
            FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->ControllerFdo->DeviceExtension;

            Status = RtlStringFromGUID(Stack->Parameters.QueryInterface.InterfaceType, &GuidString);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to create string from GUID!\n");
            }

            /* Assume success */
            Status = STATUS_SUCCESS;
            Information = 0;

            if (IsEqualGUIDAligned(Stack->Parameters.QueryInterface.InterfaceType, &USB_BUS_INTERFACE_HUB_GUID))
            {
                InterfaceHub = (PUSB_BUS_INTERFACE_HUB_V5)Stack->Parameters.QueryInterface.Interface;
                InterfaceHub->Version = Stack->Parameters.QueryInterface.Version;
                if (Stack->Parameters.QueryInterface.Version >= 0)
                {
                    InterfaceHub->Size = Stack->Parameters.QueryInterface.Size;
                    InterfaceHub->BusContext = PdoDeviceExtension->DeviceObject;
                    InterfaceHub->InterfaceReference = (PINTERFACE_REFERENCE)InterfaceReference;
                    InterfaceHub->InterfaceDereference = (PINTERFACE_DEREFERENCE)InterfaceDereference;
                }
                if (Stack->Parameters.QueryInterface.Version >= 1)
                {
                    InterfaceHub->CreateUsbDevice = CreateUsbDevice;
                    InterfaceHub->InitializeUsbDevice = InitializeUsbDevice;
                    InterfaceHub->GetUsbDescriptors = GetUsbDescriptors;
                    InterfaceHub->RemoveUsbDevice = RemoveUsbDevice;
                    InterfaceHub->RestoreUsbDevice = RestoreUsbDevice;
                    InterfaceHub->GetPortHackFlags = GetPortHackFlags;
                    InterfaceHub->QueryDeviceInformation = QueryDeviceInformation;
                }
                if (Stack->Parameters.QueryInterface.Version >= 2)
                {
                    InterfaceHub->GetControllerInformation = GetControllerInformation;
                    InterfaceHub->ControllerSelectiveSuspend = ControllerSelectiveSuspend;
                    InterfaceHub->GetExtendedHubInformation = GetExtendedHubInformation;
                    InterfaceHub->GetRootHubSymbolicName = GetRootHubSymbolicName;
                    InterfaceHub->GetDeviceBusContext = GetDeviceBusContext;
                    InterfaceHub->Initialize20Hub = Initialize20Hub;

                }
                if (Stack->Parameters.QueryInterface.Version >= 3)
                {
                    InterfaceHub->RootHubInitNotification = RootHubInitNotification;
                }
                if (Stack->Parameters.QueryInterface.Version >= 4)
                {
                    InterfaceHub->FlushTransfers = FlushTransfers;
                }
                if (Stack->Parameters.QueryInterface.Version >= 5)
                {
                    InterfaceHub->SetDeviceHandleData = SetDeviceHandleData;
                }
                if (Stack->Parameters.QueryInterface.Version >= 6)
                {
                    DPRINT1("USB_BUS_INTERFACE_HUB_GUID version not supported!\n");
                }
                break;
            }

            if (IsEqualGUIDAligned(Stack->Parameters.QueryInterface.InterfaceType, &USB_BUS_INTERFACE_USBDI_GUID))
            {
                InterfaceDI = (PUSB_BUS_INTERFACE_USBDI_V2) Stack->Parameters.QueryInterface.Interface;
                InterfaceDI->Version = Stack->Parameters.QueryInterface.Version;
                if (Stack->Parameters.QueryInterface.Version >= 0)
                {
                    //InterfaceDI->Size = sizeof(USB_BUS_INTERFACE_USBDI_V2);
                    InterfaceDI->Size = Stack->Parameters.QueryInterface.Size;
                    InterfaceDI->BusContext = PdoDeviceExtension->DeviceObject;
                    InterfaceDI->InterfaceReference = (PINTERFACE_REFERENCE)InterfaceReference;
                    InterfaceDI->InterfaceDereference = (PINTERFACE_DEREFERENCE)InterfaceDereference;
                    InterfaceDI->GetUSBDIVersion = GetUSBDIVersion;
                    InterfaceDI->QueryBusTime = QueryBusTime;
                    InterfaceDI->SubmitIsoOutUrb = SubmitIsoOutUrb;
                    InterfaceDI->QueryBusInformation = QueryBusInformation;
                }
                if (Stack->Parameters.QueryInterface.Version >= 1)
                {
                    InterfaceDI->IsDeviceHighSpeed = IsDeviceHighSpeed;
                }
                if (Stack->Parameters.QueryInterface.Version >= 2)
                {
                    InterfaceDI->EnumLogEntry = EnumLogEntry;
                }

                if (Stack->Parameters.QueryInterface.Version >= 3)
                {
                    DPRINT1("SB_BUS_INTERFACE_USBDI_GUID version not supported!\n");
                }
                break;
            }

            DPRINT1("GUID Not Supported\n");
            Status = Irp->IoStatus.Status;
            Information = Irp->IoStatus.Information;

            break;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            PPNP_BUS_INFORMATION BusInfo;

            BusInfo = (PPNP_BUS_INFORMATION)ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
            if (!BusInfo)
                Status = STATUS_INSUFFICIENT_RESOURCES;
            else
            {
                /* FIXME */
                /*RtlCopyMemory(
                    &BusInfo->BusTypeGuid,
                    &GUID_DEVINTERFACE_XXX,
                    sizeof(GUID));*/

                BusInfo->LegacyBusType = PNPBus;
                BusInfo->BusNumber = 0;
                Information = (ULONG_PTR)BusInfo;
                Status = STATUS_SUCCESS;
            }
            break;
        }
        default:
        {
            /* We are the PDO. So ignore */
            DPRINT1("IRP_MJ_PNP / Unknown minor function 0x%lx\n", MinorFunction);
            break;
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
