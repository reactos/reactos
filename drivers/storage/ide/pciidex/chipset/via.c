/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     VIA ATA controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * Some code taken from the NetBSD viaide driver
 * Copyright (c) 1999, 2000, 2001 Manuel Bouyer
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_VT82C576M_IDE              0x1571
#define PCI_DEV_VT82C586A_IDE              0x0571
#define PCI_DEV_VT6410_IDE_RAID            0x3164
#define PCI_DEV_VT6415_IDE                 0x0415
#define PCI_DEV_VT6420_IDE                 0x4149
#define PCI_DEV_VT6421A_RAID               0x3249
#define PCI_DEV_CX700M2_IDE_SATA           0x5324
#define PCI_DEV_CX700M2_IDE_SATA_RAID      0x0581
#define PCI_DEV_VX855_IDE                  0xC409

#define PCI_DEV_VT8237_SATA                0x3149
#define PCI_DEV_VT8237A_SATA               0x0591
#define PCI_DEV_VT8237A_SATA_2             0x5337
#define PCI_DEV_VT8237S_SATA               0x5372
#define PCI_DEV_VT8237S_SATA_RAID          0x7372
#define PCI_DEV_VT8251_SATA_AHCI           0x3349
#define PCI_DEV_VT8251_SATA_2              0x5287
#define PCI_DEV_VT8261_SATA                0x9000
#define PCI_DEV_VT8261_SATA_RAID           0x9040
#define PCI_DEV_VX900_SATA                 0x9001
#define PCI_DEV_VX900_SATA_RAID            0x9041

#define PCI_DEV_BRIDGE_VT82C586            0x0586
#define PCI_DEV_BRIDGE_VT82C596A           0x0596
#define PCI_DEV_BRIDGE_VT82C686A           0x0686
#define PCI_DEV_BRIDGE_VT8231              0x8231
#define PCI_DEV_BRIDGE_VT8233              0x3074
#define PCI_DEV_BRIDGE_VT8233A             0x3147
#define PCI_DEV_BRIDGE_VT8233C             0x3109
#define PCI_DEV_BRIDGE_VT8235              0x3177
#define PCI_DEV_BRIDGE_VT8237              0x3227
#define PCI_DEV_BRIDGE_VT8237A             0x3337
#define PCI_DEV_BRIDGE_VT8237S             0x3372
#define PCI_DEV_BRIDGE_VT8251              0x3287
#define PCI_DEV_BRIDGE_VT8261              0x3402

#define HW_FLAGS_TYPE_MASK           0x000F
#define HW_FLAGS_CHECK_BRIDGE        0x0010
#define HW_FLAGS_HAS_UDMA_CLOCK      0x0020
#define HW_FLAGS_SINGLE_CHAN         0x0040
#define HW_FLAGS_SINGLE_PORT         0x0080
#define HW_FLAGS_FIFO_FIX            0x0100

#define TYPE_33              0
#define TYPE_66              1
#define TYPE_100             2
#define TYPE_133             3
#define TYPE_MWDMA           4
#define TYPE_SATA            5

#define GET_TYPE(Flags)  ((Flags) & HW_FLAGS_TYPE_MASK)

#define VIA_PCI_CLOCK    30000

#define VIA_REG_ENABLE_CTRL                      0x40
#define VIA_REG_DRIVE_TIMING                     0x48
#define VIA_REG_PORT_TIMING(Channel)             (0x4F - (Channel))
#define VIA_REG_ADDRESS_SETUP                    0x4C
#define VIA_REG_UDMA_CTRL                        0x50

#define VIA_UDMA_CLOCK_UDMA66   0x08
#define VIA_UDMA_CABLE_BITS     0x10
#define VIA_UDMA_CLEAR_MASK     0xEF // Save the cable bits

#define VIA_UDMA_CLOCK_ENABLED(Reg32, Channel) \
    (((Reg32) & (0x00080000 >> ((Channel) * 16))) != 0)

#define VIA_UDMA_CABLE_PRESENT(Reg32, Channel) \
    (((Reg32) & (0x10100000 >> ((Channel) * 16))) != 0)

#define VIA_SINGLE_CHAN_UDMA_CABLE_PRESENT(Controller) \
    ((PciRead16(Controller, 0x50) & 0x1010) != 0)

/* Note that the cable bits are inverted */
#define VT6421A_UDMA_CABLE_PRESENT(Controller) \
    ((PciRead16(Controller, 0xB2) & 0x1010) == 0)

#define VIA_REG_SATA_PORT_MAP                    0x49

#define VT6421A_REG_SATA_CTRL             0x52 // Also for VT8237
#define VT6421A_REG_TIMING_CTRL(Drive)    (0xAB - (Drive))
#define VT6421A_REG_UDMA_CTRL(Drive)      (0xB3 - (Drive))

#define VT6421A_UDMA_SLOW                 0x0F
#define VT6421A_FIFO_WATERMARK_64DW       0x04

PCIIDEX_PAGED_DATA
static const struct
{
    USHORT DeviceID;
    USHORT Flags;
} ViaControllerList[] =
{
    { PCI_DEV_VT82C576M_IDE,     TYPE_MWDMA },
    { PCI_DEV_VT82C586A_IDE,     HW_FLAGS_CHECK_BRIDGE },
    { PCI_DEV_VT6410_IDE_RAID,   TYPE_133 },
    { PCI_DEV_VT6415_IDE,        TYPE_133 | HW_FLAGS_SINGLE_CHAN },
    { PCI_DEV_VT6420_IDE,        TYPE_133 | HW_FLAGS_SINGLE_CHAN },
    { PCI_DEV_VX855_IDE,         TYPE_133 | HW_FLAGS_SINGLE_CHAN },
    { PCI_DEV_VT8237_SATA,       TYPE_SATA | HW_FLAGS_SINGLE_PORT | HW_FLAGS_FIFO_FIX },
    { PCI_DEV_VT8237A_SATA,      TYPE_SATA | HW_FLAGS_SINGLE_PORT },
    { PCI_DEV_VT8237A_SATA_2,    TYPE_SATA | HW_FLAGS_SINGLE_PORT },
    { PCI_DEV_VT8237S_SATA,      TYPE_SATA | HW_FLAGS_SINGLE_PORT },
    { PCI_DEV_VT8237S_SATA_RAID, TYPE_SATA | HW_FLAGS_SINGLE_PORT },
    { PCI_DEV_VT8251_SATA_AHCI,  TYPE_SATA },
    { PCI_DEV_VT8251_SATA_2,     TYPE_SATA },
    { PCI_DEV_VT8261_SATA,       TYPE_SATA },
    { PCI_DEV_VT8261_SATA_RAID,  TYPE_SATA },
    { PCI_DEV_VX900_SATA,        TYPE_SATA },
    { PCI_DEV_VX900_SATA_RAID,   TYPE_SATA },
};

static const UCHAR Via6421PioTimings[] =
{
    // 0     1     2     3     4
    0xA8, 0x65, 0x65, 0x31, 0x20
};

static const struct
{
    UCHAR DefaultValue;
    UCHAR Data[7];
} ViaUdmaTimings[] =
{
    //   UDMA    0     1     2     3     4     5     6
    { 0x03, { 0xE2, 0xE1, 0xE0                         } }, // TYPE_33
    { 0x03, { 0xE6, 0xE4, 0xE2, 0xE1, 0xE0             } }, // TYPE_66
    { 0x07, { 0xEA, 0xE6, 0xE4, 0xE2, 0xE1, 0xE0       } }, // TYPE_100
    { 0x07, { 0xEE, 0xE8, 0xE6, 0xE4, 0xE2, 0xE1, 0xE0 } }, // TYPE_133
};

PCIIDEX_PAGED_DATA
static const ATA_PCI_ENABLE_BITS ViaPataEnableBits[MAX_IDE_CHANNEL] =
{
    { 0x40, 0x02, 0x02 },
    { 0x40, 0x01, 0x01 },
};

PCIIDEX_PAGED_DATA
static const ATA_PCI_ENABLE_BITS ViaCx700EnableBits[MAX_IDE_CHANNEL] =
{
    { 0x40, 0x02, 0x02 },
    { 0xC0, 0x01, 0x01 },
};

/* FUNCTIONS ******************************************************************/

static
VOID
ViaSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    PCHANNEL_DATA_PATA ChanData = Controller->Channels[Channel];
    ATA_TIMING DeviceTimings[MAX_IDE_DEVICE];
    ULONG i;
    ULONG DriveTimReg, UdmaTimReg;
    UCHAR PortTimReg;

    AtaSelectTimings(DeviceList, DeviceTimings, VIA_PCI_CLOCK, SHARED_CMD_TIMINGS);

    INFO("CH %lu: Config (before)\n"
         "DRV  %08lX\n"
         "PORT %08lX\n"
         "UDMA %08lX\n"
         "CFG  %08lX\n",
         Channel,
         PciRead32(Controller, 0x48),
         PciRead32(Controller, 0x4C),
         PciRead32(Controller, 0x50),
         PciRead32(Controller, 0x40));

    DriveTimReg = PciRead32(Controller, VIA_REG_DRIVE_TIMING);
    PortTimReg = PciRead8(Controller, VIA_REG_PORT_TIMING(Channel));

    if (GET_TYPE(ChanData->HwFlags) != TYPE_MWDMA)
        UdmaTimReg = PciRead32(Controller, VIA_REG_UDMA_CTRL);

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        PATA_TIMING Timing = &DeviceTimings[i];
        const ULONG DeviceIndex = (Channel << 1) + i; // 0   1   2   3
        const ULONG Shift = (3 - DeviceIndex) << 3;   // 24  16  8   0
        ULONG Value;

        /* UDMA timings */
        if (GET_TYPE(ChanData->HwFlags) != TYPE_MWDMA)
        {
            if (Device && (Device->DmaMode >= UDMA_MODE(0)))
            {
                ULONG ModeIndex = Device->DmaMode - UDMA_MODE(0);
                Value = ViaUdmaTimings[GET_TYPE(ChanData->HwFlags)].Data[ModeIndex];
            }
            else
            {
                Value = ViaUdmaTimings[GET_TYPE(ChanData->HwFlags)].DefaultValue;
            }

            /* Turn on the 66MHz UDMA clock */
            if ((i != 0) && (GET_TYPE(ChanData->HwFlags) == TYPE_66))
                Value |= VIA_UDMA_CLOCK_UDMA66;

            UdmaTimReg &= ~(VIA_UDMA_CLEAR_MASK << Shift);
            UdmaTimReg |= Value << Shift;
        }

        if (!Device)
            continue;

        /* PIO and DMA timings */
        Timing->AddressSetup = CLAMP_TIMING(Timing->AddressSetup, 1, 4) - 1;
        Timing->CmdRecovery  = CLAMP_TIMING(Timing->CmdRecovery, 1, 16) - 1;
        Timing->CmdActive    = CLAMP_TIMING(Timing->CmdActive, 1, 16) - 1;
        Timing->DataRecovery = CLAMP_TIMING(Timing->DataRecovery, 1, 16) - 1;
        Timing->DataActive   = CLAMP_TIMING(Timing->DataActive, 1, 16) - 1;

        Value = (Timing->DataActive << 4) | Timing->DataRecovery;
        DriveTimReg &= ~(0xFF << Shift);
        DriveTimReg |= Value << Shift;

        PortTimReg = (Timing->CmdActive << 4) | Timing->CmdRecovery;
    }

    PciWrite8(Controller, VIA_REG_PORT_TIMING(Channel), PortTimReg);
    PciWrite32(Controller, VIA_REG_DRIVE_TIMING, DriveTimReg);

    if (GET_TYPE(ChanData->HwFlags) != TYPE_MWDMA)
        PciWrite32(Controller, VIA_REG_UDMA_CTRL, UdmaTimReg);

    INFO("CH %lu: Config (after)\n"
         "DRV  %08lX\n"
         "PORT %08lX\n"
         "UDMA %08lX\n"
         "CFG  %08lX\n",
         Channel,
         PciRead32(Controller, 0x48),
         PciRead32(Controller, 0x4C),
         PciRead32(Controller, 0x50),
         PciRead32(Controller, 0x40));
}

static
VOID
ViaSataSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    UCHAR SataControl = PciRead8(Controller, VT6421A_REG_SATA_CTRL);
    ULONG i;

    if (!(SataControl & VT6421A_FIFO_WATERMARK_64DW))
    {
        for (i = 0; i < MAX_IDE_DEVICE; ++i)
        {
            PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];

            if (!Device)
                continue;

            /* Fix freezes with Western Digital drives. See CORE-5897 for more details */
            if (_strnicmp(Device->FriendlyName, "WD", 2) == 0)
            {
                /* This will slow down I/O performance */
                WARN("Enabling workaround for the '%s' drive\n", Device->FriendlyName);
                SataControl |= VT6421A_FIFO_WATERMARK_64DW;
                PciWrite8(Controller, VT6421A_REG_SATA_CTRL, SataControl);
                break;
            }
        }
    }

    SataSetTransferMode(Controller, Channel, DeviceList);
}

static
VOID
Via6421SetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    ULONG i;

    UNREFERENCED_PARAMETER(Channel);

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        UCHAR UdmaTimReg;

        /* UDMA timings */
        UdmaTimReg = PciRead8(Controller, VT6421A_REG_UDMA_CTRL(i));
        UdmaTimReg &= VIA_UDMA_CABLE_BITS;
        if (Device && (Device->DmaMode >= UDMA_MODE(0)))
            UdmaTimReg |= ViaUdmaTimings[TYPE_133].Data[Device->DmaMode - UDMA_MODE(0)];
        else
            UdmaTimReg |= VT6421A_UDMA_SLOW;
        PciWrite8(Controller, VT6421A_REG_UDMA_CTRL(i), UdmaTimReg);

        if (!Device)
            continue;

        /* PIO timings */
        PciWrite8(Controller, VT6421A_REG_TIMING_CTRL(i), Via6421PioTimings[Device->PioMode]);
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
Via6421ParseResources(
    _In_ PVOID ChannelContext,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    PUCHAR CommandPortBase, ControlPortBase, DmaBase;

    PAGED_CODE();
    ASSERT(ChanData->Channel < 3);

    /*
     * [BAR 0]: I/O ports at E150 [size=16]  Channel 0
     * [BAR 1]: I/O ports at E140 [size=16]  Channel 1
     * [BAR 2]: I/O ports at E130 [size=16]  Channel 2
     * [BAR 3]: I/O ports at E120 [size=16]
     * [BAR 4]: I/O ports at E100 [size=32]  BM DMA
     * [BAR 5]: I/O ports at E000 [size=256] SATA PHY (minimum size = 128)
     */

    CommandPortBase = AtaCtrlPciGetBar(ChanData->Controller, ChanData->Channel, 16);
    if (!CommandPortBase)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    DmaBase = AtaCtrlPciGetBar(ChanData->Controller, 4, 32);
    if (!DmaBase)
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    ChanData->Regs.Dma = DmaBase + (ChanData->Channel * 8);

    ControlPortBase = CommandPortBase + PCIIDE_COMMAND_IO_RANGE_LENGTH;

    PciIdeInitTaskFileIoResources(ChanData,
                                  (ULONG_PTR)CommandPortBase,
                                  (ULONG_PTR)ControlPortBase + PCIIDE_CONTROL_IO_BAR_OFFSET,
                                  1);
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
Via6421GetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i;

    PAGED_CODE();

    /*
     * Channel 0: SATA port #0 (M only)
     * Channel 1: SATA port #1 (M only)
     * Channel 2: PATA channel
     */
    Controller->MaxChannels = 3;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        /* Non-standard register layout */
        ChanData->ParseResources = Via6421ParseResources;

        if (i == 2)
        {
            /* No MWDMA support */
            ChanData->SetTransferMode = Via6421SetTransferMode;
            ChanData->TransferModeSupported = PIO_ALL | UDMA_ALL;

            if (!VT6421A_UDMA_CABLE_PRESENT(Controller))
            {
                INFO("CH %lu: BIOS detected 40-conductor cable\n", ChanData->Channel);
                ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
            }
        }
        else
        {
            ChanData->ChanInfo |= CHANNEL_FLAG_NO_SLAVE;
            ChanData->SetTransferMode = ViaSataSetTransferMode;
            ChanData->TransferModeSupported = SATA_ALL;
        }
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
ViaCx700GetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i;

    PAGED_CODE();

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    Controller->ChannelEnableBits = ViaCx700EnableBits;

    /*
     * Channel 0: 2 SATA ports (M/S emulation)
     * Channel 1: PATA channel
     */
    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        if (i == 0)
        {
            ChanData->SetTransferMode = SataSetTransferMode;
            ChanData->TransferModeSupported = SATA_ALL;
        }
        else
        {
            ChanData->HwFlags |= TYPE_133;
            ChanData->SetTransferMode = ViaSetTransferMode;
            ChanData->TransferModeSupported = PIO_ALL | MWDMA_ALL | UDMA_ALL;

            if (!VIA_SINGLE_CHAN_UDMA_CABLE_PRESENT(Controller))
            {
                INFO("CH %lu: BIOS detected 40-conductor cable\n", ChanData->Channel);
                ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
            }
        }
    }

    return STATUS_SUCCESS;
}

static
VOID
Via6410ControllerStart(
    _In_ PATA_CONTROLLER Controller)
{
    UCHAR Value;

    /*
     * Enable the primary and secondary IDE channels.
     *
     * The VT6410 can be either a controller soldered onto the motherboard
     * or added on an external PCI card. These PCI cards usually come
     * without an option ROM on their own and have these bits set to 0s by default.
     */
    Value = PciRead8(Controller, VIA_REG_ENABLE_CTRL);
    Value |= 0x03;
    PciWrite8(Controller, VIA_REG_ENABLE_CTRL, Value);
}

static
CODE_SEG("PAGE")
BOOLEAN
ViaQuerySouthBridgeInformation(
    _In_ PVOID Context,
    _In_ ULONG BusNumber,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_ PPCI_COMMON_HEADER PciConfig)
{
    PULONG HwFlags = Context;
    ULONG i;
    PCIIDEX_PAGED_DATA
    static const struct
    {
        USHORT DeviceID;
        UCHAR MinimumRevisionID;
        UCHAR Flags;
    } ViaBridgeList[] =
    {
        { PCI_DEV_BRIDGE_VT82C586,  0x20, TYPE_33 },
        { PCI_DEV_BRIDGE_VT82C586,  0,    TYPE_MWDMA },
        { PCI_DEV_BRIDGE_VT82C596A, 0x10, TYPE_66 | HW_FLAGS_HAS_UDMA_CLOCK },
        { PCI_DEV_BRIDGE_VT82C596A, 0,    TYPE_33 },
        { PCI_DEV_BRIDGE_VT82C686A, 0x40, TYPE_100 },
        { PCI_DEV_BRIDGE_VT82C686A, 0,    TYPE_66 | HW_FLAGS_HAS_UDMA_CLOCK },
        { PCI_DEV_BRIDGE_VT8231,    0,    TYPE_100 },
        { PCI_DEV_BRIDGE_VT8233,    0,    TYPE_100 },
        { PCI_DEV_BRIDGE_VT8233A,   0,    TYPE_133 },
        { PCI_DEV_BRIDGE_VT8233C,   0,    TYPE_100 },
        { PCI_DEV_BRIDGE_VT8235,    0,    TYPE_133 },
        { PCI_DEV_BRIDGE_VT8237,    0,    TYPE_133 },
        { PCI_DEV_BRIDGE_VT8237A,   0,    TYPE_133 },
        { PCI_DEV_BRIDGE_VT8237S,   0,    TYPE_133 },
        { PCI_DEV_BRIDGE_VT8251,    0,    TYPE_133 },
        { PCI_DEV_BRIDGE_VT8261,    0,    TYPE_133 },
    };

    UNREFERENCED_PARAMETER(BusNumber);
    UNREFERENCED_PARAMETER(PciSlot);

    PAGED_CODE();

    if (PciConfig->VendorID != PCI_VEN_VIA)
        return FALSE;

    for (i = 0; i < RTL_NUMBER_OF(ViaBridgeList); ++i)
    {
        if ((PciConfig->DeviceID == ViaBridgeList[i].DeviceID) &&
            (PciConfig->RevisionID >= ViaBridgeList[i].MinimumRevisionID))
        {
            INFO("Found %04X:%04X.%02X VIA bridge\n",
                 PciConfig->VendorID, PciConfig->DeviceID, PciConfig->RevisionID);

            *HwFlags = ViaBridgeList[i].Flags;
            return TRUE;
        }
    }

    return FALSE;
}

static
CODE_SEG("PAGE")
NTSTATUS
ViaPataGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller,
    _In_ ULONG HwFlags)
{
    ULONG i, UdmaTimReg;

    PAGED_CODE();

    /*
     * Look for a PCI-ISA bridge to see what features are available.
     * Early VIA PATA controllers share the same PCI device ID
     * but have very different capabilities.
     */
    if (HwFlags & HW_FLAGS_CHECK_BRIDGE)
    {
        if (!PciFindDevice(ViaQuerySouthBridgeInformation, &HwFlags))
        {
            ERR("Unable to find the VIA bridge\n");
            ASSERT(FALSE);
            return STATUS_NO_MATCH;
        }
    }

    if (Controller->Pci.DeviceID == PCI_DEV_VT6410_IDE_RAID)
        Controller->Start = Via6410ControllerStart;

    /* The VT6415 has no channel enable bits */
    if (Controller->Pci.DeviceID != PCI_DEV_VT6415_IDE)
        Controller->ChannelEnableBits = ViaPataEnableBits;

    if (GET_TYPE(HwFlags) != TYPE_MWDMA)
        UdmaTimReg = PciRead32(Controller, VIA_REG_UDMA_CTRL);

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        ChanData->HwFlags = HwFlags;
        ChanData->SetTransferMode = ViaSetTransferMode;

        switch (GET_TYPE(ChanData->HwFlags))
        {
            case TYPE_MWDMA:
            {
                ChanData->TransferModeSupported = PIO_ALL | MWDMA_ALL;
                break;
            }
            case TYPE_33:
            {
                ChanData->TransferModeSupported = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 2);
                break;
            }
            case TYPE_66:
            {
                ChanData->TransferModeSupported = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 4);

                /* Check to see if the BIOS has enabled the 66MHz UDMA clock for us */
                if (!VIA_UDMA_CLOCK_ENABLED(UdmaTimReg, i))
                {
                    /* It seems that the clock can also be disabled due to hardware errata */
                    INFO("CH %lu: BIOS detected 40-conductor cable "
                         "or this chip is ATA/33 capable\n", ChanData->Channel);
                    ChanData->TransferModeSupported &= ~UDMA_80C_ALL;

                    /* Use timing table based on a 33MHz UDMA clock */
                    ChanData->HwFlags &= ~HW_FLAGS_TYPE_MASK;
                    ChanData->HwFlags |= TYPE_33;
                }
                break;
            }
            case TYPE_100:
            {
                ChanData->TransferModeSupported = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 5);

                if (!VIA_UDMA_CABLE_PRESENT(UdmaTimReg, i))
                {
                    INFO("CH %lu: BIOS detected 40-conductor cable\n", ChanData->Channel);
                    ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
                }
                break;
            }
            case TYPE_133:
            {
                ChanData->TransferModeSupported = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 6);

                if (!VIA_UDMA_CABLE_PRESENT(UdmaTimReg, i))
                {
                    INFO("CH %lu: BIOS detected 40-conductor cable\n", ChanData->Channel);
                    ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
                }
                break;
            }

            default:
                ASSERT(FALSE);
                UNREACHABLE;
        }
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
ViaSataGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller,
    _In_ ULONG HwFlags)
{
    ULONG i;

    PAGED_CODE();

    INFO("Port map: %02X\n", PciRead8(Controller, VIA_REG_SATA_PORT_MAP));

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        ChanData->HwFlags = HwFlags;
        ChanData->TransferModeSupported = SATA_ALL;

        if (HwFlags & HW_FLAGS_FIFO_FIX)
            ChanData->SetTransferMode = ViaSataSetTransferMode;
        else
            ChanData->SetTransferMode = SataSetTransferMode;

        if (HwFlags & HW_FLAGS_SINGLE_PORT)
            ChanData->ChanInfo |= CHANNEL_FLAG_NO_SLAVE;
    }

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
ViaGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i, HwFlags;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_VIA);

    if (Controller->Pci.DeviceID == PCI_DEV_VT6421A_RAID)
    {
        return Via6421GetControllerProperties(Controller);
    }
    else if ((Controller->Pci.DeviceID == PCI_DEV_CX700M2_IDE_SATA) ||
             (Controller->Pci.DeviceID == PCI_DEV_CX700M2_IDE_SATA_RAID))
    {
        return ViaCx700GetControllerProperties(Controller);
    }

    for (i = 0; i < RTL_NUMBER_OF(ViaControllerList); ++i)
    {
        HwFlags = ViaControllerList[i].Flags;

        if (Controller->Pci.DeviceID == ViaControllerList[i].DeviceID)
            break;
    }
    if (i == RTL_NUMBER_OF(ViaControllerList))
        return STATUS_NO_MATCH;

    if (HwFlags & HW_FLAGS_SINGLE_CHAN)
        Controller->MaxChannels = 1;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    if (GET_TYPE(HwFlags) == TYPE_SATA)
        return ViaSataGetControllerProperties(Controller, HwFlags);

    return ViaPataGetControllerProperties(Controller, HwFlags);
}
