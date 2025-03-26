/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     21142/21143/21145 media support code
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
VOID
Media143SelectNextSerialMedia(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG MediaNumber;

    MediaNumber = Adapter->MediaNumber;

    /* The HMR media isn't checked as HMR boards use it instead of AUI and BNC */
    if (MediaNumber == MEDIA_AUI || MediaNumber == MEDIA_BNC)
    {
        if (MediaNumber == MEDIA_AUI)
            Adapter->ModeFlags |= DC_MODE_AUI_FAILED;
        else if (MediaNumber == MEDIA_BNC)
            Adapter->ModeFlags |= DC_MODE_BNC_FAILED;

        if ((Adapter->ModeFlags & (DC_MODE_AUI_FAILED | DC_MODE_BNC_FAILED)) !=
            (DC_MODE_AUI_FAILED | DC_MODE_BNC_FAILED))
        {
            MediaNumber = (MEDIA_BNC + MEDIA_AUI) - MediaNumber;

            if (Adapter->MediaBitmap & (1 << MediaNumber))
            {
                Adapter->MediaNumber = MediaNumber;
                MediaSiaSelect(Adapter);
                return;
            }
        }
    }

    if (Adapter->Features & DC_HAS_MII)
    {
        Adapter->MediaNumber = MEDIA_MII;

        DcStopTxRxProcess(Adapter);
        MediaSelectMiiPort(Adapter, FALSE);
        MediaMiiSelect(Adapter);
    }
    else
    {
        Adapter->MediaNumber = MEDIA_10T;
        MediaSiaSelect(Adapter);
    }
}

static
VOID
Media143SelectNextMedia(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG SiaStatus)
{
    ULONG MediaBitmap, MediaNumber;

    MediaIndicateConnect(Adapter, FALSE);

    MediaBitmap = Adapter->MediaBitmap;

    if (MediaBitmap & (1 << MEDIA_HMR))
    {
        MediaNumber = MEDIA_HMR;
    }
    else if ((MediaBitmap & MEDIA_AUI_BNC_MASK) == MEDIA_AUI_BNC_MASK)
    {
        if (SiaStatus & DC_SIA_STATUS_AUI_ACTIVITY)
        {
            MediaNumber = MEDIA_AUI;
        }
        else
        {
            MediaNumber = MEDIA_BNC;
        }
    }
    else if (MediaBitmap & (1 << MEDIA_AUI))
    {
        MediaNumber = MEDIA_AUI;
    }
    else if (MediaBitmap & (1 << MEDIA_BNC))
    {
        MediaNumber = MEDIA_BNC;
    }
    else
    {
        MediaNumber = MEDIA_10T;
    }

    Adapter->ModeFlags &= ~DC_MODE_AUTONEG_MASK;
    Adapter->ModeFlags |= DC_MODE_AUTONEG_NONE;
    NdisMSetTimer(&Adapter->MediaMonitorTimer, 3000);

    if (Adapter->MediaNumber != MediaNumber)
    {
        Adapter->MediaNumber = MediaNumber;
        MediaSiaSelect(Adapter);
    }

    Adapter->ModeFlags &= ~(DC_MODE_TEST_PACKET |
                            DC_MODE_AUI_FAILED |
                            DC_MODE_BNC_FAILED);
    Adapter->LastReceiveActivity = (ULONG)Adapter->Statistics.ReceiveOk;
}

static
VOID
Media143Handle10LinkFail(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG SiaStatus)
{
    INFO_VERB("Link failed, CSR12 %08lx\n", SiaStatus);

    /* 10Base-T link is down */
    MediaIndicateConnect(Adapter, FALSE);

    /* Select the other port */
    if (!MEDIA_IS_FIXED(Adapter))
    {
        Media143SelectNextMedia(Adapter, SiaStatus);
    }
}

static
VOID
Media143Handle10LinkPass(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG SiaStatus)
{
    INFO_VERB("Link passed, CSR12 %08lx\n", SiaStatus);

    /* 10Base-T is the active port now */
    if (!MEDIA_IS_10T(Adapter->MediaNumber))
    {
        /* Switch to TP medium */
        if (!MEDIA_IS_FIXED(Adapter))
        {
            Adapter->MediaNumber = MEDIA_10T;
            MediaSiaSelect(Adapter);

            /* Wait for a link pass interrupt to signal the link test completed */
            Adapter->ModeFlags &= ~DC_MODE_AUTONEG_MASK;
            Adapter->ModeFlags |= DC_MODE_AUTONEG_WAIT_INTERRUPT;
            NdisMSetTimer(&Adapter->MediaMonitorTimer, 3000);
        }
    }
    else
    {
        /* 10Base-T link is up */
        MediaIndicateConnect(Adapter, TRUE);
    }
}

static
VOID
Media143Handle100LinkChange(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG SiaStatus)
{
    BOOLEAN LinkUp;

    INFO_VERB("Link changed, CSR12 %08lx\n", SiaStatus);

    LinkUp = !(SiaStatus & DC_SIA_STATUS_100T_LINK_FAIL);

    if (MEDIA_IS_FIXED(Adapter))
    {
        MediaIndicateConnect(Adapter, LinkUp);
    }
    else
    {
        /* Select the other port */
        if (!LinkUp)
        {
            Media143SelectNextMedia(Adapter, SiaStatus);
        }
        else
        {
            /* Ignore this hint */
        }
    }
}

static
VOID
Media143HandleNWayComplete(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG SiaStatus)
{
    ULONG MediaNumber, AdvLpa;

    /* Select media according to auto-negotiation result */
    if (SiaStatus & DC_SIA_STATUS_LP_AUTONED_SUPPORTED)
    {
        INFO_VERB("Auto-negotiation has completed, LPA %08lx ADV %08lx\n",
                  SiaStatus, Adapter->SymAdvertising);

        AdvLpa = (SiaStatus >> DC_SIA_STATUS_LP_CODE_WORD_SHIFT) & Adapter->SymAdvertising;
        if (AdvLpa & MII_ADV_100T_FD)
        {
            MediaNumber = MEDIA_100TX_FD;
        }
        else if (AdvLpa & MII_ADV_100T4)
        {
            MediaNumber = MEDIA_100T4;
        }
        else if (AdvLpa & MII_ADV_100T_HD)
        {
            MediaNumber = MEDIA_100TX_HD;
        }
        else if (AdvLpa & MII_ADV_10T_FD)
        {
            MediaNumber = MEDIA_10T_FD;
        }
        else if (AdvLpa & MII_ADV_10T_HD)
        {
            MediaNumber = MEDIA_10T;
        }
        else
        {
            INFO_VERB("No common mode\n");

            /* No common mode, select the other port */
            Media143SelectNextMedia(Adapter, SiaStatus);
            return;
        }
    }
    else
    {
        INFO_VERB("Link partner does not support auto-negotiation, CSR12 %08lx\n", SiaStatus);

        /* Check the results of parallel detection */
        if (!(SiaStatus & DC_SIA_STATUS_100T_LINK_FAIL))
        {
            MediaNumber = MEDIA_100TX_HD;
        }
        else if (!(SiaStatus & DC_SIA_STATUS_10T_LINK_FAIL))
        {
            MediaNumber = MEDIA_10T;
        }
        else
        {
            /* No link detected, select the other port */
            Media143SelectNextMedia(Adapter, SiaStatus);
            return;
        }
    }

    if (MEDIA_IS_10T(MediaNumber) && (MediaNumber != Adapter->MediaNumber))
    {
        /* Set the time limit for auto-negotiation */
        Adapter->ModeFlags &= ~DC_MODE_AUTONEG_MASK;
        Adapter->ModeFlags |= DC_MODE_AUTONEG_WAIT_INTERRUPT;
        NdisMSetTimer(&Adapter->MediaMonitorTimer, 5000);
    }
    else
    {
        /* Wait for the link integrity test to complete before we can read the link status */
        Adapter->ModeFlags &= ~DC_MODE_AUTONEG_MASK;
        Adapter->ModeFlags |= DC_MODE_AUTONEG_LINK_STATUS_CHECK;
        NdisMSetTimer(&Adapter->MediaMonitorTimer, 1000);
    }

    if (Adapter->MediaNumber != MediaNumber)
    {
        Adapter->MediaNumber = MediaNumber;
        MediaSiaSelect(Adapter);
    }
}

VOID
MediaLinkStateChange21143(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG InterruptStatus)
{
    ULONG SiaStatus;

    INFO_VERB("Link interrupt, CSR5 %08lx\n", InterruptStatus);

    NdisDprAcquireSpinLock(&Adapter->ModeLock);

    /* Ignore link changes caused by media being estabilished */
    if ((Adapter->ModeFlags & DC_MODE_AUTONEG_MASK) == DC_MODE_AUTONEG_LINK_STATUS_CHECK)
    {
        NdisDprReleaseSpinLock(&Adapter->ModeLock);
        return;
    }

    SiaStatus = DC_READ(Adapter, DcCsr12_SiaStatus);

    if ((InterruptStatus & DC_IRQ_LINK_FAIL) && MEDIA_IS_10T(Adapter->MediaNumber))
    {
        /* Link has failed */
        Media143Handle10LinkFail(Adapter, SiaStatus);
    }
    else if (InterruptStatus & DC_IRQ_LINK_PASS)
    {
        if (DC_READ(Adapter, DcCsr14_SiaTxRx) & DC_SIA_TXRX_AUTONEG)
        {
            /* Auto-negotiation has completed */
            Media143HandleNWayComplete(Adapter, SiaStatus);
        }
        else
        {
            /* Link has passed */
            Media143Handle10LinkPass(Adapter, SiaStatus);
        }
    }
    else
    {
        /* NOTE: The Link Changed bit is reserved on the 21142 and always reads as 1 */
        if (InterruptStatus & Adapter->LinkStateChangeMask & DC_IRQ_LINK_CHANGED)
        {
            /* Link has changed */
            Media143Handle100LinkChange(Adapter, SiaStatus);
        }
    }

    NdisDprReleaseSpinLock(&Adapter->ModeLock);
}

static
BOOLEAN
Media143CheckLink(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG SiaStatus)
{
    if (MEDIA_IS_100(Adapter->MediaNumber))
    {
        if (SiaStatus & DC_SIA_STATUS_100T_LINK_FAIL)
            return FALSE;
    }
    else
    {
        /* The auto-negotiation process can be restarted upon link failure in 10Base-T mode */
        if ((SiaStatus & DC_SIA_STATUS_ANS_MASK) != DC_SIA_STATUS_ANS_AUTONEG_COMPLETE)
            return FALSE;

        if (SiaStatus & DC_SIA_STATUS_10T_LINK_FAIL)
            return FALSE;
    }

    return TRUE;
}

static
VOID
MediaMonitor143(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG SiaStatus;
    BOOLEAN LinkUp;

    SiaStatus = DC_READ(Adapter, DcCsr12_SiaStatus);

    switch (Adapter->ModeFlags & DC_MODE_AUTONEG_MASK)
    {
        case DC_MODE_AUTONEG_WAIT_INTERRUPT:
        {
            /* Timeout, select the other port */
            Media143SelectNextMedia(Adapter, SiaStatus);
            break;
        }

        case DC_MODE_AUTONEG_LINK_STATUS_CHECK:
        {
            /* Check the link status */
            LinkUp = Media143CheckLink(Adapter, SiaStatus);
            if (LinkUp)
            {
                Adapter->ModeFlags &= ~DC_MODE_AUTONEG_MASK;
                Adapter->ModeFlags |= DC_MODE_AUTONEG_NONE;

                MediaIndicateConnect(Adapter, TRUE);
            }
            else
            {
                /* No link detected, select the other port */
                Media143SelectNextMedia(Adapter, SiaStatus);
            }
            break;
        }

        case DC_MODE_AUTONEG_NONE:
        {
            break;
        }

        default:
            ASSERT(FALSE);
            UNREACHABLE;
            break;
    }
}

VOID
NTAPI
MediaMonitor21143Dpc(
    _In_ PVOID SystemSpecific1,
    _In_ PVOID FunctionContext,
    _In_ PVOID SystemSpecific2,
    _In_ PVOID SystemSpecific3)
{
    PDC21X4_ADAPTER Adapter = FunctionContext;
    ULONG DelayMs;
    BOOLEAN LinkUp;

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);

    if (!(Adapter->Flags & DC_ACTIVE))
        return;

    NdisDprAcquireSpinLock(&Adapter->ModeLock);

    switch (Adapter->MediaNumber)
    {
        case MEDIA_MII:
        {
            LinkUp = MediaMiiCheckLink(Adapter);

            MediaIndicateConnect(Adapter, LinkUp);

            NdisMSetTimer(&Adapter->MediaMonitorTimer, 5000);
            break;
        }

        case MEDIA_AUI:
        case MEDIA_BNC:
        case MEDIA_HMR:
        {
            ULONG ReceiveActivity = (ULONG)Adapter->Statistics.ReceiveOk;

            if (!(Adapter->ModeFlags & DC_MODE_TEST_PACKET))
            {
                if ((Adapter->MediaNumber == MEDIA_AUI || Adapter->MediaNumber == MEDIA_HMR) &&
                    (DC_READ(Adapter, DcCsr12_SiaStatus) & DC_SIA_STATUS_AUI_ACTIVITY))
                {
                    /* Clear the AUI/HMR port activity bit */
                    DC_WRITE(Adapter, DcCsr12_SiaStatus, DC_SIA_STATUS_AUI_ACTIVITY);

                    MediaIndicateConnect(Adapter, TRUE);

                    DelayMs = 5000;
                }
                /* Check for any received packets */
                else if (ReceiveActivity != Adapter->LastReceiveActivity)
                {
                    MediaIndicateConnect(Adapter, TRUE);

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

                MediaIndicateConnect(Adapter, LinkUp);

                /* Select the other port */
                if (!LinkUp && !MEDIA_IS_FIXED(Adapter))
                {
                    Media143SelectNextSerialMedia(Adapter);

                    DelayMs = 3000;
                }
                else
                {
                    DelayMs = 5000;
                }
            }
            Adapter->LastReceiveActivity = ReceiveActivity;

            NdisMSetTimer(&Adapter->MediaMonitorTimer, DelayMs);
            break;
        }

        default:
        {
            MediaMonitor143(Adapter);
            break;
        }
    }

    NdisDprReleaseSpinLock(&Adapter->ModeLock);
}
