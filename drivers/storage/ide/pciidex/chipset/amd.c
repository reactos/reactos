/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AMD PCI IDE controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_AMD_756                0x7409
#define PCI_DEV_AMD_766                0x7411
#define PCI_DEV_AMD_768                0x7441
#define PCI_DEV_AMD_8111               0x7469
#define PCI_DEV_AMD_CS5536             0x2092
#define PCI_DEV_AMD_CS5536_2           0x209A

#define PCI_DEV_NFORCE_IDE             0x01BC
#define PCI_DEV_NFORCE2_IDE            0x0065
#define PCI_DEV_NFORCE2_IDE_2          0x0085
#define PCI_DEV_NFORCE3_IDE            0x00D5
#define PCI_DEV_NFORCE3_IDE_2          0x00E5
#define PCI_DEV_CK804_IDE              0x0053
#define PCI_DEV_MCP04_IDE              0x0035
#define PCI_DEV_MCP51_IDE              0x0265
#define PCI_DEV_MCP55_IDE              0x036E
#define PCI_DEV_MCP61_IDE              0x03EC
#define PCI_DEV_MCP65_IDE              0x0448
#define PCI_DEV_MCP67_IDE              0x0560
#define PCI_DEV_MCP73_IDE              0x056C
#define PCI_DEV_MCP77_IDE              0x0759

#define PCI_DEV_NFORCE2_SATA           0x008E
#define PCI_DEV_NFORCE3_SATA           0x00E3
#define PCI_DEV_NFORCE3_SATA_2         0x00EE
#define PCI_DEV_CK804_SATA             0x0054
#define PCI_DEV_CK804_SATA_2           0x0055
#define PCI_DEV_MCP04_SATA             0x0036
#define PCI_DEV_MCP04_SATA_2           0x003E
#define PCI_DEV_MCP51_SATA             0x0266
#define PCI_DEV_MCP51_SATA_2           0x0267
#define PCI_DEV_MCP55_SATA             0x037E
#define PCI_DEV_MCP55_SATA_2           0x037F
#define PCI_DEV_MCP61_SATA             0x03E7
#define PCI_DEV_MCP61_SATA_2           0x03F6
#define PCI_DEV_MCP61_SATA_3           0x03F7

#define PCI_DEV_MCP89_SATA             0x0D85

#define PCI_SUBSYSTEM_AMD_SERENADE     0x36C0

#define AMD_CONFIG_BASE     0x40
#define NV_CONFIG_BASE      0x50

#define MAKE_CLOCK(Active, Recovery)     (((Active - 1) << 4) | ((Recovery) - 1))


/**
 * @brief Controller Configuration Register - Post-write buffer control
 */
/*@{*/
#define AMD_REG_CONFIG_PREFETCH(IoBase) (IoBase + 1)
/*@}*/

/**
 * @brief Controller Configuration Register - Cable detect
 */
/*@{*/
#define AMD_REG_CONFIG_CR(IoBase)       (IoBase + 2)

/** Cable Reporting for drives 0-1 */
#define AMD_CONFIG_CR(Channel)          (0x03 << ((Channel) * 2))
/*@}*/

/**
 * @brief Drive Timing Control Register
 *
 * 0x48 - Secondary drive 1 data DIOR_L/DIOW_L minimum recovery time.
 *        Secondary drive 1 data DIOR_L/DIOW_L active pulse width.
 *
 * 0x49 - Secondary drive 0 data DIOR_L/DIOW_L minimum recovery time.
 *        Secondary drive 0 data DIOR_L/DIOW_L active pulse width
 *
 * 0x4A - Primary drive 1 data DIOR_L/DIOW_L minimum recovery time
 *        Primary drive 1 data DIOR_L/DIOW_L active pulse width.
 *
 * 0x4B - Primary drive 0 data DIOR_L/DIOW_L minimum recovery time.
 *        Primary drive 0 data DIOR_L/DIOW_L active pulse width.
 */
/*@{*/
#define AMD_REG_TIMING_CTRL(IoBase, Channel)      ((IoBase) + 0x08 + (2 - (Channel * 2)))

#define AMD_UDMA_TIME(x)     (x)  ///< Cycle Time
#define AMD_UDMA_EN          0xC0 ///< Ultra-DMA mode enable

#define AMD_TIMING_CTRL(Drive, Value)  ((Value) << ((1 - (Drive)) * 8))
/*@}*/

/**
 * @brief UDMA Extended Timing Control Register
 *
 * 0x50 - Secondary Drive 1.
 * 0x51 - Secondary Drive 0.
 * 0x52 - Primary Drive 1.
 * 0x53 - Primary Drive 0.
 */
/*@{*/
#define AMD_REG_UDMA(IoBase, Channel)    (IoBase + 0x10 + (2 - (Channel * 2)))

#define AMD_UDMA_CTRL(Drive, Value)  ((Value) << ((1 - (Drive)) * 8))

PCIIDEX_PAGED_DATA
static const struct
{
    USHORT VendorID;
    USHORT DeviceID;
    USHORT Flags;
#define HW_FLAGS_UDMA4               0x0001
#define HW_FLAGS_UDMA5               0x0002
#define HW_FLAGS_CHECK_SYSBOARD      0x0004
#define HW_FLAGS_NO_PREFETCH         0x0008
#define HW_FLAGS_SATA                0x0010
#define HW_FLAGS_AHCI                (0x0020 | HW_FLAGS_SATA)
} AmdControllerList[] =
{
    { PCI_VEN_AMD,    PCI_DEV_AMD_756,        HW_FLAGS_UDMA4 },
    { PCI_VEN_AMD,    PCI_DEV_AMD_766,        HW_FLAGS_UDMA5 | HW_FLAGS_NO_PREFETCH },
    { PCI_VEN_AMD,    PCI_DEV_AMD_768,        HW_FLAGS_UDMA5 },
    { PCI_VEN_AMD,    PCI_DEV_AMD_8111,       HW_FLAGS_CHECK_SYSBOARD },
    { PCI_VEN_AMD,    PCI_DEV_AMD_CS5536,     HW_FLAGS_UDMA5 },
    { PCI_VEN_AMD,    PCI_DEV_AMD_CS5536_2,   HW_FLAGS_UDMA5 },
    { PCI_VEN_NVIDIA, PCI_DEV_NFORCE_IDE,     HW_FLAGS_UDMA5 },
    { PCI_VEN_NVIDIA, PCI_DEV_NFORCE2_IDE,    0 },
    { PCI_VEN_NVIDIA, PCI_DEV_NFORCE2_IDE_2,  0 },
    { PCI_VEN_NVIDIA, PCI_DEV_NFORCE3_IDE,    0 },
    { PCI_VEN_NVIDIA, PCI_DEV_NFORCE3_IDE_2,  0 },
    { PCI_VEN_NVIDIA, PCI_DEV_CK804_IDE,      0 },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP04_IDE,      0 },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP51_IDE,      0 },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP55_IDE,      0 },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP61_IDE,      0 },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP65_IDE,      0 },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP67_IDE,      0 },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP73_IDE,      0 },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP77_IDE,      0 },
    { PCI_VEN_NVIDIA, PCI_DEV_NFORCE2_SATA,   HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_NFORCE3_SATA,   HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_NFORCE3_SATA_2, HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_CK804_SATA,     HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_CK804_SATA_2,   HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP04_SATA,     HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP04_SATA_2,   HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP51_SATA,     HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP51_SATA_2,   HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP55_SATA,     HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP55_SATA_2,   HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP61_SATA,     HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP61_SATA_2,   HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP61_SATA_3,   HW_FLAGS_SATA },
    { PCI_VEN_NVIDIA, PCI_DEV_MCP89_SATA,     HW_FLAGS_AHCI },
};

static const struct
{
    UCHAR ActRec;
    UCHAR MinimumPioMode;
} AmdClockSettings[] =
{
    { MAKE_CLOCK(11, 9),  0,           }, // 330 + 270 = 600  PIO 0
    { MAKE_CLOCK(7, 6),   0,           }, // 210 + 180 = 390  PIO 1
    { MAKE_CLOCK(5, 3),   0,           }, // 150 + 90  = 240  PIO 2
    { MAKE_CLOCK(3, 3),   0,           }, // 90  + 90  = 180  PIO 3
    { MAKE_CLOCK(3, 1),   0,           }, // 90  + 30  = 120  PIO 4

    { MAKE_CLOCK(16, 16), PIO_MODE(0), }, // 480 + 480 = 960  SW 0
    { MAKE_CLOCK(8, 8),   PIO_MODE(1), }, // 240 + 240 = 480  SW 1
    { MAKE_CLOCK(5, 3),   PIO_MODE(2), }, // 150 + 90  = 240  SW 2

    { MAKE_CLOCK(8, 8),   PIO_MODE(1), }, // 240 + 240 = 480  MW 0
    { MAKE_CLOCK(3, 2),   PIO_MODE(3), }, // 90  + 60  = 150  MW 1
    { MAKE_CLOCK(3, 1),   PIO_MODE(4), }, // 90  + 30  = 120  MW 2
};

static const UCHAR AmdUdmaSettings[] =
{
    AMD_UDMA_EN | AMD_UDMA_TIME(2), // UDMA 0
    AMD_UDMA_EN | AMD_UDMA_TIME(1), // UDMA 1
    AMD_UDMA_EN | AMD_UDMA_TIME(0), // UDMA 2
    AMD_UDMA_EN | AMD_UDMA_TIME(4), // UDMA 3
    AMD_UDMA_EN | AMD_UDMA_TIME(5), // UDMA 4
    AMD_UDMA_EN | AMD_UDMA_TIME(6), // UDMA 5
    AMD_UDMA_EN | AMD_UDMA_TIME(7), // UDMA 6
};

PCIIDEX_PAGED_DATA
static const ATA_PCI_ENABLE_BITS AmdEnableBits[2] =
{
    { 0x40, 0x02, 0x02 },
    { 0x40, 0x01, 0x01 },
};

PCIIDEX_PAGED_DATA
static const ATA_PCI_ENABLE_BITS NvEnableBits[2] =
{
    { 0x50, 0x02, 0x02 },
    { 0x50, 0x01, 0x01 },
};

/* FUNCTIONS ******************************************************************/

static
ULONG
AmdGetPciConfigIoBase(
    _In_ PATA_CONTROLLER Controller)
{
    if (Controller->Pci.VendorID == PCI_VEN_AMD)
        return AMD_CONFIG_BASE;

    return NV_CONFIG_BASE;
}

static
VOID
AmdSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    PCHANNEL_DATA_PATA ChanData = Controller->Channels[Channel];
    UCHAR IdeConfigReg, IdeTimReg[MAX_IDE_DEVICE], IdeUdmaTimReg[MAX_IDE_DEVICE];
    ULONG i, DmaMode;
    USHORT IdeTimingReg, IdeUdmaTimingReg;
    ULONG IoBase = AmdGetPciConfigIoBase(Controller);

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        ULONG TimingMode;

        /* Init default values, set compatible PIO timings */
        if (IoBase == AMD_CONFIG_BASE)
            IdeUdmaTimReg[i] = AMD_UDMA_TIME(3); // "Slow UDMA mode 0"
        else
            IdeUdmaTimReg[i] = AMD_UDMA_TIME(0);
        IdeTimReg[i] = AmdClockSettings[PIO_MODE(0)].ActRec;

        if (!Device)
            continue;

        TimingMode = Device->PioMode;

        if (Device->DmaMode != PIO_MODE(0))
        {
            /* UDMA works independently of any PIO mode */
            if (Device->DmaMode >= UDMA_MODE(0))
            {
                IdeUdmaTimReg[i] = AmdUdmaSettings[Device->DmaMode - UDMA_MODE(0)];
            }
            else
            {
                /* Select the fastest DMA mode while satisfying PIO timings */
                for (DmaMode = Device->DmaMode; DmaMode > PIO_MODE(4); DmaMode--)
                {
                    if (!(Device->SupportedModes & (1 << DmaMode)))
                        continue;

                    if (!(Device->SupportedModes & (1 << AmdClockSettings[DmaMode].MinimumPioMode)))
                        continue;

                    TimingMode = DmaMode;
                    Device->PioMode = AmdClockSettings[DmaMode].MinimumPioMode;
                    break;
                }

                /* Last entry, disable DMA */
                if (DmaMode == PIO_MODE(4))
                {
                    WARN("CH %lu: Too slow device, disabling DMA\n", Channel);
                    Device->DmaMode = PIO_MODE(0);
                }
            }
        }

        IdeTimReg[i] = AmdClockSettings[TimingMode].ActRec;
    }

    IdeConfigReg = PciRead8(Controller, AMD_REG_CONFIG_PREFETCH(IoBase));
    if (ChanData->HwFlags & HW_FLAGS_NO_PREFETCH)
    {
        /* Errata: IDE Read / Write Prefetch Hangs PCI Bus */
        IdeConfigReg &= ~0xF0;
    }
    else if (Controller->Pci.VendorID == PCI_VEN_AMD)
    {
        IdeConfigReg |= 0xF0; // TODO: ATAPI and non-32 bit data access?
    }
    PciWrite8(Controller, AMD_REG_CONFIG_PREFETCH(IoBase), IdeConfigReg);

    // FIXME: Init other timing registers, merge shared timings

    IdeTimingReg = IdeTimReg[0] << 8 | IdeTimReg[1];
    PciWrite16(Controller, AMD_REG_TIMING_CTRL(IoBase, Channel), IdeTimingReg);

    IdeUdmaTimingReg = IdeUdmaTimReg[0] << 8 | IdeUdmaTimReg[1];
    PciWrite16(Controller, AMD_REG_UDMA(IoBase, Channel), IdeUdmaTimingReg);

    INFO("CH %lu: Config\n"
         "IDETIM  %04X\n"
         "UDMATIM %04X\n",
         Channel, IdeTimingReg, IdeUdmaTimingReg);
}

static
BOOLEAN
CODE_SEG("PAGE")
NvHasUdmaCable(
    _In_ USHORT IdeUdmaTimingReg,
    _In_ ULONG Drive)
{
    PAGED_CODE();

    /* UDMA was disabled by BIOS */
    if ((AMD_UDMA_CTRL(Drive, IdeUdmaTimingReg) & AMD_UDMA_EN) != AMD_UDMA_EN)
        return FALSE;

    /* Check clock settings, see if UDMA3 or higher mode is active */
    return !!((AMD_UDMA_CTRL(Drive, IdeUdmaTimingReg) & 0x7) & AMD_UDMA_TIME(4));
}

CODE_SEG("PAGE")
NTSTATUS
AmdGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i, HwFlags, SupportedMode;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_AMD || Controller->Pci.VendorID == PCI_VEN_NVIDIA);

    for (i = 0; i < RTL_NUMBER_OF(AmdControllerList); ++i)
    {
        HwFlags = AmdControllerList[i].Flags;

        if ((AmdControllerList[i].VendorID == Controller->Pci.VendorID) &&
            (AmdControllerList[i].DeviceID == Controller->Pci.DeviceID))
        {
            break;
        }
    }
    if (i == RTL_NUMBER_OF(AmdControllerList))
        return STATUS_NOT_SUPPORTED;

    if (HwFlags & HW_FLAGS_AHCI)
    {
        if (Controller->Pci.SubClass == PCI_SUBCLASS_MSC_AHCI_CTLR)
            return STATUS_NOT_SUPPORTED;
    }

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    if (HwFlags & HW_FLAGS_SATA)
    {
        SupportedMode = SATA_ALL;
    }
    else
    {
        SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_ALL;

        if (HwFlags & HW_FLAGS_CHECK_SYSBOARD)
        {
            if (Controller->Pci.SubSystemID == PCI_SUBSYSTEM_AMD_SERENADE)
                SupportedMode &= ~UDMA_MODE6;
        }
        else
        {
            if (HwFlags & HW_FLAGS_UDMA5)
                SupportedMode &= ~UDMA_MODE6;
            else if (HwFlags & HW_FLAGS_UDMA4)
                SupportedMode &= ~UDMA_MODES(5, 6);
        }

        if (Controller->Pci.VendorID == PCI_VEN_AMD)
            Controller->ChannelEnableBits = AmdEnableBits;
        else
            Controller->ChannelEnableBits = NvEnableBits;
    }

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        ChanData->HwFlags = HwFlags;
        ChanData->ChanInfo |= CHANNEL_FLAG_FLAG_IO32;
        ChanData->Info.TransferModeSupported = SupportedMode;

        if (HwFlags & HW_FLAGS_SATA)
        {
            ChanData->ChanInfo |= CHANNEL_FLAG_NO_SLAVE;
            ChanData->SetTransferMode = SataSetTransferMode;
        }
        else
        {
            ChanData->SetTransferMode = AmdSetTransferMode;

            /* Check for 80-conductor cable */
            if (Controller->Pci.VendorID == PCI_VEN_AMD)
            {
                UCHAR IdeConfigReg = PciRead8(Controller, AMD_REG_CONFIG_CR(AMD_CONFIG_BASE));

                if (!(IdeConfigReg & AMD_REG_CONFIG_CR(i)))
                {
                    WARN("CH %lu: BIOS detected 40-conductor cable\n", i);
                    ChanData->Info.TransferModeSupported &= ~UDMA_80C_ALL;
                }
            }
            else
            {
                USHORT IdeUdmaTimingReg = PciRead16(Controller, AMD_REG_UDMA(NV_CONFIG_BASE, i));

                if (!NvHasUdmaCable(IdeUdmaTimingReg, 0) && !NvHasUdmaCable(IdeUdmaTimingReg, 1))
                {
                    WARN("CH %lu: BIOS hasn't selected mode faster than UDMA 2, "
                         "assume 40-conductor cable\n",
                         ChanData->Channel);
                    ChanData->Info.TransferModeSupported &= ~UDMA_80C_ALL;
                }
            }
        }
    }

    return STATUS_SUCCESS;
}
