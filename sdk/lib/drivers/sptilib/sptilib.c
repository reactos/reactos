/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ATA and SCSI Pass Through Interface for storage drivers
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * This library provides helper code for handling ATA and SCSI Pass Through IOCTLs.
 * Typically, these IRPs come from user-mode applications.
 * The handler will translate the IOCTL into an SRB and send it to the PDO.
 */

/* INCLUDES *******************************************************************/

#include <ntddk.h>
#include <ntintsafe.h>
#include <pseh/pseh2.h>
#include <scsi.h>
#include <ntddscsi.h>

#define NDEBUG
#include <debug.h>

#include "sptilib.h"
#include "sptilibp.h"

/* PRIVATE FUNCTIONS **********************************************************/

_At_(IrpContext->Srb.SenseInfoBuffer, __drv_freesMem(Mem))
static
CODE_SEG("PAGE")
VOID
SptiFreeIrpContext(
    _In_opt_ __drv_freesMem(Mem) PPASSTHROUGH_IRP_CONTEXT IrpContext)
{
    PIRP Irp;

    PAGED_CODE();

    ASSERT(IrpContext);

    Irp = IrpContext->Irp;
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

    if (IrpContext->Srb.SenseInfoBuffer)
        ExFreePoolWithTag(IrpContext->Srb.SenseInfoBuffer, TAG_SPTI);

    ExFreePoolWithTag(IrpContext, TAG_SPTI);
}

static IO_COMPLETION_ROUTINE SptiCompletionRoutine;
static
NTSTATUS
NTAPI
SptiCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
        KeSetEvent(Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

__drv_allocatesMem(Mem)
static
CODE_SEG("PAGE")
PPASSTHROUGH_IRP_CONTEXT
SptiCreateIrpContext(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP OriginalIrp,
    _In_ PVOID DataBuffer,
    _In_ ULONG DataBufferLength,
    _In_ BOOLEAN IsDirectMemoryAccess,
    _In_ BOOLEAN IsBufferReadAccess)
{
    PPASSTHROUGH_IRP_CONTEXT IrpContext;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    IrpContext = ExAllocatePoolZero(NonPagedPool, sizeof(*IrpContext), TAG_SPTI);
    if (!IrpContext)
        return NULL;

    IrpContext->Irp = Irp = IoAllocateIrp(DeviceObject->StackSize, 0);
    if (!Irp)
        goto Cleanup;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    if (DataBuffer)
    {
        if (!IoAllocateMdl(DataBuffer, DataBufferLength, FALSE, FALSE, Irp))
            goto Cleanup;
        ASSERT(Irp->MdlAddress);

        _SEH2_TRY
        {
            MmProbeAndLockPages(Irp->MdlAddress,
                                IsDirectMemoryAccess ? OriginalIrp->RequestorMode : KernelMode,
                                IsBufferReadAccess ? IoReadAccess : IoWriteAccess);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(goto Cleanup);
        }
        _SEH2_END;
    }

    IrpContext->Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
    IrpContext->Srb.Length = RTL_FIELD_SIZE(PASSTHROUGH_IRP_CONTEXT, Srb);
    IrpContext->Srb.OriginalRequest = Irp;
    IrpContext->Srb.DataBuffer = DataBuffer;
    IrpContext->Srb.DataTransferLength = DataBufferLength;
    IrpContext->Srb.SrbFlags = SRB_FLAGS_NO_QUEUE_FREEZE;

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MinorFunction = IRP_MN_SCSI_CLASS;
    IoStack->MajorFunction = IRP_MJ_SCSI;
    IoStack->Parameters.Scsi.Srb = &IrpContext->Srb;

    return IrpContext;

Cleanup:
    DPRINT1("Failed to create IRP\n");
    SptiFreeIrpContext(IrpContext);
    return NULL;
}

static
CODE_SEG("PAGE")
VOID
SptiInitializeOutputBuffer(
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG InputBufferLength = IoStack->Parameters.DeviceIoControl.InputBufferLength;

    PAGED_CODE();

    if (OutputBufferLength > InputBufferLength)
    {
        ULONG_PTR BufferStart = (ULONG_PTR)Irp->AssociatedIrp.SystemBuffer + InputBufferLength;
        RtlZeroMemory((PVOID)BufferStart, OutputBufferLength - InputBufferLength);
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
SptiCallDriver(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PPASSTHROUGH_IRP_CONTEXT IrpContext)
{
    PIRP Irp = IrpContext->Irp;
    KEVENT Event;
    NTSTATUS Status;

    PAGED_CODE();

    SptiInitializeOutputBuffer(Irp);

    // TODO: Send the IRP in an asynchronous way (do not block the user app thread)
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp,
                           SptiCompletionRoutine,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    return Status;
}

_At_(Srb->SenseInfoBuffer, __drv_allocatesMem(Mem))
static
CODE_SEG("PAGE")
NTSTATUS
SptiSrbAppendSenseBuffer(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG BufferSize)
{
    PAGED_CODE();

    if (BufferSize == 0)
    {
        Srb->SrbFlags |= SRB_FLAGS_DISABLE_AUTOSENSE;
        return STATUS_SUCCESS;
    }

    Srb->SenseInfoBuffer = ExAllocatePoolUninitialized(NonPagedPoolCacheAligned,
                                                       BufferSize,
                                                       TAG_SPTI);
    if (!Srb->SenseInfoBuffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    Srb->SenseInfoBufferLength = BufferSize;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
SptiTranslateTaskFileToCdb(
    _Out_ CDB* __restrict Cdb,
    _In_ UCHAR* __restrict TaskFile,
    _In_ USHORT AtaFlags)
{
    UCHAR Protocol;

    PAGED_CODE();

    Cdb->ATA_PASSTHROUGH16.OperationCode = SCSIOP_ATA_PASSTHROUGH16;

    Cdb->ATA_PASSTHROUGH16.Features7_0    = TaskFile[8 + 0];
    Cdb->ATA_PASSTHROUGH16.SectorCount7_0 = TaskFile[8 + 1];
    Cdb->ATA_PASSTHROUGH16.LbaLow7_0      = TaskFile[8 + 2];
    Cdb->ATA_PASSTHROUGH16.LbaMid7_0      = TaskFile[8 + 3];
    Cdb->ATA_PASSTHROUGH16.LbaHigh7_0     = TaskFile[8 + 4];
    Cdb->ATA_PASSTHROUGH16.Device         = TaskFile[8 + 5];
    Cdb->ATA_PASSTHROUGH16.Command        = TaskFile[8 + 6];

    if (AtaFlags & ATA_FLAGS_48BIT_COMMAND)
    {
        Cdb->ATA_PASSTHROUGH16.Features15_8    = TaskFile[0];
        Cdb->ATA_PASSTHROUGH16.SectorCount15_8 = TaskFile[1];
        Cdb->ATA_PASSTHROUGH16.LbaLow15_8      = TaskFile[2];
        Cdb->ATA_PASSTHROUGH16.LbaMid15_8      = TaskFile[3];
        Cdb->ATA_PASSTHROUGH16.LbaHigh15_8     = TaskFile[4];

        Cdb->ATA_PASSTHROUGH16.Extend = 1;
    }

    /* Enable the check condition to get ATA fields from the device */
    Cdb->ATA_PASSTHROUGH16.CkCond = 1;

    if (AtaFlags & (ATA_FLAGS_DATA_IN | ATA_FLAGS_DATA_OUT))
    {
        Cdb->ATA_PASSTHROUGH16.TLength = 3;

        if (AtaFlags & ATA_FLAGS_DATA_IN)
            Cdb->ATA_PASSTHROUGH16.TDir = 1;
    }

    if (AtaFlags & ATA_FLAGS_USE_DMA)
    {
        Protocol = ATA_PASSTHROUGH_PROTOCOL_DMA;
    }
    else
    {
        if (AtaFlags & (ATA_FLAGS_DATA_IN | ATA_FLAGS_DATA_OUT))
        {
            if (AtaFlags & ATA_FLAGS_DATA_IN)
                Protocol = ATA_PASSTHROUGH_PROTOCOL_PIO_DATA_IN;
            else
                Protocol = ATA_PASSTHROUGH_PROTOCOL_PIO_DATA_OUT;
        }
        else
        {
            Protocol = ATA_PASSTHROUGH_PROTOCOL_NON_DATA;
        }
    }
    Cdb->ATA_PASSTHROUGH16.Protocol = Protocol;
}

static
CODE_SEG("PAGE")
BOOLEAN
SptiTranslateAtaStatusReturnToTaskFile(
    _In_ SCSI_REQUEST_BLOCK* __restrict Srb,
    _In_ DESCRIPTOR_SENSE_DATA* __restrict SenseData,
    _Out_ UCHAR* __restrict TaskFile)
{
    BOOLEAN WasCheckCondition = FALSE;

    PAGED_CODE();

    if (!(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID))
        return WasCheckCondition;

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

    /*
     * The check condition bit will cause the APT command to return an error upon completion.
     * We should determine if there was a real error or not.
     */
    if ((Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION) &&
        (SenseData->SenseKey == SCSI_SENSE_RECOVERED_ERROR) &&
        (SenseData->AdditionalSenseCode == SCSI_ADSENSE_NO_SENSE) &&
        (SenseData->AdditionalSenseCodeQualifier ==
         SCSI_SENSEQ_ATA_PASS_THROUGH_INFORMATION_AVAILABLE))
    {
        WasCheckCondition = TRUE;
    }

    return WasCheckCondition;
}

static
CODE_SEG("PAGE")
NTSTATUS
SptiTranslateAptToSrb(
    _Out_ SCSI_REQUEST_BLOCK* __restrict Srb,
    _In_ ATA_PASS_THROUGH_EX* __restrict Apt,
    _In_ PUCHAR TaskFile)
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = SptiSrbAppendSenseBuffer(Srb,
                                      FIELD_OFFSET(DESCRIPTOR_SENSE_DATA, DescriptorBuffer) +
                                      sizeof(SCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN));
    if (!NT_SUCCESS(Status))
        return Status;

    Srb->CdbLength = RTL_FIELD_SIZE(CDB, ATA_PASSTHROUGH16);
    Srb->TimeOutValue = Apt->TimeOutValue;

    Srb->PathId = Apt->PathId;
    Srb->TargetId = Apt->TargetId;
    Srb->Lun = Apt->Lun;

    if (Apt->AtaFlags & ATA_FLAGS_DATA_IN)
        Srb->SrbFlags |= SRB_FLAGS_DATA_IN;
    if (Apt->AtaFlags & ATA_FLAGS_DATA_OUT)
        Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;

    SptiTranslateTaskFileToCdb((PCDB)Srb->Cdb, TaskFile, Apt->AtaFlags);
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
SptiInitializeApt(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack,
    _In_ BOOLEAN IsDirectMemoryAccess,
    _In_ ULONG MaximumTransferLength,
    _In_ ULONG MaximumPhysicalPages,
    _In_ PATA_PASS_THROUGH_EX Apt,
    _Out_ PPASSTHROUGH_DATA AptData,
    _Out_ PUCHAR* TaskFile,
    _Out_ PVOID* DataBuffer)
{
    ULONG StructSize;
    BOOLEAN IsNativeStructSize;

    PAGED_CODE();

#if defined(_WIN64) && defined(BUILD_WOW64_ENABLED)
    if (IoIs32bitProcess(Irp))
    {
        StructSize = sizeof(ATA_PASS_THROUGH_EX32);
        *TaskFile = (PVOID)((ULONG_PTR)Apt + FIELD_OFFSET(ATA_PASS_THROUGH_EX32, PreviousTaskFile));
        IsNativeStructSize = FALSE;
    }
    else
#endif
    {
        StructSize = sizeof(ATA_PASS_THROUGH_EX);
        *TaskFile = (PVOID)((ULONG_PTR)Apt + FIELD_OFFSET(ATA_PASS_THROUGH_EX, PreviousTaskFile));
        IsNativeStructSize = TRUE;
    }

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < StructSize)
    {
        DPRINT1("Buffer too small %lu/%lu\n",
                IoStack->Parameters.DeviceIoControl.InputBufferLength, StructSize);
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (Apt->Length != StructSize)
    {
        DPRINT1("Unknown structure size %lu\n", StructSize);
        return STATUS_REVISION_MISMATCH;
    }

    if (Apt->DataTransferLength > MaximumTransferLength)
    {
        DPRINT1("Too large transfer %lu/%lu\n", Apt->DataTransferLength, MaximumTransferLength);
        return STATUS_INVALID_PARAMETER;
    }

    if (BYTES_TO_PAGES(Apt->DataTransferLength) > MaximumPhysicalPages)
    {
        DPRINT1("Too large transfer %lu/%lu pages\n",
                BYTES_TO_PAGES(Apt->DataTransferLength), MaximumPhysicalPages);
        return STATUS_INVALID_PARAMETER;
    }

    /* Retrieve the data buffer or the data buffer offset */
#if defined(_WIN64) && defined(BUILD_WOW64_ENABLED)
    AptData->Buffer = NULL;
    RtlCopyMemory(AptData,
                  (PVOID)((ULONG_PTR)Apt + FIELD_OFFSET(ATA_PASS_THROUGH_EX, DataBufferOffset)),
                  IsNativeStructSize ? sizeof(PVOID) : sizeof(ULONG32));
#else
    DBG_UNREFERENCED_LOCAL_VARIABLE(IsNativeStructSize);

    AptData->BufferOffset = Apt->DataBufferOffset;
#endif

    if (Apt->DataTransferLength != 0)
    {
        if (IsDirectMemoryAccess)
            *DataBuffer = AptData->Buffer;
        else
            *DataBuffer = (PVOID)((ULONG_PTR)Apt + AptData->BufferOffset);
    }
    else
    {
        *DataBuffer = NULL;
    }

    // FIXME: Validate the command opcode

    if (*(PULONG_PTR)DataBuffer & DeviceObject->AlignmentRequirement)
    {
        DPRINT1("Unaligned data buffer %p:%lx\n",
                (PVOID)*(PULONG_PTR)DataBuffer, DeviceObject->AlignmentRequirement);
        return STATUS_INVALID_PARAMETER;
    }

    if (Apt->DataTransferLength & DeviceObject->AlignmentRequirement)
    {
        DPRINT1("Unaligned data transfer length %lx:%lx\n",
                Apt->DataTransferLength, DeviceObject->AlignmentRequirement);
        return STATUS_INVALID_PARAMETER;
    }

    if ((Apt->AtaFlags & (ATA_FLAGS_DATA_IN | ATA_FLAGS_DATA_OUT)) &&
        (Apt->DataTransferLength == 0))
    {
        DPRINT1("Data buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (Apt->TimeOutValue < PASSTHROUGH_CMD_TIMEOUT_MIN_SEC ||
        Apt->TimeOutValue > PASSTHROUGH_CMD_TIMEOUT_MAX_SEC)
    {
        DPRINT1("Invalid timeout value %lu\n", Apt->TimeOutValue);
        return STATUS_INVALID_PARAMETER;
    }

    if (!IsDirectMemoryAccess)
    {
        ULONG_PTR DataBufferEnd;
        NTSTATUS Status;

        Status = RtlULongPtrAdd(AptData->BufferOffset, Apt->DataTransferLength, &DataBufferEnd);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Invalid data buffer offset\n");
            return Status;
        }

        if ((Apt->Length > AptData->BufferOffset) && (Apt->DataTransferLength != 0))
        {
            DPRINT1("Data buffer overlaps APT\n");
            return STATUS_INVALID_PARAMETER;
        }

        if ((Apt->AtaFlags & ATA_FLAGS_DATA_IN) &&
            (DataBufferEnd > IoStack->Parameters.DeviceIoControl.OutputBufferLength))
        {
            DPRINT1("Data buffer outside of available space\n");
            return STATUS_INVALID_PARAMETER;
        }

        if ((Apt->AtaFlags & ATA_FLAGS_DATA_OUT) &&
            (DataBufferEnd > IoStack->Parameters.DeviceIoControl.InputBufferLength))
        {
            DPRINT1("Data buffer outside of available space\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
SptiTranslateSptToSrb(
    _Out_ SCSI_REQUEST_BLOCK* __restrict Srb,
    _In_ SCSI_PASS_THROUGH* __restrict Spt,
    _In_ PUCHAR Cdb)
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = SptiSrbAppendSenseBuffer(Srb, Spt->SenseInfoLength);
    if (!NT_SUCCESS(Status))
        return Status;

    Srb->CdbLength = Spt->CdbLength;
    Srb->TimeOutValue = Spt->TimeOutValue;

    Srb->PathId = Spt->PathId;
    Srb->TargetId = Spt->TargetId;
    Srb->Lun = Spt->Lun;

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

    RtlCopyMemory(Srb->Cdb, Cdb, Srb->CdbLength);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
SptiInitializeSpt(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack,
    _In_ BOOLEAN IsDirectMemoryAccess,
    _In_ ULONG MaximumTransferLength,
    _In_ ULONG MaximumPhysicalPages,
    _In_ PSCSI_PASS_THROUGH Spt,
    _Out_ PPASSTHROUGH_DATA SptData,
    _Out_ PULONG SenseInfoOffset,
    _Out_ PUCHAR* Cdb,
    _Out_ PVOID* DataBuffer)
{
    ULONG StructSize;
    BOOLEAN IsNativeStructSize;
    PULONG SenseInfoOffsetPtr;

    PAGED_CODE();

#if defined(_WIN64) && defined(BUILD_WOW64_ENABLED)
    if (IoIs32bitProcess(Irp))
    {
        StructSize = sizeof(SCSI_PASS_THROUGH32);
        SenseInfoOffsetPtr = (PVOID)((ULONG_PTR)Spt +
                                     FIELD_OFFSET(SCSI_PASS_THROUGH32, SenseInfoOffset));
        *Cdb = (PVOID)((ULONG_PTR)Spt + FIELD_OFFSET(SCSI_PASS_THROUGH32, Cdb));
        IsNativeStructSize = FALSE;
    }
    else
#endif
    {
        StructSize = sizeof(SCSI_PASS_THROUGH);
        SenseInfoOffsetPtr = (PVOID)((ULONG_PTR)Spt +
                                     FIELD_OFFSET(SCSI_PASS_THROUGH, SenseInfoOffset));
        *Cdb = (PVOID)((ULONG_PTR)Spt + FIELD_OFFSET(SCSI_PASS_THROUGH, Cdb));
        IsNativeStructSize = TRUE;
    }

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < StructSize)
    {
        DPRINT1("Buffer too small %lu/%lu\n",
                IoStack->Parameters.DeviceIoControl.InputBufferLength, StructSize);
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (Spt->Length != StructSize)
    {
        DPRINT1("Unknown structure size %lu\n", StructSize);
        return STATUS_REVISION_MISMATCH;
    }

    if (Spt->DataTransferLength > MaximumTransferLength)
    {
        DPRINT1("Too large transfer %lu/%lu\n", Spt->DataTransferLength, MaximumTransferLength);
        return STATUS_INVALID_PARAMETER;
    }

    if (BYTES_TO_PAGES(Spt->DataTransferLength) > MaximumPhysicalPages)
    {
        DPRINT1("Too large transfer %lu/%lu pages\n",
                BYTES_TO_PAGES(Spt->DataTransferLength), MaximumPhysicalPages);
        return STATUS_INVALID_PARAMETER;
    }

    /* Retrieve the data buffer or the data buffer offset */
#if defined(_WIN64) && defined(BUILD_WOW64_ENABLED)
    SptData->Buffer = NULL;
    RtlCopyMemory(SptData,
                  (PVOID)((ULONG_PTR)Spt + FIELD_OFFSET(SCSI_PASS_THROUGH, DataBufferOffset)),
                  IsNativeStructSize ? sizeof(PVOID) : sizeof(ULONG32));
#else
    DBG_UNREFERENCED_LOCAL_VARIABLE(IsNativeStructSize);

    SptData->BufferOffset = Spt->DataBufferOffset;
#endif
    *SenseInfoOffset = *SenseInfoOffsetPtr;

    if (Spt->DataTransferLength != 0)
    {
        if (IsDirectMemoryAccess)
            *DataBuffer = SptData->Buffer;
        else
            *DataBuffer = (PVOID)((ULONG_PTR)Spt + SptData->BufferOffset);
    }
    else
    {
        *DataBuffer = NULL;
    }

    // FIXME: Validate the command opcode

    if (*(PULONG_PTR)DataBuffer & DeviceObject->AlignmentRequirement)
    {
        DPRINT1("Unaligned data buffer %p:%lx\n",
                (PVOID)*(PULONG_PTR)DataBuffer, DeviceObject->AlignmentRequirement);
        return STATUS_INVALID_PARAMETER;
    }

    if (Spt->DataTransferLength & DeviceObject->AlignmentRequirement)
    {
        DPRINT1("Unaligned data transfer length %lx:%lx\n",
                Spt->DataTransferLength, DeviceObject->AlignmentRequirement);
        return STATUS_INVALID_PARAMETER;
    }

    if (Spt->TimeOutValue < PASSTHROUGH_CMD_TIMEOUT_MIN_SEC ||
        Spt->TimeOutValue > PASSTHROUGH_CMD_TIMEOUT_MAX_SEC)
    {
        DPRINT1("Invalid timeout value %lu\n", Spt->TimeOutValue);
        return STATUS_INVALID_PARAMETER;
    }

    if (Spt->CdbLength > RTL_FIELD_SIZE(SCSI_REQUEST_BLOCK, Cdb))
    {
        DPRINT1("Invalid CDB size %u\n", Spt->CdbLength);
        return STATUS_INVALID_PARAMETER;
    }

    if (Spt->DataIn > SCSI_IOCTL_DATA_UNSPECIFIED)
    {
        DPRINT1("Unknown DataIn value %u\n", Spt->DataIn);
        return STATUS_INVALID_PARAMETER;
    }

    if (Spt->SenseInfoLength != 0)
    {
        ULONG SenseBufferEnd;
        NTSTATUS Status;

        Status = RtlULongAdd(*SenseInfoOffset, Spt->SenseInfoLength, &SenseBufferEnd);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Invalid sense buffer offset\n");
            return Status;
        }

        if (Spt->Length > *SenseInfoOffset)
        {
            DPRINT1("Sense buffer overlaps SPT\n");
            return STATUS_INVALID_PARAMETER;
        }

        if (!IsDirectMemoryAccess)
        {
            if ((SenseBufferEnd > SptData->BufferOffset) && (Spt->DataTransferLength != 0))
            {
                DPRINT1("Sense buffer overlaps data buffer\n");
                return STATUS_INVALID_PARAMETER;
            }

            if (SenseBufferEnd > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
            {
                DPRINT1("Sense buffer outside of available space\n");
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    if (!IsDirectMemoryAccess)
    {
        ULONG_PTR DataBufferEnd;
        NTSTATUS Status;

        Status = RtlULongPtrAdd(SptData->BufferOffset, Spt->DataTransferLength, &DataBufferEnd);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Invalid data buffer offset\n");
            return Status;
        }

        if ((Spt->Length > SptData->BufferOffset) && (Spt->DataTransferLength != 0))
        {
            DPRINT1("Data buffer inside of structure bounds\n");
            return STATUS_INVALID_PARAMETER;
        }

        if (Spt->DataIn == SCSI_IOCTL_DATA_IN || Spt->DataIn == SCSI_IOCTL_DATA_UNSPECIFIED)
        {
            if (DataBufferEnd > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
            {
                DPRINT1("Data buffer outside of available space\n");
                return STATUS_INVALID_PARAMETER;
            }
        }

        if (Spt->DataIn == SCSI_IOCTL_DATA_OUT || Spt->DataIn == SCSI_IOCTL_DATA_UNSPECIFIED)
        {
            if (DataBufferEnd > IoStack->Parameters.DeviceIoControl.InputBufferLength)
            {
                DPRINT1("Data buffer outside of available space\n");
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS ***********************************************************/

CODE_SEG("PAGE")
NTSTATUS
SptiHandleAtaPassthru(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ ULONG MaximumTransferLength,
    _In_ ULONG MaximumPhysicalPages)
{
    PATA_PASS_THROUGH_EX Apt = Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION IoStack;
    BOOLEAN IsDirectMemoryAccess;
    PASSTHROUGH_DATA AptData;
    PUCHAR TaskFile;
    NTSTATUS Status;
    PVOID DataBuffer;
    PPASSTHROUGH_IRP_CONTEXT IrpContext;
    PSCSI_REQUEST_BLOCK Srb;

    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(GET_IOCTL(IoStack) == IOCTL_ATA_PASS_THROUGH ||
           GET_IOCTL(IoStack) == IOCTL_ATA_PASS_THROUGH_DIRECT);

    IsDirectMemoryAccess = (GET_IOCTL(IoStack) == IOCTL_ATA_PASS_THROUGH_DIRECT);

    Status = SptiInitializeApt(DeviceObject,
                               Irp,
                               IoStack,
                               IsDirectMemoryAccess,
                               MaximumTransferLength,
                               MaximumPhysicalPages,
                               Apt,
                               &AptData,
                               &TaskFile,
                               &DataBuffer);
    if (!NT_SUCCESS(Status))
        return Status;

    DPRINT("APT: Flags %x DTL %lu\n", Apt->AtaFlags, Apt->DataTransferLength);
    DPRINT("APT: Curr %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
           TaskFile[8 + 0], TaskFile[8 + 1], TaskFile[8 + 2], TaskFile[8 + 3],
           TaskFile[8 + 4], TaskFile[8 + 5], TaskFile[8 + 6],
           Apt->AtaFlags,
           Apt->DataTransferLength);
    if (Apt->AtaFlags & ATA_FLAGS_48BIT_COMMAND)
    {
        DPRINT("APT: Prev %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
               TaskFile[0], TaskFile[1], TaskFile[2], TaskFile[3],
               TaskFile[4], TaskFile[5], TaskFile[6]);
    }

    IrpContext = SptiCreateIrpContext(DeviceObject,
                                      Irp,
                                      DataBuffer,
                                      Apt->DataTransferLength,
                                      IsDirectMemoryAccess,
                                      !!(Apt->AtaFlags & ATA_FLAGS_DATA_OUT));
    if (!IrpContext)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SptiTranslateAptToSrb(&IrpContext->Srb, Apt, TaskFile);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    Status = SptiCallDriver(DeviceObject, IrpContext);

    Srb = &IrpContext->Srb;

    if (SptiTranslateAtaStatusReturnToTaskFile(Srb, Srb->SenseInfoBuffer, TaskFile))
        Status = STATUS_SUCCESS;

    /* Update the output buffer size */
    Apt->DataTransferLength = Srb->DataTransferLength;
    if (IsDirectMemoryAccess)
    {
        Irp->IoStatus.Information = Apt->Length;
    }
    else
    {
        if ((Apt->AtaFlags & ATA_FLAGS_DATA_IN) && (AptData.BufferOffset != 0))
            Irp->IoStatus.Information = AptData.BufferOffset + Apt->DataTransferLength;
        else
            Irp->IoStatus.Information = Apt->Length;
    }

    DPRINT("APT: Return %lx, DTL %lu, %lu bytes\n",
           Status,
           Apt->DataTransferLength,
           (ULONG)Irp->IoStatus.Information);
    DPRINT("APT: Return curr %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
           TaskFile[8 + 0], TaskFile[8 + 1], TaskFile[8 + 2], TaskFile[8 + 3],
           TaskFile[8 + 4], TaskFile[8 + 5], TaskFile[8 + 6]);
    if (Apt->AtaFlags & ATA_FLAGS_48BIT_COMMAND)
    {
        DPRINT("APT: Return prev %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
               TaskFile[0], TaskFile[1], TaskFile[2], TaskFile[3],
               TaskFile[4], TaskFile[5], TaskFile[6]);
    }

Cleanup:
    SptiFreeIrpContext(IrpContext);
    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
SptiHandleScsiPassthru(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ ULONG MaximumTransferLength,
    _In_ ULONG MaximumPhysicalPages)
{
    PSCSI_PASS_THROUGH Spt = Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION IoStack;
    BOOLEAN IsDirectMemoryAccess;
    PASSTHROUGH_DATA SptData;
    ULONG SenseInfoOffset;
    PUCHAR Cdb;
    NTSTATUS Status;
    PVOID DataBuffer;
    PPASSTHROUGH_IRP_CONTEXT IrpContext;
    PSCSI_REQUEST_BLOCK Srb;

    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(GET_IOCTL(IoStack) == IOCTL_SCSI_PASS_THROUGH ||
           GET_IOCTL(IoStack) == IOCTL_SCSI_PASS_THROUGH_DIRECT);

    IsDirectMemoryAccess = (GET_IOCTL(IoStack) == IOCTL_SCSI_PASS_THROUGH_DIRECT);

    Status = SptiInitializeSpt(DeviceObject,
                               Irp,
                               IoStack,
                               IsDirectMemoryAccess,
                               MaximumTransferLength,
                               MaximumPhysicalPages,
                               Spt,
                               &SptData,
                               &SenseInfoOffset,
                               &Cdb,
                               &DataBuffer);
    if (!NT_SUCCESS(Status))
        return Status;

    DPRINT("SPT: Flags %u, DTL %lu, sense len %u\n",
           Spt->DataIn,
           Spt->DataTransferLength,
           Spt->SenseInfoLength);
    DPRINT("SPT: CDB %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
           Cdb[0], Cdb[1], Cdb[2], Cdb[3], Cdb[4], Cdb[5], Cdb[6], Cdb[7], Cdb[8], Cdb[9]);

    IrpContext = SptiCreateIrpContext(DeviceObject,
                                      Irp,
                                      DataBuffer,
                                      Spt->DataTransferLength,
                                      IsDirectMemoryAccess,
                                      !!(Spt->DataIn == SCSI_IOCTL_DATA_OUT));
    if (!IrpContext)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SptiTranslateSptToSrb(&IrpContext->Srb, Spt, Cdb);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    Status = SptiCallDriver(DeviceObject, IrpContext);

    Srb = &IrpContext->Srb;

    /*
     * Low-level storage drivers map SRB_STATUS_DATA_OVERRUN to STATUS_BUFFER_OVERFLOW.
     * This SRB status is usually set on an underrun and means success in that case.
     */
    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN)
        Status = STATUS_SUCCESS;

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)
    {
        ASSERT(Srb->SenseInfoBufferLength <= Spt->SenseInfoLength);

        Spt->SenseInfoLength = Srb->SenseInfoBufferLength;

        /* Copy the sense buffer back */
        RtlCopyMemory((PVOID)((ULONG_PTR)Spt + SenseInfoOffset),
                      Srb->SenseInfoBuffer,
                      Srb->SenseInfoBufferLength);
    }
    else
    {
        Spt->SenseInfoLength = 0;
    }

    Spt->ScsiStatus = Srb->ScsiStatus;

    /* Update the output buffer size */
    Spt->DataTransferLength = Srb->DataTransferLength;
    if (IsDirectMemoryAccess)
    {
        if (Spt->SenseInfoLength != 0)
            Irp->IoStatus.Information = SenseInfoOffset + Spt->SenseInfoLength;
        else
            Irp->IoStatus.Information = Spt->Length;
    }
    else
    {
        if ((Srb->SrbFlags & SRB_FLAGS_DATA_IN) && (SptData.BufferOffset != 0))
            Irp->IoStatus.Information = SptData.BufferOffset + Spt->DataTransferLength;
        else if (Spt->SenseInfoLength != 0)
            Irp->IoStatus.Information = SenseInfoOffset + Spt->SenseInfoLength;
        else
            Irp->IoStatus.Information = Spt->Length;
    }

    DPRINT("SPT: Return %lx, DTL %lu, "
           "Srb status %02x, SCSI status %02x, sense len %u, %lu bytes\n",
           Status,
           Spt->DataTransferLength,
           Srb->SrbStatus,
           Spt->ScsiStatus,
           Spt->SenseInfoLength,
           (ULONG)Irp->IoStatus.Information);

Cleanup:
    SptiFreeIrpContext(IrpContext);
    return Status;
}
