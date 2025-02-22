/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     21140 media support code
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
MediaGprCheckLink(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG GpioData;
    PDC_MEDIA Media;

    Media = &Adapter->Media[Adapter->MediaNumber];

    /* No media activity indicator */
    if (Media->LinkMask == 0)
    {
        TRACE("No activity indicator\n");

        /* Assume we have a link */
        if (MEDIA_IS_FIXED(Adapter))
            return TRUE;

        return FALSE;
    }

    GpioData = DC_READ(Adapter, DcCsr12_Gpio);

    TRACE("CSR12 %08lx\n", GpioData);

    /* This media supports link indication via GPIO pins */
    return !!((GpioData & Media->LinkMask) ^ Media->Polarity);
}

static
ULONG
MediaGprNextMedia(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG i;

    /* No half-duplex media in the media list */
    if (!(Adapter->MediaBitmap & ~MEDIA_FD_MASK))
    {
        return Adapter->MediaNumber;
    }

    /* Find the next half-duplex media */
    i = Adapter->MediaNumber;
    while (TRUE)
    {
        /* We have reached the end of the media list, try the first media */
        if (i == 0)
        {
            _BitScanReverse(&i, Adapter->MediaBitmap);
        }
        else
        {
            --i;
        }

        if ((Adapter->MediaBitmap & (1 << i)) && !MEDIA_IS_FD(i))
        {
            return i;
        }
    }
}

VOID
NTAPI
MediaMonitor21140Dpc(
    _In_ PVOID SystemSpecific1,
    _In_ PVOID FunctionContext,
    _In_ PVOID SystemSpecific2,
    _In_ PVOID SystemSpecific3)
{
    PDC21X4_ADAPTER Adapter = FunctionContext;
    ULONG DelayMs, MediaNumber;
    BOOLEAN LinkUp;

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);

    if (!(Adapter->Flags & DC_ACTIVE))
        return;

    NdisDprAcquireSpinLock(&Adapter->ModeLock);

    if (Adapter->MediaNumber == MEDIA_MII)
    {
        LinkUp = MediaMiiCheckLink(Adapter);

        DelayMs = 5000;
    }
    else
    {
        LinkUp = MediaGprCheckLink(Adapter);

        /* This media is unconnected, try the next media */
        if (!LinkUp && !MEDIA_IS_FIXED(Adapter))
        {
            MediaNumber = MediaGprNextMedia(Adapter);

            if (Adapter->MediaNumber != MediaNumber)
            {
                Adapter->MediaNumber = MediaNumber;
                MediaGprSelect(Adapter);
            }

            DelayMs = 3000;
        }
        else
        {
            /* If we are forcing media, then we need to poll the media less frequently */
            DelayMs = 5000;
        }
    }

    MediaIndicateConnect(Adapter, LinkUp);

    NdisDprReleaseSpinLock(&Adapter->ModeLock);

    NdisMSetTimer(&Adapter->MediaMonitorTimer, DelayMs);
}
