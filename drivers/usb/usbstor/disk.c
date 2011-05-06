/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/disk.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

NTSTATUS
USBSTOR_HandleExecuteSCSI(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PSCSI_REQUEST_BLOCK Request,
    IN PPDO_DEVICE_EXTENSION PDODeviceExtension)
{
    DPRINT1("USBSTOR_HandleExecuteSCSI\n");

    DbgBreakPoint();

    Request->SrbStatus = SRB_STATUS_ERROR;
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USBSTOR_HandleInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    NTSTATUS Status;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get request block
    //
    Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(Request);
    ASSERT(PDODeviceExtension);

    switch(Request->Function)
    {
        case SRB_FUNCTION_EXECUTE_SCSI:
        {
            DPRINT1("SRB_FUNCTION_EXECUTE_SCSI\n");
            Status = USBSTOR_HandleExecuteSCSI(DeviceObject, Irp, Request, PDODeviceExtension);

        }
        case SRB_FUNCTION_RELEASE_DEVICE:
        {
            DPRINT1("SRB_FUNCTION_RELEASE_DEVICE\n");
            //
            // sanity check
            //
            ASSERT(PDODeviceExtension->Claimed == TRUE);

            //
            // release claim
            //
            PDODeviceExtension->Claimed = TRUE;
            Status = STATUS_SUCCESS;
            break;
        }
        case SRB_FUNCTION_CLAIM_DEVICE:
        {
            DPRINT1("SRB_FUNCTION_CLAIM_DEVICE\n");
            //
            // check if the device has been claimed
            //
            if (PDODeviceExtension->Claimed)
            {
                //
                // device has already been claimed
                //
                Status = STATUS_DEVICE_BUSY;
                Request->SrbStatus = SRB_STATUS_BUSY;
                break;
            }

            //
            // claim device
            //
            PDODeviceExtension->Claimed = TRUE;

            //
            // output device object
            //
            Request->DataBuffer = DeviceObject;

            //
            // completed successfully
            //
            Status = STATUS_SUCCESS;
            break;
        }
        case SRB_FUNCTION_RELEASE_QUEUE:
        {
            DPRINT1("SRB_FUNCTION_RELEASE_QUEUE UNIMPLEMENTED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        case SRB_FUNCTION_FLUSH:
        {
            DPRINT1("SRB_FUNCTION_FLUSH UNIMPLEMENTED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        case SRB_FUNCTION_SET_LINK_TIMEOUT:
        {
            DPRINT1("SRB_FUNCTION_FLUSH UNIMPLEMENTED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        default:
        {
            //
            // not supported
            //
            Status = STATUS_NOT_SUPPORTED;
            Request->SrbStatus = SRB_STATUS_ERROR;
        }
    }

    return Status;
}

ULONG
USBSTOR_GetFieldLength(
    IN PUCHAR Name,
    IN ULONG MaxLength)
{
    ULONG Index;
    ULONG LastCharacterPosition = 0;

    //
    // scan the field and return last positon which contains a valid character
    //
    for(Index = 0; Index < MaxLength; Index++)
    {
        if (Name[Index] != ' ')
        {
            //
            // trim white spaces from field
            //
            LastCharacterPosition = Index;
        }
    }

    //
    // convert from zero based index to length
    //
    return LastCharacterPosition + 1;
}

NTSTATUS
USBSTOR_HandleQueryProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PSTORAGE_PROPERTY_QUERY PropertyQuery;
    PSTORAGE_DESCRIPTOR_HEADER DescriptorHeader;
    PSTORAGE_ADAPTER_DESCRIPTOR AdapterDescriptor;
    ULONG FieldLengthVendor, FieldLengthProduct, FieldLengthRevision, TotalLength;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PUFI_INQUIRY_RESPONSE InquiryData;
    PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;
    PUCHAR Buffer;

    DPRINT1("USBSTOR_HandleQueryProperty\n");

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // sanity check
    //
    ASSERT(IoStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(STORAGE_PROPERTY_QUERY));
    ASSERT(Irp->AssociatedIrp.SystemBuffer);

    //
    // get property query
    //
    PropertyQuery = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;

    //
    // check property type
    //
    if (PropertyQuery->PropertyId != StorageDeviceProperty &&
        PropertyQuery->PropertyId != StorageAdapterProperty)
    {
        //
        // only device property / adapter property are supported
        //
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // check query type
    //
    if (PropertyQuery->QueryType == PropertyExistsQuery)
    {
        //
        // device property / adapter property is supported
        //
        return STATUS_SUCCESS;
    }

    if (PropertyQuery->QueryType != PropertyStandardQuery)
    {
        //
        // only standard query and exists query are supported
        //
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // check if it is a device property
    //
    if (PropertyQuery->PropertyId == StorageDeviceProperty)
    {
        DPRINT1("USBSTOR_HandleQueryProperty StorageDeviceProperty OutputBufferLength %lu\n", IoStack->Parameters.DeviceIoControl.OutputBufferLength);

        //
        // get device extension
        //
        PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
        ASSERT(PDODeviceExtension);

        //
        // get inquiry data
        //
        InquiryData = (PUFI_INQUIRY_RESPONSE)PDODeviceExtension->InquiryData;
        ASSERT(InquiryData);

        //
        // compute extra parameters length
        //
        FieldLengthVendor = USBSTOR_GetFieldLength(InquiryData->Vendor, 8);
        FieldLengthProduct = USBSTOR_GetFieldLength(InquiryData->Product, 16);
        FieldLengthRevision = USBSTOR_GetFieldLength(InquiryData->Revision, 4);

        //
        // FIXME handle serial number
        //

        //
        // total length required is sizeof(STORAGE_DEVICE_DESCRIPTOR) + FieldLength + 3 extra null bytes - 1
        // -1 due STORAGE_DEVICE_DESCRIPTOR contains one byte length of parameter data
        //
        TotalLength = sizeof(STORAGE_DEVICE_DESCRIPTOR) + FieldLengthVendor + FieldLengthProduct + FieldLengthRevision + 2;

        //
        // check if output buffer is long enough
        //
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < TotalLength)
        {
            //
            // buffer too small
            //
            DescriptorHeader = (PSTORAGE_DESCRIPTOR_HEADER)Irp->AssociatedIrp.SystemBuffer;
            ASSERT(IoStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(STORAGE_DESCRIPTOR_HEADER));

            //
            // return required size
            //
            DescriptorHeader->Version = TotalLength;
            DescriptorHeader->Size = TotalLength;

            Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            return STATUS_SUCCESS;
        }

        //
        // get device descriptor
        //
        DeviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;

        //
        // initialize device descriptor
        //
        DeviceDescriptor->Version = TotalLength;
        DeviceDescriptor->Size = TotalLength;
        DeviceDescriptor->DeviceType = InquiryData->DeviceType;
        DeviceDescriptor->DeviceTypeModifier = (InquiryData->RMB & 0x7F);
        DeviceDescriptor->RemovableMedia = FALSE; //FIXME check if floppy
        DeviceDescriptor->CommandQueueing = FALSE;
        DeviceDescriptor->BusType = BusTypeUsb;
        DeviceDescriptor->VendorIdOffset = sizeof(STORAGE_DEVICE_DESCRIPTOR) - sizeof(UCHAR);
        DeviceDescriptor->ProductIdOffset = DeviceDescriptor->VendorIdOffset + FieldLengthVendor + 1;
        DeviceDescriptor->ProductRevisionOffset = DeviceDescriptor->ProductIdOffset + FieldLengthProduct + 1;
        DeviceDescriptor->SerialNumberOffset = 0; //FIXME
        DeviceDescriptor->RawPropertiesLength = FieldLengthVendor + FieldLengthProduct + FieldLengthRevision + 3;

        //
        // copy descriptors
        //
        Buffer = (PUCHAR)((ULONG_PTR)DeviceDescriptor + sizeof(STORAGE_DEVICE_DESCRIPTOR) - sizeof(UCHAR));

        //
        // copy vendor
        //
        RtlCopyMemory(Buffer, InquiryData->Vendor, FieldLengthVendor);
        Buffer[FieldLengthVendor] = '\0';
        Buffer += FieldLengthVendor + 1;

        //
        // copy product
        //
        RtlCopyMemory(Buffer, InquiryData->Product, FieldLengthProduct);
        Buffer[FieldLengthProduct] = '\0';
        Buffer += FieldLengthProduct + 1;

        //
        // copy revision
        //
        RtlCopyMemory(Buffer, InquiryData->Revision, FieldLengthRevision);
        Buffer[FieldLengthRevision] = '\0';
        Buffer += FieldLengthRevision + 1;

        //
        // TODO: copy revision
        //

        DPRINT("Vendor %s\n", (LPCSTR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->VendorIdOffset));
        DPRINT("Product %s\n", (LPCSTR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->ProductIdOffset));
        DPRINT("Revision %s\n", (LPCSTR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->ProductRevisionOffset));

        //
        // done
        //
        Irp->IoStatus.Information = TotalLength;
        return STATUS_SUCCESS;
    }
    else
    {
        //
        // adapter property query request
        //
        DPRINT1("USBSTOR_HandleQueryProperty StorageAdapterProperty OutputBufferLength %lu\n", IoStack->Parameters.DeviceIoControl.OutputBufferLength);

        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_ADAPTER_DESCRIPTOR))
        {
            //
            // buffer too small
            //
            DescriptorHeader = (PSTORAGE_DESCRIPTOR_HEADER)Irp->AssociatedIrp.SystemBuffer;
            ASSERT(IoStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(STORAGE_DESCRIPTOR_HEADER));

            //
            // return required size
            //
            DescriptorHeader->Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
            DescriptorHeader->Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR);

            Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            return STATUS_SUCCESS;
        }

        //
        // get adapter descriptor, information is returned in the same buffer
        //
        AdapterDescriptor = (PSTORAGE_ADAPTER_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;

        //
        // fill out descriptor
        //
        AdapterDescriptor->Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
        AdapterDescriptor->Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
        AdapterDescriptor->MaximumTransferLength = MAXULONG; //FIXME compute some sane value
        AdapterDescriptor->MaximumPhysicalPages = 25; //FIXME compute some sane value
        AdapterDescriptor->AlignmentMask = 0;
        AdapterDescriptor->AdapterUsesPio = FALSE;
        AdapterDescriptor->AdapterScansDown = FALSE;
        AdapterDescriptor->CommandQueueing = FALSE;
        AdapterDescriptor->AcceleratedTransfer = FALSE;
        AdapterDescriptor->BusType = BusTypeUsb;
        AdapterDescriptor->BusMajorVersion = 0x2; //FIXME verify
        AdapterDescriptor->BusMinorVersion = 0x00; //FIXME

        //
        // store returned length
        //
        Irp->IoStatus.Information = sizeof(STORAGE_ADAPTER_DESCRIPTOR);

        //
        // done
        //
        return STATUS_SUCCESS;
    }
}



NTSTATUS
USBSTOR_HandleDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY)
    {
        //
        // query property
        //
        return USBSTOR_HandleQueryProperty(DeviceObject, Irp);
    }

    DPRINT1("USBSTOR_HandleDeviceControl IoControl %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
    DPRINT1("USBSTOR_HandleDeviceControl InputBufferLength %x\n", IoStack->Parameters.DeviceIoControl.InputBufferLength);
    DPRINT1("USBSTOR_HandleDeviceControl OutputBufferLength %x\n", IoStack->Parameters.DeviceIoControl.OutputBufferLength);
    DPRINT1("USBSTOR_HandleDeviceControl InputBuffer %x\n", IoStack->Parameters.DeviceIoControl.Type3InputBuffer);
    DPRINT1("USBSTOR_HandleDeviceControl SystemBuffer %x\n", Irp->AssociatedIrp.SystemBuffer);
    DPRINT1("USBSTOR_HandleDeviceControl UserBuffer %x\n", Irp->UserBuffer);
    DPRINT1("USBSTOR_HandleDeviceControl MdlAddress %x\n", Irp->MdlAddress);


    return STATUS_NOT_SUPPORTED;
}
