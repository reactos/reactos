/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA PIO and DMA timings support
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 *
 * REFERENCES:  For more details refer to the Intel order no 298600-004
 *              "Determining a Drive's Transfer Rate Capabilities"
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static const ULONG AtapModeToCycleTime[] =
{
    /* 0   1    2    3    4  PIO_MODE */
    600, 383, 240, 180, 120,

    /* 0   1    2  SWDMA_MODE */
    960, 480, 240,

    /* 0   1    2  MWDMA_MODE */
    480, 150, 120,

    /* 0  1   2   3   4   5   6  UDMA_MODE */
    120, 80, 60, 45, 30, 20, 15
};

/* FUNCTIONS ******************************************************************/

static
VOID
AtaTimQueryPioModeSupport(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG AllowedModesMask)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    ULONG SupportedMode, BestMode, CurrentMode, CycleTime;

    /* Find the best possible PIO mode */
    if (IdentifyData->TranslationFieldsValid & 0x2)
    {
        /* PIO 3-4 */
        SupportedMode = IdentifyData->AdvancedPIOModes & 0x3;
        SupportedMode <<= PIO_MODE(3);

        /* The device is assumed to have PIO 0-2 */
        SupportedMode |= PIO_MODE0 | PIO_MODE1 | PIO_MODE2;

        SupportedMode &= AllowedModesMask;

        if (IdentifyData->Capabilities.IordySupported)
            CycleTime = IdentifyData->MinimumPIOCycleTimeIORDY;
        else
            CycleTime = IdentifyData->MinimumPIOCycleTime;

        /*
         * Any device that supports PIO 3 or above
         * should support word 67 and word 68, but be defensive.
         */
        if (CycleTime == 0)
        {
            WARN("Device '%s' returned zero cycle time\n", DevExt->FriendlyName);
            NT_VERIFY(_BitScanReverse(&BestMode, SupportedMode));

            /* Fall back to the default value */
            ASSERT(BestMode < RTL_NUMBER_OF(AtapModeToCycleTime));
            CycleTime = AtapModeToCycleTime[BestMode];
        }
    }
    else
    {
        /* PIO 0-4 (maximum supported mode) */
        SupportedMode = IdentifyData->ObsoleteWords51[0] >> 8;
        if (SupportedMode > PIO_MODE(4))
            SupportedMode = PIO_MODE(0);

        /* Convert the maximum mode to a mask */
        SupportedMode = (1 << (SupportedMode + 1)) - 1;
        SupportedMode &= AllowedModesMask;

        NT_VERIFY(_BitScanReverse(&BestMode, SupportedMode));

        ASSERT(BestMode < RTL_NUMBER_OF(AtapModeToCycleTime));
        CycleTime = AtapModeToCycleTime[BestMode];
    }
    DevExt->TransferModeSupportedBitmap |= SupportedMode;
    DevExt->MinimumPioCycleTime = CycleTime;

    /*
     * Since there is no way to obtain the current PIO mode from identify data
     * we just look for the fastest supported mode.
     */
    NT_VERIFY(_BitScanReverse(&BestMode, SupportedMode));
    CurrentMode = BestMode;
    DevExt->TransferModeCurrentBitmap |= 1 << CurrentMode;
}

static
VOID
AtaTimQuerySwDmaModeSupport(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG AllowedModesMask)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    ULONG SupportedMode, BestMode, CurrentMode, CycleTime;

    CycleTime = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;

    /* Find the current active SWDMA mode */
    CurrentMode = ((IdentifyData->ObsoleteWord62 & 0xFF00) >> 16) & 0x7;
    if (CurrentMode != 0)
    {
        NT_VERIFY(_BitScanReverse(&CurrentMode, CurrentMode));
        CurrentMode += SWDMA_MODE(0);

        DevExt->TransferModeCurrentBitmap |= 1 << CurrentMode;
    }

    /* Find the best possible SWDMA mode */
    SupportedMode = (IdentifyData->ObsoleteWord62 & 0x00FF) & 0x7;
    SupportedMode <<= SWDMA_MODE(0);
    SupportedMode &= AllowedModesMask;
    if (SupportedMode != 0)
    {
        DevExt->TransferModeSupportedBitmap |= SupportedMode;

        NT_VERIFY(_BitScanReverse(&BestMode, SupportedMode));

        ASSERT(BestMode < RTL_NUMBER_OF(AtapModeToCycleTime));
        CycleTime = AtapModeToCycleTime[BestMode];
    }

    DevExt->MinimumSingleWordDmaCycleTime = CycleTime;
}

static
VOID
AtaTimQueryMwDmaModeSupport(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG AllowedModesMask)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    ULONG SupportedMode, BestMode, CurrentMode, CycleTime;

    CycleTime = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;

    /* Find the current active MWDMA mode */
    CurrentMode = IdentifyData->MultiWordDMAActive & 0x7;
    if (CurrentMode != 0)
    {
        NT_VERIFY(_BitScanReverse(&CurrentMode, CurrentMode));
        CurrentMode += MWDMA_MODE(0);

        DevExt->TransferModeCurrentBitmap |= 1 << CurrentMode;
    }

    /* Find the best possible MWDMA mode */
    SupportedMode = IdentifyData->MultiWordDMASupport & 0x7;
    SupportedMode <<= MWDMA_MODE(0);
    SupportedMode &= AllowedModesMask;
    if (SupportedMode != 0)
    {
        DevExt->TransferModeSupportedBitmap |= SupportedMode;

        NT_VERIFY(_BitScanReverse(&BestMode, SupportedMode));

        /* Prefer the minimum cycle time, if words 64-70 are valid */
        if ((IdentifyData->TranslationFieldsValid & 0x2) &&
            (IdentifyData->MinimumMWXferCycleTime != 0) &&
            (IdentifyData->RecommendedMWXferCycleTime != 0))
        {
            CycleTime = IdentifyData->MinimumMWXferCycleTime;
        }
        else
        {
            ASSERT(BestMode < RTL_NUMBER_OF(AtapModeToCycleTime));
            CycleTime = AtapModeToCycleTime[BestMode];
        }
    }

    DevExt->MinimumMultiWordDmaCycleTime = CycleTime;
}

static
VOID
AtaTimQueryUDmaModeSupport(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG AllowedModesMask)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    ULONG SupportedMode, BestMode, CurrentMode, CycleTime;

    CycleTime = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;

    /* Word 88 is not valid */
    if (!(IdentifyData->TranslationFieldsValid & 0x4))
        goto Exit;

    /* Find the current active UDMA mode */
    CurrentMode = IdentifyData->UltraDMAActive & 0x7F;
    if (CurrentMode != 0)
    {
        NT_VERIFY(_BitScanReverse(&CurrentMode, CurrentMode));
        CurrentMode += UDMA_MODE(0);

        DevExt->TransferModeCurrentBitmap |= 1 << CurrentMode;
    }

    /* Find the best possible UDMA mode */
    SupportedMode = IdentifyData->UltraDMASupport & 0x7F;
    SupportedMode <<= UDMA_MODE(0);
    SupportedMode &= AllowedModesMask;
    if (SupportedMode != 0)
    {
        DevExt->TransferModeSupportedBitmap |= SupportedMode;

        NT_VERIFY(_BitScanReverse(&BestMode, SupportedMode));

        ASSERT(BestMode < RTL_NUMBER_OF(AtapModeToCycleTime));
        CycleTime = AtapModeToCycleTime[BestMode];
    }

Exit:
    DevExt->MinimumUltraDmaCycleTime = CycleTime;
}

static
VOID
AtaTimDumpTimingInfo(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    INFO("CH %lu: Device %u '%s'\n"
         "Cycle time       UDMA[%ld] MWDMA[%ld] SWDMA[%ld] PIO[%ld]\n"
         "Supported modes  0x%08lX\n"
         "Active modes     0x%08lX\n"
         "Selected modes   0x%08lX\n"
         "IOREADY          %u\n",
        PortData->PortNumber,
        DevExt->Device.AtaScsiAddress.TargetId,
        DevExt->FriendlyName,
        (LONG)DevExt->MinimumUltraDmaCycleTime,
        (LONG)DevExt->MinimumMultiWordDmaCycleTime,
        (LONG)DevExt->MinimumSingleWordDmaCycleTime,
        (LONG)DevExt->MinimumPioCycleTime,
        DevExt->TransferModeSupportedBitmap,
        DevExt->TransferModeCurrentBitmap,
        DevExt->TransferModeSelectedBitmap,
        DevExt->IdentifyDeviceData.Capabilities.IordySupported);
}

VOID
AtaPortSelectTimings(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ BOOLEAN ForceCompatibleTimings)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    ULONG i;
    CHANNEL_DEVICE_CONFIG Config[ATA_MAX_DEVICE] = { 0 };
    PCHANNEL_DEVICE_CONFIG DeviceList[ATA_MAX_DEVICE] = { 0 };

    if (ForceCompatibleTimings)
    {
        for (i = 0; i < ATA_MAX_DEVICE; ++i)
        {
            /* Fake device presence */
            DeviceList[i] = &Config[i];

            /* Disable DMA and select compatible timings */
            Config[i].SupportedModes = PIO_MODE0;
            Config[i].CurrentModes = PIO_MODE0;
            Config[i].MinPioCycleTime = AtapModeToCycleTime[PIO_MODE(0)];
            Config[i].MinSwDmaCycleTime = AtapModeToCycleTime[SWDMA_MODE(0)];
            Config[i].MinMwDmaCycleTime = AtapModeToCycleTime[MWDMA_MODE(0)];
            Config[i].IsFixedDisk = TRUE;
            Config[i].IoReadySupported = FALSE;
            Config[i].IsNewDevice = FALSE;
            Config[i].FriendlyName = "";
        }

        /* Call the chipset driver */
        PortData->SetTransferMode(PortData->ChannelContext, DeviceList);
        return;
    }

    ChanExt = CONTAINING_RECORD(PortData, ATAPORT_CHANNEL_EXTENSION, PortData);

    for (i = 0; i < ATA_MAX_DEVICE; ++i)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        ULONG AllowedModesMask;

        DevExt = AtaFdoFindDeviceByPath(ChanExt,
                                        AtaMarshallScsiAddress(PortData->PortNumber, i, 0),
                                        NULL);
        if (!DevExt)
            continue;

        DevExt->TransferModeCurrentBitmap = 0;
        DevExt->TransferModeSupportedBitmap = 0;

        AllowedModesMask = DevExt->TransferModeUserAllowedMask;
        AllowedModesMask &= DevExt->TransferModeAllowedMask;
        /* Mode cannot be slower than PIO0 */
        AllowedModesMask |= PIO_MODE0;

        AtaTimQueryPioModeSupport(DevExt, AllowedModesMask);
        AtaTimQuerySwDmaModeSupport(DevExt, AllowedModesMask);
        AtaTimQueryMwDmaModeSupport(DevExt, AllowedModesMask);
        AtaTimQueryUDmaModeSupport(DevExt, AllowedModesMask);

        DeviceList[i] = &Config[i];
        Config[i].CurrentModes = DevExt->TransferModeCurrentBitmap;
        Config[i].SupportedModes = DevExt->TransferModeSupportedBitmap;
        Config[i].MinPioCycleTime = DevExt->MinimumPioCycleTime;
        Config[i].MinSwDmaCycleTime = DevExt->MinimumSingleWordDmaCycleTime;
        Config[i].MinMwDmaCycleTime = DevExt->MinimumMultiWordDmaCycleTime;
        Config[i].IsFixedDisk = !IS_ATAPI(&DevExt->Device);
        Config[i].IoReadySupported = !!DevExt->IdentifyDeviceData.Capabilities.IordySupported;
        Config[i].IsNewDevice = !(DevExt->Device.DeviceFlags & DEVICE_PNP_STARTED);
        Config[i].FriendlyName = DevExt->FriendlyName;
    }

    /* Call the chipset driver */
    PortData->SetTransferMode(PortData->ChannelContext, DeviceList);

    /* Save the result */
    for (i = 0; i < ATA_MAX_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG DeviceConfig = DeviceList[i];
        PATAPORT_DEVICE_EXTENSION DevExt;

        if (!DeviceConfig)
            continue;

        DevExt = AtaFdoFindDeviceByPath(ChanExt,
                                        AtaMarshallScsiAddress(PortData->PortNumber, i, 0),
                                        NULL);
        ASSERT(DevExt);

        DevExt->TransferModeSelectedBitmap = 1 << DeviceConfig->PioMode;
        if (DeviceConfig->DmaMode != PIO_MODE(0))
        {
            DevExt->TransferModeSelectedBitmap |= 1 << DeviceConfig->DmaMode;
        }

        AtaTimDumpTimingInfo(PortData, DevExt);
    }
}
