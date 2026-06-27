/*
 * PROJECT:     QLogic ISP SCSI Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * HW access code was taken from the Matthew Jacob's
 * multiplatform driver for ISP chipsets.
 * Copyright (C) 2000-2007 by Matthew Jacob <mjacob@NetBSD.org>
 */

/* INCLUDES *******************************************************************/

#include "qla1xxx.h"

#ifndef ISP_FIRMWARE_DISABLE
#include "firmware/asm_1040.h"
#include "firmware/asm_1080.h"
#include "firmware/asm_12160.h"
#endif

/* GLOBALS ********************************************************************/

static HW_INTERRUPT IspHwInterrupt;
static HW_INITIALIZE IspHwInitialize;

static const USHORT IspMailboxTestPattern[QL_MAX_MAILBOX - 1] =
{
    0x1234,
    0x2345,
    0x3456,
    0x4567,
    0x5678,
    0x6789,
    0x789A
};

static const UCHAR IspCompletionStatusToSrbStatus[] =
{
    SRB_STATUS_SUCCESS,                 // RQCS_COMPLETE
    SRB_STATUS_SELECTION_TIMEOUT,       // RQCS_INCOMPLETE
    SRB_STATUS_ERROR,                   // RQCS_DMA_ERROR
    SRB_STATUS_ERROR,                   // RQCS_TRANSPORT_ERROR
    SRB_STATUS_BUS_RESET,               // RQCS_RESET_OCCURRED
    SRB_STATUS_ABORTED,                 // RQCS_ABORTED
    SRB_STATUS_TIMEOUT,                 // RQCS_TIMEOUT
    SRB_STATUS_DATA_OVERRUN,            // RQCS_DATA_OVERRUN
    SRB_STATUS_PHASE_SEQUENCE_FAILURE,  // RQCS_COMMAND_OVERRUN
    SRB_STATUS_PHASE_SEQUENCE_FAILURE,  // RQCS_STATUS_OVERRUN
    SRB_STATUS_PHASE_SEQUENCE_FAILURE,  // RQCS_BAD_MESSAGE
    SRB_STATUS_PHASE_SEQUENCE_FAILURE,  // RQCS_NO_MESSAGE_OUT
    SRB_STATUS_MESSAGE_REJECTED,        // RQCS_EXT_ID_FAILED
    SRB_STATUS_MESSAGE_REJECTED,        // RQCS_IDE_MSG_FAILED
    SRB_STATUS_MESSAGE_REJECTED,        // RQCS_ABORT_MSG_FAILED
    SRB_STATUS_MESSAGE_REJECTED,        // RQCS_REJECT_MSG_FAILED
    SRB_STATUS_MESSAGE_REJECTED,        // RQCS_NOP_MSG_FAILED
    SRB_STATUS_MESSAGE_REJECTED,        // RQCS_PARITY_ERROR_MSG_FAILED
    SRB_STATUS_MESSAGE_REJECTED,        // RQCS_DEVICE_RESET_MSG_FAILED
    SRB_STATUS_MESSAGE_REJECTED,        // RQCS_ID_MSG_FAILED
    SRB_STATUS_UNEXPECTED_BUS_FREE,     // RQCS_UNEXP_BUS_FREE
    SRB_STATUS_DATA_OVERRUN             // RQCS_DATA_UNDERRUN
};

/* FUNCTIONS ******************************************************************/

static
VOID
IspLogError(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ ULONG ErrorCode,
    _In_ ULONG UniqueId)
{
    StorPortLogError(HwExt, NULL, 0, 0, 0, ErrorCode, UniqueId);
}

_IRQL_requires_(HIGH_LEVEL)
static
VOID
IspOnBusReset(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ ULONG PathId)
{
    ASSERT(PathId < QL_MAX_BUSES);

    StorPortNotification(ResetDetected, HwExt, 0);

    HwExt->MarkerNeeded[PathId] = TRUE;
}

static
VOID
IspQueueRecovery(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ USHORT Status,
    _In_ UCHAR Flags)
{
    TRACE("Queue recovery 0x%X for status 0x%X\n", Flags, Status);

    if ((HwExt->InterruptFlags & Flags) == Flags)
        return;
    HwExt->InterruptFlags |= Flags;

    if (HwExt->InterruptFlags & ISP_INT_FLAG_NEED_RESET_ASIC)
        HwExt->InterruptFlags &= ~ISP_INT_FLAG_ADAPTER_ACTIVE;

    IspLogError(HwExt, SP_INTERNAL_ADAPTER_ERROR, Status);

    /* Block any new I/O requests */
    StorPortPause(HwExt, 60);

    StorPortIssueDpc(HwExt, &HwExt->RecoveryDpc, HwExt, NULL);
}

static
BOOLEAN
IspResetAsic(
    _In_ PISP_HW_EXTENSION HwExt)
{
    USHORT Mbox;
    ULONG i;

    INFO("Resetting ASIC\n");

    HwExt->InterruptFlags &= ~ISP_INT_FLAG_ADAPTER_ACTIVE;
    HwExt->RequestProd = 0;
    HwExt->RequestCons = 0;
    HwExt->ResponseCons = 0;

    /* Disable interrupts */
    QL_WRITE(HwExt, QL_REG_INT_CTRL, 0);

    /* Soft reset */
    QL_WRITE(HwExt, QL_REG_INT_CTRL, QL_IFACE_SOFT_RESET);
    QL_WAIT(100);

    /* Reset RISC processor */
    QL_WRITE(HwExt, QL_REG_HOST_CMD, QL_HC_RESET_RISC);
    QL_WAIT(10);
    QL_WRITE(HwExt, QL_REG_HOST_CMD, QL_HC_RELEASE_RISC);
    QL_WAIT(10);
    QL_WRITE(HwExt, QL_REG_HOST_CMD, QL_HC_DISABLE_BIOS);
    QL_WAIT(10);

    /* Wait for a second */
    for (i = 100000; i > 0; i--)
    {
        if ((QL_READ(HwExt, QL_REG_MAILBOX0) == 0) &&
            !(QL_READ(HwExt, QL_REG_INT_CTRL) & QL_IFACE_SOFT_RESET))
        {
            break;
        }

        QL_WAIT(10);
    }
    if (i == 0)
        return FALSE;

    /* Verify the ISP signature 'ISP   ' */
    if (QL_READ(HwExt, QL_REG_MAILBOX1) != 0x4953)
        return FALSE;
    Mbox = QL_READ(HwExt, QL_REG_MAILBOX2);
    if ((Mbox != 0x5020) && (Mbox != 0x0000))
        return FALSE;
    if (QL_READ(HwExt, QL_REG_MAILBOX3) != 0x2020)
        return FALSE;
    if (QL_READ(HwExt, QL_REG_MAILBOX4) != 1)
        return FALSE;

    QL_WRITE(HwExt, QL_REG_CFG1, 0);

    QL_WRITE(HwExt, QL_REG_SEMAPHORE, 0);
    QL_WRITE(HwExt, QL_REG_HOST_CMD, QL_HC_CLEAR_RISC_INT);
    return TRUE;
}

static
VOID
IspPutHeader(
    _Out_ PQL_IOCB_HEADER Header,
    _In_ UCHAR Type,
    _In_ UCHAR Count)
{
    Header->EntryType = Type;
    Header->EntryCount = Count;
    Header->SequenceNumber = 0;
    Header->Flags = 0;
}

static
VOID
IspMailboxPutAddress(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ ULONG64 PhysicalAddress)
{
    HwExt->Mailbox[3] = (PhysicalAddress >> 0) & 0xFFFF;
    HwExt->Mailbox[2] = (PhysicalAddress >> 16) & 0xFFFF;
    HwExt->Mailbox[7] = (PhysicalAddress >> 32) & 0xFFFF;
    HwExt->Mailbox[6] = (PhysicalAddress >> 48) & 0xFFFF;
}

_IRQL_requires_(HIGH_LEVEL)
static
BOOLEAN
IspMailboxSend(
    _In_ PISP_HW_EXTENSION HwExt)
{
    USHORT OpCode = HwExt->Mailbox[0];
    ULONG i, ParamBitmap;

    DBG_UNREFERENCED_LOCAL_VARIABLE(OpCode);

    TRACE("Send command 0x%X\n", OpCode);

    /* We must have at least the opcode and status regs for the command */
    ASSERT(HwExt->InBitmap & 0x01);
    ASSERT(HwExt->OutBitmap & 0x01);

    HwExt->InterruptFlags |= ISP_INT_FLAG_MAILBOX_ACTIVE;

    /* Load mailbox registers */
    ParamBitmap = HwExt->InBitmap;
    for (i = RTL_NUMBER_OF(HwExt->Mailbox); i--; )
    {
        if (ParamBitmap & (1 << i))
            QL_WRITE(HwExt, QL_REG_MAILBOX0 + i * 2, HwExt->Mailbox[i]);
    }
    QL_WRITE(HwExt, QL_REG_HOST_CMD, QL_HC_SET_HOST_INT);

    /* Wait up to 100 ms for the active mailbox command to be completed by firmware */
    for (i = 10000; i > 0; i--)
    {
        IspHwInterrupt(HwExt);
        if (!(HwExt->InterruptFlags & ISP_INT_FLAG_MAILBOX_ACTIVE))
            break;

        QL_WAIT(10);
    }
    if (i == 0)
    {
        ERR("Mailbox command 0x%X timed out\n", OpCode);

        HwExt->InterruptFlags &= ~ISP_INT_FLAG_MAILBOX_ACTIVE;
        return FALSE;
    }

    if (HwExt->Mailbox[0] != QL_MBOX_STATUS_COMMAND_COMPLETE)
    {
        ERR("Mailbox command 0x%X failed with status 0x%04X\n", OpCode, HwExt->Mailbox[0]);
        return FALSE;
    }

    return TRUE;
}

#ifndef ISP_FIRMWARE_DISABLE
static
BOOLEAN
IspDownloadFirmwareDma(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_reads_(CodeSize) const USHORT* Image,
    _In_ ULONG CodeSize)
{
    PUSHORT DmaBufferVa = (PUSHORT)HwExt->RequestQueueBase;
    ULONG i, Address = 0;

    while (CodeSize > 0)
    {
        USHORT BlockSize;

        /* Reuse storage allocated for the request and response queues */
        BlockSize = min(ISP_QUEUE_MEM_BLOCK_SIZE, QL_FW_DMA_BLOCK_SIZE);
        BlockSize /= sizeof(USHORT);
        BlockSize = min(BlockSize, CodeSize);

        for (i = 0; i < BlockSize; ++i)
        {
            DmaBufferVa[i] = *Image++;
        }

        if (HwExt->Flags & ISP_FLAG_64BIT_ADDRESS)
            MBX_INIT(HwExt, QL_MBOX_CMD_LOAD_RAM_A64);
        else
            MBX_INIT(HwExt, QL_MBOX_CMD_LOAD_RAM);
        HwExt->Mailbox[1] = QL_CODE_ORG + Address;
        HwExt->Mailbox[4] = BlockSize;
        IspMailboxPutAddress(HwExt, HwExt->RequestQueuePa);
        if (!IspMailboxSend(HwExt))
            return FALSE;

        Address += BlockSize;
        CodeSize -= BlockSize;
    }

    return TRUE;
}

static
BOOLEAN
IspDownloadFirmwarePio(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_reads_(CodeSize) const USHORT* Image,
    _In_ ULONG CodeSize)
{
    ULONG i;

    for (i = 0; i < CodeSize; ++i)
    {
        MBX_INIT(HwExt, QL_MBOX_CMD_WRITE_RAM_WORD);
        HwExt->Mailbox[1] = QL_CODE_ORG + i;
        HwExt->Mailbox[2] = Image[i];
        if (!IspMailboxSend(HwExt))
            return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
IspDownloadFirmware(
    _In_ PISP_HW_EXTENSION HwExt)
{
    const USHORT* Image;

    INFO("Downloading new firmware\n");

    switch (HwExt->IspFamily)
    {
        case QL_ISP_FAMILY_1040:
            Image = isp_1040_risc_code;
            break;

        case QL_ISP_FAMILY_1080:
            Image = isp_1080_risc_code;
            break;

        case QL_ISP_FAMILY_12160:
            Image = isp_12160_risc_code;
            break;

        DEFAULT_UNREACHABLE;
    }

    if (HwExt->IspFamily == QL_ISP_FAMILY_1040)
        return IspDownloadFirmwarePio(HwExt, Image, Image[3]);

    return IspDownloadFirmwareDma(HwExt, Image, Image[3]);
}
#endif

static
PSCSI_REQUEST_BLOCK
IspGetSrbFromHandle(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ ULONG Handle)
{
    PSCSI_REQUEST_BLOCK Srb;

    if (Handle >= RTL_NUMBER_OF(HwExt->ActiveSrb))
        return NULL;

    Srb = HwExt->ActiveSrb[Handle];
    if (!Srb)
        return NULL;

    HwExt->ActiveSrb[Handle] = NULL;

    return Srb;
}

static
VOID
IspHandleAsynchronousEvent(
    _In_ PISP_HW_EXTENSION HwExt)
{
    USHORT Status;

    Status = QL_READ(HwExt, QL_REG_MBOX_STATUS);
    TRACE("Async event %X\n", Status);
    switch (Status)
    {
        case QL_MBOX_ASYNC_BUS_RESET:
        case QL_MBOX_ASYNC_TIMEOUT_RESET:
        case QL_MBOX_ASYNC_DEVICE_RESET:
        case QL_MBOX_ASYNC_KILLED_BUS:
        {
            ULONG PathId = QL_READ(HwExt, QL_REG_MAILBOX6);

            if (Status == QL_MBOX_ASYNC_BUS_RESET)
                INFO("SCSI Bus %lu reset\n", PathId);
            else if (Status == QL_MBOX_ASYNC_TIMEOUT_RESET)
                ERR("Timeout initiated SCSI Bus %lu reset\n", PathId);
            else if (Status == QL_MBOX_ASYNC_DEVICE_RESET)
                INFO("Device reset on SCSI Bus %lu\n", PathId);
            else
                ERR("SCSI Bus %lu reset after DATA Overrun\n", PathId);

            IspOnBusReset(HwExt, PathId & 0x1);
            break;
        }

        case QL_MBOX_ASYNC_HUNG_SCSI:
        {
            ULONG PathId = QL_READ(HwExt, QL_REG_MAILBOX6);
            UCHAR Flags;

            if (PathId == 1)
                Flags = ISP_INT_FLAG_NEED_RESET_BUS2;
            else
                Flags = ISP_INT_FLAG_NEED_RESET_BUS1;
            ERR("Stalled SCSI Bus %lu after DATA Overrun\n", PathId);

            IspQueueRecovery(HwExt, Status, Flags);
            break;
        }

        case QL_MBOX_ASYNC_SYSTEM_ERROR:
        case QL_MBOX_ASYNC_RQS_XFER_ERR:
        case QL_MBOX_ASYNC_RSP_XFER_ERR:
        {
            if (Status == QL_MBOX_ASYNC_SYSTEM_ERROR)
                ERR("ISP has crashed\n");
            else if (Status == QL_MBOX_ASYNC_RQS_XFER_ERR)
                ERR("Request queue transfer error\n");
            else
                ERR("Response queue transfer error\n");

            IspQueueRecovery(HwExt, Status, ISP_INT_FLAG_NEED_RESET_ASIC);
            break;
        }

        case QL_MBOX_ASYNC_CMD_CMPLT:
        {
            PSCSI_REQUEST_BLOCK Srb;
            ULONG Handle;

            Handle = QL_READ(HwExt, QL_REG_MBOX_HNDL_LOW);
            Handle |= QL_READ(HwExt, QL_REG_MBOX_HNDL_HIGH) << 16;

            Srb = IspGetSrbFromHandle(HwExt, Handle);
            if (!Srb)
            {
                ERR("Invalid handle 0x%08lX\n", Handle);
                IspQueueRecovery(HwExt, Status, ISP_INT_FLAG_NEED_RESET_ASIC);
                break;
            }

            TRACE("Complete SRB %p\n", Srb);
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            StorPortNotification(RequestComplete, HwExt, Srb);
            break;
        }

        default:
        {
            ULONG i, ParamBitmap;

            /* Complete the active mailbox command */
            if (QL_MBOX_COMPLETED(Status))
            {
                if (HwExt->InterruptFlags & ISP_INT_FLAG_MAILBOX_ACTIVE)
                {
                    HwExt->InterruptFlags &= ~ISP_INT_FLAG_MAILBOX_ACTIVE;

                    /* Copy back output registers */
                    ParamBitmap = HwExt->OutBitmap;
                    for (i = RTL_NUMBER_OF(HwExt->Mailbox); i--; )
                    {
                        if (ParamBitmap & (1 << i))
                            HwExt->Mailbox[i] = QL_READ(HwExt, QL_REG_MAILBOX0 + i * 2);
                    }

                    TRACE("Complete mailbox command\n");
                    break;
                }

                ERR("Spurious mailbox interrupt 0x%X\n", Status);
                IspQueueRecovery(HwExt, Status, ISP_INT_FLAG_NEED_RESET_ASIC);
                break;
            }

            WARN("Unknown async event 0x%X\n", Status);
            break;
        }
    }
}

static
VOID
IspProcessResponseEntry(
    _In_ ISP_HW_EXTENSION* __restrict HwExt,
    _In_ QL_IOCB_STATUS* __restrict Response,
    _In_ SCSI_REQUEST_BLOCK* __restrict Srb)
{
    TRACE("Complete SRB %p\n", Srb);

    /* Normal SRB completion path */
    if ((Response->CompletionStatus == RQCS_COMPLETE) && (Response->ScsiStatus == SCSISTAT_GOOD))
    {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        return;
    }

    TRACE("RESP h 0x%lX scsi 0x%X comp 0x%X statefl 0x%X statfl 0x%X time %u sens %u resid %lu\n",
          Response->Handle,
          Response->ScsiStatus,
          Response->CompletionStatus,
          Response->StateFlags,
          Response->StatusFlags,
          Response->Time,
          Response->SenseLength,
          Response->ResidualLength);

    if ((Response->CompletionStatus == RQCS_RESET_OCCURRED) ||
        (Response->CompletionStatus == RQCS_ABORTED))
    {
        HwExt->MarkerNeeded[Srb->PathId] = TRUE;
    }

    if (Response->CompletionStatus == RQCS_QUEUE_FULL)
        Srb->ScsiStatus = SCSISTAT_BUSY;
    else
        Srb->ScsiStatus = Response->ScsiStatus;

    if (Response->CompletionStatus < RTL_NUMBER_OF(IspCompletionStatusToSrbStatus))
        Srb->SrbStatus = IspCompletionStatusToSrbStatus[Response->CompletionStatus];
    else
        Srb->SrbStatus = SRB_STATUS_ERROR;

    /* Residual underrun */
    if (Response->CompletionStatus == RQCS_DATA_UNDERRUN)
    {
        ASSERT(Srb->DataTransferLength >= Response->ResidualLength);
        Srb->DataTransferLength -= Response->ResidualLength;
    }

    /* Auto request sense */
    if ((Response->StateFlags & RQSF_GOT_SENSE) && (Srb->SenseInfoBufferLength != 0))
    {
        Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        Srb->SenseInfoBufferLength = min(Srb->SenseInfoBufferLength, Response->SenseLength);

        ASSERT(Response->SenseLength <= sizeof(Response->SenseData));
        StorPortCopyMemory(Srb->SenseInfoBuffer,
                           Response->SenseData,
                           Srb->SenseInfoBufferLength);
    }
}

static
VOID
IspProcessResponseQueue(
    _In_ PISP_HW_EXTENSION HwExt)
{
    ULONG ResponseProd, ResponseCons;

    ResponseProd = QL_READ(HwExt, QL_REG_MBOX_RESP);
    ResponseCons = HwExt->ResponseCons;

    while (ResponseCons != ResponseProd)
    {
        PQL_IOCB_STATUS Response;

        Response = ISP_QUEUE_ENTRY(HwExt->ResponseQueueBase, ResponseCons);
        ResponseCons = ISP_NEXT_QENTRY(ResponseCons, ISP_RESPONSE_QUEUE_SIZE);

        if (Response->Header.EntryType == RQSTYPE_RESPONSE)
        {
            PSCSI_REQUEST_BLOCK Srb;

            Srb = IspGetSrbFromHandle(HwExt, Response->Handle);
            if (!Srb)
            {
                ERR("Invalid handle 0x%08lX\n", Response->Handle);
                IspQueueRecovery(HwExt, 0xFEED, ISP_INT_FLAG_NEED_RESET_ASIC);
                break;
            }

            IspProcessResponseEntry(HwExt, Response, Srb);
            StorPortNotification(RequestComplete, HwExt, Srb);
        }
        else
        {
            ERR("Unknown response entry 0x%X\n", Response->Header.EntryType);
        }
    }

    if (ResponseCons != HwExt->ResponseCons)
    {
        QL_WRITE(HwExt, QL_REG_MBOX_RESP, ResponseCons);
        HwExt->ResponseCons = ResponseCons;
    }
}

static
BOOLEAN
NTAPI
IspHwInterrupt(
    _In_ PVOID DeviceExtension)
{
    PISP_HW_EXTENSION HwExt = DeviceExtension;
    USHORT InterruptStatus, Semaphore;

    InterruptStatus = QL_READ(HwExt, QL_REG_INT_STATUS);
    if (!(InterruptStatus & QL_IFACE_RISC_INTR))
        return FALSE;

    TRACE("Interrupt %X\n", InterruptStatus);

    Semaphore = QL_READ(HwExt, QL_REG_SEMAPHORE);
    QL_WRITE(HwExt, QL_REG_HOST_CMD, QL_HC_CLEAR_RISC_INT);

    /* Mailbox interrupt */
    if (Semaphore & QL_SEMAPHORE_LOCK)
    {
        IspHandleAsynchronousEvent(HwExt);

        /* Release mailbox registers */
        QL_WRITE(HwExt, QL_REG_SEMAPHORE, 0);
    }

    if (HwExt->InterruptFlags & ISP_INT_FLAG_ADAPTER_ACTIVE)
    {
        IspProcessResponseQueue(HwExt);
    }

    return TRUE;
}

static
BOOLEAN
IspRequestQueueHasRoom(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ ULONG EntryCount)
{
    ULONG FreeSlots;

    FreeSlots = ISP_QENTRY_FREE(HwExt->RequestProd, HwExt->RequestCons, ISP_REQUEST_QUEUE_SIZE);
    if (EntryCount <= FreeSlots)
        return TRUE;

    /* Query the hardware and check again */
    HwExt->RequestCons = QL_READ(HwExt, QL_REG_MBOX_RQST);

    FreeSlots = ISP_QENTRY_FREE(HwExt->RequestProd, HwExt->RequestCons, ISP_REQUEST_QUEUE_SIZE);
    return (EntryCount <= FreeSlots);
}

static
PVOID
IspGetAvailablePacket(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ ULONG EntryCount)
{
    PVOID Packet;

    /* Make sure we have space to put something on the request queue */
    if (!IspRequestQueueHasRoom(HwExt, EntryCount))
        return NULL;

    Packet = ISP_QUEUE_ENTRY(HwExt->RequestQueueBase, HwExt->RequestProd);
    ASSERT(Packet != NULL);
    __assume(Packet != NULL); // Removes a NULL check after being inlined

    return Packet;
}

static
BOOLEAN
IspSendMarker(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ UCHAR PathId)
{
    PQL_IOCB_MARKER Marker;

    Marker = IspGetAvailablePacket(HwExt, 1);
    if (!Marker)
        return FALSE;

    RtlZeroMemory(Marker, sizeof(*Marker));
    IspPutHeader(&Marker->Header, RQSTYPE_MARKER, 0);
    Marker->Target = PathId << 7;
    Marker->Modifier = QL_IOCB_MODIFIER_SYNC_ALL;

    HwExt->RequestProd = ISP_NEXT_QENTRY(HwExt->RequestProd, ISP_REQUEST_QUEUE_SIZE);
    QL_WRITE(HwExt, QL_REG_MBOX_RQST, HwExt->RequestProd);

    HwExt->MarkerNeeded[PathId] = FALSE;
    return TRUE;
}

static
ULONG
IspBuildContinuationPackets64(
    _In_ UCHAR* __restrict RequestQueueBase,
    _In_ STOR_SCATTER_GATHER_ELEMENT* __restrict SgElement,
    _In_ ULONG SegmentCount,
    _In_ ULONG RequestProd)
{
    while (SegmentCount > 0)
    {
        PQL_IOCB_CONT_64 Continuation;
        ULONG i, MaxSegments;

        Continuation = ISP_QUEUE_ENTRY(RequestQueueBase, RequestProd);
        RequestProd = ISP_NEXT_QENTRY(RequestProd, ISP_REQUEST_QUEUE_SIZE);

        IspPutHeader(&Continuation->Header, RQSTYPE_A64_CONT, 1);

        MaxSegments = min(SegmentCount, QL_IOCB_MAX_CONT_SEG_64);
        for (i = 0; i < MaxSegments; ++i)
        {
            ASSERT(SgElement->Length != 0);

            Continuation->DataSegment[i].AddressLow = SgElement->PhysicalAddress.LowPart;
            Continuation->DataSegment[i].AddressHigh = SgElement->PhysicalAddress.HighPart;
            Continuation->DataSegment[i].Length = SgElement->Length;

            SgElement++;
        }

        SegmentCount -= MaxSegments;
    }

    return RequestProd;
}

static
ULONG
IspBuildContinuationPackets32(
    _In_ UCHAR* __restrict RequestQueueBase,
    _In_ STOR_SCATTER_GATHER_ELEMENT* __restrict SgElement,
    _In_ ULONG SegmentCount,
    _In_ ULONG RequestProd)
{
    while (SegmentCount > 0)
    {
        PQL_IOCB_CONT_32 Continuation;
        ULONG i, MaxSegments;

        Continuation = ISP_QUEUE_ENTRY(RequestQueueBase, RequestProd);
        RequestProd = ISP_NEXT_QENTRY(RequestProd, ISP_REQUEST_QUEUE_SIZE);

        IspPutHeader(&Continuation->Header, RQSTYPE_DATASEG, 1);
        Continuation->Reserved = 0;

        MaxSegments = min(SegmentCount, QL_IOCB_MAX_CONT_SEG_32);
        for (i = 0; i < MaxSegments; ++i)
        {
            ASSERT(SgElement->Length != 0);
            ASSERT(SgElement->PhysicalAddress.HighPart == 0);

            Continuation->DataSegment[i].Address = SgElement->PhysicalAddress.LowPart;
            Continuation->DataSegment[i].Length = SgElement->Length;

            SgElement++;
        }

        SegmentCount -= MaxSegments;
    }

    return RequestProd;
}

static
VOID
IspLoadCommand(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PISP_SRB_EXTENSION SrbExt)
{
    PQL_IOCB_REQUEST Packet;
    ULONG RequestProd, SegmentCount;

    TRACE("Send SRB %p\n", Srb);

    /* Put in a synchronization marker if needed */
    if (HwExt->MarkerNeeded[Srb->PathId])
    {
        if (!IspSendMarker(HwExt, Srb->PathId))
            goto QueueFull;
    }

    Packet = IspGetAvailablePacket(HwExt, SrbExt->Packet.Header.EntryCount);
    if (!Packet)
        goto QueueFull;
    StorPortCopyMemory(Packet, &SrbExt->Packet, sizeof(*Packet));

    /* Assign a handle identifying the SRB */
    RequestProd = HwExt->RequestProd;
    Packet->Handle = RequestProd;
    ASSERT(HwExt->ActiveSrb[RequestProd] == NULL);
    HwExt->ActiveSrb[RequestProd] = Srb;

    RequestProd = ISP_NEXT_QENTRY(RequestProd, ISP_REQUEST_QUEUE_SIZE);

    /* Start building additional continuation segments as needed */
    SegmentCount = SrbExt->Packet.SegmentCount;
    if (HwExt->Flags & ISP_FLAG_64BIT_ADDRESS)
    {
        if (SegmentCount > QL_IOCB_MAX_REQ_SEG_64)
        {
            ASSERT(SrbExt->Sgl);

            RequestProd = IspBuildContinuationPackets64(HwExt->RequestQueueBase,
                                                        &SrbExt->Sgl->List[QL_IOCB_MAX_REQ_SEG_64],
                                                        SegmentCount - QL_IOCB_MAX_REQ_SEG_64,
                                                        RequestProd);
        }
    }
    else
    {
        if (SegmentCount > QL_IOCB_MAX_REQ_SEG_32)
        {
            ASSERT(SrbExt->Sgl);

            RequestProd = IspBuildContinuationPackets32(HwExt->RequestQueueBase,
                                                        &SrbExt->Sgl->List[QL_IOCB_MAX_REQ_SEG_32],
                                                        SegmentCount - QL_IOCB_MAX_REQ_SEG_32,
                                                        RequestProd);
        }
    }

    /* Start execution of the packet */
    QL_WRITE(HwExt, QL_REG_MBOX_RQST, RequestProd);
    HwExt->RequestProd = RequestProd;
    return;

QueueFull:
    /* Resume I/O after some requests have completed */
    TRACE("Request queue full\n");
    StorPortBusy(HwExt, min(4, ISP_REQUEST_QUEUE_SIZE));

    Srb->SrbStatus = SRB_STATUS_BUSY;
    StorPortNotification(RequestComplete, HwExt, Srb);
}

static
BOOLEAN
IspResetBus(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ UCHAR PathId)
{
    BOOLEAN Success;

    MBX_INIT(HwExt, QL_MBOX_CMD_BUS_RESET);
    HwExt->Mailbox[1] = HwExt->BusData[PathId].ResetDelay;
    HwExt->Mailbox[2] = PathId;
    Success = IspMailboxSend(HwExt);
    if (Success)
    {
        IspOnBusReset(HwExt, PathId);
    }
    else
    {
        /* Bus reset failed, release active I/O requests via a chip reset */
        IspQueueRecovery(HwExt, 0xFACE, ISP_INT_FLAG_NEED_RESET_ASIC);
    }

    return Success;
}

static
BOOLEAN
IspResetTarget(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    BOOLEAN Success;

    MBX_INIT(HwExt, QL_MBOX_CMD_ABORT_TARGET);
    HwExt->Mailbox[1] = HwExt->BusData[Srb->PathId].ResetDelay;
    HwExt->Mailbox[2] = (Srb->PathId << 15) | (Srb->TargetId << 8);
    Success = IspMailboxSend(HwExt);

    HwExt->MarkerNeeded[Srb->PathId] = TRUE;
    return Success;
}

static
BOOLEAN
IspResetLun(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    BOOLEAN Success;

    MBX_INIT(HwExt, QL_MBOX_CMD_ABORT_DEVICE);
    HwExt->Mailbox[1] = (Srb->PathId << 15) | (Srb->TargetId << 8) | Srb->Lun;
    Success = IspMailboxSend(HwExt);

    HwExt->MarkerNeeded[Srb->PathId] = TRUE;
    return Success;
}

static
BOOLEAN
NTAPI
IspHwStartIo(
    _In_ PVOID DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PISP_HW_EXTENSION HwExt = DeviceExtension;
    STOR_LOCK_HANDLE LockHandle = { 0 };

    switch (Srb->Function)
    {
        case SRB_FUNCTION_EXECUTE_SCSI:
        {
            StorPortAcquireSpinLock(HwExt, InterruptLock, NULL, &LockHandle);
            IspLoadCommand(HwExt, Srb, Srb->SrbExtension);
            StorPortReleaseSpinLock(HwExt, &LockHandle);
            return TRUE;
        }
        case SRB_FUNCTION_RESET_BUS:
        {
            StorPortAcquireSpinLock(HwExt, InterruptLock, NULL, &LockHandle);
            if (IspResetBus(HwExt, Srb->PathId))
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
            else
                Srb->SrbStatus = SRB_STATUS_ERROR;
            StorPortReleaseSpinLock(HwExt, &LockHandle);
            break;
        }
        case SRB_FUNCTION_RESET_DEVICE:
        {
            StorPortAcquireSpinLock(HwExt, InterruptLock, NULL, &LockHandle);
            if (IspResetTarget(HwExt, Srb))
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
            else
                Srb->SrbStatus = SRB_STATUS_ERROR;
            StorPortReleaseSpinLock(HwExt, &LockHandle);
            break;
        }
        case SRB_FUNCTION_RESET_LOGICAL_UNIT:
        {
            StorPortAcquireSpinLock(HwExt, InterruptLock, NULL, &LockHandle);
            if (IspResetLun(HwExt, Srb))
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
            else
                Srb->SrbStatus = SRB_STATUS_ERROR;
            StorPortReleaseSpinLock(HwExt, &LockHandle);
            break;
        }

        default:
            INFO("Unsupported function %x\n", Srb->Function);
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            break;
    }

    StorPortNotification(RequestComplete, HwExt, Srb);
    return TRUE;
}

static
VOID
IspTranslateSrbToIocb(
    _In_ ISP_HW_EXTENSION* __restrict HwExt,
    _In_ SCSI_REQUEST_BLOCK* __restrict Srb,
    _In_ STOR_SCATTER_GATHER_LIST* __restrict Sgl,
    _Out_ QL_IOCB_REQUEST* __restrict Packet)
{
    ULONG i, EntryCount, SegmentCount, MaxSegments;
    USHORT Flags;

    Flags = REQFLAG_NODATA;

    if (Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
        Flags |= REQFLAG_NODISCON;

    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN)
        Flags |= REQFLAG_DATA_IN;

    if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT)
        Flags |= REQFLAG_DATA_OUT;

    if (Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE)
        Flags |= REQFLAG_DISARQ;

    if (Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE)
    {
        if (Srb->QueueAction == SRB_SIMPLE_TAG_REQUEST)
            Flags |= REQFLAG_STAG;
        else if (Srb->QueueAction == SRB_HEAD_OF_QUEUE_TAG_REQUEST)
            Flags |= REQFLAG_HTAG;
        else // SRB_ORDERED_QUEUE_TAG_REQUEST
            Flags |= REQFLAG_OTAG;
    }

    Packet->Flags = Flags;
    Packet->Timeout = min(Srb->TimeOutValue, 0xFFFF);
    Packet->CdbLength = Srb->CdbLength;
    Packet->Lun = Srb->Lun;
    Packet->Target = Srb->TargetId | (Srb->PathId << 7);
    StorPortCopyMemory(Packet->Cdb, Srb->Cdb, sizeof(Packet->Cdb));

    SegmentCount = 0;

    /* Fill in the first S/G list */
    if (HwExt->Flags & ISP_FLAG_64BIT_ADDRESS)
    {
        IspPutHeader(&Packet->Header, RQSTYPE_REQUEST_A64, 1);

        if (Sgl)
        {
            SegmentCount = Sgl->NumberOfElements;

            ASSERT((SegmentCount > 0) && (SegmentCount <= QL_SG_LIST_MAX_SEG_64));

            if (SegmentCount > QL_IOCB_MAX_REQ_SEG_64)
            {
                EntryCount = 1;
                EntryCount += (SegmentCount - QL_IOCB_MAX_REQ_SEG_64) / QL_IOCB_MAX_CONT_SEG_64;
                if ((SegmentCount - QL_IOCB_MAX_REQ_SEG_64) % QL_IOCB_MAX_CONT_SEG_64)
                    EntryCount++;

                Packet->Header.EntryCount = EntryCount;
            }

            MaxSegments = min(SegmentCount, QL_IOCB_MAX_REQ_SEG_64);
            for (i = 0; i < MaxSegments; ++i)
            {
                ASSERT(Sgl->List[i].Length != 0);

                Packet->x64.DataSegment[i].AddressLow = Sgl->List[i].PhysicalAddress.LowPart;
                Packet->x64.DataSegment[i].AddressHigh = Sgl->List[i].PhysicalAddress.HighPart;
                Packet->x64.DataSegment[i].Length = Sgl->List[i].Length;
            }
        }
    }
    else
    {
        IspPutHeader(&Packet->Header, RQSTYPE_REQUEST, 1);

        if (Sgl)
        {
            SegmentCount = Sgl->NumberOfElements;

            ASSERT((SegmentCount > 0) && (SegmentCount <= QL_SG_LIST_MAX_SEG_32));

            if (SegmentCount > QL_IOCB_MAX_REQ_SEG_32)
            {
                EntryCount = 1;
                EntryCount += (SegmentCount - QL_IOCB_MAX_REQ_SEG_32) / QL_IOCB_MAX_CONT_SEG_32;
                if ((SegmentCount - QL_IOCB_MAX_REQ_SEG_32) % QL_IOCB_MAX_CONT_SEG_32)
                    EntryCount++;

                Packet->Header.EntryCount = EntryCount;
            }

            MaxSegments = min(SegmentCount, QL_IOCB_MAX_REQ_SEG_32);
            for (i = 0; i < MaxSegments; ++i)
            {
                ASSERT(Sgl->List[i].Length != 0);
                ASSERT(Sgl->List[i].PhysicalAddress.HighPart == 0);

                Packet->x32.DataSegment[i].Address = Sgl->List[i].PhysicalAddress.LowPart;
                Packet->x32.DataSegment[i].Length = Sgl->List[i].Length;
            }
        }
    }
    Packet->SegmentCount = SegmentCount;
}

static
BOOLEAN
NTAPI
IspHwBuildIo(
    _In_ PVOID DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PISP_HW_EXTENSION HwExt = DeviceExtension;

    if (Srb->PathId >= HwExt->ScsiBusCount || Srb->TargetId >= QL_MAX_TARGETS)
    {
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        StorPortNotification(RequestComplete, HwExt, Srb);
        return FALSE;
    }

    if (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI)
    {
        PISP_SRB_EXTENSION SrbExt = Srb->SrbExtension;

        SrbExt->Sgl = StorPortGetScatterGatherList(HwExt, Srb);

        IspTranslateSrbToIocb(HwExt, Srb, SrbExt->Sgl, &SrbExt->Packet);
    }

    return TRUE;
}

static
BOOLEAN
NTAPI
IspHwResetBus(
    _In_ PVOID DeviceExtension,
    _In_ ULONG PathId)
{
    PISP_HW_EXTENSION HwExt = DeviceExtension;
    STOR_LOCK_HANDLE LockHandle = { 0 };
    BOOLEAN Success;

    ASSERT(PathId < QL_MAX_BUSES);

    StorPortAcquireSpinLock(HwExt, InterruptLock, NULL, &LockHandle);
    Success = IspResetBus(HwExt, PathId);
    StorPortReleaseSpinLock(HwExt, &LockHandle);

    return Success;
}

static
BOOLEAN
IspInitializeAdapter(
    _In_ PISP_HW_EXTENSION HwExt)
{
    ULONG i, PathId, ResetDelay;

    HwExt->InterruptFlags &= ~ISP_INT_FLAG_ADAPTER_ACTIVE;

    /*
     * Up until this point we have done everything by just reading or
     * setting registers. From this point on we rely on at least *some*
     * kind of firmware running in the card.
     */

    /*
     * Do some sanity checking by running a NOP command.
     * Check if the ROM firmware is now running.
     */
    MBX_INIT(HwExt, QL_MBOX_CMD_NO_OP);
    if (!IspMailboxSend(HwExt))
    {
        ERR("NOP command failed\n");
        return FALSE;
    }

    /* Do some operational tests */
    MBX_INIT(HwExt, QL_MBOX_CMD_MAILBOX_REG_TEST);
    for (i = 0; i < RTL_NUMBER_OF(IspMailboxTestPattern); ++i)
    {
        HwExt->Mailbox[i + 1] = IspMailboxTestPattern[i];
    }
    if (!IspMailboxSend(HwExt))
    {
        ERR("Register test failed\n");
        return FALSE;
    }
    for (i = 0; i < RTL_NUMBER_OF(IspMailboxTestPattern); ++i)
    {
        if (HwExt->Mailbox[i + 1] != IspMailboxTestPattern[i])
        {
            ERR("Register test failed\n");
            return FALSE;
        }
    }

#ifndef ISP_FIRMWARE_DISABLE
    /* Download new firmware */
    if (!IspDownloadFirmware(HwExt))
    {
        ERR("Failed to download F/W\n");
        return FALSE;
    }

    /* Verify firmware checksum */
    MBX_INIT(HwExt, QL_MBOX_CMD_VERIFY_CHECKSUM);
    HwExt->Mailbox[1] = QL_CODE_ORG;
    if (!IspMailboxSend(HwExt))
    {
        ERR("Invalid F/W checksum\n");
        return FALSE;
    }
#endif

    /* Set SCSI termination */
    if (HwExt->IspFamily != QL_ISP_FAMILY_1040)
    {
        QL_WRITE(HwExt, QL_REG_GPIO_ENABLE, 0x0F);
        QL_WRITE(HwExt, QL_REG_GPIO_DATA, HwExt->TerminationControl);
    }

    /*
     * Now start it rolling.
     *
     * If we did not actually download f/w, we still need to (re)start it.
     */
    MBX_INIT(HwExt, QL_MBOX_CMD_EXEC_FIRMWARE);
    HwExt->Mailbox[1] = QL_CODE_ORG;
    if (!IspMailboxSend(HwExt))
    {
        ERR("Unable to exec F/W\n");
        return FALSE;
    }

    QL_WRITE(HwExt, QL_REG_CFG1, HwExt->IspConfig);

    MBX_INIT(HwExt, QL_MBOX_CMD_SET_PCI_PARAMETERS);
    HwExt->Mailbox[1] = (HwExt->Flags & ISP_FLAG_DDMA_BURST_ENABLE) ? 2 : 0;
    HwExt->Mailbox[2] = (HwExt->Flags & ISP_FLAG_CDMA_BURST_ENABLE) ? 2 : 0;
    if (!IspMailboxSend(HwExt))
    {
        ERR("Failed to set PCI parameters\n");
        return FALSE;
    }

    MBX_INIT(HwExt, QL_MBOX_CMD_SET_CLOCK_RATE);
    HwExt->Mailbox[1] = HwExt->IspClock;
    (VOID)IspMailboxSend(HwExt);
    /* Ignore failures */

    /*
     * Ask the chip for the current firmware version.
     * This should prove that the new firmware is working.
     */
    MBX_INIT(HwExt, QL_MBOX_CMD_ABOUT_FIRMWARE);
    if (!IspMailboxSend(HwExt))
    {
        ERR("Failed to get F/W version\n");
        return FALSE;
    }
    INFO("ISP F/W version %u.%u.%u\n", HwExt->Mailbox[1], HwExt->Mailbox[2], HwExt->Mailbox[3]);

    MBX_INIT(HwExt, QL_MBOX_CMD_SET_SYSTEM_PARAMETER);
    HwExt->Mailbox[1] = HwExt->IspParameter;
    if (!IspMailboxSend(HwExt))
    {
        ERR("Failed to set system parameter\n");
        return FALSE;
    }

    /* Turn on LVD transitions for ULTRA2 or better and other features */
    MBX_INIT(HwExt, QL_MBOX_CMD_SET_FW_FEATURES);
    HwExt->Mailbox[1] = HwExt->FwFeatures & QL_FW_FEATURE_FAST_POST;
    if ((HwExt->IspType >= QL_ISP_TYPE_1080) && (HwExt->FwFeatures & QL_FW_FEATURE_LVD_NOTIFY))
        HwExt->Mailbox[1] |= QL_FW_FEATURE_LVD_NOTIFY;
    if (!IspMailboxSend(HwExt))
    {
        ERR("Failed to set F/W features\n");
        return FALSE;
    }

    MBX_INIT(HwExt, QL_MBOX_CMD_SET_RETRY_COUNT);
    HwExt->Mailbox[1] = HwExt->BusData[0].RetryCount;
    HwExt->Mailbox[2] = HwExt->BusData[0].RetryDelay;
    HwExt->Mailbox[6] = HwExt->BusData[1].RetryCount;
    HwExt->Mailbox[7] = HwExt->BusData[1].RetryDelay;
    if (!IspMailboxSend(HwExt))
    {
        ERR("Failed to set retry count and retry delay\n");
        return FALSE;
    }

    MBX_INIT(HwExt, QL_MBOX_CMD_SET_ASYNC_DATA_SETUP_TIME);
    HwExt->Mailbox[1] = HwExt->BusData[0].AsyncDataSetupTime;
    HwExt->Mailbox[2] = HwExt->BusData[1].AsyncDataSetupTime;
    if (!IspMailboxSend(HwExt))
    {
        ERR("Failed to set async data setup time\n");
        return FALSE;
    }

    MBX_INIT(HwExt, QL_MBOX_CMD_SET_ACT_NEG_STATE);
    HwExt->Mailbox[1] = (HwExt->BusData[0].ReqAckActiveNegation << 4) |
                        (HwExt->BusData[0].DataLineActiveNegation << 5);
    HwExt->Mailbox[2] = (HwExt->BusData[1].ReqAckActiveNegation << 4) |
                        (HwExt->BusData[1].DataLineActiveNegation << 5);
    if (!IspMailboxSend(HwExt))
    {
        ERR("Failed to set active negation state\n");
        return FALSE;
    }

    MBX_INIT(HwExt, QL_MBOX_CMD_SET_DATA_OVERRUN_RECOVERY);
    HwExt->Mailbox[1] = 2;
    if (!IspMailboxSend(HwExt))
    {
        ERR("Failed to set data overrun recovery\n");
        return FALSE;
    }

    MBX_INIT(HwExt, QL_MBOX_CMD_SET_SELECT_TIMEOUT);
    HwExt->Mailbox[1] = HwExt->BusData[0].SelectionTimeout;
    HwExt->Mailbox[2] = HwExt->BusData[1].SelectionTimeout;
    if (!IspMailboxSend(HwExt))
    {
        ERR("Failed to set selection timeout\n");
        return FALSE;
    }

    MBX_INIT(HwExt, QL_MBOX_CMD_SET_TAG_AGE_LIMIT);
    HwExt->Mailbox[1] = HwExt->BusData[0].TagAgeLimit;
    HwExt->Mailbox[2] = HwExt->BusData[1].TagAgeLimit;
    if (!IspMailboxSend(HwExt))
    {
        ERR("Failed to set tag aging limit\n");
        return FALSE;
    }

    /* Set bus parameters */
    for (PathId = 0; PathId < HwExt->ScsiBusCount; ++PathId)
    {
        PISP_BUS_DATA BusData = &HwExt->BusData[PathId];
        ULONG TargetId;

        TRACE("Setup SCSI Bus %lu\n", PathId);

        MBX_INIT(HwExt, QL_MBOX_CMD_SET_INIT_SCSI_ID);
        HwExt->Mailbox[1] = (PathId << 7) | BusData->InitiatorId;
        if (!IspMailboxSend(HwExt))
        {
            ERR("Failed to set Initiator ID for bus %lu\n", PathId);
            return FALSE;
        }

        /* Set target parameters */
        for (TargetId = 0; TargetId < QL_MAX_TARGETS; ++TargetId)
        {
            PISP_TARGET_DATA TargetData = &HwExt->Target[PathId][TargetId];
            ULONG Lun;

            TRACE("Setup target %lu.%lu\n", PathId, TargetId);

            MBX_INIT(HwExt, QL_MBOX_CMD_SET_TARGET_PARAMS);
            HwExt->Mailbox[1] = (PathId << 15) | (TargetId << 8);
            HwExt->Mailbox[3] = (TargetData->SyncOffset << 8) | TargetData->SyncPeriod;
            HwExt->Mailbox[2] = TargetData->Parameters;

            /* Force auto request sense and tagged-queuing (required by Storport) */
            HwExt->Mailbox[2] |= DPARM_ARQ | DPARM_TQING;

            /* Disable "Freeze Queue on Error" */
            HwExt->Mailbox[2] &= ~DPARM_QFRZ;

            if (HwExt->IspFamily == QL_ISP_FAMILY_12160)
            {
                if (TargetData->PprOptions & 0x80)
                    HwExt->Mailbox[2] |= DPARM_PPR;

                HwExt->Mailbox[6] = ((TargetData->PprOptions & 0x0F) << 8) |
                                    ((TargetData->PprOptions & 0x30) >> 4);
            }
            else
            {
                HwExt->Mailbox[6] = 0;
            }

            if (!IspMailboxSend(HwExt))
            {
                ERR("Failed to set parameters for target %lu.%lu\n", PathId, TargetId);
                return FALSE;
            }

            /* Set lun parameters */
            for (Lun = 0; Lun < QL_MAX_LUN; ++Lun)
            {
                TRACE("Setup lun %lu.%lu.%lu\n", PathId, TargetId, Lun);

                MBX_INIT(HwExt, QL_MBOX_CMD_SET_DEV_QUEUE_PARAMS);
                HwExt->Mailbox[1] = (PathId << 15) | (TargetId << 8) | Lun;
                HwExt->Mailbox[2] = BusData->MaxQueueDepth;
                HwExt->Mailbox[3] = TargetData->ExecutionThrottle;
                if (!IspMailboxSend(HwExt))
                {
                    ERR("Failed to set parameters for lun %lu.%lu.%lu\n", PathId, TargetId, Lun);
                    return FALSE;
                }
            }
        }
    }

    /* Enable request and response queues */
    if (HwExt->Flags & ISP_FLAG_64BIT_ADDRESS)
        MBX_INIT(HwExt, QL_MBOX_CMD_INIT_REQ_QUEUE_A64);
    else
        MBX_INIT(HwExt, QL_MBOX_CMD_INIT_REQ_QUEUE);
    HwExt->Mailbox[1] = ISP_REQUEST_QUEUE_SIZE;
    HwExt->Mailbox[4] = 0;
    IspMailboxPutAddress(HwExt, HwExt->RequestQueuePa);
    if (!IspMailboxSend(HwExt))
        return FALSE;

    if (HwExt->Flags & ISP_FLAG_64BIT_ADDRESS)
        MBX_INIT(HwExt, QL_MBOX_CMD_INIT_RES_QUEUE_A64);
    else
        MBX_INIT(HwExt, QL_MBOX_CMD_INIT_RES_QUEUE);
    HwExt->Mailbox[1] = ISP_RESPONSE_QUEUE_SIZE;
    HwExt->Mailbox[5] = 0;
    IspMailboxPutAddress(HwExt, HwExt->ResponseQueuePa);
    if (!IspMailboxSend(HwExt))
        return FALSE;

    HwExt->InterruptFlags |= ISP_INT_FLAG_ADAPTER_ACTIVE;

    /* Perform an initial bus reset */
    ResetDelay = 0;
    for (PathId = 0; PathId < HwExt->ScsiBusCount; ++PathId)
    {
        TRACE("Reset SCSI Bus %lu, delay %u\n", PathId, HwExt->BusData[PathId].ResetDelay);

        if (HwExt->BusData[PathId].ResetDelay > ResetDelay)
            ResetDelay = HwExt->BusData[PathId].ResetDelay;

        MBX_INIT(HwExt, QL_MBOX_CMD_BUS_RESET);
        HwExt->Mailbox[1] = HwExt->BusData[PathId].ResetDelay;
        HwExt->Mailbox[2] = PathId;
        if (!IspMailboxSend(HwExt))
        {
            ERR("Failed to reset bus %lu\n", PathId);
            return FALSE;
        }

        HwExt->MarkerNeeded[PathId] = TRUE;
    }
    TRACE("Bus reset delay %lu\n", ResetDelay);

    /* Defer processing of I/O requests until all buses have settled */
    StorPortPause(HwExt, ResetDelay);

    /* Enable interrupts */
    QL_WRITE(HwExt, QL_REG_INT_CTRL, QL_IFACE_ISR_PEND | QL_IFACE_RISC_INTR);
    return TRUE;
}

static
VOID
NTAPI
IspRecoveryDpcRoutine(
    _In_ PSTOR_DPC Dpc,
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PISP_HW_EXTENSION HwExt = HwDeviceExtension;
    STOR_LOCK_HANDLE LockHandle = { 0 };
    ULONG i;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    TRACE("Enter DPC\n");

    StorPortAcquireSpinLock(HwExt, InterruptLock, NULL, &LockHandle);

    /* Resume I/O requests after the interrupt spinlock */
    StorPortResume(HwExt);

    if (HwExt->InterruptFlags & ISP_INT_FLAG_NEED_RESET_ASIC)
    {
        HwExt->InterruptFlags &= ~ISP_INT_FLAG_NEED_RESET_ASIC;

        /* Re-initialize the adapter */
        (VOID)IspHwInitialize(HwExt);

        /* Complete active commands */
        for (i = 0; i < RTL_NUMBER_OF(HwExt->ActiveSrb); ++i)
        {
            PSCSI_REQUEST_BLOCK Srb = HwExt->ActiveSrb[i];

            if (!Srb)
                continue;

            HwExt->ActiveSrb[i] = NULL;

            Srb->SrbStatus = SRB_STATUS_BUS_RESET;
            StorPortNotification(RequestComplete, HwExt, Srb);
        }
    }
    else
    {
        if (HwExt->InterruptFlags & ISP_INT_FLAG_NEED_RESET_BUS1)
            IspResetBus(HwExt, 0);

        if (HwExt->InterruptFlags & ISP_INT_FLAG_NEED_RESET_BUS2)
            IspResetBus(HwExt, 1);

        HwExt->InterruptFlags &= ~(ISP_INT_FLAG_NEED_RESET_BUS1 | ISP_INT_FLAG_NEED_RESET_BUS2);
    }

    StorPortReleaseSpinLock(HwExt, &LockHandle);
}

static
BOOLEAN
NTAPI
IspHwPassiveInitializeRoutine(
    _In_ PVOID DeviceExtension)
{
    PISP_HW_EXTENSION HwExt = DeviceExtension;

    StorPortInitializeDpc(HwExt, &HwExt->RecoveryDpc, IspRecoveryDpcRoutine);
    return TRUE;
}

static
BOOLEAN
NTAPI
IspHwInitialize(
    _In_ PVOID DeviceExtension)
{
    PISP_HW_EXTENSION HwExt = DeviceExtension;

    StorPortEnablePassiveInitialization(HwExt, IspHwPassiveInitializeRoutine);

    /* No need to reset the chip twice */
    if (HwExt->Flags & ISP_FLAG_IN_RESET)
    {
        HwExt->Flags &= ~ISP_FLAG_IN_RESET;
    }
    else
    {
        if (!IspResetAsic(HwExt))
        {
            ERR("Failed to reset ASIC\n");
            return FALSE;
        }
    }

    return IspInitializeAdapter(HwExt);
}

static
VOID
IspInitPciConfig(
    _In_ PISP_HW_EXTENSION HwExt)
{
    UCHAR Buffer[RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_CONFIG, u.type0.ROMBaseAddress)];
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)Buffer; // Partial PCI header

    StorPortGetBusData(HwExt,
                       PCIConfiguration,
                       HwExt->SystemIoBusNumber,
                       HwExt->SlotNumber,
                       Buffer,
                       sizeof(Buffer));

    INFO("Starting controller %04X:%04X.%02X-%04X.%04X\n",
         PciConfig->VendorID,
         PciConfig->DeviceID,
         PciConfig->RevisionID,
         PciConfig->u.type0.SubVendorID,
         PciConfig->u.type0.SubSystemID);

    /* Set the command register */
    PciConfig->Command |= PCI_ENABLE_WRITE_AND_INVALIDATE |
                          PCI_ENABLE_PARITY |
                          PCI_ENABLE_SERR;
    StorPortSetBusDataByOffset(HwExt,
                               PCIConfiguration,
                               HwExt->SystemIoBusNumber,
                               HwExt->SlotNumber,
                               Buffer,
                               FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                               sizeof(PciConfig->Command));

    /*
     * Make sure the ROM BAR is disabled.
     * Some ISP chips stop responding to MEM and I/O space once their ROM is enabled.
     */
    if (PciConfig->u.type0.ROMBaseAddress & PCI_ROMADDRESS_ENABLED)
    {
        PciConfig->u.type0.ROMBaseAddress &= ~PCI_ROMADDRESS_ENABLED;

        StorPortSetBusDataByOffset(HwExt,
                                   PCIConfiguration,
                                   HwExt->SystemIoBusNumber,
                                   HwExt->SlotNumber,
                                   Buffer,
                                   FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.ROMBaseAddress),
                                   sizeof(PciConfig->u.type0.ROMBaseAddress));
    }
}

static
SCSI_ADAPTER_CONTROL_STATUS
IspRestartAdapter(
    _In_ PISP_HW_EXTENSION HwExt)
{
    if (!IspHwInitialize(HwExt))
    {
        ERR("Failed to initialize hardware\n");
        return ScsiAdapterControlUnsuccessful;
    }

    return ScsiAdapterControlSuccess;
}

static
VOID
IspStopAdapter(
    _In_ PISP_HW_EXTENSION HwExt)
{
    /* Disable interrupts */
    QL_WRITE(HwExt, QL_REG_INT_CTRL, 0);

    /* Pause RISC */
    QL_WRITE(HwExt, QL_REG_HOST_CMD, QL_HC_PAUSE_RISC);
}

static
SCSI_ADAPTER_CONTROL_STATUS
NTAPI
IspHwAdapterControl(
    _In_ PVOID DeviceExtension,
    _In_ SCSI_ADAPTER_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters)
{
    PISP_HW_EXTENSION HwExt = DeviceExtension;
    SCSI_ADAPTER_CONTROL_STATUS Status = ScsiAdapterControlSuccess;

    switch (ControlType)
    {
        case ScsiQuerySupportedControlTypes:
        {
            PSCSI_SUPPORTED_CONTROL_TYPE_LIST ControlTypeList = Parameters;

#define SUPPORT_CONTROL_TYPE(Type) \
            if ((Type) < ControlTypeList->MaxControlType) { \
                ControlTypeList->SupportedTypeList[Type] = TRUE; \
            }
            SUPPORT_CONTROL_TYPE(ScsiStopAdapter);
            SUPPORT_CONTROL_TYPE(ScsiSetRunningConfig);
            SUPPORT_CONTROL_TYPE(ScsiRestartAdapter);
#undef SUPPORT_CONTROL_TYPE
            break;
        }
        case ScsiStopAdapter:
        {
            IspStopAdapter(HwExt);
            break;
        }
        case ScsiSetRunningConfig:
        {
            IspInitPciConfig(HwExt);
            break;
        }
        case ScsiRestartAdapter:
        {
            Status = IspRestartAdapter(HwExt);
            break;
        }

        default:
            Status = ScsiAdapterControlUnsuccessful;
            break;
    }

    return Status;
}

static
BOOLEAN
IspAllocateQueues(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo)
{
    PUCHAR MemBlockVa;
    ULONG64 MemBlockPa;
    STOR_PHYSICAL_ADDRESS PhysicalAddress;
    ULONG MappedLength;

    MemBlockVa = StorPortGetUncachedExtension(HwExt, ConfigInfo, ISP_QUEUE_MEM_BLOCK_SIZE);
    if (!MemBlockVa)
        return FALSE;

    PhysicalAddress = StorPortGetPhysicalAddress(HwExt, NULL, MemBlockVa, &MappedLength);
    MemBlockPa = PhysicalAddress.QuadPart;
    if (!MemBlockPa)
        return FALSE;

    HwExt->RequestQueueBase = MemBlockVa;
    HwExt->ResponseQueueBase = MemBlockVa + ISP_REQUEST_QUEUE_SIZE * QL_QENTRY_LEN;

    HwExt->RequestQueuePa = MemBlockPa;
    HwExt->ResponseQueuePa = MemBlockPa + ISP_REQUEST_QUEUE_SIZE * QL_QENTRY_LEN;

    return TRUE;
}

static
ULONG
IspRecognizeHardware(
    _In_ PISP_HW_EXTENSION HwExt)
{
    UCHAR Buffer[RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_CONFIG, DeviceID)];
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)Buffer; // Partial PCI header
    ULONG i, Bytes;
    static const struct
    {
        USHORT DeviceID;
        UCHAR IspFamily;
        UCHAR IspType;
        UCHAR ScsiBusCount;
        UCHAR IspClock;
    } IspControllerList[] =
    {
        { QL_PCI_DEV_ISP1040,  QL_ISP_FAMILY_1040,  255,               1, 255 },
        { QL_PCI_DEV_ISP1240,  QL_ISP_FAMILY_1080,  QL_ISP_TYPE_1240,  2, 60  },
        { QL_PCI_DEV_ISP1080,  QL_ISP_FAMILY_1080,  QL_ISP_TYPE_1080,  1, 100 },
        { QL_PCI_DEV_ISP1280,  QL_ISP_FAMILY_1080,  QL_ISP_TYPE_1280,  2, 100 },
        { QL_PCI_DEV_ISP10160, QL_ISP_FAMILY_12160, QL_ISP_TYPE_10160, 1, 100 },
        { QL_PCI_DEV_ISP12160, QL_ISP_FAMILY_12160, QL_ISP_TYPE_12160, 2, 100 }
    };

    Bytes = StorPortGetBusData(HwExt,
                               PCIConfiguration,
                               HwExt->SystemIoBusNumber,
                               HwExt->SlotNumber,
                               Buffer,
                               sizeof(Buffer));
    if (Bytes != sizeof(Buffer))
        return SP_RETURN_ERROR;

    if (PciConfig->VendorID != QL_PCI_VEN_QLOGIC)
        return SP_RETURN_NOT_FOUND;

    for (i = 0; i < RTL_NUMBER_OF(IspControllerList); ++i)
    {
        if (PciConfig->DeviceID != IspControllerList[i].DeviceID)
            continue;

        HwExt->IspFamily = IspControllerList[i].IspFamily;
        HwExt->IspType = IspControllerList[i].IspType;
        HwExt->ScsiBusCount = IspControllerList[i].ScsiBusCount;
        HwExt->IspClock = IspControllerList[i].IspClock;
        break;
    }
    if (i == RTL_NUMBER_OF(IspControllerList))
        return SP_RETURN_NOT_FOUND;

    IspInitPciConfig(HwExt);

    INFO("Bus ID [%04X:%04X], config [%04X:%04X]\n",
         QL_READ(HwExt, QL_REG_ID_LOW),
         QL_READ(HwExt, QL_REG_ID_HIGH),
         QL_READ(HwExt, QL_REG_CFG0),
         QL_READ(HwExt, QL_REG_CFG1));

    if (PciConfig->DeviceID == QL_PCI_DEV_ISP1040)
    {
        USHORT BusCfg0, BusCfg1, Psr;

        BusCfg0 = QL_READ(HwExt, QL_REG_CFG0);
        switch (BusCfg0 & BIU_CONF0_HW_MASK)
        {
            default:
                WARN("Unknown chip type 0x%X\n", BusCfg0);
                __fallthrough;
            case BIU_CHIP_TYPE_1020:
                HwExt->IspType = QL_ISP_TYPE_1020;
                HwExt->IspClock = 40;
                break;
            case BIU_CHIP_TYPE_1020A:
                /*
                 * Some 1020A chips are Ultra Capable, but don't
                 * run the clock rate up for that unless told to
                 * do so by the Ultra Capable bits being set.
                 */
                HwExt->IspType = QL_ISP_TYPE_1020A;
                HwExt->IspClock = 40;
                break;
            case BIU_CHIP_TYPE_1040:
                HwExt->IspType = QL_ISP_TYPE_1040;
                HwExt->IspClock = 60;
                break;
            case BIU_CHIP_TYPE_1040A:
                HwExt->IspType = QL_ISP_TYPE_1040A;
                HwExt->IspClock = 60;
                break;
            case BIU_CHIP_TYPE_1040B:
                HwExt->IspType = QL_ISP_TYPE_1040B;
                HwExt->IspClock = 60;
                break;
            case BIU_CHIP_TYPE_1040C:
                HwExt->IspType = QL_ISP_TYPE_1040C;
                HwExt->IspClock = 60;
                break;
        }

        QL_WRITE(HwExt, QL_REG_HOST_CMD, QL_HC_PAUSE_RISC);

        BusCfg1 = QL_READ(HwExt, QL_REG_CFG1);
        if (BusCfg1 & BIU_PCI_CONF1_SXP)
            QL_WRITE(HwExt, QL_REG_CFG1, BusCfg1 & ~BIU_PCI_CONF1_SXP);
        Psr = QL_READ(HwExt, QL_REG_RISC_PSR);
        if (BusCfg1 & BIU_PCI_CONF1_SXP)
            QL_WRITE(HwExt, QL_REG_CFG1, BusCfg1);

        QL_WRITE(HwExt, QL_REG_HOST_CMD, QL_HC_RELEASE_RISC);

        INFO("PSR 0x%X\n", Psr);

        if (Psr & QL_RISC_PSR_PCI_ULTRA)
        {
            INFO("Ultra Mode Capable\n");
            HwExt->Flags |= ISP_FLAG_1040_ULTRAMODE;
            HwExt->IspClock = 60;
        }
    }

    return SP_RETURN_FOUND;
}

static
ULONG
NTAPI
IspHwFindAdapter(
    _In_ PVOID DeviceExtension,
    _In_ PVOID HwContext,
    _In_ PVOID BusInformation,
    _In_z_ PCHAR ArgumentString,
    _Inout_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    _In_ PBOOLEAN Reserved3)
{
    PISP_HW_EXTENSION HwExt = DeviceExtension;
    PACCESS_RANGE AccessRange;
    ULONG i, Status, MaxSegments;

    UNREFERENCED_PARAMETER(HwContext);
    UNREFERENCED_PARAMETER(BusInformation);
    UNREFERENCED_PARAMETER(Reserved3);

    /* Retrieve the MMIO BAR */
    AccessRange = &((*(ConfigInfo->AccessRanges))[1]);
    if (!AccessRange || !AccessRange->RangeInMemory)
        return SP_RETURN_NOT_FOUND;

    HwExt->IoBase = StorPortGetDeviceBase(HwExt,
                                          ConfigInfo->AdapterInterfaceType,
                                          ConfigInfo->SystemIoBusNumber,
                                          AccessRange->RangeStart,
                                          AccessRange->RangeLength,
                                          FALSE);
    if (!HwExt->IoBase)
        return SP_RETURN_NOT_FOUND;

    HwExt->SystemIoBusNumber = ConfigInfo->SystemIoBusNumber;
    HwExt->SlotNumber = ConfigInfo->SlotNumber;

    Status = IspRecognizeHardware(HwExt);
    if (Status != SP_RETURN_FOUND)
    {
        ERR("Failed to recognize hardware\n");
        return Status;
    }

    if (!IspResetAsic(HwExt))
    {
        ERR("Failed to reset ASIC\n");
        return SP_RETURN_ERROR;
    }
    HwExt->Flags |= ISP_FLAG_IN_RESET;

    IspReadEeprom(HwExt);

    /* Busted FIFO on the 1040A. Turn off all but burst enables */
    if (HwExt->IspType == QL_ISP_TYPE_1040A)
    {
        HwExt->IspConfig &= ~BIU_BURST_ENABLE;
        HwExt->Flags &= ~(ISP_FLAG_DDMA_BURST_ENABLE | ISP_FLAG_CDMA_BURST_ENABLE);
    }

    if (HwExt->IspType <= QL_ISP_TYPE_1020A)
        ConfigInfo->MaximumTransferLength = 0x00FFFFFF;
    else
        ConfigInfo->MaximumTransferLength = 0x3FFFFFFF;

    ConfigInfo->AlignmentMask = 0;
    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->ResetTargetSupported = TRUE;
    ConfigInfo->SynchronizationModel = StorSynchronizeFullDuplex;
    ConfigInfo->NumberOfBuses = HwExt->ScsiBusCount;
    ConfigInfo->MaximumNumberOfTargets = QL_MAX_TARGETS;
    ConfigInfo->MaximumNumberOfLogicalUnits = QL_MAX_LUN;

    for (i = 0; i < HwExt->ScsiBusCount; ++i)
    {
        if (ConfigInfo->InitiatorBusId[i] == SP_UNINITIALIZED_VALUE)
            ConfigInfo->InitiatorBusId[i] = HwExt->BusData[i].InitiatorId;
        else
            HwExt->BusData[i].InitiatorId = ConfigInfo->InitiatorBusId[i] & 0x0F;
    }

    /*
     * We use 32-bit queue entries if possible,
     * because every 32-bit queue entry can hold up to 4 S/G elements,
     * and every 64-bit queue entry can hold up to 2 S/G elements.
     * This way it would require less queue entries for the setup of S/G elements
     * which will reduce PCI bus traffic on 32-bit non-PAE systems.
     */
    if (ConfigInfo->Dma64BitAddresses == SCSI_DMA64_SYSTEM_SUPPORTED)
    {
        ConfigInfo->Dma64BitAddresses = SCSI_DMA64_MINIPORT_SUPPORTED;

        HwExt->Flags |= ISP_FLAG_64BIT_ADDRESS;

        MaxSegments = QL_IOCB_MAX_REQ_SEG_64;
        MaxSegments += (ISP_REQUEST_QUEUE_SIZE - 1) * QL_IOCB_MAX_CONT_SEG_64;
        MaxSegments = min(MaxSegments, QL_SG_LIST_MAX_SEG_64);
    }
    else
    {
        MaxSegments = QL_IOCB_MAX_REQ_SEG_32;
        MaxSegments += (ISP_REQUEST_QUEUE_SIZE - 1) * QL_IOCB_MAX_CONT_SEG_32;
        MaxSegments = min(MaxSegments, QL_SG_LIST_MAX_SEG_32);
    }
    ConfigInfo->NumberOfPhysicalBreaks = MaxSegments;

    if (!IspAllocateQueues(HwExt, ConfigInfo))
    {
        ERR("No memory\n");
        return SP_RETURN_ERROR;
    }

    return SP_RETURN_FOUND;
}

ULONG
NTAPI
DriverEntry(
    _In_ PVOID DriverObject,
    _In_ PVOID RegistryPath)
{
    HW_INITIALIZATION_DATA HwInitializationData = { 0 };

    HwInitializationData.HwInitializationDataSize = sizeof(HwInitializationData);

    HwInitializationData.HwInitialize = IspHwInitialize;
    HwInitializationData.HwStartIo = IspHwStartIo;
    HwInitializationData.HwInterrupt = IspHwInterrupt;
    HwInitializationData.HwFindAdapter = IspHwFindAdapter;
    HwInitializationData.HwResetBus = IspHwResetBus;
    HwInitializationData.HwAdapterControl = IspHwAdapterControl;
    HwInitializationData.HwBuildIo = IspHwBuildIo;

    HwInitializationData.AdapterInterfaceType = PCIBus;
    HwInitializationData.NumberOfAccessRanges = 2; // I/O BAR and MMIO BAR

    HwInitializationData.AutoRequestSense = TRUE;
    HwInitializationData.NeedPhysicalAddresses = TRUE;
    HwInitializationData.MapBuffers = STOR_MAP_NO_BUFFERS;
    HwInitializationData.TaggedQueuing = TRUE;
    HwInitializationData.MultipleRequestPerLu = TRUE;

    HwInitializationData.DeviceExtensionSize = sizeof(ISP_HW_EXTENSION);
    HwInitializationData.SrbExtensionSize = sizeof(ISP_SRB_EXTENSION);

    return StorPortInitialize(DriverObject, RegistryPath, &HwInitializationData, NULL);
}
