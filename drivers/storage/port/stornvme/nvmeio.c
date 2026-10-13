/*
 * PROJECT:     ReactOS NVM Express Miniport Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Read, write and flush translation
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "stornvme.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/**
 * @brief Returns the page aligned page that follows the request context.
 *
 * Holds the PRP list of a transfer, or the data of an admin command issued for an SRB.
 */
PVOID
NvmpContextPage(
    _In_ PNVME_REQUEST_CONTEXT Context)
{
    ULONG_PTR Address = (ULONG_PTR)(Context + 1);

    return (PVOID)ROUND_TO_PAGES(Address);
}


/**
 * @brief Fills PRP1 and PRP2 from the scatter gather list, using a PRP list when needed.
 *
 * Only the first entry may have a page offset.
 */
static
BOOLEAN
NvmpBuildPrp(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_REQUEST_CONTEXT Context,
    _In_ PSTOR_SCATTER_GATHER_LIST ScatterGather,
    _In_ PNVME_COMMAND Command)
{
    PULONGLONG List;
    PHYSICAL_ADDRESS ListAddress;
    ULONGLONG Address;
    ULONG Element;
    ULONG Offset;
    ULONG Count = 0;
    ULONG Length;

    List = NvmpContextPage(Context);

    for (Element = 0; Element < ScatterGather->NumberOfElements; Element++)
    {
        Address = ScatterGather->List[Element].PhysicalAddress.QuadPart;
        Length = ScatterGather->List[Element].Length;

        Offset = (ULONG)(Address & (Adapter->PageSize - 1));
        if (Offset != 0 && Count != 0)
        {
            DPRINT1("Transfer element %lu is not page aligned\n", Element);
            return FALSE;
        }

        while (Length != 0)
        {
            ULONG Step = Adapter->PageSize - Offset;

            if (Step > Length)
                Step = Length;

            if (Count >= NVME_MAX_PRP_ENTRIES)
            {
                DPRINT1("Transfer needs more than %lu region pages\n",
                        (ULONG)NVME_MAX_PRP_ENTRIES);
                return FALSE;
            }

            List[Count++] = Address;

            Address += Step;
            Length -= Step;
            Offset = 0;
        }
    }

    if (Count == 0)
    {
        DPRINT1("Transfer describes no memory\n");
        return FALSE;
    }

    Command->PRP1 = List[0];

    if (Count == 1)
    {
        Command->PRP2 = 0;
        return TRUE;
    }

    if (Count == 2)
    {
        Command->PRP2 = List[1];
        return TRUE;
    }

    /* PRP2 points to a list holding every entry after the first */
    ListAddress = StorPortGetPhysicalAddress(Adapter, NULL, &List[1], &Length);
    if (ListAddress.QuadPart == 0 || Length < (Count - 1) * sizeof(*List))
    {
        DPRINT1("Region page list is not addressable\n");
        return FALSE;
    }

    Command->PRP2 = ListAddress.QuadPart;

    return TRUE;
}


/**
 * @brief Builds an NVM Read or Write command from a SCSI READ or WRITE (10) or (16).
 *
 * The block count comes from the SRB transfer length, the CDB only supplies the LBA and FUA.
 *
 * @return TRUE if the request is ready to post, FALSE if it was already completed.
 */
BOOLEAN
NvmpBuildReadWrite(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ PNVME_NAMESPACE Namespace,
    _In_ PNVME_REQUEST_CONTEXT Context,
    _In_ PCDB Cdb)
{
    PNVME_COMMAND Command = &Context->Command;
    PSTOR_SCATTER_GATHER_LIST ScatterGather;
    ULONGLONG BlockAddress;
    ULONG Length;
    BOOLEAN Write;

    Length = SrbGetDataTransferLength(Srb);

    if ((SrbGetSrbFlags(Srb) & (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)) == 0 ||
        Length == 0 ||
        (Length & (Namespace->BlockSize - 1)) != 0)
    {
        NvmpCompleteIllegalRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST,
                                   SCSI_ADSENSE_INVALID_CDB);
        return FALSE;
    }

    if (Cdb->CDB6GENERIC.OperationCode == SCSIOP_READ16 ||
        Cdb->CDB6GENERIC.OperationCode == SCSIOP_WRITE16)
    {
        REVERSE_BYTES_QUAD(&BlockAddress, &Cdb->CDB16.LogicalBlock);
    }
    else
    {
        ULONG Address;

        REVERSE_BYTES(&Address, &Cdb->CDB10.LogicalBlockByte0);
        BlockAddress = Address;
    }

    Write = (Cdb->CDB6GENERIC.OperationCode == SCSIOP_WRITE ||
             Cdb->CDB6GENERIC.OperationCode == SCSIOP_WRITE16);

    Command->CDW0.OPC = Write ? NVME_NVM_COMMAND_WRITE : NVME_NVM_COMMAND_READ;
    Command->NSID = Namespace->NamespaceId;
    Command->u.READWRITE.LBALOW = (ULONG)BlockAddress;
    Command->u.READWRITE.LBAHIGH = (ULONG)(BlockAddress >> 32);

    /* 0 based */
    Command->u.READWRITE.CDW12.NLB = (Length >> Namespace->BlockShift) - 1;

    /* Byte 1 bit 3 in both the 10 and 16 byte forms */
    Command->u.READWRITE.CDW12.FUA = Cdb->CDB10.ForceUnitAccess;

    ScatterGather = StorPortGetScatterGatherList(Adapter, Srb);
    if (ScatterGather == NULL)
    {
        DPRINT1("Request has no scatter gather list\n");
        NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_ERROR);
        return FALSE;
    }

    if (!NvmpBuildPrp(Adapter, Context, ScatterGather, Command))
    {
        NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_ERROR);
        return FALSE;
    }

    return TRUE;
}


/**
 * @brief Builds an NVM Flush for SYNCHRONIZE CACHE (10).
 *
 * @return TRUE if the request is ready to post, FALSE if it was already completed.
 */
BOOLEAN
NvmpBuildFlush(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb)
{
    PNVME_REQUEST_CONTEXT Context = SrbGetMiniportContext(Srb);
    UCHAR Lun;

    /* No volatile write cache, nothing to flush */
    if (!Adapter->VolatileWriteCache)
    {
        NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
        return FALSE;
    }

    SrbGetPathTargetLun(Srb, NULL, NULL, &Lun);

    Context->Command.CDW0.OPC = NVME_NVM_COMMAND_FLUSH;
    Context->Command.NSID = (ULONG)Lun + 1;

    return TRUE;
}


/**
 * @brief Fills the free command identifier stack.
 *
 * Holds depth - 1 identifiers, since a full queue keeps one slot empty.
 */
VOID
NvmpInitializeCommandIds(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    ULONG Index;

    Adapter->FreeCommandCount = Adapter->IoQueueDepth - 1;

    for (Index = 0; Index < Adapter->FreeCommandCount; Index++)
        Adapter->FreeCommandIds[Index] = (USHORT)Index;
}


/**
 * @brief Submits a built command to the admin or IO queue.
 */
VOID
NvmpPostCommand(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb)
{
    PNVME_REQUEST_CONTEXT Context;
    NVME_QUEUE_LOCK Lock;
    USHORT CommandId;
    BOOLEAN Submitted = FALSE;

    Context = SrbGetMiniportContext(Srb);

    /* Serializes against the interrupt handler, which also uses the request tables */
    NvmpAcquireQueueLock(Adapter, &Lock);

    /* A reset in progress also empties the free identifier stack */
    if (Adapter->State == NvmeAdapterRunning)
    {
        if (Context->AdminCompletion != NULL)
        {
            Submitted = NvmpSubmitAdminRequest(Adapter, Context);
        }
        else if (Adapter->FreeCommandCount != 0)
        {
            CommandId = Adapter->FreeCommandIds[--Adapter->FreeCommandCount];
            Context->Command.CDW0.CID = CommandId;

            Adapter->Requests[CommandId] = Context;

            NvmpSubmitCommand(Adapter, &Adapter->IoQueue, &Context->Command);
            Submitted = TRUE;
        }
    }

    NvmpReleaseQueueLock(Adapter, &Lock);

    /* Storport will retry it */
    if (!Submitted)
        NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_BUSY);
}


/**
 * @brief Completes every outstanding request with SrbStatus.
 *
 * Reset path only, the caller already owns the queues.
 */
VOID
NvmpFailOutstandingRequests(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ UCHAR SrbStatus)
{
    PNVME_REQUEST_CONTEXT Context;
    ULONG CommandId;

    for (CommandId = 0; CommandId < Adapter->IoQueueDepth; CommandId++)
    {
        Context = Adapter->Requests[CommandId];
        if (Context == NULL)
            continue;

        Adapter->Requests[CommandId] = NULL;

        SrbSetDataTransferLength(Context->Srb, 0);
        NvmpCompleteRequest(Adapter, Context->Srb, SrbStatus);
    }

    for (CommandId = 0; CommandId < NVME_ADMIN_SLOTS; CommandId++)
    {
        Context = Adapter->AdminRequests[CommandId];
        if (Context == NULL)
            continue;

        Adapter->AdminRequests[CommandId] = NULL;

        SrbSetDataTransferLength(Context->Srb, 0);
        NvmpCompleteRequest(Adapter, Context->Srb, SrbStatus);
    }

    /* Rebuilt by NvmpInitializeCommandIds once the controller is back */
    Adapter->FreeCommandCount = 0;
}


/**
 * @brief Sense data that describes a failed NVMe command.
 */
typedef struct _NVME_STATUS_SENSE
{
    UCHAR ScsiStatus;
    UCHAR SenseKey;
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
} NVME_STATUS_SENSE, *PNVME_STATUS_SENSE;


/**
 * @brief Maps a generic command status to sense data.
 *
 * @return FALSE if the status is reported without sense data.
 */
static
BOOLEAN
NvmpSenseForGenericStatus(
    _In_ NVME_COMMAND_STATUS Status,
    _Out_ PNVME_STATUS_SENSE Sense)
{
    switch (Status.SC)
    {
        case NVME_STATUS_INVALID_COMMAND_OPCODE:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_ILLEGAL_COMMAND;
            return TRUE;

        case NVME_STATUS_INVALID_FIELD_IN_COMMAND:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_INVALID_CDB;
            return TRUE;

        case NVME_STATUS_COMMAND_ID_CONFLICT:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_INVALID_TOKEN;
            return TRUE;

        case NVME_STATUS_DATA_TRANSFER_ERROR:
        case NVME_STATUS_NVM_CAPACITY_EXCEEDED:
            Sense->SenseKey = SCSI_SENSE_MEDIUM_ERROR;
            return TRUE;

        case NVME_STATUS_COMMAND_ABORTED_DUE_TO_POWER_LOSS_NOTIFICATION:
            Sense->ScsiStatus = SCSISTAT_COMMAND_TERMINATED;
            Sense->SenseKey = SCSI_SENSE_ABORTED_COMMAND;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_WARNING;
            Sense->AdditionalSenseCodeQualifier = SCSI_SENSEQ_POWER_LOSS_EXPECTED;
            return TRUE;

        case NVME_STATUS_INTERNAL_DEVICE_ERROR:
            Sense->SenseKey = SCSI_SENSE_HARDWARE_ERROR;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_INTERNAL_TARGET_FAILURE;
            return TRUE;

        case NVME_STATUS_COMMAND_ABORT_REQUESTED:
        case NVME_STATUS_COMMAND_ABORTED_DUE_TO_SQ_DELETION:
        case NVME_STATUS_COMMAND_ABORTED_DUE_TO_FAILED_FUSED_COMMAND:
        case NVME_STATUS_COMMAND_ABORTED_DUE_TO_FAILED_MISSING_COMMAND:
            Sense->ScsiStatus = SCSISTAT_COMMAND_TERMINATED;
            Sense->SenseKey = SCSI_SENSE_ABORTED_COMMAND;
            return TRUE;

        case NVME_STATUS_INVALID_NAMESPACE_OR_FORMAT:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_ACCESS_DENIED;
            Sense->AdditionalSenseCodeQualifier = SCSI_SENSEQ_INVALID_LU_ID;
            return TRUE;

        case NVME_STATUS_COMMAND_SEQUENCE_ERROR:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_COMMAND_SEQUENCE_ERROR;
            return TRUE;

        case NVME_STATUS_INVALID_SGL_LAST_SEGMENT_DESCR:
        case NVME_STATUS_INVALID_NUMBER_OF_SGL_DESCR:
        case NVME_STATUS_DATA_SGL_LENGTH_INVALID:
        case NVME_STATUS_METADATA_SGL_LENGTH_INVALID:
        case NVME_STATUS_SGL_DESCR_TYPE_INVALID:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            return TRUE;

        case NVME_STATUS_NVM_LBA_OUT_OF_RANGE:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_ILLEGAL_BLOCK;
            return TRUE;

        case NVME_STATUS_NVM_NAMESPACE_NOT_READY:
            /* Becoming ready unless the controller says a retry will not help */
            Sense->SenseKey = SCSI_SENSE_NOT_READY;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_LUN_NOT_READY;
            if (!Status.DNR)
                Sense->AdditionalSenseCodeQualifier = SCSI_SENSEQ_BECOMING_READY;
            return TRUE;

        case NVME_STATUS_NVM_RESERVATION_CONFLICT:
            Sense->ScsiStatus = SCSISTAT_RESERVATION_CONFLICT;
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_COMMAND_SEQUENCE_ERROR;
            Sense->AdditionalSenseCodeQualifier = SCSI_SENSEQ_PREVIOUS_RESERVATION_CONFLICT;
            return TRUE;

        default:
            return FALSE;
    }
}


/**
 * @brief Maps a command specific status to sense data.
 *
 * @return FALSE if the status is reported without sense data.
 */
static
BOOLEAN
NvmpSenseForCommandSpecificStatus(
    _In_ NVME_COMMAND_STATUS Status,
    _Out_ PNVME_STATUS_SENSE Sense)
{
    switch (Status.SC)
    {
        case NVME_STATUS_COMPLETION_QUEUE_INVALID:
        case NVME_STATUS_INVALID_QUEUE_IDENTIFIER:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_INVALID_FIELD_PARAMETER_LIST;
            return TRUE;

        case NVME_STATUS_MAX_QUEUE_SIZE_EXCEEDED:
        case NVME_STATUS_ABORT_COMMAND_LIMIT_EXCEEDED:
        case NVME_STATUS_ASYNC_EVENT_REQUEST_LIMIT_EXCEEDED:
        case NVME_STATUS_INVALID_FIRMWARE_IMAGE:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            return TRUE;

        case NVME_STATUS_INVALID_FIRMWARE_SLOT:
        case NVME_STATUS_INVALID_INTERRUPT_VECTOR:
        case NVME_STATUS_INVALID_LOG_PAGE:
        case NVME_STATUS_INVALID_QUEUE_DELETION:
        case NVME_STATUS_FEATURE_ID_NOT_SAVEABLE:
        case NVME_STATUS_FEATURE_NOT_CHANGEABLE:
        case NVME_STATUS_FEATURE_NOT_NAMESPACE_SPECIFIC:
        case NVME_STATUS_NVM_CONFLICTING_ATTRIBUTES:
        case NVME_STATUS_NVM_INVALID_PROTECTION_INFORMATION:
        case NVME_STATUS_NVM_ATTEMPTED_WRITE_TO_READ_ONLY_RANGE:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_INVALID_CDB;
            return TRUE;

        case NVME_STATUS_INVALID_FORMAT:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_FORMAT_CORRUPTED;
            Sense->AdditionalSenseCodeQualifier = SCSI_SENSEQ_FORMAT_COMMAND_FAILED;
            return TRUE;

        case NVME_STATUS_FIRMWARE_ACTIVATION_REQUIRES_CONVENTIONAL_RESET:
        case NVME_STATUS_FIRMWARE_ACTIVATION_REQUIRES_NVM_SUBSYSTEM_RESET:
            Sense->SenseKey = SCSI_SENSE_UNIT_ATTENTION;
            return TRUE;

        default:
            return FALSE;
    }
}


/**
 * @brief Maps a media and data integrity status to sense data.
 *
 * The application and reference tag errors report ASC 0x11 with ASCQ 2 and 3.
 *
 * @return FALSE if the status is reported without sense data.
 */
static
BOOLEAN
NvmpSenseForMediaStatus(
    _In_ NVME_COMMAND_STATUS Status,
    _Out_ PNVME_STATUS_SENSE Sense)
{
    Sense->SenseKey = SCSI_SENSE_MEDIUM_ERROR;

    switch (Status.SC)
    {
        case NVME_STATUS_NVM_WRITE_FAULT:
            Sense->AdditionalSenseCode = SCSI_ADSENSE_WRITE;
            return TRUE;

        case NVME_STATUS_NVM_UNRECOVERED_READ_ERROR:
            Sense->AdditionalSenseCode = SCSI_ADSENSE_UNRECOVERED_ERROR;
            return TRUE;

        case NVME_STATUS_NVM_END_TO_END_GUARD_CHECK_ERROR:
            Sense->AdditionalSenseCode = SCSI_ADSENSE_CRC_OR_ECC_ERROR;
            Sense->AdditionalSenseCodeQualifier = SCSI_SENSEQ_LOGICAL_BLOCK_GUARD_CHECK_FAILED;
            return TRUE;

        case NVME_STATUS_NVM_END_TO_END_APPLICATION_TAG_CHECK_ERROR:
            Sense->AdditionalSenseCode = SCSI_ADSENSE_UNRECOVERED_ERROR;
            Sense->AdditionalSenseCodeQualifier = NVME_SENSEQ_APPLICATION_TAG_CHECK_FAILED;
            return TRUE;

        case NVME_STATUS_NVM_END_TO_END_REFERENCE_TAG_CHECK_ERROR:
            Sense->AdditionalSenseCode = SCSI_ADSENSE_UNRECOVERED_ERROR;
            Sense->AdditionalSenseCodeQualifier = NVME_SENSEQ_REFERENCE_TAG_CHECK_FAILED;
            return TRUE;

        case NVME_STATUS_NVM_COMPARE_FAILURE:
            Sense->SenseKey = SCSI_SENSE_MISCOMPARE;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_MISCOMPARE_DURING_VERIFY_OPERATION;
            return TRUE;

        case NVME_STATUS_NVM_ACCESS_DENIED:
            Sense->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            Sense->AdditionalSenseCode = SCSI_ADSENSE_ACCESS_DENIED;
            Sense->AdditionalSenseCodeQualifier = SCSI_SENSEQ_INVALID_LU_ID;
            return TRUE;

        default:
            return FALSE;
    }
}


/**
 * @brief Completes a request whose NVMe command failed.
 *
 * Statuses with no SCSI equivalent complete with SRB_STATUS_ERROR and no sense data.
 */
VOID
NvmpCompleteWithStatus(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ NVME_COMMAND_STATUS Status)
{
    NVME_STATUS_SENSE Sense;
    BOOLEAN HasSense;

    DPRINT1("Command failed, type %u code 0x%02x\n", Status.SCT, Status.SC);

    RtlZeroMemory(&Sense, sizeof(Sense));
    Sense.ScsiStatus = SCSISTAT_CHECK_CONDITION;

    switch (Status.SCT)
    {
        case NVME_STATUS_TYPE_GENERIC_COMMAND:
            HasSense = NvmpSenseForGenericStatus(Status, &Sense);
            break;

        case NVME_STATUS_TYPE_COMMAND_SPECIFIC:
            HasSense = NvmpSenseForCommandSpecificStatus(Status, &Sense);
            break;

        case NVME_STATUS_TYPE_MEDIA_ERROR:
            HasSense = NvmpSenseForMediaStatus(Status, &Sense);
            break;

        default:
            HasSense = FALSE;
            break;
    }

    SrbSetSrbStatus(Srb, SRB_STATUS_ERROR);

    if (HasSense)
    {
        NvmpSetSenseData(Srb,
                         Sense.ScsiStatus,
                         Sense.SenseKey,
                         Sense.AdditionalSenseCode,
                         Sense.AdditionalSenseCodeQualifier);
    }
    else
    {
        SrbSetScsiStatus(Srb, SCSISTAT_GOOD);
    }

    StorPortNotification(RequestComplete, Adapter, Srb);
}


/**
 * @brief Completes the IO request that a completion entry belongs to.
 */
VOID
NvmpCompleteFromEntry(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_COMPLETION_ENTRY Completion)
{
    PNVME_REQUEST_CONTEXT Context;
    USHORT CommandId = Completion->DW3.CID;

    if (CommandId >= Adapter->IoQueueDepth)
    {
        DPRINT1("Completion names command %u, out of range\n", CommandId);
        return;
    }

    Context = Adapter->Requests[CommandId];
    if (Context == NULL)
    {
        DPRINT1("Completion names command %u, which is not outstanding\n", CommandId);
        return;
    }

    Adapter->Requests[CommandId] = NULL;
    Adapter->FreeCommandIds[Adapter->FreeCommandCount++] = CommandId;

    if (Completion->DW3.Status.SC == NVME_STATUS_SUCCESS_COMPLETION &&
        Completion->DW3.Status.SCT == NVME_STATUS_TYPE_GENERIC_COMMAND)
    {
        NvmpCompleteRequest(Adapter, Context->Srb, SRB_STATUS_SUCCESS);
        return;
    }

    NvmpCompleteWithStatus(Adapter, Context->Srb, Completion->DW3.Status);
}
