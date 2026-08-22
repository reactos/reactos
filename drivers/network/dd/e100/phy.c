/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     PHY layer setup and management
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
ULONG
MiiWrite(
    _In_ PE100_ADAPTER Adapter,
    _In_ ULONG RegAddress,
    _In_ ULONG Data)
{
    ULONG i;

    CSR_WRITE_32(Adapter,
                 FXP_CSR_MDICONTROL,
                 (FXP_MDIO_WRITE      << FXP_MDI_OPCODE_SHIFT) |
                 (Adapter->PhyAddress << FXP_MDI_PHY_ADDR_SHIFT) |
                 (RegAddress          << FXP_MDI_REG_ADDR_SHIFT) |
                 Data);

    for (i = FXP_MIIPHY_DELAYMAX; i > 0; i--)
    {
        NdisStallExecution(FXP_MIIPHY_DELAY);

        if (CSR_READ_32(Adapter, FXP_CSR_MDICONTROL) & FXP_MDI_READY)
            break;
    }
    if (i == 0)
    {
        ERR("PHY[%lu] MII reg %lx read timed out\n", Adapter->PhyAddress, RegAddress);
        return MII_FLAG_FAILED;
    }

    return 0;
}

static
ULONG
MiiRead(
    _In_ PE100_ADAPTER Adapter,
    _In_ ULONG RegAddress)
{
    ULONG i, Csr;

    CSR_WRITE_32(Adapter,
                 FXP_CSR_MDICONTROL,
                 (FXP_MDIO_READ       << FXP_MDI_OPCODE_SHIFT) |
                 (Adapter->PhyAddress << FXP_MDI_PHY_ADDR_SHIFT) |
                 (RegAddress          << FXP_MDI_REG_ADDR_SHIFT));

    for (i = FXP_MIIPHY_DELAYMAX; i > 0; i--)
    {
        NdisStallExecution(FXP_MIIPHY_DELAY);

        Csr = CSR_READ_32(Adapter, FXP_CSR_MDICONTROL);
        if (Csr & FXP_MDI_READY)
            break;
    }

    if (i == 0)
    {
        ERR("PHY[%lu] MII reg %lx write timed out\n", Adapter->PhyAddress, RegAddress);
        return MII_FLAG_FAILED;
    }

    Csr &= FXP_MDI_DATA_MASK;

    return Csr;
}

static
UCHAR
EePhy503GetSpeedAndDuplex(
    _In_ PE100_ADAPTER Adapter)
{
    if (!(Adapter->DefaultMedia & E100_MEDIA_AUTO))
        return Adapter->DefaultMedia & ~E100_MEDIA_100T;

    /*
     * The serial PHY supports only 10Mbps half-duplex operation
     * (the user can force 10Mbps full duplex) and it lacks
     * a link activity indicator, so we assume that the link is always up.
     */
    return 0;
}

UCHAR
EePhyGetSpeedAndDuplex(
    _In_ PE100_ADAPTER Adapter)
{
    ULONG MiiStatus, MiiAdvertise, MiiLinkPartnerAbility, AdvLpa;
    UCHAR CurrentMedia;

    if (Adapter->PhyType == E100_PHY_TYPE_503)
        return EePhy503GetSpeedAndDuplex(Adapter);

    /* Optimization for later chips */
    if (Adapter->RevisionID >= FXP_REV_82559_A0)
    {
        if (!(CSR_READ_8(Adapter, FXP_CSR_GENSTATUS) & FXP_GENSTATUS_LINK_UP))
            return E100_MEDIA_NONE;

        if (!(Adapter->DefaultMedia & E100_MEDIA_AUTO))
            return Adapter->DefaultMedia;
    }

    /* The link status is a latched-low bit, read it twice */
    MiiRead(Adapter, MII_STATUS);
    MiiStatus = MiiRead(Adapter, MII_STATUS);
    if (!(MiiStatus & MII_SR_LINK_STATUS))
        return E100_MEDIA_NONE;

    /* If we are forcing speed and duplex */
    if (!(Adapter->DefaultMedia & E100_MEDIA_AUTO))
        return Adapter->DefaultMedia;

    /* Check auto-negotiation is complete */
    if (!(MiiStatus & MII_SR_AUTONEG_COMPLETE))
        return E100_MEDIA_NONE;

    MiiAdvertise = MiiRead(Adapter, MII_AUTONEG_ADVERTISE);
    MiiLinkPartnerAbility = MiiRead(Adapter, MII_AUTONEG_LINK_PARTNER);

    AdvLpa = MiiAdvertise & MiiLinkPartnerAbility;
    TRACE("MII ADV:%04lX LPA:%04lX = %04lX\n", MiiAdvertise, MiiLinkPartnerAbility, AdvLpa);

    if (AdvLpa & MII_LP_100T_FD)
        CurrentMedia = E100_MEDIA_FD | E100_MEDIA_100T;
    else if (AdvLpa & MII_LP_100T4)
        CurrentMedia = E100_MEDIA_100T;
    else if (AdvLpa & MII_LP_100T_HD)
        CurrentMedia = E100_MEDIA_100T;
    else if (AdvLpa & MII_LP_10T_FD)
        CurrentMedia = E100_MEDIA_FD;
    else
        CurrentMedia = 0;

    /* Resolve flow control */
    if (CurrentMedia & E100_MEDIA_FD)
    {
        if (Adapter->DefaultMedia & E100_MEDIA_PAUSE_AUTO)
        {
            if (MiiAdvertise & MiiLinkPartnerAbility & MII_ADV_PAUSE_SYM)
            {
                CurrentMedia |= E100_MEDIA_PAUSE_TX | E100_MEDIA_PAUSE_RX;
            }
            else if (MiiAdvertise & MiiLinkPartnerAbility & MII_ADV_PAUSE_ASYM)
            {
                if (MiiAdvertise & MII_ADV_PAUSE_SYM)
                    CurrentMedia |= E100_MEDIA_PAUSE_RX;
                else if (MiiLinkPartnerAbility & MII_ADV_PAUSE_SYM)
                    CurrentMedia |= E100_MEDIA_PAUSE_TX;
            }
        }
        else
        {
            CurrentMedia |= Adapter->DefaultMedia & (E100_MEDIA_PAUSE_TX | E100_MEDIA_PAUSE_RX);
        }
    }

    return CurrentMedia;
}

CODE_SEG("PAGE")
VOID
EePhyPowerSaveDowngradeLinkSpeed(
    _In_ PE100_ADAPTER Adapter)
{
    ULONG MiiAdvertise, MiiLinkPartnerAbility, AdvLpa, MiiControl;

    PAGED_CODE();

    if (!(Adapter->Flags & E100_FLAG_REDUCE_WOL_LINK_SPEED))
        return;

    if (Adapter->PhyType == E100_PHY_TYPE_503)
        return;

    if (!(Adapter->DefaultMedia & E100_MEDIA_AUTO))
        return;

    MiiAdvertise = MiiRead(Adapter, MII_AUTONEG_ADVERTISE);
    MiiLinkPartnerAbility = MiiRead(Adapter, MII_AUTONEG_LINK_PARTNER);

    /* Switch to 10Mb if possible */
    AdvLpa = MiiAdvertise & MiiLinkPartnerAbility;
    if (!(AdvLpa & (MII_LP_10T_FD | MII_LP_10T_HD)))
        return;

    MiiAdvertise &= ~(MII_ADV_100T_HD |
                      MII_ADV_100T_FD |
                      MII_ADV_100T4);
    MiiWrite(Adapter, MII_AUTONEG_ADVERTISE, MiiAdvertise);

    MiiControl = MiiRead(Adapter, MII_CONTROL);
    MiiControl |= MII_CR_AUTONEG | MII_CR_AUTONEG_RESTART;
    MiiWrite(Adapter, MII_CONTROL, MiiControl);
}

VOID
EePhySetPower(
    _In_ PE100_ADAPTER Adapter,
    _In_ BOOLEAN PowerUp)
{
    ULONG MiiControl;

    if (Adapter->PhyType == E100_PHY_TYPE_503)
        return;

    if (PowerUp)
    {
        MiiControl = MiiRead(Adapter, MII_CONTROL);
        MiiControl &= ~(MII_CR_POWER_DOWN | MII_CR_ISOLATE);
    }
    else
    {
        MiiControl = MII_CR_POWER_DOWN | MII_CR_ISOLATE;
    }

    MiiWrite(Adapter, MII_CONTROL, MiiControl);
}

CODE_SEG("PAGE")
VOID
EePhyInit(
    _In_ PE100_ADAPTER Adapter)
{
    ULONG Value, MiiAdvertise, MiiControl;

    PAGED_CODE();

    if (Adapter->PhyType == E100_PHY_TYPE_503)
        return;

    if (Adapter->PhyType == E100_PHY_TYPE_DP83840)
    {
        /* This enables full-duplex operation on some PRO/100 boards */
        Value = MiiRead(Adapter, PHY_DP_REG_PCS);
        Value |= PHY_DP_PCS_LED4_MODE |
                 PHY_DP_PCS_F_CONNECT |
                 PHY_DP_PCS_BIT_8 |
                 PHY_DP_PCS_BIT_10;
        MiiWrite(Adapter, PHY_DP_REG_PCS, Value);
    }

    MiiAdvertise = MiiRead(Adapter, MII_AUTONEG_ADVERTISE);
    MiiAdvertise |= MII_ADV_CSMA;
    MiiAdvertise &= ~(MII_ADV_10T_HD |
                      MII_ADV_10T_FD |
                      MII_ADV_100T_HD |
                      MII_ADV_100T_FD |
                      MII_ADV_100T4 |
                      MII_ADV_PAUSE_SYM |
                      MII_ADV_PAUSE_ASYM);

    MiiControl = MiiRead(Adapter, MII_CONTROL);

    if (Adapter->DefaultMedia & E100_MEDIA_AUTO)
    {
        MiiControl |= MII_CR_AUTONEG | MII_CR_AUTONEG_RESTART;

        MiiAdvertise |= Adapter->PhyCapabilities;
    }
    else
    {
        MiiControl &= ~(MII_CR_AUTONEG | MII_CR_AUTONEG_RESTART);

        if (Adapter->DefaultMedia & E100_MEDIA_FD)
            MiiControl |= MII_CR_FULL_DUPLEX;
        else
            MiiControl &= ~MII_CR_FULL_DUPLEX;

        if (Adapter->DefaultMedia & E100_MEDIA_100T)
            MiiControl |= MII_CR_SPEED_SELECTION;
        else
            MiiControl &= ~MII_CR_SPEED_SELECTION;
    }

    if (Adapter->DefaultMedia & E100_MEDIA_PAUSE_RX)
        MiiAdvertise |= MII_ADV_PAUSE_SYM | MII_ADV_PAUSE_ASYM;

    if (Adapter->DefaultMedia & E100_MEDIA_PAUSE_TX)
        MiiAdvertise |= MII_ADV_PAUSE_ASYM;

    MiiWrite(Adapter, MII_AUTONEG_ADVERTISE, MiiAdvertise);
    MiiWrite(Adapter, MII_CONTROL, MiiControl);
}

CODE_SEG("PAGE")
NDIS_STATUS
EeFindMiiPhy(
    _In_ PE100_ADAPTER Adapter)
{
    ULONG PhyAddress;

    PAGED_CODE();

    if (Adapter->PhyType == E100_PHY_TYPE_503)
    {
        INFO_VERB("Using non-MII PHY\n");
        return NDIS_STATUS_SUCCESS;
    }

    /* Look for the first connected PHY */
    for (PhyAddress = 1; PhyAddress <= MII_MAX_PHY_ADDRESSES; ++PhyAddress)
    {
        ULONG MiiStatus, PhyId;
#if DBG
        ULONG MiiControl, MiiAdvertise;
#endif

        /* Check the PHY 0 last */
        Adapter->PhyAddress = PhyAddress % MII_MAX_PHY_ADDRESSES;

        /*
         * Read the status register. Some PHYs, such as the ML6692,
         * don't implement the IEEE ID registers.
         */
        if (!MII_SUCCESS(MiiRead(Adapter, MII_STATUS)))
            continue;
        MiiStatus = MiiRead(Adapter, MII_STATUS);
        if (!MII_SUCCESS(MiiStatus))
            continue;
        if (MiiStatus == 0xFFFF || MiiStatus == 0)
            continue;

        Adapter->PhyCapabilities = (MiiStatus & 0xFFFF) >> 6;

        PhyId = MiiRead(Adapter, MII_PHY_ID1);
        PhyId |= MiiRead(Adapter, MII_PHY_ID2) << 16;

#if DBG
        MiiControl = MiiRead(Adapter, MII_CONTROL);
        MiiAdvertise = MiiRead(Adapter, MII_AUTONEG_ADVERTISE);

        INFO_VERB("Found PHY at address %lu: ID %08lX, Ctrl %04X, Status %04X, Adv %04X\n",
                  PhyAddress,
                  PhyId,
                  MiiControl,
                  MiiStatus,
                  MiiAdvertise);
#endif

        if (PHY_IS_DP83840(PhyId))
        {
            Adapter->PhyType = E100_PHY_TYPE_DP83840;
        }

        return NDIS_STATUS_SUCCESS;
    }

    return NDIS_STATUS_FAILURE;
}
