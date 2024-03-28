/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PATA timings support
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

#define ADVPIO_MODE_MASK  ((1 << 2) - 1) /* 2 modes */
#define SWMA_MODE_MASK    ((1 << 3) - 1) /* 3 modes */
#define MWMA_MODE_MASK    ((1 << 3) - 1) /* 3 modes */
#define UDMA_MODE_MASK    ((1 << 7) - 1) /* 7 modes */
#define PIO0_SHIFT        0
#define PIO3_SHIFT        3
#define PIO4_SHIFT        4
#define SWDMA0_SHIFT      5
#define MWDMA0_SHIFT      8
#define UDMA0_SHIFT       11

static const ULONG AtapTimingTable[] =
{
    /* PIO 0-4 */
    600, 383, 240, 180, 120,

    /* SWDMA 0-2 */
    960, 480, 240,

    /* MWDMA 0-2 */
    480, 150, 120,

    /* UDMA 0-6 */
    120, 80, 60, 45, 30, 20, 15
};

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
AtaTimInitPioMode(
    _In_ ULONG Device,
    _Inout_ PPCIIDE_TRANSFER_MODE_SELECT XferMode)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = (PIDENTIFY_DEVICE_DATA)&XferMode->IdentifyData[Device];
    ULONG SupportedMode, BestMode, CurrentMode, CycleTime;

    PAGED_CODE();

    CycleTime = (ULONG)-1;

    /* Find the best possible PIO mode */
    if (IdentifyData->TranslationFieldsValid & 0x2)
    {
        /* PIO 3-4 */
        SupportedMode = IdentifyData->AdvancedPIOModes & ADVPIO_MODE_MASK;
        SupportedMode <<= PIO3_SHIFT;

        /* The device is assumed to have PIO 0-2 */
        SupportedMode |= (PIO_MODE0 | PIO_MODE1 | PIO_MODE2);

        if (IdentifyData->Capabilities.IordySupported)
            CycleTime = IdentifyData->MinimumPIOCycleTimeIORDY;
        else
            CycleTime = IdentifyData->MinimumPIOCycleTime;
    }
    else
    {
        /* PIO 0-4 */
        SupportedMode = IdentifyData->ObsoleteWords51[0] >> 8;
        if (SupportedMode > PIO4_SHIFT)
        {
            SupportedMode = PIO0_SHIFT;
        }
        BestMode = SupportedMode;
        SupportedMode = (1 << (SupportedMode + 1)) - 1;

        if (BestMode < XferMode->TransferModeTableLength)
        {
            CycleTime = XferMode->TransferModeTimingTable[BestMode];
        }
    }
    XferMode->DeviceTransferModeSupported[Device] |= SupportedMode;
    XferMode->BestPioCycleTime[Device] = CycleTime;

    /* FIXME: How to get the current PIO mode? */
    _BitScanReverse(&BestMode, SupportedMode);
    CurrentMode = BestMode;
    XferMode->DeviceTransferModeCurrent[Device] |= 1 << CurrentMode;
}

static
CODE_SEG("PAGE")
VOID
AtaTimInitSingleWordDmaMode(
    _In_ ULONG Device,
    _Inout_ PPCIIDE_TRANSFER_MODE_SELECT XferMode)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = (PIDENTIFY_DEVICE_DATA)&XferMode->IdentifyData[Device];
    ULONG BestMode, CurrentMode, CycleTime;

    PAGED_CODE();

    CycleTime = (ULONG)-1;

    /* Find the current active SWDMA mode */
    CurrentMode = ((IdentifyData->ObsoleteWord62 & 0xFF00) >> 16) & SWMA_MODE_MASK;
    if (CurrentMode != 0)
    {
        _BitScanReverse(&CurrentMode, CurrentMode);
        CurrentMode += SWDMA0_SHIFT;

        XferMode->DeviceTransferModeCurrent[Device] |= 1 << CurrentMode;
    }

    /* Find the best possible SWDMA mode */
    BestMode = (IdentifyData->ObsoleteWord62 & 0x00FF) & SWMA_MODE_MASK;
    if (BestMode != 0)
    {
        BestMode <<= SWDMA0_SHIFT;

        XferMode->DeviceTransferModeSupported[Device] |= BestMode;

        _BitScanReverse(&BestMode, BestMode);
        if (BestMode < XferMode->TransferModeTableLength)
        {
            CycleTime = XferMode->TransferModeTimingTable[BestMode];
        }
    }

    XferMode->BestSwDmaCycleTime[Device] = CycleTime;
}

static
CODE_SEG("PAGE")
VOID
AtaTimInitMultiWordDmaMode(
    _In_ ULONG Device,
    _Inout_ PPCIIDE_TRANSFER_MODE_SELECT XferMode)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = (PIDENTIFY_DEVICE_DATA)&XferMode->IdentifyData[Device];
    ULONG SupportedMode, BestMode, CurrentMode, CycleTime;

    PAGED_CODE();

    CycleTime = (ULONG)-1;

    /* Find the current active MWDMA mode */
    CurrentMode = IdentifyData->MultiWordDMAActive & MWMA_MODE_MASK;
    if (CurrentMode != 0)
    {
        _BitScanReverse(&CurrentMode, CurrentMode);
        CurrentMode += MWDMA0_SHIFT;

        XferMode->DeviceTransferModeCurrent[Device] |= 1 << CurrentMode;
    }

    /* Find the best possible MWDMA mode */
    SupportedMode = IdentifyData->MultiWordDMASupport & MWMA_MODE_MASK;
    if (SupportedMode != 0)
    {
        SupportedMode <<= MWDMA0_SHIFT;
        XferMode->DeviceTransferModeSupported[Device] |= SupportedMode;

        _BitScanReverse(&BestMode, SupportedMode);

        /* Prefer the minimum cycle time, if words 64-70 are valid */
        if ((IdentifyData->TranslationFieldsValid & 0x2) &&
            (IdentifyData->MinimumMWXferCycleTime != 0) &&
            (IdentifyData->RecommendedMWXferCycleTime != 0))
        {
            CycleTime = IdentifyData->MinimumMWXferCycleTime;
        }
        else if (BestMode < XferMode->TransferModeTableLength)
        {
            CycleTime = XferMode->TransferModeTimingTable[BestMode];
        }
    }

    XferMode->BestMwDmaCycleTime[Device] = CycleTime;
}

static
CODE_SEG("PAGE")
VOID
AtaTimInitUltraDmaMode(
    _In_ ULONG Device,
    _Inout_ PPCIIDE_TRANSFER_MODE_SELECT XferMode)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = (PIDENTIFY_DEVICE_DATA)&XferMode->IdentifyData[Device];
    ULONG SupportedMode, BestMode, CurrentMode, CycleTime;

    PAGED_CODE();

    CycleTime = (ULONG)-1;

    /* Word 88 is not valid */
    if (!(IdentifyData->TranslationFieldsValid & 0x4))
        goto Exit;

    /* Find the current active UDMA mode */
    CurrentMode = IdentifyData->UltraDMAActive & UDMA_MODE_MASK;
    if (CurrentMode != 0)
    {
        _BitScanReverse(&CurrentMode, CurrentMode);
        CurrentMode += UDMA0_SHIFT;

        XferMode->DeviceTransferModeCurrent[Device] |= 1 << CurrentMode;
    }

    /* Find the best possible UDMA mode */
    SupportedMode = IdentifyData->UltraDMASupport & UDMA_MODE_MASK;
    if (SupportedMode != 0)
    {
        SupportedMode <<= UDMA0_SHIFT;
        XferMode->DeviceTransferModeSupported[Device] |= SupportedMode;

        _BitScanReverse(&BestMode, SupportedMode);
        if (BestMode < XferMode->TransferModeTableLength)
        {
            CycleTime = XferMode->TransferModeTimingTable[BestMode];
        }
    }

Exit:
    XferMode->BestUDmaCycleTime[Device] = CycleTime;
}

static
CODE_SEG("PAGE")
VOID
AtaAcpiSelectTimings(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PPCIIDE_TRANSFER_MODE_SELECT XferMode)
{
    IDE_ACPI_TIMING_MODE_BLOCK TimingMode;
    ULONG i, CurrentModeFlags;

    PAGED_CODE();

    CurrentModeFlags = ChannelExtension->TimingMode.ModeFlags;
    CurrentModeFlags &= (IDE_ACPI_TIMING_MODE_FLAG_INDEPENDENT_TIMINGS |
                         IDE_ACPI_TIMING_MODE_FLAG_IORDY |
                         IDE_ACPI_TIMING_MODE_FLAG_IORDY2);
    TimingMode.ModeFlags = CurrentModeFlags;

    /* NOTE: Some ACPI BIOS implementations (e.g. VPC 2007) don't return correct timings */
    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        TimingMode.Drive[i].PioSpeed = ChannelExtension->TimingMode.Drive[i].PioSpeed;
        TimingMode.Drive[i].DmaSpeed = ChannelExtension->TimingMode.Drive[i].DmaSpeed;
    }

    AtaAcpiSetTimingMode(ChannelExtension,
                         &TimingMode,
                         (PIDENTIFY_DEVICE_DATA)&XferMode->IdentifyData[0],
                         (PIDENTIFY_DEVICE_DATA)&XferMode->IdentifyData[1]);
}

CODE_SEG("PAGE")
VOID
AtaSelectTimings(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PCIIDE_TRANSFER_MODE_SELECT XferMode;
    ULONG i;

    PAGED_CODE();

    RtlZeroMemory(&XferMode, sizeof(XferMode));

    if (!(ChannelExtension->Flags & CHANNEL_HAS_CHIPSET_INTERFACE))
    {
        XferMode.TransferModeTimingTable = (PULONG)AtapTimingTable;
        XferMode.TransferModeTableLength = RTL_NUMBER_OF(AtapTimingTable);
    }

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PATAPORT_DEVICE_EXTENSION DeviceExtension = ChannelExtension->Device[i];
        PIDENTIFY_DEVICE_DATA IdentifyData = &DeviceExtension->IdentifyDeviceData;

        if (!DeviceExtension)
            continue;

        RtlCopyMemory(&XferMode.IdentifyData[i], IdentifyData, sizeof(*IdentifyData));

        XferMode.DevicePresent[i] = TRUE;
        XferMode.FixedDisk[i] = IS_ATAPI(DeviceExtension);
        XferMode.IoReadySupported[i] = !!IdentifyData->Capabilities.IordySupported;
        XferMode.UserChoiceTransferMode[i] = 0x7FFFFFFF;

        AtaTimInitPioMode(i, &XferMode);
        AtaTimInitSingleWordDmaMode(i, &XferMode);
        AtaTimInitMultiWordDmaMode(i, &XferMode);
        AtaTimInitUltraDmaMode(i, &XferMode);

        INFO("PIO cycle time              %ld\n", (LONG)XferMode.BestPioCycleTime[i]);
        INFO("SWDMA cycle time            %ld\n", (LONG)XferMode.BestSwDmaCycleTime[i]);
        INFO("MWDMA cycle time            %ld\n", (LONG)XferMode.BestMwDmaCycleTime[i]);
        INFO("UDMA cycle time             %ld\n", (LONG)XferMode.BestUDmaCycleTime[i]);
        INFO("DeviceTransferModeCurrent   0x%08lx\n", XferMode.DeviceTransferModeCurrent[i]);
        INFO("DeviceTransferModeSupported 0x%08lx\n", XferMode.DeviceTransferModeSupported[i]);
        INFO("UserChoiceTransferMode      0x%08lx\n", XferMode.UserChoiceTransferMode[i]);
        INFO("IOREADY support             %u\n", XferMode.IoReadySupported[i]);
    }

    if (ChannelExtension->Flags & CHANNEL_HAS_GTM)
    {
        /* Set the transfer timings through ACPI */
        AtaAcpiSelectTimings(ChannelExtension, &XferMode);
    }
}
