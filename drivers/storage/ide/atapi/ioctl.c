/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     I/O control handling
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
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
    DeviceDescriptor->BusType = IS_AHCI(&DevExt->Device) ? BusTypeSata : BusTypeAta;

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

    AtaScsiAddress = DevExt->Device.AtaScsiAddress;

    ScsiAddress = Irp->AssociatedIrp.SystemBuffer;
    ScsiAddress->Length = sizeof(*ScsiAddress);
    ScsiAddress->PortNumber = DevExt->Device.ChanExt->ScsiPortNumber;
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

    LargeInt.QuadPart = 1;
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SCSI,
                                       DevExt->Common.Self,
                                       SrbControl,
                                       BufferSize,
                                       &LargeInt,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
    {
        ExFreePoolWithTag(Srb, ATAPORT_TAG);

        return STATUS_INSUFFICIENT_RESOURCES;
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
CODE_SEG("PAGE")
VOID
AtaDestroySynchronousUserBufferIrp(
    _In_opt_ __drv_freesMem(Mem) PIRP Irp,
    _In_opt_ __drv_freesMem(Mem) PSCSI_REQUEST_BLOCK Srb)
{
    PAGED_CODE();

    if (Srb)
        ExFreePoolWithTag(Srb, ATAPORT_TAG);

    if (Irp)
    {
        if (Irp->MdlAddress)
        {
            MmUnlockPages(Irp->MdlAddress);
            IoFreeMdl(Irp->MdlAddress);
        }
        Irp->MdlAddress = NULL;
        IoFreeIrp(Irp);
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaCreateSynchronousUserBufferIrp(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PVOID DataBuffer,
    _In_ ULONG Length,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN IsBufferReadAccess,
    _In_ PKEVENT Event,
    _In_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_ PIRP* NewIrp,
    _Out_ PSCSI_REQUEST_BLOCK* NewSrb)
{
    PSCSI_REQUEST_BLOCK Srb = NULL;
    PIRP Irp = NULL;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    KeInitializeEvent(Event, NotificationEvent, FALSE);

    Srb = ExAllocatePoolZero(NonPagedPool, sizeof(*Srb), ATAPORT_TAG);
    if (!Srb)
        goto Failure;

    Irp = IoAllocateIrp(DevExt->Common.Self->StackSize, 0);
    if (!Irp)
        goto Failure;

    if (DataBuffer)
    {
        if (!IoAllocateMdl(DataBuffer, Length, FALSE, FALSE, Irp))
            goto Failure;
        ASSERT(Irp->MdlAddress);

        /* Lock the user buffer because PASSTHROUGH commands come from user mode */
        _SEH2_TRY
        {
            MmProbeAndLockPages(Irp->MdlAddress,
                                AccessMode,
                                IsBufferReadAccess ? IoReadAccess : IoWriteAccess);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(goto Failure);
        }
        _SEH2_END;
    }

    *NewIrp = Irp;
    *NewSrb = Srb;

    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    Srb->Length = sizeof(*Srb);
    Srb->OriginalRequest = Irp;
    Srb->DataBuffer = DataBuffer;
    Srb->DataTransferLength = Length;

    Srb->TargetId = DevExt->Device.AtaScsiAddress.TargetId;
    Srb->Lun = DevExt->Device.AtaScsiAddress.Lun;
    Srb->PathId = DevExt->Device.AtaScsiAddress.PathId;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->UserIosb = IoStatusBlock;

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MinorFunction = IRP_MN_SCSI_CLASS;
    IoStack->MajorFunction = IRP_MJ_SCSI;
    IoStack->Parameters.Scsi.Srb = Srb;

    IoSetCompletionRoutine(Irp,
                           AtaPdoCompletion,
                           Event,
                           TRUE,
                           TRUE,
                           TRUE);

    return STATUS_SUCCESS;

Failure:
    ERR("Failed to create IRP\n");
    AtaDestroySynchronousUserBufferIrp(Irp, Srb);

    return STATUS_INSUFFICIENT_RESOURCES;
}

static
CODE_SEG("PAGE")
VOID
AtaTranslateAtaPassthroughToCdb(
    _Out_ ATA_PASSTHROUGH16* __restrict Cdb,
    _In_ UCHAR* __restrict TaskFile,
    _In_ USHORT AtaFlags)
{
    PAGED_CODE();

    Cdb->OperationCode = SCSIOP_ATA_PASSTHROUGH16;

    Cdb->Features = TaskFile[8 + 0];
    Cdb->Count    = TaskFile[8 + 1];
    Cdb->LowLba   = TaskFile[8 + 2];
    Cdb->MidLba   = TaskFile[8 + 3];
    Cdb->HighLba  = TaskFile[8 + 4];
    Cdb->Device   = TaskFile[8 + 5];
    Cdb->Command  = TaskFile[8 + 6];

    if (AtaFlags & ATA_FLAGS_48BIT_COMMAND)
    {
        Cdb->FeaturesEx = TaskFile[0];
        Cdb->CountEx    = TaskFile[1];
        Cdb->LowLbaEx   = TaskFile[2];
        Cdb->MidLbaEx   = TaskFile[3];
        Cdb->HighLbaEx  = TaskFile[4];

        Cdb->Extend = 1;
    }

    Cdb->CkCond = 1;

    if (AtaFlags & ATA_FLAGS_DATA_IN)
        Cdb->TDir = 1;

    if (AtaFlags & (ATA_FLAGS_DATA_IN | ATA_FLAGS_DATA_OUT))
        Cdb->TLength = 3;

    if (AtaFlags & ATA_FLAGS_USE_DMA)
    {
        Cdb->Protocol = ATA_PASSTHROUGH_PROTOCOL_DMA;
    }
    else
    {
        if (AtaFlags & (ATA_FLAGS_DATA_IN | ATA_FLAGS_DATA_OUT))
        {
            if (AtaFlags & ATA_FLAGS_DATA_IN)
                Cdb->Protocol = ATA_PASSTHROUGH_PROTOCOL_PIO_DATA_IN;
            else
                Cdb->Protocol = ATA_PASSTHROUGH_PROTOCOL_PIO_DATA_OUT;
        }
        else
        {
            Cdb->Protocol = ATA_PASSTHROUGH_PROTOCOL_NON_DATA;
        }
    }
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleAtaPassthrough(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack,
    _In_ BOOLEAN IsDirect)
{
    PATA_PASS_THROUGH_EX Apt = Irp->AssociatedIrp.SystemBuffer;
    union
    {
        PVOID Buffer;
        ULONG_PTR BufferOffset;
    } Data;
    ULONG StructSize;
    BOOLEAN IsNativeSize;
    PUCHAR TaskFile;
    NTSTATUS Status;
    PVOID Buffer;
    PSCSI_REQUEST_BLOCK Srb;
    PDESCRIPTOR_SENSE_DATA SenseData;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP NewIrp;

    PAGED_CODE();

#if defined(_WIN64) && defined(BUILD_WOW64_ENABLED)
    if (IoIs32bitProcess(Irp))
    {
        StructSize = sizeof(ATA_PASS_THROUGH_EX32);
        TaskFile = (PVOID)((ULONG_PTR)Apt + FIELD_OFFSET(ATA_PASS_THROUGH_EX32, PreviousTaskFile));
        IsNativeSize = FALSE;
    }
    else
#endif
    {
        StructSize = sizeof(ATA_PASS_THROUGH_EX);
        TaskFile = (PVOID)((ULONG_PTR)Apt + FIELD_OFFSET(ATA_PASS_THROUGH_EX, PreviousTaskFile));
        IsNativeSize = TRUE;
    }

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < StructSize)
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (Apt->Length != StructSize)
    {
        ERR("Unknown structure size %lu\n", StructSize);
        return STATUS_REVISION_MISMATCH;
    }

    /* Retrieve the data buffer or the data buffer offset */
    Data.Buffer = NULL;
    RtlCopyMemory(&Data.Buffer,
                  (PVOID)((ULONG_PTR)Apt + FIELD_OFFSET(ATA_PASS_THROUGH_EX, DataBufferOffset)),
                  IsNativeSize ? sizeof(PVOID) : sizeof(ULONG32));

    if ((Apt->AtaFlags & (ATA_FLAGS_DATA_IN | ATA_FLAGS_DATA_OUT)) &&
        (Apt->DataTransferLength == 0))
    {
        ERR("Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    // TODO More checks

    if (Apt->DataTransferLength & DevExt->Common.Self->AlignmentRequirement)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Apt->TimeOutValue == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength >
        IoStack->Parameters.DeviceIoControl.InputBufferLength)
    {
        RtlZeroMemory((PVOID)((ULONG_PTR)Apt + IoStack->Parameters.DeviceIoControl.InputBufferLength),
                      IoStack->Parameters.DeviceIoControl.OutputBufferLength -
                      IoStack->Parameters.DeviceIoControl.InputBufferLength);
    }

    TRACE("APT: Prev %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          Apt->PreviousTaskFile[0],
          Apt->PreviousTaskFile[1],
          Apt->PreviousTaskFile[2],
          Apt->PreviousTaskFile[3],
          Apt->PreviousTaskFile[4],
          Apt->PreviousTaskFile[5],
          Apt->PreviousTaskFile[6]);
    TRACE("APT: Cur %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          Apt->CurrentTaskFile[0],
          Apt->CurrentTaskFile[1],
          Apt->CurrentTaskFile[2],
          Apt->CurrentTaskFile[3],
          Apt->CurrentTaskFile[4],
          Apt->CurrentTaskFile[5],
          Apt->CurrentTaskFile[6]);

    Srb = ExAllocatePoolZero(NonPagedPool, sizeof(*Srb), ATAPORT_TAG);
    if (!Srb)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (Apt->DataTransferLength)
    {
        if (IsDirect)
            Buffer = Data.Buffer;
        else
            Buffer = (PVOID)((ULONG_PTR)Apt + Data.BufferOffset);
    }
    else
    {
        Buffer = NULL;
    }

    Status = AtaCreateSynchronousUserBufferIrp(DevExt,
                                               Buffer,
                                               Apt->DataTransferLength,
                                               IsDirect ? Irp->RequestorMode : KernelMode,
                                               !!(Apt->AtaFlags & ATA_FLAGS_DATA_OUT),
                                               &Event,
                                               &IoStatusBlock,
                                               &NewIrp,
                                               &Srb);
    if (!NT_SUCCESS(Status))
        return Status;

    SenseData = ExAllocatePoolZero(NonPagedPool,
                                   FIELD_OFFSET(DESCRIPTOR_SENSE_DATA, DescriptorBuffer) +
                                   sizeof(SCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN),
                                   ATAPORT_TAG);
    if (!SenseData)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Srb->SenseInfoBuffer = SenseData;
    Srb->SenseInfoBufferLength = FIELD_OFFSET(DESCRIPTOR_SENSE_DATA, DescriptorBuffer) +
                                 sizeof(SCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN);

    Srb->TimeOutValue = Apt->TimeOutValue;
    Srb->CdbLength = sizeof(ATA_PASSTHROUGH16);

    AtaTranslateAtaPassthroughToCdb((PATA_PASSTHROUGH16)Srb->Cdb, TaskFile, Apt->AtaFlags);

    Srb->SrbFlags = SRB_FLAGS_NO_QUEUE_FREEZE;
    if (Apt->AtaFlags & ATA_FLAGS_DATA_IN)
        Srb->SrbFlags |= SRB_FLAGS_DATA_IN;
    if (Apt->AtaFlags & ATA_FLAGS_DATA_OUT)
        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;

    Status = IoCallDriver(DevExt->Common.Self, NewIrp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)
    {
        if ((Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION) &&
            (SenseData->SenseKey == SCSI_SENSE_RECOVERED_ERROR) &&
            (SenseData->AdditionalSenseCode == SCSI_ADSENSE_NO_SENSE) &&
            (SenseData->AdditionalSenseCodeQualifier ==
             SCSI_SENSEQ_ATA_PASS_THROUGH_INFORMATION_AVAILABLE))
        {
            Status = STATUS_SUCCESS;
        }

        if (SenseData->ErrorCode == SCSI_SENSE_ERRORCODE_DESCRIPTOR_CURRENT)
        {
            PSCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN Descriptor;

            Descriptor = (PSCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN)&SenseData->DescriptorBuffer[0];

            if (Descriptor->Header.DescriptorType == SCSI_SENSE_DESCRIPTOR_TYPE_ATA_STATUS_RETURN)
            {
                TaskFile[8 + 0] = Descriptor->Error;
                TaskFile[8 + 1] = Descriptor->SectorCount7_0;
                TaskFile[8 + 2] = Descriptor->LbaLow7_0;
                TaskFile[8 + 3] = Descriptor->LbaMid7_0;
                TaskFile[8 + 4] = Descriptor->LbaHigh7_0;
                TaskFile[8 + 5] = Descriptor->Device;
                TaskFile[8 + 6] = Descriptor->Status;

                if (Descriptor->Extend)
                {
                    TaskFile[1] = Descriptor->SectorCount15_8;
                    TaskFile[2] = Descriptor->LbaLow15_8;
                    TaskFile[3] = Descriptor->LbaMid15_8;
                    TaskFile[4] = Descriptor->LbaHigh15_8;
                }
            }
        }
    }

    Apt->DataTransferLength = Srb->DataTransferLength;

    if (!IsDirect && (Srb->SrbFlags & SRB_FLAGS_DATA_IN) && (Data.BufferOffset != 0))
        Irp->IoStatus.Information = Data.BufferOffset + Apt->DataTransferLength;
    else
        Irp->IoStatus.Information = Apt->Length;

Cleanup:
    if (SenseData)
        ExFreePoolWithTag(SenseData, ATAPORT_TAG);

    AtaDestroySynchronousUserBufferIrp(NewIrp, Srb);

    return Status;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleScsiPassthrough(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack,
    _In_ BOOLEAN IsDirect)
{
    PSCSI_PASS_THROUGH Spt = Irp->AssociatedIrp.SystemBuffer;
    union
    {
        PVOID Buffer;
        ULONG_PTR BufferOffset;
    } Data;
    ULONG StructSize;
    PULONG SenseInfoOff;
    BOOLEAN IsNativeSize;
    PVOID Cdb;
    NTSTATUS Status;
    PVOID Buffer;
    PSCSI_REQUEST_BLOCK Srb;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP NewIrp;

    PAGED_CODE();

#if defined(_WIN64) && defined(BUILD_WOW64_ENABLED)
    if (IoIs32bitProcess(Irp))
    {
        StructSize = sizeof(SCSI_PASS_THROUGH32);
        SenseInfoOff = (PULONG)((ULONG_PTR)Spt + FIELD_OFFSET(SCSI_PASS_THROUGH32, SenseInfoOffset));
        Cdb = (PVOID)((ULONG_PTR)Spt + FIELD_OFFSET(SCSI_PASS_THROUGH32, Cdb));
        IsNativeSize = FALSE;
    }
    else
#endif
    {
        StructSize = sizeof(SCSI_PASS_THROUGH);
        SenseInfoOff = (PULONG)((ULONG_PTR)Spt + FIELD_OFFSET(SCSI_PASS_THROUGH, SenseInfoOffset));
        Cdb = (PVOID)((ULONG_PTR)Spt + FIELD_OFFSET(SCSI_PASS_THROUGH, Cdb));
        IsNativeSize = TRUE;
    }

    TRACE("SPT: CDB %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          Spt->Cdb[0],
          Spt->Cdb[1],
          Spt->Cdb[2],
          Spt->Cdb[3],
          Spt->Cdb[4],
          Spt->Cdb[5],
          Spt->Cdb[6]);

    /* Retrieve the data buffer or the data buffer offset */
    Data.Buffer = NULL;
    RtlCopyMemory(&Data.Buffer,
                  (PVOID)((ULONG_PTR)Spt + FIELD_OFFSET(SCSI_PASS_THROUGH, DataBufferOffset)),
                  IsNativeSize ? sizeof(PVOID) : sizeof(ULONG32));

    if (Spt->DataTransferLength)
    {
        if (IsDirect)
            Buffer = Data.Buffer;
        else
            Buffer = (PVOID)((ULONG_PTR)Spt + Data.BufferOffset);
    }
    else
    {
        Buffer = NULL;
    }

    // TODO More checks
    (StructSize);

    Status = AtaCreateSynchronousUserBufferIrp(DevExt,
                                               Buffer,
                                               Spt->DataTransferLength,
                                               IsDirect ? Irp->RequestorMode : KernelMode,
                                               !!(Spt->DataIn == SCSI_IOCTL_DATA_OUT),
                                               &Event,
                                               &IoStatusBlock,
                                               &NewIrp,
                                               &Srb);
    if (!NT_SUCCESS(Status))
        return Status;

    if (*SenseInfoOff != 0)
    {
        Srb->SenseInfoBuffer = (PVOID)((ULONG_PTR)Spt + *SenseInfoOff);
        Srb->SenseInfoBufferLength = Spt->SenseInfoLength;
    }

    Srb->TimeOutValue = Spt->TimeOutValue;
    Srb->CdbLength = Spt->CdbLength;

    RtlCopyMemory(Srb->Cdb, Cdb, Srb->CdbLength);

    Srb->SrbFlags = SRB_FLAGS_NO_QUEUE_FREEZE;
    if (Spt->DataTransferLength != 0)
    {
        switch (Spt->DataIn)
        {
            case SCSI_IOCTL_DATA_IN:
                Srb->SrbFlags |= SRB_FLAGS_DATA_IN;
                break;
            case SCSI_IOCTL_DATA_OUT:
                Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
                break;
            default: // SCSI_IOCTL_DATA_UNSPECIFIED
                Srb->SrbFlags |= SRB_FLAGS_UNSPECIFIED_DIRECTION;
                break;
        }
    }

    Status = IoCallDriver(DevExt->Common.Self, NewIrp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)
        Spt->SenseInfoLength = Srb->SenseInfoBufferLength;
    else
        Spt->SenseInfoLength = 0;

    Spt->ScsiStatus = Srb->ScsiStatus;
    Spt->DataTransferLength = Srb->DataTransferLength;

    if (!IsDirect && (Srb->SrbFlags & SRB_FLAGS_DATA_IN) && (Data.BufferOffset != 0))
    {
        Irp->IoStatus.Information = Data.BufferOffset + Spt->DataTransferLength;
    }
    else
    {
        if (Spt->SenseInfoLength != 0)
            Irp->IoStatus.Information = *SenseInfoOff + Spt->SenseInfoLength;
        else
            Irp->IoStatus.Information = Spt->Length;
    }

    AtaDestroySynchronousUserBufferIrp(NewIrp, Srb);

    return Status;
}

static
NTSTATUS
AtaPdoDeviceControl(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    ULONG Ioctl;
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
    Ioctl = IoStack->Parameters.DeviceIoControl.IoControlCode;
    switch (Ioctl)
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
            Status = AtaPdoHandleAtaPassthrough(DevExt,
                                                Irp,
                                                IoStack,
                                                Ioctl == IOCTL_ATA_PASS_THROUGH_DIRECT);
            break;

        case IOCTL_SCSI_PASS_THROUGH:
        case IOCTL_SCSI_PASS_THROUGH_DIRECT:
            Status = AtaPdoHandleScsiPassthrough(DevExt,
                                                 Irp,
                                                 IoStack,
                                                 Ioctl == IOCTL_SCSI_PASS_THROUGH_DIRECT);
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
        PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->Device.ChanExt;

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
    AdapterDescriptor->MaximumTransferLength = ChanExt->MaximumTransferLength;
    AdapterDescriptor->MaximumPhysicalPages = BYTES_TO_PAGES(ChanExt->MaximumTransferLength);
    AdapterDescriptor->AlignmentMask = ChanExt->Common.Self->AlignmentRequirement;
    AdapterDescriptor->AdapterUsesPio = !!(ChanExt->Flags && CHANNEL_PIO_ONLY);
    AdapterDescriptor->AdapterScansDown = FALSE;
    AdapterDescriptor->CommandQueueing = FALSE; /* Disable request tagging */
    AdapterDescriptor->AcceleratedTransfer = FALSE;
    AdapterDescriptor->BusType = IS_AHCI_EXT(ChanExt) ? BusTypeSata : BusTypeAta;
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
    Capabilities->MaximumTransferLength = ChanExt->MaximumTransferLength;
    Capabilities->MaximumPhysicalPages = BYTES_TO_PAGES(ChanExt->MaximumTransferLength);
    Capabilities->SupportedAsynchronousEvents = FALSE;
    Capabilities->AlignmentMask = ChanExt->Common.Self->AlignmentRequirement;
    Capabilities->TaggedQueuing = FALSE;
    Capabilities->AdapterScansDown = FALSE;
    Capabilities->AdapterUsesPio = !!(ChanExt->Flags && CHANNEL_PIO_ONLY);

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

        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, SEARCH_FDO_DEV, NULL);
        if (!DevExt)
            break;

        ++PdoCount;
    }

    EntrySize = ALIGN_UP(sizeof(*ScsiInquiryData) - 1 + INQUIRYDATABUFFERSIZE, ULONG);
    TotalSize = sizeof(*ScsiAdapterBusInfo) + EntrySize * PdoCount;
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

    AtaScsiAddress.AsULONG = 0;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        PINQUIRYDATA InquiryData;

        DevExt = AtaFdoFindNextDeviceByPath(ChanExt, &AtaScsiAddress, SEARCH_FDO_DEV, Irp);
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
    _In_ PIO_STACK_LOCATION IoStack,
    _In_ BOOLEAN IsDirect)
{
    PSCSI_PASS_THROUGH Spt;
    PATAPORT_DEVICE_EXTENSION DevExt;
    NTSTATUS Status;

    PAGED_CODE();

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength <
        RTL_SIZEOF_THROUGH_FIELD(SCSI_PASS_THROUGH, Lun))
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

    Status = AtaPdoHandleScsiPassthrough(DevExt, Irp, IoStack, IsDirect);

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
    ULONG Ioctl;

    Status = IoAcquireRemoveLock(&ChanExt->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Ioctl = IoStack->Parameters.DeviceIoControl.IoControlCode;
    switch (Ioctl)
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
            Status = AtaFdoHandleScsiPassthrough(ChanExt,
                                                 Irp,
                                                 IoStack,
                                                 Ioctl == IOCTL_SCSI_PASS_THROUGH_DIRECT);
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
