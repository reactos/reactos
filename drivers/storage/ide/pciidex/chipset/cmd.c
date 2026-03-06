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
#define HW_FLAGS_HAS_UDMA_REG      0x0002
#define HW_FLAGS_NEED_PIO_FIX      0x0004
#define HW_FLAGS_NO_PREFETCH       0x0008

#define CMD_PCI_CLOCK    30000

#define CMD_REG_CFR                    0x50
#define CMD_REG_CNTRL                  0x51
#define CMD_REG_CMDTIM                 0x52
#define CMD_REG_ARTTIM0                0x53
#define CMD_REG_DRWTIM0                0x54
#define CMD_REG_ARTTIM1                0x55
#define CMD_REG_DRWTIM1                0x56
#define CMD_REG_ARTTIM23               0x57
#define CMD_REG_DRWTIM2                0x58
#define CMD_REG_DRWTIM23               0x58 // 640 only
#define CMD_REG_DRWTIM3                0x5B
#define CMD_REG_MRDMODE                0x71
#define CMD_REG_BMIDECSR               0x79
#define CMD_REG_UDIDETCR(Channel)      (((Channel) == 0) ? 0x73 : 0x7B)

#define CMD_CFR_INTR(Channel)          (((Channel) == 0) ? 0x04 : 0x10)

#define CMD_CNTRL_CHAN_EN(Channel)     (0x04 << (Channel))

#define CMD_ARTTIM_ART_MASK            0xC0

#define CMD_MRDMODE_READ_MULTIPLE      0x01
#define CMD_MRDMODE_INTR(Channel)      (0x04 << (Channel))
#define CMD_MRDMODE_INTR_CH0_EN        0x10
#define CMD_MRDMODE_INTR_CH1_EN        0x20

#define CMD_BMIDECSR_CR(Channel)       (0x01 << (Channel))

#define CMD_UDIDETCR_CLEAR(Drive)      (((Drive) == 0) ? 0x35 : 0xCA)
#define CMD_UDIDETCR_EN(Drive)         (0x01 << (Drive))

typedef struct _CMD_HW_EXTENSION
{
    ATATIM CmdActive[MAX_IDE_CHANNEL * MAX_IDE_DEVICE];
    ATATIM CmdRecovery[MAX_IDE_CHANNEL * MAX_IDE_DEVICE];
} CMD_HW_EXTENSION, *PCMD_HW_EXTENSION;

static const UCHAR CmdPrefetchDisable[MAX_IDE_CHANNEL][MAX_IDE_DEVICE] =
{
    // M    S
    { 0x40, 0x80 }, // Pri
    { 0x04, 0x08 }  // Sec
};

static const ULONG CmdPrefetchRegs[MAX_IDE_CHANNEL] =
{
    // Pri         Sec
    CMD_REG_CNTRL, CMD_REG_ARTTIM23
};

static const ULONG CmdDrwTimRegs[MAX_IDE_CHANNEL][MAX_IDE_DEVICE] =
{
    // M               S
    { CMD_REG_DRWTIM0, CMD_REG_DRWTIM1 }, // Pri
    { CMD_REG_DRWTIM2, CMD_REG_DRWTIM3 }  // Sec
};

static const ULONG CmdArtTimRegs[MAX_IDE_CHANNEL][MAX_IDE_DEVICE] =
{
    // M                S
    { CMD_REG_ARTTIM0,  CMD_REG_ARTTIM1  }, // Pri
    { CMD_REG_ARTTIM23, CMD_REG_ARTTIM23 }  // Sec
};

static const UCHAR CmdArtTimings[] =
{
    0x40, // 0
    0x40, // 1
    0x40, // 2
    0x80, // 3
    0x00, // 4
    0xC0  // 5
};

static const UCHAR CmdUdmaTimings[6][MAX_IDE_DEVICE] =
{
    //  M     S
    { 0x31, 0xC2 }, // 0
    { 0x21, 0x82 }, // 1
    { 0x11, 0x42 }, // 2
    { 0x25, 0x8A }, // 3
    { 0x15, 0x4A }, // 4
    { 0x05, 0x0A }  // 5
};

/* FUNCTIONS ******************************************************************/

static
VOID
CmdFixRecoveryTiming(
    _In_ PCHANNEL_DEVICE_CONFIG Device,
    _Inout_ PATA_TIMING Timing,
    _In_ ATATIM CycleTimeClocks,
    _In_ ATATIM CmdActiveClocks,
    _In_ ATATIM CmdRecoveryClocks,
    _In_ ATATIM DataActiveClocks,
    _In_ ATATIM DataRecoveryClocks)
{
    if (((Timing->DataActive + Timing->DataRecovery) > CycleTimeClocks) || // Cycle time not met
        (Device->DmaMode == MWDMA_MODE(0))) // Data active time not met
    {
        if (Timing->CmdActive < CmdActiveClocks)
            Timing->CmdActive = CmdActiveClocks;
        if (Timing->CmdRecovery < CmdRecoveryClocks)
            Timing->CmdRecovery = CmdRecoveryClocks;

        if (Timing->DataActive < DataActiveClocks)
            Timing->DataActive = DataActiveClocks;
        if (Timing->DataRecovery < DataRecoveryClocks)
            Timing->DataRecovery = DataRecoveryClocks;
    }
    else
    {
        Timing->CmdActive = CmdActiveClocks;
        Timing->CmdRecovery = CmdRecoveryClocks;
        Timing->DataActive = DataActiveClocks;
        Timing->DataRecovery = DataRecoveryClocks;
    }
}

static
VOID
CmdDerateTimings(
    _In_ PCHANNEL_DEVICE_CONFIG Device,
    _Inout_ PATA_TIMING Timing)
{
    /* Make the timings run slower to address the PCI Bus Hang errata */
    switch (Device->PioMode)
    {
        case PIO_MODE(0):
        {
            /*
             * 600ns is minimum in PIO mode 0, but program the chip to 570ns.
             * 480 + 90 = 570
             */
            CmdFixRecoveryTiming(Device, Timing, 20, 16, 3, 16, 3);
            break;
        }
        case PIO_MODE(1):
        {
            /* 300 + 90 = 390 */
            CmdFixRecoveryTiming(Device, Timing, 13, 10, 3, 10, 3);
            break;
        }
        case PIO_MODE(2):
        {
            /*
             * Cmd  300 + 90 = 390
             * Data 150 + 90 = 240
             */
            CmdFixRecoveryTiming(Device, Timing, 8, 10, 3, 5, 3);
            break;
        }

        /* PIO 3 & 4 unaffected by the errata */
        default:
            break;
    }
}

static
UCHAR
CmdPackTimings(
    _In_ PATA_CONTROLLER Controller,
    _In_ ATATIM Active,
    _In_ ATATIM Recovery)
{
    if (Controller->Pci.DeviceID == PCI_DEV_PCI0640)
    {
        if (Active >= 16)
            Active = 0;
        else if (Active < 2)
            Active = 2;

        if (Controller->Pci.RevisionID > 1)
        {
            /* 640B */
            if (Recovery >= 17)
                Recovery = 0;
            else if (Recovery > 2)
                --Recovery;
            else
                Recovery = 2;
        }
        else
        {
            /* 640A */
            if (Recovery >= 16)
                Recovery = 0;
            else if (Recovery < 2)
                Recovery = 2;
        }
    }
    else
    {
        if (Active >= 16)
            Active = 0;

        if (Recovery >= 16)
            Recovery = 0;
        else if (Recovery > 1)
            --Recovery;
        else
            Recovery = 15;
    }

    return (Active << 4) | Recovery;
}

static
VOID
CmdSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    PCHANNEL_DATA_PATA ChanData = Controller->Channels[Channel];
    PCMD_HW_EXTENSION HwExt = Controller->HwExt;
    ATA_TIMING DeviceTimings[MAX_IDE_DEVICE];
    ULONG i, TimingFlags;
    UCHAR Value;

    /* Some timings are shared between both drives drives on the secondary channel */
    TimingFlags = 0;
    if (Channel != 0)
    {
        TimingFlags |= SHARED_ADDR_TIMINGS;

        if (Controller->Pci.DeviceID == PCI_DEV_PCI0640)
            TimingFlags |= SHARED_DATA_TIMINGS;
    }
    AtaSelectTimings(DeviceList, DeviceTimings, CMD_PCI_CLOCK, TimingFlags);

    /* Enable or disable prefetch mode */
    Value = PciRead8(Controller, CmdPrefetchRegs[Channel]);
    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];

        if (!Device)
            continue;

        if ((ChanData->HwFlags & HW_FLAGS_NO_PREFETCH) || !Device->IsFixedDisk)
            Value |= CmdPrefetchDisable[Channel][i];
        else
            Value &= ~CmdPrefetchDisable[Channel][i];
    }
    PciWrite8(Controller, CmdPrefetchRegs[Channel], Value);

    /* Program timing settings */
    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        PATA_TIMING Timing = &DeviceTimings[i];
        UCHAR UdmaTimReg;
        ULONG Register;

        /* UDMA timings */
        if (ChanData->HwFlags & HW_FLAGS_HAS_UDMA_REG)
        {
            UdmaTimReg = PciRead8(Controller, CMD_REG_UDIDETCR(Channel));
            if (Device && (Device->DmaMode >= UDMA_MODE(0)))
            {
                UdmaTimReg &= ~CMD_UDIDETCR_CLEAR(i);
                UdmaTimReg |= CmdUdmaTimings[Device->DmaMode - UDMA_MODE(0)][i];
            }
            else
            {
                UdmaTimReg &= ~CMD_UDIDETCR_EN(i);
            }
            PciWrite8(Controller, CMD_REG_UDIDETCR(Channel), UdmaTimReg);
        }

        if (!Device)
        {
            HwExt->CmdActive[i + Channel * MAX_IDE_DEVICE] = 0;
            HwExt->CmdRecovery[i + Channel * MAX_IDE_DEVICE] = 0;
            continue;
        }

        if (ChanData->HwFlags & HW_FLAGS_NEED_PIO_FIX)
            CmdDerateTimings(Device, Timing);

        /* 8-bit timings */
        HwExt->CmdActive[i + Channel * MAX_IDE_DEVICE] = Timing->CmdActive;
        HwExt->CmdRecovery[i + Channel * MAX_IDE_DEVICE] = Timing->CmdRecovery;

        /* Address setup timing */
        if (Timing->AddressSetup > RTL_NUMBER_OF(CmdArtTimings) - 1)
            Timing->AddressSetup = RTL_NUMBER_OF(CmdArtTimings) - 1;
        Value = PciRead8(Controller, CmdArtTimRegs[Channel][i]);
        Value &= ~CMD_ARTTIM_ART_MASK;
        Value |= CmdArtTimings[Timing->AddressSetup];
        PciWrite8(Controller, CmdArtTimRegs[Channel][i], Value);

        /* 16-bit timings */
        Value = CmdPackTimings(Controller, Timing->DataActive, Timing->DataRecovery);
        if ((Controller->Pci.DeviceID == PCI_DEV_PCI0640) && (Channel != 0))
            Register = CMD_REG_DRWTIM23;
        else
            Register = CmdDrwTimRegs[Channel][i];
        PciWrite8(Controller, Register, Value);
    }

    /* 8-bit timings are shared between both the primary and secondary channels */
    for (i = 0; i < MAX_IDE_CHANNEL * MAX_IDE_DEVICE; ++i)
    {
        if (HwExt->CmdActive[i] > DeviceTimings[0].CmdActive)
            DeviceTimings[0].CmdActive = HwExt->CmdActive[i];

        if (HwExt->CmdRecovery[i] > DeviceTimings[0].CmdRecovery)
            DeviceTimings[0].CmdRecovery = HwExt->CmdRecovery[i];
    }
    if ((DeviceTimings[0].CmdActive != 0) || (DeviceTimings[0].CmdRecovery != 0))
    {
        Value = CmdPackTimings(Controller,
                               DeviceTimings[0].CmdActive,
                               DeviceTimings[0].CmdRecovery);
        PciWrite8(Controller, CMD_REG_CMDTIM, Value);
    }
}

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
    if (Control & CMD_CNTRL_CHAN_EN(Channel))
        return ChannelEnabled;

    return ChannelDisabled;
}

static
BOOLEAN
CmdCheckInterruptMrdMode(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    PATA_CONTROLLER Controller = ChanData->Controller;
    UCHAR Control, InterruptMask;

    if (ChanData->Regs.Dma)
        Control = ATA_READ(ChanData->Regs.Dma + 1);
    else
        Control = PciRead8(Controller, CMD_REG_MRDMODE);

    InterruptMask = CMD_MRDMODE_INTR(ChanData->Channel);
    if (!(Control & InterruptMask))
        return FALSE;

    Control &= ~(CMD_MRDMODE_INTR(0) | CMD_MRDMODE_INTR(1));
    Control |= InterruptMask;
    /* Make sure we do not clear the write-only bits */
    Control |= CMD_MRDMODE_READ_MULTIPLE;

    /* Clear the interrupt */
    if (ChanData->Regs.Dma)
        ATA_WRITE(ChanData->Regs.Dma + 1, Control);
    else
        PciWrite8(Controller, CMD_REG_MRDMODE, Control);

    return TRUE;
}

static
BOOLEAN
CmdCheckInterruptPci(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    PATA_CONTROLLER Controller = ChanData->Controller;
    UCHAR InterruptMask;
    ULONG Register;

    InterruptMask = CMD_CFR_INTR(ChanData->Channel);
    Register = (ChanData->Channel == 0) ? CMD_REG_CFR : CMD_REG_ARTTIM23;

    /* Read CFR or REG_ARTTIM23 to clear the interrupt */
    return !!(PciRead8(Controller, Register) & InterruptMask);
}

static
VOID
CmdControllerStart(
    _In_ PATA_CONTROLLER Controller)
{
    ULONG i;
    UCHAR Control;

    /*
     * Initialize the controller to compatible PIO timings.
     * We dynamically adjust these timings depending on attached devices.
     */
    PciWrite8(Controller, CMD_REG_CMDTIM, 0);

    if (Controller->Pci.DeviceID == PCI_DEV_PCI0640)
    {
        /*
         * Errata: Disable the Read-Ahead Mode.
         * https://www.mindprod.com/jgloss/eideflaw.html
         */
        for (i = 0; i < RTL_NUMBER_OF(CmdPrefetchRegs); ++i)
        {
            Control = PciRead8(Controller, CmdPrefetchRegs[i]);
            Control |= CmdPrefetchDisable[i][0] | CmdPrefetchDisable[i][1];
            PciWrite8(Controller, CmdPrefetchRegs[i], Control);
        }
    }
    else
    {
        Control = PciRead8(Controller, CMD_REG_MRDMODE);
        Control &= ~(CMD_MRDMODE_INTR_CH0_EN | CMD_MRDMODE_INTR_CH1_EN);
        Control |= CMD_MRDMODE_READ_MULTIPLE;
        PciWrite8(Controller, CMD_REG_MRDMODE, Control);
    }
}

CODE_SEG("PAGE")
NTSTATUS
CmdGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i, SupportedMode, HwFlags = 0;
    PCHANNEL_CHECK_INTERRUPT CheckInterrupt = CmdCheckInterruptPci;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_CMD);

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

            SupportedMode = PIO_ALL;
            HwFlags |= HW_FLAGS_PRIMARY_ENABLED | HW_FLAGS_NO_PREFETCH;
            break;

        case PCI_DEV_PCI0643:
            SupportedMode = PIO_ALL | SWDMA_ALL | MWDMA_ALL;

            if (Controller->Pci.RevisionID < 6)
                HwFlags |= HW_FLAGS_PRIMARY_ENABLED;
            break;

        case PCI_DEV_PCI0646:
            SupportedMode = PIO_ALL | SWDMA_ALL | MWDMA_ALL;
            HwFlags |= HW_FLAGS_HAS_UDMA_REG;

            if (Controller->Pci.RevisionID < 3)
                HwFlags |= HW_FLAGS_PRIMARY_ENABLED;
            else
                CheckInterrupt = CmdCheckInterruptMrdMode;

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
            HwFlags |= HW_FLAGS_HAS_UDMA_REG | HW_FLAGS_NO_PREFETCH;
            CheckInterrupt = CmdCheckInterruptMrdMode;
            break;

        case PCI_DEV_CMD0649:
            SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 5);
            HwFlags |= HW_FLAGS_HAS_UDMA_REG;

            if (Controller->Pci.RevisionID == 2)
                HwFlags |= HW_FLAGS_NEED_PIO_FIX;

            CheckInterrupt = CmdCheckInterruptMrdMode;
            break;

        default:
            return STATUS_NO_MATCH;
    }

    Controller->Start = CmdControllerStart;

    Controller->Flags |= CTRL_FLAG_USE_TEST_FUNCTION;
    Controller->ChannelEnabledTest = CmdChannelEnabledTest;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    Controller->HwExt = ExAllocatePoolZero(NonPagedPool, sizeof(CMD_HW_EXTENSION), TAG_PCIIDEX);
    if (!Controller->HwExt)
        return STATUS_INSUFFICIENT_RESOURCES;

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        if (Controller->Pci.DeviceID == PCI_DEV_PCI0640)
            ChanData->ChanInfo &= ~CHANNEL_FLAG_IO32;

        ChanData->HwFlags = HwFlags;
        ChanData->CheckInterrupt = CheckInterrupt;
        ChanData->SetTransferMode = CmdSetTransferMode;
        ChanData->TransferModeSupported = SupportedMode;

        /* Check for 80-conductor cable */
        if (ChanData->TransferModeSupported & UDMA_80C_ALL)
        {
            UCHAR BmControl = PciRead8(Controller, CMD_REG_BMIDECSR);
            if (!(BmControl & CMD_BMIDECSR_CR(i)))
            {
                INFO("CH %lu: BIOS detected 40-conductor cable\n", ChanData->Channel);
                ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
            }
        }
    }

    return STATUS_SUCCESS;
}
