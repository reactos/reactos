/*
 * PROJECT:     QLogic ISP SCSI Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     EEPROM support code
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "qla1xxx.h"

/* GLOBALS ********************************************************************/

#define NVRAM_READ(HwExt, Data)  \
    do { \
        *Data = QL_READ((HwExt), QL_REG_NVRAM); \
        QL_WAIT(10); \
    } while (0)

#define NVRAM_WRITE(HwExt, Value)  \
    do { \
        QL_WRITE((HwExt), QL_REG_NVRAM, Value); \
        QL_WAIT(10); \
    } while (0)

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
IspNvramChecksumValid(
    _In_reads_bytes_(Size) const VOID* Buffer,
    _In_ ULONG Size)
{
    const UCHAR* Data = Buffer;
    ULONG i;
    UCHAR Crc = 0;

    for (i = 0; i < Size; ++i)
    {
        Crc += Data[i];
    }

    return (Crc == 0);
}

static
VOID
IspNvramShiftOut(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ ULONG Sequence,
    _In_ ULONG BitCount)
{
    LONG i;

    for (i = BitCount; i >= 0; i--)
    {
        USHORT DataOut = ((Sequence >> i) & 1) << QL_NVRAM_DATAOUT_SHIFT;

        NVRAM_WRITE(HwExt, DataOut | QL_NVRAM_SELECT);
        NVRAM_WRITE(HwExt, DataOut | QL_NVRAM_SELECT | QL_NVRAM_CLOCK);
        NVRAM_WRITE(HwExt, DataOut | QL_NVRAM_SELECT);
    }
}

static
USHORT
IspNvramShiftIn(
    _In_ PISP_HW_EXTENSION HwExt)
{
    USHORT SerialData, DataIn;
    ULONG i;

    /* Shift the data out of the EEPROM */
    SerialData = 0;
    for (i = 0; i < RTL_BITS_OF(USHORT); ++i)
    {
        NVRAM_WRITE(HwExt, QL_NVRAM_SELECT | QL_NVRAM_CLOCK);

        NVRAM_READ(HwExt, &DataIn);
        SerialData = (SerialData << 1) | ((DataIn >> QL_NVRAM_DATAIN_SHIFT) & 1);

        NVRAM_WRITE(HwExt, QL_NVRAM_SELECT);
    }

    /* End the read cycle */
    NVRAM_WRITE(HwExt, 0);

    return SerialData;
}

static
BOOLEAN
IspNvramRead(
    _In_ PISP_HW_EXTENSION HwExt)
{
    ULONG Address, NvramWords, Command, AddressBusWidth, Bits;

    if (HwExt->IspFamily == QL_ISP_FAMILY_1040)
    {
        NvramWords = 64;
        AddressBusWidth = 6;
    }
    else
    {
        NvramWords = 128;
        AddressBusWidth = 8;
    }
    Command = QL_NVRAM_CMD_READ << AddressBusWidth;
    Bits = QL_NVRAM_CMD_LENGTH + AddressBusWidth - 1;

    NVRAM_WRITE(HwExt, QL_NVRAM_SELECT);
    NVRAM_WRITE(HwExt, QL_NVRAM_SELECT | QL_NVRAM_CLOCK);

    /* Read the NVRAM contents once */
    for (Address = 0; Address < NvramWords; ++Address)
    {
        IspNvramShiftOut(HwExt, Command | Address, Bits);

        HwExt->Nvram[Address] = IspNvramShiftIn(HwExt);

        /* Verify the ID header */
        if (Address == 4)
        {
            PQL_NVRAM_HEADER Header = (PQL_NVRAM_HEADER)HwExt->Nvram;

            if ((Header->Id[0] != 'I') ||
                (Header->Id[1] != 'S') ||
                (Header->Id[2] != 'P') ||
                (Header->Id[3] != ' ') ||
                (Header->Version < ((HwExt->IspFamily == QL_ISP_FAMILY_1040) ? 2 : 1)))
            {
                ERR("Bad NVRAM header %02X.%02X.%02X.%02X.%02X\n",
                    Header->Id[0],
                    Header->Id[1],
                    Header->Id[2],
                    Header->Id[3],
                    Header->Version);
                return FALSE;
            }
        }
    }

    /* Validate the NVRAM checksum */
    if (!IspNvramChecksumValid(HwExt->Nvram, NvramWords * 2))
    {
        ERR("NVRAM checksum failed\n");
        return FALSE;
    }

    return TRUE;
}

static
VOID
IspNvramParse1040(
    _In_ PISP_HW_EXTENSION HwExt)
{
    PQL_NVRAM_ISP1040 Nvram = (PQL_NVRAM_ISP1040)HwExt->Nvram;
    PISP_BUS_DATA BusData = &HwExt->BusData[0];
    ULONG TargetId;

    HwExt->TerminationControl = Nvram->Flags & 0x03;
    HwExt->IspParameter = Nvram->IspParameter;
    HwExt->FwFeatures = Nvram->FwFeatures;

    HwExt->IspConfig = (Nvram->IspConfig1 & 0x03) << 4;
    if (Nvram->IspConfig2 & 0xC0)
        HwExt->IspConfig |= BIU_BURST_ENABLE;
    if (Nvram->Flags & 0x40)
        HwExt->IspConfig |= BIU_PCI_CONF1_FIFO_128;

    if (Nvram->IspConfig2 & 0x40)
        HwExt->Flags |= ISP_FLAG_DDMA_BURST_ENABLE;
    if (Nvram->IspConfig2 & 0x80)
        HwExt->Flags |= ISP_FLAG_CDMA_BURST_ENABLE;

    BusData->InitiatorId = Nvram->IspConfig1 >> 4;
    BusData->ResetDelay = Nvram->ResetDelay;
    BusData->RetryCount = Nvram->RetryCount;
    BusData->RetryDelay = Nvram->RetryDelay;
    BusData->AsyncDataSetupTime = Nvram->IspConfig2 & 0x0F;
    BusData->ReqAckActiveNegation = (Nvram->IspConfig2 & 0x10) >> 4;
    BusData->DataLineActiveNegation = (Nvram->IspConfig2 & 0x20) >> 5;
    BusData->TagAgeLimit = Nvram->TagAgeLimit;
    BusData->SelectionTimeout = Nvram->SelectionTimeout;
    BusData->MaxQueueDepth = Nvram->MaxQueueDepth;

    for (TargetId = 0; TargetId < QL_MAX_TARGETS; ++TargetId)
    {
        PQL_NVRAM_TARGET_DATA NvramTargetData = &Nvram->Target[TargetId];
        PISP_TARGET_DATA TargetData = &HwExt->Target[0][TargetId];

        TargetData->Parameters = NvramTargetData->Parameters << 8;
        TargetData->ExecutionThrottle = NvramTargetData->ExecutionThrottle;
        TargetData->SyncPeriod = NvramTargetData->SyncPeriod;
        TargetData->SyncOffset = NvramTargetData->Flags & 0x0F;
    }
}

static
VOID
IspNvramParse1080(
    _In_ PISP_HW_EXTENSION HwExt)
{
    PQL_NVRAM_ISP1080 Nvram = (PQL_NVRAM_ISP1080)HwExt->Nvram;
    ULONG PathId, TargetId;

    HwExt->TerminationControl = Nvram->TerminationControl & 0x0F;
    HwExt->IspParameter = Nvram->IspParameter;
    HwExt->FwFeatures = Nvram->FwFeatures;

    HwExt->IspConfig = Nvram->IspConfig;
    if (HwExt->IspConfig & BIU_BURST_ENABLE)
        HwExt->Flags |= ISP_FLAG_DDMA_BURST_ENABLE | ISP_FLAG_CDMA_BURST_ENABLE;

    for (PathId = 0; PathId < HwExt->ScsiBusCount; ++PathId)
    {
        PQL_NVRAM_BUS_DATA NvramBusData = &Nvram->Bus[PathId];
        PISP_BUS_DATA BusData = &HwExt->BusData[PathId];

        BusData->InitiatorId = NvramBusData->Config1 & 0x0F;
        BusData->ResetDelay = NvramBusData->ResetDelay;
        BusData->RetryCount = NvramBusData->RetryCount;
        BusData->RetryDelay = NvramBusData->RetryDelay;
        BusData->AsyncDataSetupTime = NvramBusData->Config2 & 0x0F;
        BusData->ReqAckActiveNegation = (NvramBusData->Config2 & 0x10) >> 4;
        BusData->DataLineActiveNegation = (NvramBusData->Config2 & 0x20) >> 5;
        BusData->TagAgeLimit = 8;
        BusData->SelectionTimeout = NvramBusData->SelectionTimeout;
        BusData->MaxQueueDepth = NvramBusData->MaxQueueDepth;

        for (TargetId = 0; TargetId < QL_MAX_TARGETS; ++TargetId)
        {
            PQL_NVRAM_TARGET_DATA NvramTargetData = &NvramBusData->Target[TargetId];
            PISP_TARGET_DATA TargetData = &HwExt->Target[PathId][TargetId];

            TargetData->Parameters = NvramTargetData->Parameters << 8;
            TargetData->ExecutionThrottle = NvramTargetData->ExecutionThrottle;
            TargetData->SyncPeriod = NvramTargetData->SyncPeriod;
            if (HwExt->IspFamily == QL_ISP_FAMILY_12160)
            {
                TargetData->SyncOffset = NvramTargetData->Flags & 0x1F;
                TargetData->PprOptions = NvramTargetData->PprOptions;
            }
            else
            {
                TargetData->SyncOffset = NvramTargetData->Flags & 0x0F;
            }
        }
    }
}

static
VOID
IspNvramSetDefaultValues(
    _In_ PISP_HW_EXTENSION HwExt)
{
    ULONG PathId, TargetId;
    UCHAR SyncPeriod, SyncOffset;

    if (HwExt->IspType == QL_ISP_TYPE_1240)
    {
        SyncPeriod = 12;
        SyncOffset = 12;
    }
    else if (HwExt->IspFamily == QL_ISP_FAMILY_12160)
    {
        SyncPeriod = 9;
        SyncOffset = 12;
    }
    else if (HwExt->IspFamily == QL_ISP_FAMILY_1080)
    {
        SyncPeriod = 10;
        SyncOffset = 12;
    }
    else if ((HwExt->IspType < QL_ISP_TYPE_1040) ||
             (HwExt->IspClock < 60) ||
             (!(HwExt->Flags & ISP_FLAG_1040_ULTRAMODE)))
    {
        SyncPeriod = 25;
        SyncOffset = 12;
    }
    else
    {
        SyncPeriod = 12;
        SyncOffset = 8;
    }

    HwExt->TerminationControl = 0x0F;
    HwExt->IspParameter = 0;
    HwExt->FwFeatures = QL_FW_FEATURE_FAST_POST;

    if (HwExt->IspFamily == QL_ISP_FAMILY_1040)
        HwExt->IspConfig = BIU_PCI_CONF1_FIFO_64 | BIU_BURST_ENABLE;
    else
        HwExt->IspConfig = BIU_PCI_CONF1_FIFO_128 | BIU_BURST_ENABLE;
    HwExt->Flags |= ISP_FLAG_DDMA_BURST_ENABLE | ISP_FLAG_CDMA_BURST_ENABLE;

    for (PathId = 0; PathId < HwExt->ScsiBusCount; ++PathId)
    {
        PISP_BUS_DATA BusData = &HwExt->BusData[PathId];

        BusData->InitiatorId = 7;
        BusData->ResetDelay = 5;
        BusData->RetryCount = 0;
        BusData->RetryDelay = 0;
        BusData->AsyncDataSetupTime = (HwExt->IspType >= QL_ISP_TYPE_1040) ? 9 : 6;
        BusData->ReqAckActiveNegation = 1;
        BusData->DataLineActiveNegation = 1;
        BusData->TagAgeLimit = 8;
        BusData->SelectionTimeout = 250;
        BusData->MaxQueueDepth = 32;

        for (TargetId = 0; TargetId < QL_MAX_TARGETS; ++TargetId)
        {
            PISP_TARGET_DATA TargetData = &HwExt->Target[PathId][TargetId];

            TargetData->Parameters = DPARM_DEFAULT;
            TargetData->ExecutionThrottle = 16;
            TargetData->SyncPeriod = SyncPeriod;
            TargetData->SyncOffset = SyncOffset;
        }
    }
}

VOID
IspReadEeprom(
    _In_ PISP_HW_EXTENSION HwExt)
{
    BOOLEAN Success;

    Success = IspNvramRead(HwExt);
    if (!Success)
    {
        ERR("Failed to read NVRAM, using default parameters\n");
        IspNvramSetDefaultValues(HwExt);
        return;
    }

    /*
     * Set default values for bus 2 in case we only have a single bus adapter.
     * Not sure we even need this, but some mailbox initialization commands
     * set both channels at the same time.
     */
    if (HwExt->ScsiBusCount < 2)
    {
        PISP_BUS_DATA BusData = &HwExt->BusData[1];

        BusData->RetryCount = 0;
        BusData->RetryDelay = 0;
        BusData->AsyncDataSetupTime = (HwExt->IspType >= QL_ISP_TYPE_1040) ? 9 : 6;
        BusData->ReqAckActiveNegation = 1;
        BusData->DataLineActiveNegation = 1;
        BusData->SelectionTimeout = 250;
    }

    if (HwExt->IspFamily == QL_ISP_FAMILY_1040)
        IspNvramParse1040(HwExt);
    else
        IspNvramParse1080(HwExt);
}
