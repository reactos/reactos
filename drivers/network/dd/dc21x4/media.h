/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Media support header file
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

typedef struct _DC_MEDIA
{
    ULONG OpMode;

    USHORT GpioCtrl;
    USHORT GpioData;

    union
    {
        struct _DC_MEDIA_SIA_DATA
        {
            ULONG Csr13;
            ULONG Csr14;
            ULONG Csr15;
        };

        struct _DC_MEDIA_GPR_DATA
        {
            ULONG LinkMask;
            ULONG Polarity;
        };
    };
} DC_MEDIA, *PDC_MEDIA;

typedef struct _DC_MII_MEDIA
{
    UCHAR SetupStreamLength;
    UCHAR ResetStreamLength;
    USHORT Advertising;
    USHORT SetupStream[SROM_MAX_STREAM_REGS + 1]; // +1 for GPIO direction (21140)
    USHORT ResetStream[SROM_MAX_STREAM_REGS];
} DC_MII_MEDIA, *PDC_MII_MEDIA;

/*
 * SROM-defined media values
 */
#define MEDIA_10T                    0
#define MEDIA_BNC                    1
#define MEDIA_AUI                    2
#define MEDIA_100TX_HD               3
#define MEDIA_10T_HD                 MEDIA_100TX_HD /* 21041 only */
#define MEDIA_10T_FD                 4
#define MEDIA_100TX_FD               5
#define MEDIA_100T4                  6
#define MEDIA_100FX_HD               7
#define MEDIA_100FX_FD               8
#define MEDIA_HMR                    9
#define MEDIA_LIST_MAX               10

/*
 * Registry configuration
 */
#define MEDIA_AUTO                   MEDIA_LIST_MAX

/*
 * Extra codes
 */
#define MEDIA_MII                    MEDIA_LIST_MAX
#define MEDIA_MAX                    (MEDIA_LIST_MAX + 1)

#define MEDIA_MII_OVERRIDE_MASK \
    ((1 << MEDIA_AUI) | \
     (1 << MEDIA_BNC) | \
     (1 << MEDIA_100FX_HD) | \
     (1 << MEDIA_100FX_FD) | \
     (1 << MEDIA_HMR))

#define MEDIA_FD_MASK \
    ((1 << MEDIA_10T_FD) | \
     (1 << MEDIA_100TX_FD) | \
     (1 << MEDIA_100FX_FD))

#define MEDIA_AUI_BNC_MASK \
    ((1 << MEDIA_AUI) | \
     (1 << MEDIA_BNC))

#define MEDIA_10T_MASK \
    ((1 << MEDIA_10T) | \
     (1 << MEDIA_10T_FD))

#define MEDIA_100_MASK \
    ((1 << MEDIA_100TX_HD) | \
     (1 << MEDIA_100TX_FD) | \
     (1 << MEDIA_100T4) | \
     (1 << MEDIA_100FX_HD) | \
     (1 << MEDIA_100FX_FD))

#define MEDIA_FX_MASK \
    ((1 << MEDIA_100FX_HD) | \
     (1 << MEDIA_100FX_FD))

/* Specifying this media code override the default MII selection */
#define MEDIA_MII_OVERRIDE(MediaNumber) \
    (((1 << (MediaNumber)) & MEDIA_MII_OVERRIDE_MASK) != 0)

/* Full-duplex media */
#define MEDIA_IS_FD(MediaNumber) \
    (((1 << (MediaNumber)) & MEDIA_FD_MASK) != 0)

/* AUI or BNC media */
#define MEDIA_IS_AUI_BNC(MediaNumber) \
    (((1 << (MediaNumber)) & MEDIA_AUI_BNC_MASK) != 0)

/* 10Base-T media */
#define MEDIA_IS_10T(MediaNumber) \
    (((1 << (MediaNumber)) & MEDIA_10T_MASK) != 0)

/* 100mbps media */
#define MEDIA_IS_100(MediaNumber) \
    (((1 << (MediaNumber)) & MEDIA_100_MASK) != 0)

/* 100Base-FX media */
#define MEDIA_IS_FX(MediaNumber) \
    (((1 << (MediaNumber)) & MEDIA_FX_MASK) != 0)

/* Forced speed and duplex */
#define MEDIA_IS_FIXED(Adapter) \
    (((Adapter)->Flags & DC_AUTOSENSE) == 0)
