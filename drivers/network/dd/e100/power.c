/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Power management
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
EeDownloadFlexiblePacketFilters(
    _In_ PE100_ADAPTER Adapter)
{
    PFXP_CB_LOAD_FILTER FilterSetup = &Adapter->ControlBlock->FilterSetup;
    PULONG FilterData = &FilterSetup->Data[0];
    PULONG LastFilterHeader;
    PLIST_ENTRY Entry;
    ULONG i;

    PAGED_CODE();

    /* Build the list of programmable filters */
    for (Entry = Adapter->WakeUpFrameList.Flink;
         Entry != &Adapter->WakeUpFrameList;
         Entry = Entry->Flink)
    {
        PE100_WAKE_UP_FRAME WakeFrame = CONTAINING_RECORD(Entry, E100_WAKE_UP_FRAME, ListEntry);
        ULONG MaskLength;

        LastFilterHeader = FilterData;

        /* Store the type dword */
        ASSERT(WakeFrame->FilterSize > 0 && WakeFrame->FilterSize <= FXP_WOL_FILTER_MASKS);
        switch (WakeFrame->FilterSize)
        {
            default:
                MaskLength = 0;
                break;
            case 2:
                MaskLength = 1;
                break;
            case 3:
                MaskLength = 3;
                break;
            case 4:
                MaskLength = 7;
                break;
        }
        *FilterData++ = htole32((MaskLength << FXP_FILTER_MLEN_SHIFT) | WakeFrame->Signature);

        /* Store the mask dwords */
        for (i = 0; i < WakeFrame->FilterSize; ++i)
        {
            *FilterData++ = htole32(WakeFrame->PatternMask[i]);
        }
    }

    /* Last active filter */
    *LastFilterHeader |= htole32(FXP_FILTER_EL);

    EeCommandUnitSendCommand(Adapter, FXP_CB_COMMAND_LOADFILT, &FilterSetup->Header);
}

static
CODE_SEG("PAGE")
VOID
EeProgramWakeUpEvents(
    _In_ PE100_ADAPTER Adapter)
{
    PAGED_CODE();

    if ((Adapter->WakeUpFlags & NDIS_PNP_WAKE_UP_LINK_CHANGE) ||
        (Adapter->WakeUpFlags & NDIS_PNP_WAKE_UP_MAGIC_PACKET))
    {
        PFXP_CB_CONFIGURE Config = &Adapter->ControlBlock->Configuration;

        if (Adapter->WakeUpFlags & NDIS_PNP_WAKE_UP_LINK_CHANGE)
        {
            Config->LinkStatusChangeWakeEnable = 1;
        }

        if (Adapter->WakeUpFlags & NDIS_PNP_WAKE_UP_MAGIC_PACKET)
        {
            ASSERT(Adapter->Flags & E100_FLAG_HAS_WOL);

            Config->MagicPacketWakeDisable = 0;
        }

        EeCommandUnitSendCommand(Adapter, FXP_CB_COMMAND_CONFIG, &Config->Header);
    }

    /*
     * If wake on interesting packet is required then
     * this must be the last CU command issued by the driver
     * before the 8255x is set to the power down state.
     */
    if ((Adapter->WakeUpFlags & NDIS_PNP_WAKE_UP_PATTERN_MATCH) &&
        !IsListEmpty(&Adapter->WakeUpFrameList))
    {
        ASSERT(Adapter->Flags & E100_FLAG_HAS_WOL);

        EeDownloadFlexiblePacketFilters(Adapter);
    }
}

static
CODE_SEG("PAGE")
VOID
EePowerDown(
    _In_ PE100_ADAPTER Adapter)
{
    PAGED_CODE();

    EeStopAdapter(Adapter);

    /* Put the CU and RU into the idle state and disable interrupts */
    EeSoftReset(Adapter, FALSE);

    EeFlushTransmitQueue(Adapter);

    if (Adapter->WakeUpFlags != 0)
    {
        EePhyPowerSaveDowngradeLinkSpeed(Adapter);
        EeProgramWakeUpEvents(Adapter);
    }
    else
    {
        EePhySetPower(Adapter, FALSE);
    }
}

static
CODE_SEG("PAGE")
VOID
EePowerUp(
    _In_ PE100_ADAPTER Adapter)
{
    PAGED_CODE();

    if (Adapter->WakeUpFlags != 0)
    {
        /* Clear wakeup events */
        CSR_WRITE_8(Adapter, FXP_CSR_PMDR, CSR_READ_8(Adapter, FXP_CSR_PMDR));
    }
    else
    {
        EePhySetPower(Adapter, TRUE);
    }

    EeSoftReset(Adapter, FALSE);

    EeSetupAdapter(Adapter);

    /* Restore the RX filter */
    EeApplyPacketFilter(Adapter);

    EeStartAdapter(Adapter);
}

CODE_SEG("PAGE")
VOID
NTAPI
EePowerWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context)
{
    PE100_ADAPTER Adapter = Context;

    UNREFERENCED_PARAMETER(WorkItem);

    PAGED_CODE();

    INFO("Power state %lu\n", Adapter->PowerState);

    ASSERT(Adapter->RevisionID != FXP_REV_82557);

    if (Adapter->PowerState == NdisDeviceStateD0)
    {
        EePowerUp(Adapter);
    }
    else
    {
        EePowerDown(Adapter);
    }
    Adapter->PrevPowerState = Adapter->PowerState;

    NdisMSetInformationComplete(Adapter->AdapterHandle, NDIS_STATUS_SUCCESS);
}

static
ULONG
EeBuildFrameSignature(
    _In_ ULONG PatternSize,
    _In_ const UCHAR* PatternMask,
    _In_ const UCHAR* PatternData)
{
    ULONG i, j = 0, Crc = 0;

    for (i = 0; i < PatternSize; ++i)
    {
        if (PatternMask[i / 8] & (1 << (i % 8)))
        {
            ULONG Shift = (j++ % 3) * 8;

            if (Crc & 0x80000000)
                Crc = ((Crc << 1) ^ (PatternData[i] << Shift)) ^ 0x04C11DB7;
            else
                Crc = ((Crc << 1) ^ (PatternData[i] << Shift));
        }
    }

    return Crc & 0xFFFFFF;
}

static
ULONG
EeFlexibleFilterSize(
    _In_ ULONG MaskSize)
{
    ULONG FilterSize;

    /* Filter mask dwords */
    FilterSize = MaskSize / sizeof(ULONG);
    if (MaskSize % sizeof(ULONG))
        FilterSize++;

    return FilterSize;
}

NDIS_STATUS
EeAddWakeUpPattern(
    _In_ PE100_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN PmPattern)
{
    ULONG MaskSize, FilterSize;
    PE100_WAKE_UP_FRAME WakeFrame;
    NDIS_STATUS Status;

    MaskSize = min(PmPattern->MaskSize, FXP_WOL_FILTER_MASKS * sizeof(ULONG));

    FilterSize = EeFlexibleFilterSize(MaskSize);
    if (Adapter->FlexibleFiltersUsed + (FilterSize + 1) > FXP_WOL_PATTERN_FILTERS)
        return NDIS_STATUS_RESOURCES;

    Status = NdisAllocateMemoryWithTag((PVOID*)&WakeFrame, sizeof(*WakeFrame), E100_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return NDIS_STATUS_RESOURCES;

    /* +1 to include the filter type dword */
    Adapter->FlexibleFiltersUsed += FilterSize + 1;

    WakeFrame->FilterSize = FilterSize;
    WakeFrame->Signature = EeBuildFrameSignature(MaskSize * RTL_BITS_OF(UCHAR),
                                                 (PUCHAR)PmPattern + sizeof(*PmPattern),
                                                 (PUCHAR)PmPattern + PmPattern->PatternOffset);
    RtlCopyMemory(&WakeFrame->PatternMask,
                  (PUCHAR)PmPattern + sizeof(*PmPattern),
                  MaskSize);

    InsertTailList(&Adapter->WakeUpFrameList, &WakeFrame->ListEntry);

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
EeRemoveWakeUpPattern(
    _In_ PE100_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN PmPattern)
{
    PLIST_ENTRY Entry;
    ULONG MaskSize, FilterSize, Signature;

    MaskSize = min(PmPattern->MaskSize, FXP_WOL_FILTER_MASKS * sizeof(ULONG));

    FilterSize = EeFlexibleFilterSize(MaskSize);

    Signature = EeBuildFrameSignature(MaskSize * RTL_BITS_OF(UCHAR),
                                      (PUCHAR)PmPattern + sizeof(*PmPattern),
                                      (PUCHAR)PmPattern + PmPattern->PatternOffset);

    for (Entry = Adapter->WakeUpFrameList.Flink;
         Entry != &Adapter->WakeUpFrameList;
         Entry = Entry->Flink)
    {
        PE100_WAKE_UP_FRAME WakeFrame = CONTAINING_RECORD(Entry, E100_WAKE_UP_FRAME, ListEntry);

        if (WakeFrame->FilterSize != FilterSize)
            continue;

        if (WakeFrame->Signature != Signature)
            continue;

        if (!RtlEqualMemory(&WakeFrame->PatternMask,
                            (PUCHAR)PmPattern + sizeof(*PmPattern),
                            MaskSize))
        {
            continue;
        }

        RemoveEntryList(&WakeFrame->ListEntry);

        Adapter->FlexibleFiltersUsed -= WakeFrame->FilterSize + 1;

        NdisFreeMemory(WakeFrame, sizeof(*WakeFrame), 0);
        return NDIS_STATUS_SUCCESS;
    }

    return NDIS_STATUS_INVALID_DATA;
}

NDIS_STATUS
EeGetPowerManagementCapabilities(
    _In_ PE100_ADAPTER Adapter,
    _Out_ PNDIS_PNP_CAPABILITIES Capabilities)
{
    /* The 82557 has no power management support */
    if (Adapter->RevisionID == FXP_REV_82557)
        return NDIS_STATUS_NOT_SUPPORTED;

    if (!(Adapter->Flags & E100_FLAG_HAS_WOL))
    {
        Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
        Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateUnspecified;

        /* The 82559ER can only wake on link status change */
        if (Adapter->RevisionID == FXP_REV_82559S_A)
            Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateD3;
        else
            Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateUnspecified;
    }
    else
    {
        if (Adapter->RevisionID == FXP_REV_82558_A4)
        {
            Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateD1;
            Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateUnspecified;
            Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateUnspecified;
        }
        else
        {
            Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateD3;
            Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateD3;
            Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateD3;
        }

        /*
         * The 82558 B-step requires the driver to download a specific microcode
         * to support flexible packet filtering (which has never been released to the public).
         */
        if (Adapter->RevisionID == FXP_REV_82558_B0)
        {
            Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateUnspecified;
        }
    }

    return NDIS_STATUS_SUCCESS;
}
