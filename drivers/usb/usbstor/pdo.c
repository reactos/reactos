/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     USB block storage device driver.
 * COPYRIGHT:   2005-2006 James Tabor
 *              2011-2012 Michael Martin (michael.martin@reactos.org)
 *              2011-2013 Johannes Anderwald (johannes.anderwald@reactos.org)
 *              2017 Vadim Galyant
 *              2019 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include "usbstor.h"

#define NDEBUG
#include <debug.h>


static
LPCSTR
USBSTOR_GetDeviceType(
    IN PINQUIRYDATA InquiryData)
{
    switch (InquiryData->DeviceType)
    {
        case DIRECT_ACCESS_DEVICE:
            return "Disk";
        case SEQUENTIAL_ACCESS_DEVICE:
            // sequential device, i.e magnetic tape
            return "Sequential";
        case WRITE_ONCE_READ_MULTIPLE_DEVICE:
            return "Worm";
        case READ_ONLY_DIRECT_ACCESS_DEVICE:
            return "CdRom";
        case OPTICAL_DEVICE:
            return "Optical";
        case MEDIUM_CHANGER:
            return "Changer";
        default:
            return "Other";
    }
}

static
LPCSTR
USBSTOR_GetGenericType(
    IN PINQUIRYDATA InquiryData)
{
    switch (InquiryData->DeviceType)
    {
        case DIRECT_ACCESS_DEVICE:
            return "GenDisk";
        case SEQUENTIAL_ACCESS_DEVICE:
            // sequential device, i.e magnetic tape
            return "GenSequential";
        case WRITE_ONCE_READ_MULTIPLE_DEVICE:
            return "GenWorm";
        case READ_ONLY_DIRECT_ACCESS_DEVICE:
            return "GenCdRom";
        case OPTICAL_DEVICE:
            return "GenOptical";
        case MEDIUM_CHANGER:
            return "GenChanger";
        default:
            return "UsbstorOther";
    }
}

static
ULONG
CopyField(
    IN PUCHAR Name,
    IN PCHAR Buffer,
    IN ULONG MaxLength)
{
    ULONG Index;

    for (Index = 0; Index < MaxLength; Index++)
    {
        if (Name[Index] <= ' ' || Name[Index] >= 0x7F /* last printable ascii character */ ||  Name[Index] == ',')
        {
            // convert to underscore
            Buffer[Index] = '_';
        }
        else
        {
            // just copy character
            Buffer[Index] = Name[Index];
        }
    }

    return MaxLength;
}

static
ULONG
CopyFieldTruncate(
    IN PUCHAR Name,
    IN PCHAR Buffer,
    IN ULONG MaxLength)
{
    ULONG Index;

    for (Index = 0; Index < MaxLength; Index++)
    {
        if (Name[Index] == '\0')
        {
            break;
        }
        else if (Name[Index] <= ' ' || Name[Index] >= 0x7F /* last printable ascii character */ ||  Name[Index] == ',')
        {
            // convert to underscore
            Buffer[Index] = ' ';
        }
        else
        {
            // just copy character
            Buffer[Index] = Name[Index];
        }
    }

    return Index;
}

NTSTATUS
USBSTOR_PdoHandleQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStack;
    CHAR LocalBuffer[26];
    UINT32 Offset = 0;
    PINQUIRYDATA InquiryData;
    ANSI_STRING AnsiString;
    UNICODE_STRING DeviceDescription;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    InquiryData = (PINQUIRYDATA)&DeviceExtension->InquiryData;

    switch (IoStack->Parameters.QueryDeviceText.DeviceTextType)
    {
        case DeviceTextDescription:
        case DeviceTextLocationInformation:
        {
            DPRINT("USBSTOR_PdoHandleQueryDeviceText\n");

            Offset += CopyFieldTruncate(InquiryData->VendorId, &LocalBuffer[Offset], sizeof(InquiryData->VendorId));
            LocalBuffer[Offset++] = ' ';
            Offset += CopyFieldTruncate(InquiryData->ProductId, &LocalBuffer[Offset], sizeof(InquiryData->ProductId));
            LocalBuffer[Offset++] = '\0';

            RtlInitAnsiString(&AnsiString, (PCSZ)&LocalBuffer);

            DeviceDescription.Length = 0;
            DeviceDescription.MaximumLength = (USHORT)(Offset * sizeof(WCHAR));
            DeviceDescription.Buffer = ExAllocatePoolWithTag(PagedPool, DeviceDescription.MaximumLength, USB_STOR_TAG);
            if (!DeviceDescription.Buffer)
            {
                Irp->IoStatus.Information = 0;
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlAnsiStringToUnicodeString(&DeviceDescription, &AnsiString, FALSE);

            Irp->IoStatus.Information = (ULONG_PTR)DeviceDescription.Buffer;
            return STATUS_SUCCESS;
        }
        default:
        {
            Irp->IoStatus.Information = 0;
            return Irp->IoStatus.Status;
        }
    }
}

NTSTATUS
USBSTOR_PdoHandleQueryDeviceId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    CHAR Buffer[100] = {0};
    LPCSTR DeviceType;
    ULONG Offset = 0;
    PINQUIRYDATA InquiryData;
    ANSI_STRING AnsiString;
    UNICODE_STRING DeviceId;

    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    InquiryData = (PINQUIRYDATA)&DeviceExtension->InquiryData;

    DeviceType = USBSTOR_GetDeviceType(InquiryData);

    // lets create device string
    Offset = sprintf(&Buffer[Offset], "USBSTOR\\");
    Offset += sprintf(&Buffer[Offset], DeviceType);
    Offset += sprintf(&Buffer[Offset], "&Ven_");
    Offset += CopyField(InquiryData->VendorId, &Buffer[Offset], 8);
    Offset += sprintf(&Buffer[Offset], "&Prod_");
    Offset += CopyField(InquiryData->ProductId, &Buffer[Offset], 16);
    Offset += sprintf(&Buffer[Offset], "&Rev_");
    Offset += CopyField(InquiryData->ProductRevisionLevel, &Buffer[Offset], 4);

    RtlInitAnsiString(&AnsiString, (PCSZ)Buffer);

    // allocate DeviceId string
    DeviceId.Length = 0;
    DeviceId.MaximumLength = (USHORT)((strlen((PCHAR)Buffer) + 1) * sizeof(WCHAR));
    DeviceId.Buffer = ExAllocatePoolWithTag(PagedPool, DeviceId.MaximumLength, USB_STOR_TAG);
    if (!DeviceId.Buffer)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlAnsiStringToUnicodeString(&DeviceId, &AnsiString, FALSE);

    if (NT_SUCCESS(Status))
    {
        Irp->IoStatus.Information = (ULONG_PTR)DeviceId.Buffer;
    }

    DPRINT("DeviceId %wZ Status %x\n", &DeviceId, Status);

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

    // construct destination string
    DeviceString.Buffer = &ResultBuffer[ResultBufferOffset];
    DeviceString.Length = 0;
    DeviceString.MaximumLength = (ResultBufferLength - ResultBufferOffset) * sizeof(WCHAR);

    // initialize source string
    RtlInitAnsiString(&AnsiString, Buffer);

    Status = RtlAnsiStringToUnicodeString(&DeviceString, &AnsiString, FALSE);
    ASSERT(Status == STATUS_SUCCESS);

    // subtract consumed bytes
    ResultBufferLength -= (DeviceString.Length + sizeof(WCHAR)) / sizeof(WCHAR);
    ResultBufferOffset += (DeviceString.Length + sizeof(WCHAR)) / sizeof(WCHAR);

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
    CHAR Id1[50], Id2[50], Id3[50], Id4[50], Id5[50], Id6[50], Id7[50];
    ULONG Id1Length, Id2Length, Id3Length, Id4Length, Id5Length, Id6Length, Id7Length;
    ULONG Offset, TotalLength, Length;
    PINQUIRYDATA InquiryData;

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->DeviceDescriptor);
    InquiryData = (PINQUIRYDATA)&PDODeviceExtension->InquiryData;

    DeviceType = USBSTOR_GetDeviceType(InquiryData);
    GenericType = USBSTOR_GetGenericType(InquiryData);

    ASSERT(GenericType);

    // generate id 1
    // USBSTOR\SCSIType_VendorId(8)_ProductId(16)_Revision(4)
    RtlZeroMemory(Id1, sizeof(Id1));
    Offset = 0;
    Offset = sprintf(&Id1[Offset], "USBSTOR\\");
    Offset += sprintf(&Id1[Offset], DeviceType);
    Offset += CopyField(InquiryData->VendorId, &Id1[Offset], 8);
    Offset += CopyField(InquiryData->ProductId, &Id1[Offset], 16);
    Offset += CopyField(InquiryData->ProductRevisionLevel, &Id1[Offset], 4);
    Id1Length = strlen(Id1) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId1 %s\n", Id1);

    // generate id 2
    // USBSTOR\SCSIType_VendorId(8)_ProductId(16)
    RtlZeroMemory(Id2, sizeof(Id2));
    Offset = 0;
    Offset = sprintf(&Id2[Offset], "USBSTOR\\");
    Offset += sprintf(&Id2[Offset], DeviceType);
    Offset += CopyField(InquiryData->VendorId, &Id2[Offset], 8);
    Offset += CopyField(InquiryData->ProductId, &Id2[Offset], 16);
    Id2Length = strlen(Id2) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId2 %s\n", Id2);

    // generate id 3
    // USBSTOR\SCSIType_VendorId(8)
    RtlZeroMemory(Id3, sizeof(Id3));
    Offset = 0;
    Offset = sprintf(&Id3[Offset], "USBSTOR\\");
    Offset += sprintf(&Id3[Offset], DeviceType);
    Offset += CopyField(InquiryData->VendorId, &Id3[Offset], 8);
    Id3Length = strlen(Id3) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId3 %s\n", Id3);

    // generate id 4
    // USBSTOR\SCSIType_VendorId(8)_ProductId(16)_Revision(1)
    RtlZeroMemory(Id4, sizeof(Id4));
    Offset = 0;
    Offset = sprintf(&Id4[Offset], "USBSTOR\\");
    Offset += sprintf(&Id4[Offset], DeviceType);
    Offset += CopyField(InquiryData->VendorId, &Id4[Offset], 8);
    Offset += CopyField(InquiryData->ProductId, &Id4[Offset], 16);
    Offset += CopyField(InquiryData->ProductRevisionLevel, &Id4[Offset], 1);
    Id4Length = strlen(Id4) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId4 %s\n", Id4);

    // generate id 5
    // SCSIType_VendorId(8)_ProductId(16)_Revision(1)
    RtlZeroMemory(Id5, sizeof(Id5));
    Offset = 0;
    Offset = sprintf(&Id5[Offset], DeviceType);
    Offset += CopyField(InquiryData->VendorId, &Id5[Offset], 8);
    Offset += CopyField(InquiryData->ProductId, &Id5[Offset], 16);
    Offset += CopyField(InquiryData->ProductRevisionLevel, &Id5[Offset], 1);
    Id5Length = strlen(Id5) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId5 %s\n", Id5);

    // generate id 6
    // USBSTOR\SCSIType
    RtlZeroMemory(Id6, sizeof(Id6));
    Offset = 0;
    Offset = sprintf(&Id6[Offset], "USBSTOR\\");
    Offset += sprintf(&Id6[Offset], GenericType);
    Id6Length = strlen(Id6) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId6 %s\n", Id6);

    // generate id 7
    // SCSIType
    RtlZeroMemory(Id7, sizeof(Id7));
    Offset = 0;
    Offset = sprintf(&Id7[Offset], GenericType);
    Id7Length = strlen(Id7) + 1;
    DPRINT("USBSTOR_PdoHandleQueryHardwareId HardwareId7 %s\n", Id7);

    TotalLength = Id1Length + Id2Length + Id3Length + Id4Length + Id5Length + Id6Length + Id7Length + 1;

    Buffer = ExAllocatePoolWithTag(PagedPool, TotalLength * sizeof(WCHAR), USB_STOR_TAG);
    if (!Buffer)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // reset offset
    Offset = 0;
    Length = TotalLength;

    USBSTOR_ConvertToUnicodeString(Id1, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id2, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id3, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id4, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id5, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id6, Length, Offset, Buffer, &Offset);
    USBSTOR_ConvertToUnicodeString(Id7, Length, Offset, Buffer, &Offset);

    ASSERT(Offset + 1 == Length);

    Irp->IoStatus.Information = (ULONG_PTR)Buffer;
    return STATUS_SUCCESS;
}

NTSTATUS
USBSTOR_PdoHandleQueryCompatibleId(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    CHAR Buffer[100] = {0};
    ULONG Length, Offset;
    LPWSTR InstanceId;
    LPCSTR DeviceType;

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->DeviceDescriptor);
    DeviceType = USBSTOR_GetDeviceType((PINQUIRYDATA)&PDODeviceExtension->InquiryData);

    // format instance id
    Length = sprintf(Buffer, "USBSTOR\\%s", DeviceType) + 1;
    Length += sprintf(&Buffer[Length], "USBSTOR\\%s", "RAW") + 2;

    InstanceId = ExAllocatePoolWithTag(PagedPool, Length * sizeof(WCHAR), USB_STOR_TAG);
    if (!InstanceId)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    USBSTOR_ConvertToUnicodeString(Buffer, Length, 0, InstanceId, &Offset);
    USBSTOR_ConvertToUnicodeString(&Buffer[Offset], Length, Offset, InstanceId, &Offset);

    DPRINT("USBSTOR_PdoHandleQueryCompatibleId %S\n", InstanceId);

    Irp->IoStatus.Information = (ULONG_PTR)InstanceId;
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

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    // format instance id
    if (FDODeviceExtension->SerialNumber)
    {
        // using serial number from device
        swprintf(Buffer, L"%s&%c", FDODeviceExtension->SerialNumber->bString, PDODeviceExtension->LUN);
    }
    else
    {
        // use instance count and LUN
        swprintf(Buffer, L"%04lu&%c", FDODeviceExtension->InstanceCount, PDODeviceExtension->LUN);
    }

    Length = wcslen(Buffer) + 1;

    InstanceId = ExAllocatePoolWithTag(PagedPool, Length * sizeof(WCHAR), USB_STOR_TAG);
    if (!InstanceId)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    wcscpy(InstanceId, Buffer);

    DPRINT("USBSTOR_PdoHandleQueryInstanceId %S\n", InstanceId);

    Irp->IoStatus.Information = (ULONG_PTR)InstanceId;
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

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // check if relation type is BusRelations
    if (IoStack->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
    {
        // PDO handles only target device relation
        return Irp->IoStatus.Status;
    }

    DeviceRelations = ExAllocatePoolWithTag(PagedPool, sizeof(DEVICE_RELATIONS), USB_STOR_TAG);
    if (!DeviceRelations)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // initialize device relations
    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = DeviceObject;
    ObReferenceObject(DeviceObject);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
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

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
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
               Status = USBSTOR_PdoHandleQueryDeviceId(DeviceObject, Irp);
               break;
           }
           else if (IoStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
           {
               Status = USBSTOR_PdoHandleQueryHardwareId(DeviceObject, Irp);
               break;
           }
           else if (IoStack->Parameters.QueryId.IdType == BusQueryInstanceID)
           {
               Status = USBSTOR_PdoHandleQueryInstanceId(DeviceObject, Irp);
               break;
           }
           else if (IoStack->Parameters.QueryId.IdType == BusQueryCompatibleIDs)
           {
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
               *DeviceExtension->PDODeviceObject = NULL;
               bDelete = TRUE;
           }
           else
           {
               // device object already marked for deletion
               bDelete = FALSE;
           }

           Irp->IoStatus.Status = STATUS_SUCCESS;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);

           if (bDelete)
           {
               IoDeleteDevice(DeviceObject);
           }
           return STATUS_SUCCESS;
       }
       case IRP_MN_QUERY_CAPABILITIES:
       {
           // just forward irp to lower device
           Status = USBSTOR_SyncForwardIrp(DeviceExtension->LowerDeviceObject, Irp);
           ASSERT(Status == STATUS_SUCCESS);

           if (NT_SUCCESS(Status))
           {
               // check if no unique id
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
           // no-op for PDO
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
            // do nothing
            Status = Irp->IoStatus.Status;
        }
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

NTSTATUS
NTAPI
USBSTOR_SyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx)
{
    KeSetEvent((PKEVENT)Ctx, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*
* @name USBSTOR_SendInternalCdb
*
* Issues an internal SCSI request to device.
* The request is sent in a synchronous way.
*/
static
NTSTATUS
USBSTOR_SendInternalCdb(
    IN PDEVICE_OBJECT PdoDevice,
    IN PCDB Cdb,
    IN UCHAR CdbLength,
    IN ULONG TimeOutValue,
    OUT PVOID OutDataBuffer,
    OUT PULONG OutDataTransferLength)
{
    PSCSI_REQUEST_BLOCK Srb;
    PSENSE_DATA SenseBuffer;
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;
    PIRP Irp = NULL;
    PMDL Mdl = NULL;
    ULONG ix = 0;
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    UCHAR SrbStatus;

    DPRINT("USBSTOR_SendInternalCdb SCSIOP %x\n", Cdb->CDB6GENERIC.OperationCode);

    Srb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(SCSI_REQUEST_BLOCK),
                                USB_STOR_TAG);

    if (Srb)
    {
        SenseBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                            SENSE_BUFFER_SIZE,
                                            USB_STOR_TAG);

        if (SenseBuffer)
        {
            Mdl = IoAllocateMdl(OutDataBuffer,
                    *OutDataTransferLength,
                    FALSE,
                    FALSE,
                    NULL);

            if (!Mdl)
            {
                ExFreePoolWithTag(SenseBuffer, USB_STOR_TAG);
                ExFreePoolWithTag(Srb, USB_STOR_TAG);
                return Status;
            }

            MmBuildMdlForNonPagedPool(Mdl);

            // make 3 attempts - the device may be in STALL state after the first one
            do
            {
                Irp = IoAllocateIrp(PdoDevice->StackSize, FALSE);

                if (!Irp)
                {
                    break;
                }

                IoStack = IoGetNextIrpStackLocation(Irp);
                IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                IoStack->Parameters.Scsi.Srb = Srb;

                RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));

                Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
                Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
                Srb->CdbLength = CdbLength;
                Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
                Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_NO_QUEUE_FREEZE;
                Srb->DataTransferLength = *OutDataTransferLength;
                Srb->TimeOutValue = TimeOutValue;
                Srb->DataBuffer = OutDataBuffer;
                Srb->SenseInfoBuffer = SenseBuffer;

                RtlCopyMemory(Srb->Cdb, Cdb, CdbLength);

                Irp->MdlAddress = Mdl;

                KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

                IoSetCompletionRoutine(Irp,
                                       USBSTOR_SyncCompletionRoutine,
                                       &Event,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                if (IoCallDriver(PdoDevice, Irp) == STATUS_PENDING)
                {
                    KeWaitForSingleObject(&Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                }

                SrbStatus = SRB_STATUS(Srb->SrbStatus);

                IoFreeIrp(Irp);
                Irp = NULL;

                if (SrbStatus == SRB_STATUS_SUCCESS ||
                     SrbStatus == SRB_STATUS_DATA_OVERRUN)
                {
                    Status = STATUS_SUCCESS;
                    *OutDataTransferLength = Srb->DataTransferLength;
                    break;
                }

                Status = STATUS_UNSUCCESSFUL;

                ++ix;
            } while (ix < 3);

            if (Mdl)
            {
                IoFreeMdl(Mdl);
            }

            ExFreePoolWithTag(SenseBuffer, USB_STOR_TAG);
        }

        ExFreePoolWithTag(Srb, USB_STOR_TAG);
    }

    return Status;
}

/*
* @name USBSTOR_FillInquiryData
*
* Sends a SCSI Inquiry request and fills in the PDODeviceExtension->InquiryData field with a result.
*/
static
NTSTATUS
USBSTOR_FillInquiryData(
    IN PDEVICE_OBJECT PDODeviceObject)
{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    PPDO_DEVICE_EXTENSION PDODeviceExtension = (PPDO_DEVICE_EXTENSION)PDODeviceObject->DeviceExtension;
    CDB Cdb;
    ULONG DataTransferLength = INQUIRYDATABUFFERSIZE;
    PINQUIRYDATA InquiryData = (PINQUIRYDATA)&PDODeviceExtension->InquiryData;

    RtlZeroMemory(&Cdb, sizeof(Cdb));
    Cdb.CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    Cdb.CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

    Status = USBSTOR_SendInternalCdb(PDODeviceObject, &Cdb, CDB6GENERIC_LENGTH, 20, InquiryData, &DataTransferLength);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBSTOR_FillInquiryData failed with %x\n", Status);
        return Status;
    }

    DPRINT("DeviceType %x\n", InquiryData->DeviceType);
    DPRINT("DeviceTypeModifier %x\n", InquiryData->DeviceTypeModifier);
    DPRINT("RemovableMedia %x\n", InquiryData->RemovableMedia);
    DPRINT("Version %x\n", InquiryData->Versions);
    DPRINT("Format %x\n", InquiryData->ResponseDataFormat);
    DPRINT("Length %x\n", InquiryData->AdditionalLength);
    DPRINT("Reserved %p\n", InquiryData->Reserved);
    DPRINT("VendorId %c%c%c%c%c%c%c%c\n", InquiryData->VendorId[0], InquiryData->VendorId[1], InquiryData->VendorId[2], InquiryData->VendorId[3], InquiryData->VendorId[4], InquiryData->VendorId[5], InquiryData->VendorId[6], InquiryData->VendorId[7]);
    DPRINT("ProductId %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", InquiryData->ProductId[0], InquiryData->ProductId[1], InquiryData->ProductId[2], InquiryData->ProductId[3],
                                                           InquiryData->ProductId[4], InquiryData->ProductId[5], InquiryData->ProductId[6], InquiryData->ProductId[7],
                                                           InquiryData->ProductId[8], InquiryData->ProductId[9], InquiryData->ProductId[10], InquiryData->ProductId[11],
                                                           InquiryData->ProductId[12], InquiryData->ProductId[13], InquiryData->ProductId[14], InquiryData->ProductId[15]);

    DPRINT("Revision %c%c%c%c\n", InquiryData->ProductRevisionLevel[0], InquiryData->ProductRevisionLevel[1], InquiryData->ProductRevisionLevel[2], InquiryData->ProductRevisionLevel[3]);

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
    PINQUIRYDATA InquiryData;

    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // create child device object
    Status = IoCreateDevice(DeviceObject->DriverObject, sizeof(PDO_DEVICE_EXTENSION), NULL, FILE_DEVICE_MASS_STORAGE, FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN, FALSE, &PDO);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create PDO, status %x\n", Status);
        return Status;
    }

    // patch the stack size
    PDO->StackSize = DeviceObject->StackSize;

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)PDO->DeviceExtension;
    InquiryData = (PINQUIRYDATA)&PDODeviceExtension->InquiryData;

    // initialize device extension
    RtlZeroMemory(PDODeviceExtension, sizeof(PDO_DEVICE_EXTENSION));
    PDODeviceExtension->Common.IsFDO = FALSE;
    PDODeviceExtension->LowerDeviceObject = DeviceObject;
    PDODeviceExtension->PDODeviceObject = &FDODeviceExtension->ChildPDO[LUN];
    PDODeviceExtension->Self = PDO;
    PDODeviceExtension->LUN = LUN;

    PDO->Flags |= DO_DIRECT_IO;

    // device is initialized
    PDO->Flags &= ~DO_DEVICE_INITIALIZING;

    // output device object
    FDODeviceExtension->ChildPDO[LUN] = PDO;

    // send inquiry command by irp
    Status = USBSTOR_FillInquiryData(PDO);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (InquiryData->DeviceType != DIRECT_ACCESS_DEVICE &&
        InquiryData->DeviceType != READ_ONLY_DIRECT_ACCESS_DEVICE)
    {
        return STATUS_NOT_SUPPORTED;
    }

    return Status;
}
