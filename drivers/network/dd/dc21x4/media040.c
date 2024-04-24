/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     21040 media support code
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
MediaLinkStateChange21040(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG InterruptStatus)
{
    UNREFERENCED_PARAMETER(InterruptStatus);

    INFO_VERB("Link failed, CSR12 %08lx\n", DC_READ(Adapter, DcCsr12_SiaStatus));

    NdisDprAcquireSpinLock(&Adapter->ModeLock);

    /* Start the media timer when the 10Base-T link has changed state */
    if (Adapter->MediaNumber != MEDIA_BNC)
    {
        MediaIndicateConnect(Adapter, FALSE);

        NdisMSetTimer(&Adapter->MediaMonitorTimer, 3000);
    }

    NdisDprReleaseSpinLock(&Adapter->ModeLock);
}

VOID
NTAPI
MediaMonitor21040Dpc(
    _In_ PVOID SystemSpecific1,
    _In_ PVOID FunctionContext,
    _In_ PVOID SystemSpecific2,
    _In_ PVOID SystemSpecific3)
{
    PDC21X4_ADAPTER Adapter = FunctionContext;
    BOOLEAN LinkUp, Report, RunAgain;

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);

    if (!(Adapter->Flags & DC_ACTIVE))
        return;

    LinkUp = FALSE;
    Report = FALSE;
    RunAgain = TRUE;

    NdisDprAcquireSpinLock(&Adapter->ModeLock);

    if (Adapter->MediaNumber != MEDIA_BNC)
    {
        ULONG SiaStatus = DC_READ(Adapter, DcCsr12_SiaStatus);

        Report = TRUE;
        LinkUp = !(SiaStatus & (DC_SIA_STATUS_NETWORK_CONNECTION_ERROR |
                                DC_SIA_STATUS_10T_LINK_FAIL));

        if (!LinkUp)
        {
            /* Select the other port */
            if (!MEDIA_IS_FIXED(Adapter))
            {
                Adapter->MediaNumber = MEDIA_BNC;

                DcWriteSia(Adapter,
                           Adapter->Media[MEDIA_BNC].Csr13,
                           Adapter->Media[MEDIA_BNC].Csr14,
                           Adapter->Media[MEDIA_BNC].Csr15);

                Adapter->LastReceiveActivity = (ULONG)Adapter->Statistics.ReceiveOk;
            }
        }
        else
        {
            /* Wait until the next link change event */
            RunAgain = FALSE;
        }
    }
    else
    {
        ULONG ReceiveActivity = (ULONG)Adapter->Statistics.ReceiveOk;

        if (!(Adapter->ModeFlags & DC_MODE_TEST_PACKET))
        {
            /* Check for any received packets */
            if (ReceiveActivity != Adapter->LastReceiveActivity)
            {
                LinkUp = TRUE;
                Report = TRUE;
            }
            else
            {
                /* Send a loopback packet */
                NdisDprAcquireSpinLock(&Adapter->SendLock);
                DcTestPacket(Adapter);
                NdisDprReleaseSpinLock(&Adapter->SendLock);
            }
        }
        else
        {
            Adapter->ModeFlags &= ~DC_MODE_TEST_PACKET;

            LinkUp = !!Adapter->MediaTestStatus;
            Report = TRUE;

            /* Select the other port */
            if (!LinkUp && !MEDIA_IS_FIXED(Adapter))
            {
                Adapter->MediaNumber = MEDIA_10T;

                DcWriteSia(Adapter,
                           Adapter->Media[MEDIA_10T].Csr13,
                           Adapter->Media[MEDIA_10T].Csr14,
                           Adapter->Media[MEDIA_10T].Csr15);
            }
        }
        Adapter->LastReceiveActivity = ReceiveActivity;
    }

    if (Report)
    {
        MediaIndicateConnect(Adapter, LinkUp);
    }

    NdisDprReleaseSpinLock(&Adapter->ModeLock);

    if (RunAgain)
    {
        NdisMSetTimer(&Adapter->MediaMonitorTimer, LinkUp ? 6000 : 3000);
    }
}
