/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/pdo.c
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin
 */

/* INCLUDES *******************************************************************/
#define INITGUID
#include "usbehci.h"
#include <wdmguid.h>
#include <stdio.h>

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
PdoDispatchInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PIO_STACK_LOCATION Stack = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG_PTR Information = 0;
    DPRINT("PdoDispatchInternalDeviceControl\n");

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) PdoDeviceExtension->ControllerFdo->DeviceExtension;

    ASSERT(PdoDeviceExtension->Common.IsFdo == FALSE);
    Stack =  IoGetCurrentIrpStackLocation(Irp);
    DPRINT("IoControlCode %x\n", Stack->Parameters.DeviceIoControl.IoControlCode);
    switch(Stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_INTERNAL_USB_SUBMIT_URB:
        {
            URB *Urb;

            DPRINT1("IOCTL_INTERNAL_USB_SUBMIT_URB\n");
            DPRINT("Stack->Parameters.DeviceIoControl.InputBufferLength %d\n",
                     Stack->Parameters.DeviceIoControl.InputBufferLength);
            DPRINT("Stack->Parameters.Others.Argument1 %x\n", Stack->Parameters.Others.Argument1);

            Urb = (PURB) Stack->Parameters.Others.Argument1;
            DPRINT("Header Size %d\n", Urb->UrbHeader.Length);
            DPRINT("Header Type %d\n", Urb->UrbHeader.Function);
            DPRINT("Index %x\n", Urb->UrbControlDescriptorRequest.Index);

            /* Check the type */
            switch(Urb->UrbHeader.Function)
            {
                case URB_FUNCTION_SELECT_CONFIGURATION:
                {
                    DPRINT1("URB_FUNCTION_SELECT_CONFIGURATION\n");
                    break;
                }
                case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
                {
                    URB *Urb;
                    DPRINT1("URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE\n");
                    Urb = (PURB) Stack->Parameters.Others.Argument1;
                    Urb->UrbHeader.Status = 0;

                    DPRINT1("Irp->CancelRoutine %x\n",Irp->CancelRoutine);
                    QueueRequest(FdoDeviceExtension, Irp);
  
                    Information = 0;

                    IoMarkIrpPending(Irp);
                    Status = STATUS_PENDING;

                    break;
                }
                /* FIXME: Handle all other Functions */
                default:
                    DPRINT1("Not handled yet!\n");
            }
            break;
        }
        case IOCTL_INTERNAL_USB_CYCLE_PORT:
        {
            DPRINT("IOCTL_INTERNAL_USB_CYCLE_PORT\n");
            break;
        }
        case IOCTL_INTERNAL_USB_ENABLE_PORT:
        {
            DPRINT("IOCTL_INTERNAL_USB_ENABLE_PORT\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_BUS_INFO:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_BUS_INFO\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_BUSGUID_INFO:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_BUSGUID_INFO\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_HUB_COUNT:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_HUB_COUNT\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_HUB_NAME:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_HUB_NAME\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_PORT_STATUS:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_PORT_STATUS\n");
            break;
        }
        case IOCTL_INTERNAL_USB_RESET_PORT:
        {
            DPRINT("IOCTL_INTERNAL_USB_RESET_PORT\n");
            break;
        }
        case IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO\n");
            /* This is document as Argument1 = PDO and Argument2 = FDO.
               Its actually reversed, the FDO goes in Argument1 and PDO goes in Argument2 */
            if (Stack->Parameters.Others.Argument1)
                Stack->Parameters.Others.Argument1 = FdoDeviceExtension->DeviceObject;
            if (Stack->Parameters.Others.Argument2)
                Stack->Parameters.Others.Argument2 = FdoDeviceExtension->Pdo;

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            return STATUS_SUCCESS;
            break;
        }
        case IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION:
        {
            DPRINT("IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION\n");
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
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    WCHAR Buffer[256];
    ULONG Index = 0;
    ULONG IdType;
    UNICODE_STRING SourceString;
    UNICODE_STRING String;
    NTSTATUS Status;

    IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->ControllerFdo->DeviceExtension;

    /* FIXME: Read values from registry */

    switch (IdType)
    {
        case BusQueryDeviceID:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
            RtlInitUnicodeString(&SourceString, L"USB\\ROOT_HUB20");
            break;
        }
        case BusQueryHardwareIDs:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");

            Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB20&VID8086&PID265C&REV0000") + 1;
            Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB20&VID8086&PID265") + 1;
            Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB20") + 1;

            Buffer[Index] = UNICODE_NULL;
            SourceString.Length = SourceString.MaximumLength = Index * sizeof(WCHAR);
            SourceString.Buffer = Buffer;
            break;
        }
        case BusQueryCompatibleIDs:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
                        /* We have none */
            return STATUS_SUCCESS;
            break;
        }
        case BusQueryInstanceID:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");

               /*
               Do we need to implement this?
               At one point I hade DeviceCapabilities->UniqueID set to TRUE.
               And caused usbhub to fail attaching
               to the PDO. Setting UniqueID to FALSE, it works
               */

            return STATUS_SUCCESS;
            break;
        }
        default:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
            return STATUS_NOT_SUPPORTED;
        }
    }

    /* Lifted from hpoussin */
    Status = DuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                    &SourceString,
                                    &String);

    *Information = (ULONG_PTR)String.Buffer;
    return Status;
}

NTSTATUS
PdoQueryDeviceRelations(PDEVICE_OBJECT DeviceObject, PDEVICE_RELATIONS* pDeviceRelations)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_RELATIONS DeviceRelations;

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    ObReferenceObject(DeviceObject);
    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = DeviceObject;

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
    ULONG_PTR Information = 0;
    NTSTATUS Status;

    Stack = IoGetCurrentIrpStackLocation(Irp);
    MinorFunction = Stack->MinorFunction;

    switch (MinorFunction)
    {
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_DEVICE_TEXT:
        case IRP_MN_SURPRISE_REMOVAL:
        {
            Information = Irp->IoStatus.Information;
            Status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_START_DEVICE:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
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
                    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / TargetDeviceRelation\n");
                    Status = PdoQueryDeviceRelations(DeviceObject, &DeviceRelations);
                    Information = (ULONG_PTR)DeviceRelations;
                    break;
                }
                default:
                {
                    DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unhandled type 0x%lx\n",
                        Stack->Parameters.QueryDeviceRelations.Type);
                    //ASSERT(FALSE);
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
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_CAPABILITIES\n");
            DeviceCapabilities = (PDEVICE_CAPABILITIES)Stack->Parameters.DeviceCapabilities.Capabilities;
            /* FIXME: capabilities can change with connected device */
            DeviceCapabilities->LockSupported = FALSE;
            DeviceCapabilities->EjectSupported = FALSE;
            DeviceCapabilities->Removable = FALSE;
            DeviceCapabilities->DockDevice = FALSE;
            DeviceCapabilities->UniqueID = FALSE;//TRUE;
            DeviceCapabilities->SilentInstall = FALSE;
            DeviceCapabilities->RawDeviceOK = TRUE;
            DeviceCapabilities->SurpriseRemovalOK = FALSE;

             /* FIXME */
            DeviceCapabilities->HardwareDisabled = FALSE;
            //DeviceCapabilities->NoDisplayInUI = FALSE;
            DeviceCapabilities->DeviceState[0] = PowerDeviceD0;
            for (i = 0; i < PowerSystemMaximum; i++)
                DeviceCapabilities->DeviceState[i] = PowerDeviceD3;
            //DeviceCapabilities->DeviceWake = PowerDeviceUndefined;
            DeviceCapabilities->D1Latency = 0;
            DeviceCapabilities->D2Latency = 0;
            DeviceCapabilities->D3Latency = 0;
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_RESOURCES:
        {
            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
            break;
        }
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
            break;
        }
        /*case IRP_MN_QUERY_DEVICE_TEXT:
        {
            Status = STATUS_NOT_SUPPORTED;
            break;
        }*/
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
            break;
        }
        case IRP_MN_QUERY_ID:
        {
            Status = PdoQueryId(DeviceObject, Irp, &Information);
            break;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            PPNP_BUS_INFORMATION BusInfo;
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_BUS_INFORMATION\n");

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

            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
            break;
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

