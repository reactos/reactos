/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AMD ATA controller minidriver
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

#define AMD_PCI_CLOCK    30000

/**
 * @brief Controller Configuration Register - Post-write buffer control
 */
/*@{*/
#define AMD_REG_CONFIG_PREFETCH(IoBase) ((IoBase) + 1)

#define AMD_CONFIG_PREFETCH(Channel) (0xC0 >> ((Channel) * 2))
/*@}*/

/**
 * @brief Controller Configuration Register - Cable detect
 */
/*@{*/
#define AMD_REG_CONFIG_CR(IoBase)       ((IoBase) + 2)

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
#define AMD_REG_TIMING_CTRL(IoBase)      ((IoBase) + 0x08)

#define AMD_UDMA_TIME(x)     (x)  ///< Cycle Time
#define AMD_UDMA_EN          0xC0 ///< Ultra-DMA mode enable
/*@}*/

/**
 * @brief Cycle Time and Address Setup Time Register
 *
 * 0x4C - Secondary drive 1 address setup time.
 *        Secondary drive 0 address setup time.
 *        Primary drive 1 address setup time.
 *        Primary drive 0 address setup time.
 *
 * 0x4D - Reserved.
 *
 * 0x4E - Secondary command/control DIOR_L/DIOW_L recovery time.
 *        Secondary command/control DIOR_L/DIOW_L active pulse width.
 *
 * 0x4F - Primary command/control DIOR_L/DIOW_L recovery time.
 *        Primary command/control DIOR_L/DIOW_L active pulse width.
 */
/*@{*/
#define AMD_REG_ADDRESS_SETUP(IoBase)    ((IoBase) + 0x0C)
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
#define AMD_REG_UDMA(IoBase, Channel)    ((IoBase) + 0x10 + (2 - (Channel * 2)))

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
    { PCI_VEN_NVIDIA, PCI_DEV_MCP89_SATA,     HW_FLAGS_SATA },
};

static const UCHAR AmdUdmaSettings[] =
{
    AMD_UDMA_EN | AMD_UDMA_TIME(2), // 0
    AMD_UDMA_EN | AMD_UDMA_TIME(1), // 1
    AMD_UDMA_EN | AMD_UDMA_TIME(0), // 2
    AMD_UDMA_EN | AMD_UDMA_TIME(4), // 3
    AMD_UDMA_EN | AMD_UDMA_TIME(5), // 4
    AMD_UDMA_EN | AMD_UDMA_TIME(6), // 5
    AMD_UDMA_EN | AMD_UDMA_TIME(7), // 6
};

static const ULONG AmdTimingControlShift[MAX_IDE_CHANNEL][MAX_IDE_DEVICE] =
{
    // M  S
    { 24, 16 }, // Pri
    { 8,  0  }, // Sec
};
static const ULONG AmdAddressSetupShift[MAX_IDE_CHANNEL][MAX_IDE_DEVICE] =
{
    // M  S
    { 6, 4 }, // Pri
    { 2, 0 }, // Sec
};
static const ULONG AmdPortTimShift[MAX_IDE_CHANNEL] =
{
    24, // Pri
    16  // Sec
};

PCIIDEX_PAGED_DATA
static const ATA_PCI_ENABLE_BITS AmdEnableBits[MAX_IDE_CHANNEL] =
{
    { AMD_CONFIG_BASE, 0x02, 0x02 },
    { AMD_CONFIG_BASE, 0x01, 0x01 },
};

PCIIDEX_PAGED_DATA
static const ATA_PCI_ENABLE_BITS NvEnableBits[MAX_IDE_CHANNEL] =
{
    { NV_CONFIG_BASE, 0x02, 0x02 },
    { NV_CONFIG_BASE, 0x01, 0x01 },
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
AmdEnablePostedWriteBuffer(
    _In_ PATA_CONTROLLER Controller,
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList,
    _In_ ULONG IoBase)
{
    UCHAR ConfigReg;
    ULONG i;

    ConfigReg = PciRead8(Controller, AMD_REG_CONFIG_PREFETCH(IoBase));
    if (ChanData->HwFlags & HW_FLAGS_NO_PREFETCH)
    {
        /* Errata: IDE Read / Write Prefetch Hangs PCI Bus */
        ConfigReg &= ~(AMD_CONFIG_PREFETCH(0) | AMD_CONFIG_PREFETCH(1));
    }
    else
    {
        ConfigReg |= AMD_CONFIG_PREFETCH(Channel);

        for (i = 0; i < MAX_IDE_DEVICE; ++i)
        {
            PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];

            if (Device && !Device->IsFixedDisk)
            {
                ConfigReg &= ~AMD_CONFIG_PREFETCH(Channel);
                break;
            }
        }
    }
    PciWrite8(Controller, AMD_REG_CONFIG_PREFETCH(IoBase), ConfigReg);
}

static
VOID
AmdSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    PCHANNEL_DATA_PATA ChanData = Controller->Channels[Channel];
    const ULONG IoBase = AmdGetPciConfigIoBase(Controller);
    ATA_TIMING DeviceTimings[MAX_IDE_DEVICE];
    ULONG i, DriveTimReg, PortTimReg, UdmaTimReg;

    AtaSelectTimings(DeviceList, DeviceTimings, AMD_PCI_CLOCK, SHARED_CMD_TIMINGS);

    DriveTimReg = PciRead32(Controller, AMD_REG_TIMING_CTRL(IoBase));
    PortTimReg = PciRead32(Controller, AMD_REG_ADDRESS_SETUP(IoBase));
    UdmaTimReg = PciRead32(Controller, AMD_REG_UDMA(IoBase, 1));

    INFO("CH %lu: Config (before)\n"
         "DRV  %08lX\n"
         "PORT %08lX\n"
         "UDMA %08lX\n"
         "CFG  %08lX\n",
         Channel, DriveTimReg, PortTimReg, UdmaTimReg, PciRead32(Controller, IoBase));

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        PATA_TIMING Timing = &DeviceTimings[i];
        ULONG Value;

        /* UDMA timings */
        if (Device && (Device->DmaMode >= UDMA_MODE(0)))
        {
            Value = AmdUdmaSettings[Device->DmaMode - UDMA_MODE(0)];
        }
        else
        {
            /* Set "Slow UDMA mode 0" by default */
            if (Controller->Pci.VendorID == PCI_VEN_AMD)
                Value = AMD_UDMA_TIME(3);
            else
                Value = 0;
        }
        UdmaTimReg &= ~(0xFF << AmdTimingControlShift[Channel][i]);
        UdmaTimReg |= Value << AmdTimingControlShift[Channel][i];

        if (!Device)
            continue;

        /* PIO and DMA timings */
        Timing->AddressSetup = CLAMP_TIMING(Timing->AddressSetup, 1, 4) - 1;
        Timing->CmdRecovery  = CLAMP_TIMING(Timing->CmdRecovery, 1, 16) - 1;
        Timing->CmdActive    = CLAMP_TIMING(Timing->CmdActive, 1, 16) - 1;
        Timing->DataRecovery = CLAMP_TIMING(Timing->DataRecovery, 1, 16) - 1;
        Timing->DataActive   = CLAMP_TIMING(Timing->DataActive, 1, 16) - 1;

        Value = Timing->AddressSetup;
        PortTimReg &= ~(0x03 << AmdAddressSetupShift[Channel][i]);
        PortTimReg |= Value << AmdAddressSetupShift[Channel][i];

        Value = (Timing->CmdActive << 4) | Timing->CmdRecovery;
        PortTimReg &= ~(0xFF << AmdPortTimShift[Channel]);
        PortTimReg |= Value << AmdPortTimShift[Channel];

        Value = (Timing->DataActive << 4) | Timing->DataRecovery;
        DriveTimReg &= ~(0xFF << AmdTimingControlShift[Channel][i]);
        DriveTimReg |= Value << AmdTimingControlShift[Channel][i];
    }

    AmdEnablePostedWriteBuffer(Controller, ChanData, Channel, DeviceList, IoBase);

    PciWrite32(Controller, AMD_REG_TIMING_CTRL(IoBase), DriveTimReg);
    PciWrite32(Controller, AMD_REG_ADDRESS_SETUP(IoBase), PortTimReg);
    PciWrite32(Controller, AMD_REG_UDMA(IoBase, 1), UdmaTimReg);

    INFO("CH %lu: Config (after)\n"
         "DRV  %08lX\n"
         "PORT %08lX\n"
         "UDMA %08lX\n"
         "CFG  %08lX\n",
         Channel, DriveTimReg, PortTimReg, UdmaTimReg, PciRead32(Controller, IoBase));
}

static
VOID
AmdControllerStart(
    _In_ PATA_CONTROLLER Controller)
{
    UCHAR ConfigReg;

    ASSERT(Controller->Pci.VendorID == PCI_VEN_AMD);

    /* Initialize controller before issuing the identify command */
    ConfigReg = PciRead8(Controller, AMD_REG_CONFIG_PREFETCH(AMD_CONFIG_BASE));
    ConfigReg &= ~(AMD_CONFIG_PREFETCH(0) | AMD_CONFIG_PREFETCH(1));
    PciWrite8(Controller, AMD_REG_CONFIG_PREFETCH(AMD_CONFIG_BASE), ConfigReg);
}

static
BOOLEAN
CODE_SEG("PAGE")
NvHasUdmaCable(
    _In_ USHORT UdmaTimReg,
    _In_ ULONG Drive)
{
    PAGED_CODE();

    /* UDMA was disabled by BIOS */
    if ((AMD_UDMA_CTRL(Drive, UdmaTimReg) & AMD_UDMA_EN) != AMD_UDMA_EN)
        return FALSE;

    /* Check clock settings, see if UDMA3 or higher mode is active */
    return !!((AMD_UDMA_CTRL(Drive, UdmaTimReg) & 0x7) & AMD_UDMA_TIME(4));
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

        if ((Controller->Pci.VendorID == AmdControllerList[i].VendorID) &&
            (Controller->Pci.DeviceID == AmdControllerList[i].DeviceID))
        {
            break;
        }
    }
    if (i == RTL_NUMBER_OF(AmdControllerList))
        return STATUS_NO_MATCH;

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
        {
            Controller->ChannelEnableBits = AmdEnableBits;
            Controller->Start = AmdControllerStart;
        }
        else
        {
            Controller->ChannelEnableBits = NvEnableBits;
        }
    }

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        ChanData->HwFlags = HwFlags;
        ChanData->TransferModeSupported = SupportedMode;

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
                UCHAR ConfigReg = PciRead8(Controller, AMD_REG_CONFIG_CR(AMD_CONFIG_BASE));

                if (!(ConfigReg & AMD_REG_CONFIG_CR(i)))
                {
                    INFO("CH %lu: BIOS detected 40-conductor cable\n", i);
                    ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
                }
            }
            else
            {
                USHORT UdmaTimReg = PciRead16(Controller, AMD_REG_UDMA(NV_CONFIG_BASE, i));

                if (!NvHasUdmaCable(UdmaTimReg, 0) && !NvHasUdmaCable(UdmaTimReg, 1))
                {
                    INFO("CH %lu: BIOS hasn't selected mode faster than UDMA 2, "
                         "assume 40-conductor cable\n",
                         ChanData->Channel);
                    ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
                }
            }
        }
    }

    return STATUS_SUCCESS;
}
