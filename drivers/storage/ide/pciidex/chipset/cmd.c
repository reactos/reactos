/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CMD PCI IDE controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * The CMD-640 does not support 32-bit PCI configuration space writes.
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_PCI0640            0x0640
#define PCI_DEV_PCI0643            0x0643
#define PCI_DEV_PCI0646            0x0646
#define PCI_DEV_PCI0648            0x0648
#define PCI_DEV_CMD0649            0x0649
// #define PCI_DEV_SIL0680            0x0680

#define HW_FLAGS_PRIMARY_ENABLED   0x0001
#define HW_FLAGS_PIO_BUG           0x0002

#define CMD_REG_CNTRL          0x51
#define CMD_REG_ARTTIM23       0x57

#define CMD_CONFIG_PREFETCH_DISABLE(Channel)  (0xC0 >> (4 * (Channel)))

/* FUNCTIONS ******************************************************************/

#if 0 // TODO
static
VOID
CmdSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    PCHANNEL_DATA_PATA ChanData = Controller->Channels[Channel];
    ATA_TIMING DeviceTimings[MAX_IDE_DEVICE];
    ULONG i;

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        PATA_TIMING Timing = &DeviceTimings[i];

        if (!Device)
            continue;

    }
}
#endif

static
IDE_CHANNEL_STATE
CmdChannelEnabledTest(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel)
{
    PCHANNEL_DATA_PATA ChanData = Controller->Channels[Channel];
    UCHAR Control;

    /* Some controllers lack the primary channel enable bit */
    if ((Channel == 0) && (ChanData->HwFlags & HW_FLAGS_PRIMARY_ENABLED))
        return ChannelEnabled;

    Control = PciRead8(Controller, CMD_REG_CNTRL);
    if (Control & (0x04 << Channel))
        return ChannelEnabled;

    return ChannelDisabled;
}

/*
 * Errata: Disable the Read-Ahead Mode.
 * https://www.mindprod.com/jgloss/eideflaw.html
 */
static
VOID
Cmd640ControllerStart(
    _In_ PATA_CONTROLLER Controller)
{
    static const ULONG Registers[] = { CMD_REG_CNTRL, CMD_REG_ARTTIM23 };
    ULONG i;
    UCHAR Value;

    for (i = 0; i < RTL_NUMBER_OF(Registers); ++i)
    {
        Value = PciRead8(Controller, Registers[i]);
        Value |= CMD_CONFIG_PREFETCH_DISABLE(i);
        PciWrite8(Controller, Registers[i], Value);
    }
}

CODE_SEG("PAGE")
NTSTATUS
CmdGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i, SupportedMode, HwFlags = 0;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_CMD);

    // TODO: Allocate a struct per controller

    switch (Controller->Pci.DeviceID)
    {
        case PCI_DEV_PCI0640:
            /*
             * Errata: The CMD-640 has one set of task file registers per controller
             * and thus the two channels cannot be used simultaneously.
             * This also applies to PIO commands.
             * https://www.mindprod.com/jgloss/eideflaw.html
             */
            Controller->Flags |= CTRL_FLAG_IS_SIMPLEX;

            Controller->Start = Cmd640ControllerStart;

            SupportedMode = PIO_ALL;
            HwFlags |= HW_FLAGS_PRIMARY_ENABLED;
            break;

        case PCI_DEV_PCI0643:
            SupportedMode = PIO_ALL | SWDMA_ALL | MWDMA_ALL;

            if (Controller->Pci.RevisionID < 6)
                HwFlags |= HW_FLAGS_PRIMARY_ENABLED;
            break;

        case PCI_DEV_PCI0646:
            SupportedMode = PIO_ALL | SWDMA_ALL | MWDMA_ALL;

            if (Controller->Pci.RevisionID < 3)
                HwFlags |= HW_FLAGS_PRIMARY_ENABLED;

            if (Controller->Pci.RevisionID == 5 || Controller->Pci.RevisionID == 6)
            {
                /*
                 * Early 646U2 revisions can support UDMA2 only at a PCI bus speed of 33MHz.
                 * When it runs at 25 MHz or 30 MHZ, the transfer speed must be limited to UDMA1.
                 */
                SupportedMode |= UDMA_MODES(0, 1);
            }
            else if (Controller->Pci.RevisionID > 6)
            {
                SupportedMode |= UDMA_MODES(0, 2);
            }
            break;

        case PCI_DEV_PCI0648:
            SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 4);
            break;

        case PCI_DEV_CMD0649:
            SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 5);

            if (Controller->Pci.RevisionID == 2)
                HwFlags |= HW_FLAGS_PIO_BUG;
            break;

        default:
            return STATUS_NO_MATCH;
    }

    Controller->Flags |= CTRL_FLAG_USE_TEST_FUNCTION;
    Controller->ChannelEnabledTest = CmdChannelEnabledTest;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        if (Controller->Pci.DeviceID == PCI_DEV_PCI0640)
            ChanData->ChanInfo &= ~CHANNEL_FLAG_IO32;

        ChanData->HwFlags = HwFlags;
        // ChanData->SetTransferMode = CmdSetTransferMode;
        ChanData->TransferModeSupported = SupportedMode;
    }

    return STATUS_SUCCESS;
}
