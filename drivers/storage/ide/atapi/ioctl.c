/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     I/O control handling
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

#define AtaVerifyInBuffer(Stack, Struct) \
    ((Stack)->Parameters.DeviceIoControl.InputBufferLength >= sizeof(Struct))

#define AtaVerifyOutBuffer(Stack, Struct) \
    ((Stack)->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(Struct))

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
BOOLEAN
AtaCheckPropertyQuery(
    _Inout_ PIRP Irp,
    _In_ ULONG DescriptorSize,
    _Out_ NTSTATUS* Status)
{
    PSTORAGE_PROPERTY_QUERY PropertyQuery = Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    /* Check the type of a property query */
    if (PropertyQuery->QueryType != PropertyStandardQuery &&
        PropertyQuery->QueryType != PropertyExistsQuery)
    {
        *Status = STATUS_NOT_SUPPORTED;
        return FALSE;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (!AtaVerifyOutBuffer(IoStack, STORAGE_DESCRIPTOR_HEADER))
    {
        *Status = STATUS_INFO_LENGTH_MISMATCH;
        return FALSE;
    }

    /* The requested property is supported */
    if (PropertyQuery->QueryType == PropertyExistsQuery)
    {
        *Status = STATUS_SUCCESS;
        return FALSE;
    }

    /* Caller can determine required size based upon DescriptorHeader */
    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < DescriptorSize)
    {
        PSTORAGE_DESCRIPTOR_HEADER DescriptorHeader = (PSTORAGE_DESCRIPTOR_HEADER)PropertyQuery;

        DescriptorHeader->Version = DescriptorSize;
        DescriptorHeader->Size = DescriptorSize;

        Irp->IoStatus.Information = sizeof(*DescriptorHeader);

        *Status = STATUS_SUCCESS;
        return FALSE;
    }

    return TRUE;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryStorageDeviceProperty(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PINQUIRYDATA InquiryData = &DevExt->InquiryData;
    PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;
    PUCHAR Buffer;
    NTSTATUS Status;
    ULONG DescriptorSize, RawPropertiesLength, Length1, Length2, Length3;

    PAGED_CODE();

    Length1 = strlen(DevExt->FriendlyName) + sizeof(ANSI_NULL);
    Length2 = strlen(DevExt->RevisionNumber) + sizeof(ANSI_NULL);
    Length3 = strlen(DevExt->SerialNumber) + sizeof(ANSI_NULL);
    RawPropertiesLength = Length1 + Length2 + Length3;

    DescriptorSize = FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties) +
                     RawPropertiesLength;

    if (!AtaCheckPropertyQuery(Irp, DescriptorSize, &Status))
        return Status;

    DeviceDescriptor = Irp->AssociatedIrp.SystemBuffer;
    DeviceDescriptor->RawPropertiesLength = RawPropertiesLength;
    DeviceDescriptor->Version = sizeof(*DeviceDescriptor);
    DeviceDescriptor->Size = DescriptorSize;
    DeviceDescriptor->DeviceType = InquiryData->DeviceType;
    DeviceDescriptor->DeviceTypeModifier = InquiryData->DeviceTypeModifier;
    DeviceDescriptor->RemovableMedia = InquiryData->RemovableMedia;
    DeviceDescriptor->CommandQueueing = FALSE; /* Disable request tagging */
    DeviceDescriptor->BusType = BusTypeAta;

    /* Property 1: The vendor ID. We return a NULL string here */
    DeviceDescriptor->VendorIdOffset = 0;

    /* Property 2: The product ID */
    DeviceDescriptor->ProductIdOffset =
        FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties);

    Buffer = (PUCHAR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->ProductIdOffset);

    RtlCopyMemory(Buffer, DevExt->FriendlyName, Length1);

    Buffer += Length1;

    /* Property 3: The product revision */
    DeviceDescriptor->ProductRevisionOffset = DeviceDescriptor->ProductIdOffset + Length1;

    RtlCopyMemory(Buffer, DevExt->RevisionNumber, Length2);

    Buffer += Length2;

    /* Property 4: The serial number */
    DeviceDescriptor->SerialNumberOffset = DeviceDescriptor->ProductRevisionOffset + Length2;

    RtlCopyMemory(Buffer, DevExt->SerialNumber, Length3);

    Irp->IoStatus.Information = DescriptorSize;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryStorageAccessAlignmentProperty(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR AccessAlignmentDescriptor;
    ULONG LogicalSectorsPerPhysicalSector, Dummy;
    NTSTATUS Status;

    PAGED_CODE();

    if (IS_ATAPI(DevExt))
        return STATUS_NOT_SUPPORTED;

    if (!AtaCheckPropertyQuery(Irp, sizeof(*AccessAlignmentDescriptor), &Status))
        return Status;

    AccessAlignmentDescriptor = Irp->AssociatedIrp.SystemBuffer;
    AccessAlignmentDescriptor->Version = sizeof(*AccessAlignmentDescriptor);
    AccessAlignmentDescriptor->Size = sizeof(*AccessAlignmentDescriptor);
    AccessAlignmentDescriptor->BytesPerCacheLine = 0;
    AccessAlignmentDescriptor->BytesOffsetForCacheAlignment = 0;
    AccessAlignmentDescriptor->BytesPerLogicalSector = DevExt->SectorSize;

    LogicalSectorsPerPhysicalSector =
        AtaLogicalSectorsPerPhysicalSector(&DevExt->IdentifyDeviceData, &Dummy);

    AccessAlignmentDescriptor->BytesPerPhysicalSector =
        DevExt->SectorSize * LogicalSectorsPerPhysicalSector;

    AccessAlignmentDescriptor->BytesOffsetForSectorAlignment =
        DevExt->SectorSize *
        AtaLogicalSectorAlignment(&DevExt->IdentifyDeviceData);

    Irp->IoStatus.Information = sizeof(*AccessAlignmentDescriptor);
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryStorageDeviceSeekPenaltyProperty(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PDEVICE_SEEK_PENALTY_DESCRIPTOR PenaltyDescriptor;
    NTSTATUS Status;
    BOOLEAN IncursSeekPenalty;

    PAGED_CODE();

    if (!AtaCheckPropertyQuery(Irp, sizeof(*PenaltyDescriptor), &Status))
        return Status;

    if (AtaDeviceIsRotatingDevice(&DevExt->IdentifyDeviceData))
        IncursSeekPenalty = TRUE;
    else if (AtaDeviceIsSsd(&DevExt->IdentifyDeviceData))
        IncursSeekPenalty = FALSE;
    else
        return STATUS_UNSUCCESSFUL; /* Undetermined */

    PenaltyDescriptor = Irp->AssociatedIrp.SystemBuffer;
    PenaltyDescriptor->Version = sizeof(*PenaltyDescriptor);
    PenaltyDescriptor->Size = sizeof(*PenaltyDescriptor);
    PenaltyDescriptor->IncursSeekPenalty = IncursSeekPenalty;

    Irp->IoStatus.Information = sizeof(*PenaltyDescriptor);
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoQueryStorageDeviceTrimProperty(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp)
{
    PDEVICE_TRIM_DESCRIPTOR TrimDescriptor;
    NTSTATUS Status;

    PAGED_CODE();

    if (IS_ATAPI(DevExt))
        return STATUS_NOT_SUPPORTED;

    if (!AtaCheckPropertyQuery(Irp, sizeof(*TrimDescriptor), &Status))
        return Status;

    TrimDescriptor = Irp->AssociatedIrp.SystemBuffer;
    TrimDescriptor->Version = sizeof(*TrimDescriptor);
    TrimDescriptor->Size = sizeof(*TrimDescriptor);
    TrimDescriptor->TrimEnabled = AtaHasTrimFunction(&DevExt->IdentifyDeviceData);

    Irp->IoStatus.Information = sizeof(*TrimDescriptor);
    return STATUS_SUCCESS;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleStorageQueryProperty(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack,
    _Out_ PBOOLEAN ForwardToFdo)
{
    PSTORAGE_PROPERTY_QUERY PropertyQuery;
    NTSTATUS Status;

    PAGED_CODE();

    if (!AtaVerifyInBuffer(IoStack, STORAGE_PROPERTY_QUERY))
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    PropertyQuery = Irp->AssociatedIrp.SystemBuffer;
    switch (PropertyQuery->PropertyId)
    {
        case StorageDeviceProperty:
            Status = AtaPdoQueryStorageDeviceProperty(DevExt, Irp);
            break;

        case StorageAccessAlignmentProperty:
            Status = AtaPdoQueryStorageAccessAlignmentProperty(DevExt, Irp);
            break;

        case StorageDeviceSeekPenaltyProperty:
            Status = AtaPdoQueryStorageDeviceSeekPenaltyProperty(DevExt, Irp);
            break;

        case StorageDeviceTrimProperty:
            Status = AtaPdoQueryStorageDeviceTrimProperty(DevExt, Irp);
            break;

        default:
            *ForwardToFdo = TRUE;
            return STATUS_MORE_PROCESSING_REQUIRED;
    }

    *ForwardToFdo = FALSE;
    return Status;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleGetScsiAddress(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PSCSI_ADDRESS ScsiAddress;
    ATA_SCSI_ADDRESS AtaScsiAddress;

    PAGED_CODE();

    if (!AtaVerifyOutBuffer(IoStack, SCSI_ADDRESS))
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    AtaScsiAddress = DevExt->AtaScsiAddress;

    ScsiAddress = Irp->AssociatedIrp.SystemBuffer;
    ScsiAddress->Length = sizeof(*ScsiAddress);
    ScsiAddress->PortNumber = DevExt->ChanExt->PortNumber;
    ScsiAddress->PathId = AtaScsiAddress.PathId;
    ScsiAddress->TargetId = AtaScsiAddress.TargetId;
    ScsiAddress->Lun = AtaScsiAddress.Lun;

    Irp->IoStatus.Information = sizeof(*ScsiAddress);
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoSendHbaControl(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSRB_IO_CONTROL SrbControl,
    _In_ ULONG BufferSize)
{
    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleScsiMiniport(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PSRB_IO_CONTROL SrbControl;
    NTSTATUS Status;
    ULONG CmdBufferSize, BufferSize;

    PAGED_CODE();

    if (!AtaVerifyInBuffer(IoStack, SRB_IO_CONTROL))
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    SrbControl = Irp->AssociatedIrp.SystemBuffer;
    if (SrbControl->HeaderLength != sizeof(*SrbControl))
    {
        ERR("Unknown structure size %u\n", SrbControl->HeaderLength);
        return STATUS_REVISION_MISMATCH;
    }

    Status = RtlULongAdd(SrbControl->Length, sizeof(*SrbControl), &BufferSize);
    if (!NT_SUCCESS(Status))
    {
        ERR("Too large buffer 0x%lx\n", SrbControl->Length);
        return Status;
    }

    CmdBufferSize = IoStack->Parameters.DeviceIoControl.InputBufferLength;
    CmdBufferSize = max(CmdBufferSize, IoStack->Parameters.DeviceIoControl.OutputBufferLength);
    if (CmdBufferSize < BufferSize)
    {
        ERR("Cmd buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (RtlEqualMemory(SrbControl->Signature, "SCSIDISK", sizeof("SCSIDISK") - 1))
    {
        Status = AtaPdoSendHbaControl(DevExt, SrbControl, BufferSize);
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleAtaPassthrough(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
AtaPdoDeviceControl(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    BOOLEAN ForwardToFdo = FALSE;

    Status = IoAcquireRemoveLock(&DevExt->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_STORAGE_QUERY_PROPERTY:
            Status = AtaPdoHandleStorageQueryProperty(DevExt, Irp, IoStack, &ForwardToFdo);
            break;

        case IOCTL_SCSI_GET_ADDRESS:
            Status = AtaPdoHandleGetScsiAddress(DevExt, Irp, IoStack);
            break;

        case IOCTL_SCSI_MINIPORT:
            Status = AtaPdoHandleScsiMiniport(DevExt, Irp, IoStack);
            break;

        case IOCTL_ATA_PASS_THROUGH:
        case IOCTL_ATA_PASS_THROUGH_DIRECT:
            Status = AtaPdoHandleAtaPassthrough(DevExt, Irp, IoStack);
            break;

        case IOCTL_SCSI_GET_CAPABILITIES:
        case IOCTL_SCSI_GET_INQUIRY_DATA:
            ForwardToFdo = TRUE;
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    if (ForwardToFdo)
    {
        PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->ChanExt;

        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(ChanExt->Common.Self, Irp);
    }
    else
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    IoReleaseRemoveLock(&DevExt->Common.RemoveLock, Irp);

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryStorageAdapterProperty(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp)
{
    PSTORAGE_ADAPTER_DESCRIPTOR AdapterDescriptor;
    NTSTATUS Status;

    if (!AtaCheckPropertyQuery(Irp, sizeof(*AdapterDescriptor), &Status))
        return Status;

    AdapterDescriptor = Irp->AssociatedIrp.SystemBuffer;

    /*
     * This structure has to be zeroed out first
     * in order to not confuse the upper storage class drivers.
     * Also the new version of the descriptor structure (NT6.2+)
     * has two extra fields instead of the unnamed "padding" fields.
     */
    RtlZeroMemory(AdapterDescriptor, sizeof(*AdapterDescriptor));

    AdapterDescriptor->Version = sizeof(*AdapterDescriptor);
    AdapterDescriptor->Size = sizeof(*AdapterDescriptor);
    AdapterDescriptor->MaximumPhysicalPages = (ULONG)-1; /* Undetermined */
    AdapterDescriptor->AlignmentMask = ChanExt->Common.Self->AlignmentRequirement;
    AdapterDescriptor->AdapterUsesPio = TRUE;
    AdapterDescriptor->AdapterScansDown = FALSE;
    AdapterDescriptor->CommandQueueing = FALSE;
    AdapterDescriptor->AcceleratedTransfer = FALSE;
    AdapterDescriptor->BusType = BusTypeAta;
    AdapterDescriptor->BusMajorVersion = 1;
    AdapterDescriptor->BusMinorVersion = 0;
    AdapterDescriptor->MaximumTransferLength = ChanExt->MaximumTransferLength;

    Irp->IoStatus.Information = sizeof(*AdapterDescriptor);
    return STATUS_SUCCESS;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaFdoHandleStorageQueryProperty(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS Status;
    PSTORAGE_PROPERTY_QUERY PropertyQuery;

    PAGED_CODE();

    if (!AtaVerifyInBuffer(IoStack, STORAGE_PROPERTY_QUERY))
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    PropertyQuery = Irp->AssociatedIrp.SystemBuffer;
    switch (PropertyQuery->PropertyId)
    {
        case StorageAdapterProperty:
            Status = AtaFdoQueryStorageAdapterProperty(ChanExt, Irp);
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            break;
    }

    return Status;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaFdoHandleGetScsiCapabilities(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PIO_SCSI_CAPABILITIES Capabilities;

    PAGED_CODE();

    if (!AtaVerifyOutBuffer(IoStack, IO_SCSI_CAPABILITIES))
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    Capabilities = Irp->AssociatedIrp.SystemBuffer;
    Capabilities->Length = sizeof(*Capabilities);
    Capabilities->MaximumPhysicalPages = (ULONG)-1; /* Undetermined */
    Capabilities->SupportedAsynchronousEvents = FALSE;
    Capabilities->AlignmentMask = ChanExt->Common.Self->AlignmentRequirement;
    Capabilities->TaggedQueuing = FALSE;
    Capabilities->AdapterScansDown = FALSE;
    Capabilities->AdapterUsesPio = FALSE; /* This differs from what AdapterProperty returns */
    Capabilities->MaximumTransferLength = ChanExt->MaximumTransferLength;

    Irp->IoStatus.Information = sizeof(*Capabilities);
    return STATUS_SUCCESS;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaFdoHandleGetScsiInquiryData(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PSCSI_INQUIRY_DATA ScsiInquiryData;
    PSCSI_ADAPTER_BUS_INFO ScsiAdapterBusInfo;
    ULONG DeviceCount, EntrySize, TotalSize;
    PSINGLE_LIST_ENTRY Entry;

    PAGED_CODE();

    /*
     * Save the current device count. This structure member can be
     * modified while we are handling this IOCTL request.
     */
    DeviceCount = ChanExt->DeviceCount;
    KeMemoryBarrierWithoutFence();

    EntrySize = ALIGN_UP(sizeof(*ScsiInquiryData) - 1 + INQUIRYDATABUFFERSIZE, ULONG);
    TotalSize = sizeof(*ScsiAdapterBusInfo) + EntrySize * DeviceCount;

    TRACE("Total size %lu\n", TotalSize);

    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < TotalSize)
        return STATUS_BUFFER_TOO_SMALL;

    Irp->IoStatus.Information = TotalSize;

    ScsiAdapterBusInfo = Irp->AssociatedIrp.SystemBuffer;
    ScsiAdapterBusInfo->NumberOfBuses = 1;
    ScsiAdapterBusInfo->BusData[0].NumberOfLogicalUnits = 0;
    ScsiAdapterBusInfo->BusData[0].InitiatorBusId = 0xFF;
    ScsiAdapterBusInfo->BusData[0].InquiryDataOffset = sizeof(*ScsiAdapterBusInfo);

    ScsiInquiryData = (PSCSI_INQUIRY_DATA)(ScsiAdapterBusInfo + 1);

    ExAcquireFastMutex(&ChanExt->DeviceSyncMutex);

    for (Entry = &ChanExt->DeviceList;
         Entry != NULL;
         Entry = Entry->Next)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        ATA_SCSI_ADDRESS AtaScsiAddress;
        PINQUIRYDATA InquiryData;

        DevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);
        if (DevExt->ReportedMissing)
            continue;

        AtaScsiAddress = DevExt->AtaScsiAddress;

        ScsiInquiryData->PathId = AtaScsiAddress.PathId;
        ScsiInquiryData->TargetId = AtaScsiAddress.TargetId;
        ScsiInquiryData->Lun = AtaScsiAddress.Lun;
        ScsiInquiryData->InquiryDataLength = INQUIRYDATABUFFERSIZE;
        ScsiInquiryData->DeviceClaimed = DevExt->DeviceClaimed;
        ScsiInquiryData->NextInquiryDataOffset =
            (ULONG)((ULONG_PTR)ScsiInquiryData + EntrySize - (ULONG_PTR)ScsiAdapterBusInfo);

        RtlCopyMemory(ScsiInquiryData->InquiryData,
                      &DevExt->InquiryData,
                      INQUIRYDATABUFFERSIZE);

        /* This is a legacy IOCTL, limit the standard INQUIRY data to 36 bytes */
        InquiryData = (PINQUIRYDATA)ScsiInquiryData->InquiryData;
        InquiryData->AdditionalLength =
            INQUIRYDATABUFFERSIZE - RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, AdditionalLength);

        ScsiInquiryData = (PSCSI_INQUIRY_DATA)((ULONG_PTR)ScsiInquiryData + EntrySize);

        if (++ScsiAdapterBusInfo->BusData[0].NumberOfLogicalUnits >= DeviceCount)
            break;
    }

    ExReleaseFastMutex(&ChanExt->DeviceSyncMutex);

    /* Terminate the last entry */
    if (ScsiAdapterBusInfo->BusData[0].NumberOfLogicalUnits != 0)
    {
        ScsiInquiryData = ((PSCSI_INQUIRY_DATA)((ULONG_PTR)ScsiInquiryData - EntrySize));
        ScsiInquiryData->NextInquiryDataOffset = 0;
    }
    else
    {
        ScsiAdapterBusInfo->BusData[0].InquiryDataOffset = 0;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
AtaFdoDeviceControl(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    Status = IoAcquireRemoveLock(&ChanExt->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_STORAGE_QUERY_PROPERTY:
            Status = AtaFdoHandleStorageQueryProperty(ChanExt, Irp, IoStack);
            break;

        case IOCTL_SCSI_GET_CAPABILITIES:
            Status = AtaFdoHandleGetScsiCapabilities(ChanExt, Irp, IoStack);
            break;

        case IOCTL_SCSI_GET_INQUIRY_DATA:
            Status = AtaFdoHandleGetScsiInquiryData(ChanExt, Irp, IoStack);
            break;

        case IOCTL_SCSI_RESCAN_BUS:
            IoInvalidateDeviceRelations(ChanExt->Common.Self, BusRelations);
            Status = STATUS_SUCCESS;
            break;

        default:
        {
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(ChanExt->Ldo, Irp);

            IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

            return Status;
        }
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

    return Status;
}

/*
 * For storage drivers this dispatch function must be not paged,
 * because it must be present when it is received unknown IOCTL,
 * otherwise we risk locking up the whole system.
 */
NTSTATUS
NTAPI
AtaDispatchDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    if (IS_FDO(DeviceObject->DeviceExtension))
        return AtaFdoDeviceControl(DeviceObject->DeviceExtension, Irp);
    else
        return AtaPdoDeviceControl(DeviceObject->DeviceExtension, Irp);
}
