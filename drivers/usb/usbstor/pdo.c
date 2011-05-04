/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/pdo.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

LPCSTR
USBSTOR_GetDeviceType(
    IN PUFI_INQUIRY_RESPONSE InquiryData)
{
    //
    // check if device type is zero
    //
    if (InquiryData->DeviceType == 0)
    {
        //
        // direct access device
        //

        //
        // FIXME: check if floppy
        //
        return "Disk";
    }

    //
    // FIXME: use constant - derrived from http://en.wikipedia.org/wiki/SCSI_Peripheral_Device_Type
    // 
    switch (InquiryData->DeviceType)
    {
        case 1:
        {
            //
            // sequential device, i.e magnetic tape
            //
            return "Sequential";
        }
        case 4:
        {
            //
            // write once device
            //
            return "Worm";
        }
        case 5:
        {
            //
            // CDROM device
            //
            return "CdRom";
        }
        case 7:
        {
            //
            // optical memory device
            //
            return "Optical";
        }
        case 8:
        {
            //
            // medium change device
            //
            return "Changer";
        }
        default:
        {
            //
            // other device
            //
            return "Other";
        }
    }
}

LPCWSTR
USBSTOR_GetGenericType(
    IN PUFI_INQUIRY_RESPONSE InquiryData)
{
    //
    // check if device type is zero
    //
    if (InquiryData->DeviceType == 0)
    {
        //
        // direct access device
        //

        //
        // FIXME: check if floppy
        //
        return L"GenDisk";
    }

    //
    // FIXME: use constant - derrived from http://en.wikipedia.org/wiki/SCSI_Peripheral_Device_Type
    // 
    switch (InquiryData->DeviceType)
    {
        case 1:
        {
            //
            // sequential device, i.e magnetic tape
            //
            return L"GenSequential";
        }
        case 4:
        {
            //
            // write once device
            //
            return L"GenWorm";
        }
        case 5:
        {
            //
            // CDROM device
            //
            return L"GenCdRom";
        }
        case 7:
        {
            //
            // optical memory device
            //
            return L"GenOptical";
        }
        case 8:
        {
            //
            // medium change device
            //
            return L"GenChanger";
        }
        default:
        {
            //
            // other device
            //
            return L"UsbstorOther";
        }
    }
}


ULONG
CopyField(
    IN PUCHAR Name,
    IN PUCHAR Buffer,
    IN ULONG MaxLength)
{
    ULONG Index;

    for(Index = 0; Index < MaxLength; Index++)
    {
        if (Name[Index] <= ' ' || Name[Index] >= 0x7F /* last printable ascii character */ ||  Name[Index] == ',')
        {
            //
            // convert to underscore
            //
            Buffer[Index] = '_';
        }
        else
        {
            //
            // just copy character
            //
            Buffer[Index] = Name[Index];
        }
    }

    return MaxLength;
}

NTSTATUS
USBSTOR_PdoHandleQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    //PPDO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStack;
    LPWSTR Buffer;
    static WCHAR DeviceText[] = L"USB Mass Storage Device";

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription)
    {
        DPRINT1("USBSTOR_PdoHandleQueryDeviceText DeviceTextDescription\n");

        //
        // allocate item
        //
        Buffer = (LPWSTR)AllocateItem(PagedPool, sizeof(DeviceText));
        if (!Buffer)
        {
            //
            // no memory
            //
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // copy buffer
        //
        wcscpy(Buffer, DeviceText);

        //
        // save result
        //
        Irp->IoStatus.Information = (ULONG_PTR)Buffer;
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("USBSTOR_PdoHandleQueryDeviceText DeviceTextLocationInformation\n");

        //
        // allocate item
        //
        Buffer = (LPWSTR)AllocateItem(PagedPool, sizeof(DeviceText));
        if (!Buffer)
        {
            //
            // no memory
            //
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // copy buffer
        //
        wcscpy(Buffer, DeviceText);

        //
        // save result
        //
        Irp->IoStatus.Information = (ULONG_PTR)Buffer;
        return STATUS_SUCCESS;
    }

}




NTSTATUS
USBSTOR_PdoHandleQueryDeviceId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    UCHAR Buffer[100];
    LPCSTR DeviceType;
    ULONG Offset = 0;
    PUFI_INQUIRY_RESPONSE InquiryData;
    ANSI_STRING AnsiString;
    UNICODE_STRING DeviceId;

    DPRINT1("USBSTOR_PdoHandleQueryDeviceId\n");

    //
    // get device extension
    //
    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(DeviceExtension->InquiryData);

    //
    // get inquiry data
    //
    InquiryData = (PUFI_INQUIRY_RESPONSE)DeviceExtension->InquiryData;

    //
    // get device type
    //
    DeviceType = USBSTOR_GetDeviceType(InquiryData);

    //
    // zero buffer
    //
    RtlZeroMemory(Buffer, sizeof(Buffer));

    //
    // lets create device string
    //
    Offset = sprintf(&Buffer[Offset], "USBSTOR\\") + 1;

    //
    // copy vendor id
    //
    Offset += CopyField(InquiryData->Vendor, &Buffer[Offset], 8);

    //
    // copy product identifier
    //
    Offset += CopyField(InquiryData->Product, &Buffer[Offset], 16);

    //
    // copy revision identifer
    //
    Offset += CopyField(InquiryData->Revision, &Buffer[Offset], 4);

    //
    // now initialize ansi string
    //
    RtlInitAnsiString(&AnsiString, (PCSZ)Buffer);

    //
    // allocate DeviceId string
    //
    DeviceId.Length = 0;
    DeviceId.MaximumLength = (strlen((PCHAR)Buffer) + 1) * sizeof(WCHAR);
    DeviceId.Buffer = (LPWSTR)AllocateItem(PagedPool, DeviceId.MaximumLength);
    if (!DeviceId.Buffer)
    {
        //
        // no memory
        //
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // convert to unicode
    //
    Status = RtlAnsiStringToUnicodeString(&DeviceId, &AnsiString, FALSE);

    if (NT_SUCCESS(Status))
    {
        //
        // store result
        //
        Irp->IoStatus.Information = (ULONG_PTR)DeviceId.Buffer;
    }

    DPRINT1("DeviceId %wZ Status %x\n", &DeviceId, Status);

    //
    // done
    //
    return Status;
}

VOID
USBSTOR_ConvertToUnicodeString(
    IN CHAR * Buffer,
    IN ULONG ResultBufferLength,
    IN ULONG ResultBufferOffset,
    OUT LPWSTR ResultBuffer,
    OUT PULONG NewResultBufferOffset)
{
    UNICODE_STRING DeviceString;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    ASSERT(ResultBufferLength);
    ASSERT(ResultBufferLength > ResultBufferOffset);

    DPRINT1("ResultBufferOffset %lu ResultBufferLength %lu Buffer %s Length %lu\n", ResultBufferOffset, ResultBufferLength, Buffer, strlen(Buffer));

    DeviceString.Buffer = &ResultBuffer[ResultBufferOffset];
    DeviceString.Length = 0;
    DeviceString.MaximumLength = (ResultBufferLength - ResultBufferOffset) * sizeof(WCHAR);

    RtlInitAnsiString(&AnsiString, Buffer);

    Status = RtlAnsiStringToUnicodeString(&DeviceString, &AnsiString, FALSE);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // subtract consumed bytes
    //
    ResultBufferLength -= (DeviceString.Length + sizeof(WCHAR)) / sizeof(WCHAR);
    ResultBufferOffset += (DeviceString.Length + sizeof(WCHAR)) / sizeof(WCHAR);

    //
    // store new offset
    //
    *NewResultBufferOffset = ResultBufferOffset;
}



NTSTATUS
USBSTOR_PdoHandleQueryHardwareId(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    LPCWSTR GenericType;
    LPWSTR Buffer;
    CHAR Id1[50], Id2[50], Id3[50], Id4[50], Id5[50], Id6[50];
    ULONG Id1Length, Id2Length, Id3Length, Id4Length, Id5Length,Id6Length;
    ULONG Offset, TotalLength, Length;
    PUFI_INQUIRY_RESPONSE InquiryData;

    DPRINT1("USBSTOR_PdoHandleQueryHardwareId\n");

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->DeviceDescriptor);

    //
    // get inquiry data
    //
    InquiryData = (PUFI_INQUIRY_RESPONSE)PDODeviceExtension->InquiryData;


    //
    // get generic type
    //
    GenericType = USBSTOR_GetGenericType(InquiryData);
    ASSERT(GenericType);

    //
    // generate id 1
    // USBSTOR\SCSIType_Vendor(8)_Product(16)_Revision(4)
    //
    RtlZeroMemory(Id1, sizeof(Id1));
    Offset = 0;
    Offset = sprintf(&Id1[Offset], "USBSTOR\\");
    Offset += sprintf(&Id1[Offset], "Disk"); //FIXME
    Offset += CopyField(InquiryData->Vendor, &Id1[Offset], 8);
    Offset += CopyField(InquiryData->Product, &Id1[Offset], 16);
    Offset += CopyField(InquiryData->Revision, &Id1[Offset], 4);
    Id1Length = strlen(Id1) + 1;
    DPRINT1("HardwareId1 %s\n", Id1);

    //
    // generate id 2
    // USBSTOR\SCSIType_VENDOR(8)_Product(16)
    //
    RtlZeroMemory(Id2, sizeof(Id2));
    Offset = 0;
    Offset = sprintf(&Id2[Offset], "USBSTOR\\");
    Offset += sprintf(&Id2[Offset], "Disk"); //FIXME
    Offset += CopyField(InquiryData->Vendor, &Id2[Offset], 8);
    Offset += CopyField(InquiryData->Product, &Id2[Offset], 16);
    Id2Length = strlen(Id2) + 1;
    DPRINT1("HardwareId2 %s\n", Id2);

    //
    // generate id 3
    // USBSTOR\SCSIType_VENDOR(8)
    //
    RtlZeroMemory(Id3, sizeof(Id3));
    Offset = 0;
    Offset = sprintf(&Id3[Offset], "USBSTOR\\");
    Offset += sprintf(&Id3[Offset], "Disk"); //FIXME
    Offset += CopyField(InquiryData->Vendor, &Id3[Offset], 8);
    Id3Length = strlen(Id3) + 1;
    DPRINT1("HardwareId3 %s\n", Id3);

    //
    // generate id 4
    // USBSTOR\SCSIType_VENDOR(8)_Product(16)_Revision(1)
    //
    RtlZeroMemory(Id4, sizeof(Id4));
    Offset = 0;
    Offset = sprintf(&Id4[Offset], "USBSTOR\\");
    Offset += sprintf(&Id4[Offset], "Disk"); //FIXME
    Offset += CopyField(InquiryData->Vendor, &Id4[Offset], 8);
    Offset += CopyField(InquiryData->Product, &Id4[Offset], 16);
    Offset += CopyField(InquiryData->Revision, &Id4[Offset], 1);
    Id4Length = strlen(Id4) + 1;
    DPRINT1("HardwareId4 %s\n", Id4);

    //
    // generate id 5
    // USBSTOR\SCSIType
    //
    RtlZeroMemory(Id5, sizeof(Id5));
    Offset = 0;
    Offset = sprintf(&Id5[Offset], "USBSTOR\\");
    Offset += sprintf(&Id5[Offset], "GenDisk"); //FIXME
    Id5Length = strlen(Id5) + 1;
    DPRINT1("HardwareId5 %s\n", Id5);

    //
    // generate id 6
    // SCSIType
    //
    RtlZeroMemory(Id6, sizeof(Id6));
    Offset = 0;
    Offset = sprintf(&Id6[Offset], "GenDisk"); //FIXME
    Id6Length = strlen(Id6) + 1;
    DPRINT1("HardwareId6 %s\n", Id6);

    //
    // compute total length
    //
    TotalLength = Id1Length + Id2Length + Id3Length + Id4Length + Id5Length + Id6Length + 1;

    //
    // allocate buffer
    //
    Buffer = (LPWSTR)AllocateItem(PagedPool, TotalLength * sizeof(WCHAR));
    if (!Buffer)
    {
        //
        // no memory
        //
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // reset offset
    //
    Offset = 0;
    Length = TotalLength;

    USBSTOR_ConvertToUnicodeString(Id1, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id2, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id3, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id4, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id5, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id6, Length, Offset, Buffer, &Offset);

    DPRINT1("Offset %lu Length %lu\n", Offset, Length);

    //
    // store result
    //
    Irp->IoStatus.Information = (ULONG_PTR)Buffer;

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USBSTOR_PdoHandleQueryCompatibleId(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    WCHAR Buffer[100];
    ULONG Length;
    LPWSTR InstanceId;
    LPCSTR DeviceType;

    DPRINT1("USBSTOR_PdoHandleQueryCompatibleId\n");

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->DeviceDescriptor);

    //
    // get target device type
    //
    DeviceType = USBSTOR_GetDeviceType((PUFI_INQUIRY_RESPONSE)PDODeviceExtension->InquiryData);

    //
    // zero memory
    //
    RtlZeroMemory(Buffer, sizeof(Buffer));

    //
    // format instance id
    //
    Length = swprintf(Buffer, L"USBSTOR\\%s", L"Disk") + 1;
    Length += swprintf(&Buffer[Length], L"USBSTOR\\%s", L"RAW") + 2;

    //
    // verify this
    //
   // Length += swprintf(&Buffer[Length], L"USBSTOR\\RAW") + 1;

    //Buffer[Length] = UNICODE_NULL;
    //Buffer[Length+1] = UNICODE_NULL;
    //Length++;

    //
    // calculate length
    //


    //
    // allocate instance id
    //
    InstanceId = (LPWSTR)AllocateItem(PagedPool, Length * sizeof(WCHAR));
    if (!InstanceId)
    {
        //
        // no memory
        //
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
 
    //
    // copy instance id
    //
    wcscpy(InstanceId, Buffer);

    DPRINT1("USBSTOR_PdoHandleQueryInstanceId %S\n", InstanceId);

    //
    // store result
    //
    Irp->IoStatus.Information = (ULONG_PTR)InstanceId;

    //
    // completed successfully
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USBSTOR_PdoHandleQueryInstanceId(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    WCHAR Buffer[100];
    ULONG Length;
    LPWSTR InstanceId;

    DPRINT1("USBSTOR_PdoHandleQueryInstanceId\n");

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->DeviceDescriptor);

    //
    // format instance id
    //
    swprintf(Buffer, L"USB\\VID_%04x&PID_%04x\\%s", FDODeviceExtension->DeviceDescriptor->idVendor, FDODeviceExtension->DeviceDescriptor->idProduct, L"09188212515A");

    //
    // calculate length
    //
    Length = wcslen(Buffer) + 1;

    //
    // allocate instance id
    //
    InstanceId = (LPWSTR)AllocateItem(PagedPool, Length * sizeof(WCHAR));
    if (!InstanceId)
    {
        //
        // no memory
        //
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
 
    //
    // copy instance id
    //
    wcscpy(InstanceId, Buffer);

    DPRINT1("USBSTOR_PdoHandleQueryInstanceId %S\n", InstanceId);

    //
    // store result
    //
    Irp->IoStatus.Information = (ULONG_PTR)InstanceId;

    //
    // completed successfully
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USBSTOR_PdoHandleDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations;
    PIO_STACK_LOCATION IoStack;

    DPRINT1("USBSTOR_PdoHandleDeviceRelations\n");

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
USBSTOR_PdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PPDO_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    PDEVICE_CAPABILITIES Caps;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(DeviceExtension->Common.IsFDO == FALSE);

    switch(IoStack->MinorFunction)
    {
       case IRP_MN_QUERY_DEVICE_RELATIONS:
       {
           Status = USBSTOR_PdoHandleDeviceRelations(DeviceObject, Irp);
           break;
       }
       case IRP_MN_QUERY_DEVICE_TEXT:
       {
           Status = USBSTOR_PdoHandleQueryDeviceText(DeviceObject, Irp);
           break;
       }
       case IRP_MN_QUERY_ID:
       {
           if (IoStack->Parameters.QueryId.IdType == BusQueryDeviceID)
           {
               //
               // handle query device id
               //
               Status = USBSTOR_PdoHandleQueryDeviceId(DeviceObject, Irp);
               break;
           }
           else if (IoStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
           {
               //
               // handle instance id
               //
               Status = USBSTOR_PdoHandleQueryHardwareId(DeviceObject, Irp);
               break;
           }
           else if (IoStack->Parameters.QueryId.IdType == BusQueryInstanceID)
           {
               //
               // handle instance id
               //
               Status = USBSTOR_PdoHandleQueryInstanceId(DeviceObject, Irp);
               break;
           }
           else if (IoStack->Parameters.QueryId.IdType == BusQueryCompatibleIDs)
           {
               //
               // handle instance id
               //
               Status = USBSTOR_PdoHandleQueryCompatibleId(DeviceObject, Irp);
               break;
           }

           DPRINT1("USBSTOR_PdoHandlePnp: IRP_MN_QUERY_ID IdType %x unimplemented\n", IoStack->Parameters.QueryId.IdType);
           Status = STATUS_NOT_SUPPORTED;
           Irp->IoStatus.Information = 0;
           break;
       }
       case IRP_MN_REMOVE_DEVICE:
           DPRINT1("USBSTOR_PdoHandlePnp: IRP_MN_REMOVE_DEVICE unimplemented\n");
           Status = STATUS_SUCCESS;
           break;
       case IRP_MN_QUERY_CAPABILITIES:
       {
           //
           // just forward irp to lower device
           //
           Status = USBSTOR_SyncForwardIrp(DeviceExtension->LowerDeviceObject, Irp);
           ASSERT(Status == STATUS_SUCCESS);

           if (NT_SUCCESS(Status))
           {
               //
               // check if no unique id
               //
               Caps = (PDEVICE_CAPABILITIES)IoStack->Parameters.DeviceCapabilities.Capabilities;
               Caps->UniqueID = TRUE; //FIXME
               Caps->Removable = TRUE; //FIXME
           }
           break;
       }
       case IRP_MN_START_DEVICE:
       {
           //
           // no-op for PDO
           //
           Status = STATUS_SUCCESS;
           break;
       }
       default:
        {
            //
            // do nothing
            //
            Status = Irp->IoStatus.Status;
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
USBSTOR_CreatePDO(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_OBJECT *ChildDeviceObject)
{
    PDEVICE_OBJECT PDO;
    NTSTATUS Status;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;

    //
    // create child device object
    //
    Status = IoCreateDevice(DeviceObject->DriverObject, sizeof(PDO_DEVICE_EXTENSION), NULL, FILE_DEVICE_MASS_STORAGE, FILE_AUTOGENERATED_DEVICE_NAME, FALSE, &PDO);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create device
        //
        return Status;
    }

    //
    // patch the stack size
    //
    PDO->StackSize = DeviceObject->StackSize;

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)PDO->DeviceExtension;

    //
    // initialize device extension
    //
    RtlZeroMemory(PDODeviceExtension, sizeof(PDO_DEVICE_EXTENSION));
    PDODeviceExtension->Common.IsFDO = FALSE;
    PDODeviceExtension->LowerDeviceObject = DeviceObject;

    //
    // set device flags
    //
    PDO->Flags |= DO_DIRECT_IO | DO_MAP_IO_BUFFER;

    //
    // device is initialized
    //
    PDO->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // output device object
    //
    *ChildDeviceObject = PDO;

    USBSTOR_SendInquiryCmd(PDO);

    //
    // done
    //
    return Status;
}
