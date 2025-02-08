/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbccgp/pdo.c
 * PURPOSE:     USB  device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 *              Cameron Gutman
 */

#include "usbccgp.h"

#include <ntddk.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
USBCCGP_PdoHandleQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    LPWSTR Buffer;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    LPWSTR GenericString = L"Composite USB Device";

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // check if type is description
    //
    if (IoStack->Parameters.QueryDeviceText.DeviceTextType != DeviceTextDescription)
    {
        //
        // we only handle description
        //
        return Irp->IoStatus.Status;
    }

    //
    // is there a device description
    //
    if (PDODeviceExtension->FunctionDescriptor->FunctionDescription.Length)
    {
        //
        // allocate buffer
        //
        Buffer = AllocateItem(NonPagedPool, PDODeviceExtension->FunctionDescriptor->FunctionDescription.Length + sizeof(WCHAR));
        if (!Buffer)
        {
            //
            // no memory
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // copy buffer
        //
        Irp->IoStatus.Information = (ULONG_PTR)Buffer;
        RtlCopyMemory(Buffer, PDODeviceExtension->FunctionDescriptor->FunctionDescription.Buffer, PDODeviceExtension->FunctionDescriptor->FunctionDescription.Length);
        return STATUS_SUCCESS;
    }

    //
    // FIXME use GenericCompositeUSBDeviceString
    //
    UNIMPLEMENTED;
    Buffer = AllocateItem(PagedPool, (wcslen(GenericString) + 1) * sizeof(WCHAR));
    if (!Buffer)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(Buffer, GenericString, (wcslen(GenericString) + 1) * sizeof(WCHAR));
    Irp->IoStatus.Information = (ULONG_PTR)Buffer;

    return STATUS_SUCCESS;
}

NTSTATUS
USBCCGP_PdoHandleDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations;
    PIO_STACK_LOCATION IoStack;

    DPRINT("USBCCGP_PdoHandleDeviceRelations\n");

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // check if relation type is BusRelations
    //
    if (IoStack->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
    {
        //
        // PDO handles only target device relation
        //
        return Irp->IoStatus.Status;
    }

    //
    // allocate device relations
    //
    DeviceRelations = (PDEVICE_RELATIONS)AllocateItem(PagedPool, sizeof(DEVICE_RELATIONS));
    if (!DeviceRelations)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize device relations
    //
    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = DeviceObject;
    ObReferenceObject(DeviceObject);

    //
    // store result
    //
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    //
    // completed successfully
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USBCCGP_PdoAppendInterfaceNumber(
    IN LPWSTR DeviceId,
    IN ULONG InterfaceNumber,
    OUT LPWSTR *OutString)
{
    ULONG Length = 0, StringLength;
    LPWSTR String;

    //
    // count length of string
    //
    String = DeviceId;
    while (*String)
    {
        StringLength = wcslen(String) + 1;
        Length += StringLength;
        Length += 6; //&MI_XX
        String += StringLength;
    }

    //
    // now allocate the buffer
    //
    String = AllocateItem(NonPagedPool, (Length + 2) * sizeof(WCHAR));
    if (!String)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // store result
    //
    *OutString = String;

    while (*DeviceId)
    {
        StringLength = swprintf(String, L"%s&MI_%02x", DeviceId, InterfaceNumber) + 1;
        Length = wcslen(DeviceId) + 1;
        DPRINT("String %p\n", String);

        //
        // next string
        //
        String += StringLength;
        DeviceId += Length;
    }

    //
    // success
    //
    return STATUS_SUCCESS;
}


NTSTATUS
USBCCGP_PdoHandleQueryId(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PUNICODE_STRING DeviceString = NULL;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    NTSTATUS Status;
    LPWSTR Buffer;

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;


    if (IoStack->Parameters.QueryId.IdType == BusQueryDeviceID)
    {
        //
        // handle query device id
        //
        if (IoForwardIrpSynchronously(PDODeviceExtension->NextDeviceObject, Irp))
        {
            Status = Irp->IoStatus.Status;
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }

        if (NT_SUCCESS(Status))
        {
            //
            // allocate buffer
            //
            Buffer = AllocateItem(NonPagedPool, (wcslen((LPWSTR)Irp->IoStatus.Information) + 7) * sizeof(WCHAR));
            if (Buffer)
            {
                //
                // append interface number
                //
                ASSERT(Irp->IoStatus.Information);
                swprintf(Buffer, L"%s&MI_%02x", (LPWSTR)Irp->IoStatus.Information, PDODeviceExtension->FunctionDescriptor->FunctionNumber);
                DPRINT("BusQueryDeviceID %S\n", Buffer);

                ExFreePool((PVOID)Irp->IoStatus.Information);
                Irp->IoStatus.Information = (ULONG_PTR)Buffer;
            }
            else
            {
                //
                // no memory
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        return Status;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
    {
        //
        // handle instance id
        //
        DeviceString = &PDODeviceExtension->FunctionDescriptor->HardwareId;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryInstanceID)
    {
        //
        // handle instance id
        //
        Buffer = AllocateItem(NonPagedPool, 5 * sizeof(WCHAR));
        if (!Buffer)
        {
            //
            // no memory
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // use function number
        //
        swprintf(Buffer, L"%04x", PDODeviceExtension->FunctionDescriptor->FunctionNumber);
        Irp->IoStatus.Information = (ULONG_PTR)Buffer;
        return STATUS_SUCCESS;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryCompatibleIDs)
    {
        //
        // handle instance id
        //
        DeviceString = &PDODeviceExtension->FunctionDescriptor->CompatibleId;
    }
    else
    {
        //
        // unsupported query
        //
        return Irp->IoStatus.Status;
    }

    //
    // sanity check
    //
    ASSERT(DeviceString != NULL);

    //
    // allocate buffer
    //
    Buffer = AllocateItem(NonPagedPool, DeviceString->Length + sizeof(WCHAR));
    if (!Buffer)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy buffer
    //
    RtlCopyMemory(Buffer, DeviceString->Buffer, DeviceString->Length);
    Buffer[DeviceString->Length / sizeof(WCHAR)] = UNICODE_NULL;
    Irp->IoStatus.Information = (ULONG_PTR)Buffer;

    return STATUS_SUCCESS;
}

NTSTATUS
PDO_HandlePnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    NTSTATUS Status;
    ULONG Index, bFound;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    switch(IoStack->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            //
            // handle device relations
            //
            Status = USBCCGP_PdoHandleDeviceRelations(DeviceObject, Irp);
            break;
        }
        case IRP_MN_QUERY_DEVICE_TEXT:
        {
            //
            // handle query device text
            //
            Status = USBCCGP_PdoHandleQueryDeviceText(DeviceObject, Irp);
            break;
        }
        case IRP_MN_QUERY_ID:
        {
            //
            // handle request
            //
            Status = USBCCGP_PdoHandleQueryId(DeviceObject, Irp);
            break;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            //
            // remove us from the fdo's pdo list
            //
            bFound = FALSE;
            for(Index = 0; Index < PDODeviceExtension->FDODeviceExtension->FunctionDescriptorCount; Index++)
            {
                if (PDODeviceExtension->FDODeviceExtension->ChildPDO[Index] == DeviceObject)
                {
                    //
                    // remove us
                    //
                    PDODeviceExtension->FDODeviceExtension->ChildPDO[Index] = NULL;
                    bFound = TRUE;
                    break;
                }
            }

            //
            // Complete the IRP
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            if (bFound)
            {
                //
                // Delete the device object
                //
                IoDeleteDevice(DeviceObject);
            }
            return STATUS_SUCCESS;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            //
            // copy device capabilities
            //
            RtlCopyMemory(IoStack->Parameters.DeviceCapabilities.Capabilities, &PDODeviceExtension->Capabilities, sizeof(DEVICE_CAPABILITIES));

            /* Complete the IRP */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        {
            //
            // sure
            //
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_START_DEVICE:
        {
            //
            // no-op for PDO
            //
            DPRINT("[USBCCGP] PDO IRP_MN_START\n");
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_INTERFACE:
        {
            //
            // forward to lower device object
            //
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(PDODeviceExtension->NextDeviceObject, Irp);
        }
        default:
        {
            //
            // do nothing
            //
            Status = Irp->IoStatus.Status;
            break;
        }
    }

    //
    // complete request
    //
    if (Status != STATUS_PENDING)
    {
        //
        // store result
        //
        Irp->IoStatus.Status = Status;

        //
        // complete request
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    //
    // done processing
    //
    return Status;

}

NTSTATUS
USBCCGP_BuildConfigurationDescriptor(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    ULONG TotalSize, Index;
    ULONG Size;
    PURB Urb;
    PVOID Buffer;
    PUCHAR BufferPtr;
    UCHAR InterfaceNumber;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("USBCCGP_BuildConfigurationDescriptor\n");

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get configuration descriptor
    //
    ConfigurationDescriptor = PDODeviceExtension->ConfigurationDescriptor;

    //
    // calculate size of configuration descriptor
    //
    TotalSize = sizeof(USB_CONFIGURATION_DESCRIPTOR);

    for (Index = 0; Index < PDODeviceExtension->FunctionDescriptor->NumberOfInterfaces; Index++)
    {
        //
        // get current interface descriptor
        //
        InterfaceDescriptor = PDODeviceExtension->FunctionDescriptor->InterfaceDescriptorList[Index];
        InterfaceNumber = InterfaceDescriptor->bInterfaceNumber;

        //
        // add to size and move to next descriptor
        //
        TotalSize += InterfaceDescriptor->bLength;
        InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength);

        do
        {
            if ((ULONG_PTR)InterfaceDescriptor >= ((ULONG_PTR)ConfigurationDescriptor + ConfigurationDescriptor->wTotalLength))
            {
                //
                // reached end of configuration descriptor
                //
                break;
            }

            //
            // association descriptors are removed
            //
            if (InterfaceDescriptor->bDescriptorType != USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE)
            {
                if (InterfaceDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
                {
                    if (InterfaceNumber != InterfaceDescriptor->bInterfaceNumber)
                    {
                        //
                        // reached next descriptor
                        //
                        break;
                    }

                    //
                    // include alternate descriptor
                    //
                }

                //
                // append size
                //
                TotalSize += InterfaceDescriptor->bLength;
            }

            //
            // move to next descriptor
            //
            InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength);
        } while(TRUE);
    }

    //
    // now allocate temporary buffer for the configuration descriptor
    //
    Buffer = AllocateItem(NonPagedPool, TotalSize);
    if (!Buffer)
    {
        //
        // failed to allocate buffer
        //
        DPRINT1("[USBCCGP] Failed to allocate %lu Bytes\n", TotalSize);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // first copy the configuration descriptor
    //
    RtlCopyMemory(Buffer, ConfigurationDescriptor, sizeof(USB_CONFIGURATION_DESCRIPTOR));
    BufferPtr = (PUCHAR)((ULONG_PTR)Buffer + ConfigurationDescriptor->bLength);

    for (Index = 0; Index < PDODeviceExtension->FunctionDescriptor->NumberOfInterfaces; Index++)
    {
        //
        // get current interface descriptor
        //
        InterfaceDescriptor = PDODeviceExtension->FunctionDescriptor->InterfaceDescriptorList[Index];
        InterfaceNumber = InterfaceDescriptor->bInterfaceNumber;

        //
        // copy descriptor and move to next descriptor
        //
        RtlCopyMemory(BufferPtr, InterfaceDescriptor, InterfaceDescriptor->bLength);
        BufferPtr += InterfaceDescriptor->bLength;
        InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength);

        do
        {
            if ((ULONG_PTR)InterfaceDescriptor >= ((ULONG_PTR)ConfigurationDescriptor + ConfigurationDescriptor->wTotalLength))
            {
                //
                // reached end of configuration descriptor
                //
                break;
            }

            //
            // association descriptors are removed
            //
            if (InterfaceDescriptor->bDescriptorType != USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE)
            {
                if (InterfaceDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
                {
                    if (InterfaceNumber != InterfaceDescriptor->bInterfaceNumber)
                    {
                        //
                        // reached next descriptor
                        //
                        break;
                    }

                    //
                    // include alternate descriptor
                    //
                    DPRINT("InterfaceDescriptor %p Alternate %x InterfaceNumber %x\n", InterfaceDescriptor, InterfaceDescriptor->bAlternateSetting, InterfaceDescriptor->bInterfaceNumber);
                }

                //
                // copy descriptor
                //
                RtlCopyMemory(BufferPtr, InterfaceDescriptor, InterfaceDescriptor->bLength);
                BufferPtr += InterfaceDescriptor->bLength;
            }

            //
            // move to next descriptor
            //
            InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength);
        } while(TRUE);
    }

    //
    // modify configuration descriptor
    //
    ConfigurationDescriptor = Buffer;
    ConfigurationDescriptor->wTotalLength = (USHORT)TotalSize;
    ConfigurationDescriptor->bNumInterfaces = PDODeviceExtension->FunctionDescriptor->NumberOfInterfaces;

    //
    // get urb
    //
    Urb = (PURB)IoStack->Parameters.Others.Argument1;
    ASSERT(Urb);

    //
    // copy descriptor
    //
    Size = min(TotalSize, Urb->UrbControlDescriptorRequest.TransferBufferLength);
    RtlCopyMemory(Urb->UrbControlDescriptorRequest.TransferBuffer, Buffer, Size);

    //
    // store final size
    //
    Urb->UrbControlDescriptorRequest.TransferBufferLength = Size;

    //
    // free buffer
    //
    FreeItem(Buffer);

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USBCCGP_PDOSelectConfiguration(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PURB Urb, NewUrb;
    PUSBD_INTERFACE_INFORMATION InterfaceInformation;
    ULONG InterfaceIndex, Length;
    PUSBD_INTERFACE_LIST_ENTRY Entry;
    ULONG NeedSelect, FoundInterface;
    NTSTATUS Status;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get urb
    //
    Urb = (PURB)IoStack->Parameters.Others.Argument1;
    ASSERT(Urb);

    //
    // is there already an configuration handle
    //
    if (Urb->UrbSelectConfiguration.ConfigurationHandle)
    {
        //
        // nothing to do
        //
        return STATUS_SUCCESS;
    }

    //
    // if there is no configuration descriptor, unconfigure the device
    //
    if (Urb->UrbSelectConfiguration.ConfigurationDescriptor == NULL)
    {
        return STATUS_SUCCESS;
    }

    // sanity checks
    //C_ASSERT(sizeof(struct _URB_HEADER) == 16);
    //C_ASSERT(FIELD_OFFSET(struct _URB_SELECT_CONFIGURATION, Interface.Length) == 24);
    //C_ASSERT(sizeof(USBD_INTERFACE_INFORMATION) == 36);
    //C_ASSERT(sizeof(struct _URB_SELECT_CONFIGURATION) == 0x3C);

    // available buffer length
    Length = Urb->UrbSelectConfiguration.Hdr.Length - FIELD_OFFSET(struct _URB_SELECT_CONFIGURATION, Interface.Length);

    //
    // check all interfaces
    //
    InterfaceInformation = &Urb->UrbSelectConfiguration.Interface;

    Entry = NULL;
    do
    {
        DPRINT("[USBCCGP] SelectConfiguration Function %x InterfaceNumber %x Alternative %x Length %lu InterfaceInformation->Length %lu\n", 
                PDODeviceExtension->FunctionDescriptor->FunctionNumber, InterfaceInformation->InterfaceNumber, InterfaceInformation->AlternateSetting, Length, InterfaceInformation->Length);
        ASSERT(InterfaceInformation->Length);
        //
        // search for the interface in the local interface list
        //
        FoundInterface = FALSE;
        for (InterfaceIndex = 0; InterfaceIndex < PDODeviceExtension->FunctionDescriptor->NumberOfInterfaces; InterfaceIndex++)
        {
            if (PDODeviceExtension->FunctionDescriptor->InterfaceDescriptorList[InterfaceIndex]->bInterfaceNumber == InterfaceInformation->InterfaceNumber)
            {
                // found interface entry
                FoundInterface = TRUE;
                break;
            }
        }

        if (!FoundInterface)
        {
            //
            // invalid parameter
            //
            DPRINT1("InterfaceInformation InterfaceNumber %x Alternative %x NumberOfPipes %x not found\n", InterfaceInformation->InterfaceNumber, InterfaceInformation->AlternateSetting, InterfaceInformation->NumberOfPipes);
            return STATUS_INVALID_PARAMETER;
        }

        //
        // now query the total interface list
        //
        Entry = NULL;
        for (InterfaceIndex = 0; InterfaceIndex < PDODeviceExtension->InterfaceListCount; InterfaceIndex++)
        {
            if (PDODeviceExtension->InterfaceList[InterfaceIndex].Interface->InterfaceNumber == InterfaceInformation->InterfaceNumber)
            {
                //
                // found entry
                //
                Entry = &PDODeviceExtension->InterfaceList[InterfaceIndex];
            }
        }

        //
        // sanity check
        //
        ASSERT(Entry);
        if (!Entry)
        {
            //
            // corruption detected
            //
            KeBugCheck(0);
        }

        NeedSelect = FALSE;
        if (Entry->InterfaceDescriptor->bAlternateSetting == InterfaceInformation->AlternateSetting)
        {
            for(InterfaceIndex = 0; InterfaceIndex < InterfaceInformation->NumberOfPipes; InterfaceIndex++)
            {
                if (InterfaceInformation->Pipes[InterfaceIndex].MaximumTransferSize != Entry->Interface->Pipes[InterfaceIndex].MaximumTransferSize)
                {
                    //
                    // changed interface
                    //
                    NeedSelect = TRUE;
                }
            }
        }
        else
        {
            //
            // need select as the interface number differ
            //
            NeedSelect = TRUE;
        }

        if (!NeedSelect)
        {
            //
            // interface is already selected
            //
            ASSERT(Length >= Entry->Interface->Length);
            RtlCopyMemory(InterfaceInformation, Entry->Interface, Entry->Interface->Length);

            //
            // adjust remaining buffer size
            //
            ASSERT(Entry->Interface->Length);
            Length -= Entry->Interface->Length;

            //
            // move to next output interface information
            //
            InterfaceInformation = (PUSBD_INTERFACE_INFORMATION)((ULONG_PTR)InterfaceInformation + Entry->Interface->Length);
        }
        else
        {
            //
            // select interface
            //
            DPRINT1("Selecting InterfaceIndex %lu AlternateSetting %lu NumberOfPipes %lu\n", InterfaceInformation->InterfaceNumber, InterfaceInformation->AlternateSetting, InterfaceInformation->NumberOfPipes);
            ASSERT(InterfaceInformation->Length == Entry->Interface->Length);

            //
            // build urb
            //
            NewUrb = AllocateItem(NonPagedPool, GET_SELECT_INTERFACE_REQUEST_SIZE(InterfaceInformation->NumberOfPipes));
            if (!NewUrb)
            {
                //
                // no memory
                //
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // now prepare interface urb
            //
            UsbBuildSelectInterfaceRequest(NewUrb, (USHORT)GET_SELECT_INTERFACE_REQUEST_SIZE(InterfaceInformation->NumberOfPipes), PDODeviceExtension->ConfigurationHandle, InterfaceInformation->InterfaceNumber, InterfaceInformation->AlternateSetting);

            //
            // now select the interface
            //
            Status = USBCCGP_SyncUrbRequest(PDODeviceExtension->NextDeviceObject, NewUrb);
            DPRINT1("SelectInterface Status %x\n", Status);

            if (!NT_SUCCESS(Status))
            {
                 //
                 // failed
                 //
                 break;
            }

            //
            // update configuration info
            //
            ASSERT(Entry->Interface->Length >= NewUrb->UrbSelectInterface.Interface.Length);
            ASSERT(Length >= NewUrb->UrbSelectInterface.Interface.Length);
            RtlCopyMemory(Entry->Interface, &NewUrb->UrbSelectInterface.Interface, NewUrb->UrbSelectInterface.Interface.Length);

            //
            // update provided interface information
            //
            ASSERT(Length >= Entry->Interface->Length);
            RtlCopyMemory(InterfaceInformation, Entry->Interface, Entry->Interface->Length);

            //
            // decrement remaining buffer size
            //
            ASSERT(Entry->Interface->Length);
            Length -= Entry->Interface->Length;

            //
            // adjust output buffer offset
            //
            InterfaceInformation = (PUSBD_INTERFACE_INFORMATION)((ULONG_PTR)InterfaceInformation + Entry->Interface->Length);

            //
            // free urb
            //
            FreeItem(NewUrb);
        }

    } while(Length);

    //
    // store configuration handle
    //
    Urb->UrbSelectConfiguration.ConfigurationHandle = PDODeviceExtension->ConfigurationHandle;

    DPRINT1("[USBCCGP] SelectConfiguration Function %x Completed\n", PDODeviceExtension->FunctionDescriptor->FunctionNumber);

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PDO_HandleInternalDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    NTSTATUS Status;
    PURB Urb;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB)
    {
        //
        // get urb
        //
        Urb = (PURB)IoStack->Parameters.Others.Argument1;
        ASSERT(Urb);
        DPRINT("IOCTL_INTERNAL_USB_SUBMIT_URB Function %x\n", Urb->UrbHeader.Function);

        if (Urb->UrbHeader.Function == URB_FUNCTION_SELECT_CONFIGURATION)
        {
            //
            // select configuration
            //
            Status = USBCCGP_PDOSelectConfiguration(DeviceObject, Irp);
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        else if (Urb->UrbHeader.Function == URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE)
        {
            if (Urb->UrbControlDescriptorRequest.DescriptorType == USB_DEVICE_DESCRIPTOR_TYPE)
            {
                //
                // is the buffer big enough
                //
                if (Urb->UrbControlDescriptorRequest.TransferBufferLength < sizeof(USB_DEVICE_DESCRIPTOR))
                {
                    //
                    // invalid buffer size
                    //
                    DPRINT1("[USBCCGP] invalid device descriptor size %lu\n", Urb->UrbControlDescriptorRequest.TransferBufferLength);
                    Urb->UrbControlDescriptorRequest.TransferBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);
                    Irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_INVALID_BUFFER_SIZE;
                }

                //
                // copy device descriptor
                //
                ASSERT(Urb->UrbControlDescriptorRequest.TransferBuffer);
                RtlCopyMemory(Urb->UrbControlDescriptorRequest.TransferBuffer, &PDODeviceExtension->DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_SUCCESS;
            }
            else if (Urb->UrbControlDescriptorRequest.DescriptorType == USB_CONFIGURATION_DESCRIPTOR_TYPE)
            {
                //
                // build configuration descriptor
                //
                Status = USBCCGP_BuildConfigurationDescriptor(DeviceObject, Irp);
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }
            else if (Urb->UrbControlDescriptorRequest.DescriptorType == USB_STRING_DESCRIPTOR_TYPE)
            {
                PUSB_STRING_DESCRIPTOR StringDescriptor;

                //
                // get the requested string descriptor
                //
                ASSERT(Urb->UrbControlDescriptorRequest.TransferBuffer);
                Status = USBCCGP_GetDescriptor(PDODeviceExtension->FDODeviceExtension->NextDeviceObject,
                                               USB_STRING_DESCRIPTOR_TYPE,
                                               Urb->UrbControlDescriptorRequest.TransferBufferLength,
                                               Urb->UrbControlDescriptorRequest.Index,
                                               Urb->UrbControlDescriptorRequest.LanguageId,
                                               (PVOID*)&StringDescriptor);
                if (NT_SUCCESS(Status))
                {
                    if (StringDescriptor->bLength == 2)
                    {
                        FreeItem(StringDescriptor);
                        Status = STATUS_DEVICE_DATA_ERROR;
                    }
                    else
                    {
                        RtlCopyMemory(Urb->UrbControlDescriptorRequest.TransferBuffer,
                                      StringDescriptor->bString,
                                      StringDescriptor->bLength + sizeof(WCHAR));
                        FreeItem(StringDescriptor);
                        Status = STATUS_SUCCESS;
                    }
                }
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }
        }
        else
        {
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(PDODeviceExtension->NextDeviceObject, Irp);
            return Status;
        }
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_GET_PORT_STATUS)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(PDODeviceExtension->NextDeviceObject, Irp);
        return Status;
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_RESET_PORT)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(PDODeviceExtension->NextDeviceObject, Irp);
        return Status;
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_CYCLE_PORT)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(PDODeviceExtension->NextDeviceObject, Irp);
        return Status;
    }

    DPRINT1("IOCTL %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
    DPRINT1("InputBufferLength %lu\n", IoStack->Parameters.DeviceIoControl.InputBufferLength);
    DPRINT1("OutputBufferLength %lu\n", IoStack->Parameters.DeviceIoControl.OutputBufferLength);
    DPRINT1("Type3InputBuffer %p\n", IoStack->Parameters.DeviceIoControl.Type3InputBuffer);

    ASSERT(FALSE);

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
PDO_HandlePower(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->MinorFunction)
    {
        case IRP_MN_SET_POWER:
        case IRP_MN_QUERY_POWER:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
    }

    Status = Irp->IoStatus.Status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


NTSTATUS
PDO_Dispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IoStack->MajorFunction)
    {
        case IRP_MJ_PNP:
            return PDO_HandlePnp(DeviceObject, Irp);
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            return PDO_HandleInternalDeviceControl(DeviceObject, Irp);
        case IRP_MJ_POWER:
            return PDO_HandlePower(DeviceObject, Irp);
        default:
            DPRINT1("PDO_Dispatch Function %x not implemented\n", IoStack->MajorFunction);
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
    }
}
