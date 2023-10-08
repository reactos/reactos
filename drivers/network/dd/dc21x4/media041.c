/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     21041 media support code
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
MediaLinkStateChange21041(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG InterruptStatus)
{
    NdisDprAcquireSpinLock(&Adapter->ModeLock);

    if (InterruptStatus & DC_IRQ_LINK_PASS)
    {
        INFO_VERB("Link passed, CSR12 %08lx\n", DC_READ(Adapter, DcCsr12_SiaStatus));

        /* 10Base-T is the active port now */
        if (MEDIA_IS_AUI_BNC(Adapter->MediaNumber))
        {
            /* Switch to TP medium */
            if (!MEDIA_IS_FIXED(Adapter))
            {
                Adapter->MediaNumber = MEDIA_10T;
                MediaSiaSelect(Adapter);

                NdisMSetTimer(&Adapter->MediaMonitorTimer, 3000);
            }
        }

        /* 10Base-T link is up */
        if (!MEDIA_IS_AUI_BNC(Adapter->MediaNumber))
        {
            MediaIndicateConnect(Adapter, TRUE);
        }
    }
    else // DC_IRQ_LINK_FAIL
    {
        INFO_VERB("Link failed, CSR12 %08lx\n", DC_READ(Adapter, DcCsr12_SiaStatus));

        /* 10Base-T link is down */
        if (!MEDIA_IS_AUI_BNC(Adapter->MediaNumber))
        {
            MediaIndicateConnect(Adapter, FALSE);
        }
    }

    NdisDprReleaseSpinLock(&Adapter->ModeLock);
}

static
VOID
Media041SelectNextMedia(
    _In_ PDC21X4_ADAPTER Adapter)
{
    if (Adapter->MediaNumber == MEDIA_AUI)
        Adapter->ModeFlags |= DC_MODE_AUI_FAILED;
    else if (Adapter->MediaNumber == MEDIA_BNC)
        Adapter->ModeFlags |= DC_MODE_BNC_FAILED;

    if ((Adapter->ModeFlags & (DC_MODE_AUI_FAILED | DC_MODE_BNC_FAILED)) ==
        (DC_MODE_AUI_FAILED | DC_MODE_BNC_FAILED))
    {
        Adapter->MediaNumber = MEDIA_10T;
    }
    else
    {
        Adapter->MediaNumber = (MEDIA_BNC + MEDIA_AUI) - Adapter->MediaNumber;
    }

    MediaSiaSelect(Adapter);
}

VOID
NTAPI
MediaMonitor21041Dpc(
    _In_ PVOID SystemSpecific1,
    _In_ PVOID FunctionContext,
    _In_ PVOID SystemSpecific2,
    _In_ PVOID SystemSpecific3)
{
    PDC21X4_ADAPTER Adapter = FunctionContext;
    BOOLEAN LinkUp, Report;
    ULONG DelayMs, SiaStatus;

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);

    if (!(Adapter->Flags & DC_ACTIVE))
        return;

    Report = FALSE;
    DelayMs = 5000;

    NdisDprAcquireSpinLock(&Adapter->ModeLock);

    SiaStatus = DC_READ(Adapter, DcCsr12_SiaStatus);

    if (MEDIA_IS_AUI_BNC(Adapter->MediaNumber))
    {
        if ((Adapter->ModeFlags & DC_MODE_PORT_AUTOSENSE))
        {
            Adapter->ModeFlags &= ~DC_MODE_PORT_AUTOSENSE;

            /* Select the other port */
            if (!(SiaStatus & DC_SIA_STATUS_SELECTED_PORT_ACTIVITY))
            {
                Adapter->MediaNumber = MEDIA_BNC;
                MediaSiaSelect(Adapter);
            }

            DelayMs = 1000;
        }
        else
        {
            ULONG ReceiveActivity = (ULONG)Adapter->Statistics.ReceiveOk;

            if (!(Adapter->ModeFlags & DC_MODE_TEST_PACKET))
            {
                if ((Adapter->MediaNumber == MEDIA_AUI) &&
                    (SiaStatus & DC_SIA_STATUS_SELECTED_PORT_ACTIVITY))
                {
                    /* Clear the selected port activity bit */
                    DC_WRITE(Adapter, DcCsr12_SiaStatus, DC_SIA_STATUS_SELECTED_PORT_ACTIVITY);

                    LinkUp = TRUE;
                    Report = TRUE;
                }
                /* Check for any received packets */
                else if (ReceiveActivity != Adapter->LastReceiveActivity)
                {
                    LinkUp = TRUE;
                    Report = TRUE;

                    DelayMs = 3000;
                }
                else
                {
                    /* Send a loopback packet */
                    NdisDprAcquireSpinLock(&Adapter->SendLock);
                    DcTestPacket(Adapter);
                    NdisDprReleaseSpinLock(&Adapter->SendLock);

                    DelayMs = 3000;
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
                    Media041SelectNextMedia(Adapter);

                    DelayMs = 3000;
                }
            }
            Adapter->LastReceiveActivity = ReceiveActivity;
        }
    }
    else // 10Base-T
    {
        Report = TRUE;
        LinkUp = !(SiaStatus & (DC_SIA_STATUS_NETWORK_CONNECTION_ERROR |
                                DC_SIA_STATUS_10T_LINK_FAIL));

        if (!LinkUp)
        {
            DelayMs = 3000;

            /* Select the AUI or BNC port */
            if (!MEDIA_IS_FIXED(Adapter) && (Adapter->MediaBitmap & MEDIA_AUI_BNC_MASK))
            {
                Adapter->ModeFlags &= ~(DC_MODE_AUI_FAILED |
                                        DC_MODE_BNC_FAILED |
                                        DC_MODE_TEST_PACKET);

                if (SiaStatus & DC_SIA_STATUS_NONSEL_PORT_ACTIVITY)
                {
                    Adapter->MediaNumber = MEDIA_AUI;
                    Adapter->ModeFlags |= DC_MODE_PORT_AUTOSENSE;
                }
                else
                {
                    Adapter->MediaNumber = MEDIA_BNC;
                }
                MediaSiaSelect(Adapter);

                Adapter->LastReceiveActivity = (ULONG)Adapter->Statistics.ReceiveOk;

                /* Clear the port activity bits */
                DC_WRITE(Adapter, DcCsr12_SiaStatus, DC_SIA_STATUS_SELECTED_PORT_ACTIVITY |
                                                     DC_SIA_STATUS_NONSEL_PORT_ACTIVITY);

                DelayMs = 1000;
            }
        }
    }

    if (Report)
    {
        MediaIndicateConnect(Adapter, LinkUp);
    }

    NdisDprReleaseSpinLock(&Adapter->ModeLock);

    NdisMSetTimer(&Adapter->MediaMonitorTimer, DelayMs);
}
