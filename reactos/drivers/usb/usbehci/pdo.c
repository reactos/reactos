/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/pdo.c
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin
 */

#define INITGUID
#define NDEBUG

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
    0x40,       /* bmAttributes; 
    Bit 7: must be set,
        6: Self-powered,
        5: Remote wakeup,
        4..0: reserved */
    0x00       /* MaxPower; */
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
    0x00       /* iInterface; */
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

/* FIXME: Do something better */
VOID NTAPI
UrbWorkerThread(PVOID Context)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)Context;

    while (PdoDeviceExtension->HaltUrbHandling == FALSE)
    {
        CompletePendingURBRequest(PdoDeviceExtension);
        KeStallExecutionProcessor(10);
    }
    DPRINT1("Thread terminated\n");
}

PVOID InternalCreateUsbDevice(UCHAR DeviceNumber, ULONG Port, PUSB_DEVICE Parent, BOOLEAN Hub)
{
    PUSB_DEVICE UsbDevicePointer = NULL;
    UsbDevicePointer = ExAllocatePool(NonPagedPool, sizeof(USB_DEVICE));
    if (!UsbDevicePointer)
    {
        DPRINT1("Out of memory\n");
        return NULL;
    }

    RtlZeroMemory(UsbDevicePointer, sizeof(USB_DEVICE));

    if ((Hub) && (!Parent))
    {
        DPRINT1("This is the root hub\n");
    }

    UsbDevicePointer->Address = DeviceNumber;
    UsbDevicePointer->Port = Port;
    UsbDevicePointer->ParentDevice = Parent;

    UsbDevicePointer->IsHub = Hub;

    return UsbDevicePointer;
}

NTSTATUS NTAPI
PdoDispatchInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PIO_STACK_LOCATION Stack = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG_PTR Information = 0;

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) PdoDeviceExtension->ControllerFdo->DeviceExtension;

    ASSERT(PdoDeviceExtension->Common.IsFdo == FALSE);

    Stack =  IoGetCurrentIrpStackLocation(Irp);

    switch(Stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_INTERNAL_USB_SUBMIT_URB:
        {
            URB *Urb;

            Urb = (PURB) Stack->Parameters.Others.Argument1;
            DPRINT("Header Length %d\n", Urb->UrbHeader.Length);
            DPRINT("Header Function %d\n", Urb->UrbHeader.Function);

            /* Queue all request for now, kernel thread will complete them */
            QueueURBRequest(PdoDeviceExtension, Irp);
            Information = 0;
            IoMarkIrpPending(Irp);
            Status = STATUS_PENDING;
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
            DPRINT1("IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE %x\n", IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE);
            if (Stack->Parameters.Others.Argument1)
            {
                /* Return the root hubs devicehandle */
                DPRINT1("Returning RootHub Handle %x\n", PdoDeviceExtension->UsbDevices[0]);
                *(PVOID *)Stack->Parameters.Others.Argument1 = (PVOID)PdoDeviceExtension->UsbDevices[0];
                Status = STATUS_SUCCESS;
            }
            else
                Status = STATUS_INVALID_DEVICE_REQUEST;

            break;

        }
        case IOCTL_INTERNAL_USB_GET_HUB_COUNT:
        {
            DPRINT1("IOCTL_INTERNAL_USB_GET_HUB_COUNT %x\n", IOCTL_INTERNAL_USB_GET_HUB_COUNT);
            ASSERT(Stack->Parameters.Others.Argument1 != NULL);
            if (Stack->Parameters.Others.Argument1)
            {
                /* FIXME: Determine the number of hubs between the usb device and root hub */
                DPRINT1("RootHubCount %x\n", *(PULONG)Stack->Parameters.Others.Argument1);
                *(PULONG)Stack->Parameters.Others.Argument1 = 0;
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
            DPRINT1("IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO\n");

            if (Stack->Parameters.Others.Argument1)
                *(PVOID *)Stack->Parameters.Others.Argument1 = FdoDeviceExtension->Pdo;
            if (Stack->Parameters.Others.Argument2)
                *(PVOID *)Stack->Parameters.Others.Argument2 = IoGetAttachedDeviceReference(FdoDeviceExtension->DeviceObject);

            Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION:
        {
            DPRINT1("IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION\n");
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
            LONG i;

            PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
            FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->ControllerFdo->DeviceExtension;

            /* Create the root hub */
            RootHubDevice = InternalCreateUsbDevice(1, 0, NULL, TRUE);

            for (i = 0; i < 8; i++)
            {
                PdoDeviceExtension->Ports[i].PortStatus = USB_PORT_STATUS_ENABLE;
                PdoDeviceExtension->Ports[i].PortChange = 0;
            }

            RtlCopyMemory(&RootHubDevice->DeviceDescriptor,
                          ROOTHUB2_DEVICE_DESCRIPTOR,
                          sizeof(ROOTHUB2_DEVICE_DESCRIPTOR));

            RootHubDevice->DeviceDescriptor.idVendor = FdoDeviceExtension->VendorId;
            RootHubDevice->DeviceDescriptor.idProduct = FdoDeviceExtension->DeviceId;

            RootHubDevice->Configs = ExAllocatePoolWithTag(NonPagedPool,
                                                            sizeof(PVOID) * RootHubDevice->DeviceDescriptor.bNumConfigurations,
                                                            USB_POOL_TAG);

            RootHubDevice->Configs[0] = ExAllocatePoolWithTag(NonPagedPool,
                                                            sizeof(USB_CONFIGURATION) + sizeof(PVOID) * ROOTHUB2_CONFIGURATION_DESCRIPTOR[5],
                                                            USB_POOL_TAG);

            RootHubDevice->Configs[0]->Interfaces[0] = ExAllocatePoolWithTag(NonPagedPool,
                                                            sizeof(USB_INTERFACE) + sizeof(PVOID) * ROOTHUB2_INTERFACE_DESCRIPTOR[3],
                                                            USB_POOL_TAG);

            RootHubDevice->Configs[0]->Interfaces[0]->EndPoints[0] = ExAllocatePoolWithTag(NonPagedPool,
                                                            sizeof(USB_ENDPOINT),
                                                            USB_POOL_TAG);

            DPRINT1("before: ActiveConfig %x\n", RootHubDevice->ActiveConfig);
            RootHubDevice->ActiveConfig = RootHubDevice->Configs[0];
            DPRINT1("after: ActiveConfig %x\n", RootHubDevice->ActiveConfig);

            DPRINT1("before: ActiveConfig->Interfaces[0] %x\n", RootHubDevice->ActiveConfig->Interfaces[0]);
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

            /* Create a thread to handle the URB's */
            Status = PsCreateSystemThread(&PdoDeviceExtension->ThreadHandle,
                                          THREAD_ALL_ACCESS,
                                          NULL,
                                          NULL,
                                          NULL,
                                          UrbWorkerThread,
                                          (PVOID)PdoDeviceExtension);

            if (!NT_SUCCESS(Status))
                DPRINT1("Failed Thread Creation with Status: %x\n", Status);

            Status = IoRegisterDeviceInterface(DeviceObject, &GUID_DEVINTERFACE_USB_HUB, NULL, &InterfaceSymLinkName);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to register interface\n");
                ASSERT(FALSE);
            }
            else
            {
                Status = IoSetDeviceInterfaceState(&InterfaceSymLinkName, TRUE);
                DPRINT1("Set interface state %x\n", Status);
                if (!NT_SUCCESS(Status)) ASSERT(FALSE);
            }

            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
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
                    DPRINT1("BusRelations!!!!!\n");
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
            PDEVICE_CAPABILITIES DeviceCapabilities;
            ULONG i;

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

             /* FIXME */
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

            PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
            FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->ControllerFdo->DeviceExtension;

            Status = RtlStringFromGUID(Stack->Parameters.QueryInterface.InterfaceType, &GuidString);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to create string from GUID!\n");
            }
            DPRINT1("Interface GUID requested %wZ\n", &GuidString);
            DPRINT1("QueryInterface.Size %x\n", Stack->Parameters.QueryInterface.Size);
            DPRINT1("QueryInterface.Version %x\n", Stack->Parameters.QueryInterface.Version);

            Status = STATUS_SUCCESS;
            Information = 0;

            /* FIXME: Check the actual Guid */
            if (Stack->Parameters.QueryInterface.Size == sizeof(USB_BUS_INTERFACE_USBDI_V2) && (Stack->Parameters.QueryInterface.Version == 2))
            {
                InterfaceDI = (PUSB_BUS_INTERFACE_USBDI_V2) Stack->Parameters.QueryInterface.Interface;
                InterfaceDI->Size = sizeof(USB_BUS_INTERFACE_USBDI_V2);
                InterfaceDI->Version = 2;
                InterfaceDI->BusContext = PdoDeviceExtension->DeviceObject;
                InterfaceDI->InterfaceReference = (PINTERFACE_REFERENCE)InterfaceReference;
                InterfaceDI->InterfaceDereference = (PINTERFACE_DEREFERENCE)InterfaceDereference;
                InterfaceDI->GetUSBDIVersion = GetUSBDIVersion;
                InterfaceDI->QueryBusTime = QueryBusTime;
                InterfaceDI->SubmitIsoOutUrb = SubmitIsoOutUrb;
                InterfaceDI->QueryBusInformation = QueryBusInformation;
                InterfaceDI->IsDeviceHighSpeed = IsDeviceHighSpeed;
                InterfaceDI->EnumLogEntry = EnumLogEntry;
            }
            /* FIXME: Check the actual Guid */
            else if (Stack->Parameters.QueryInterface.Size == sizeof(USB_BUS_INTERFACE_HUB_V5) &&
                    (Stack->Parameters.QueryInterface.Version == 5))
            {
                InterfaceHub = (PUSB_BUS_INTERFACE_HUB_V5)Stack->Parameters.QueryInterface.Interface;
                InterfaceHub->Version = 5;
                InterfaceHub->Size = sizeof(USB_BUS_INTERFACE_HUB_V5);
                InterfaceHub->BusContext = PdoDeviceExtension->DeviceObject;
                InterfaceHub->InterfaceReference = (PINTERFACE_REFERENCE)InterfaceReference;
                InterfaceHub->InterfaceDereference = (PINTERFACE_DEREFERENCE)InterfaceDereference;
                InterfaceHub->CreateUsbDevice = CreateUsbDevice;
                InterfaceHub->InitializeUsbDevice = InitializeUsbDevice;
                InterfaceHub->GetUsbDescriptors = GetUsbDescriptors;
                InterfaceHub->RemoveUsbDevice = RemoveUsbDevice;
                InterfaceHub->RestoreUsbDevice = RestoreUsbDevice;
                InterfaceHub->GetPortHackFlags = GetPortHackFlags;
                InterfaceHub->QueryDeviceInformation = QueryDeviceInformation;
                InterfaceHub->GetControllerInformation = GetControllerInformation;
                InterfaceHub->ControllerSelectiveSuspend = ControllerSelectiveSuspend;
                InterfaceHub->GetExtendedHubInformation = GetExtendedHubInformation;
                InterfaceHub->GetRootHubSymbolicName = GetRootHubSymbolicName;
                InterfaceHub->GetDeviceBusContext = GetDeviceBusContext;
                InterfaceHub->Initialize20Hub = Initialize20Hub;
                InterfaceHub->RootHubInitNotification = RootHubInitNotification;
                InterfaceHub->FlushTransfers = FlushTransfers;
                InterfaceHub->SetDeviceHandleData = SetDeviceHandleData;
            }
            else
            {
                DPRINT1("Not Supported\n");
                Status = Irp->IoStatus.Status;
                Information = Irp->IoStatus.Information;
            }
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
