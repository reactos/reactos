/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware specific definitions
 * COPYRIGHT:   Copyright 2021-2022 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * Definitions were taken from the Linux forcedeth driver
 * Copyright (C) 2003,4,5 Manfred Spraul
 * Copyright (C) 2004 Andrew de Quincey
 * Copyright (C) 2004 Carl-Daniel Hailfinger
 * Copyright (c) 2004,2005,2006,2007,2008,2009 NVIDIA Corporation
 */

#pragma once

#define DEV_NEED_TIMERIRQ        0x00000001 /* Set the timer IRQ flag in the IRQ mask */
#define DEV_NEED_LINKTIMER       0x00000002 /* Poll link settings. Relies on the timer IRQ */
#define DEV_HAS_LARGEDESC        0x00000004 /* Device supports Jumbo Frames
                                             * and needs packet format 2 */
#define DEV_HAS_HIGH_DMA         0x00000008 /* Device supports 64-bit DMA */
#define DEV_HAS_CHECKSUM         0x00000010 /* Device supports TX and RX checksum offloads */
#define DEV_HAS_VLAN             0x00000020 /* Device supports VLAN tagging and striping */
#define DEV_HAS_MSI              0x00000040 /* Device supports MSI */
#define DEV_HAS_MSI_X            0x00000080 /* Device supports MSI-X */
#define DEV_HAS_POWER_CNTRL      0x00000100 /* Device supports power savings */
#define DEV_HAS_STATISTICS_V1    0x00000200 /* Device supports HW statistics version 1 */
#define DEV_HAS_STATISTICS_V2    0x00000400 /* Device supports HW statistics version 2 */
#define DEV_HAS_STATISTICS_V3    0x00000800 /* Device supports HW statistics version 3 */
#define DEV_HAS_TEST_EXTENDED    0x00001000 /* Device supports extended diagnostic test */
#define DEV_HAS_MGMT_UNIT        0x00002000 /* Device supports management unit */
#define DEV_HAS_CORRECT_MACADDR  0x00004000 /* Device supports correct MAC address order */
#define DEV_HAS_COLLISION_FIX    0x00008000 /* Device supports TX collision fix */
#define DEV_HAS_PAUSEFRAME_TX_V1 0x00010000 /* Device supports TX pause frames version 1 */
#define DEV_HAS_PAUSEFRAME_TX_V2 0x00020000 /* Device supports TX pause frames version 2 */
#define DEV_HAS_PAUSEFRAME_TX_V3 0x00040000 /* Device supports TX pause frames version 3 */
#define DEV_NEED_TX_LIMIT        0x00080000 /* Device needs to limit TX */
#define DEV_NEED_TX_LIMIT2       0x00100000 /* Device needs to limit TX, expect for some revs */
#define DEV_HAS_GEAR_MODE        0x00200000 /* Device supports gear mode */
#define DEV_NEED_PHY_INIT_FIX    0x00400000 /* Device needs specific PHY workaround */
#define DEV_NEED_LOW_POWER_FIX   0x00800000 /* Device needs special power up workaround */
#define DEV_NEED_MSI_FIX         0x01000000 /* Device needs MSI workaround */

#define DEV_HAS_STATISTICS_COUNTERS    (DEV_HAS_STATISTICS_V1 | DEV_HAS_STATISTICS_V2 | \
                                        DEV_HAS_STATISTICS_V3)
#define DEV_HAS_TX_PAUSEFRAME    (DEV_HAS_PAUSEFRAME_TX_V1 | DEV_HAS_PAUSEFRAME_TX_V2 | \
                                  DEV_HAS_PAUSEFRAME_TX_V3)

typedef enum _NVNET_REGISTER
{
    NvRegIrqStatus = 0x000,
#define NVREG_IRQSTAT_MIIEVENT  0x040
#define NVREG_IRQSTAT_MASK      0x83ff

    NvRegIrqMask = 0x004,
#define NVREG_IRQ_RX_ERROR       0x0001
#define NVREG_IRQ_RX             0x0002
#define NVREG_IRQ_RX_NOBUF       0x0004
#define NVREG_IRQ_TX_ERR         0x0008
#define NVREG_IRQ_TX_OK          0x0010
#define NVREG_IRQ_TIMER          0x0020
#define NVREG_IRQ_LINK           0x0040
#define NVREG_IRQ_RX_FORCED      0x0080
#define NVREG_IRQ_TX_FORCED      0x0100
#define NVREG_IRQ_RECOVER_ERROR  0x8200
#define NVREG_IRQMASK_THROUGHPUT 0x00df
#define NVREG_IRQMASK_CPU        0x0060
#define NVREG_IRQ_TX_ALL    (NVREG_IRQ_TX_ERR|NVREG_IRQ_TX_OK|NVREG_IRQ_TX_FORCED)
#define NVREG_IRQ_RX_ALL    (NVREG_IRQ_RX_ERROR|NVREG_IRQ_RX|NVREG_IRQ_RX_NOBUF| \
                             NVREG_IRQ_RX_FORCED)
#define NVREG_IRQ_OTHER     (NVREG_IRQ_TIMER|NVREG_IRQ_LINK|NVREG_IRQ_RECOVER_ERROR)

    NvRegUnknownSetupReg6 = 0x008,
#define NVREG_UNKSETUP6_VAL    3

    NvRegPollingInterval = 0x00c,
#define NVREG_POLL_DEFAULT_THROUGHPUT  65535
#define NVREG_POLL_DEFAULT_CPU         13

    NvRegMSIMap0 = 0x020,
    NvRegMSIMap1 = 0x024,

    NvRegMSIIrqMask = 0x030,
#define NVREG_MSI_VECTOR_0_ENABLED 0x01

    NvRegMacReset = 0x34,
#define NVREG_MAC_RESET_ASSERT  0x0F3

    NvRegMisc1 = 0x080,
#define NVREG_MISC1_PAUSE_TX  0x01
#define NVREG_MISC1_HD        0x02
#define NVREG_MISC1_FORCE     0x3b0f3c

    NvRegTransmitterControl = 0x084,
#define NVREG_XMITCTL_START           0x01
#define NVREG_XMITCTL_MGMT_ST         0x40000000
#define NVREG_XMITCTL_SYNC_MASK       0x000f0000
#define NVREG_XMITCTL_SYNC_NOT_READY  0x0
#define NVREG_XMITCTL_SYNC_PHY_INIT   0x00040000
#define NVREG_XMITCTL_MGMT_SEMA_MASK  0x00000f00
#define NVREG_XMITCTL_MGMT_SEMA_FREE  0x0
#define NVREG_XMITCTL_HOST_SEMA_MASK  0x0000f000
#define NVREG_XMITCTL_HOST_SEMA_ACQ   0x0000f000
#define NVREG_XMITCTL_HOST_LOADED     0x00004000
#define NVREG_XMITCTL_TX_PATH_EN      0x01000000
#define NVREG_XMITCTL_DATA_START      0x00100000
#define NVREG_XMITCTL_DATA_READY      0x00010000
#define NVREG_XMITCTL_DATA_ERROR      0x00020000

    NvRegTransmitterStatus = 0x088,
#define NVREG_XMITSTAT_BUSY  0x01

    NvRegPacketFilterFlags = 0x8c,
#define NVREG_PFF_PAUSE_RX  0x08
#define NVREG_PFF_ALWAYS    0x7F0000
#define NVREG_PFF_PROMISC   0x80
#define NVREG_PFF_MYADDR    0x20
#define NVREG_PFF_LOOPBACK  0x10

    NvRegOffloadConfig = 0x90,
#define NVREG_OFFLOAD_HOMEPHY  0x601
#define NVREG_OFFLOAD_NORMAL   RX_NIC_BUFSIZE

    NvRegReceiverControl = 0x094,
#define NVREG_RCVCTL_START       0x01
#define NVREG_RCVCTL_RX_PATH_EN  0x01000000

    NvRegReceiverStatus = 0x98,
#define NVREG_RCVSTAT_BUSY  0x01

    NvRegSlotTime = 0x9c,
#define NVREG_SLOTTIME_LEGBF_ENABLED 0x80000000
#define NVREG_SLOTTIME_10_100_FULL   0x00007f00
#define NVREG_SLOTTIME_1000_FULL     0x0003ff00
#define NVREG_SLOTTIME_HALF          0x0000ff00
#define NVREG_SLOTTIME_DEFAULT       0x00007f00
#define NVREG_SLOTTIME_MASK          0x000000ff

    NvRegTxDeferral = 0xA0,
#define NVREG_TX_DEFERRAL_DEFAULT           0x15050f
#define NVREG_TX_DEFERRAL_RGMII_10_100      0x16070f
#define NVREG_TX_DEFERRAL_RGMII_1000        0x14050f
#define NVREG_TX_DEFERRAL_RGMII_STRETCH_10  0x16190f
#define NVREG_TX_DEFERRAL_RGMII_STRETCH_100 0x16300f
#define NVREG_TX_DEFERRAL_MII_STRETCH       0x152000

    NvRegRxDeferral = 0xA4,
#define NVREG_RX_DEFERRAL_DEFAULT  0x16

    NvRegMacAddrA = 0xA8,
    NvRegMacAddrB = 0xAC,

    NvRegMulticastAddrA = 0xB0,
    NvRegMulticastAddrB = 0xB4,
#define NVREG_MCASTADDRA_FORCE  0x01

    NvRegMulticastMaskA = 0xB8,
#define NVREG_MCASTMASKA_NONE    0xffffffff

    NvRegMulticastMaskB = 0xBC,
#define NVREG_MCASTMASKB_NONE    0xffff

    NvRegPhyInterface = 0xC0,
#define PHY_100      0x1
#define PHY_1000     0x2
#define PHY_HALF     0x100
#define PHY_RGMII    0x10000000

    NvRegBackOffControl = 0xC4,
#define NVREG_BKOFFCTRL_DEFAULT   0x70000000
#define NVREG_BKOFFCTRL_SEED_MASK 0x000003ff
#define NVREG_BKOFFCTRL_SELECT    24
#define NVREG_BKOFFCTRL_GEAR      12

    NvRegTxRingPhysAddr = 0x100,
    NvRegRxRingPhysAddr = 0x104,

    NvRegRingSizes = 0x108,
#define NVREG_RINGSZ_TXSHIFT 0
#define NVREG_RINGSZ_RXSHIFT 16

    NvRegTransmitPoll = 0x10c,
#define NVREG_TRANSMITPOLL_MAC_ADDR_REV  0x00008000

    NvRegLinkSpeed = 0x110,
#define NVREG_LINKSPEED_FORCE 0x10000
#define NVREG_LINKSPEED_10    1000
#define NVREG_LINKSPEED_100   100
#define NVREG_LINKSPEED_1000  50
#define NVREG_LINKSPEED_MASK  (0xFFF)

    NvRegUnknownSetupReg5 = 0x130,
#define NVREG_UNKSETUP5_BIT31  (1<<31)

    NvRegTxWatermark = 0x13c,
#define NVREG_TX_WM_DESC1_DEFAULT    0x0200010
#define NVREG_TX_WM_DESC2_3_DEFAULT  0x1e08000
#define NVREG_TX_WM_DESC2_3_1000     0xfe08000

    NvRegTxRxControl = 0x144,
#define NVREG_TXRXCTL_KICK      0x0001
#define NVREG_TXRXCTL_BIT1      0x0002
#define NVREG_TXRXCTL_BIT2      0x0004
#define NVREG_TXRXCTL_IDLE      0x0008
#define NVREG_TXRXCTL_RESET     0x0010
#define NVREG_TXRXCTL_RXCHECK   0x0400
#define NVREG_TXRXCTL_DESC_1    0
#define NVREG_TXRXCTL_DESC_2    0x002100
#define NVREG_TXRXCTL_DESC_3    0xc02200
#define NVREG_TXRXCTL_VLANSTRIP 0x00040
#define NVREG_TXRXCTL_VLANINS   0x00080

    NvRegTxRingPhysAddrHigh = 0x148,
    NvRegRxRingPhysAddrHigh = 0x14C,

    NvRegTxPauseFrame = 0x170,
#define NVREG_TX_PAUSEFRAME_DISABLE    0x0fff0080
#define NVREG_TX_PAUSEFRAME_ENABLE_V1  0x01800010
#define NVREG_TX_PAUSEFRAME_ENABLE_V2  0x056003f0
#define NVREG_TX_PAUSEFRAME_ENABLE_V3  0x09f00880

    NvRegTxPauseFrameLimit = 0x174,
#define NVREG_TX_PAUSEFRAMELIMIT_ENABLE  0x00010000

    NvRegMIIStatus = 0x180,
#define NVREG_MIISTAT_ERROR      0x0001
#define NVREG_MIISTAT_LINKCHANGE 0x0008
#define NVREG_MIISTAT_MASK_RW    0x0007
#define NVREG_MIISTAT_MASK_ALL   0x000f

    NvRegMIIMask = 0x184,
#define NVREG_MII_LINKCHANGE    0x0008

    NvRegAdapterControl = 0x188,
#define NVREG_ADAPTCTL_START     0x02
#define NVREG_ADAPTCTL_LINKUP    0x04
#define NVREG_ADAPTCTL_PHYVALID  0x40000
#define NVREG_ADAPTCTL_RUNNING   0x100000
#define NVREG_ADAPTCTL_PHYSHIFT  24

    NvRegMIISpeed = 0x18c,
#define NVREG_MIISPEED_BIT8  (1<<8)
#define NVREG_MIIDELAY  5

    NvRegMIIControl = 0x190,
#define NVREG_MIICTL_INUSE  0x08000
#define NVREG_MIICTL_WRITE  0x00400
#define NVREG_MIICTL_ADDRSHIFT  5

    NvRegMIIData = 0x194,
    NvRegTxUnicast = 0x1a0,
    NvRegTxMulticast = 0x1a4,
    NvRegTxBroadcast = 0x1a8,

    NvRegWakeUpFlags = 0x200,
#define NVREG_WAKEUPFLAGS_VAL    0x7770
#define NVREG_WAKEUPFLAGS_BUSYSHIFT    24
#define NVREG_WAKEUPFLAGS_ENABLESHIFT  16
#define NVREG_WAKEUPFLAGS_D3SHIFT  12
#define NVREG_WAKEUPFLAGS_D2SHIFT  8
#define NVREG_WAKEUPFLAGS_D1SHIFT  4
#define NVREG_WAKEUPFLAGS_D0SHIFT  0
#define NVREG_WAKEUPFLAGS_ACCEPT_MAGPAT     0x01
#define NVREG_WAKEUPFLAGS_ACCEPT_WAKEUPPAT  0x02
#define NVREG_WAKEUPFLAGS_ACCEPT_LINKCHANGE 0x04
#define NVREG_WAKEUPFLAGS_ENABLE_MAGPAT     0x1111
#define NVREG_WAKEUPFLAGS_ENABLE_WAKEUPPAT  0x2222
#define NVREG_WAKEUPFLAGS_ENABLE_LINKCHANGE 0x4444

    NvRegPatternCrc = 0x204,
    NvRegPatternMask0 = 0x208,
    NvRegPatternMask1 = 0x20C,
    NvRegPatternMask2 = 0x210,
    NvRegPatternMask3 = 0x214,

    NvRegMgmtUnitGetVersion = 0x204,
#define NVREG_MGMTUNITGETVERSION  0x01

    NvRegMgmtUnitVersion = 0x208,
#define NVREG_MGMTUNITVERSION    0x08

    NvRegPowerCap = 0x268,
#define NVREG_POWERCAP_D3SUPP  (1<<30)
#define NVREG_POWERCAP_D2SUPP  (1<<26)
#define NVREG_POWERCAP_D1SUPP  (1<<25)

    NvRegPowerState = 0x26c,
#define NVREG_POWERSTATE_POWEREDUP  0x8000
#define NVREG_POWERSTATE_VALID      0x0100
#define NVREG_POWERSTATE_MASK  0x0003
#define NVREG_POWERSTATE_D0    0x0000
#define NVREG_POWERSTATE_D1    0x0001
#define NVREG_POWERSTATE_D2    0x0002
#define NVREG_POWERSTATE_D3    0x0003

    NvRegMgmtUnitControl = 0x278,
#define NVREG_MGMTUNITCONTROL_INUSE  0x20000

    NvRegTxCnt = 0x280,
    NvRegTxZeroReXmt = 0x284,
    NvRegTxOneReXmt = 0x288,
    NvRegTxManyReXmt = 0x28c,
    NvRegTxLateCol = 0x290,
    NvRegTxUnderflow = 0x294,
    NvRegTxLossCarrier = 0x298,
    NvRegTxExcessDef = 0x29c,
    NvRegTxRetryErr = 0x2a0,
    NvRegRxFrameErr = 0x2a4,
    NvRegRxExtraByte = 0x2a8,
    NvRegRxLateCol = 0x2ac,
    NvRegRxRunt = 0x2b0,
    NvRegRxFrameTooLong = 0x2b4,
    NvRegRxOverflow = 0x2b8,
    NvRegRxFCSErr = 0x2bc,
    NvRegRxFrameAlignErr = 0x2c0,
    NvRegRxLenErr = 0x2c4,
    NvRegRxUnicast = 0x2c8,
    NvRegRxMulticast = 0x2cc,
    NvRegRxBroadcast = 0x2d0,
    NvRegTxDef = 0x2d4,
    NvRegTxFrame = 0x2d8,
    NvRegRxCnt = 0x2dc,
    NvRegTxPause = 0x2e0,
    NvRegRxPause = 0x2e4,
    NvRegRxDropFrame = 0x2e8,

    NvRegVlanControl = 0x300,
#define NVREG_VLANCONTROL_ENABLE  0x2000

    NvRegMSIXMap0 = 0x3e0,
    NvRegMSIXMap1 = 0x3e4,
    NvRegMSIXIrqStatus = 0x3f0,

    NvRegPowerState2 = 0x600,
#define NVREG_POWERSTATE2_POWERUP_MASK   0x0F15
#define NVREG_POWERSTATE2_POWERUP_REV_A3 0x0001
#define NVREG_POWERSTATE2_PHY_RESET      0x0004
#define NVREG_POWERSTATE2_GATE_CLOCK_1   0x0100
#define NVREG_POWERSTATE2_GATE_CLOCK_2   0x0200
#define NVREG_POWERSTATE2_GATE_CLOCK_3   0x0400
#define NVREG_POWERSTATE2_GATE_CLOCKS    0x0F00
#define NVREG_POWERSTATE2_WAKEUPPAT_5    (1<<16)
#define NVREG_POWERSTATE2_WAKEUPPAT_6    (1<<17)
#define NVREG_POWERSTATE2_WAKEUPPAT_7    (1<<18)

    NvRegPatternCrcEx = 0x604,
    NvRegPatternMask0Ex = 0x608,
    NvRegPatternMask1Ex = 0x60C,
    NvRegPatternMask2Ex = 0x610,
    NvRegPatternMask3Ex = 0x614
} NVNET_REGISTER;

#include <pshpack1.h>
typedef struct _NVNET_DESCRIPTOR_32
{
    ULONG Address;
    ULONG FlagsLength;
} NVNET_DESCRIPTOR_32, *PNVNET_DESCRIPTOR_32;

typedef struct _NVNET_DESCRIPTOR_64
{
    ULONG AddressHigh;
    ULONG AddressLow;
    ULONG VlanTag;
    ULONG FlagsLength;
} NVNET_DESCRIPTOR_64, *PNVNET_DESCRIPTOR_64;
#include <poppack.h>

#define FLAG_MASK_V1 0xffff0000
#define FLAG_MASK_V2 0xffffc000
#define LEN_MASK_V1 (0xffffffff ^ FLAG_MASK_V1)
#define LEN_MASK_V2 (0xffffffff ^ FLAG_MASK_V2)

#define NV_TX_LASTPACKET     (1<<16)
#define NV_TX_RETRYERROR     (1<<19)
#define NV_TX_RETRYCOUNT_MASK   (0xF<<20)
#define NV_TX_ONE_RETRY         (1<<20)
#define NV_TX_FORCED_INTERRUPT  (1<<24)
#define NV_TX_DEFERRED       (1<<26)
#define NV_TX_CARRIERLOST    (1<<27)
#define NV_TX_LATECOLLISION  (1<<28)
#define NV_TX_UNDERFLOW      (1<<29)
#define NV_TX_ERROR    (1<<30)
#define NV_TX_VALID    (1<<31)

#define NV_TX2_LASTPACKET    (1<<29)
#define NV_TX2_RETRYERROR    (1<<18)
#define NV_TX2_RETRYCOUNT_MASK  (0xF<<19)
#define NV_TX2_FORCED_INTERRUPT  (1<<30)
#define NV_TX2_DEFERRED      (1<<25)
#define NV_TX2_CARRIERLOST   (1<<26)
#define NV_TX2_LATECOLLISION (1<<27)
#define NV_TX2_UNDERFLOW     (1<<28)
/* Error and valid are the same for both */
#define NV_TX2_ERROR         (1<<30)
#define NV_TX2_VALID         (1<<31)
#define NV_TX2_TSO           (1<<28)
#define NV_TX2_TSO_SHIFT  14
#define NV_TX2_TSO_MAX_SHIFT  14
#define NV_TX2_CHECKSUM_L3   (1<<27)
#define NV_TX2_CHECKSUM_L4   (1<<26)

#define NV_MAXIMUM_SG_SIZE  (1<<NV_TX2_TSO_MAX_SHIFT)

#define NV_TX3_VLAN_TAG_PRESENT (1<<18)

#define NV_RX_DESCRIPTORVALID  (1<<16)
#define NV_RX_MISSEDFRAME  (1<<17)
#define NV_RX_SUBTRACT1    (1<<18)
#define NV_RX_ERROR1       (1<<23)
#define NV_RX_ERROR2       (1<<24)
#define NV_RX_ERROR3       (1<<25)
#define NV_RX_ERROR4       (1<<26)
#define NV_RX_CRCERR       (1<<27)
#define NV_RX_OVERFLOW     (1<<28)
#define NV_RX_FRAMINGERR   (1<<29)
#define NV_RX_ERROR    (1<<30)
#define NV_RX_AVAIL    (1<<31)
#define NV_RX_ERROR_MASK  (NV_RX_ERROR1|NV_RX_ERROR2|NV_RX_ERROR3|NV_RX_ERROR4|NV_RX_CRCERR| \
                           NV_RX_OVERFLOW|NV_RX_FRAMINGERR)

#define NV_RX2_CHECKSUMMASK  (0x1C000000)
#define NV_RX2_CHECKSUM_IP  (0x10000000)
#define NV_RX2_CHECKSUM_IP_TCP  (0x14000000)
#define NV_RX2_CHECKSUM_IP_UDP  (0x18000000)
#define NV_RX2_DESCRIPTORVALID  (1<<29)
#define NV_RX2_SUBTRACT1   (1<<25)
#define NV_RX2_ERROR1      (1<<18)
#define NV_RX2_ERROR2      (1<<19)
#define NV_RX2_ERROR3      (1<<20)
#define NV_RX2_ERROR4      (1<<21)
#define NV_RX2_CRCERR      (1<<22)
#define NV_RX2_OVERFLOW    (1<<23)
#define NV_RX2_FRAMINGERR  (1<<24)
/* Error and avail are the same for both */
#define NV_RX2_ERROR    (1<<30)
#define NV_RX2_AVAIL    (1<<31)
#define NV_RX2_ERROR_MASK  (NV_RX2_ERROR1|NV_RX2_ERROR2|NV_RX2_ERROR3|NV_RX2_ERROR4| \
                            NV_RX2_CRCERR|NV_RX2_OVERFLOW|NV_RX2_FRAMINGERR)

#define NV_RX3_VLAN_TAG_PRESENT (1<<16)
#define NV_RX3_VLAN_TAG_MASK  (0x0000FFFF)

#define NV_TXRX_RESET_DELAY  4
#define NV_TXSTOP_DELAY1  10
#define NV_TXSTOP_DELAY1MAX  500000
#define NV_TXSTOP_DELAY2  100
#define NV_TXIDLE_DELAY  10
#define NV_TXIDLE_ATTEMPTS  100000
#define NV_RXSTOP_DELAY1  10
#define NV_RXSTOP_DELAY1MAX  500000
#define NV_RXSTOP_DELAY2  100
#define NV_SETUP5_DELAY    5
#define NV_SETUP5_DELAYMAX  50000
#define NV_POWERUP_DELAY  5
#define NV_POWERUP_DELAYMAX  5000
#define NV_POWER_DELAY  50
#define NV_POWER_STALL  3000
#define NV_POWER_ATTEMPTS  20
#define NV_MIIBUSY_DELAY  50
#define NV_MIIPHY_DELAY  10
#define NV_MIIPHY_DELAYMAX  10000
#define NV_MAC_RESET_DELAY  64

#define NV_WAKEUPPATTERNS     5
#define NV_WAKEUPPATTERNS_V2  8
#define NV_WAKEUPMASKENTRIES  4
#define NV_PATTERN_V2_OFFSET  0x39C

/* RX/TX MAC address + type + VLAN + align + slack */
#define NV_RX_HEADERS    (64)
/* even more slack. */
#define NV_RX_ALLOC_PAD    (64)

#define PHY_OUI_MARVELL    0x5043
#define PHY_OUI_CICADA    0x03f1
#define PHY_OUI_VITESSE    0x01c1
#define PHY_OUI_REALTEK    0x0732
#define PHY_OUI_REALTEK2  0x0020
#define PHY_MODEL_REALTEK_8211    0x0110
#define PHY_MODEL_REALTEK_8201    0x0200
#define PHY_MODEL_MARVELL_E3016    0x0220

#define PHYID1_OUI_MASK  0x03ff
#define PHYID1_OUI_SHFT  6
#define PHYID2_MODEL_MASK 0x03f0
#define PHYID2_OUI_MASK   0xfc00
#define PHYID2_OUI_SHFT   10

#define PHY_GIGABIT    0x0100

#define PHY_CICADA_INIT_REG1     0x16
#define     PHY_CICADA_INIT6  0x02000
#define PHY_CICADA_INIT_REG2     0x17
#define     PHY_CICADA_INIT1  0x0f000
#define     PHY_CICADA_INIT2  0x0e00
#define     PHY_CICADA_INIT3  0x01000
#define     PHY_CICADA_INIT4  0x0200
#define PHY_CICADA_INIT_REG3     0x1c
#define     PHY_CICADA_INIT5  0x0004
#define PHY_MARVELL_INIT_REG1    0x1c
#define     PHY_MARVELL_E3016_INITMASK  0x0300
#define PHY_VITESSE_INIT_REG2    0x10
#define     PHY_VITESSE_INIT2  0xaf8a
#define     PHY_VITESSE_INIT4  0x8f8a
#define     PHY_VITESSE_INIT5  0xaf86
#define     PHY_VITESSE_INIT6  0x8f86
#define     PHY_VITESSE_INIT7  0xaf82
#define     PHY_VITESSE_INIT9  0x8f82
#define PHY_VITESSE_INIT_REG3    0x11
#define PHY_VITESSE_INIT_REG4    0x12
#define     PHY_VITESSE_INIT_MSK1  0xc
#define     PHY_VITESSE_INIT3      0x8
#define     PHY_VITESSE_INIT_MSK2  0x0180
#define     PHY_VITESSE_INIT8      0x0100
#define PHY_VITESSE_INIT_REG1    0x1f
#define     PHY_VITESSE_INIT1  0x52b5
#define     PHY_VITESSE_INIT10 0x0
#define PHY_REALTEK_INIT_REG7    0x01
#define     PHY_REALTEK_INIT11  0x0200
#define PHY_REALTEK_INIT_REG6    0x11
#define     PHY_REALTEK_INIT7  0x1000
#define     PHY_REALTEK_INIT9  0x0008
#define PHY_REALTEK_INIT_REG3    0x13
#define     PHY_REALTEK_INIT4  0xad17
#define PHY_REALTEK_INIT_REG4    0x14
#define     PHY_REALTEK_INIT5  0xfb54
#define PHY_REALTEK_REVISION     0x17
#define     PHY_REV_MASK             0x0001
#define     PHY_REV_REALTEK_8211B    0x0000
#define     PHY_REV_REALTEK_8211C    0x0001
#define PHY_REALTEK_INIT_REG5    0x18
#define     PHY_REALTEK_INIT6  0xf5c7
#define PHY_REALTEK_INIT_REG2    0x19
#define     PHY_REALTEK_INIT2      0x8e00
#define     PHY_REALTEK_INIT8      0x0003
#define     PHY_REALTEK_INIT_MSK1  0x0003
#define PHY_REALTEK_INIT_REG1    0x1f
#define     PHY_REALTEK_INIT1  0x0000
#define     PHY_REALTEK_INIT3  0x0001
#define     PHY_REALTEK_INIT10 0x0005

#define NV_PAUSEFRAME_RX_CAPABLE 0x0001
#define NV_PAUSEFRAME_TX_CAPABLE 0x0002
#define NV_PAUSEFRAME_RX_ENABLE  0x0004
#define NV_PAUSEFRAME_TX_ENABLE  0x0008
#define NV_PAUSEFRAME_RX_REQ     0x0010
#define NV_PAUSEFRAME_TX_REQ     0x0020
#define NV_PAUSEFRAME_AUTONEG    0x0040

#define NV_MSI_X_MAX_VECTORS  8
#define NV_MSI_X_VECTORS_MASK 0x000f
#define NV_MSI_CAPABLE        0x0010
#define NV_MSI_X_CAPABLE      0x0020
#define NV_MSI_ENABLED        0x0040
#define NV_MSI_X_ENABLED      0x0080

#define NV_MSI_X_VECTOR_ALL   0x0
#define NV_MSI_X_VECTOR_RX    0x0
#define NV_MSI_X_VECTOR_TX    0x1
#define NV_MSI_X_VECTOR_OTHER 0x2

#define NV_MSI_PRIV_OFFSET 0x68
#define NV_MSI_PRIV_VALUE  0xffffffff

#define NV_TX_LIMIT_COUNT     16
