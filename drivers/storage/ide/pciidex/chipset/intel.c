/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Intel ATA controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 *
 * REFERENCES:  For more details, see Intel order no 298600-004
 */

/*
 * 0x1230 PIIX - no separate timings
 * 0x1234 MPIIX - no separate timings, no PCI I/O resources, bridge device, single-channel
 * 0x811A SCH - single-channel, no enable bits
 * 0x2652, 0x2653 ICH6 - share the same PCI ID between AHCI and PATA mode
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_PIIX_82371FB                0x1230
#define PCI_DEV_MPIIX_82371MX               0x1234

#define PCI_DEV_PIIX3_82371SB               0x7010

#define PCI_DEV_PIIX4_82371AB               0x7111
#define PCI_DEV_PIIX4E_82372FB              0x7601
#define PCI_DEV_PIIX4E_82443MX              0x7199
#define PCI_DEV_PIIX4E_82451NX              0x84CA

#define PCI_DEV_ICH_82801AA                 0x2411

#define PCI_DEV_ICH0_82801AB                0x2421

#define PCI_DEV_ICH2_82801BAM               0x244A
#define PCI_DEV_ICH2_82801BA                0x244B

#define PCI_DEV_C_ICH_82801E                0x245B

#define PCI_DEV_ICH3_M_82801CAM             0x248A
#define PCI_DEV_ICH3_S_82801CA              0x248B

#define PCI_DEV_ICH4_L_82801DBL             0x24C1
#define PCI_DEV_ICH4_M_82801DBM             0x24CA
#define PCI_DEV_ICH4_82801DB                0x24CB

#define PCI_DEV_ICH5_82801EB_SATA           0x24D1
#define PCI_DEV_ICH5_82801EB_IDE            0x24DB
#define PCI_DEV_ICH5_R_82801ER              0x24DF
#define PCI_DEV_ICH5_6300ESB_IDE            0x25A2
#define PCI_DEV_ICH5_6300ESB_SATA           0x25A3
#define PCI_DEV_ICH5_6300ESB_RAID           0x25B0

#define PCI_DEV_ICH6_82801FB_SATA           0x2651
#define PCI_DEV_ICH6_R_82801FB              0x2652
#define PCI_DEV_ICH6_M_82801FBM             0x2653
#define PCI_DEV_ICH6_82801FB_IDE            0x266F

#define PCI_DEV_ESB2_63XXESB                0x2680

#define PCI_DEV_ICH7_6321ESB                0x269E
#define PCI_DEV_ICH7_82801GB                0x27C0
#define PCI_DEV_ICH7_M_82801GBM             0x27C4
#define PCI_DEV_ICH7_82801G                 0x27DF

#define PCI_DEV_ICH8_82801H                 0x2820
#define PCI_DEV_ICH8_R_82801HR              0x2825
#define PCI_DEV_ICH8_M_82801HM              0x2828
#define PCI_DEV_ICH8_M_82801HBM             0x2850

#define PCI_DEV_ICH9_R_82801IR_1            0x2920
#define PCI_DEV_ICH9_R_82801IR_2            0x2921
#define PCI_DEV_ICH9_82801I                 0x2926
#define PCI_DEV_ICH9_M_82801IBM_1           0x2928
#define PCI_DEV_ICH9_M_82801IBM_2           0x292D
#define PCI_DEV_ICH9_M_82801IBM_3           0x292E

#define PCI_DEV_ICH10_82801JD_1             0x3A00
#define PCI_DEV_ICH10_82801JD_2             0x3A06
#define PCI_DEV_ICH10_82801JI_1             0x3A20
#define PCI_DEV_ICH10_82801JI_2             0x3A26

#define PCI_DEV_SCH_ATOM_Z5XX               0x811A

#define PCI_DEV_EP80579                     0x5028

#define PCI_DEV_PCH_5SERIES_1               0x3B20
#define PCI_DEV_PCH_5SERIES_2               0x3B21
#define PCI_DEV_PCH_5SERIES_3               0x3B26
#define PCI_DEV_PCH_5SERIES_4               0x3B28
#define PCI_DEV_PCH_5SERIES_5               0x3B2D
#define PCI_DEV_PCH_5SERIES_6               0x3B2E

#define PCI_DEV_PCH_6SERIES_1               0x1C00
#define PCI_DEV_PCH_6SERIES_2               0x1C01
#define PCI_DEV_PCH_6SERIES_3               0x1C08
#define PCI_DEV_PCH_6SERIES_4               0x1C09

#define PCI_DEV_PCH_7SERIES_1               0x1E00
#define PCI_DEV_PCH_7SERIES_2               0x1E01
#define PCI_DEV_PCH_7SERIES_3               0x1E08
#define PCI_DEV_PCH_7SERIES_4               0x1E09

#define PCI_DEV_PCH_X79_1                   0x1D00
#define PCI_DEV_PCH_X79_2                   0x1D08

#define PCI_DEV_PCH_8900_1                  0x2326
#define PCI_DEV_PCH_8900_2                  0x23A6

#define PCI_DEV_ATOM_C2000_1                0x1F20
#define PCI_DEV_ATOM_C2000_2                0x1F21
#define PCI_DEV_ATOM_C2000_3                0x1F30
#define PCI_DEV_ATOM_C2000_4                0x1F31

#define PCI_DEV_PCH_8SERIES_1               0x8C00
#define PCI_DEV_PCH_8SERIES_2               0x8C01
#define PCI_DEV_PCH_8SERIES_3               0x8C08
#define PCI_DEV_PCH_8SERIES_4               0x8C09
#define PCI_DEV_PCH_8SERIES_5               0x9C00
#define PCI_DEV_PCH_8SERIES_6               0x9C01
#define PCI_DEV_PCH_8SERIES_7               0x9C08
#define PCI_DEV_PCH_8SERIES_8               0x9C09

#define PCI_DEV_ATOM_E3800_1                0x0F20
#define PCI_DEV_ATOM_E3800_2                0x0F21

#define PCI_DEV_PCH_X99_1                   0x8D00
#define PCI_DEV_PCH_X99_2                   0x8D08
#define PCI_DEV_PCH_X99_3                   0x8D60
#define PCI_DEV_PCH_X99_4                   0x8D68

#define PCI_DEV_PCH_9SERIES_1               0x8C80
#define PCI_DEV_PCH_9SERIES_2               0x8C81
#define PCI_DEV_PCH_9SERIES_3               0x8C88
#define PCI_DEV_PCH_9SERIES_4               0x8C89

#define PCI_DEV_BRIDGE_450KX                0x84C4
#define PCI_DEV_BRIDGE_450NX                0x84CB

#define BRIDGE_450KX_REV_B0       0x04

#define BRIDGE_450NX_REV_B0       0x00
#define BRIDGE_450NX_REV_B1       0x02
#define BRIDGE_450NX_REV_C0       0x04

/**
 * @brief PXB configuration register (450NX)
 */
/*@{*/
#define PXB_REG_CONFIG            0x40

#define PXB_CONFIG_BUF_RESTREAM   0x0040 ///< Re-streaming Buffer Enable
#define PXB_CONFIG_PCI_BUS_LOCK   0x4000 ///< PCI Bus Lock Enable
/*@}*/

/**
 * @brief IDE timing register (PIIX, MPIIX, PIIX3/4, ICH)
 *
 * 0x40-0x41 - primary, 0x42-0x43 - secondary.
 * 0x6C-0x6D - primary and secondary (MPIIX).
 */
/*@{*/
#define PIIX_REG_IDETIM(Channel)  (0x40 + ((Channel) * 2))
#define MPIIX_REG_IDETIM          0x6C

#define PIIX_IDETIM_TIME(Drive) (0x0001 << (4 * (Drive))) ///< Fast Timing Bank Enable
#define PIIX_IDETIM_IE(Drive)   (0x0002 << (4 * (Drive))) ///< IORDY Sample Point Enable
#define PIIX_IDETIM_PPE(Drive)  (0x0004 << (4 * (Drive))) ///< Prefetch/Posting Enable
#define PIIX_IDETIM_DTE(Drive)  (0x0008 << (4 * (Drive))) ///< DMA Timing Only (except for MPIIX)

#define PIIX_IDETIM_RCT_MASK    0x0300 ///< Recovery Time
#define PIIX_IDETIM_RSV_MASK    0x0C00 ///< Reserved
#define PIIX_IDETIM_ISP_MASK    0x3000 ///< IORDY Sample Point
#define PIIX_IDETIM_SITRE       0x4000 ///< Slave IDE Timing Register Enable (PIIX3/4, ICH)
#define PIIX_IDETIM_SECONDARY   0x4000 ///< IDE Decode Enable for secondary channel (MPIIX only)
#define PIIX_IDETIM_IDE         0x8000 ///< IDE Decode Enable

#define PIIX_IDETIM_RCT(Value) ((Value) << 8) ///< Recovery Time
#define PIIX_IDETIM_ISP(Value) ((Value) << 12) ///< IORDY Sample Point
#define PIIX_IDETIM_SETTINGS(Drive, Value) ((Value) << (4 * (Drive))) ///< TIME, IE, PPE, DTE
/*@}*/

/**
 * @brief Slave IDE timing register (PIIX3/4, ICH)
 */
/*@{*/
#define PIIX_REG_SIDETIM   0x44

#define PIIX_SIDETIM_ISPRCT_MASK(Channel) (0x0F << ((Channel) * 4))
#define PIIX_SIDETIM_RCT(Channel, Value) ((Value) << ((Channel) * 4)) ///< Recovery Time
#define PIIX_SIDETIM_ISP(Channel, Value) ((Value) << (((Channel) * 4) + 2)) ///< IORDY Sample Point
/*@}*/

/**
 * @brief Ultra DMA control register (PIIX4, ICH)
 */
/*@{*/
#define PIIX_REG_UDMACTL       0x48

#define PIIX_UDMACTL_EN_MASK(Channel)   (0x03 << ((Channel) * 2)) ///< Ultra DMA Mode Enable

/** Ultra DMA Mode Enable */
#define PIIX_UDMACTL_EN(Channel, Drive)    (0x01 << ((Channel) * 2 + (Drive)))
/*@}*/

/**
 * @brief Ultra DMA timing register (PIIX4, ICH)
 *
 * 0x4A - primary, 0x4B - secondary.
 */
/*@{*/
#define PIIX_REG_UDMATIM(Channel)      (0x4A + (Channel))

#define PIIX_UDMATIM_CT_MASK    0x33 ///< Cycle Time and Ready to Pause time

/** Cycle Time and Ready to Pause time */
#define PIIX_UDMATIM_CT(Drive, Value)  ((Value) << ((Drive) * 4))
/*@}*/

/**
 * @brief IDE config register (ICH)
 */
/*@{*/
#define PIIX_REG_CONFIG          0x54

#define PIIX_CONFIG_KEEP_MASK    0x0FF0 ///< Save the reserved, cable, and ping-pong bits

/** Cable Reporting for drives 0-1 */
#define PIIX_CONFIG_CR(Channel)                   (0x0030 << ((Channel) * 2))
/** 66 MHz Clock */
#define PIIX_CONFIG_CLOCK_UDMA66(Channel, Drive)  (0x0001 << (((Channel) * 2) + Drive))
/** 100 MHz Clock */
#define PIIX_CONFIG_CLOCK_UDMA100(Channel, Drive) (0x1000 << (((Channel) * 2) + Drive))
/** PIO Ping-Pong enable */
#define PIIX_CONFIG_WR_PING_PONG                  0x0400
/*@}*/

/**
 * @brief Device 0/1 Timing Register (SCH)
 */
/*@{*/
#define SCH_REG_DTIM(Drive)   (0x80 + (Drive) * 4)

#define SCH_DTIM_PM_MASK      0x00000007 ///< PIO Mode
#define SCH_DTIM_MDM_MASK     0x00000300 ///< Mutli-word DMA Mode
#define SCH_DTIM_UDM_MASK     0x00070000 ///< Ultra DMA Mode
#define SCH_DTIM_PPE          0x40000000 ///< Prefetch/Post Enable
#define SCH_DTIM_USD          0x80000000 ///< Use Synchronous DMA

#define SCH_DTIM_PM(Mode)     (Mode)
#define SCH_DTIM_MDM(Mode)    ((Mode) << 8)
#define SCH_DTIM_UDM(Mode)    ((Mode) << 16)
/*@}*/

#define HW_FLAGS_TYPE_MASK      0x000F
#define HW_FLAGS_DISABLE_DMA    0x0010
#define HW_FLAGS_HAS_CFG_REG    0x0080
#define HW_FLAGS_HAS_UDMA_REG   0x8000

typedef struct _INTEL_CONTROLLER_INFO
{
    USHORT DeviceID;
    USHORT Type;
#define TYPE_MPIIX      0
#define TYPE_PIIX       1
#define TYPE_PIIX3      2
#define TYPE_PIIX4      (3 | HW_FLAGS_HAS_UDMA_REG)
#define TYPE_ICH        (4 | HW_FLAGS_HAS_UDMA_REG | HW_FLAGS_HAS_CFG_REG)
#define TYPE_SATA       5
#define TYPE_SCH        6
} INTEL_CONTROLLER_INFO, *PINTEL_CONTROLLER_INFO;

typedef struct _INTEL_HW_EXTENSION
{
    ULONG Register;
    USHORT CurrentTiming;
    struct
    {
        USHORT Timing[2];
    } Device[MAX_IDE_DEVICE];
} INTEL_HW_EXTENSION, *PINTEL_HW_EXTENSION;

PCIIDEX_PAGED_DATA
static const INTEL_CONTROLLER_INFO IntelControllerList[] =
{
    { PCI_DEV_PIIX_82371FB,      TYPE_PIIX,    },
    { PCI_DEV_MPIIX_82371MX,     TYPE_MPIIX,   },
    { PCI_DEV_PIIX3_82371SB,     TYPE_PIIX3,   },
    { PCI_DEV_PIIX4_82371AB,     TYPE_PIIX4,   },
    { PCI_DEV_PIIX4E_82372FB,    TYPE_PIIX4,   },
    { PCI_DEV_PIIX4E_82443MX,    TYPE_PIIX4,   },
    { PCI_DEV_PIIX4E_82451NX,    TYPE_PIIX4,   },
    { PCI_DEV_ICH_82801AA,       TYPE_ICH,     },
    { PCI_DEV_ICH0_82801AB,      TYPE_ICH,     },
    { PCI_DEV_ICH2_82801BAM,     TYPE_ICH,     },
    { PCI_DEV_ICH2_82801BA,      TYPE_ICH,     },
    { PCI_DEV_C_ICH_82801E,      TYPE_ICH,     },
    { PCI_DEV_ICH3_M_82801CAM,   TYPE_ICH,     },
    { PCI_DEV_ICH3_S_82801CA,    TYPE_ICH,     },
    { PCI_DEV_ICH4_L_82801DBL,   TYPE_ICH,     },
    { PCI_DEV_ICH4_M_82801DBM,   TYPE_ICH,     },
    { PCI_DEV_ICH4_82801DB,      TYPE_ICH,     },
    { PCI_DEV_ICH5_82801EB_SATA, TYPE_SATA,    },
    { PCI_DEV_ICH5_82801EB_IDE,  TYPE_ICH,     },
    { PCI_DEV_ICH5_R_82801ER,    TYPE_SATA,    },
    { PCI_DEV_ICH5_6300ESB_IDE,  TYPE_ICH,     },
    { PCI_DEV_ICH5_6300ESB_SATA, TYPE_SATA,    },
    { PCI_DEV_ICH5_6300ESB_RAID, TYPE_SATA,    },
    { PCI_DEV_ICH6_82801FB_SATA, TYPE_SATA,    },
    { PCI_DEV_ICH6_R_82801FB,    TYPE_SATA,    },
    { PCI_DEV_ICH6_M_82801FBM,   TYPE_SATA,    },
    { PCI_DEV_ICH6_82801FB_IDE,  TYPE_ICH,     },
    { PCI_DEV_ESB2_63XXESB,      TYPE_SATA,    },
    { PCI_DEV_ICH7_6321ESB,      TYPE_ICH,     },
    { PCI_DEV_ICH7_82801GB,      TYPE_SATA,    },
    { PCI_DEV_ICH7_M_82801GBM,   TYPE_SATA,    },
    { PCI_DEV_ICH7_82801G,       TYPE_ICH,     },
    { PCI_DEV_ICH8_82801H,       TYPE_SATA,    },
    { PCI_DEV_ICH8_R_82801HR,    TYPE_SATA,    },
    { PCI_DEV_ICH8_M_82801HM,    TYPE_SATA,    },
    { PCI_DEV_ICH8_M_82801HBM,   TYPE_ICH,     },
    { PCI_DEV_ICH9_R_82801IR_1,  TYPE_SATA,    },
    { PCI_DEV_ICH9_R_82801IR_2,  TYPE_SATA,    },
    { PCI_DEV_ICH9_82801I,       TYPE_SATA,    },
    { PCI_DEV_ICH9_M_82801IBM_1, TYPE_SATA,    },
    { PCI_DEV_ICH9_M_82801IBM_2, TYPE_SATA,    },
    { PCI_DEV_ICH9_M_82801IBM_3, TYPE_SATA,    },
    { PCI_DEV_ICH10_82801JD_1,   TYPE_SATA,    },
    { PCI_DEV_ICH10_82801JD_2,   TYPE_SATA,    },
    { PCI_DEV_ICH10_82801JI_1,   TYPE_SATA,    },
    { PCI_DEV_ICH10_82801JI_2,   TYPE_SATA,    },
    { PCI_DEV_SCH_ATOM_Z5XX,     TYPE_SCH,     },
    { PCI_DEV_EP80579,           TYPE_SATA,    },
    { PCI_DEV_PCH_5SERIES_1,     TYPE_SATA,    },
    { PCI_DEV_PCH_5SERIES_2,     TYPE_SATA,    },
    { PCI_DEV_PCH_5SERIES_3,     TYPE_SATA,    },
    { PCI_DEV_PCH_5SERIES_4,     TYPE_SATA,    },
    { PCI_DEV_PCH_5SERIES_5,     TYPE_SATA,    },
    { PCI_DEV_PCH_5SERIES_6,     TYPE_SATA,    },
    { PCI_DEV_PCH_6SERIES_1,     TYPE_SATA,    },
    { PCI_DEV_PCH_6SERIES_2,     TYPE_SATA,    },
    { PCI_DEV_PCH_6SERIES_3,     TYPE_SATA,    },
    { PCI_DEV_PCH_6SERIES_4,     TYPE_SATA,    },
    { PCI_DEV_PCH_7SERIES_1,     TYPE_SATA,    },
    { PCI_DEV_PCH_7SERIES_2,     TYPE_SATA,    },
    { PCI_DEV_PCH_7SERIES_3,     TYPE_SATA,    },
    { PCI_DEV_PCH_7SERIES_4,     TYPE_SATA,    },
    { PCI_DEV_PCH_X79_1,         TYPE_SATA,    },
    { PCI_DEV_PCH_X79_2,         TYPE_SATA,    },
    { PCI_DEV_PCH_8900_1,        TYPE_SATA,    },
    { PCI_DEV_PCH_8900_2,        TYPE_SATA,    },
    { PCI_DEV_ATOM_C2000_1,      TYPE_SATA,    },
    { PCI_DEV_ATOM_C2000_2,      TYPE_SATA,    },
    { PCI_DEV_ATOM_C2000_3,      TYPE_SATA,    },
    { PCI_DEV_ATOM_C2000_4,      TYPE_SATA,    },
    { PCI_DEV_PCH_8SERIES_1,     TYPE_SATA,    },
    { PCI_DEV_PCH_8SERIES_2,     TYPE_SATA,    },
    { PCI_DEV_PCH_8SERIES_3,     TYPE_SATA,    },
    { PCI_DEV_PCH_8SERIES_4,     TYPE_SATA,    },
    { PCI_DEV_PCH_8SERIES_5,     TYPE_SATA,    },
    { PCI_DEV_PCH_8SERIES_6,     TYPE_SATA,    },
    { PCI_DEV_PCH_8SERIES_7,     TYPE_SATA,    },
    { PCI_DEV_PCH_8SERIES_8,     TYPE_SATA,    },
    { PCI_DEV_ATOM_E3800_1,      TYPE_SATA,    },
    { PCI_DEV_ATOM_E3800_2,      TYPE_SATA,    },
    { PCI_DEV_PCH_X99_1,         TYPE_SATA,    },
    { PCI_DEV_PCH_X99_2,         TYPE_SATA,    },
    { PCI_DEV_PCH_X99_3,         TYPE_SATA,    },
    { PCI_DEV_PCH_X99_4,         TYPE_SATA,    },
    { PCI_DEV_PCH_9SERIES_1,     TYPE_SATA,    },
    { PCI_DEV_PCH_9SERIES_2,     TYPE_SATA,    },
    { PCI_DEV_PCH_9SERIES_3,     TYPE_SATA,    },
    { PCI_DEV_PCH_9SERIES_4,     TYPE_SATA,    },
};

PCIIDEX_PAGED_DATA
static const ATA_PCI_ENABLE_BITS IntelPiixEnableBits[MAX_IDE_CHANNEL] =
{
    { 0x41, 0x80, 0x80 },
    { 0x43, 0x80, 0x80 },
};

PCIIDEX_PAGED_DATA
static const ATA_PCI_ENABLE_BITS IntelMpiixEnableBits[MAX_IDE_CHANNEL] =
{
    { 0x6D, 0xC0, 0x80 },
    { 0x6D, 0xC0, 0xC0 },
};

static const UCHAR IntelClockSettings[5][2] =
{
    // ISP, RCT
    { 0, 0 }, // Mode 0  ISP = 5  RCT = 4  NOTE: Some old chips do ISP 6 and RCT 14 = 600 ns
    { 0, 0 }, // Mode 1  Ditto
    { 1, 0 }, // Mode 2  ISP = 4  RCT = 4
    { 2, 1 }, // Mode 3  ISP = 3  RCT = 3
    { 2, 3 }, // Mode 4  ISP = 3  RCT = 1
};

/** @sa IntelClockSettings */
static const ULONG IntelTimingModeToCycleTime[5] =
{
    900, // Mode 0  NOTE: Some old chips do ISP 6 and RCT 14 = 600 ns
    900, // Mode 1  Ditto
    240, // Mode 2
    180, // Mode 3
    120, // Mode 4
};

static const UCHAR IntelModeSettings[] =
{
    0,                                       // Mode 0
    0,                                       // Mode 1
    PIIX_IDETIM_TIME(0),                     // Mode 2
    PIIX_IDETIM_TIME(0) | PIIX_IDETIM_IE(0), // Mode 3
    PIIX_IDETIM_TIME(0) | PIIX_IDETIM_IE(0), // Mode 4
};

static const UCHAR IntelUdmaSettings[] =
{
    0, // UDMA 0
    1, // UDMA 1
    2, // UDMA 2
    1, // UDMA 3
    2, // UDMA 4
    1  // UDMA 5
};

static const ULONG IntelDmaModeToTimingMode[] = {
    PIO_MODE(2), // SWDMA_MODE(2)
    PIO_MODE(0), // Unused, MWDMA0 is not supported by the chip
    PIO_MODE(3), // MWDMA_MODE(1)
    PIO_MODE(4)  // MWDMA_MODE(2)
};

/* FUNCTIONS ******************************************************************/

/*
 * Make sure that the selected PIO and DMA speeds
 * match the cycle time of IntelClockSettings[].
 */
static
VOID
IntelPiixChooseDeviceSpeed(
    _In_ ULONG Channel,
    _In_ PCHANNEL_DEVICE_CONFIG Device)
{
    ULONG Mode;

    /* PIO speed */
    for (Mode = Device->PioMode; Mode > PIO_MODE(0); Mode--)
    {
        if (!(Device->SupportedModes & (1 << Mode)))
            continue;

        if (IntelTimingModeToCycleTime[Mode] >= Device->MinPioCycleTime)
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
        ULONG MinimumCycleTime, TimingMode;

        if (!(Device->SupportedModes & ~PIO_ALL & (1 << Mode)))
            continue;

        ASSERT((Mode == SWDMA_MODE(2)) || (Mode == MWDMA_MODE(1)) || (Mode == MWDMA_MODE(2)));

        if (Device->DmaMode >= MWDMA_MODE(0))
            MinimumCycleTime = Device->MinMwDmaCycleTime;
        else
            MinimumCycleTime = Device->MinSwDmaCycleTime;

        TimingMode = IntelDmaModeToTimingMode[Mode - SWDMA_MODE(2)];

        if (IntelTimingModeToCycleTime[TimingMode] >= MinimumCycleTime)
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

static
VOID
IntelPiixSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    PCHANNEL_DATA_PATA ChanData = Controller->Channels[Channel];
    USHORT IdeTimReg, IdeConfigReg;
    UCHAR IdeSlaveTimReg, IdeUdmaCtrlReg, IdeUdmaTimReg;
    ULONG i;

    IdeUdmaCtrlReg = 0;
    IdeUdmaTimReg = 0;
    IdeConfigReg = 0;

    IdeTimReg = PciRead16(Controller, PIIX_REG_IDETIM(Channel));
    IdeSlaveTimReg = PciRead8(Controller, PIIX_REG_SIDETIM);
    if (ChanData->HwFlags & HW_FLAGS_HAS_UDMA_REG)
    {
        IdeUdmaCtrlReg = PciRead8(Controller, PIIX_REG_UDMACTL);
        IdeUdmaTimReg = PciRead8(Controller, PIIX_REG_UDMATIM(Channel));
        if (ChanData->HwFlags & HW_FLAGS_HAS_CFG_REG)
        {
            IdeConfigReg = PciRead16(Controller, PIIX_REG_CONFIG);
        }
    }

    INFO("CH %lu: Config (before)\n"
         "IDETIM  %04X\n"
         "SIDETIM   %02X\n"
         "UDMACTL   %02X\n"
         "UDMATIM   %02X\n"
         "CONFIG  %04X\n",
         Channel, IdeTimReg, IdeSlaveTimReg, IdeUdmaCtrlReg, IdeUdmaTimReg, IdeConfigReg);

    /* Clear the current mode configuration */
    IdeTimReg &= PIIX_IDETIM_IDE | PIIX_IDETIM_RSV_MASK;
    IdeSlaveTimReg &= ~PIIX_SIDETIM_ISPRCT_MASK(Channel);
    IdeUdmaCtrlReg &= ~PIIX_UDMACTL_EN_MASK(Channel);
    IdeUdmaTimReg &= ~PIIX_UDMATIM_CT_MASK;
    IdeConfigReg &= PIIX_CONFIG_KEEP_MASK;

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        ULONG TimingMode;

        if (!Device)
            continue;

        IntelPiixChooseDeviceSpeed(Channel, Device);

        TimingMode = Device->PioMode;

        /* UDMA timings */
        if (Device->DmaMode >= UDMA_MODE(0))
        {
            /* UDMA works independently of any PIO mode */
            IdeUdmaTimReg |= PIIX_UDMATIM_CT(i, IntelUdmaSettings[Device->DmaMode - UDMA_MODE(0)]);
            IdeUdmaCtrlReg |= PIIX_UDMACTL_EN(Channel, i);

            if (Device->DmaMode == UDMA_MODE(5))
                IdeConfigReg |= PIIX_CONFIG_CLOCK_UDMA100(Channel, i);
            else if (Device->DmaMode > UDMA_MODE(2))
                IdeConfigReg |= PIIX_CONFIG_CLOCK_UDMA66(Channel, i);
        }
        /* DMA timings */
        else if (Device->DmaMode != PIO_MODE(0))
        {
            ASSERT((Device->DmaMode == SWDMA_MODE(2)) ||
                   (Device->DmaMode == MWDMA_MODE(1)) ||
                   (Device->DmaMode == MWDMA_MODE(2)));

            /* Find the fastest mode while satisfying PIO timings */
            TimingMode = IntelDmaModeToTimingMode[Device->DmaMode - SWDMA_MODE(2)];
            if (TimingMode < Device->PioMode)
            {
                /*
                 * No common mode, we have to slow down the device's PIO speed
                 * by forcing the compatible PIO timings for PIO transfers.
                 */
                IdeTimReg |= PIIX_IDETIM_DTE(i);
            }
        }

        /* DMA and PIO timings */
        if (i == 0)
        {
            IdeTimReg |= PIIX_IDETIM_ISP(IntelClockSettings[TimingMode][0]);
            IdeTimReg |= PIIX_IDETIM_RCT(IntelClockSettings[TimingMode][1]);
        }
        else
        {
            IdeSlaveTimReg |= PIIX_SIDETIM_ISP(Channel, IntelClockSettings[TimingMode][0]);
            IdeSlaveTimReg |= PIIX_SIDETIM_RCT(Channel, IntelClockSettings[TimingMode][1]);

            /* Enable effect of the PIIX_REG_SIDETIM register */
            IdeTimReg |= PIIX_IDETIM_SITRE;
        }

        IdeTimReg |= PIIX_IDETIM_SETTINGS(i, IntelModeSettings[TimingMode]);

        if (((TimingMode == PIO_MODE(2)) && Device->IoReadySupported))
            IdeTimReg |= PIIX_IDETIM_IE(i);

        if (Device->IsFixedDisk)
            IdeTimReg |= PIIX_IDETIM_PPE(i);
    }

    PciWrite8(Controller, PIIX_REG_SIDETIM, IdeSlaveTimReg);
    PciWrite16(Controller, PIIX_REG_IDETIM(Channel), IdeTimReg);
    if (ChanData->HwFlags & HW_FLAGS_HAS_UDMA_REG)
    {
        PciWrite8(Controller, PIIX_REG_UDMACTL, IdeUdmaCtrlReg);
        PciWrite8(Controller, PIIX_REG_UDMATIM(Channel), IdeUdmaTimReg);
        if (ChanData->HwFlags & HW_FLAGS_HAS_CFG_REG)
        {
            /* Enable this feature for performance enhancement */
            IdeConfigReg |= PIIX_CONFIG_WR_PING_PONG;

            PciWrite16(Controller, PIIX_REG_CONFIG, IdeConfigReg);
        }
    }

    INFO("CH %lu: Config (after)\n"
         "IDETIM  %04X\n"
         "SIDETIM   %02X\n"
         "UDMACTL   %02X\n"
         "UDMATIM   %02X\n"
         "CONFIG  %04X\n",
         Channel, IdeTimReg, IdeSlaveTimReg, IdeUdmaCtrlReg, IdeUdmaTimReg, IdeConfigReg);
}

static
VOID
IntelSchSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    ULONG i;

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        ULONG DeviceTimingReg;

        if (!Device)
            continue;

        DeviceTimingReg = PciRead32(Controller, SCH_REG_DTIM(i));
        DeviceTimingReg &= ~(SCH_DTIM_PM_MASK |
                             SCH_DTIM_MDM_MASK |
                             SCH_DTIM_UDM_MASK |
                             SCH_DTIM_PPE |
                             SCH_DTIM_USD);

        if (Device->IsFixedDisk)
            DeviceTimingReg |= SCH_DTIM_PPE;

        /* DMA timings */
        if (Device->DmaMode >= UDMA_MODE(0))
            DeviceTimingReg |= SCH_DTIM_USD | SCH_DTIM_UDM(Device->DmaMode - UDMA_MODE(0));
        else if (Device->DmaMode >= MWDMA_MODE(0))
            DeviceTimingReg |= SCH_DTIM_MDM(Device->DmaMode - MWDMA_MODE(0));

        /* PIO timings */
        DeviceTimingReg |= SCH_DTIM_PM(Device->PioMode);

        PciWrite32(Controller, SCH_REG_DTIM(i), DeviceTimingReg);
    }
}

static
VOID
IntelPiixLegacyPrepareIo(
    _In_ PVOID ChannelContext,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    PINTEL_HW_EXTENSION HwExt = (PVOID)ChanData->HwExt;
    ULONG Index = (Request->Flags & REQUEST_FLAG_PROGRAM_DMA) ? 1 : 0;
    USHORT NewTimingReg = HwExt->Device[DEV_NUMBER(Request->Device)].Timing[Index];

    /* Set the proper device timings */
    if (HwExt->CurrentTiming != NewTimingReg)
    {
        HwExt->CurrentTiming = NewTimingReg;

        PciWrite16(ChanData->Controller, HwExt->Register, NewTimingReg);
        TRACE("CH %lu: IDETIM %04X\n", ChanData->Channel, NewTimingReg);
    }

    PataPrepareIo(ChanData, Request);
}

static
USHORT
IntelPiixLegacyComputeIdeTiming(
    _In_ PCHANNEL_DEVICE_CONFIG Device,
    _In_ ULONG Number,
    _In_ ULONG Mode)
{
    USHORT IdeTimReg;
    ULONG TimingMode;

    if (Mode > PIO_MODE(4))
    {
        ASSERT((Mode == SWDMA_MODE(2)) || (Mode == MWDMA_MODE(1)) || (Mode == MWDMA_MODE(2)));

        TimingMode = IntelDmaModeToTimingMode[Mode - SWDMA_MODE(2)];
    }
    else
    {
        TimingMode = Mode;
    }

    IdeTimReg = PIIX_IDETIM_ISP(IntelClockSettings[TimingMode][0]);
    IdeTimReg |= PIIX_IDETIM_RCT(IntelClockSettings[TimingMode][1]);
    IdeTimReg |= PIIX_IDETIM_SETTINGS(Number, IntelModeSettings[TimingMode]);

    if (Device->IsFixedDisk)
        IdeTimReg |= PIIX_IDETIM_PPE(Number);

    if (((TimingMode == PIO_MODE(2)) && Device->IoReadySupported))
        IdeTimReg |= PIIX_IDETIM_IE(Number);

    return IdeTimReg;
}

/*
 * Unfortunately, the first PIIX chip cannot specify separate device timings (>PIO1)
 * for both of the drives. The PIIX_IDETIM_SITRE bit is reserved here.
 * This problem can be solved in two ways:
 *
 * 1) If we have a common timing mode for all devices use it, otherwise take the lower mode.
 * 2) Snoop an ATA command by software and run with the proper timings for that device.
 *
 * The code below deals with the second solution.
 */
static
VOID
IntelPiixLegacySetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    PCHANNEL_DATA_PATA ChanData = Controller->Channels[Channel];
    PINTEL_HW_EXTENSION HwExt = (PVOID)ChanData->HwExt;
    USHORT IdeTimReg;
    ULONG i;

    /* Clear the current mode configuration */
    IdeTimReg = PciRead16(Controller, HwExt->Register);
    IdeTimReg &= PIIX_IDETIM_IDE | PIIX_IDETIM_SITRE | PIIX_IDETIM_RSV_MASK;
    PciWrite16(Controller, HwExt->Register, IdeTimReg);

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];

        if (!Device)
            continue;

        IntelPiixChooseDeviceSpeed(Channel, Device);

        /* Compute the timing bits for this device */
        HwExt->Device[i].Timing[0] = IntelPiixLegacyComputeIdeTiming(Device, i, Device->PioMode);
        HwExt->Device[i].Timing[1] = IntelPiixLegacyComputeIdeTiming(Device, i, Device->DmaMode);

        INFO("CH %lu: Drive #%lu PIO:%04X DMA:04X\n",
             Channel, i, HwExt->Device[i].Timing[0], HwExt->Device[i].Timing[1]);
    }
    HwExt->CurrentTiming = 0;
}

/*
 * 450KX: Intel order no 243109-015
 * 450NX: Intel order no 243848-009
 */
static
CODE_SEG("PAGE")
BOOLEAN
IntelPciBridgeErrataMatch(
    _In_ PVOID Context,
    _In_ ULONG BusNumber,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_ PPCI_COMMON_HEADER PciConfig)
{
    UNREFERENCED_PARAMETER(Context);

    PAGED_CODE();

    if (PciConfig->VendorID != PCI_VEN_INTEL)
        return FALSE;

    /* 450KX errata: 0-Byte Length Write failure */
    if ((PciConfig->DeviceID == PCI_DEV_BRIDGE_450KX) &&
        (PciConfig->RevisionID < BRIDGE_450KX_REV_B0))
    {
        WARN("Enabling workaround for the 450KX #2\n");
        return TRUE;
    }

    /* 450NX errata */
    if (PciConfig->DeviceID == PCI_DEV_BRIDGE_450NX)
    {
        USHORT ConfigReg;

        /* PXB livelock */
        if (PciConfig->RevisionID == BRIDGE_450NX_REV_B0)
        {
            WARN("Enabling workaround for the 450NX #19\n");
            return TRUE;
        }

        HalGetBusDataByOffset(PCIConfiguration,
                              BusNumber,
                              PciSlot.u.AsULONG,
                              &ConfigReg,
                              PXB_REG_CONFIG,
                              sizeof(ConfigReg));
        TRACE("PXB CFG: %04X\n", ConfigReg);

        /* PXB arbiter deadlock */
        if ((PciConfig->RevisionID != BRIDGE_450NX_REV_B1) &&
            (ConfigReg & PXB_CONFIG_PCI_BUS_LOCK))
        {
            WARN("Enabling workaround for the 450NX #20\n");
            return TRUE;
        }

        /* PCI data corruption */
        if ((PciConfig->RevisionID == BRIDGE_450NX_REV_C0) &&
            (ConfigReg & PXB_CONFIG_BUF_RESTREAM))
        {
            WARN("Enabling workaround for the 450NX #25\n");
            return TRUE;
        }
    }

    return FALSE;
}

static
CODE_SEG("PAGE")
VOID
IntelInitChannel(
    _In_ PATA_CONTROLLER Controller,
    _In_ const INTEL_CONTROLLER_INFO* ControllerInfo,
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    ULONG SupportedMode;

    PAGED_CODE();

    switch (ControllerInfo->Type)
    {
        case TYPE_MPIIX:
        case TYPE_PIIX:
        {
            PINTEL_HW_EXTENSION HwExt = (PVOID)ChanData->HwExt;

            if (ControllerInfo->Type == TYPE_MPIIX)
                HwExt->Register = MPIIX_REG_IDETIM;
            else
                HwExt->Register = PIIX_REG_IDETIM(ChanData->Channel);

            ChanData->SetTransferMode = IntelPiixLegacySetTransferMode;
            ChanData->PrepareIo = IntelPiixLegacyPrepareIo;
            break;
        }

        case TYPE_PIIX3:
        case TYPE_PIIX4:
        case TYPE_ICH:
            ChanData->SetTransferMode = IntelPiixSetTransferMode;
            break;

        case TYPE_SCH:
            ChanData->SetTransferMode = IntelSchSetTransferMode;
            break;

        default:
            ChanData->SetTransferMode = SataSetTransferMode;
            break;
    }

    switch (ControllerInfo->Type)
    {
        case TYPE_MPIIX:
            SupportedMode = PIO_ALL;
            break;

        case TYPE_PIIX:
        case TYPE_PIIX3:
            SupportedMode = PIO_ALL | SWDMA_MODE2 | MWDMA_MODES(1, 2);
            break;

        case TYPE_PIIX4:
            SupportedMode = PIO_ALL | SWDMA_MODE2 | MWDMA_MODES(1, 2) | UDMA_MODES(0, 2);
            break;

        case TYPE_ICH:
        {
            /* Errata: MW DMA Mode-1 Tdh (unable to drive MWDMA1 and SWDMA2 timings) */
            SupportedMode = PIO_ALL | MWDMA_MODE2 | UDMA_MODES(0, 5);

            if (Controller->Pci.DeviceID == PCI_DEV_ICH_82801AA ||
                Controller->Pci.DeviceID == PCI_DEV_ICH0_82801AB)
            {
                /* Errata: IDE Bus Master Concurrency */
                if (Controller->Pci.RevisionID == 0)
                {
                    SupportedMode = PIO_ALL;
                }
                else
                {
                    /* ICH is UDMA4 */
                    SupportedMode &= ~UDMA_MODE5;

                    /* ICH0 is UDMA2 */
                    if (Controller->Pci.DeviceID == PCI_DEV_ICH0_82801AB)
                        SupportedMode &= ~(UDMA_MODE3 | UDMA_MODE4);
                }
            }

            /* Check for 80-conductor cable */
            if (SupportedMode & UDMA_80C_ALL)
            {
                USHORT IdeConfigReg = PciRead16(Controller, PIIX_REG_CONFIG);
                if (!(IdeConfigReg & PIIX_CONFIG_CR(ChanData->Channel)))
                {
                    INFO("CH %lu: BIOS detected 40-conductor cable\n", ChanData->Channel);
                    SupportedMode &= ~UDMA_80C_ALL;
                }
            }
            break;
        }

        case TYPE_SCH:
        {
            ULONG DeviceTimingReg[2];

            SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 5);

            /* Check for 80-conductor cable */
            PciRead(Controller, &DeviceTimingReg, SCH_REG_DTIM(0), sizeof(DeviceTimingReg));
            if ((DeviceTimingReg[0] & SCH_DTIM_UDM_MASK) <= SCH_DTIM_UDM(2) &&
                (DeviceTimingReg[1] & SCH_DTIM_UDM_MASK) <= SCH_DTIM_UDM(2))
            {
                INFO("CH %lu: BIOS hasn't selected mode faster than UDMA 2, "
                     "assume 40-conductor cable\n",
                     ChanData->Channel);
                SupportedMode &= ~UDMA_80C_ALL;
            }
            break;
        }

        case TYPE_SATA:
        {
            SupportedMode = SATA_ALL;

            // TODO: Read the port map register and set CHANNEL_FLAG_NO_SLAVE when appropriate
            ERR("CH %lu: PMR %02X PCS %04X\n",
                ChanData->Channel,
                PciRead8(Controller, 0x90),
                PciRead16(Controller, 0x92));
            break;
        }

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }
    ChanData->TransferModeSupported = SupportedMode;
}

CODE_SEG("PAGE")
NTSTATUS
IntelGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    const INTEL_CONTROLLER_INFO* ControllerInfo;
    ULONG i, ExtensionSize, HwFlags;
    NTSTATUS Status;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_INTEL);

    for (i = 0; i < RTL_NUMBER_OF(IntelControllerList); ++i)
    {
        ControllerInfo = &IntelControllerList[i];

        if (Controller->Pci.DeviceID == ControllerInfo->DeviceID)
            break;
    }
    if (i == RTL_NUMBER_OF(IntelControllerList))
        return STATUS_NO_MATCH;

    if (ControllerInfo->Type == TYPE_SCH)
        Controller->MaxChannels = 1;

    if (ControllerInfo->Type == TYPE_PIIX || ControllerInfo->Type == TYPE_MPIIX)
        ExtensionSize = sizeof(INTEL_HW_EXTENSION);
    else
        ExtensionSize = 0;

    Status = PciIdeCreateChannelData(Controller, ExtensionSize);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (ControllerInfo->Type)
    {
        case TYPE_MPIIX:
            /*
             * This is a bridge device and there are no PCI resources allocated,
             * so mark the controller as a legacy device.
             */
            Controller->Flags |= CTRL_FLAG_IN_LEGACY_MODE;
            Controller->ChannelEnableBits = IntelMpiixEnableBits;
            break;

        case TYPE_PIIX:
        case TYPE_PIIX3:
        case TYPE_PIIX4:
        case TYPE_ICH:
            Controller->ChannelEnableBits = IntelPiixEnableBits;
            break;

        case TYPE_SATA:
            // TODO: Implement the map support
            //Controller->ChannelEnabledTest = IntelCombinedEnabledTest;
            //HwFlags = TYPE_ICH;
            break;

        default:
            break;
    }
    HwFlags = ControllerInfo->Type;

    if (PciFindDevice(IntelPciBridgeErrataMatch, NULL))
        HwFlags |= HW_FLAGS_DISABLE_DMA;

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        ChanData->HwFlags = HwFlags;

        IntelInitChannel(Controller, ControllerInfo, ChanData);

        if (HwFlags & HW_FLAGS_DISABLE_DMA)
            ChanData->TransferModeSupported &= PIO_ALL;
    }

    return STATUS_SUCCESS;
}
