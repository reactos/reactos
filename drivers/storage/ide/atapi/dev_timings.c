/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA PIO and DMA timings support
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 *
 * REFERENCES:  For more details refer to the Intel Manual (248704-001)
 *              "Determining a Drive's Transfer Rate Capabilities"
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static const ULONG AtapTimingTable[] =
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
AtaTimInitPioMode(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    ULONG SupportedMode, BestMode, CurrentMode, CycleTime;

    CycleTime = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;

    /* Find the best possible PIO mode */
    if (IdentifyData->TranslationFieldsValid & 0x2)
    {
        /* PIO 3-4 */
        SupportedMode = IdentifyData->AdvancedPIOModes & 0x3;
        SupportedMode <<= PIO_MODE(3);

        /* The device is assumed to have PIO 0-2 */
        SupportedMode |= (PIO_MODE0 | PIO_MODE1 | PIO_MODE2);

        SupportedMode &= DevExt->TransferModeAllowedMask;

        if (IdentifyData->Capabilities.IordySupported)
            CycleTime = IdentifyData->MinimumPIOCycleTimeIORDY;
        else
            CycleTime = IdentifyData->MinimumPIOCycleTime;
    }
    else
    {
        /* PIO 0-4 */
        SupportedMode = IdentifyData->ObsoleteWords51[0] >> 8;
        if (SupportedMode > PIO_MODE(4))
        {
            SupportedMode = PIO_MODE(0);
        }
        BestMode = SupportedMode;
        SupportedMode = (1 << (SupportedMode + 1)) - 1;

        SupportedMode &= DevExt->TransferModeAllowedMask;

        if (BestMode < RTL_NUMBER_OF(AtapTimingTable))
        {
            CycleTime = AtapTimingTable[BestMode];
        }
    }
    DevExt->TransferModeSupportedBitmap |= SupportedMode;
    DevExt->PioCycleTime = CycleTime;

    /*
     * Since there is no way to obtain the current PIO mode
     * we just look for the fastest supported mode.
     */
    NT_VERIFY(_BitScanReverse(&BestMode, SupportedMode));
    CurrentMode = BestMode;
    DevExt->TransferModeCurrentBitmap |= 1 << CurrentMode;
}

static
VOID
AtaTimInitSingleWordDmaMode(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    ULONG SupportedMode, BestMode, CurrentMode, CycleTime;

    CycleTime = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;

    /* Find the current active SWDMA mode */
    CurrentMode = ((IdentifyData->ObsoleteWord62 & 0xFF00) >> 16) & 0x7;
    if (CurrentMode != 0)
    {
        _BitScanReverse(&CurrentMode, CurrentMode);
        CurrentMode += SWDMA_MODE(0);

        DevExt->TransferModeCurrentBitmap |= 1 << CurrentMode;
    }

    /* Find the best possible SWDMA mode */
    SupportedMode = (IdentifyData->ObsoleteWord62 & 0x00FF) & 0x7;
    if (SupportedMode != 0)
    {
        SupportedMode <<= SWDMA_MODE(0);
        SupportedMode &= DevExt->TransferModeAllowedMask;

        DevExt->TransferModeSupportedBitmap |= SupportedMode;

        _BitScanReverse(&BestMode, SupportedMode);
        if (BestMode < RTL_NUMBER_OF(AtapTimingTable))
        {
            CycleTime = AtapTimingTable[BestMode];
        }
    }

    DevExt->SingleWordDmaCycleTime = CycleTime;
}

static
VOID
AtaTimInitMultiWordDmaMode(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    ULONG SupportedMode, BestMode, CurrentMode, CycleTime;

    CycleTime = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;

    /* Find the current active MWDMA mode */
    CurrentMode = IdentifyData->MultiWordDMAActive & 0x7;
    if (CurrentMode != 0)
    {
        _BitScanReverse(&CurrentMode, CurrentMode);
        CurrentMode += MWDMA_MODE(0);

        DevExt->TransferModeCurrentBitmap |= 1 << CurrentMode;
    }

    /* Find the best possible MWDMA mode */
    SupportedMode = IdentifyData->MultiWordDMASupport & 0x7;
    if (SupportedMode != 0)
    {
        SupportedMode <<= MWDMA_MODE(0);
        SupportedMode &= DevExt->TransferModeAllowedMask;

        DevExt->TransferModeSupportedBitmap |= SupportedMode;

        _BitScanReverse(&BestMode, SupportedMode);

        /* Prefer the minimum cycle time, if words 64-70 are valid */
        if ((IdentifyData->TranslationFieldsValid & 0x2) &&
            (IdentifyData->MinimumMWXferCycleTime != 0) &&
            (IdentifyData->RecommendedMWXferCycleTime != 0))
        {
            CycleTime = IdentifyData->MinimumMWXferCycleTime;
        }
        else if (BestMode < RTL_NUMBER_OF(AtapTimingTable))
        {
            CycleTime = AtapTimingTable[BestMode];
        }
    }

    DevExt->MultiWordDmaCycleTime = CycleTime;
}

static
VOID
AtaTimInitUltraDmaMode(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
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
        _BitScanReverse(&CurrentMode, CurrentMode);
        CurrentMode += UDMA_MODE(0);

        DevExt->TransferModeCurrentBitmap |= 1 << CurrentMode;
    }

    /* Find the best possible UDMA mode */
    SupportedMode = IdentifyData->UltraDMASupport & 0x7F;
    if (SupportedMode != 0)
    {
        SupportedMode <<= UDMA_MODE(0);
        SupportedMode &= DevExt->TransferModeAllowedMask;

        DevExt->TransferModeSupportedBitmap |= SupportedMode;

        _BitScanReverse(&BestMode, SupportedMode);
        if (BestMode < RTL_NUMBER_OF(AtapTimingTable))
        {
            CycleTime = AtapTimingTable[BestMode];
        }
    }

Exit:
    DevExt->UltraDmaCycleTime = CycleTime;
}

static
VOID
AtaTimDumpTimingInfo(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    INFO("Device %u:%u '%s'\n"
         "PIO cycle time   %ld\n"
         "SWDMA cycle time %ld\n"
         "MWDMA cycle time %ld\n"
         "UDMA cycle time  %ld\n"
         "Current modes    0x%08lX\n"
         "Supported modes  0x%08lX\n"
         "Selected modes   0x%08lX\n"
         "IOREADY          %u\n",
        DevExt->Device.AtaScsiAddress.PathId,
        DevExt->Device.AtaScsiAddress.TargetId,
        DevExt->FriendlyName,
        (LONG)DevExt->PioCycleTime,
        (LONG)DevExt->SingleWordDmaCycleTime,
        (LONG)DevExt->MultiWordDmaCycleTime,
        (LONG)DevExt->UltraDmaCycleTime,
        DevExt->TransferModeCurrentBitmap,
        DevExt->TransferModeSupportedBitmap,
        DevExt->TransferModeSelectedBitmap,
        DevExt->IdentifyDeviceData.Capabilities.IordySupported);
}

static
BOOLEAN
AtaTimShouldUseAcpi(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* No _GTM data available */
    if (!(PortData->ChanExt->Flags & CHANNEL_HAS_GTM))
        return FALSE;

    /* We do not want to deal with legacy controllers */
    if (!(PortData->Pata.CurrentTimingMode.ModeFlags &
          IDE_ACPI_TIMING_MODE_FLAG_INDEPENDENT_TIMINGS))
    {
        return FALSE;
    }

    // TODO: Any more special cases?

    return TRUE;
}

static
ULONG
AtaTimFindMode(
    _In_ ULONG SupportedModesBitmap,
    _In_ ULONG GtmCycleTime,
    _In_ ULONG StartMode,
    _Out_ PULONG CycleTime,
    _Out_ PULONG BestMode)
{
    LONG i;

    for (i = StartMode; i >= 0; i--)
    {
        if ((SupportedModesBitmap & (1 << i)) && (AtapTimingTable[i] >= GtmCycleTime))
        {
            *CycleTime = AtapTimingTable[i];
            *BestMode = i;
            return TRUE;
        }
    }

    INFO("Failed to find mode for %ld ns cycle time from %lu in %08lx\n",
         (LONG)GtmCycleTime,
         StartMode,
         SupportedModesBitmap);
    return FALSE;
}

static
VOID
AtaTimSelectTimingsAcpi(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PIDE_ACPI_TIMING_MODE_BLOCK CurrentMode = &PortData->Pata.CurrentTimingMode;
    PATAPORT_DEVICE_EXTENSION Devices[MAX_IDE_DEVICE];
    IDE_ACPI_TIMING_MODE_BLOCK NewMode;
    ULONG i;

    ASSERT(PortData->ChanExt->Flags & CHANNEL_HAS_GTM);

    NewMode.ModeFlags = CurrentMode->ModeFlags;

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        BOOLEAN Success, HasUltraDMA;
        ULONG CycleTime, BestMode;

        DevExt = AtaFdoFindDeviceByPath(PortData->ChanExt, AtaMarshallScsiAddress(0, i, 0));
        Devices[i] = DevExt;
        if (!DevExt)
        {
            NewMode.Drive[i].PioSpeed = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;
            NewMode.Drive[i].DmaSpeed = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;
            continue;
        }

        /* PIO timing */
        Success = AtaTimFindMode(DevExt->TransferModeSupportedBitmap,
                                 CurrentMode->Drive[i].PioSpeed,
                                 PIO_MODE(4),
                                 &CycleTime,
                                 &BestMode);
        if (!Success)
        {
            ULONG Modes;

            /*
             * For example, _GTM on VPC 2007 will return
             *   Drive[0].PioSpeed  900 ns
             *   Drive[0].DmaSpeed  900 ns
             * for an ATAPI device, although the device advertises itself as
             *   PIO speed          508 ns
             *   SWDMA speed        240 ns
             *   MWDMA speed        120 ns
             *   UDMA speed         -1  ns
             * The raw device identify data words:
             *   53 = 0x0003
             *   51 = 0x0200
             *   64 = 0x0003
             *   88 = 0x0000
             *   62 = 0x0007
             *   63 = 0x0407
             *   65 = 120
             *   66 = 120
             *   67 = 508
             *   68 = 180
             * So we try to determine the cycle value by other means.
             */

            Modes = DevExt->TransferModeCurrentBitmap & PIO_ALL;
            Modes &= DevExt->TransferModeAllowedMask;

            /* Search for the best enabled PIO mode */
            if (_BitScanReverse(&BestMode, Modes) && BestMode < RTL_NUMBER_OF(AtapTimingTable))
            {
                CycleTime = AtapTimingTable[BestMode];
            }
            else
            {
                CycleTime = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;
                BestMode = PIO_MODE(0);
            }
        }
        NewMode.Drive[i].PioSpeed = CycleTime;
        DevExt->TransferModeSelectedBitmap |= 1 << BestMode;

        /* DMA timing */
        HasUltraDMA = CurrentMode->ModeFlags & (IDE_ACPI_TIMING_MODE_FLAG_UDMA << (2 * i));
        Success = AtaTimFindMode(DevExt->TransferModeSupportedBitmap,
                                 CurrentMode->Drive[i].DmaSpeed,
                                 HasUltraDMA ? UDMA_MODE(6) : MWDMA_MODE(2),
                                 &CycleTime,
                                 &BestMode);
        if (!Success || (BestMode < MWDMA_MODE(0)))
        {
            ULONG Modes;

            Modes = DevExt->TransferModeCurrentBitmap & ~(PIO_ALL | SWDMA_ALL);
            Modes &= DevExt->TransferModeAllowedMask;

            /* Search for the best enabled DMA mode */
            if (_BitScanReverse(&BestMode, Modes) && BestMode < RTL_NUMBER_OF(AtapTimingTable))
            {
                CycleTime = AtapTimingTable[BestMode];
            }
            else
            {
                CycleTime = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;
                BestMode = PIO_MODE(0);
            }
        }
        NewMode.Drive[i].DmaSpeed = CycleTime;
        DevExt->TransferModeSelectedBitmap |= 1 << BestMode;

        if (BestMode >= UDMA_MODE(0))
            NewMode.ModeFlags |= IDE_ACPI_TIMING_MODE_FLAG_UDMA << (2 * i);
        else
            NewMode.ModeFlags &= ~(IDE_ACPI_TIMING_MODE_FLAG_UDMA << (2 * i));
    }

    /* The underlying chipset might not allow the devices to be independently configured */
    if (!(NewMode.ModeFlags & IDE_ACPI_TIMING_MODE_FLAG_INDEPENDENT_TIMINGS) &&
        Devices[0] && Devices[1])
    {
        if (NewMode.Drive[0].DmaSpeed != IDE_ACPI_TIMING_MODE_NOT_SUPPORTED)
        {
            /* Disable DMA for drive 1 */
            NewMode.ModeFlags &= IDE_ACPI_TIMING_MODE_FLAG_UDMA2;
            NewMode.Drive[1].PioSpeed = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;
            NewMode.Drive[1].DmaSpeed = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;

            Devices[1]->TransferModeSelectedBitmap &= (PIO_MODE0 | PIO_MODE1 | PIO_MODE2);
        }
        else
        {
            /* Disable DMA for drive 0 */
            NewMode.ModeFlags &= IDE_ACPI_TIMING_MODE_FLAG_UDMA;
            NewMode.Drive[0].PioSpeed = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;
            NewMode.Drive[0].DmaSpeed = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;

            Devices[0]->TransferModeSelectedBitmap &= (PIO_MODE0 | PIO_MODE1 | PIO_MODE2);
        }
    }

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        if (Devices[i])
            AtaTimDumpTimingInfo(Devices[i]);
    }

    INFO("Old timings %-2lX 0:%-3ld 0:%-3ld 1:%-3ld 1:%-3ld\n",
         CurrentMode->ModeFlags,
         (LONG)CurrentMode->Drive[0].PioSpeed,
         (LONG)CurrentMode->Drive[0].DmaSpeed,
         (LONG)CurrentMode->Drive[1].PioSpeed,
         (LONG)CurrentMode->Drive[1].DmaSpeed);

    INFO("New timings %-2lX 0:%-3ld 0:%-3ld 1:%-3ld 1:%-3ld\n",
         NewMode.ModeFlags,
         (LONG)NewMode.Drive[0].PioSpeed,
         (LONG)NewMode.Drive[0].DmaSpeed,
         (LONG)NewMode.Drive[1].PioSpeed,
         (LONG)NewMode.Drive[1].DmaSpeed);

    AtaAcpiSetTimingMode(PortData->ChanExt,
                         &NewMode,
                         Devices[0] ? &(Devices[0]->IdentifyDeviceData) : NULL,
                         Devices[1] ? &(Devices[1]->IdentifyDeviceData) : NULL);

    AtaAcpiGetTimingMode(PortData->ChanExt, CurrentMode);
}

static
VOID
AtaTimSelectTimingsWithDriver(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PPCIIDE_INTERFACE PciIdeInterface = &PortData->Pata.PciIdeInterface;
    PATAPORT_DEVICE_EXTENSION Devices[MAX_IDE_DEVICE];
    PCIIDE_TRANSFER_MODE_SELECT XferMode;
    ULONG i;

    ASSERT(PciIdeInterface->ProgramTimingMode != NULL);

    RtlZeroMemory(&XferMode, sizeof(XferMode));
    XferMode.TransferModeTimingTable = (PULONG)AtapTimingTable;
    XferMode.TransferModeTableLength = RTL_NUMBER_OF(AtapTimingTable);

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;

        DevExt = AtaFdoFindDeviceByPath(PortData->ChanExt, AtaMarshallScsiAddress(0, i, 0));
        Devices[i] = DevExt;
        if (!DevExt)
            continue;

        RtlCopyMemory(&XferMode.IdentifyData[i],
                      &DevExt->IdentifyDeviceData,
                      sizeof(DevExt->IdentifyDeviceData));

        XferMode.DevicePresent[i] = TRUE;
        XferMode.FixedDisk[i] = !IS_ATAPI(&DevExt->Device);
        XferMode.IoReadySupported[i] = !!DevExt->IdentifyDeviceData.Capabilities.IordySupported;
        XferMode.UserChoiceTransferMode[i] = 0x7FFFFFFF; // Not used by chipset drivers anyway
        XferMode.BestPioCycleTime[i] = DevExt->PioCycleTime;
        XferMode.BestSwDmaCycleTime[i] = DevExt->SingleWordDmaCycleTime;
        XferMode.BestMwDmaCycleTime[i] = DevExt->MultiWordDmaCycleTime;
        XferMode.BestUDmaCycleTime[i] = DevExt->UltraDmaCycleTime;
        XferMode.DeviceTransferModeCurrent[i] = DevExt->TransferModeCurrentBitmap;
        XferMode.DeviceTransferModeSupported[i] = DevExt->TransferModeSupportedBitmap;
    }

    /* Call the chipset driver */
    PciIdeInterface->ProgramTimingMode(PciIdeInterface->DeviceObject->DeviceExtension, &XferMode);

    /* Save the result */
    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PATAPORT_DEVICE_EXTENSION DevExt = Devices[i];

        if (!DevExt)
            continue;

        DevExt->TransferModeSelectedBitmap = XferMode.DeviceTransferModeSelected[i];

        AtaTimDumpTimingInfo(DevExt);
    }
}

static
VOID
AtaPortSelectTimingsPata(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ASSERT(!IS_AHCI_EXT(PortData->ChanExt));

    if (AtaTimShouldUseAcpi(PortData))
    {
        /* Set the PATA transfer timings through ACPI */
        AtaTimSelectTimingsAcpi(PortData);
    }
    else
    {
        /* Let the chipset driver handle it */
        AtaTimSelectTimingsWithDriver(PortData);
    }
}

static
VOID
AtaTimSelectTimingsGeneric(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ULONG BestMode;

    /* Select PIO mode */
    if (_BitScanReverse(&BestMode, DevExt->TransferModeSupportedBitmap & PIO_ALL))
    {
        DevExt->TransferModeSelectedBitmap |= 1 << BestMode;
    }

    /* Select DMA mode for SATA. Legacy IDE devices do not support DMA */
    if (!IS_AHCI(&DevExt->Device))
        return;

    if (_BitScanReverse(&BestMode, DevExt->TransferModeSupportedBitmap & ~(PIO_ALL | SWDMA_ALL)))
    {
        DevExt->TransferModeSelectedBitmap |= 1 << BestMode;
    }
}

VOID
AtaPortUpdateTimingInformation(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ATA_SCSI_ADDRESS AtaScsiAddress;
    BOOLEAN DoGenericSelection;

    /* Default method for SATA or legacy IDE devices */
    DoGenericSelection = !IS_PCIIDE_EXT(PortData->ChanExt) ||
                         (PortData->ChanExt->Flags & CHANNEL_PIO_ONLY);

    AtaScsiAddress.AsULONG = 0;
    AtaScsiAddress.PathId = PortData->PortNumber;
    while (TRUE)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;

        DevExt = AtaFdoFindNextDeviceByPath(PortData->ChanExt, &AtaScsiAddress, FALSE, NULL);
        if (!DevExt)
            break;

        if (DevExt->Worker.Flags & DEV_WORKER_FLAG_REMOVED)
            continue;

        DevExt->TransferModeCurrentBitmap = 0;
        DevExt->TransferModeSupportedBitmap = 0;
        DevExt->TransferModeSelectedBitmap = 0;

        AtaTimInitPioMode(DevExt);
        AtaTimInitSingleWordDmaMode(DevExt);
        AtaTimInitMultiWordDmaMode(DevExt);
        AtaTimInitUltraDmaMode(DevExt);

        if (DoGenericSelection)
        {
            AtaTimSelectTimingsGeneric(DevExt);
            AtaTimDumpTimingInfo(DevExt);
        }
    }

    if (!DoGenericSelection)
        AtaPortSelectTimingsPata(PortData);
}
