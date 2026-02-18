/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     I/O control handling
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

#define AtaVerifyInBuffer(IoStack, Length) \
    ((IoStack)->Parameters.DeviceIoControl.InputBufferLength >= (Length))

#define AtaVerifyOutBuffer(IoStack, Length) \
    ((IoStack)->Parameters.DeviceIoControl.OutputBufferLength >= (Length))

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
    PSTORAGE_DESCRIPTOR_HEADER DescriptorHeader;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (!AtaVerifyInBuffer(IoStack, sizeof(*PropertyQuery)))
    {
        *Status = STATUS_INVALID_PARAMETER;
        return FALSE;
    }

    /* Check the type of a property query */
    if (PropertyQuery->QueryType != PropertyStandardQuery &&
        PropertyQuery->QueryType != PropertyExistsQuery)
    {
        *Status = STATUS_NOT_SUPPORTED;
        return FALSE;
    }

    /* The requested property is supported */
    if (PropertyQuery->QueryType == PropertyExistsQuery)
    {
        *Status = STATUS_SUCCESS;
        return FALSE;
    }

    if (!AtaVerifyOutBuffer(IoStack, sizeof(*DescriptorHeader)))
    {
        *Status = STATUS_INFO_LENGTH_MISMATCH;
        return FALSE;
    }

    /* Caller can determine required size based upon DescriptorHeader */
    if (!AtaVerifyOutBuffer(IoStack, DescriptorSize))
    {
        DescriptorHeader = (PSTORAGE_DESCRIPTOR_HEADER)PropertyQuery;
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
    NTSTATUS Status;
    ULONG DescriptorSize, RawPropertiesLength, Length1, Length2, Length3;

    PAGED_CODE();

    Length1 = (ULONG)strlen(DevExt->FriendlyName) + sizeof(ANSI_NULL);
    Length2 = (ULONG)strlen(DevExt->RevisionNumber) + sizeof(ANSI_NULL);
    Length3 = (ULONG)strlen(DevExt->SerialNumber) + sizeof(ANSI_NULL);
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
    DeviceDescriptor->CommandQueueing = FALSE; // Disable request tagging
    if (DevExt->Device.PortData->PortFlags & PORT_FLAG_IS_AHCI)
        DeviceDescriptor->BusType = BusTypeSata;
    else
        DeviceDescriptor->BusType = BusTypeAta;

    /* Property 1: The vendor ID. We return a NULL string here */
    DeviceDescriptor->VendorIdOffset = 0;

    /* Property 2: The product ID */
    DeviceDescriptor->ProductIdOffset =
        FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties);
    RtlCopyMemory((PUCHAR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->ProductIdOffset),
                  DevExt->FriendlyName,
                  Length1);

    /* Property 3: The product revision */
    DeviceDescriptor->ProductRevisionOffset = DeviceDescriptor->ProductIdOffset + Length1;
    RtlCopyMemory((PUCHAR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->ProductRevisionOffset),
                  DevExt->RevisionNumber,
                  Length2);

    /* Property 4: The serial number */
    DeviceDescriptor->SerialNumberOffset = DeviceDescriptor->ProductRevisionOffset + Length2;
    RtlCopyMemory((PUCHAR)((ULONG_PTR)DeviceDescriptor + DeviceDescriptor->SerialNumberOffset),
                  DevExt->SerialNumber,
                  Length3);

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

    if (IS_ATAPI(&DevExt->Device))
        return STATUS_NOT_SUPPORTED;

    if (!AtaCheckPropertyQuery(Irp, sizeof(*AccessAlignmentDescriptor), &Status))
        return Status;

    AccessAlignmentDescriptor = Irp->AssociatedIrp.SystemBuffer;
    AccessAlignmentDescriptor->Version = sizeof(*AccessAlignmentDescriptor);
    AccessAlignmentDescriptor->Size = sizeof(*AccessAlignmentDescriptor);
    AccessAlignmentDescriptor->BytesPerCacheLine = 0;
    AccessAlignmentDescriptor->BytesOffsetForCacheAlignment = 0;
    AccessAlignmentDescriptor->BytesPerLogicalSector = DevExt->Device.SectorSize;

    LogicalSectorsPerPhysicalSector =
        AtaDevLogicalSectorsPerPhysicalSector(&DevExt->IdentifyDeviceData, &Dummy);

    AccessAlignmentDescriptor->BytesPerPhysicalSector =
        DevExt->Device.SectorSize * LogicalSectorsPerPhysicalSector;

    AccessAlignmentDescriptor->BytesOffsetForSectorAlignment =
        DevExt->Device.SectorSize * AtaDevLogicalSectorAlignment(&DevExt->IdentifyDeviceData);

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

    if (AtaDevIsRotatingDevice(&DevExt->IdentifyDeviceData))
        IncursSeekPenalty = TRUE;
    else if (AtaDevIsSsd(&DevExt->IdentifyDeviceData))
        IncursSeekPenalty = FALSE;
    else
        return STATUS_UNSUCCESSFUL; // Undetermined

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

    if (IS_ATAPI(&DevExt->Device))
        return STATUS_NOT_SUPPORTED;

    if (!AtaCheckPropertyQuery(Irp, sizeof(*TrimDescriptor), &Status))
        return Status;

    TrimDescriptor = Irp->AssociatedIrp.SystemBuffer;
    TrimDescriptor->Version = sizeof(*TrimDescriptor);
    TrimDescriptor->Size = sizeof(*TrimDescriptor);
    TrimDescriptor->TrimEnabled = AtaDevHasTrimFunction(&DevExt->IdentifyDeviceData);

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

    if (!AtaVerifyInBuffer(IoStack, sizeof(*PropertyQuery)))
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
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    PSCSI_ADDRESS ScsiAddress;
    ATA_SCSI_ADDRESS AtaScsiAddress;

    PAGED_CODE();

    if (!AtaVerifyOutBuffer(IoStack, sizeof(*ScsiAddress)))
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    ChanExt = DevExt->Common.FdoExt;
    AtaScsiAddress = DevExt->Device.AtaScsiAddress;

    ScsiAddress = Irp->AssociatedIrp.SystemBuffer;
    ScsiAddress->Length = sizeof(*ScsiAddress);
    ScsiAddress->PortNumber = ChanExt->ScsiPortNumber;
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
    PSCSI_REQUEST_BLOCK Srb;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    LARGE_INTEGER LargeInt;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    Srb = ExAllocatePoolZero(NonPagedPool, sizeof(*Srb), ATAPORT_TAG);
    if (!Srb)
        return STATUS_INSUFFICIENT_RESOURCES;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    LargeInt.QuadPart = 1; // For compatibility only
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SCSI,
                                       DevExt->Common.Self,
                                       SrbControl,
                                       BufferSize,
                                       &LargeInt,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Srb->OriginalRequest = Irp;

    Srb->Function = SRB_FUNCTION_IO_CONTROL;
    Srb->Length = sizeof(*Srb);

    Srb->TargetId = DevExt->Device.AtaScsiAddress.TargetId;
    Srb->Lun = DevExt->Device.AtaScsiAddress.Lun;
    Srb->PathId = DevExt->Device.AtaScsiAddress.PathId;

    Srb->TimeOutValue = SrbControl->Timeout;

    Srb->SrbFlags = SRB_FLAGS_NO_QUEUE_FREEZE | SRB_FLAGS_DATA_IN;

    Srb->DataBuffer = SrbControl;
    Srb->DataTransferLength = BufferSize;

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->Parameters.Scsi.Srb = Srb;

    Status = IoCallDriver(DevExt->Common.Self, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

Cleanup:
    ExFreePoolWithTag(Srb, ATAPORT_TAG);
    return Status;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleStorageManageDataSetAttributes(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PAGED_CODE();

    // TODO: Implement
    return Irp->IoStatus.Status;
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

    if (!AtaVerifyInBuffer(IoStack, sizeof(*SrbControl)))
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    SrbControl = Irp->AssociatedIrp.SystemBuffer;
    if (SrbControl->HeaderLength != sizeof(*SrbControl))
    {
        ERR("Unknown structure size %lu\n", SrbControl->HeaderLength);
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
        Status = AtaPdoSendHbaControl(DevExt, SrbControl, BufferSize);
    else
        Status = STATUS_INVALID_PARAMETER;

    return Status;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleAtaPassthrough(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->Common.FdoExt;

    PAGED_CODE();

    return SptiHandleAtaPassthru(DevExt->Common.Self,
                                 Irp,
                                 ChanExt->PortData.MaximumTransferLength,
                                 ChanExt->PortData.MaximumPhysicalPages);
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleScsiPassthrough(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt;

    PAGED_CODE();

    /* Skip requests that bypassed the class driver. See also cdrom!RequestHandleScsiPassThrough */
    if ((IoStack->MinorFunction == 0) && DevExt->DeviceClaimed)
        return STATUS_INVALID_DEVICE_REQUEST;

    ChanExt = DevExt->Common.FdoExt;

    return SptiHandleScsiPassthru(DevExt->Common.Self,
                                  Irp,
                                  ChanExt->PortData.MaximumTransferLength,
                                  ChanExt->PortData.MaximumPhysicalPages);
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

        case IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES:
            Status = AtaPdoHandleStorageManageDataSetAttributes(DevExt, Irp, IoStack);
            break;

        case IOCTL_ATA_PASS_THROUGH:
        case IOCTL_ATA_PASS_THROUGH_DIRECT:
            Status = AtaPdoHandleAtaPassthrough(DevExt, Irp);
            break;

        case IOCTL_SCSI_PASS_THROUGH:
        case IOCTL_SCSI_PASS_THROUGH_DIRECT:
            Status = AtaPdoHandleScsiPassthrough(DevExt, Irp, IoStack);
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
        PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->Common.FdoExt;

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

    PAGED_CODE();

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
    AdapterDescriptor->MaximumTransferLength = ChanExt->PortData.MaximumTransferLength;
    AdapterDescriptor->MaximumPhysicalPages = ChanExt->PortData.MaximumPhysicalPages;
    AdapterDescriptor->AlignmentMask = ChanExt->Common.Self->AlignmentRequirement;
    AdapterDescriptor->AdapterUsesPio = !!(ChanExt->PortData.PortFlags && PORT_FLAG_PIO_ONLY);
    AdapterDescriptor->AdapterScansDown = FALSE;
    AdapterDescriptor->CommandQueueing = FALSE; // Disable request tagging
    AdapterDescriptor->AcceleratedTransfer = FALSE;
    if (ChanExt->PortData.PortFlags & PORT_FLAG_IS_AHCI)
        AdapterDescriptor->BusType = BusTypeSata;
    else
        AdapterDescriptor->BusType = BusTypeAta;
    AdapterDescriptor->BusMajorVersion = 1;
    AdapterDescriptor->BusMinorVersion = 0;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    AdapterDescriptor->SrbType = SRB_TYPE_SCSI_REQUEST_BLOCK;
    AdapterDescriptor->AddressType = STORAGE_ADDRESS_TYPE_BTL8;
#endif

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
    PSTORAGE_PROPERTY_QUERY PropertyQuery;
    NTSTATUS Status;

    PAGED_CODE();

    if (!AtaVerifyInBuffer(IoStack, sizeof(*PropertyQuery)))
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

    if (!AtaVerifyOutBuffer(IoStack, sizeof(*Capabilities)))
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    Capabilities = Irp->AssociatedIrp.SystemBuffer;
    Capabilities->Length = sizeof(*Capabilities);
    Capabilities->MaximumTransferLength = ChanExt->PortData.MaximumTransferLength;
    Capabilities->MaximumPhysicalPages = ChanExt->PortData.MaximumPhysicalPages;
    Capabilities->SupportedAsynchronousEvents = FALSE;
    Capabilities->AlignmentMask = ChanExt->Common.Self->AlignmentRequirement;
    Capabilities->TaggedQueuing = FALSE;
    Capabilities->AdapterScansDown = FALSE;
    Capabilities->AdapterUsesPio = !!(ChanExt->PortData.PortFlags && PORT_FLAG_PIO_ONLY);

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
    ULONG PdoCount, EntrySize, TotalSize;
    ATA_SCSI_ADDRESS AtaScsiAddress;

    PAGED_CODE();

    PdoCount = 0;
    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;

        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, FALSE, NULL);
        if (!DevExt)
            break;

        ++PdoCount;
    }

    EntrySize = ALIGN_UP(sizeof(*ScsiInquiryData) - 1 + INQUIRYDATABUFFERSIZE, ULONG);
    TotalSize = sizeof(*ScsiAdapterBusInfo) + EntrySize * PdoCount;
    TRACE("Total size %lu\n", TotalSize);

    if (!AtaVerifyOutBuffer(IoStack, TotalSize))
        return STATUS_BUFFER_TOO_SMALL;

    Irp->IoStatus.Information = TotalSize;

    ScsiAdapterBusInfo = Irp->AssociatedIrp.SystemBuffer;
    ScsiAdapterBusInfo->NumberOfBuses = 1;
    ScsiAdapterBusInfo->BusData[0].NumberOfLogicalUnits = 0;
    ScsiAdapterBusInfo->BusData[0].InitiatorBusId = 0xFF;
    ScsiAdapterBusInfo->BusData[0].InquiryDataOffset = sizeof(*ScsiAdapterBusInfo);

    ScsiInquiryData = (PSCSI_INQUIRY_DATA)(ScsiAdapterBusInfo + 1);

    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        PINQUIRYDATA InquiryData;

        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, FALSE, Irp);
        if (!DevExt)
            break;

        AtaScsiAddress = DevExt->Device.AtaScsiAddress;

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

        IoReleaseRemoveLock(&DevExt->Common.RemoveLock, Irp);

        ScsiInquiryData = (PSCSI_INQUIRY_DATA)((ULONG_PTR)ScsiInquiryData + EntrySize);

        if (++ScsiAdapterBusInfo->BusData[0].NumberOfLogicalUnits >= PdoCount)
            break;
    }

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
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaFdoHandleScsiMiniport(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    NTSTATUS Status;

    PAGED_CODE();

    /* FDO requests are routed directly to 0:0:0 */
    DevExt = AtaFdoFindDeviceByPath(ChanExt, AtaMarshallScsiAddress(0, 0, 0), Irp);
    if (!DevExt)
        return STATUS_NO_SUCH_DEVICE;

    Status = AtaPdoHandleScsiMiniport(DevExt, Irp, IoStack);

    IoReleaseRemoveLock(&DevExt->Common.RemoveLock, Irp);

    return Status;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaFdoHandleScsiPassthrough(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PSCSI_PASS_THROUGH Spt;
    PATAPORT_DEVICE_EXTENSION DevExt;
    NTSTATUS Status;

    PAGED_CODE();

    if (!AtaVerifyInBuffer(IoStack, RTL_SIZEOF_THROUGH_FIELD(SCSI_PASS_THROUGH, Lun)))
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    Spt = Irp->AssociatedIrp.SystemBuffer;

    DevExt = AtaFdoFindDeviceByPath(ChanExt,
                                    AtaMarshallScsiAddress(Spt->PathId, Spt->TargetId, Spt->Lun),
                                    Irp);
    if (!DevExt)
        return STATUS_NO_SUCH_DEVICE;

    Status = AtaPdoHandleScsiPassthrough(DevExt, Irp, IoStack);

    IoReleaseRemoveLock(&DevExt->Common.RemoveLock, Irp);

    return Status;
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

        case IOCTL_SCSI_MINIPORT:
            Status = AtaFdoHandleScsiMiniport(ChanExt, Irp, IoStack);
            break;

        case IOCTL_SCSI_PASS_THROUGH:
        case IOCTL_SCSI_PASS_THROUGH_DIRECT:
            Status = AtaFdoHandleScsiPassthrough(ChanExt, Irp, IoStack);
            break;

        case IOCTL_SCSI_RESCAN_BUS:
            IoInvalidateDeviceRelations(ChanExt->Pdo, BusRelations);
            Status = STATUS_SUCCESS;
            break;

        default:
        {
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(ChanExt->Common.LowerDeviceObject, Irp);

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
