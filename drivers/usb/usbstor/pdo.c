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
        if (Name[Index] == '\0')
            return Index;

        Buffer[Index] = Name[Index];
    }

    return MaxLength;
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
    ULONG Offset = 0, Index;
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
    Offset = sprintf(&Buffer[Offset], "USBSTOR\\%s&Ven_", DeviceType);

    //
    // copy vendor id
    //
    Offset += CopyField(InquiryData->Vendor, &Buffer[Offset], 8);

    //
    // copy product string
    //
    Offset += sprintf(&Buffer[Offset], "&Prod_");

    //
    // copy product identifier
    //
    Offset += CopyField(InquiryData->Product, &Buffer[Offset], 16);

    //
    // copy revision string
    //
    Offset += sprintf(&Buffer[Offset], "&Rev_");

    //
    // copy revision identifer
    //
    Offset += CopyField(InquiryData->Revision, &Buffer[Offset], 4);

    //
    // FIXME: device serial number
    //
    Offset +=sprintf(&Buffer[Offset], "\\00000000&%d", DeviceExtension->LUN);

    //
    // now convert restricted characters to underscores
    //
    for(Index = 0; Index < Offset; Index++)
    {
        if (Buffer[Index] <= ' ' || Buffer[Index] >= 0x7F /* last printable ascii character */ ||  Buffer[Index] == ',')
        {
            //
            // convert to underscore
            //
            Buffer[Index] = '_';
        }
    }

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

NTSTATUS
USBSTOR_PdoHandleQueryHardwareId(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    WCHAR Buffer[200];
    ULONG Length;
    LPWSTR DeviceName;
    LPCWSTR GenericType;

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
    // get generic type
    //
    GenericType = USBSTOR_GetGenericType((PUFI_INQUIRY_RESPONSE)PDODeviceExtension->InquiryData);
    ASSERT(GenericType);

    //
    // zero buffer
    //
    RtlZeroMemory(Buffer, sizeof(Buffer));

    //
    // format hardware id
    //
    Length = swprintf(Buffer, L"USB\\VID_%04x&Pid_%04x&Rev_%04x", FDODeviceExtension->DeviceDescriptor->idVendor, FDODeviceExtension->DeviceDescriptor->idProduct, FDODeviceExtension->DeviceDescriptor->bcdDevice) + 1;
    Length += swprintf(&Buffer[Length], L"USB\\VID_%04x&Pid_%04x", FDODeviceExtension->DeviceDescriptor->idVendor, FDODeviceExtension->DeviceDescriptor->idProduct) + 1;
    Length += swprintf(&Buffer[Length], L"USBSTOR\\%s", GenericType) + 1;
    Length += swprintf(&Buffer[Length], L"%s", GenericType) + 1;

    //
    // TODO: add more ids
    //

    Buffer[Length] = UNICODE_NULL;
    Length++;

    DPRINT1("Name %S\n", Buffer);

    //
    // allocate buffer
    //
    DeviceName = (LPWSTR)AllocateItem(PagedPool, Length * sizeof(WCHAR));

    if (!DeviceName)
    {
        //
        // no memory
        //
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy device name
    //
    RtlMoveMemory(DeviceName, Buffer, Length * sizeof(WCHAR));

    //
    // store result
    //
    Irp->IoStatus.Information = (ULONG_PTR)DeviceName;

    //
    // done
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
    swprintf(Buffer, L"USB\\VID_%04x&PID_%04x\\%s", FDODeviceExtension->DeviceDescriptor->idVendor, FDODeviceExtension->DeviceDescriptor->idProduct, L"00000000");

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
           DPRINT1("USBSTOR_PdoHandlePnp: IRP_MN_QUERY_DEVICE_TEXT unimplemented\n");
           Status = STATUS_NOT_SUPPORTED;
           break;
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

           if (NT_SUCCESS(Status))
           {
               //
               // check if no unique id
               //
               Caps = (PDEVICE_CAPABILITIES)IoStack->Parameters.DeviceCapabilities.Capabilities;
               Caps->UniqueID = FALSE; //FIXME
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
    Status = IoCreateDevice(DeviceObject->DriverObject, sizeof(PDO_DEVICE_EXTENSION), NULL, FILE_DEVICE_MASS_STORAGE, 0, FALSE, &PDO);
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
    PDO->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;

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
