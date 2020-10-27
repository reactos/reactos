/*
 * PROJECT:     ReactOS Storage Stack / SCSIPORT storage port library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     IOCTL handlers
 * COPYRIGHT:   Eric Kohl (eric.kohl@reactos.org)
 *              Aleksey Bragin (aleksey@reactos.org)
 *              2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include "scsiport.h"

#define NDEBUG
#include <debug.h>


static
NTSTATUS
SpiGetInquiryData(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp)
{
    ULONG InquiryDataSize;
    ULONG BusCount, Length;
    PIO_STACK_LOCATION IrpStack;
    PSCSI_ADAPTER_BUS_INFO AdapterBusInfo;
    PSCSI_INQUIRY_DATA InquiryData;
    PUCHAR Buffer;

    DPRINT("SpiGetInquiryData() called\n");

    /* Get pointer to the buffer */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    /* Initialize bus and LUN counters */
    BusCount = DeviceExtension->NumberOfBuses;

    /* Calculate size of inquiry data, rounding up to sizeof(ULONG) */
    InquiryDataSize = ALIGN_UP(sizeof(SCSI_INQUIRY_DATA) - 1 + INQUIRYDATABUFFERSIZE, ULONG);

    /* Calculate data size */
    Length = sizeof(SCSI_ADAPTER_BUS_INFO) + (BusCount - 1) * sizeof(SCSI_BUS_DATA);

    Length += InquiryDataSize * DeviceExtension->TotalLUCount;

    /* Check, if all data is going to fit into provided buffer */
    if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < Length)
    {
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Store data size in the IRP */
    Irp->IoStatus.Information = Length;

    DPRINT("Data size: %lu\n", Length);

    AdapterBusInfo = (PSCSI_ADAPTER_BUS_INFO)Buffer;

    AdapterBusInfo->NumberOfBuses = (UCHAR)BusCount;

    /* Point InquiryData to the corresponding place inside Buffer */
    InquiryData = (PSCSI_INQUIRY_DATA)(Buffer + sizeof(SCSI_ADAPTER_BUS_INFO) +
                    (BusCount - 1) * sizeof(SCSI_BUS_DATA));

    /* Loop each bus */
    for (UINT8 pathId = 0; pathId < DeviceExtension->NumberOfBuses; pathId++)
    {
        PSCSI_BUS_DATA BusData = &AdapterBusInfo->BusData[pathId];

        /* Calculate and save an offset of the inquiry data */
        BusData->InquiryDataOffset = (ULONG)((PUCHAR)InquiryData - Buffer);

        /* Store Initiator Bus Id */
        BusData->InitiatorBusId = DeviceExtension->Buses[pathId].BusIdentifier;

        /* Store LUN count */
        BusData->NumberOfLogicalUnits = DeviceExtension->Buses[pathId].LogicalUnitsCount;

        /* Loop all LUNs */
        PSCSI_BUS_INFO bus = &DeviceExtension->Buses[pathId];

        for (PLIST_ENTRY lunEntry = bus->LunsListHead.Flink;
             lunEntry != &bus->LunsListHead;
             lunEntry = lunEntry->Flink)
        {
            PSCSI_PORT_LUN_EXTENSION lunExt =
                CONTAINING_RECORD(lunEntry, SCSI_PORT_LUN_EXTENSION, LunEntry);

            DPRINT("(Bus %lu Target %lu Lun %lu)\n", pathId, lunExt->TargetId, lunExt->Lun);

            /* Fill InquiryData with values */
            InquiryData->PathId = lunExt->PathId;
            InquiryData->TargetId = lunExt->TargetId;
            InquiryData->Lun = lunExt->Lun;
            InquiryData->InquiryDataLength = INQUIRYDATABUFFERSIZE;
            InquiryData->DeviceClaimed = lunExt->DeviceClaimed;
            InquiryData->NextInquiryDataOffset =
                (ULONG)((PUCHAR)InquiryData + InquiryDataSize - Buffer);

            /* Copy data in it */
            RtlCopyMemory(InquiryData->InquiryData,
                          &lunExt->InquiryData,
                          INQUIRYDATABUFFERSIZE);

            /* Move to the next LUN */
            InquiryData = (PSCSI_INQUIRY_DATA) ((ULONG_PTR)InquiryData + InquiryDataSize);
        }

        /* Either mark the end, or set offset to 0 */
        if (BusData->NumberOfLogicalUnits != 0)
            ((PSCSI_INQUIRY_DATA) ((PCHAR)InquiryData - InquiryDataSize))->NextInquiryDataOffset = 0;
        else
            BusData->InquiryDataOffset = 0;
    }

    /* Finish with success */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

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
PdoHandleQueryProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_PORT_LUN_EXTENSION lunExt = DeviceObject->DeviceExtension;
    NTSTATUS status;

    ASSERT(ioStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(STORAGE_PROPERTY_QUERY));
    ASSERT(Irp->AssociatedIrp.SystemBuffer);
    ASSERT(!lunExt->Common.IsFDO);

    PSTORAGE_PROPERTY_QUERY PropertyQuery = Irp->AssociatedIrp.SystemBuffer;

    // check property type
    if (PropertyQuery->PropertyId != StorageDeviceProperty &&
        PropertyQuery->PropertyId != StorageAdapterProperty)
    {
        // only device property / adapter property are supported
        status = STATUS_INVALID_PARAMETER_1;
        goto completeIrp;
    }

    // check query type
    if (PropertyQuery->QueryType == PropertyExistsQuery)
    {
        // device property / adapter property is supported
        status = STATUS_SUCCESS;
        goto completeIrp;
    }

    if (PropertyQuery->QueryType != PropertyStandardQuery)
    {
        // only standard query and exists query are supported
        status = STATUS_INVALID_PARAMETER_2;
        goto completeIrp;
    }

    switch (PropertyQuery->PropertyId)
    {
        case StorageDeviceProperty:
        {
            PINQUIRYDATA inquiryData = &lunExt->InquiryData;

            // compute extra parameters length
            UINT32 FieldLengthVendor = GetFieldLength(inquiryData->VendorId, 8),
                FieldLengthProduct = GetFieldLength(inquiryData->ProductId, 16),
                FieldLengthRevision = GetFieldLength(inquiryData->ProductRevisionLevel, 4);

            // total length required is sizeof(STORAGE_DEVICE_DESCRIPTOR) + FieldLength + 4 extra null bytes - 1
            // -1 due STORAGE_DEVICE_DESCRIPTOR contains one byte length of parameter data
            UINT32 TotalLength = sizeof(STORAGE_DEVICE_DESCRIPTOR)
                                 + FieldLengthVendor
                                 + FieldLengthProduct
                                 + FieldLengthRevision
                                 + 3;

            // check if output buffer is long enough
            if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < TotalLength)
            {
                // buffer too small
                PSTORAGE_DESCRIPTOR_HEADER DescriptorHeader = Irp->AssociatedIrp.SystemBuffer;
                ASSERT(ioStack->Parameters.DeviceIoControl.OutputBufferLength >=
                       sizeof(STORAGE_DESCRIPTOR_HEADER));

                // return required size
                DescriptorHeader->Version = TotalLength;
                DescriptorHeader->Size = TotalLength;

                Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
                status = STATUS_SUCCESS;
                goto completeIrp;
            }

            // initialize the device descriptor
            PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor = Irp->AssociatedIrp.SystemBuffer;

            deviceDescriptor->Version = sizeof(STORAGE_DEVICE_DESCRIPTOR);
            deviceDescriptor->Size = TotalLength;
            deviceDescriptor->DeviceType = inquiryData->DeviceType;
            deviceDescriptor->DeviceTypeModifier = inquiryData->DeviceTypeModifier;
            deviceDescriptor->RemovableMedia = inquiryData->RemovableMedia;
            deviceDescriptor->CommandQueueing = inquiryData->CommandQueue;
            deviceDescriptor->BusType = BusTypeScsi;
            deviceDescriptor->VendorIdOffset =
                FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties);
            deviceDescriptor->ProductIdOffset =
                deviceDescriptor->VendorIdOffset + FieldLengthVendor + 1;
            deviceDescriptor->ProductRevisionOffset =
                deviceDescriptor->ProductIdOffset + FieldLengthProduct + 1;
            deviceDescriptor->SerialNumberOffset = 0;
            deviceDescriptor->RawPropertiesLength =
                FieldLengthVendor + FieldLengthProduct + FieldLengthRevision + 3;

            // copy descriptors
            PUCHAR Buffer = deviceDescriptor->RawDeviceProperties;

            RtlCopyMemory(Buffer, inquiryData->VendorId, FieldLengthVendor);
            Buffer[FieldLengthVendor] = '\0';
            Buffer += FieldLengthVendor + 1;

            RtlCopyMemory(Buffer, inquiryData->ProductId, FieldLengthProduct);
            Buffer[FieldLengthProduct] = '\0';
            Buffer += FieldLengthProduct + 1;

            RtlCopyMemory(Buffer, inquiryData->ProductRevisionLevel, FieldLengthRevision);
            Buffer[FieldLengthRevision] = '\0';
            Buffer += FieldLengthRevision + 1;

            DPRINT("Vendor %s\n",
                (LPCSTR)((ULONG_PTR)deviceDescriptor + deviceDescriptor->VendorIdOffset));
            DPRINT("Product %s\n",
                (LPCSTR)((ULONG_PTR)deviceDescriptor + deviceDescriptor->ProductIdOffset));
            DPRINT("Revision %s\n",
                (LPCSTR)((ULONG_PTR)deviceDescriptor + deviceDescriptor->ProductRevisionOffset));

            Irp->IoStatus.Information = TotalLength;
            status = STATUS_SUCCESS;
            goto completeIrp;
        }
        case StorageAdapterProperty:
        {
            // forward to the lower device
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(lunExt->Common.LowerDevice, Irp);
        }
        case StorageDeviceIdProperty:
        {
            // TODO
        }
        default:
        {
            UNREACHABLE;
            status = STATUS_NOT_IMPLEMENTED;
            goto completeIrp;
        }
    }

completeIrp:
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

static
NTSTATUS
FdoHandleQueryProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_PORT_DEVICE_EXTENSION portExt = DeviceObject->DeviceExtension;
    NTSTATUS status;

    ASSERT(ioStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(STORAGE_PROPERTY_QUERY));
    ASSERT(Irp->AssociatedIrp.SystemBuffer);
    ASSERT(portExt->Common.IsFDO);

    PSTORAGE_PROPERTY_QUERY PropertyQuery = Irp->AssociatedIrp.SystemBuffer;

    // check property type (handle only StorageAdapterProperty)
    if (PropertyQuery->PropertyId != StorageAdapterProperty)
    {
        if (PropertyQuery->PropertyId == StorageDeviceProperty ||
            PropertyQuery->PropertyId == StorageDeviceIdProperty)
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            status = STATUS_INVALID_PARAMETER_1;
        }

        goto completeIrp;
    }

    // check query type
    if (PropertyQuery->QueryType == PropertyExistsQuery)
    {
        // device property / adapter property is supported
        status = STATUS_SUCCESS;
        goto completeIrp;
    }

    if (PropertyQuery->QueryType != PropertyStandardQuery)
    {
        // only standard query and exists query are supported
        status = STATUS_INVALID_PARAMETER_2;
        goto completeIrp;
    }

    if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_ADAPTER_DESCRIPTOR))
    {
        // buffer too small
        PSTORAGE_DESCRIPTOR_HEADER DescriptorHeader = Irp->AssociatedIrp.SystemBuffer;
        ASSERT(ioStack->Parameters.DeviceIoControl.OutputBufferLength
               >= sizeof(STORAGE_DESCRIPTOR_HEADER));

        // return required size
        DescriptorHeader->Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
        DescriptorHeader->Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR);

        Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
        status = STATUS_SUCCESS;
        goto completeIrp;
    }

    // get adapter descriptor, information is returned in the same buffer
    PSTORAGE_ADAPTER_DESCRIPTOR adapterDescriptor = Irp->AssociatedIrp.SystemBuffer;

    // fill out descriptor
    // NOTE: STORAGE_ADAPTER_DESCRIPTOR may vary in size, so it's important to zero out
    // all unused fields
    *adapterDescriptor = (STORAGE_ADAPTER_DESCRIPTOR) {
        .Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR),
        .Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR),
        .MaximumTransferLength = portExt->PortCapabilities.MaximumTransferLength,
        .MaximumPhysicalPages = portExt->PortCapabilities.MaximumPhysicalPages,
        .AlignmentMask = portExt->PortCapabilities.AlignmentMask,
        .AdapterUsesPio = portExt->PortCapabilities.AdapterUsesPio,
        .AdapterScansDown = portExt->PortCapabilities.AdapterScansDown,
        .CommandQueueing = portExt->PortCapabilities.TaggedQueuing,
        .AcceleratedTransfer = TRUE,
        .BusType = BusTypeScsi, // FIXME
        .BusMajorVersion = 2,
        .BusMinorVersion = 0
    };

    // store returned length
    Irp->IoStatus.Information = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
    status = STATUS_SUCCESS;

completeIrp:
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/**********************************************************************
 * NAME                         INTERNAL
 *  ScsiPortDeviceControl
 *
 * DESCRIPTION
 *  Answer requests for device control calls
 *
 * RUN LEVEL
 *  PASSIVE_LEVEL
 *
 * ARGUMENTS
 *  Standard dispatch arguments
 *
 * RETURNS
 *  NTSTATUS
 */

NTSTATUS
NTAPI
ScsiPortDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    PSCSI_PORT_COMMON_EXTENSION comExt = DeviceObject->DeviceExtension;
    PSCSI_PORT_DEVICE_EXTENSION portExt;
    PSCSI_PORT_LUN_EXTENSION lunExt;
    NTSTATUS status;

    DPRINT("ScsiPortDeviceControl()\n");

    Irp->IoStatus.Information = 0;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_STORAGE_QUERY_PROPERTY:
        {
            DPRINT("  IOCTL_STORAGE_QUERY_PROPERTY\n");

            if (!VerifyIrpInBufferSize(Irp, sizeof(STORAGE_PROPERTY_QUERY)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if (comExt->IsFDO)
                return FdoHandleQueryProperty(DeviceObject, Irp);
            else
                return PdoHandleQueryProperty(DeviceObject, Irp);
        }
        case IOCTL_SCSI_GET_ADDRESS:
        {
            DPRINT("  IOCTL_SCSI_GET_ADDRESS\n");

            if (comExt->IsFDO)
            {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            PSCSI_ADDRESS address = Irp->AssociatedIrp.SystemBuffer;
            if (!VerifyIrpOutBufferSize(Irp, sizeof(*address)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            lunExt = DeviceObject->DeviceExtension;
            portExt = comExt->LowerDevice->DeviceExtension;

            address->Length = sizeof(SCSI_ADDRESS);
            address->PortNumber = portExt->PortNumber;
            address->PathId = lunExt->PathId;
            address->TargetId = lunExt->TargetId;
            address->Lun = lunExt->Lun;

            Irp->IoStatus.Information = sizeof(SCSI_ADDRESS);
            status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_SCSI_GET_DUMP_POINTERS:
        {
            DPRINT("  IOCTL_SCSI_GET_DUMP_POINTERS\n");

            if (!comExt->IsFDO)
            {
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(comExt->LowerDevice, Irp);
            }

            PDUMP_POINTERS dumpPointers = Irp->AssociatedIrp.SystemBuffer;
            if (!VerifyIrpOutBufferSize(Irp, sizeof(*dumpPointers)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            dumpPointers->DeviceObject = DeviceObject;
            /* More data.. ? */

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(DUMP_POINTERS);
            break;
        }
        case IOCTL_SCSI_GET_CAPABILITIES:
        {
            DPRINT("  IOCTL_SCSI_GET_CAPABILITIES\n");

            if (!comExt->IsFDO)
            {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            if (!VerifyIrpOutBufferSize(Irp, sizeof(IO_SCSI_CAPABILITIES)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            portExt = DeviceObject->DeviceExtension;

            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                          &portExt->PortCapabilities,
                          sizeof(IO_SCSI_CAPABILITIES));

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(IO_SCSI_CAPABILITIES);
            break;
        }
        case IOCTL_SCSI_GET_INQUIRY_DATA:
        {
            DPRINT("  IOCTL_SCSI_GET_INQUIRY_DATA\n");

            if (!comExt->IsFDO)
            {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            /* Copy inquiry data to the port device extension */
            status = SpiGetInquiryData(DeviceObject->DeviceExtension, Irp);
            break;
        }
        case IOCTL_SCSI_MINIPORT:
            DPRINT1("IOCTL_SCSI_MINIPORT unimplemented!\n");
            status = STATUS_NOT_IMPLEMENTED;
            break;

        case IOCTL_SCSI_PASS_THROUGH:
            DPRINT1("IOCTL_SCSI_PASS_THROUGH unimplemented!\n");
            status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            DPRINT1("unknown ioctl code: 0x%lX\n", Stack->Parameters.DeviceIoControl.IoControlCode);
            status = STATUS_NOT_SUPPORTED;
            break;
    }

    /* Complete the request with the given status */
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}
