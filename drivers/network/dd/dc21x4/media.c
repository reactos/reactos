/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Media common code
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
MediaIndicateConnect(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN LinkUp)
{
    /* Nothing to do */
    if (Adapter->LinkUp == LinkUp)
        return;

    Adapter->LinkUp = LinkUp;

    INFO_VERB("Link %sconnected, media is %s\n",
              LinkUp ? "" : "dis",
              MediaNumber2Str(Adapter, Adapter->MediaNumber));

    NdisDprReleaseSpinLock(&Adapter->ModeLock);

    NdisMIndicateStatus(Adapter->AdapterHandle,
                        LinkUp ? NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT,
                        NULL,
                        0);
    NdisMIndicateStatusComplete(Adapter->AdapterHandle);

    NdisDprAcquireSpinLock(&Adapter->ModeLock);
}

static
ULONG
MediaMiiNextMedia(
    _In_ PDC21X4_ADAPTER Adapter)
{
    Adapter->ModeFlags &= ~(DC_MODE_TEST_PACKET | DC_MODE_AUI_FAILED | DC_MODE_BNC_FAILED);
    Adapter->LastReceiveActivity = (ULONG)Adapter->Statistics.ReceiveOk;

    /*
     * In MII mode, we don't know exactly which port is active.
     * Switch to the media with a higher priority.
     */
    if (Adapter->MediaBitmap & (1 << MEDIA_HMR))
        return MEDIA_HMR;
    else if (Adapter->MediaBitmap & (1 << MEDIA_AUI))
        return MEDIA_AUI;
    else
        return MEDIA_BNC;
}

static
VOID
MediaMiiSetSpeedAndDuplex(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN FullDuplex,
    _In_ BOOLEAN Speed100)
{
    ULONG OpMode = Adapter->OpMode;

    if (FullDuplex)
        OpMode |= DC_OPMODE_FULL_DUPLEX;
    else
        OpMode &= ~DC_OPMODE_FULL_DUPLEX;

    if (Speed100)
        OpMode &= ~DC_OPMODE_PORT_XMIT_10;
    else
        OpMode |= DC_OPMODE_PORT_XMIT_10;

    /* Nothing to do */
    if (OpMode == Adapter->OpMode)
        return;

    INFO_VERB("Configuring MAC from %u %s-duplex to %u %s-duplex\n",
              Adapter->LinkSpeedMbps,
              (Adapter->OpMode & DC_OPMODE_FULL_DUPLEX) ? "full" : "half",
              Speed100 ? 100 : 10,
              FullDuplex ? "full" : "half");

    Adapter->LinkSpeedMbps = Speed100 ? 100 : 10;

    DcStopTxRxProcess(Adapter);

    Adapter->OpMode = OpMode;
    DC_WRITE(Adapter, DcCsr6_OpMode, Adapter->OpMode);
}

static
VOID
MediaMiiGetSpeedAndDuplex(
    _In_ PDC21X4_ADAPTER Adapter,
    _Out_ PBOOLEAN FullDuplex,
    _Out_ PBOOLEAN Speed100)
{
    ULONG MiiLinkPartnerAbility, AdvLpa;

    MiiRead(Adapter, Adapter->PhyAddress, MII_AUTONEG_LINK_PARTNER, &MiiLinkPartnerAbility);

    TRACE("MII LPA %04lx\n", MiiLinkPartnerAbility);

    AdvLpa = Adapter->MiiMedia.Advertising & MiiLinkPartnerAbility;
    if (AdvLpa & MII_LP_100T_FD)
    {
        *FullDuplex = TRUE;
        *Speed100 = TRUE;
    }
    else if (AdvLpa & MII_LP_100T4)
    {
        *FullDuplex = FALSE;
        *Speed100 = TRUE;
    }
    else if (AdvLpa & MII_LP_100T_HD)
    {
        *FullDuplex = FALSE;
        *Speed100 = TRUE;
    }
    else if (AdvLpa & MII_LP_10T_FD)
    {
        *FullDuplex = TRUE;
        *Speed100 = FALSE;
    }
    else
    {
        *FullDuplex = FALSE;
        *Speed100 = FALSE;
    }
}

BOOLEAN
MediaMiiCheckLink(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG MiiStatus;
    BOOLEAN FullDuplex, Speed100;

    /* The link status is a latched-low bit, read it twice */
    if (!MiiRead(Adapter, Adapter->PhyAddress, MII_STATUS, &MiiStatus))
    {
        goto NoLink;
    }
    if (!(MiiStatus & MII_SR_LINK_STATUS))
    {
        MiiRead(Adapter, Adapter->PhyAddress, MII_STATUS, &MiiStatus);
    }
    TRACE("MII Status %04lx\n", MiiStatus);

    /* Check the link status */
    if (!(MiiStatus & MII_SR_LINK_STATUS))
    {
NoLink:
        /* No link detected, check the other port */
        if (Adapter->MediaBitmap & ((1 << MEDIA_HMR) | (1 << MEDIA_AUI) | (1 << MEDIA_BNC)))
        {
            if ((Adapter->Features & DC_MII_AUTOSENSE) && !MEDIA_IS_FIXED(Adapter))
            {
                Adapter->MediaNumber = MediaMiiNextMedia(Adapter);
                MediaSiaSelect(Adapter);
            }
        }

        return FALSE;
    }

    /* If we are forcing speed and duplex */
    if (MEDIA_IS_FIXED(Adapter))
    {
        FullDuplex = !!(Adapter->MiiControl & MII_CR_FULL_DUPLEX);
        Speed100 = !!(Adapter->MiiControl & MII_CR_SPEED_SELECTION);
    }
    else
    {
        /* Check auto-negotiation is complete */
        if (!(MiiStatus & MII_SR_AUTONEG_COMPLETE))
            return FALSE;

        MediaMiiGetSpeedAndDuplex(Adapter, &FullDuplex, &Speed100);
    }

    /* Set the link speed and duplex */
    MediaMiiSetSpeedAndDuplex(Adapter, FullDuplex, Speed100);

    return TRUE;
}

VOID
MediaMiiSelect(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG MiiControl, MiiAdvertise;

    MiiRead(Adapter, Adapter->PhyAddress, MII_CONTROL, &MiiControl);
    MiiControl &= ~(MII_CR_POWER_DOWN | MII_CR_ISOLATE | MII_CR_RESET | MII_CR_SPEED_SELECTION |
                    MII_CR_FULL_DUPLEX | MII_CR_AUTONEG | MII_CR_AUTONEG_RESTART);
    MiiWrite(Adapter, Adapter->PhyAddress, MII_CONTROL, MiiControl);

    MiiControl |= Adapter->MiiControl;
    MiiAdvertise = Adapter->MiiAdvertising;

    MiiWrite(Adapter, Adapter->PhyAddress, MII_AUTONEG_ADVERTISE, MiiAdvertise | MII_ADV_CSMA);
    MiiWrite(Adapter, Adapter->PhyAddress, MII_CONTROL, MiiControl);
}

VOID
MediaSelectMiiPort(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN ResetPhy)
{
    ULONG OpMode, i;

    if (Adapter->ChipType != DC21140)
    {
        DcWriteSia(Adapter, 0, 0, 0);
    }

    OpMode = Adapter->OpMode;
    OpMode &= ~DC_OPMODE_MEDIA_MASK;
    OpMode |= DC_OPMODE_PORT_SELECT | DC_OPMODE_PORT_HEARTBEAT_DISABLE;
    Adapter->OpMode = OpMode;

    DC_WRITE(Adapter, DcCsr6_OpMode, OpMode);

    NdisStallExecution(10);

    if (ResetPhy)
    {
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

        /* Execute the GPIO reset sequence */
        if (Adapter->MiiMedia.ResetStreamLength)
        {
            /* Set the GPIO direction */
            DcWriteGpio(Adapter, Adapter->MiiMedia.SetupStream[0]);

            for (i = 0; i < Adapter->MiiMedia.ResetStreamLength; ++i)
            {
                NdisMSleep(100);
                DcWriteGpio(Adapter, Adapter->MiiMedia.ResetStream[i]);
            }

            /* Give the PHY some time to reset */
            NdisMSleep(5000);
        }
    }

    /* Set the GPIO direction */
    DcWriteGpio(Adapter, Adapter->MiiMedia.SetupStream[0]);

    /* Execute the GPIO setup sequence */
    for (i = 1; i < Adapter->MiiMedia.SetupStreamLength; ++i)
    {
        NdisStallExecution(10);
        DcWriteGpio(Adapter, Adapter->MiiMedia.SetupStream[i]);
    }
}

VOID
MediaSiaSelect(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG OpMode;
    PDC_MEDIA Media;

    INFO_VERB("Selected media %s\n",
              MediaNumber2Str(Adapter, Adapter->MediaNumber));

    Media = &Adapter->Media[Adapter->MediaNumber];

    DcStopTxRxProcess(Adapter);

    if (Adapter->ChipType != DC21041)
    {
        /* Make sure the reset pulse is wide enough */
        NdisStallExecution(100);
        DcWriteGpio(Adapter, Media->GpioCtrl);
        NdisStallExecution(100);
        DcWriteGpio(Adapter, Media->GpioData);
    }

    DcWriteSia(Adapter, Media->Csr13, Media->Csr14, Media->Csr15);

    NdisStallExecution(10);

    OpMode = Adapter->OpMode;
    OpMode &= ~DC_OPMODE_MEDIA_MASK;
    OpMode |= Media->OpMode;
    Adapter->OpMode = OpMode;

    DC_WRITE(Adapter, DcCsr6_OpMode, OpMode);
}

VOID
MediaGprSelect(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG OpMode;
    PDC_MEDIA Media;

    INFO("Selected media %s\n", MediaNumber2Str(Adapter, Adapter->MediaNumber));

    Media = &Adapter->Media[Adapter->MediaNumber];

    DC_WRITE(Adapter, DcCsr12_Gpio, Media->GpioData);

    OpMode = Adapter->OpMode;
    OpMode &= ~DC_OPMODE_MEDIA_MASK;
    OpMode |= Media->OpMode;
    Adapter->OpMode = OpMode;

    DC_WRITE(Adapter, DcCsr6_OpMode, OpMode);
}

CODE_SEG("PAGE")
VOID
MediaInitDefaultMedia(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG MediaNumber)
{
    ULONG Csr14, MiiAdvertising, MiiControl, i;
    BOOLEAN UseMii;

    PAGED_CODE();

    /* Media auto-detection */
    if (MediaNumber == MEDIA_AUTO)
    {
        Adapter->Flags |= DC_AUTOSENSE;

        /* Initial value for all boards */
        Adapter->DefaultMedia = MEDIA_10T;

        Adapter->MiiAdvertising = Adapter->MiiMedia.Advertising;
        Adapter->MiiControl = MII_CR_AUTONEG | MII_CR_AUTONEG_RESTART;

        switch (Adapter->ChipType)
        {
            case DC21041:
            {
                /* Errata: don't enable auto-negotiation */
                if (Adapter->RevisionId < 0x20)
                    break;

                /* Advertise 10T HD and 10T FD. The chip chooses the 10T FD mode automatically */
                Adapter->Media[MEDIA_10T].Csr14 |= DC_SIA_TXRX_AUTONEG | DC_SIA_TXRX_ADV_10T_HD;
                Adapter->Media[MEDIA_10T].OpMode |= DC_OPMODE_FULL_DUPLEX;
                break;
            }

            case DC21140:
            {
                /* Pick the default media */
                if (Adapter->Features & DC_HAS_MII)
                {
                    Adapter->DefaultMedia = MEDIA_MII;
                    break;
                }

                /* The final entry in the media list should be checked first */
                _BitScanReverse(&Adapter->DefaultMedia, Adapter->MediaBitmap);

                /*
                 * Select the first half-duplex media.
                 * If you want to be able to use 21140 boards without MII in full-duplex mode,
                 * you have to manually select the media.
                 */
                for (i = Adapter->DefaultMedia; i > 0; --i)
                {
                    if ((Adapter->MediaBitmap & (1 << i)) && !MEDIA_IS_FD(i))
                        break;
                }
                Adapter->DefaultMedia = i;
                break;
            }

            case DC21143:
            case DC21145:
            {
                /* Pick the default media */
                if (Adapter->Features & DC_HAS_MII)
                {
                    Adapter->DefaultMedia = MEDIA_MII;
                }
                else if (Adapter->MediaBitmap & (1 << MEDIA_10T))
                {
                    /* Start at 10mbps to do internal auto-negotiation */
                    Adapter->DefaultMedia = MEDIA_10T;
                }
                else
                {
                    /* The final entry in the media list should be checked first */
                    _BitScanReverse(&Adapter->DefaultMedia, Adapter->MediaBitmap);
                }

                /* Enable the PCS function to do 100mbps parallel detection */
                if (Adapter->SymAdvertising & MII_ADV_100)
                {
                    Adapter->Media[MEDIA_10T].OpMode |= DC_OPMODE_PORT_PCS;
                    Adapter->Media[MEDIA_10T_FD].OpMode |= DC_OPMODE_PORT_PCS;
                    Adapter->Media[MEDIA_AUI].OpMode |= DC_OPMODE_PORT_PCS;
                    Adapter->Media[MEDIA_BNC].OpMode |= DC_OPMODE_PORT_PCS;
                    Adapter->Media[MEDIA_HMR].OpMode |= DC_OPMODE_PORT_PCS;
                }

                Csr14 = DC_SIA_TXRX_AUTONEG;

                if (Adapter->SymAdvertising & MII_ADV_10T_HD)
                    Csr14 |= DC_SIA_TXRX_ADV_10T_HD;

                /* When NWay is turned on, the FDX bit advertises 10T FD */
                if (Adapter->SymAdvertising & MII_ADV_10T_FD)
                    Adapter->Media[MEDIA_10T].OpMode |= DC_OPMODE_FULL_DUPLEX;

                if (Adapter->SymAdvertising & MII_ADV_100T_HD)
                    Csr14 |= DC_SIA_TXRX_ADV_100TX_HD;

                if (Adapter->SymAdvertising & MII_ADV_100T_FD)
                    Csr14 |= DC_SIA_TXRX_ADV_100TX_FD;

                if (Adapter->SymAdvertising & MII_ADV_100T4)
                    Csr14 |= DC_SIA_TXRX_ADV_100T4;

                /* Advertise the PHY capability */
                Adapter->Media[MEDIA_10T].Csr14 |= Csr14;

                /* This media may use GPIO data different from the 10T HD */
                Adapter->Media[MEDIA_10T_FD].Csr14 |= Csr14;
                break;
            }

            default:
                break;
        }
    }
    else /* Forced speed and duplex */
    {
        UseMii = FALSE;

        if (Adapter->Features & DC_HAS_MII)
        {
            if (!MEDIA_MII_OVERRIDE(MediaNumber))
            {
                UseMii = TRUE;
            }
        }

        if (!UseMii)
        {
            Adapter->DefaultMedia = MediaNumber;

            if (MEDIA_IS_10T(MediaNumber))
            {
                Adapter->InterruptMask &= ~DC_IRQ_LINK_CHANGED;
                Adapter->LinkStateChangeMask &= ~DC_IRQ_LINK_CHANGED;
            }

            if (MEDIA_IS_100(MediaNumber))
            {
                Adapter->InterruptMask &= ~(DC_IRQ_LINK_FAIL | DC_IRQ_LINK_PASS);
                Adapter->LinkStateChangeMask &= ~(DC_IRQ_LINK_FAIL | DC_IRQ_LINK_PASS);
            }
        }
        else
        {
            Adapter->DefaultMedia = MEDIA_MII;

            switch (MediaNumber)
            {
                case MEDIA_10T:
                    MiiAdvertising = MII_ADV_10T_HD;
                    break;
                case MEDIA_10T_FD:
                    MiiAdvertising = MII_ADV_10T_FD;
                    MiiControl = MII_CR_FULL_DUPLEX;
                    break;
                case MEDIA_100TX_HD:
                    MiiAdvertising = MII_ADV_100T_HD;
                    MiiControl = MII_CR_SPEED_SELECTION;
                    break;
                case MEDIA_100TX_FD:
                    MiiAdvertising = MII_ADV_100T_FD;
                    MiiControl = MII_CR_FULL_DUPLEX | MII_CR_SPEED_SELECTION;
                    break;
                case MEDIA_100T4:
                    MiiAdvertising = MII_ADV_100T4 | MII_CR_SPEED_SELECTION;
                    break;

                default:
                    MiiAdvertising = 0;
                    MiiControl = 0;
                    break;
            }

            if (MiiControl & MII_CR_SPEED_SELECTION)
                Adapter->LinkSpeedMbps = 100;
            else
                Adapter->LinkSpeedMbps = 10;

            Adapter->MiiAdvertising = MiiAdvertising;
            Adapter->MiiControl = MiiControl;
        }
    }

    INFO("Default media is %s\n", MediaNumber2Str(Adapter, Adapter->DefaultMedia));
}

static
CODE_SEG("PAGE")
VOID
MediaInitOpMode2114x(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PAGED_CODE();

    Adapter->Media[MEDIA_10T     ].OpMode = 0;
    Adapter->Media[MEDIA_BNC     ].OpMode = 0;
    Adapter->Media[MEDIA_AUI     ].OpMode = 0;
    Adapter->Media[MEDIA_100TX_HD].OpMode = DC_OPMODE_PORT_SELECT |
                                            DC_OPMODE_PORT_HEARTBEAT_DISABLE;
    Adapter->Media[MEDIA_10T_FD  ].OpMode = DC_OPMODE_FULL_DUPLEX;
    Adapter->Media[MEDIA_100TX_FD].OpMode = DC_OPMODE_PORT_SELECT | DC_OPMODE_FULL_DUPLEX |
                                            DC_OPMODE_PORT_HEARTBEAT_DISABLE;
    Adapter->Media[MEDIA_100T4   ].OpMode = DC_OPMODE_PORT_SELECT |
                                            DC_OPMODE_PORT_HEARTBEAT_DISABLE;
    Adapter->Media[MEDIA_100FX_HD].OpMode = DC_OPMODE_PORT_SELECT |
                                            DC_OPMODE_PORT_HEARTBEAT_DISABLE |
                                            DC_OPMODE_PORT_PCS;
    Adapter->Media[MEDIA_100FX_FD].OpMode = DC_OPMODE_PORT_SELECT | DC_OPMODE_FULL_DUPLEX |
                                            DC_OPMODE_PORT_HEARTBEAT_DISABLE |
                                            DC_OPMODE_PORT_PCS;
    Adapter->Media[MEDIA_HMR     ].OpMode = DC_OPMODE_PORT_HEARTBEAT_DISABLE;
}

CODE_SEG("PAGE")
VOID
MediaInitMediaList(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PAGED_CODE();

    /*
     * Set the default internal values for the SIA/SYM operating modes.
     * The SROM parsing code may later overwrite them.
     */
    switch (Adapter->ChipType)
    {
        case DC21040:
        {
            Adapter->Media[MEDIA_10T].Csr13 = 0x8F01;
            Adapter->Media[MEDIA_10T].Csr14 = 0xFFFF;
            Adapter->Media[MEDIA_10T].Csr15 = 0x0000;

            Adapter->Media[MEDIA_BNC].Csr13 = 0x8F09;
            Adapter->Media[MEDIA_BNC].Csr14 = 0x0705;
            Adapter->Media[MEDIA_BNC].Csr15 = 0x0006;

            Adapter->Media[MEDIA_10T_FD].Csr13 = 0x8F01;
            Adapter->Media[MEDIA_10T_FD].Csr14 = 0xFFFD;
            Adapter->Media[MEDIA_10T_FD].Csr15 = 0x0000;
            Adapter->Media[MEDIA_10T_FD].OpMode = DC_OPMODE_FULL_DUPLEX;
            break;
        }

        case DC21041:
        {
            Adapter->Media[MEDIA_10T].Csr13 = 0xEF01;
            Adapter->Media[MEDIA_10T].Csr14 = 0xFF3F;
            Adapter->Media[MEDIA_10T].Csr15 = 0x0008;

            Adapter->Media[MEDIA_BNC].Csr13 = 0xEF09;
            Adapter->Media[MEDIA_BNC].Csr14 = 0xF7FD;
            Adapter->Media[MEDIA_BNC].Csr15 = 0x0006;

            Adapter->Media[MEDIA_AUI].Csr13 = 0xEF09;
            Adapter->Media[MEDIA_AUI].Csr14 = 0xF7FD;
            Adapter->Media[MEDIA_AUI].Csr15 = 0x000E;

            Adapter->Media[MEDIA_10T_HD].Csr13 = 0xEF01;
            Adapter->Media[MEDIA_10T_HD].Csr14 = 0x7F3F;
            Adapter->Media[MEDIA_10T_HD].Csr15 = 0x0008;

            Adapter->Media[MEDIA_10T_FD].Csr13 = 0xEF01;
            Adapter->Media[MEDIA_10T_FD].Csr14 = 0x7F3D;
            Adapter->Media[MEDIA_10T_FD].Csr15 = 0x0008;
            Adapter->Media[MEDIA_10T_FD].OpMode = DC_OPMODE_FULL_DUPLEX;
            break;
        }

        case DC21140:
        {
            MediaInitOpMode2114x(Adapter);
            break;
        }

        case DC21143:
        case DC21145:
        {
            Adapter->Media[MEDIA_10T].Csr13 = 0x0001;
            Adapter->Media[MEDIA_10T].Csr14 = 0x7F3F;
            Adapter->Media[MEDIA_10T].Csr15 = 0x0008;

            Adapter->Media[MEDIA_BNC].Csr13 = 0x0009;
            Adapter->Media[MEDIA_BNC].Csr14 = 0x0705;
            Adapter->Media[MEDIA_BNC].Csr15 = 0x0006;

            Adapter->Media[MEDIA_AUI].Csr13 = 0x0009;
            Adapter->Media[MEDIA_AUI].Csr14 = 0x0705;
            Adapter->Media[MEDIA_AUI].Csr15 = 0x000E;

            Adapter->Media[MEDIA_10T_FD].Csr13 = 0x0001;
            Adapter->Media[MEDIA_10T_FD].Csr14 = 0x7F3D;
            Adapter->Media[MEDIA_10T_FD].Csr15 = 0x0008;

            Adapter->Media[MEDIA_HMR].Csr13 = 0x0009;
            Adapter->Media[MEDIA_HMR].Csr14 = 0x0505;
            Adapter->Media[MEDIA_HMR].Csr15 = 0x0010;

            MediaInitOpMode2114x(Adapter);
            break;
        }

        default:
            break;
    }
}
