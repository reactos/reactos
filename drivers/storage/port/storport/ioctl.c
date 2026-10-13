/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport IOCTL handling
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

static
UINT32
GetFieldLength(
    _In_ PUCHAR Name,
    _In_ UINT32 MaxLength)
{
    UINT32 Index;
    UINT32 LastCharacterPosition = 0;

    // scan the field and return last position which contains a valid character
    for (Index = 0; Index < MaxLength; Index++)
    {
        if (Name[Index] != ' ')
        {
            // trim white spaces from field
            LastCharacterPosition = Index;
        }
    }

    // convert from zero based index to length
    return LastCharacterPosition + 1;
}


static
NTSTATUS
FdoDeviceControlQueryProperty(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PMINIPORT Miniport;
    PSTORAGE_ADAPTER_DESCRIPTOR AdapterDescriptor;
    PSTORAGE_PROPERTY_QUERY Query;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
    Miniport = &FdoExtension->Miniport;

    do
    {
        /* check property type (handle only StorageAdapterProperty) */
        if (Query->PropertyId != StorageAdapterProperty)
        {
            if (Query->PropertyId == StorageDeviceProperty ||
                Query->PropertyId == StorageDeviceIdProperty)
            {
                Status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER_1;
            }

            break;
        }

        /* check query type */
        if (Query->QueryType == PropertyExistsQuery)
        {
            /* device property / adapter property is supported */
            Status = STATUS_SUCCESS;
            break;
        }

        if (Query->QueryType != PropertyStandardQuery)
        {
            /* only standard query and exists query are supported */
            Status = STATUS_INVALID_PARAMETER_2;
            break;
        }

        /* Check buffer length */
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(*AdapterDescriptor))
        {
            PSTORAGE_DESCRIPTOR_HEADER DescriptorHeader = Irp->AssociatedIrp.SystemBuffer;

            /* Fail request if the buffer is simply too small */
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(STORAGE_DESCRIPTOR_HEADER))
            {
                Status = STATUS_BUFFER_TOO_SMALL; /* FIXME: Is this code appropriate? */
                break;
            }

            /* Return required size */
            DescriptorHeader->Version = sizeof(*AdapterDescriptor);
            DescriptorHeader->Size = sizeof(*AdapterDescriptor);
            Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            Status = STATUS_SUCCESS;
            break;
        }

        /* Return AdapterDescriptor */
        AdapterDescriptor = Irp->AssociatedIrp.SystemBuffer;
        RtlZeroMemory(AdapterDescriptor, sizeof(*AdapterDescriptor));

        AdapterDescriptor->Version = sizeof(*AdapterDescriptor);
        AdapterDescriptor->Size = sizeof(*AdapterDescriptor);
        AdapterDescriptor->MaximumTransferLength = Miniport->PortConfig.MaximumTransferLength;
        AdapterDescriptor->MaximumPhysicalPages = Miniport->PortConfig.NumberOfPhysicalBreaks;
        AdapterDescriptor->AlignmentMask = Miniport->PortConfig.AlignmentMask;
        AdapterDescriptor->AdapterUsesPio = FALSE; /* Storport requirement */
        AdapterDescriptor->AdapterScansDown = Miniport->PortConfig.AdapterScansDown;
        AdapterDescriptor->CommandQueueing = TRUE; /* Storport requirement */
        AdapterDescriptor->AcceleratedTransfer = TRUE;
        AdapterDescriptor->BusType = BusTypeSata; /* FIXME: ＲＥＡＤ　ＦＲＯＭ　ＲＥＧＩＳＴＲＹ */
        AdapterDescriptor->BusMajorVersion = 2;
        AdapterDescriptor->BusMinorVersion = 0;
        /* We translate for the miniport, so the class layer keeps sending legacy blocks */
        AdapterDescriptor->SrbType = SRB_TYPE_SCSI_REQUEST_BLOCK;
        AdapterDescriptor->AddressType = STORAGE_ADDRESS_TYPE_BTL8;

        Irp->IoStatus.Information = sizeof(*AdapterDescriptor);
        Status = STATUS_SUCCESS;
    }
    while (0);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
NTAPI
PortFdoDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_STORAGE_QUERY_PROPERTY:
            return FdoDeviceControlQueryProperty(DeviceObject->DeviceExtension, Irp);

        default:
            // __debugbreak();
            Status = Irp->IoStatus.Status;
            break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


static
NTSTATUS
PdoDeviceControlQueryProperty(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PSTORAGE_PROPERTY_QUERY Query;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;

    do
    {
        /* check property type */
        if (Query->PropertyId != StorageDeviceProperty &&
            Query->PropertyId != StorageAdapterProperty)
        {
            // only device property / adapter property are supported
            Status = STATUS_INVALID_PARAMETER_1;
        }

        /* check query type */
        if (Query->QueryType == PropertyExistsQuery)
        {
            /* device property / adapter property is supported */
            Status = STATUS_SUCCESS;
            break;
        }

        if (Query->QueryType != PropertyStandardQuery)
        {
            /* only standard query and exists query are supported */
            Status = STATUS_INVALID_PARAMETER_2;
            break;
        }



        /* For different property types... */
        switch (Query->PropertyId)
        {
            case StorageDeviceProperty:
            {
                PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;
                PINQUIRYDATA InquiryData = PdoExtension->InquiryBuffer;
                ULONG VendorIdLength = GetFieldLength(InquiryData->VendorId, 8);
                ULONG ProductIdLength = GetFieldLength(InquiryData->ProductId, 16);
                ULONG RevisionLength = GetFieldLength(InquiryData->ProductRevisionLevel, 4);
                ULONG TotalLength;
                PUCHAR Dest;

                /*
                 * total length required is
                 *     sizeof(STORAGE_DEVICE_DESCRIPTOR) + (String + NullByte) + (...) + ... - 1
                 * STORAGE_DEVICE_DESCRIPTOR contains 1 byte at the end already
                 */
                 TotalLength = sizeof(STORAGE_DEVICE_DESCRIPTOR) +
                               VendorIdLength + 1 +
                               ProductIdLength + 1 +
                               RevisionLength + /* Null byte omitted */
                               1; /* Null byte for serial number */

                /* Check buffer length */
                if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < TotalLength)
                {
                    PSTORAGE_DESCRIPTOR_HEADER DescriptorHeader = Irp->AssociatedIrp.SystemBuffer;

                    /* Fail request if the buffer is simply too small */
                    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength <
                        sizeof(STORAGE_DESCRIPTOR_HEADER))
                    {
                        Status = STATUS_BUFFER_TOO_SMALL; /* FIXME: Is this code appropriate? */
                        break;
                    }

                    /* Return required size */
                    DescriptorHeader->Version = TotalLength;
                    DescriptorHeader->Size = TotalLength;
                    Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
                    Status = STATUS_SUCCESS;
                    break;
                }

                /* Fill in data */
                DeviceDescriptor = Irp->AssociatedIrp.SystemBuffer;
                *DeviceDescriptor = (STORAGE_DEVICE_DESCRIPTOR){
                    .Version = sizeof(STORAGE_DEVICE_DESCRIPTOR),
                    .Size = TotalLength,
                    .DeviceType = InquiryData->DeviceType,
                    .DeviceTypeModifier = InquiryData->DeviceTypeModifier,
                    .RemovableMedia = InquiryData->RemovableMedia,
                    .CommandQueueing = InquiryData->CommandQueue,
                    .BusType = BusTypeSata, /* FIXME: USE FDO BUS TYPE FROM REGISTRY */
                };

                /* Arbitrary data offsets, set them cascading */
                DeviceDescriptor->VendorIdOffset =
                    FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties);
                DeviceDescriptor->ProductIdOffset = DeviceDescriptor->VendorIdOffset +
                                                    VendorIdLength + 1;
                DeviceDescriptor->ProductRevisionOffset = DeviceDescriptor->ProductIdOffset +
                                                          ProductIdLength + 1;
                DeviceDescriptor->SerialNumberOffset = DeviceDescriptor->ProductRevisionOffset +
                                                       RevisionLength + 1;

                /* Fill the arbitrary data */
                Dest = DeviceDescriptor->RawDeviceProperties;

                RtlCopyMemory(Dest, InquiryData->VendorId, VendorIdLength);
                Dest[VendorIdLength] = '\0';
                Dest += VendorIdLength + 1;

                RtlCopyMemory(Dest, InquiryData->ProductId, ProductIdLength);
                Dest[ProductIdLength] = '\0';
                Dest += ProductIdLength + 1;

                RtlCopyMemory(Dest, InquiryData->ProductRevisionLevel, RevisionLength);
                Dest[RevisionLength] = '\0';
                Dest += RevisionLength + 1;

                /* FIXME: Read serial number when enumerating devices */
                *Dest = '\0';

                /* TODO: Verify this doesnt overflow */
                Irp->IoStatus.Information = TotalLength;
                Status = STATUS_SUCCESS;
                break;
            }
            case StorageAdapterProperty:
            {
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(PdoExtension->FdoExtension->Device, Irp);
            }
            case StorageDevicePowerProperty:
            default:
            {
                Status = STATUS_NOT_IMPLEMENTED;
                break;
            }
        }
    }
    while (0);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


static
NTSTATUS
PdoDeviceControlScsiGetAddress(
    _In_ PPDO_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp)
{
    NTSTATUS Status;
    PSCSI_ADDRESS ScsiAddress;
    PIO_STACK_LOCATION IoStack;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;

    DPRINT("PdoDeviceControlScsiGetAddress(%p %p)\n", DeviceExtension, Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NT_ASSERT(DeviceExtension->ExtensionType == PdoExtension);
    FdoDeviceExtension = DeviceExtension->FdoExtension;

    do
    {
        /* Check buffer length */
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SCSI_ADDRESS))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;
        }

        ScsiAddress = (PSCSI_ADDRESS)Irp->AssociatedIrp.SystemBuffer;
        ScsiAddress->Length = sizeof(SCSI_ADDRESS);
        ScsiAddress->PortNumber = FdoDeviceExtension->ScsiPortNumber;
        ScsiAddress->PathId = DeviceExtension->Bus;
        ScsiAddress->TargetId = DeviceExtension->Target;
        ScsiAddress->Lun = DeviceExtension->Lun;

        Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(SCSI_ADDRESS);

        DPRINT("SCSI Address for PDO %p : (Path Target Lun)(%d %d %d)\n",
               DeviceExtension->Device, ScsiAddress->PathId, ScsiAddress->TargetId, ScsiAddress->Lun);
    }
    while (0);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
NTAPI
PortPdoDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_STORAGE_QUERY_PROPERTY:
            return PdoDeviceControlQueryProperty(DeviceObject->DeviceExtension, Irp);

        case IOCTL_SCSI_GET_ADDRESS:
            return PdoDeviceControlScsiGetAddress(DeviceObject->DeviceExtension, Irp);

        default:
            // DbgBreakPoint();
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


/* EOF */
