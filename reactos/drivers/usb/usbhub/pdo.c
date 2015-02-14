/*
 * PROJECT:         ReactOS Universal Serial Bus Hub Driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/usb/usbhub/fdo.c
 * PURPOSE:         Handle PDO
 * PROGRAMMERS:
 *                  Hervé Poussineau (hpoussin@reactos.org)
 *                  Michael Martin (michael.martin@reactos.org)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbhub.h"

#include <wdmguid.h>

#define NDEBUG
#include <debug.h>

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
			0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);

NTSTATUS
NTAPI
UrbCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    PIRP OriginalIrp;
    DPRINT("Entered Urb Completion\n");

    //
    // Get the original Irp
    //
    OriginalIrp = (PIRP)Context;

    //
    // Update it to match what was returned for the IRP that was passed to RootHub
    //
    OriginalIrp->IoStatus.Status = Irp->IoStatus.Status;
    OriginalIrp->IoStatus.Information = Irp->IoStatus.Information;
    DPRINT("Status %x, Information %x\n", Irp->IoStatus.Status, Irp->IoStatus.Information);

    //
    // Complete the original Irp
    //
    IoCompleteRequest(OriginalIrp, IO_NO_INCREMENT);

    //
    // Return this status so the IO Manager doesnt mess with the Irp
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
FowardUrbToRootHub(
    PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG IoControlCode,
    PIRP Irp,
    OUT PVOID OutParameter1,
    OUT PVOID OutParameter2)
{
    PIRP ForwardIrp;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION ForwardStack, CurrentStack;
    PURB Urb;

    //
    // Get the current stack location for the Irp
    //
    CurrentStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(CurrentStack);

    //
    // Pull the Urb from that stack, it will be reused in the Irp sent to RootHub
    //
    Urb = (PURB)CurrentStack->Parameters.Others.Argument1;
    ASSERT(Urb);

    //
    // Create the Irp to forward to RootHub
    //
    ForwardIrp = IoBuildAsynchronousFsdRequest(IRP_MJ_SHUTDOWN,
                                               RootHubDeviceObject,
                                               NULL,
                                               0,
                                               0,
                                               &IoStatus);
    if (!ForwardIrp)
    {
        DPRINT1("Failed to allocate IRP\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the new Irps next stack
    //
    ForwardStack = IoGetNextIrpStackLocation(ForwardIrp);

    //
    // Copy the stack for the current irp into the next stack of new irp
    //
    RtlCopyMemory(ForwardStack, CurrentStack, sizeof(IO_STACK_LOCATION));

    IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoStatus.Information = 0;

    //
    // Mark the Irp from upper driver as pending
    //
    IoMarkIrpPending(Irp);

    //
    // Now set the completion routine for the new Irp.
    //
    IoSetCompletionRoutine(ForwardIrp,
                           UrbCompletion,
                           Irp,
                           TRUE,
                           TRUE,
                           TRUE);

    IoCallDriver(RootHubDeviceObject, ForwardIrp);

    //
    // Always return pending as the completion routine will take care of it
    //
    return STATUS_PENDING;
}

BOOLEAN
IsValidPDO(
    IN PDEVICE_OBJECT DeviceObject)
{
    ULONG Index;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;


    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(ChildDeviceExtension->Common.IsFDO == FALSE);
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)ChildDeviceExtension->ParentDeviceObject->DeviceExtension;

    for(Index = 0; Index < USB_MAXCHILDREN; Index++)
    {
        if (HubDeviceExtension->ChildDeviceObject[Index] == DeviceObject)
        {
            /* PDO exists */
            return TRUE;
        }
    }

    /* invalid pdo */
    return FALSE;
}


NTSTATUS
USBHUB_PdoHandleInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    ULONG_PTR Information = 0;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;
    PDEVICE_OBJECT RootHubDeviceObject;
    PURB Urb;

    //DPRINT1("UsbhubInternalDeviceControlPdo(%x) called\n", DeviceObject);

    //
    // get current stack location
    //
    Stack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(Stack);

    //
    // Set default status
    //
    Status = Irp->IoStatus.Status;

    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(ChildDeviceExtension->Common.IsFDO == FALSE);
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)ChildDeviceExtension->ParentDeviceObject->DeviceExtension;
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;

    if(!IsValidPDO(DeviceObject))
    {
        DPRINT1("[USBHUB] Request for removed device object %p\n", DeviceObject);
        Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO:
        {
            DPRINT("IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO\n");
            if (Irp->AssociatedIrp.SystemBuffer == NULL
                || Stack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(PVOID))
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                PVOID* pHubPointer;

                pHubPointer = (PVOID*)Irp->AssociatedIrp.SystemBuffer;
                // FIXME
                *pHubPointer = NULL;
                Information = sizeof(PVOID);
                Status = STATUS_SUCCESS;
            }
            break;
        }
        case IOCTL_INTERNAL_USB_SUBMIT_URB:
        {
            //DPRINT1("IOCTL_INTERNAL_USB_SUBMIT_URB\n");

            //
            // Get the Urb
            //
            Urb = (PURB)Stack->Parameters.Others.Argument1;
            ASSERT(Urb);

            //
            // Set the real device handle
            //
            //DPRINT("UsbdDeviceHandle %x, ChildDeviceHandle %x\n", Urb->UrbHeader.UsbdDeviceHandle, ChildDeviceExtension->UsbDeviceHandle);

            Urb->UrbHeader.UsbdDeviceHandle = ChildDeviceExtension->UsbDeviceHandle;

            //
            // Submit to RootHub
            //
            switch (Urb->UrbHeader.Function)
            {
                //
                // Debugging only
                //
                case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
                    DPRINT("URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE\n");
                    break;
                case URB_FUNCTION_CLASS_DEVICE:
                    DPRINT("URB_FUNCTION_CLASS_DEVICE\n");
                    break;
                case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
                    DPRINT("URB_FUNCTION_GET_STATUS_FROM_DEVICE\n");
                    break;
                case URB_FUNCTION_SELECT_CONFIGURATION:
                    DPRINT("URB_FUNCTION_SELECT_CONFIGURATION\n");
                    break;
                case URB_FUNCTION_SELECT_INTERFACE:
                    DPRINT("URB_FUNCTION_SELECT_INTERFACE\n");
                    break;
                case URB_FUNCTION_CLASS_OTHER:
                    DPRINT("URB_FUNCTION_CLASS_OTHER\n");
                    break;
                case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
                {
                /*
                    DPRINT1("URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER\n");
                    DPRINT1("PipeHandle %x\n", Urb->UrbBulkOrInterruptTransfer.PipeHandle);
                    DPRINT1("TransferFlags %x\n", Urb->UrbBulkOrInterruptTransfer.TransferFlags);
                    DPRINT1("Buffer %x\n", Urb->UrbBulkOrInterruptTransfer.TransferBuffer);
                    DPRINT1("BufferMDL %x\n", Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL);
                    DPRINT1("Length %x\n", Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);
                    DPRINT1("UrbLink %x\n", Urb->UrbBulkOrInterruptTransfer.UrbLink);
                    DPRINT1("hca %x\n", Urb->UrbBulkOrInterruptTransfer.hca);
                    if (Urb->UrbBulkOrInterruptTransfer.TransferFlags == USBD_SHORT_TRANSFER_OK)
                    {
                    }
                */
                    break;

                }
                case URB_FUNCTION_CLASS_INTERFACE:
                    DPRINT("URB_FUNCTION_CLASS_INTERFACE\n");
                    break;
                case URB_FUNCTION_VENDOR_DEVICE:
                    DPRINT("URB_FUNCTION_VENDOR_DEVICE\n");
                    break;
                default:
                    DPRINT1("IOCTL_INTERNAL_USB_SUBMIT_URB Function %x NOT IMPLEMENTED\n", Urb->UrbHeader.Function);
                    break;
            }
            Urb->UrbHeader.UsbdDeviceHandle = ChildDeviceExtension->UsbDeviceHandle;
            //DPRINT1("Stack->CompletionRoutine %x\n", Stack->CompletionRoutine);
            //
            // Send the request to RootHub
            //
            Status = FowardUrbToRootHub(RootHubDeviceObject, IOCTL_INTERNAL_USB_SUBMIT_URB, Irp, Urb, NULL);
            return Status;
        }
        //
        // FIXME: Can these be sent to RootHub?
        //
        case IOCTL_INTERNAL_USB_RESET_PORT:
            DPRINT1("IOCTL_INTERNAL_USB_RESET_PORT\n");
            break;
        case IOCTL_INTERNAL_USB_GET_PORT_STATUS:
        {
            PORT_STATUS_CHANGE PortStatus;
            ULONG PortId;
            PUCHAR PortStatusBits;

            PortStatusBits = (PUCHAR)Stack->Parameters.Others.Argument1;
            //
            // USBD_PORT_ENABLED (bit 0) or USBD_PORT_CONNECTED (bit 1)
            //
            DPRINT1("IOCTL_INTERNAL_USB_GET_PORT_STATUS\n");
            DPRINT("Arg1 %x\n", *PortStatusBits);
            *PortStatusBits = 0;
            if (Stack->Parameters.Others.Argument1)
            {
                for (PortId = 1; PortId <= HubDeviceExtension->UsbExtHubInfo.NumberOfPorts; PortId++)
                {
                    Status = GetPortStatusAndChange(RootHubDeviceObject, PortId, &PortStatus);
                    if (NT_SUCCESS(Status))
                    {
                        DPRINT("Connect %x\n", ((PortStatus.Status & USB_PORT_STATUS_CONNECT) << 1) << ((PortId - 1) * 2));
                        DPRINT("Enable %x\n", ((PortStatus.Status & USB_PORT_STATUS_ENABLE) >> 1) << ((PortId - 1) * 2));
                        *PortStatusBits +=
                            (((PortStatus.Status & USB_PORT_STATUS_CONNECT) << 1) << ((PortId - 1) * 2)) +
                            (((PortStatus.Status & USB_PORT_STATUS_ENABLE) >> 1) << ((PortId - 1) * 2));

                    }
                }
            }

            DPRINT1("Arg1 %x\n", *PortStatusBits);
            Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_INTERNAL_USB_ENABLE_PORT:
            DPRINT1("IOCTL_INTERNAL_USB_ENABLE_PORT\n");
            break;
        case IOCTL_INTERNAL_USB_CYCLE_PORT:
            DPRINT1("IOCTL_INTERNAL_USB_CYCLE_PORT\n");
            break;
        case IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE:
        {
            DPRINT1("IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE\n");
            if (Stack->Parameters.Others.Argument1)
            {
                // store device handle
                *(PVOID *)Stack->Parameters.Others.Argument1 = (PVOID)ChildDeviceExtension->UsbDeviceHandle;
                Status = STATUS_SUCCESS;
            }
            else
            {
                // invalid parameter
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        }
        case IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO:
        {
            if (Stack->Parameters.Others.Argument1)
            {
                // inform caller that it is a real usb hub
                *(PVOID *)Stack->Parameters.Others.Argument1 = NULL;
            }

            if (Stack->Parameters.Others.Argument2)
            {
                // output device object
                *(PVOID *)Stack->Parameters.Others.Argument2 = DeviceObject;
            }

            // done
            Status = STATUS_SUCCESS;
            break;
        }
        default:
        {
            DPRINT1("Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
        }
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Information = Information;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    return Status;
}

NTSTATUS
USBHUB_PdoStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;
    //NTSTATUS Status;
    DPRINT("USBHUB_PdoStartDevice %x\n", DeviceObject);
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // This should be a PDO
    //
    ASSERT(ChildDeviceExtension->Common.IsFDO == FALSE);

    //
    // register device interface
    //
    IoRegisterDeviceInterface(DeviceObject, &GUID_DEVINTERFACE_USB_DEVICE, NULL, &ChildDeviceExtension->SymbolicLinkName);
    IoSetDeviceInterfaceState(&ChildDeviceExtension->SymbolicLinkName, TRUE);

    UNIMPLEMENTED
    return STATUS_SUCCESS;
}

NTSTATUS
USBHUB_PdoQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT ULONG_PTR* Information)
{
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;
    ULONG IdType;
    PUNICODE_STRING SourceString = NULL;
    PWCHAR ReturnString = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    switch (IdType)
    {
        case BusQueryDeviceID:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
            SourceString = &ChildDeviceExtension->usDeviceId;
            break;
        }
        case BusQueryHardwareIDs:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");
            SourceString = &ChildDeviceExtension->usHardwareIds;
            break;
        }
        case BusQueryCompatibleIDs:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
            SourceString = &ChildDeviceExtension->usCompatibleIds;
            break;
        }
        case BusQueryInstanceID:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
            SourceString = &ChildDeviceExtension->usInstanceId;
            break;
        }
        default:
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
            return STATUS_NOT_SUPPORTED;
    }

    if (SourceString)
    {
        //
        // allocate buffer
        //
        ReturnString = ExAllocatePool(PagedPool, SourceString->MaximumLength);
        if (!ReturnString)
        {
            //
            // no memory
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // copy buffer
        //
        RtlCopyMemory(ReturnString, SourceString->Buffer, SourceString->MaximumLength);
    }

    *Information = (ULONG_PTR)ReturnString;

    return Status;
}

NTSTATUS
USBHUB_PdoQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT ULONG_PTR* Information)
{
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;
    DEVICE_TEXT_TYPE DeviceTextType;
    PUNICODE_STRING SourceString = NULL;
    PWCHAR ReturnString = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    DeviceTextType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryDeviceText.DeviceTextType;
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // FIXME: LocaleId
    //

    switch (DeviceTextType)
    {
        case DeviceTextDescription:
        case DeviceTextLocationInformation:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription\n");

            //
            // does the device provide a text description
            //
            if (ChildDeviceExtension->usTextDescription.Buffer && ChildDeviceExtension->usTextDescription.Length)
            {
                //
                // use device text
                //
                SourceString = &ChildDeviceExtension->usTextDescription;
            }
            break;
        }
        default:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown device text type 0x%lx\n", DeviceTextType);
            Status = STATUS_NOT_SUPPORTED;
            break;
        }
    }

    if (SourceString)
    {
        ReturnString = ExAllocatePool(PagedPool, SourceString->MaximumLength);
        RtlCopyMemory(ReturnString, SourceString->Buffer, SourceString->MaximumLength);
        DPRINT1("%S\n", ReturnString);
        *Information = (ULONG_PTR)ReturnString;
    }

    return Status;
}

NTSTATUS
USBHUB_PdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    ULONG MinorFunction;
    PIO_STACK_LOCATION Stack;
    ULONG_PTR Information = 0;
    PHUB_CHILDDEVICE_EXTENSION UsbChildExtension;
    ULONG Index;
    ULONG bFound;
    PDEVICE_RELATIONS DeviceRelation;
    PDEVICE_OBJECT ParentDevice;

    UsbChildExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);
    MinorFunction = Stack->MinorFunction;

    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
            Status = USBHUB_PdoStartDevice(DeviceObject, Irp);
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            PDEVICE_CAPABILITIES DeviceCapabilities;
            ULONG i;
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_CAPABILITIES\n");

            DeviceCapabilities = (PDEVICE_CAPABILITIES)Stack->Parameters.DeviceCapabilities.Capabilities;
            // FIXME: capabilities can change with connected device
            DeviceCapabilities->LockSupported = FALSE;
            DeviceCapabilities->EjectSupported = FALSE;
            DeviceCapabilities->Removable = TRUE;
            DeviceCapabilities->DockDevice = FALSE;
            DeviceCapabilities->UniqueID = FALSE;
            DeviceCapabilities->SilentInstall = FALSE;
            DeviceCapabilities->RawDeviceOK = FALSE;
            DeviceCapabilities->SurpriseRemovalOK = FALSE;
            DeviceCapabilities->HardwareDisabled = FALSE;
            //DeviceCapabilities->NoDisplayInUI = FALSE;
            DeviceCapabilities->Address = UsbChildExtension->PortNumber;
            DeviceCapabilities->UINumber = 0;
            DeviceCapabilities->DeviceState[0] = PowerDeviceD0;
            for (i = 1; i < PowerSystemMaximum; i++)
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
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCES\n");

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
        case IRP_MN_QUERY_DEVICE_TEXT:
        {
            Status = USBHUB_PdoQueryDeviceText(DeviceObject, Irp, &Information);
            break;
        }
        case IRP_MN_QUERY_ID:
        {
            Status = USBHUB_PdoQueryId(DeviceObject, Irp, &Information);
            break;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            PPNP_BUS_INFORMATION BusInfo;
            BusInfo = (PPNP_BUS_INFORMATION)ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
            RtlCopyMemory(&BusInfo->BusTypeGuid,
                          &GUID_BUS_TYPE_USB,
                          sizeof(BusInfo->BusTypeGuid));
            BusInfo->LegacyBusType = PNPBus;
            // FIXME
            BusInfo->BusNumber = 0;
            Information = (ULONG_PTR)BusInfo;
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            PHUB_DEVICE_EXTENSION HubDeviceExtension = (PHUB_DEVICE_EXTENSION)UsbChildExtension->ParentDeviceObject->DeviceExtension;
            PUSB_BUS_INTERFACE_HUB_V5 HubInterface = &HubDeviceExtension->HubInterface;
            ParentDevice = UsbChildExtension->ParentDeviceObject;

            DPRINT("IRP_MJ_PNP / IRP_MN_REMOVE_DEVICE\n");

            /* remove us from pdo list */
            bFound = FALSE;
            for(Index = 0; Index < USB_MAXCHILDREN; Index++)
            {
                if (HubDeviceExtension->ChildDeviceObject[Index] == DeviceObject)
                {
                     /* Remove the device */
                     Status = HubInterface->RemoveUsbDevice(HubDeviceExtension->UsbDInterface.BusContext, UsbChildExtension->UsbDeviceHandle, 0);

                     /* FIXME handle error */
                     ASSERT(Status == STATUS_SUCCESS);

                    /* remove us */
                    HubDeviceExtension->ChildDeviceObject[Index] = NULL;
                    bFound = TRUE;
                    break;
                }
            }

            /* Complete the IRP */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            /* delete device */
            IoDeleteDevice(DeviceObject);

            if (bFound)
            {
                /* invalidate device relations */
                IoInvalidateDeviceRelations(ParentDevice, BusRelations);
            }

            return STATUS_SUCCESS;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            /* only target relations are supported */
            if (Stack->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
            {
                /* not supported */
                Status = Irp->IoStatus.Status;
                break;
            }

            /* allocate device relations */
            DeviceRelation = (PDEVICE_RELATIONS)ExAllocatePool(NonPagedPool, sizeof(DEVICE_RELATIONS));
            if (!DeviceRelation)
            {
                /* no memory */
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            /* init device relation */
            DeviceRelation->Count = 1;
            DeviceRelation->Objects[0] = DeviceObject;
            ObReferenceObject(DeviceRelation->Objects[0]);

            /* store result */
            Irp->IoStatus.Information = (ULONG_PTR)DeviceRelation;
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        {
            /* Sure, no problem */
            Status = STATUS_SUCCESS;
            Information = 0;
            break;
        }
        case IRP_MN_QUERY_INTERFACE:
        {
            DPRINT1("IRP_MN_QUERY_INTERFACE\n");
            if (IsEqualGUIDAligned(Stack->Parameters.QueryInterface.InterfaceType, &USB_BUS_INTERFACE_USBDI_GUID))
            {
                DPRINT1("USB_BUS_INTERFACE_USBDI_GUID\n");
                RtlCopyMemory(Stack->Parameters.QueryInterface.Interface, &UsbChildExtension->DeviceInterface, Stack->Parameters.QueryInterface.Size);
                Status = STATUS_SUCCESS;
                break;
            }

            // pass irp down
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(UsbChildExtension->ParentDeviceObject, Irp);
        }
        case IRP_MN_SURPRISE_REMOVAL:
        {
            DPRINT("[USBHUB] HandlePnp IRP_MN_SURPRISE_REMOVAL\n");
            Status = STATUS_SUCCESS;
            break;
        }
        default:
        {
            DPRINT1("PDO IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

