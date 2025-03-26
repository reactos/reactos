/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     EEPROM specific definitions
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#include <pshpack1.h>
typedef struct _DC_SROM_COMPACT_BLOCK
{
    USHORT SelectedConnectionType;
    UCHAR GpioDirection; /* 21140 only */
    UCHAR BlockCount;
} DC_SROM_COMPACT_BLOCK, *PDC_SROM_COMPACT_BLOCK;
#include <poppack.h>

typedef struct _DC_SROM_REPAIR_ENTRY
{
#if DBG
    PCSTR Name;
#endif
    PUCHAR InfoLeaf;
    ULONG Length;
} DC_SROM_REPAIR_ENTRY, *PDC_SROM_REPAIR_ENTRY;

typedef struct _DC_SROM_ENTRY
{
    LIST_ENTRY ListEntry;
    ULONG BusNumber;
    DC_CHIP_TYPE ChipType;
    UCHAR DeviceNumber;
    ULONG InterruptLevel;
    ULONG InterruptVector;
    ULONG DeviceBitmap;
    UCHAR SRomImage[ANYSIZE_ARRAY];
} DC_SROM_ENTRY, *PDC_SROM_ENTRY;

#define SRomIsBlockExtended(Byte)           ((Byte) & 0x80)
#define SRomGetExtendedBlockLength(Byte)    (((Byte) & 0x7F) + 1)
#define SRomGetMediaCode(Byte)              ((Byte) & 0x3F)
#define SRomBlockHasExtendedData(Byte)      ((Byte) & 0x40)
#define SRomIsDefaultMedia(Word)            ((Word) & 0x4000)
#define SRomMediaHasActivityIndicator(Word) (((Word) & 0x8000) == 0)
#define SRomMediaActivityIsActiveLow(Word)  ((Word) & 0x80)
#define SRomMediaGetSenseMask(Word)         (1 << (((Word) & 0x0E) >> 1))
#define SRomCommandToOpMode(Word)           (((Word) & 0x71) << 18)
#define SRomMediaAutoSense(Media)           ((Media) & 0x800)
#define SRomMediaToMediaNumber(Word)        ((Word) & 0x1F)
#define SRomHmrRegAddress(Byte)             ((Byte) & 0x1F)

#define SROM_OPMODE_MASK \
    (DC_OPMODE_PORT_SELECT | \
     DC_OPMODE_PORT_XMIT_10 | \
     DC_OPMODE_PORT_PCS | \
     DC_OPMODE_PORT_SCRAMBLER)

#define EE_SIZE     128
#define EAR_SIZE    32

#define EAR_TEST_PATTERN    0xAA5500FFAA5500FFULL

#define EEPROM_CMD_WRITE    5
#define EEPROM_CMD_READ     6
#define EEPROM_CMD_ERASE    7

#define EEPROM_CMD_LENGTH   3

/*
 * Various offsets in the SROM
 */
#define SROM_VERSION                18
#define SROM_CONTROLLER_COUNT       19
#define SROM_MAC_ADDRESS            20
#define SROM_DEVICE_NUMBER(n)       (26 + ((n) * 3))
#define SROM_LEAF_OFFSET(n)         (27 + ((n) * 3))
#define SROM_CHECKSUM_V1            126
#define SROM_CHECKSUM_V2            94

/*
 * SROM compact and extended format types
 */
#define SROM_BLOCK_TYPE_GPR             0
#define SROM_BLOCK_TYPE_MII_1           1
#define SROM_BLOCK_TYPE_SIA             2
#define SROM_BLOCK_TYPE_MII_2           3
#define SROM_BLOCK_TYPE_SYM             4
#define SROM_BLOCK_TYPE_RESET           5
#define SROM_BLOCK_TYPE_PHY_SHUTDOWN    6
#define SROM_BLOCK_TYPE_HOMERUN         7

#define SROM_MAX_STREAM_REGS    6

/*
 * SROM media codes
 */
#define SROM_MEDIA_10T          0
#define SROM_MEDIA_BNC          1
#define SROM_MEDIA_AUI          2
#define SROM_MEDIA_100T_HD      3
#define SROM_MEDIA_10T_FD       4
#define SROM_MEDIA_100TX_FD     5
#define SROM_MEDIA_100T4        6
#define SROM_MEDIA_100FX_HD     7
#define SROM_MEDIA_100FX_FD     8
#define SROM_MEDIA_MAX          8
#define SROM_MEDIA_HMR          18
