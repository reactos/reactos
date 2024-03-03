/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     ATI ATA controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * Adapted from the FreeBSD ata-ati driver
 * Copyright (c) 1998-2008 Søren Schmidt <sos@FreeBSD.org>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_IXP200_IDE                  0x4349
#define PCI_DEV_IXP300_IDE                  0x4369
#define PCI_DEV_IXP400_IDE                  0x4376
#define PCI_DEV_IXP600_IDE                  0x438C
#define PCI_DEV_IXP700_IDE                  0x439C

PCIIDEX_PAGED_DATA
static const ATA_PCI_ENABLE_BITS AtiEnableBits[MAX_IDE_CHANNEL] =
{
    { 0x48, 0x01, 0x00 },
    { 0x48, 0x08, 0x00 },
};

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
NTSTATUS
AtiGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_ATI);

    if (Controller->Pci.DeviceID != PCI_DEV_IXP200_IDE &&
        Controller->Pci.DeviceID != PCI_DEV_IXP300_IDE &&
        Controller->Pci.DeviceID != PCI_DEV_IXP400_IDE &&
        Controller->Pci.DeviceID != PCI_DEV_IXP600_IDE &&
        Controller->Pci.DeviceID != PCI_DEV_IXP700_IDE)
    {
        return STATUS_NO_MATCH;
    }

    Controller->ChannelEnableBits = AtiEnableBits;

    if (Controller->Pci.DeviceID == PCI_DEV_IXP600_IDE)
        Controller->MaxChannels = 1;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        ChanData->SetTransferMode = SvwSetTransferMode;
        ChanData->TransferModeSupported = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 5);

        if (Controller->Pci.DeviceID == PCI_DEV_IXP200_IDE)
            ChanData->TransferModeSupported &= ~UDMA_MODE5;

        if (!SvwHasUdmaCable(Controller, i))
        {
            INFO("CH %lu: BIOS hasn't selected mode faster than UDMA 2, "
                 "assume 40-conductor cable\n",
                 ChanData->Channel);
            ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
        }
    }

    return STATUS_SUCCESS;
}
