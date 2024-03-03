/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     ServerWorks/Broadcom ATA controller minidriver
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * Adapted from the FreeBSD ata-serverworks driver
 * Copyright (c) 1998-2008 Søren Schmidt <sos@FreeBSD.org>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_OSB4_IDE                    0x0211
#define PCI_DEV_CSB5_IDE                    0x0212
#define PCI_DEV_CSB6_IDE                    0x0213
#define PCI_DEV_CSB6_IDE_THIRD              0x0217
#define PCI_DEV_HT1000_IDE                  0x0214

#define PCI_DEV_OSB4_BRIDGE                 0x0210

#define SVW_REG_PIO_TIMING                  0x40
#define SVW_REG_DMA_TIMING                  0x44
#define SVW_REG_PIO_MODE                    0x4A // CSB5 and later
#define SVW_REG_UDMA_ENABLE                 0x54
#define SVW_REG_UDMA_MODE                   0x56
#define SVW_REG_UDMA_CONTROL                0x5A // Absent on ATI

#define SVW_UDMA_CTRL_MODE_MASK             0x03
#define SVW_UDMA_CTRL_DISABLE               0x40

#define SVW_UDMA_CTRL_MODE_UDMA4            0x02
#define SVW_UDMA_CTRL_MODE_UDMA5            0x03

static const struct
{
    UCHAR Value;
    ULONG CycleTime;
} SvwPioTimings[] =
{
    { 0x5D, 600 }, // 0
    { 0x47, 390 }, // 1
    { 0x34, 270 }, // 2
    { 0x22, 180 }, // 3
    { 0x20, 120 }, // 4
};

static const struct
{
    UCHAR Value;
    ULONG CycleTime;
} SvwMwDmaTimings[] =
{
    { 0x77, 480 }, // 0
    { 0x21, 150 }, // 1
    { 0x20, 120 }, // 2
};

/* FUNCTIONS ******************************************************************/

static
VOID
SvwChooseDeviceSpeed(
    _In_ ULONG Channel,
    _In_ PCHANNEL_DEVICE_CONFIG Device)
{
    ULONG Mode;

    /* PIO speed */
    for (Mode = Device->PioMode; Mode > PIO_MODE(0); Mode--)
    {
        if (!(Device->SupportedModes & (1 << Mode)))
            continue;

        if (SvwPioTimings[Mode].CycleTime >= Device->MinPioCycleTime)
            break;
    }
    if (Mode != Device->PioMode)
        INFO("CH %lu: Downgrade PIO speed from %lu to %lu\n", Channel, Device->PioMode, Mode);
    Device->PioMode = Mode;

    /* UDMA works independently of any PIO mode */
    if (Device->DmaMode == PIO_MODE(0) || Device->DmaMode >= UDMA_MODE(0))
        return;

    /* DMA speed */
    for (Mode = Device->DmaMode; Mode > PIO_MODE(0); Mode--)
    {
        if (!(Device->SupportedModes & ~PIO_ALL & (1 << Mode)))
            continue;

        ASSERT((1 << Mode) & MWDMA_ALL);

        if (SvwMwDmaTimings[Mode - MWDMA_MODE(0)].CycleTime >= Device->MinMwDmaCycleTime)
            break;
    }
    if (Mode != Device->DmaMode)
    {
        if (Mode == PIO_MODE(0))
            WARN("CH %lu: Too slow device '%s', disabling DMA\n", Channel, Device->FriendlyName);
        else
            INFO("CH %lu: Downgrade DMA speed from %lu to %lu\n", Channel, Device->DmaMode, Mode);
    }
    Device->DmaMode = Mode;
}

VOID
SvwSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    USHORT PioModeReg, UdmaModeReg;
    UCHAR UdmaEnableReg;
    ULONG i, PioTimReg, DmaTimReg;

    if (Controller->Pci.DeviceID != PCI_DEV_OSB4_IDE)
        PioModeReg = PciRead16(Controller, SVW_REG_PIO_MODE);
    else
        PioModeReg = 0;

    PioTimReg = PciRead32(Controller, SVW_REG_PIO_TIMING);
    DmaTimReg = PciRead32(Controller, SVW_REG_DMA_TIMING);
    UdmaEnableReg = PciRead8(Controller, SVW_REG_UDMA_ENABLE);
    UdmaModeReg = PciRead16(Controller, SVW_REG_UDMA_MODE);

    INFO("CH %lu: Config (before)\n"
         "DMA Timing   %08lX\n"
         "PIO Timing   %08lX\n"
         "PIO Mode         %04X\n"
         "UDMA Mode        %04X\n",
         "UDMA Enable        %02X\n",
         Channel,
         DmaTimReg,
         PioTimReg,
         PioModeReg,
         UdmaModeReg,
         UdmaEnableReg);

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        const ULONG DeviceIndex = (Channel << 1) + i; // 0   1   2   3
        const ULONG Shift = (DeviceIndex ^ 1) << 3;   // 8   0   24  16

        UdmaModeReg &= ~(0x0F << (DeviceIndex << 2));
        UdmaEnableReg &= ~(0x01 << DeviceIndex);

        if (!Device)
            continue;

        /* Make sure that the selected PIO and DMA speeds match the cycle time */
        SvwChooseDeviceSpeed(Channel, Device);

        /* UDMA timings */
        if (Device->DmaMode >= UDMA_MODE(0))
        {
            ULONG ModeIndex = Device->DmaMode - UDMA_MODE(0);

            UdmaModeReg |= ModeIndex << (DeviceIndex << 2);
            UdmaEnableReg |= 1 << DeviceIndex;

            DmaTimReg &= ~(0xFF << Shift);
            DmaTimReg |= SvwMwDmaTimings[2].Value << Shift;
        }
        /* DMA timings */
        else if (Device->DmaMode != PIO_MODE(0))
        {
            ULONG ModeIndex = Device->DmaMode - MWDMA_MODE(0);

            DmaTimReg &= ~(0xFF << Shift);
            DmaTimReg |= SvwMwDmaTimings[ModeIndex].Value << Shift;
        }

        /* PIO timings */
        PioModeReg &= ~(0x0F << (DeviceIndex << 2));
        PioModeReg |= Device->PioMode << (DeviceIndex << 2);

        PioTimReg &= ~(0xFF << Shift);
        PioTimReg |= SvwPioTimings[Device->PioMode].Value << Shift;
    }

    PciWrite32(Controller, SVW_REG_PIO_TIMING, PioTimReg);

    if (Controller->Pci.DeviceID != PCI_DEV_OSB4_IDE)
        PciWrite16(Controller, SVW_REG_PIO_MODE, PioModeReg);

    PciWrite32(Controller, SVW_REG_DMA_TIMING, DmaTimReg);
    PciWrite16(Controller, SVW_REG_UDMA_MODE, UdmaModeReg);
    PciWrite8(Controller, SVW_REG_UDMA_ENABLE, UdmaEnableReg);

    INFO("CH %lu: Config (after)\n"
         "DMA Timing   %08lX\n"
         "PIO Timing   %08lX\n"
         "PIO Mode         %04X\n"
         "UDMA Mode        %04X\n",
         "UDMA Enable        %02X\n",
         Channel,
         DmaTimReg,
         PioTimReg,
         PioModeReg,
         UdmaModeReg,
         UdmaEnableReg);
}

CODE_SEG("PAGE")
BOOLEAN
SvwHasUdmaCable(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel)
{
    UCHAR UdmaModeReg;

    PAGED_CODE();

    UdmaModeReg = PciRead8(Controller, SVW_REG_UDMA_MODE + Channel);

    /* Check mode settings, see if UDMA3 or higher mode is active */
    return ((UdmaModeReg & 0x07) > 2) || ((UdmaModeReg & 0x70) > 0x20);
}

static
BOOLEAN
SvwOsb4PciBridgeInit(
    _In_ PVOID Context,
    _In_ ULONG BusNumber,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_ PPCI_COMMON_HEADER PciConfig)
{
    ULONG Value;

    UNREFERENCED_PARAMETER(Context);

    if (PciConfig->VendorID != PCI_VEN_SERVERWORKS ||
        PciConfig->DeviceID != PCI_DEV_OSB4_BRIDGE)
    {
        return FALSE;
    }

    HalGetBusDataByOffset(PCIConfiguration,
                          BusNumber,
                          PciSlot.u.AsULONG,
                          &Value,
                          0x64,
                          sizeof(Value));
    Value &= ~0x00002000; // Disable 600ns interrupt mask
    Value |= 0x00004000; // Enable UDMA/33 support
    HalSetBusDataByOffset(PCIConfiguration,
                          BusNumber,
                          PciSlot.u.AsULONG,
                          &Value,
                          0x64,
                          sizeof(Value));
    return TRUE;
}

static
VOID
SvwPataControllerStart(
    _In_ PATA_CONTROLLER Controller)
{
    if (Controller->Pci.DeviceID == PCI_DEV_OSB4_IDE)
    {
        PciFindDevice(SvwOsb4PciBridgeInit, NULL);
    }
    else
    {
        PCHANNEL_DATA_COMMON ChanData = Controller->Channels[0];
        UCHAR UdmaCtrlReg = PciRead8(Controller, SVW_REG_UDMA_CONTROL);

        UdmaCtrlReg &= ~(SVW_UDMA_CTRL_DISABLE | SVW_UDMA_CTRL_MODE_MASK);
        if (ChanData->TransferModeSupported & UDMA_MODE5)
            UdmaCtrlReg |= SVW_UDMA_CTRL_MODE_UDMA5;
        else
            UdmaCtrlReg |= SVW_UDMA_CTRL_MODE_UDMA4;

        PciWrite8(Controller, SVW_REG_UDMA_CONTROL, UdmaCtrlReg);
    }
}

CODE_SEG("PAGE")
NTSTATUS
SvwPataGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i, SupportedMode;
    BOOLEAN No64KDma = FALSE;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_SERVERWORKS);

    switch (Controller->Pci.DeviceID)
    {
        case PCI_DEV_OSB4_IDE:
            /* UDMA is supported but its use is discouraged */
            SupportedMode = PIO_ALL | MWDMA_ALL;

            No64KDma = TRUE;
            break;

        case PCI_DEV_CSB5_IDE:
            if (Controller->Pci.RevisionID < 0x92)
                SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 4);
            else
                SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 5);
            Controller->Flags |= CTRL_FLAG_DMA_INTERRUPT;
            break;

        case PCI_DEV_CSB6_IDE_THIRD:
            SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 4);
            Controller->MaxChannels = 1;
            break;

        case PCI_DEV_CSB6_IDE:
            SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 5);
            break;

        case PCI_DEV_HT1000_IDE:
            SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 5);
            Controller->MaxChannels = 1;
            No64KDma = TRUE;
            break;

        default:
            return STATUS_NO_MATCH;
    }

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    Controller->Start = SvwPataControllerStart;

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        if (No64KDma)
            ChanData->MaximumTransferLength = PCIIDE_PRD_LIMIT - 0x4000;

        ChanData->SetTransferMode = SvwSetTransferMode;
        ChanData->TransferModeSupported = SupportedMode;

        /* Check for 80-conductor cable */
        if ((ChanData->TransferModeSupported & UDMA_80C_ALL) && !SvwHasUdmaCable(Controller, i))
        {
            INFO("CH %lu: BIOS hasn't selected mode faster than UDMA 2, "
                 "assume 40-conductor cable\n",
                 ChanData->Channel);
            ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
        }
    }

    return STATUS_SUCCESS;
}
