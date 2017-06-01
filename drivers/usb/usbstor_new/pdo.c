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

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
USBSTOR_SyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    DPRINT("USBSTOR_SyncCompletionRoutine: ... \n");
    KeSetEvent((PRKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

LPCSTR
USBSTOR_GetDeviceType(
    IN PUFI_INQUIRY_RESPONSE InquiryData,
    IN UCHAR IsFloppy)
{
    //
    // check if device type is zero
    //
    if (InquiryData->DeviceType == 0)
    {
        if (IsFloppy)
        {
            //
            // floppy device
            //
            return "SFloppy";
        }

        //
        // direct access device
        //
        return "Disk";
    }

    //
    // FIXME: use constant - derived from http://en.wikipedia.org/wiki/SCSI_Peripheral_Device_Type
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

LPCSTR
USBSTOR_GetGenericType(
    IN PUFI_INQUIRY_RESPONSE InquiryData,
    IN UCHAR IsFloppy)
{
    //
    // check if device type is zero
    //
    if (InquiryData->DeviceType == 0)
    {
        if (IsFloppy)
        {
            //
            // floppy device
            //
            return "GenSFloppy";
        }

        //
        // direct access device
        //
        return "GenDisk";
    }

    //
    // FIXME: use constant - derived from http://en.wikipedia.org/wiki/SCSI_Peripheral_Device_Type
    //
    switch (InquiryData->DeviceType)
    {
        case 1:
        {
            //
            // sequential device, i.e magnetic tape
            //
            return "GenSequential";
        }
        case 4:
        {
            //
            // write once device
            //
            return "GenWorm";
        }
        case 5:
        {
            //
            // CDROM device
            //
            return "GenCdRom";
        }
        case 7:
        {
            //
            // optical memory device
            //
            return "GenOptical";
        }
        case 8:
        {
            //
            // medium change device
            //
            return "GenChanger";
        }
        default:
        {
            //
            // other device
            //
            return "UsbstorOther";
        }
    }
}


ULONG
CopyField(
    IN PUCHAR Name,
    IN PCHAR Buffer,
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
        DPRINT("USBSTOR_PdoHandleQueryDeviceText DeviceTextDescription\n");

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
        DPRINT("USBSTOR_PdoHandleQueryDeviceText DeviceTextLocationInformation\n");

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
    CHAR Buffer[100];
    LPCSTR DeviceType;
    ULONG Offset = 0;
    PUFI_INQUIRY_RESPONSE InquiryData;
    ANSI_STRING AnsiString;
    UNICODE_STRING DeviceId;

    //
    // get device extension
    //
    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(&DeviceExtension->InquiryData);

    //
    // get inquiry data
    //
    InquiryData = &DeviceExtension->InquiryData;

    //
    // get device type
    //
    DeviceType = USBSTOR_GetDeviceType(InquiryData, DeviceExtension->IsFloppy);

    //
    // zero buffer
    //
    RtlZeroMemory(Buffer, sizeof(Buffer));

    //
    // lets create device string
    //
    Offset = sprintf(&Buffer[Offset], "USBSTOR\\");
    Offset += sprintf(&Buffer[Offset], DeviceType);
    Offset += sprintf(&Buffer[Offset], "&Ven_");
    Offset += CopyField(InquiryData->Vendor, &Buffer[Offset], 8);
    Offset += sprintf(&Buffer[Offset], "&Prod_");
    Offset += CopyField(InquiryData->Product, &Buffer[Offset], 16);
    Offset += sprintf(&Buffer[Offset], "&Rev_");
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

    DPRINT("DeviceId %wZ Status %x\n", &DeviceId, Status);

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

    DPRINT("ResultBufferOffset %lu ResultBufferLength %lu Buffer %s Length %lu\n", ResultBufferOffset, ResultBufferLength, Buffer, strlen(Buffer));

    //
    // construct destination string
    //
    DeviceString.Buffer = &ResultBuffer[ResultBufferOffset];
    DeviceString.Length = 0;
    DeviceString.MaximumLength = (ResultBufferLength - ResultBufferOffset) * sizeof(WCHAR);

    //
    // initialize source string
    //
    RtlInitAnsiString(&AnsiString, Buffer);

    //
    // convert to unicode
    //
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
    LPCSTR GenericType, DeviceType;
    LPWSTR Buffer;
    CHAR Id1[50], Id2[50], Id3[50], Id4[50], Id5[50], Id6[50];
    ULONG Id1Length, Id2Length, Id3Length, Id4Length, Id5Length,Id6Length;
    ULONG Offset, TotalLength, Length;
    PUFI_INQUIRY_RESPONSE InquiryData;

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
    InquiryData = &PDODeviceExtension->InquiryData;


    //
    // get device type and generic type
    //
    DeviceType = USBSTOR_GetDeviceType(InquiryData, PDODeviceExtension->IsFloppy);
    GenericType = USBSTOR_GetGenericType(InquiryData, PDODeviceExtension->IsFloppy);

    ASSERT(GenericType);

    //
    // generate id 1
    // USBSTOR\SCSIType_Vendor(8)_Product(16)_Revision(4)
    //
    RtlZeroMemory(Id1, sizeof(Id1));
    Offset = 0;
    Offset = sprintf(&Id1[Offset], "USBSTOR\\");
    Offset += sprintf(&Id1[Offset], DeviceType);
    Offset += CopyField(InquiryData->Vendor, &Id1[Offset], 8);
    Offset += CopyField(InquiryData->Product, &Id1[Offset], 16);
    Offset += CopyField(InquiryData->Revision, &Id1[Offset], 4);
    Id1Length = strlen(Id1) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId1 %s\n", Id1);

    //
    // generate id 2
    // USBSTOR\SCSIType_VENDOR(8)_Product(16)
    //
    RtlZeroMemory(Id2, sizeof(Id2));
    Offset = 0;
    Offset = sprintf(&Id2[Offset], "USBSTOR\\");
    Offset += sprintf(&Id2[Offset], DeviceType);
    Offset += CopyField(InquiryData->Vendor, &Id2[Offset], 8);
    Offset += CopyField(InquiryData->Product, &Id2[Offset], 16);
    Id2Length = strlen(Id2) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId2 %s\n", Id2);

    //
    // generate id 3
    // USBSTOR\SCSIType_VENDOR(8)
    //
    RtlZeroMemory(Id3, sizeof(Id3));
    Offset = 0;
    Offset = sprintf(&Id3[Offset], "USBSTOR\\");
    Offset += sprintf(&Id3[Offset], DeviceType);
    Offset += CopyField(InquiryData->Vendor, &Id3[Offset], 8);
    Id3Length = strlen(Id3) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId3 %s\n", Id3);

    //
    // generate id 4
    // USBSTOR\SCSIType_VENDOR(8)_Product(16)_Revision(1)
    //
    RtlZeroMemory(Id4, sizeof(Id4));
    Offset = 0;
    Offset = sprintf(&Id4[Offset], "USBSTOR\\");
    Offset += sprintf(&Id4[Offset], DeviceType);
    Offset += CopyField(InquiryData->Vendor, &Id4[Offset], 8);
    Offset += CopyField(InquiryData->Product, &Id4[Offset], 16);
    Offset += CopyField(InquiryData->Revision, &Id4[Offset], 1);
    Id4Length = strlen(Id4) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId4 %s\n", Id4);

    //
    // generate id 5
    // USBSTOR\SCSIType
    //
    RtlZeroMemory(Id5, sizeof(Id5));
    Offset = 0;
    Offset = sprintf(&Id5[Offset], "USBSTOR\\");
    Offset += sprintf(&Id5[Offset], GenericType);
    Id5Length = strlen(Id5) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId5 %s\n", Id5);

    //
    // generate id 6
    // SCSIType
    //
    RtlZeroMemory(Id6, sizeof(Id6));
    Offset = 0;
    Offset = sprintf(&Id6[Offset], GenericType);
    Id6Length = strlen(Id6) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId6 %s\n", Id6);

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

    //
    // sanity check
    //
    ASSERT(Offset + 1 == Length);

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
    CHAR Buffer[100];
    ULONG Length, Offset;
    LPWSTR InstanceId;
    LPCSTR DeviceType;

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
    DeviceType = USBSTOR_GetDeviceType(&PDODeviceExtension->InquiryData, PDODeviceExtension->IsFloppy);

    //
    // zero memory
    //
    RtlZeroMemory(Buffer, sizeof(Buffer));

    //
    // format instance id
    //
    Length = sprintf(Buffer, "USBSTOR\\%s", DeviceType) + 1;
    Length += sprintf(&Buffer[Length], "USBSTOR\\%s", "RAW") + 2;

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

    USBSTOR_ConvertToUnicodeString(Buffer, Length, 0, InstanceId, &Offset);
    USBSTOR_ConvertToUnicodeString(&Buffer[Offset], Length, Offset, InstanceId, &Offset);

    DPRINT("USBSTOR_PdoHandleQueryCompatibleId %S\n", InstanceId);

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

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // format instance id
    //
    if (FDODeviceExtension->SerialNumber)
    {
        //
        // using serial number from device
        //
        swprintf(Buffer, L"%s&%c", FDODeviceExtension->SerialNumber->bString, PDODeviceExtension->LUN);
    }
    else
    {
        //
        // use instance count and LUN
        //
        swprintf(Buffer, L"%04lu&%c", FDODeviceExtension->InstanceCount, PDODeviceExtension->LUN);
    }

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

    DPRINT("USBSTOR_PdoHandleQueryInstanceId %S\n", InstanceId);

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

    DPRINT("USBSTOR_PdoHandleDeviceRelations\n");

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
    ULONG bDelete;

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
       {
           DPRINT("IRP_MN_REMOVE_DEVICE\n");

           if(*DeviceExtension->PDODeviceObject != NULL)
           {
               //
               // clear entry in FDO pdo list
               //
               *DeviceExtension->PDODeviceObject = NULL;
               bDelete = TRUE;
           }
           else
           {
               //
               // device object already marked for deletion
               //
               bDelete = FALSE;
           }

           /* Complete the IRP */
           Irp->IoStatus.Status = STATUS_SUCCESS;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);

           if (bDelete)
           {
               /* Delete the device object */
               IoDeleteDevice(DeviceObject);
           }
           return STATUS_SUCCESS;
       }
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
               Caps->UniqueID = FALSE; // no unique id is supported
               Caps->Removable = TRUE; //FIXME
           }
           break;
       }
       case IRP_MN_QUERY_REMOVE_DEVICE:
       case IRP_MN_QUERY_STOP_DEVICE:
       {
#if 0
           //
           // if we're not claimed it's ok
           //
           if (DeviceExtension->Claimed)
#else
           if (TRUE)
#endif
           {
               Status = STATUS_UNSUCCESSFUL;
               DPRINT1("[USBSTOR] Request %x fails because device is still claimed\n", IoStack->MinorFunction);
           }
           else
               Status = STATUS_SUCCESS;
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
       case IRP_MN_SURPRISE_REMOVAL:
       {
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
NTAPI
USBSTOR_CompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx)
{
    PKEVENT Event = (PKEVENT)Ctx;

    //
    // signal event
    //
    KeSetEvent(Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
USBSTOR_AllocateIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DataTransferLength,
    IN UCHAR OpCode,
    IN PKEVENT Event,
    OUT PSCSI_REQUEST_BLOCK *OutRequest,
    OUT PIRP *OutIrp)
{
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;
    PCDB pCDB;

    //
    // allocate irp
    //
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // get next stack location
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // create scsi block
    //
    Request = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(SCSI_REQUEST_BLOCK),
                                    USB_STOR_TAG);
    if (!Request)
    {
        //
        // no memory
        //
        IoFreeIrp(Irp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init request
    //
    RtlZeroMemory(Request, sizeof(SCSI_REQUEST_BLOCK));

    //
    // allocate data transfer block
    //
    Request->DataBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                DataTransferLength,
                                                USB_STOR_TAG);
    if (!Request->DataBuffer)
    {
        //
        // no memory
        //
        IoFreeIrp(Irp);
        ExFreePoolWithTag(Request, USB_STOR_TAG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // allocate MDL
    //
    Irp->MdlAddress = IoAllocateMdl(Request->DataBuffer, DataTransferLength, FALSE, FALSE, NULL);
    if (!Irp->MdlAddress)
    {
        //
        // no memory
        //
        IoFreeIrp(Irp);
        ExFreePoolWithTag(Request->DataBuffer, USB_STOR_TAG);
        ExFreePoolWithTag(Request, USB_STOR_TAG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // non paged pool
    //
    MmBuildMdlForNonPagedPool(Irp->MdlAddress);

    //
    // init scsi block
    //
    Request->DataTransferLength = DataTransferLength;
    Request->Function = SRB_FUNCTION_EXECUTE_SCSI;
    Request->SrbFlags = SRB_FLAGS_DATA_IN;

    RtlZeroMemory(Request->DataBuffer, DataTransferLength);


    //
    // get SCSI command data block
    //
    pCDB = (PCDB)Request->Cdb;

    //
    // set op code
    //
    pCDB->AsByte[0] = OpCode;

    //
    // store result
    //
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.Others.Argument1 = Request;
    IoStack->DeviceObject = DeviceObject;

    //
    // init event
    //
    KeInitializeEvent(Event, NotificationEvent, FALSE);

    //
    // lets setup a completion routine
    //
    IoSetCompletionRoutine(Irp, USBSTOR_CompletionRoutine, (PVOID)Event, TRUE, TRUE, TRUE);

    //
    // output result
    //
    *OutIrp = Irp;
    *OutRequest = Request;
    return STATUS_SUCCESS;
}

NTSTATUS
USBSTOR_SendIrp(
    IN PDEVICE_OBJECT PDODeviceObject,
    IN ULONG DataTransferLength,
    IN UCHAR OpCode,
    OUT PVOID *OutData)
{
    NTSTATUS Status;
    PIRP Irp;
    KEVENT Event;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PSCSI_REQUEST_BLOCK Request;

    //
    // let's allocate an irp
    //
    Status = USBSTOR_AllocateIrp(PDODeviceObject, DataTransferLength, OpCode, &Event, &Request, &Irp);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        DPRINT1("[USBSTOR] Failed to build irp\n");
        return Status;
    }

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)PDODeviceObject->DeviceExtension;

    //
    // send irp
    //
    ASSERT(Irp);
    ASSERT(PDODeviceExtension->LowerDeviceObject);
    Status = IoCallDriver(PDODeviceExtension->Self, Irp);

    if (Status == STATUS_PENDING)
    {
        //
        // wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    if (NT_SUCCESS(Status))
    {
        //
        // store result
        //
        *OutData = Request->DataBuffer;
    }
    else
    {
        //
        // free the data
        //
        ExFreePoolWithTag(Request->DataBuffer, USB_STOR_TAG);
        *OutData = NULL;
    }

    //
    // free resources
    //
    ExFreePoolWithTag(Request, USB_STOR_TAG);
    IoFreeMdl(Irp->MdlAddress);
    IoFreeIrp(Irp);
    return Status;
}

NTSTATUS
USBSTOR_SendInquiryIrp(
    IN PDEVICE_OBJECT PDODeviceObject)
{
    NTSTATUS Status;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PUFI_INQUIRY_RESPONSE Response;

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)PDODeviceObject->DeviceExtension;

    //
    // send request
    //
    Status = USBSTOR_SendIrp(PDODeviceObject, sizeof(UFI_INQUIRY_RESPONSE), SCSIOP_INQUIRY, (PVOID*)&Response);
    if (!NT_SUCCESS(Status))
    {
        //
        // command failed
        //
        DPRINT1("USBSTOR_SendInquiryIrp Failed with %x\n", Status);
        return Status;
    }

    DPRINT1("Response %p\n", Response);
    DPRINT1("DeviceType %x\n", Response->DeviceType);
    DPRINT1("RMB %x\n", Response->RMB);
    DPRINT1("Version %x\n", Response->Version);
    DPRINT1("Format %x\n", Response->Format);
    DPRINT1("Length %x\n", Response->Length);
    DPRINT1("Reserved %p\n", Response->Reserved);
    DPRINT1("Vendor %c%c%c%c%c%c%c%c\n", Response->Vendor[0], Response->Vendor[1], Response->Vendor[2], Response->Vendor[3], Response->Vendor[4], Response->Vendor[5], Response->Vendor[6], Response->Vendor[7]);
    DPRINT1("Product %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", Response->Product[0], Response->Product[1], Response->Product[2], Response->Product[3],
                                                          Response->Product[4], Response->Product[5], Response->Product[6], Response->Product[7],
                                                          Response->Product[8], Response->Product[9], Response->Product[10], Response->Product[11],
                                                          Response->Product[12], Response->Product[13], Response->Product[14], Response->Product[15]);

    DPRINT1("Revision %c%c%c%c\n", Response->Revision[0], Response->Revision[1], Response->Revision[2], Response->Revision[3]);

    //
    // store result
    //
    PDODeviceExtension->InquiryData = (PVOID)Response;
    return Status;
}

NTSTATUS
USBSTOR_SendFormatCapacityIrp(
    IN PDEVICE_OBJECT PDODeviceObject)
{
    NTSTATUS Status;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PUCHAR Response;

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)PDODeviceObject->DeviceExtension;

    //
    // send request
    //
    Status = USBSTOR_SendIrp(PDODeviceObject, 0xFC, SCSIOP_READ_FORMATTED_CAPACITY, (PVOID*)&Response);
    if (!NT_SUCCESS(Status))
    {
        //
        // command failed
        //
        return Status;
    }

    //
    // check if its a floppy
    //
    PDODeviceExtension->IsFloppy = USBSTOR_IsFloppy(Response, 0xFC /*FIXME*/, &PDODeviceExtension->MediumTypeCode);

    //
    // free response
    //
    ExFreePoolWithTag(Response, USB_STOR_TAG);
    return Status;
}



NTSTATUS
USBSTOR_CreatePDO(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR LUN)
{
    PDEVICE_OBJECT PDO;
    NTSTATUS Status;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    DPRINT("USBSTOR_CreatePDO: LUN %x\n", LUN);

    //
    // get device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;


    //
    // create child device object
    //
    Status = IoCreateDevice(DeviceObject->DriverObject,
                            sizeof(PDO_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_MASS_STORAGE,
                            FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &PDO);

    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create device
        //
       DPRINT1("USBSTOR_CreatePDO: Failed to create PDO Status %x\n", Status);
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
    PDODeviceExtension->PDODeviceObject = &FDODeviceExtension->ChildPDO[LUN];
    PDODeviceExtension->Self = PDO;
    PDODeviceExtension->LUN = LUN;

    //
    // set device flags
    //
    PDO->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;

    //
    // device is initialized
    //
    PDO->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // output device object
    //
    FDODeviceExtension->ChildPDO[LUN] = PDO;
    DPRINT("USBSTOR_CreatePDO: ChildPDO[%d] %p\n", LUN, PDO);

    //
    // send inquiry command by irp
    //
    Status = USBSTOR_GetInquiryData(PDO);

    if (NT_SUCCESS(Status) &&
        !(PDODeviceExtension->InquiryData.DeviceType & FILE_DEVICE_TAPE))
    {
        PDODeviceExtension->IsFloppy = USBSTOR_IsFloppyDevice(PDO);
    }

    //
    // done
    //
    return Status;
}
