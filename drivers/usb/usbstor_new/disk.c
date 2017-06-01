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

#define NDEBUG
#include <debug.h>

BOOLEAN
NTAPI
IsRequestValid(PIRP Irp)
{
    ULONG TransferLength;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Srb;

    DPRINT("IsRequestValid: ... \n");

    IoStack = Irp->Tail.Overlay.CurrentStackLocation;
    Srb = IoStack->Parameters.Scsi.Srb;

    if (Srb->SrbFlags & (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT))
    {
        if ((Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) ==
                             SRB_FLAGS_UNSPECIFIED_DIRECTION)
        {
            DPRINT("IsRequestValid: Not valid Srb! Srb->SrbFlags - %p\n", Srb->SrbFlags);
            return FALSE;
        }

        TransferLength = Srb->DataTransferLength;

        if (Irp->MdlAddress == NULL)
        {
            DPRINT("IsRequestValid: Not valid Srb. Irp->MdlAddress == NULL\n");
            return FALSE;
        }

        if (TransferLength == 0)
        {
            DPRINT("IsRequestValid: Not valid Srb. TransferLength == 0\n");
            return FALSE;
        }

        if (TransferLength > 0x10000) // FIXME consatnt (default MaximumTransferLength)
        {
            DPRINT("IsRequestValid: Not valid Srb. TransferLength > 0x10000\n");
            return FALSE;
        }
    }

    if (Srb->DataTransferLength)
    {
        DPRINT("IsRequestValid: Not valid Srb. Srb->DataTransferLength != 0\n");
        return FALSE;
    }

    if (Srb->DataBuffer)
    {
        DPRINT("IsRequestValid: Not valid Srb. Srb->DataBuffer != NULL\n");
        return FALSE;
    }

    if (Irp->MdlAddress)
    {
        DPRINT("IsRequestValid: Not valid Srb. Irp->MdlAddress != NULL\n");
        return FALSE;
    }

    return TRUE;
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
    Request = IoStack->Parameters.Scsi.Srb;

    //
    // sanity check
    //
    ASSERT(Request);

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    switch(Request->Function)
    {
        case SRB_FUNCTION_EXECUTE_SCSI:
        {
            DPRINT("SRB_FUNCTION_EXECUTE_SCSI\n");

            if (!IsRequestValid(Irp))
            {
                DPRINT1("USBSTOR_HandleInternalDeviceControl: Bad Srb!\n");
                Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            if (Request->Cdb[0] == SCSIOP_MODE_SENSE)
            {
                DPRINT("USBSTOR_Scsi: SRB_FUNCTION_EXECUTE_SCSI - FIXME SCSIOP_MODE_SENSE\n");
                // FIXME Get from registry WriteProtect for StorageDevicePolicies;
                // L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\StorageDevicePolicies"
                // QueryTable[0].Name = L"WriteProtect"
            }

            IoStack->Parameters.Others.Argument2 = PDODeviceExtension;

            // mark irp pending
            IoMarkIrpPending(Irp);
            Request->SrbStatus = SRB_STATUS_PENDING;

            //
            // add the request
            //
            if (!USBSTOR_QueueAddIrp(PDODeviceExtension->LowerDeviceObject, Irp))
            {
                //
                // irp was not added to the queue
                //
                IoStartPacket(PDODeviceExtension->LowerDeviceObject, Irp, &Request->QueueSortKey, USBSTOR_CancelIo);
            }

            //
            // irp pending
            //
            return STATUS_PENDING;
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
            PDODeviceExtension->Claimed = FALSE;
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
            DPRINT1("SRB_FUNCTION_RELEASE_QUEUE\n");

            //
            // release queue
            //
            USBSTOR_QueueRelease(PDODeviceExtension->LowerDeviceObject);

            //
            // set status success
            //
            Request->SrbStatus = SRB_STATUS_SUCCESS;
            Status = STATUS_SUCCESS;
            break;
        }

        case SRB_FUNCTION_SHUTDOWN:
        case SRB_FUNCTION_FLUSH:
        case SRB_FUNCTION_FLUSH_QUEUE:
        {
            DPRINT1("SRB_FUNCTION_FLUSH / SRB_FUNCTION_FLUSH_QUEUE / SRB_FUNCTION_SHUTDOWN\n");

            // HACK: don't flush pending requests
#if 0       // we really need a proper storage stack
            //
            // wait for pending requests to finish
            //
            USBSTOR_QueueWaitForPendingRequests(PDODeviceExtension->LowerDeviceObject);
#endif
            //
            // set status success
            //
            Request->SrbStatus = SRB_STATUS_SUCCESS;
            Status = STATUS_SUCCESS;
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

    //
    // complete request
    //
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
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
    // scan the field and return last position which contains a valid character
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
    ULONG FieldLengthVendor, FieldLengthProduct, FieldLengthRevision, TotalLength, FieldLengthSerialNumber;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PUFI_INQUIRY_RESPONSE InquiryData;
    PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;
    PUCHAR Buffer;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    UNICODE_STRING SerialNumber;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    DPRINT("USBSTOR_HandleQueryProperty\n");

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
        DPRINT("USBSTOR_HandleQueryProperty StorageDeviceProperty OutputBufferLength %lu\n", IoStack->Parameters.DeviceIoControl.OutputBufferLength);

        //
        // get device extension
        //
        PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
        ASSERT(PDODeviceExtension);
        ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

        //
        // get device extension
        //
        FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;
        ASSERT(FDODeviceExtension);
        ASSERT(FDODeviceExtension->Common.IsFDO);

        //
        // get inquiry data
        //
        InquiryData = &PDODeviceExtension->InquiryData;
        ASSERT(InquiryData);

        //
        // compute extra parameters length
        //
        FieldLengthVendor = USBSTOR_GetFieldLength(InquiryData->Vendor, 8);
        FieldLengthProduct = USBSTOR_GetFieldLength(InquiryData->Product, 16);
        FieldLengthRevision = USBSTOR_GetFieldLength(InquiryData->Revision, 4);

        //
        // is there a serial number
        //
        if (FDODeviceExtension->SerialNumber)
        {
            //
            // get length
            //
            FieldLengthSerialNumber = wcslen(FDODeviceExtension->SerialNumber->bString);
        }
        else
        {
            //
            // no serial number
            //
            FieldLengthSerialNumber = 0;
        }

        //
        // total length required is sizeof(STORAGE_DEVICE_DESCRIPTOR) + FieldLength + 4 extra null bytes - 1
        // -1 due STORAGE_DEVICE_DESCRIPTOR contains one byte length of parameter data
        //
        TotalLength = sizeof(STORAGE_DEVICE_DESCRIPTOR) + FieldLengthVendor + FieldLengthProduct + FieldLengthRevision + FieldLengthSerialNumber + 3;

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
        DeviceDescriptor->RemovableMedia = (InquiryData->RMB & 0x80) ? TRUE : FALSE;
        DeviceDescriptor->CommandQueueing = FALSE;
        DeviceDescriptor->BusType = BusTypeUsb;
        DeviceDescriptor->VendorIdOffset = sizeof(STORAGE_DEVICE_DESCRIPTOR) - sizeof(UCHAR);
        DeviceDescriptor->ProductIdOffset = DeviceDescriptor->VendorIdOffset + FieldLengthVendor + 1;
        DeviceDescriptor->ProductRevisionOffset = DeviceDescriptor->ProductIdOffset + FieldLengthProduct + 1;
        DeviceDescriptor->SerialNumberOffset = (FieldLengthSerialNumber > 0 ? DeviceDescriptor->ProductRevisionOffset + FieldLengthRevision + 1 : 0);
        DeviceDescriptor->RawPropertiesLength = FieldLengthVendor + FieldLengthProduct + FieldLengthRevision + FieldLengthSerialNumber + 3 + (FieldLengthSerialNumber > 0 ? + 1 : 0);

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
        // copy serial number
        //
        if (FieldLengthSerialNumber)
        {
            //
            // init unicode string
            //
            RtlInitUnicodeString(&SerialNumber, FDODeviceExtension->SerialNumber->bString);

            //
            // init ansi string
            //
            AnsiString.Buffer = (PCHAR)Buffer;
            AnsiString.Length = 0;
            AnsiString.MaximumLength = FieldLengthSerialNumber * sizeof(WCHAR);

            //
            // convert to ansi code
            //
            Status = RtlUnicodeStringToAnsiString(&AnsiString, &SerialNumber, FALSE);
            ASSERT(Status == STATUS_SUCCESS);
        }


        DPRINT("Vendor %s\n", (LPCSTR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->VendorIdOffset));
        DPRINT("Product %s\n", (LPCSTR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->ProductIdOffset));
        DPRINT("Revision %s\n", (LPCSTR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->ProductRevisionOffset));
        DPRINT("Serial %s\n", (LPCSTR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->SerialNumberOffset));

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
        DPRINT("USBSTOR_HandleQueryProperty StorageAdapterProperty OutputBufferLength %lu\n", IoStack->Parameters.DeviceIoControl.OutputBufferLength);

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
        AdapterDescriptor->MaximumTransferLength = 0x10000; // FIXME
        AdapterDescriptor->MaximumPhysicalPages = 17; // See CORE-10515 and CORE-10755
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
    NTSTATUS Status;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PSCSI_ADAPTER_BUS_INFO BusInfo;
    PSCSI_INQUIRY_DATA InquiryData;
    PINQUIRYDATA ScsiInquiryData;
    PUFI_INQUIRY_RESPONSE UFIInquiryResponse;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY)
    {
        //
        // query property
        //
        Status = USBSTOR_HandleQueryProperty(DeviceObject, Irp);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH)
    {
        //
        // query scsi pass through
        //
        DPRINT1("USBSTOR_HandleDeviceControl IOCTL_SCSI_PASS_THROUGH NOT implemented\n");
        Status = STATUS_NOT_SUPPORTED;
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT)
    {
        //
        // query scsi pass through direct
        //
        DPRINT1("USBSTOR_HandleDeviceControl IOCTL_SCSI_PASS_THROUGH_DIRECT NOT implemented\n");
        Status = STATUS_NOT_SUPPORTED;
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER)
    {
        //
        // query serial number
        //
        DPRINT1("USBSTOR_HandleDeviceControl IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER NOT implemented\n");
        Status = STATUS_NOT_SUPPORTED;
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_GET_CAPABILITIES)
    {
        PIO_SCSI_CAPABILITIES Capabilities;

        /* Legacy port capability query */
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(PVOID))
        {
            Capabilities = *((PVOID *)Irp->AssociatedIrp.SystemBuffer) = ExAllocatePoolWithTag(NonPagedPool,
                                                                                               sizeof(IO_SCSI_CAPABILITIES),
                                                                                               USB_STOR_TAG);
            Irp->IoStatus.Information = sizeof(PVOID);
        }
        else
        {
            Capabilities = Irp->AssociatedIrp.SystemBuffer;
            Irp->IoStatus.Information = sizeof(IO_SCSI_CAPABILITIES);
        }

        if (Capabilities)
        {
            Capabilities->MaximumTransferLength = 0x10000; // FIXME 
            Capabilities->MaximumPhysicalPages = 17; // See CORE-10515 and CORE-10755
            Capabilities->SupportedAsynchronousEvents = 0;
            Capabilities->AlignmentMask = 0;
            Capabilities->TaggedQueuing = FALSE;
            Capabilities->AdapterScansDown = FALSE;
            Capabilities->AdapterUsesPio = FALSE;
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } 
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_GET_INQUIRY_DATA)
    {
        //
        // get device extension
        //
        PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
        ASSERT(PDODeviceExtension);
        ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

        //
        // get parameters
        //
        BusInfo = Irp->AssociatedIrp.SystemBuffer;
        InquiryData = (PSCSI_INQUIRY_DATA)(BusInfo + 1);
        ScsiInquiryData = (PINQUIRYDATA)InquiryData->InquiryData;
        

        //
        // get inquiry data
        //
        UFIInquiryResponse = &PDODeviceExtension->InquiryData;
        ASSERT(UFIInquiryResponse);


        BusInfo->NumberOfBuses = 1;
        BusInfo->BusData[0].NumberOfLogicalUnits = 1; //FIXME
        BusInfo->BusData[0].InitiatorBusId = 0;
        BusInfo->BusData[0].InquiryDataOffset = sizeof(SCSI_ADAPTER_BUS_INFO);

        InquiryData->PathId = 0;
        InquiryData->TargetId = 0;
        InquiryData->Lun = PDODeviceExtension->LUN & MAX_LUN;
        InquiryData->DeviceClaimed = PDODeviceExtension->Claimed;
        InquiryData->InquiryDataLength = sizeof(INQUIRYDATA);
        InquiryData->NextInquiryDataOffset = 0;

        RtlZeroMemory(ScsiInquiryData, sizeof(INQUIRYDATA));
        ScsiInquiryData->DeviceType = UFIInquiryResponse->DeviceType;
        ScsiInquiryData->DeviceTypeQualifier = (UFIInquiryResponse->RMB & 0x7F);

        /* Hack for IoReadPartitionTable call in disk.sys */
        ScsiInquiryData->RemovableMedia = ((ScsiInquiryData->DeviceType != DIRECT_ACCESS_DEVICE) ? ((UFIInquiryResponse->RMB & 0x80) ? 1 : 0) : 0);
        // should be:
        //ScsiInquiryData->RemovableMedia = ((ScsiInquiryData->DeviceType == DIRECT_ACCESS_DEVICE) ? ((UFIInquiryResponse->RMB & 0x80) ? 1 : 0) : 0);

        ScsiInquiryData->Versions = 0x04;
        ScsiInquiryData->ResponseDataFormat = 0x02;
        ScsiInquiryData->AdditionalLength = 31;
        ScsiInquiryData->SoftReset = 0;
        ScsiInquiryData->CommandQueue = 0;
        ScsiInquiryData->LinkedCommands = 0;
        ScsiInquiryData->RelativeAddressing = 0;

        RtlCopyMemory(&ScsiInquiryData->VendorId, UFIInquiryResponse->Vendor, USBSTOR_GetFieldLength(UFIInquiryResponse->Vendor, 8));
        RtlCopyMemory(&ScsiInquiryData->ProductId, UFIInquiryResponse->Product, USBSTOR_GetFieldLength(UFIInquiryResponse->Product, 16));

        Irp->IoStatus.Information = sizeof(SCSI_ADAPTER_BUS_INFO) + sizeof(SCSI_INQUIRY_DATA) + sizeof(INQUIRYDATA) - 1;
        Status = STATUS_SUCCESS;
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_GET_ADDRESS)
    {
        PSCSI_ADDRESS Address = Irp->AssociatedIrp.SystemBuffer;

        Address->Length = sizeof(SCSI_ADDRESS);
        Address->PortNumber = 0;
        Address->PathId = 0;
        Address->TargetId = 0;
        Address->Lun = (((PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LUN & MAX_LUN);
        Irp->IoStatus.Information = sizeof(SCSI_ADDRESS);

        Status = STATUS_SUCCESS;
    }
    else
    {
        //
        // unsupported
        //
        DPRINT("USBSTOR_HandleDeviceControl IoControl %x not supported\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
        Status = STATUS_NOT_SUPPORTED;
    }

    return Status;
}
