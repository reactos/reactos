/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Generic PCI IDE controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

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

static const ATA_TIMING AtapTimingTable[] =
{
    /* PIO_MODE */
    { 70,  290, 240, 165, 150 }, // 0
    { 50,  290, 93,  125, 100 }, // 1
    { 30,  290, 40,  100, 90  }, // 2
    { 30,  80,  70,  80,  70  }, // 3
    { 25,  70,  25,  70,  25  }, // 4

    /* SWDMA_MODE */
    { 120, 0,   0,   480, 480 }, // 0
    { 90,  0,   0,   240, 240 }, // 1
    { 60,  0,   0,   120, 120 }, // 2

    /* MWDMA_MODE */
    { 60,  0,   0,   215, 215 }, // 0
    { 45,  0,   0,   80,  50  }, // 1
    { 25,  0,   0,   70,  25  }, // 2
};

/* FUNCTIONS ******************************************************************/

static
USHORT
AtaGetClocks(
    _In_ USHORT TimingValueNs,
    _In_ ULONG ClockPeriodPs)
{
    return ((TimingValueNs * 1000) + ClockPeriodPs - 1) / ClockPeriodPs;
}

static
VOID
AtaCalculateTimings(
    _In_ PCHANNEL_DEVICE_CONFIG Device,
    _Out_ PATA_TIMING Timing,
    _In_ ULONG Mode,
    _In_ ULONG ClockPeriodPs)
{
    USHORT DataCycleTime, CmdCycleTime;

    ASSERT(Mode < RTL_NUMBER_OF(AtapTimingTable));

    Timing->AddressSetup = AtaGetClocks(AtapTimingTable[Mode].AddressSetup, ClockPeriodPs);
    Timing->CmdActive    = AtaGetClocks(AtapTimingTable[Mode].CmdActive, ClockPeriodPs);
    Timing->CmdRecovery  = AtaGetClocks(AtapTimingTable[Mode].CmdRecovery, ClockPeriodPs);
    Timing->DataActive   = AtaGetClocks(AtapTimingTable[Mode].DataActive, ClockPeriodPs);
    Timing->DataRecovery = AtaGetClocks(AtapTimingTable[Mode].DataRecovery, ClockPeriodPs);

    if (Mode >= MWDMA_MODE(0))
        DataCycleTime = Device->MinMwDmaCycleTime;
    else if (Mode >= SWDMA_MODE(0))
        DataCycleTime = Device->MinSwDmaCycleTime;
    else
    {
        DataCycleTime = Device->MinPioCycleTime;

        /* The t0 for register transfer is the same except for mode 2 */
        if (Mode == PIO_MODE(2))
            CmdCycleTime = max(DataCycleTime, 330);
        else
            CmdCycleTime = max(DataCycleTime, AtapModeToCycleTime[Mode]);
    }
    DataCycleTime = max(DataCycleTime, AtapModeToCycleTime[Mode]);

    /*
     * The minimum total cycle time requirement t0
     * should be equal to or greater than the sum of t2 and t2i.
     */
    DataCycleTime = AtaGetClocks(DataCycleTime, ClockPeriodPs);
    if ((Timing->DataActive + Timing->DataRecovery) < DataCycleTime)
    {
        Timing->DataActive += (DataCycleTime - (Timing->DataActive + Timing->DataRecovery)) / 2;
        Timing->DataRecovery = DataCycleTime - Timing->DataActive;
    }
    if (Mode < SWDMA_MODE(0)) // PIO modes
    {
        CmdCycleTime = AtaGetClocks(CmdCycleTime, ClockPeriodPs);
        if ((Timing->CmdActive + Timing->CmdRecovery) < CmdCycleTime)
        {
            Timing->CmdActive += (CmdCycleTime - (Timing->CmdActive + Timing->CmdRecovery)) / 2;
            Timing->CmdRecovery = CmdCycleTime - Timing->CmdActive;
        }
    }
}

VOID
AtaSelectTimings(
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList,
    _Out_writes_all_(MAX_IDE_DEVICE) PATA_TIMING Timings,
    _In_range_(>, 0) ULONG ClockPeriodPs,
    _In_ ULONG Flags)
{
    ULONG i;

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        PATA_TIMING Timing = &Timings[i];
        ATA_TIMING DmaTiming;

        if (!Device)
        {
            RtlZeroMemory(Timing, sizeof(*Timing));
            continue;
        }

        /* PIO mode */
        AtaCalculateTimings(Device, Timing, Device->PioMode, ClockPeriodPs);

        /* UDMA works independently of any PIO mode */
        if (Device->DmaMode == PIO_MODE(0) || Device->DmaMode >= UDMA_MODE(0))
            continue;

        /* DMA mode */
        AtaCalculateTimings(Device, &DmaTiming, Device->DmaMode, ClockPeriodPs);

        /*
         * Typically, the ATA port driver uses PIO commands along with DMA ones.
         * Given that we program the chipset only once after device enumeration,
         * we want to make sure the PIO commands would always work.
         */
        Timing->AddressSetup = max(Timing->AddressSetup, DmaTiming.AddressSetup);
        Timing->CmdActive    = max(Timing->CmdActive,    DmaTiming.CmdActive);
        Timing->CmdRecovery  = max(Timing->CmdRecovery,  DmaTiming.CmdRecovery);
        Timing->DataActive   = max(Timing->DataActive,   DmaTiming.DataActive);
        Timing->DataRecovery = max(Timing->DataRecovery, DmaTiming.DataRecovery);
    }

    /* Merge timings shared between both drives */
    if (Flags & SHARED_ADDR_TIMINGS)
    {
        Timings[0].AddressSetup = max(Timings[0].AddressSetup, Timings[1].AddressSetup);

        Timings[1].AddressSetup = Timings[0].AddressSetup;
    }
    if (Flags & SHARED_CMD_TIMINGS)
    {
        Timings[0].CmdActive = max(Timings[0].CmdActive, Timings[1].CmdActive);
        Timings[0].CmdRecovery = max(Timings[0].CmdRecovery, Timings[1].CmdRecovery);

        Timings[1].CmdActive = Timings[0].CmdActive;
        Timings[1].CmdRecovery = Timings[0].CmdRecovery;
    }
    if (Flags & SHARED_DATA_TIMINGS)
    {
        Timings[0].DataActive = max(Timings[0].DataActive, Timings[1].DataActive);
        Timings[0].DataRecovery = max(Timings[0].DataRecovery, Timings[1].DataRecovery);

        Timings[1].DataActive = Timings[0].DataActive;
        Timings[1].DataRecovery = Timings[0].DataRecovery;
    }
}

VOID
SataSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    UNREFERENCED_PARAMETER(Controller);
    UNREFERENCED_PARAMETER(Channel);
    UNREFERENCED_PARAMETER(DeviceList);

    /*
     * Nothing to do here, just keep the selected Device->PioMode and Device->DmaMode
     * because SATA hardware snoops the SET FEATURES command.
     */
    NOTHING;
}

static
VOID
AtaAcpiFindModeForCycleTime(
    _In_ ULONG GtmCycleTime,
    _In_ ULONG SupportedModesBitmap,
    _In_ ULONG CurrentModesBitmap,
    _In_ ULONG MinimumMode,
    _In_ ULONG MaximumMode,
    _Out_ PULONG BestCycleTime,
    _Out_ PULONG BestMode)
{
    LONG i;

    if (GtmCycleTime == IDE_ACPI_TIMING_MODE_NOT_SUPPORTED)
    {
        *BestCycleTime = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;
        *BestMode = PIO_MODE(0);
        return;
    }

    for (i = MaximumMode; i >= MinimumMode; i--)
    {
        if ((SupportedModesBitmap & (1 << i)) && (AtapModeToCycleTime[i] >= GtmCycleTime))
        {
            *BestCycleTime = AtapModeToCycleTime[i];
            *BestMode = i;
            return;
        }
    }

    /*
     * This method can fail due to the first _GTM result
     * may return compatible timings for a device,
     * although the DMA mode is currently enabled by BIOS.
     * For instance, this has been observed on a VPC 2007 ACPI firmware:
     *   Drive[0].PioSpeed  900 ns
     *   Drive[0].DmaSpeed  900 ns
     * So we try to determine the transfer mode by other means below.
     */
    INFO("Failed to find mode for %ld ns cycle time from %lu in 0x%08lX\n",
          (LONG)GtmCycleTime,
          MaximumMode,
          SupportedModesBitmap);

    /* Search for the best enabled mode */
    if (_BitScanReverse(BestMode, CurrentModesBitmap))
    {
        ASSERT(*BestMode < RTL_NUMBER_OF(AtapModeToCycleTime));
        *BestCycleTime = AtapModeToCycleTime[*BestMode];
    }
    else
    {
        *BestCycleTime = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;
        *BestMode = PIO_MODE(0);
    }
}

static
VOID
PciIdeAcpiSetTransferMode(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    PIDE_ACPI_TIMING_MODE_BLOCK CurrentMode = &ChanData->CurrentTimingMode;
    IDE_ACPI_TIMING_MODE_BLOCK NewMode;
    NTSTATUS Status;
    ULONG i;

    ASSERT(ChanData->ChanInfo & CHANNEL_FLAG_HAS_ACPI_GTM);

    NewMode.ModeFlags = CurrentMode->ModeFlags;

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        BOOLEAN HasUltraDMA;

        if (!Device)
        {
            NewMode.Drive[i].PioSpeed = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;
            NewMode.Drive[i].DmaSpeed = IDE_ACPI_TIMING_MODE_NOT_SUPPORTED;
            continue;
        }

        /* PIO timing */
        AtaAcpiFindModeForCycleTime(CurrentMode->Drive[i].PioSpeed,
                                    Device->SupportedModes,
                                    Device->CurrentModes & PIO_ALL,
                                    PIO_MODE(0),
                                    PIO_MODE(4),
                                    &NewMode.Drive[i].PioSpeed,
                                    &Device->PioMode);

        /* DMA timing */
        HasUltraDMA = !!(CurrentMode->ModeFlags & (IDE_ACPI_TIMING_MODE_FLAG_UDMA(i)));
        AtaAcpiFindModeForCycleTime(CurrentMode->Drive[i].DmaSpeed,
                                    Device->SupportedModes,
                                    Device->CurrentModes & ~(PIO_ALL | SWDMA_ALL),
                                    MWDMA_MODE(0),
                                    HasUltraDMA ? UDMA_MODE(6) : MWDMA_MODE(2),
                                    &NewMode.Drive[i].DmaSpeed,
                                    &Device->DmaMode);

        if (Device->DmaMode >= UDMA_MODE(0))
            NewMode.ModeFlags |= IDE_ACPI_TIMING_MODE_FLAG_UDMA(i);
        else
            NewMode.ModeFlags &= ~IDE_ACPI_TIMING_MODE_FLAG_UDMA(i);
    }

    /*
     * The underlying chipset might not allow the devices to be independently configured.
     *
     * For example, the Intel PIIX (0x8086:0x1230) cannot specify separate device timings.
     * Please refer to PchSata.asi (PSIT bit)
     * from the edk2-platforms repository for an example ASL implementation.
     *
     * It is possible to snoop an ATA command by software and run _STM each time
     * the command being executed but an ACPI evaluation request can fail.
     * For this reason, the timing setup only needs to be done once.
     */
    if (!(NewMode.ModeFlags & IDE_ACPI_TIMING_MODE_FLAG_INDEPENDENT_TIMINGS) &&
        DeviceList[0] && DeviceList[1])
    {
        /* If we have a common timing mode for all devices use it, otherwise take the lower mode */
        if (NewMode.Drive[0].DmaSpeed != IDE_ACPI_TIMING_MODE_NOT_SUPPORTED &&
            NewMode.Drive[1].DmaSpeed != IDE_ACPI_TIMING_MODE_NOT_SUPPORTED)
        {
            NewMode.Drive[0].DmaSpeed = max(NewMode.Drive[0].DmaSpeed, NewMode.Drive[1].DmaSpeed);
            DeviceList[0]->DmaMode = min(DeviceList[0]->DmaMode, DeviceList[1]->DmaMode);

            NewMode.Drive[1].DmaSpeed = NewMode.Drive[0].DmaSpeed;
            DeviceList[1]->DmaMode = DeviceList[0]->DmaMode;
        }
        NewMode.Drive[0].PioSpeed = max(NewMode.Drive[0].PioSpeed, NewMode.Drive[1].PioSpeed);
        DeviceList[0]->PioMode = min(DeviceList[0]->PioMode, DeviceList[1]->PioMode);

        NewMode.Drive[1].PioSpeed = NewMode.Drive[0].PioSpeed;
        DeviceList[1]->PioMode = DeviceList[0]->PioMode;
    }

    INFO("CH %lu: Old timings %-2lX 0:%-3ld 0:%-3ld 1:%-3ld 1:%-3ld\n",
         ChanData->Channel,
         CurrentMode->ModeFlags,
         (LONG)CurrentMode->Drive[0].PioSpeed,
         (LONG)CurrentMode->Drive[0].DmaSpeed,
         (LONG)CurrentMode->Drive[1].PioSpeed,
         (LONG)CurrentMode->Drive[1].DmaSpeed);

    INFO("CH %lu: New timings %-2lX 0:%-3ld 0:%-3ld 1:%-3ld 1:%-3ld\n",
         ChanData->Channel,
         NewMode.ModeFlags,
         (LONG)NewMode.Drive[0].PioSpeed,
         (LONG)NewMode.Drive[0].DmaSpeed,
         (LONG)NewMode.Drive[1].PioSpeed,
         (LONG)NewMode.Drive[1].DmaSpeed);

    /* Evaluate _STM */
    Status = AtaAcpiSetTimingMode(ChanData->PdoExt->Common.Self,
                                  &NewMode,
                                  DeviceList[0] ? DeviceList[0]->IdentifyDeviceData : NULL,
                                  DeviceList[1] ? DeviceList[1]->IdentifyDeviceData : NULL);
    if (!NT_SUCCESS(Status))
    {
        /* ACPI request failed, fall back to PIO */
        for (i = 0; i < MAX_IDE_DEVICE; ++i)
        {
            PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];

            if (!Device)
                continue;

            Device->DmaMode = PIO_MODE(0);
            NT_VERIFY(_BitScanReverse(&Device->PioMode, Device->CurrentModes & PIO_ALL));
        }
        return;
    }

    /* Save the result */
    AtaAcpiGetTimingMode(ChanData->PdoExt->Common.Self, CurrentMode);
}

static
VOID
PciIdeBiosSetTransferMode(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    ULONG i;

    /* Update manually using BIOS boot settings */
    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        ULONG DmaFlags;

        if (!Device)
            continue;

        /* Get the PIO and DMA modes set by BIOS */
        if (!_BitScanReverse(&Device->DmaMode, Device->CurrentModes & ~PIO_ALL))
            Device->DmaMode = PIO_MODE(0);
        NT_VERIFY(_BitScanReverse(&Device->PioMode, Device->CurrentModes & PIO_ALL));

        if (i == 0)
            DmaFlags = CHANNEL_FLAG_DRIVE0_DMA_CAPABLE;
        else
            DmaFlags = CHANNEL_FLAG_DRIVE1_DMA_CAPABLE;

        /* Disable DMA when when it is not supported */
        if (!(ChanData->ChanInfo & DmaFlags))
            Device->DmaMode = PIO_MODE(0);
    }
}

static
VOID
PciIdeGenericSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    PCHANNEL_DATA_PATA ChanData = Controller->Channels[Channel];

    /* Set the PATA transfer timings through ACPI if possible */
    if (ChanData->ChanInfo & CHANNEL_FLAG_HAS_ACPI_GTM)
        PciIdeAcpiSetTransferMode(ChanData, DeviceList);
    else
        PciIdeBiosSetTransferMode(ChanData, DeviceList);
}

static
CODE_SEG("PAGE")
VOID
PataReleaseLegacyAddressRanges(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    PCONFIGURATION_INFORMATION ConfigInfo = IoGetConfigurationInformation();

    PAGED_CODE();

    if (ChanData->ChanInfo & CHANNEL_FLAG_PRIMARY_ADDRESS_CLAIMED)
        ConfigInfo->AtDiskPrimaryAddressClaimed = FALSE;
    if (ChanData->ChanInfo & CHANNEL_FLAG_SECONDARY_ADDRESS_CLAIMED)
        ConfigInfo->AtDiskSecondaryAddressClaimed = FALSE;

    ChanData->ChanInfo &= ~(CHANNEL_FLAG_PRIMARY_ADDRESS_CLAIMED |
                            CHANNEL_FLAG_SECONDARY_ADDRESS_CLAIMED);
}

static
CODE_SEG("PAGE")
VOID
PataClaimLegacyAddressRanges(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    PCONFIGURATION_INFORMATION ConfigInfo = IoGetConfigurationInformation();

    PAGED_CODE();

#if defined(_M_IX86)
    if (IsNEC_98)
    {
        /* On NEC PC-98 systems the legacy IDE interface may use up to four PDOs */
        if (ChanData->ChanInfo & CHANNEL_FLAG_CBUS)
        {
            ConfigInfo->AtDiskPrimaryAddressClaimed = TRUE;
            ConfigInfo->AtDiskSecondaryAddressClaimed = TRUE;

            ChanData->ChanInfo |= CHANNEL_FLAG_PRIMARY_ADDRESS_CLAIMED |
                                  CHANNEL_FLAG_SECONDARY_ADDRESS_CLAIMED;
        }
    }
    else
#endif
    if (ChanData->Regs.Data == UlongToPtr(PCIIDE_LEGACY_PRIMARY_COMMAND_BASE))
    {
        ConfigInfo->AtDiskPrimaryAddressClaimed = TRUE;
        ChanData->ChanInfo |= CHANNEL_FLAG_PRIMARY_ADDRESS_CLAIMED;
    }
    else if (ChanData->Regs.Data == UlongToPtr(PCIIDE_LEGACY_SECONDARY_COMMAND_BASE))
    {
        ConfigInfo->AtDiskSecondaryAddressClaimed = TRUE;
        ChanData->ChanInfo |= CHANNEL_FLAG_SECONDARY_ADDRESS_CLAIMED;
    }
}

static
CODE_SEG("PAGE")
VOID
PciIdeFreeMemory(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    PDMA_ADAPTER DmaAdapter = ChanData->DmaAdapter;
    PDMA_OPERATIONS DmaOperations = ChanData->DmaAdapter->DmaOperations;
    PHYSICAL_ADDRESS PrdTablePhysicalAddress;

    PAGED_CODE();

    if (!ChanData->PrdTable)
        return;

    PrdTablePhysicalAddress.QuadPart = ChanData->PrdTablePhysicalAddress;

    DmaOperations->FreeCommonBuffer(DmaAdapter,
                                    sizeof(*ChanData->PrdTable) * ChanData->MaximumPhysicalPages,
                                    PrdTablePhysicalAddress,
                                    ChanData->PrdTable,
                                    TRUE); // Cached
    ChanData->PrdTable = NULL;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeAllocateMemory(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    PDMA_OPERATIONS DmaOperations = ChanData->DmaAdapter->DmaOperations;
    PHYSICAL_ADDRESS LogicalAddress;
    ULONG BlockSize;

    PAGED_CODE();

    BlockSize = sizeof(*ChanData->PrdTable) * ChanData->MaximumPhysicalPages;
    ChanData->PrdTable = DmaOperations->AllocateCommonBuffer(ChanData->DmaAdapter,
                                                             BlockSize,
                                                             &LogicalAddress,
                                                             TRUE); // Cached
    if (!ChanData->PrdTable)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(ChanData->PrdTable, BlockSize);

    ChanData->PrdTablePhysicalAddress = LogicalAddress.LowPart;

    /* 32-bit DMA */
    ASSERT(LogicalAddress.HighPart == 0);

    /* The descriptor table must be 4 byte aligned */
    ASSERT((LogicalAddress.LowPart % sizeof(ULONG)) == 0);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
BOOLEAN
PciIdeIsDmaStatusValid(
    _In_ UCHAR DmaStatus)
{
    PAGED_CODE();

    /* The status bits [3:4] must return 0 on reads */
    if (DmaStatus & (PCIIDE_DMA_STATUS_RESERVED1 | PCIIDE_DMA_STATUS_RESERVED2))
    {
        /*
         * Not a PCI IDE DMA device. This may happen when the channel is disabled
         * (e.g. 8086:8C00 returns 0xFF for disabled SATA channels).
         */
        return FALSE;
    }

    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
PciIdeControllerIsSimplex(
    _In_ PUCHAR IoBase)
{
    UCHAR DmaStatus;

    PAGED_CODE();

    DmaStatus = ATA_READ(IoBase + PCIIDE_DMA_STATUS);
    if (!PciIdeIsDmaStatusValid(DmaStatus))
        return FALSE;

    return !!(DmaStatus & PCIIDE_DMA_STATUS_SIMPLEX);
}

static
CODE_SEG("PAGE")
BOOLEAN
PciIdeAssignDmaResources(
    _In_ PATA_CONTROLLER Controller,
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    PUCHAR IoBase;
    UCHAR DmaStatus;

    PAGED_CODE();

    /* Check if BAR4 is available */
    IoBase = AtaCtrlPciGetBar(Controller, PCIIDE_DMA_IO_BAR, PCIIDE_DMA_IO_RANGE_LENGTH);
    if (!IoBase)
    {
        if (Controller->AccessRange[PCIIDE_DMA_IO_BAR].Flags & RANGE_IS_VALID)
            ERR("CH %lu: Failed to map bus master registers", ChanData->Channel);
        else
            INFO("CH %lu: No bus master registers", ChanData->Channel);
        return FALSE;
    }

    /* We look at the primary channel status register to determine the simplex mode */
    if (!(Controller->Flags & CTRL_FLAG_IS_SIMPLEX) && (Controller->MaxChannels > 1) &&
        PciIdeControllerIsSimplex(IoBase))
    {
        Controller->Flags |= CTRL_FLAG_IS_SIMPLEX;
    }

    if (!IS_PRIMARY_CHANNEL(ChanData))
        IoBase += PCIIDE_DMA_SECONDARY_CHANNEL_OFFSET;

    /*
     * Save the DMA I/O address in any case. Some PATA controllers (Intel ICH)
     * assert the DMA interrupt even if the current command is a PIO command,
     * so we have to always clear the DMA interrupt.
     */
    ChanData->Regs.Dma = IoBase;

    /* This channel does not support DMA */
    if (!(ChanData->Actual.TransferModeSupported & ~PIO_ALL))
        return FALSE;

    /* Verify DMA status */
    DmaStatus = ATA_READ(IoBase + PCIIDE_DMA_STATUS);
    if (!PciIdeIsDmaStatusValid(DmaStatus))
    {
        WARN("CH %lu: I/O Base %p, DMA status 0x%02X\n", ChanData->Channel, IoBase, DmaStatus);
        return FALSE;
    }
    else
    {
        INFO("CH %lu: I/O Base %p, DMA status 0x%02X\n", ChanData->Channel, IoBase, DmaStatus);
    }

    /* The status bits 5:6 are set by the BIOS firmware at boot */
    if (DmaStatus & PCIIDE_DMA_STATUS_DRIVE0_DMA_CAPABLE)
        ChanData->ChanInfo |= CHANNEL_FLAG_DRIVE0_DMA_CAPABLE;
    if (DmaStatus & PCIIDE_DMA_STATUS_DRIVE1_DMA_CAPABLE)
        ChanData->ChanInfo |= CHANNEL_FLAG_DRIVE1_DMA_CAPABLE;

    /* The hardware can use DMA */
    return TRUE;
}

CODE_SEG("PAGE")
VOID
PciIdeInitTaskFileIoResources(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ ULONG_PTR CommandPortBase,
    _In_ ULONG_PTR ControlPortBase,
    _In_ ULONG CommandBlockSpare)
{
    PIDE_REGISTERS Registers = &ChanData->Regs;

    PAGED_CODE();

    /* Standard PATA register layout */
    Registers->Data        = (PVOID)(CommandPortBase + 0 * CommandBlockSpare);
    Registers->Error       = (PVOID)(CommandPortBase + 1 * CommandBlockSpare);
    Registers->SectorCount = (PVOID)(CommandPortBase + 2 * CommandBlockSpare);
    Registers->LbaLow      = (PVOID)(CommandPortBase + 3 * CommandBlockSpare);
    Registers->LbaMid      = (PVOID)(CommandPortBase + 4 * CommandBlockSpare);
    Registers->LbaHigh     = (PVOID)(CommandPortBase + 5 * CommandBlockSpare);
    Registers->Device      = (PVOID)(CommandPortBase + 6 * CommandBlockSpare);
    Registers->Status      = (PVOID)(CommandPortBase + 7 * CommandBlockSpare);

    Registers->Control     = (PVOID)ControlPortBase;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeAssignNativeResources(
    _In_ PATA_CONTROLLER Controller,
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    PVOID CommandPortBase, ControlPortBase;
    const ULONG BarIndex = ChanData->Channel * 2;

    PAGED_CODE();

    CommandPortBase = AtaCtrlPciGetBar(Controller, BarIndex, PCIIDE_COMMAND_IO_RANGE_LENGTH);
    if (!CommandPortBase)
        return STATUS_INSUFFICIENT_RESOURCES;

    ControlPortBase = AtaCtrlPciGetBar(Controller, BarIndex + 1, PCIIDE_CONTROL_IO_RANGE_LENGTH);
    if (!ControlPortBase)
        return STATUS_INSUFFICIENT_RESOURCES;

    PciIdeInitTaskFileIoResources(ChanData,
                                  (ULONG_PTR)CommandPortBase,
                                  (ULONG_PTR)ControlPortBase + PCIIDE_CONTROL_IO_BAR_OFFSET,
                                  1);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeAssignLegacyResources(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CommandPortDesc = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ControlPortDesc = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDesc = NULL;
    ULONG i;

    PAGED_CODE();

    if (!ResourcesTranslated)
        return STATUS_INSUFFICIENT_RESOURCES;

    for (i = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc;

        Desc = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        switch (Desc->Type)
        {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
            {
                if (Desc->u.Port.Length == PCIIDE_LEGACY_CONTROL_IO_RANGE_LENGTH)
                {
                    if (!ControlPortDesc)
                        ControlPortDesc = Desc;
                }
                else if (Desc->u.Port.Length == PCIIDE_LEGACY_COMMAND_IO_RANGE_LENGTH)
                {
                    if (!CommandPortDesc)
                        CommandPortDesc = Desc;
                }

                break;
            }

            case CmResourceTypeInterrupt:
            {
                if (!InterruptDesc)
                    InterruptDesc = Desc;
                break;
            }

            default:
                break;
        }
    }

    if (!CommandPortDesc || !ControlPortDesc || !InterruptDesc)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    RtlCopyMemory(&ChanData->InterruptDesc, InterruptDesc, sizeof(*InterruptDesc));

    PciIdeInitTaskFileIoResources(ChanData,
                                  (ULONG_PTR)CommandPortDesc->u.Port.Start.QuadPart,
                                  (ULONG_PTR)ControlPortDesc->u.Port.Start.QuadPart,
                                  1);

    return STATUS_SUCCESS;
}

/*
 * PCI IDE Notes:
 *
 * When the PCI IDE controller operates in compatibility mode,
 * the pciidex driver assigns a list of boot resources to each IDE channel.
 *
 *                                        _GTM, _STM                            _GTF
 * PCI0-->IDE0---------------------------+-->CHN0---------------------------+-->DRV0
 *        IO:  Start 0:FFA0, Len 10      |   IO:  Start 0:1F0, Len 8        |
 *                                       |   IO:  Start 0:3F6, Len 1        \-->DRV1
 *                                       |   INT: Lev A Vec A Aff FFFFFFFF
 *                                       |
 *                                       \-->CHN1---------------------------+-->DRV0
 *                                           IO:  Start 0:170, Len 8        |
 *                                           IO:  Start 0:376, Len 1        \-->DRV1
 *                                           INT: Lvl F Vec F Aff FFFFFFFF
 *
 * In native-PCI mode, the PCI IDE controller acts as a true PCI device
 * and no resources are assigned to the channel device object at all.
 *
 *                                        _GTM, _STM                            _GTF
 * PCI0-->IDE0---------------------------+-->CHN0---------------------------+-->DRV0
 *        IO:  Start 0:FFC0, Len 8       |                                  |
 *        IO:  Start 0:FF8C, Len 4       |                                  \-->DRV1
 *        IO:  Start 0:FF80, Len 8       |
 *        IO:  Start 0:FF88, Len 4       |
 *        IO:  Start 0:FFA0, Len 10      \-->CHN1---------------------------+-->DRV0
 *        INT: Lvl B Vec B Aff FFFFFFFF                                     |
 *                                                                          \-->DRV1
 *
 * Note that the NT architecture does not support switching only one IDE channel to native mode.
 */
static
CODE_SEG("PAGE")
NTSTATUS
PciIdeParseResources(
    _In_ PVOID ChannelContext,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    PATA_CONTROLLER Controller = ChanData->Controller;
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(ChanData->Channel < 2);

    if (Controller->Flags & CTRL_FLAG_IN_LEGACY_MODE)
        Status = PciIdeAssignLegacyResources(ChanData, ResourcesTranslated);
    else
        Status = PciIdeAssignNativeResources(Controller, ChanData);
    if (!NT_SUCCESS(Status))
    {
        ERR("CH %lu: Failed to assign I/O resources 0x%lx\n", ChanData->Channel, Status);
        return Status;
    }

    if (!PciIdeAssignDmaResources(Controller, ChanData))
    {
        /* Fall back to PIO */
        ChanData->Actual.TransferModeSupported &= PIO_ALL;
        INFO("CH %lu: Non DMA channel\n", ChanData->Channel);
    }

    PataClaimLegacyAddressRanges(ChanData);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
PciIdeFreeResources(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;

    PAGED_CODE();

    if (ChanData->InterruptObject)
    {
        AtaChanEnableInterruptsSync(ChanData, FALSE);

        IoDisconnectInterrupt(ChanData->InterruptObject);
        ChanData->InterruptObject = NULL;
    }

    ChanData->Regs.Dma = NULL;

    ChanData->ChanInfo &= ~(CHANNEL_FLAG_DRIVE0_DMA_CAPABLE |
                            CHANNEL_FLAG_DRIVE1_DMA_CAPABLE);

    PataReleaseLegacyAddressRanges(ChanData);
}

CODE_SEG("PAGE")
IDE_CHANNEL_STATE
PciIdeGetChannelState(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel)
{
    PCHANNEL_DATA_PATA ChanData = Controller->Channels[Channel];

    PAGED_CODE();

    /* SFF-8038i compliant hardware */
    if (ChanData->ParseResources == PciIdeParseResources)
    {
        /*
         * If any native PCI IDE channel is enabled,
         * its PCI I/O resources should be allocated as well:
         *
         * [BAR 0] I/O ports at FFC0 [size=8]     Channel 0 task file registers
         * [BAR 1] I/O ports at FF8C [size=4]     Channel 0 device control
         * [BAR 2] I/O ports at FF80 [size=8]     Channel 1 task file registers
         * [BAR 3] I/O ports at FF88 [size=4]     Channel 1 device control
         * [BAR 4] I/O ports at FFA0 [size=16]    Bus Mastering registers
         * [BAR 5] Memory at FE780000             Aux registers
         */
        if (!(Controller->Flags & CTRL_FLAG_IN_LEGACY_MODE))
        {
            const ULONG BarIndex = Channel * 2;

            ASSERT(Channel < 2);

            /* Check task file registers */
            if (!(Controller->AccessRange[BarIndex].Flags & RANGE_IS_VALID))
                return ChannelDisabled;

            /* Check device control */
            if (!(Controller->AccessRange[BarIndex + 1].Flags & RANGE_IS_VALID))
                return ChannelDisabled;
        }
    }

    if (Controller->ChannelEnableBits)
    {
        if (Controller->Flags & CTRL_FLAG_USE_TEST_FUNCTION)
        {
            /* Device-specific test */
            return Controller->ChannelEnabledTest(Controller, Channel);
        }
        else
        {
            const ATA_PCI_ENABLE_BITS* EnableBits = &Controller->ChannelEnableBits[Channel];
            UCHAR RegisterValue;

            /* Test PCI bits */
            RegisterValue = PciRead8(Controller, EnableBits->Register);
            if ((RegisterValue & EnableBits->Mask) == EnableBits->ValueEnabled)
                return ChannelEnabled;
            else
                return ChannelDisabled;
        }
    }

    /* This channel is always enabled */
    return ChannelStateUnknown;
}

static
VOID
PataEnableInterrupts(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN Enable)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    UCHAR Control;

    INFO("CH %lu: %sable interrupts\n", ChanData->Channel, Enable ? "En" : "Dis");

    Control = IDE_DC_ALWAYS;
    if (Enable)
        Control |= IDE_DC_REENABLE_CONTROLLER;
    else
        Control |= IDE_DC_DISABLE_INTERRUPTS;

#if defined(_M_IX86)
    if (ChanData->ChanInfo & CHANNEL_FLAG_CBUS)
    {
        ATA_SELECT_BANK(1);
        ATA_WRITE(ChanData->Regs.Control, Control);

        /* Clear interrupts */
        if (ChanData->CheckInterrupt)
            ChanData->CheckInterrupt(ChanData);
        ChanData->ReadStatus(ChanData);

        ATA_SELECT_BANK(0);
        ChanData->LastAtaBankId = 0xFF;
    }
#endif
    ATA_WRITE(ChanData->Regs.Control, Control);

    /* Clear interrupts */
    if (ChanData->CheckInterrupt)
        ChanData->CheckInterrupt(ChanData);
    ChanData->ReadStatus(ChanData);

    if (ChanData->Regs.Dma != NULL)
    {
        ATA_WRITE(ChanData->Regs.Dma + PCIIDE_DMA_STATUS,
                  ATA_READ(ChanData->Regs.Dma + PCIIDE_DMA_STATUS));
    }
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeConnectInterrupt(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    PATA_CONTROLLER Controller = ChanData->Controller;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDesc;
    PKSERVICE_ROUTINE IsrHandler;
    NTSTATUS Status;

    PAGED_CODE();

    if (Controller->Flags & CTRL_FLAG_IN_LEGACY_MODE)
        InterruptDesc = &ChanData->InterruptDesc;
    else
        InterruptDesc = &Controller->InterruptDesc;
    ASSERT(InterruptDesc);

    if (ChanData->Regs.Dma != NULL)
        IsrHandler = PciIdeChannelIsr;
    else
        IsrHandler = PataChannelIsr;
    Status = IoConnectInterrupt(&ChanData->InterruptObject,
                                IsrHandler,
                                ChanData,
                                NULL,
                                InterruptDesc->u.Interrupt.Vector,
                                InterruptDesc->u.Interrupt.Level,
                                InterruptDesc->u.Interrupt.Level,
                                (InterruptDesc->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
                                ? Latched
                                : LevelSensitive,
                                (InterruptDesc->ShareDisposition == CmResourceShareShared),
                                InterruptDesc->u.Interrupt.Affinity,
                                FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not connect to interrupt %lu, status 0x%lx\n",
            InterruptDesc->u.Interrupt.Vector, Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeAttachChannel(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN Attach)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;

    PAGED_CODE();

    AtaChanEnableInterruptsSync(ChanData, Attach);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeCreateChannelData(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG HwExtensionSize)
{
    PCHANNEL_DATA_PATA ChanData;
    ULONG i, Size;

    PAGED_CODE();
    ASSERT(Controller->MaxChannels != 0);

    Size = FIELD_OFFSET(CHANNEL_DATA_PATA, HwExt) + HwExtensionSize;
    ChanData = ExAllocatePoolZero(NonPagedPool, Size * Controller->MaxChannels, TAG_PCIIDEX);
    if (!ChanData)
        return STATUS_INSUFFICIENT_RESOURCES;

    Controller->ChanDataBlock = ChanData;
    Controller->ChannelBitmap = NUM_TO_BITMAP(Controller->MaxChannels);

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        Controller->Channels[i] = ChanData;

        ChanData->Channel = i;
        ChanData->Controller = Controller;

        /* Set default values */
        ChanData->ChanInfo = CHANNEL_FLAG_IO32;
        ChanData->ParseResources = PciIdeParseResources;
        ChanData->FreeResources = PciIdeFreeResources;
        ChanData->AllocateMemory = PciIdeAllocateMemory;
        ChanData->FreeMemory = PciIdeFreeMemory;
        ChanData->EnableInterrupts = PataEnableInterrupts;
        ChanData->PreparePrdTable = PciIdePreparePrdTable;
        ChanData->PrepareIo = PataPrepareIo;
        ChanData->StartIo = PataStartIo;
        ChanData->LoadTaskFile = PataLoadTaskFile;
        ChanData->SaveTaskFile = PataSaveTaskFile;
        ChanData->ReadStatus = PataReadStatus;
        ChanData->SetTransferMode = PciIdeGenericSetTransferMode;
        ChanData->TransferModeSupported = PIO_ALL | SWDMA_ALL | MWDMA_ALL | UDMA_ALL;

        KeInitializeDpc(&ChanData->PollingTimerDpc, PataPollingTimerDpc, ChanData);

        ChanData = (PVOID)((ULONG_PTR)ChanData + Size);
    }

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i;

    PAGED_CODE();

    /* Match the controller through the PCI ID */
    switch (Controller->Pci.VendorID)
    {
        case PCI_VEN_ATI:
            Status = AtiGetControllerProperties(Controller);
            break;
        case PCI_VEN_AMD:
        case PCI_VEN_NVIDIA:
            Status = AmdGetControllerProperties(Controller);
            break;
        case PCI_VEN_CMD:
            Status = CmdGetControllerProperties(Controller);
            break;
        case PCI_VEN_INTEL:
            Status = IntelGetControllerProperties(Controller);
            break;
        case PCI_VEN_PC_TECH:
            Status = PcTechGetControllerProperties(Controller);
            break;
        case PCI_VEN_SERVERWORKS:
            Status = SvwGetControllerProperties(Controller);
            break;
        case PCI_VEN_TOSHIBA:
            Status = ToshibaGetControllerProperties(Controller);
            break;
        case PCI_VEN_VIA:
            Status = ViaGetControllerProperties(Controller);
            break;

        default:
            Status = STATUS_NO_MATCH;
            break;
    }
    if (NT_SUCCESS(Status))
        return Status;

    if (Status != STATUS_NO_MATCH)
        return Status;

    /* Check for generic PCI IDE controller */
    if ((Controller->Pci.BaseClass != PCI_CLASS_MASS_STORAGE_CTLR) ||
        ((Controller->Pci.SubClass != PCI_SUBCLASS_MSC_IDE_CTLR) &&
         (Controller->Pci.SubClass != PCI_SUBCLASS_MSC_RAID_CTLR)))
    {
        ERR("Unknown controller\n");
        return Status;
    }

    WARN("%04X:%04X.%02X: Using generic PCI IDE minidriver\n",
         Controller->Pci.VendorID,
         Controller->Pci.DeviceID,
         Controller->Pci.RevisionID);

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        ChanData->ChanInfo &= ~CHANNEL_FLAG_IO32;
    }

    return STATUS_SUCCESS;
}
