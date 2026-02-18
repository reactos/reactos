/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Toshiba PCI IDE controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * Adapted from the NetBSD toshide driver
 * Copyright (c) 2009 The NetBSD Foundation, Inc
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_PICCOLO_1      0x0101
#define PCI_DEV_PICCOLO_2      0x0102
#define PCI_DEV_PICCOLO_3      0x0103
#define PCI_DEV_PICCOLO_5      0x0105

#define PICCOLO_REG_PIO_TIMING   0x50
#define PICCOLO_REG_DMA_TIMING   0x5C

#define PICCOLO_PIO_MASK     0xFFFFE088
#define PICCOLO_DMA_MASK     0xFFFFE088
#define PICCOLO_UDMA_MASK    0x78FFE088

static const ULONG PiccoloPioTimings[] =
{
    0x0566, // Mode 0
    0x0433, // Mode 1
    0x0311, // Mode 2
    0x0201, // Mode 3
    0x0200  // Mode 4
};

static const ULONG PiccoloMwDmaTimings[] =
{
    0x0655, // Mode 0
    0x0200, // Mode 1
    0x0200, // Mode 2
};

static const ULONG PiccoloUdmaTimings[] =
{
    0x84000222, // Mode 0
    0x83000111, // Mode 1
    0x82000000, // Mode 2
};

/* FUNCTIONS ******************************************************************/

static
VOID
ToshibaSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    ULONG i;

    UNREFERENCED_PARAMETER(Channel);

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        ULONG TimingReg;

        if (!Device)
            continue;

        TimingReg = PciRead32(Controller, PICCOLO_REG_PIO_TIMING);
        TimingReg &= PICCOLO_PIO_MASK;
        TimingReg |= PiccoloPioTimings[Device->PioMode];
        PciWrite32(Controller, PICCOLO_REG_PIO_TIMING, TimingReg);

        if (Device->DmaMode != PIO_MODE(0))
        {
            TimingReg = PciRead32(Controller, PICCOLO_REG_DMA_TIMING);

            if (Device->DmaMode >= UDMA_MODE(0))
            {
                TimingReg &= PICCOLO_UDMA_MASK;
                TimingReg |= PiccoloUdmaTimings[Device->DmaMode - UDMA_MODE(0)];
            }
            else
            {
                TimingReg &= PICCOLO_DMA_MASK;
                TimingReg |= PiccoloMwDmaTimings[Device->DmaMode - MWDMA_MODE(0)];
            }

            PciWrite32(Controller, PICCOLO_REG_DMA_TIMING, TimingReg);
        }
    }
}

CODE_SEG("PAGE")
NTSTATUS
ToshibaGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_TOSHIBA);

    if (Controller->Pci.DeviceID != PCI_DEV_PICCOLO_1 &&
        Controller->Pci.DeviceID != PCI_DEV_PICCOLO_2 &&
        Controller->Pci.DeviceID != PCI_DEV_PICCOLO_3 &&
        Controller->Pci.DeviceID != PCI_DEV_PICCOLO_5)
    {
        return STATUS_NO_MATCH;
    }

    Controller->MaxChannels = 1;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        ChanData->TransferModeSupported = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 2);
        ChanData->SetTransferMode = ToshibaSetTransferMode;
    }

    return STATUS_SUCCESS;
}
