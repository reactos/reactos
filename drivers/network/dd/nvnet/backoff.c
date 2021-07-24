/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Re-seeding random values for the backoff algorithms
 * COPYRIGHT:   Copyright 2021-2022 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * HW access code was taken from the Linux forcedeth driver
 * Copyright (C) 2003,4,5 Manfred Spraul
 * Copyright (C) 2004 Andrew de Quincey
 * Copyright (C) 2004 Carl-Daniel Hailfinger
 * Copyright (c) 2004,2005,2006,2007,2008,2009 NVIDIA Corporation
 */

/* INCLUDES *******************************************************************/

#include "nvnet.h"

#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

#define BACKOFF_SEEDSET_ROWS    8
#define BACKOFF_SEEDSET_LFSRS   15

#define REVERSE_SEED(s) ((((s) & 0xF00) >> 8) | ((s) & 0x0F0) | (((s) & 0x00F) << 8))

static const ULONG NvpMainSeedSet[BACKOFF_SEEDSET_ROWS][BACKOFF_SEEDSET_LFSRS] =
{
    {145, 155, 165, 175, 185, 196, 235, 245, 255, 265, 275, 285, 660, 690, 874},
    {245, 255, 265, 575, 385, 298, 335, 345, 355, 366, 375, 385, 761, 790, 974},
    {145, 155, 165, 175, 185, 196, 235, 245, 255, 265, 275, 285, 660, 690, 874},
    {245, 255, 265, 575, 385, 298, 335, 345, 355, 366, 375, 386, 761, 790, 974},
    {266, 265, 276, 585, 397, 208, 345, 355, 365, 376, 385, 396, 771, 700, 984},
    {266, 265, 276, 586, 397, 208, 346, 355, 365, 376, 285, 396, 771, 700, 984},
    {366, 365, 376, 686, 497, 308, 447, 455, 466, 476, 485, 496, 871, 800,  84},
    {466, 465, 476, 786, 597, 408, 547, 555, 566, 576, 585, 597, 971, 900, 184}
};

static const ULONG NvpGearSeedSet[BACKOFF_SEEDSET_ROWS][BACKOFF_SEEDSET_LFSRS] =
{
    {251, 262, 273, 324, 319, 508, 375, 364, 341, 371, 398, 193, 375,  30, 295},
    {351, 375, 373, 469, 551, 639, 477, 464, 441, 472, 498, 293, 476, 130, 395},
    {351, 375, 373, 469, 551, 639, 477, 464, 441, 472, 498, 293, 476, 130, 397},
    {251, 262, 273, 324, 319, 508, 375, 364, 341, 371, 398, 193, 375,  30, 295},
    {251, 262, 273, 324, 319, 508, 375, 364, 341, 371, 398, 193, 375,  30, 295},
    {351, 375, 373, 469, 551, 639, 477, 464, 441, 472, 498, 293, 476, 130, 395},
    {351, 375, 373, 469, 551, 639, 477, 464, 441, 472, 498, 293, 476, 130, 395},
    {351, 375, 373, 469, 551, 639, 477, 464, 441, 472, 498, 293, 476, 130, 395}
};

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
VOID
NvNetBackoffSetSlotTime(
    _In_ PNVNET_ADAPTER Adapter)
{
    LARGE_INTEGER Sample = KeQueryPerformanceCounter(NULL);

    PAGED_CODE();

    if ((Sample.LowPart & NVREG_SLOTTIME_MASK) == 0)
    {
        Sample.LowPart = 8;
    }

    if (Adapter->Features & (DEV_HAS_HIGH_DMA | DEV_HAS_LARGEDESC))
    {
        if (Adapter->Features & DEV_HAS_GEAR_MODE)
        {
            NV_WRITE(Adapter, NvRegSlotTime, NVREG_SLOTTIME_10_100_FULL);
            NvNetBackoffReseedEx(Adapter);
        }
        else
        {
            NV_WRITE(Adapter, NvRegSlotTime, (Sample.LowPart & NVREG_SLOTTIME_MASK) |
                     NVREG_SLOTTIME_LEGBF_ENABLED | NVREG_SLOTTIME_10_100_FULL);
        }
    }
    else
    {
        NV_WRITE(Adapter, NvRegSlotTime,
                 (Sample.LowPart & NVREG_SLOTTIME_MASK) | NVREG_SLOTTIME_DEFAULT);
    }
}

VOID
NvNetBackoffReseed(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG SlotTime;
    BOOLEAN RestartTransmitter = FALSE;
    LARGE_INTEGER Sample = KeQueryPerformanceCounter(NULL);

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    if ((Sample.LowPart & NVREG_SLOTTIME_MASK) == 0)
    {
        Sample.LowPart = 8;
    }

    SlotTime = NV_READ(Adapter, NvRegSlotTime) & ~NVREG_SLOTTIME_MASK;
    SlotTime |= Sample.LowPart & NVREG_SLOTTIME_MASK;

    if (NV_READ(Adapter, NvRegTransmitterControl) & NVREG_XMITCTL_START)
    {
        RestartTransmitter = TRUE;
        NvNetStopTransmitter(Adapter);
    }
    NvNetStopReceiver(Adapter);

    NV_WRITE(Adapter, NvRegSlotTime, SlotTime);

    if (RestartTransmitter)
    {
        NvNetStartTransmitter(Adapter);
    }
    NvNetStartReceiver(Adapter);
}

VOID
NvNetBackoffReseedEx(
    _In_ PNVNET_ADAPTER Adapter)
{
    LARGE_INTEGER Sample;
    ULONG Seed[3], ReversedSeed[2], CombinedSeed, SeedSet;
    ULONG i, Temp;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    Sample = KeQueryPerformanceCounter(NULL);
    Seed[0] = Sample.LowPart & 0x0FFF;
    if (Seed[0] == 0)
    {
        Seed[0] = 0x0ABC;
    }

    Sample = KeQueryPerformanceCounter(NULL);
    Seed[1] = Sample.LowPart & 0x0FFF;
    if (Seed[1] == 0)
    {
        Seed[1] = 0x0ABC;
    }
    ReversedSeed[0] = REVERSE_SEED(Seed[1]);

    Sample = KeQueryPerformanceCounter(NULL);
    Seed[2] = Sample.LowPart & 0x0FFF;
    if (Seed[2] == 0)
    {
        Seed[2] = 0x0ABC;
    }
    ReversedSeed[1] = REVERSE_SEED(Seed[2]);

    CombinedSeed = ((Seed[0] ^ ReversedSeed[0]) << 12) | (Seed[1] ^ ReversedSeed[1]);
    if ((CombinedSeed & NVREG_BKOFFCTRL_SEED_MASK) == 0)
    {
        CombinedSeed |= 8;
    }
    if ((CombinedSeed & (NVREG_BKOFFCTRL_SEED_MASK << NVREG_BKOFFCTRL_GEAR)) == 0)
    {
        CombinedSeed |= 8 << NVREG_BKOFFCTRL_GEAR;
    }

    /* No need to disable transmitter here */
    Temp = NVREG_BKOFFCTRL_DEFAULT | (0 << NVREG_BKOFFCTRL_SELECT);
    Temp |= CombinedSeed & NVREG_BKOFFCTRL_SEED_MASK;
    Temp |= CombinedSeed >> NVREG_BKOFFCTRL_GEAR;
    NV_WRITE(Adapter, NvRegBackOffControl, Temp);

    /* Setup seeds for all gear LFSRs */
    Sample = KeQueryPerformanceCounter(NULL);
    SeedSet = Sample.LowPart % BACKOFF_SEEDSET_ROWS;
    for (i = 1; i <= BACKOFF_SEEDSET_LFSRS; ++i)
    {
        Temp = NVREG_BKOFFCTRL_DEFAULT | (i << NVREG_BKOFFCTRL_SELECT);
        Temp |= NvpMainSeedSet[SeedSet][i - 1] & NVREG_BKOFFCTRL_SEED_MASK;
        Temp |= (NvpGearSeedSet[SeedSet][i - 1] & NVREG_BKOFFCTRL_SEED_MASK)
                << NVREG_BKOFFCTRL_GEAR;
        NV_WRITE(Adapter, NvRegBackOffControl, Temp);
    }
}
