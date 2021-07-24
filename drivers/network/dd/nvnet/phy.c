/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PHY layer setup and management
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

BOOLEAN
MiiWrite(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG PhyAddress,
    _In_ ULONG RegAddress,
    _In_ ULONG Data)
{
    ULONG i;

    NV_WRITE(Adapter, NvRegMIIStatus, NVREG_MIISTAT_MASK_RW);

    if (NV_READ(Adapter, NvRegMIIControl) & NVREG_MIICTL_INUSE)
    {
        NV_WRITE(Adapter, NvRegMIIControl, NVREG_MIICTL_INUSE);
        NdisStallExecution(NV_MIIBUSY_DELAY);
    }

    NV_WRITE(Adapter, NvRegMIIData, Data);
    NV_WRITE(Adapter, NvRegMIIControl,
             NVREG_MIICTL_WRITE | (PhyAddress << NVREG_MIICTL_ADDRSHIFT) | RegAddress);

    for (i = NV_MIIPHY_DELAYMAX; i > 0; --i)
    {
        NdisStallExecution(NV_MIIPHY_DELAY);

        if (!(NV_READ(Adapter, NvRegMIIControl) & NVREG_MIICTL_INUSE))
            break;
    }
    if (i == 0)
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
MiiRead(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG PhyAddress,
    _In_ ULONG RegAddress,
    _Out_ PULONG Data)
{
    ULONG i;

    NV_WRITE(Adapter, NvRegMIIStatus, NVREG_MIISTAT_MASK_RW);

    if (NV_READ(Adapter, NvRegMIIControl) & NVREG_MIICTL_INUSE)
    {
        NV_WRITE(Adapter, NvRegMIIControl, NVREG_MIICTL_INUSE);
        NdisStallExecution(NV_MIIBUSY_DELAY);
    }

    NV_WRITE(Adapter, NvRegMIIControl, (PhyAddress << NVREG_MIICTL_ADDRSHIFT) | RegAddress);

    for (i = NV_MIIPHY_DELAYMAX; i > 0; --i)
    {
        NdisStallExecution(NV_MIIPHY_DELAY);

        if (!(NV_READ(Adapter, NvRegMIIControl) & NVREG_MIICTL_INUSE))
            break;
    }
    if (i == 0)
    {
        *Data = 0;
        return FALSE;
    }

    if (NV_READ(Adapter, NvRegMIIStatus) & NVREG_MIISTAT_ERROR)
    {
        *Data = 0;
        return FALSE;
    }

    *Data = NV_READ(Adapter, NvRegMIIData);
    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
PhyInitRealtek8211b(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG i;
    const struct
    {
        ULONG Register;
        ULONG Data;
    } Sequence[] =
    {
        { PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1 },
        { PHY_REALTEK_INIT_REG2, PHY_REALTEK_INIT2 },
        { PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT3 },
        { PHY_REALTEK_INIT_REG3, PHY_REALTEK_INIT4 },
        { PHY_REALTEK_INIT_REG4, PHY_REALTEK_INIT5 },
        { PHY_REALTEK_INIT_REG5, PHY_REALTEK_INIT6 },
        { PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1 }
    };

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    for (i = 0; i < RTL_NUMBER_OF(Sequence); ++i)
    {
        if (!MiiWrite(Adapter, Adapter->PhyAddress, Sequence[i].Register, Sequence[i].Data))
            return FALSE;
    }

    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
PhyInitRealtek8211c(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG PowerState, MiiRegister;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    PowerState = NV_READ(Adapter, NvRegPowerState2);

    NV_WRITE(Adapter, NvRegPowerState2, PowerState | NVREG_POWERSTATE2_PHY_RESET);
    NdisMSleep(25000);

    NV_WRITE(Adapter, NvRegPowerState2, PowerState);
    NdisMSleep(25000);

    MiiRead(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG6, &MiiRegister);
    MiiRegister |= PHY_REALTEK_INIT9;
    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG6, MiiRegister))
        return FALSE;

    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT10))
        return FALSE;

    MiiRead(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG7, &MiiRegister);
    if (!(MiiRegister & PHY_REALTEK_INIT11))
    {
        MiiRegister |= PHY_REALTEK_INIT11;
        if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG7, MiiRegister))
            return FALSE;
    }

    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1))
        return FALSE;

    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
PhyInitRealtek8201(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ BOOLEAN DisableCrossoverDetection)
{
    ULONG MiiRegister;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    if (Adapter->Features & DEV_NEED_PHY_INIT_FIX)
    {
        MiiRead(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG6, &MiiRegister);
        MiiRegister |= PHY_REALTEK_INIT7;
        if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG6, MiiRegister))
            return FALSE;
    }

    if (DisableCrossoverDetection)
    {
        if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT3))
            return FALSE;

        MiiRead(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG2, &MiiRegister);
        MiiRegister &= ~PHY_REALTEK_INIT_MSK1;
        MiiRegister |= PHY_REALTEK_INIT3;
        if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG2, MiiRegister))
            return FALSE;

        if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1))
            return FALSE;
    }

    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
PhyInitCicadaSemiconductor(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG PhyInterface)
{
    ULONG MiiRegister;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    if (PhyInterface & PHY_RGMII)
    {
        MiiRead(Adapter, Adapter->PhyAddress, PHY_CICADA_INIT_REG2, &MiiRegister);
        MiiRegister &= ~(PHY_CICADA_INIT1 | PHY_CICADA_INIT2);
        MiiRegister |= (PHY_CICADA_INIT3 | PHY_CICADA_INIT4);
        if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_CICADA_INIT_REG2, MiiRegister))
            return FALSE;

        MiiRead(Adapter, Adapter->PhyAddress, PHY_CICADA_INIT_REG3, &MiiRegister);
        MiiRegister |= PHY_CICADA_INIT5;
        if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_CICADA_INIT_REG3, MiiRegister))
            return FALSE;
    }

    MiiRead(Adapter, Adapter->PhyAddress, PHY_CICADA_INIT_REG1, &MiiRegister);
    MiiRegister |= PHY_CICADA_INIT6;
    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_CICADA_INIT_REG1, MiiRegister))
        return FALSE;

    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
PhyInitVitesseSemiconductor(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG MiiRegister;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG1, PHY_VITESSE_INIT1))
        return FALSE;

    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT2))
        return FALSE;

    MiiRead(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG4, &MiiRegister);
    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG4, MiiRegister))
        return FALSE;

    MiiRead(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG3, &MiiRegister);
    MiiRegister &= ~PHY_VITESSE_INIT_MSK1;
    MiiRegister |= PHY_VITESSE_INIT3;
    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG3, MiiRegister))
        return FALSE;

    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT4))
        return FALSE;

    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT5))
        return FALSE;

    MiiRead(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG4, &MiiRegister);
    MiiRegister &= ~PHY_VITESSE_INIT_MSK1;
    MiiRegister |= PHY_VITESSE_INIT3;
    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG4, MiiRegister))
        return FALSE;

    MiiRead(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG3, &MiiRegister);
    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG3, MiiRegister))
        return FALSE;

    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT6))
        return FALSE;

    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT7))
        return FALSE;

    MiiRead(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG4, &MiiRegister);
    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG4, MiiRegister))
        return FALSE;

    MiiRead(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG3, &MiiRegister);
    MiiRegister &= ~PHY_VITESSE_INIT_MSK2;
    MiiRegister |= PHY_VITESSE_INIT8;
    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG3, MiiRegister))
        return FALSE;

    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT9))
        return FALSE;

    if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_VITESSE_INIT_REG1, PHY_VITESSE_INIT10))
        return FALSE;

    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
PhyReset(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG ControlSetup)
{
    ULONG Tries = 0, MiiControl = MII_CR_RESET | ControlSetup;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    if (!MiiWrite(Adapter, Adapter->PhyAddress, MII_CONTROL, MiiControl))
        return FALSE;

    NdisMSleep(500000);

    do
    {
        NdisMSleep(10000);

        MiiRead(Adapter, Adapter->PhyAddress, MII_CONTROL, &MiiControl);

        if (Tries++ > 100)
            return FALSE;
    }
    while (MiiControl & MII_CR_RESET);

    return TRUE;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
PhyInit(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG PhyInterface, MiiRegister, MiiStatus, MiiControl;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    /* PHY errata for E3016 PHY */
    if (Adapter->PhyModel == PHY_MODEL_MARVELL_E3016)
    {
        MiiRead(Adapter, Adapter->PhyAddress, PHY_MARVELL_INIT_REG1, &MiiRegister);
        MiiRegister &= ~PHY_MARVELL_E3016_INITMASK;
        if (!MiiWrite(Adapter, Adapter->PhyAddress, PHY_MARVELL_INIT_REG1, MiiRegister))
        {
            NDIS_DbgPrint(MAX_TRACE, ("PHY init failed\n"));
            return NDIS_STATUS_FAILURE;
        }
    }

    if (Adapter->PhyOui == PHY_OUI_REALTEK)
    {
        if (Adapter->PhyModel == PHY_MODEL_REALTEK_8211 &&
            Adapter->PhyRevision == PHY_REV_REALTEK_8211B)
        {
            if (!PhyInitRealtek8211b(Adapter))
            {
                NDIS_DbgPrint(MAX_TRACE, ("PHY init failed\n"));
                return NDIS_STATUS_FAILURE;
            }
        }
        else if (Adapter->PhyModel == PHY_MODEL_REALTEK_8211 &&
                 Adapter->PhyRevision == PHY_REV_REALTEK_8211C)
        {
            if (!PhyInitRealtek8211c(Adapter))
            {
                NDIS_DbgPrint(MAX_TRACE, ("PHY init failed\n"));
                return NDIS_STATUS_FAILURE;
            }
        }
        else if (Adapter->PhyModel == PHY_MODEL_REALTEK_8201)
        {
            if (!PhyInitRealtek8201(Adapter, FALSE))
            {
                NDIS_DbgPrint(MAX_TRACE, ("PHY init failed\n"));
                return NDIS_STATUS_FAILURE;
            }
        }
    }

    /* Set advertise register */
    MiiRead(Adapter, Adapter->PhyAddress, MII_AUTONEG_ADVERTISE, &MiiRegister);
    if (Adapter->Flags & NV_FORCE_SPEED_AND_DUPLEX)
    {
        MiiRegister &= ~(MII_ADV_10T_HD | MII_ADV_10T_FD | MII_ADV_100T_HD | MII_ADV_100T_FD |
                         MII_ADV_100T4 | MII_ADV_PAUSE_SYM | MII_ADV_PAUSE_ASYM);

        if (Adapter->Flags & NV_USER_SPEED_100)
        {
            if (Adapter->Flags & NV_FORCE_FULL_DUPLEX)
                MiiRegister |= MII_ADV_100T_FD;
            else
                MiiRegister |= MII_ADV_100T_HD;
        }
        else
        {
            if (Adapter->Flags & NV_FORCE_FULL_DUPLEX)
                MiiRegister |= MII_ADV_10T_FD;
            else
                MiiRegister |= MII_ADV_10T_HD;
        }

        Adapter->PauseFlags &= ~(NV_PAUSEFRAME_AUTONEG | NV_PAUSEFRAME_RX_ENABLE |
                                 NV_PAUSEFRAME_TX_ENABLE);
        if (Adapter->PauseFlags & NV_PAUSEFRAME_RX_REQ)
        {
            /* For RX we set both advertisements but disable TX pause */
            MiiRegister |= MII_ADV_PAUSE_SYM | MII_ADV_PAUSE_ASYM;
            Adapter->PauseFlags |= NV_PAUSEFRAME_RX_ENABLE;
        }
        if (Adapter->PauseFlags & NV_PAUSEFRAME_TX_REQ)
        {
            MiiRegister |= MII_ADV_PAUSE_ASYM;
            Adapter->PauseFlags |= NV_PAUSEFRAME_TX_ENABLE;
        }
    }
    else
    {
        MiiRegister |= MII_ADV_10T_HD | MII_ADV_10T_FD | MII_ADV_100T_HD |
                       MII_ADV_100T_FD | MII_ADV_PAUSE_SYM | MII_ADV_PAUSE_ASYM;
    }
    if (!MiiWrite(Adapter, Adapter->PhyAddress, MII_AUTONEG_ADVERTISE, MiiRegister))
    {
        NDIS_DbgPrint(MAX_TRACE, ("PHY init failed!\n"));
        return NDIS_STATUS_FAILURE;
    }

    /* Get PHY interface type */
    PhyInterface = NV_READ(Adapter, NvRegPhyInterface);

    /* See if gigabit PHY */
    MiiRead(Adapter, Adapter->PhyAddress, MII_STATUS, &MiiStatus);
    if (MiiStatus & PHY_GIGABIT)
    {
        ULONG MiiControl1000;

        Adapter->Flags |= NV_GIGABIT_PHY;

        MiiRead(Adapter, Adapter->PhyAddress, MII_MASTER_SLAVE_CONTROL, &MiiControl1000);
        MiiControl1000 &= ~MII_MS_CR_1000T_HD;
        if ((PhyInterface & PHY_RGMII) && !(Adapter->Flags & NV_FORCE_SPEED_AND_DUPLEX))
            MiiControl1000 |= MII_MS_CR_1000T_FD;
        else
            MiiControl1000 &= ~MII_MS_CR_1000T_FD;
        if (!MiiWrite(Adapter, Adapter->PhyAddress, MII_MASTER_SLAVE_CONTROL, MiiControl1000))
        {
            NDIS_DbgPrint(MAX_TRACE, ("PHY init failed\n"));
            return NDIS_STATUS_FAILURE;
        }
    }
    else
    {
        Adapter->Flags &= ~NV_GIGABIT_PHY;
    }

    MiiRead(Adapter, Adapter->PhyAddress, MII_CONTROL, &MiiControl);
    MiiControl |= MII_CR_AUTONEG;
    if (Adapter->PhyOui == PHY_OUI_REALTEK &&
        Adapter->PhyModel == PHY_MODEL_REALTEK_8211 &&
        Adapter->PhyRevision == PHY_REV_REALTEK_8211C)
    {
        /* Start auto-negation since we already performed HW reset above */
        MiiControl |= MII_CR_AUTONEG_RESTART;
        if (!MiiWrite(Adapter, Adapter->PhyAddress, MII_CONTROL, MiiControl))
        {
            NDIS_DbgPrint(MAX_TRACE, ("PHY init failed\n"));
            return NDIS_STATUS_FAILURE;
        }
    }
    else
    {
        /* Reset the PHY (certain PHYs need BMCR to be setup with reset) */
        if (!PhyReset(Adapter, MiiControl))
        {
            NDIS_DbgPrint(MAX_TRACE, ("PHY reset failed\n"));
            return NDIS_STATUS_FAILURE;
        }
    }

    /* PHY vendor specific configuration */
    if (Adapter->PhyOui == PHY_OUI_CICADA)
    {
        if (!PhyInitCicadaSemiconductor(Adapter, PhyInterface))
        {
            NDIS_DbgPrint(MAX_TRACE, ("PHY init failed\n"));
            return NDIS_STATUS_FAILURE;
        }
    }
    else if (Adapter->PhyOui == PHY_OUI_VITESSE)
    {
        if (!PhyInitVitesseSemiconductor(Adapter))
        {
            NDIS_DbgPrint(MAX_TRACE, ("PHY init failed\n"));
            return NDIS_STATUS_FAILURE;
        }
    }
    else if (Adapter->PhyOui == PHY_OUI_REALTEK)
    {
        if (Adapter->PhyModel == PHY_MODEL_REALTEK_8211 &&
            Adapter->PhyRevision == PHY_REV_REALTEK_8211B)
        {
            /* Reset could have cleared these out, set them back */
            if (!PhyInitRealtek8211b(Adapter))
            {
                NDIS_DbgPrint(MAX_TRACE, ("PHY init failed\n"));
                return NDIS_STATUS_FAILURE;
            }
        }
        else if (Adapter->PhyModel == PHY_MODEL_REALTEK_8201)
        {
            if (!PhyInitRealtek8201(Adapter, TRUE))
            {
                NDIS_DbgPrint(MAX_TRACE, ("PHY init failed\n"));
                return NDIS_STATUS_FAILURE;
            }
        }
    }

    /* Some PHYs clear out pause advertisement on reset, set it back */
    MiiWrite(Adapter, Adapter->PhyAddress, MII_AUTONEG_ADVERTISE, MiiRegister);

    /* Restart auto-negotiation */
    MiiRead(Adapter, Adapter->PhyAddress, MII_CONTROL, &MiiControl);
    MiiControl |= (MII_CR_AUTONEG_RESTART | MII_CR_AUTONEG);
    if (!MiiWrite(Adapter, Adapter->PhyAddress, MII_CONTROL, MiiControl))
        return NDIS_STATUS_FAILURE;

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
BOOLEAN
FindPhyDevice(
    _Inout_ PNVNET_ADAPTER Adapter)
{
    ULONG Phy;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    for (Phy = 1; Phy <= 32; ++Phy)
    {
        ULONG PhyAddress = Phy & 0x1F; /* Check the PHY 0 last */
        ULONG PhyIdLow, PhyIdHigh;

        if (!MiiRead(Adapter, PhyAddress, MII_PHY_ID1, &PhyIdLow))
            continue;
        if (PhyIdLow == 0xFFFF)
            continue;

        if (!MiiRead(Adapter, PhyAddress, MII_PHY_ID2, &PhyIdHigh))
            continue;
        if (PhyIdHigh == 0xFFFF)
            continue;

        Adapter->PhyAddress = PhyAddress;
        Adapter->PhyModel = PhyIdHigh & PHYID2_MODEL_MASK;
        Adapter->PhyOui = ((PhyIdLow & PHYID1_OUI_MASK) << PHYID1_OUI_SHFT) |
                          ((PhyIdHigh & PHYID2_OUI_MASK) >> PHYID2_OUI_SHFT);

        /* Realtek hardcoded PhyIdLow to all zero's on certain PHYs */
        if (Adapter->PhyOui == PHY_OUI_REALTEK2)
            Adapter->PhyOui = PHY_OUI_REALTEK;

        /* Setup PHY revision for Realtek */
        if (Adapter->PhyOui == PHY_OUI_REALTEK && Adapter->PhyModel == PHY_MODEL_REALTEK_8211)
        {
            ULONG PhyRevision;

            MiiRead(Adapter, PhyAddress, PHY_REALTEK_REVISION, &PhyRevision);
            Adapter->PhyRevision = PhyRevision & PHY_REV_MASK;
        }

        NDIS_DbgPrint(MIN_TRACE, ("Found PHY %X %X %X\n",
                                  Adapter->PhyAddress,
                                  Adapter->PhyModel,
                                  Adapter->PhyOui));
        break;
    }
    if (Phy == 33)
    {
        return FALSE;
    }

    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
SidebandUnitAcquireSemaphore(
    _Inout_ PNVNET_ADAPTER Adapter)
{
    ULONG i;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    for (i = 10; i > 0; --i)
    {
        if ((NV_READ(Adapter, NvRegTransmitterControl) & NVREG_XMITCTL_MGMT_SEMA_MASK) ==
            NVREG_XMITCTL_MGMT_SEMA_FREE)
        {
            break;
        }

        NdisMSleep(500000);
    }
    if (i == 0)
    {
        return FALSE;
    }

    for (i = 0; i < 2; ++i)
    {
        ULONG TxControl = NV_READ(Adapter, NvRegTransmitterControl);

        NV_WRITE(Adapter, NvRegTransmitterControl, TxControl | NVREG_XMITCTL_HOST_SEMA_ACQ);

        /* Verify that the semaphore was acquired */
        TxControl = NV_READ(Adapter, NvRegTransmitterControl);
        if (((TxControl & NVREG_XMITCTL_HOST_SEMA_MASK) == NVREG_XMITCTL_HOST_SEMA_ACQ) &&
            ((TxControl & NVREG_XMITCTL_MGMT_SEMA_MASK) == NVREG_XMITCTL_MGMT_SEMA_FREE))
        {
            Adapter->Flags |= NV_UNIT_SEMAPHORE_ACQUIRED;
            return TRUE;
        }

        NdisStallExecution(50);
    }

    return FALSE;
}

VOID
SidebandUnitReleaseSemaphore(
    _In_ PNVNET_ADAPTER Adapter)
{
    if (Adapter->Flags & NV_UNIT_SEMAPHORE_ACQUIRED)
    {
        ULONG TxControl;

        TxControl = NV_READ(Adapter, NvRegTransmitterControl);
        TxControl &= ~NVREG_XMITCTL_HOST_SEMA_ACQ;
        NV_WRITE(Adapter, NvRegTransmitterControl, TxControl);
    }
}

static
CODE_SEG("PAGE")
BOOLEAN
SidebandUnitGetVersion(
    _In_ PNVNET_ADAPTER Adapter,
    _Out_ PULONG Version)
{
    ULONG i, DataReady, DataReady2;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    DataReady = NV_READ(Adapter, NvRegTransmitterControl);

    NV_WRITE(Adapter, NvRegMgmtUnitGetVersion, NVREG_MGMTUNITGETVERSION);
    NV_WRITE(Adapter, NvRegTransmitterControl, DataReady ^ NVREG_XMITCTL_DATA_START);

    for (i = 100000; i > 0; --i)
    {
        DataReady2 = NV_READ(Adapter, NvRegTransmitterControl);

        if ((DataReady & NVREG_XMITCTL_DATA_READY) != (DataReady2 & NVREG_XMITCTL_DATA_READY))
        {
            break;
        }

        NdisStallExecution(50);
    }
    if (i == 0 || DataReady2 & NVREG_XMITCTL_DATA_ERROR)
    {
        return FALSE;
    }

    *Version = NV_READ(Adapter, NvRegMgmtUnitVersion) & NVREG_MGMTUNITVERSION;

    return TRUE;
}

static
BOOLEAN
MiiGetSpeedAndDuplex(
    _In_ PNVNET_ADAPTER Adapter,
    _Out_ PULONG MiiAdvertise,
    _Out_ PULONG MiiLinkPartnerAbility,
    _Out_ PULONG LinkSpeed,
    _Out_ PBOOLEAN FullDuplex)
{
    ULONG MiiStatus, AdvLpa;

    *MiiAdvertise = 0;
    *MiiLinkPartnerAbility = 0;

    /* Link status is a latched-low bit, read it twice */
    MiiRead(Adapter, Adapter->PhyAddress, MII_STATUS, &MiiStatus);
    MiiRead(Adapter, Adapter->PhyAddress, MII_STATUS, &MiiStatus);

    /* Check link status */
    if (!(MiiStatus & MII_SR_LINK_STATUS))
    {
        /* No link detected - configure NIC for 10 MB HD */
        *LinkSpeed = NVREG_LINKSPEED_10;
        *FullDuplex = FALSE;
        return FALSE;
    }

    /* If we are forcing speed and duplex */
    if (Adapter->Flags & NV_FORCE_SPEED_AND_DUPLEX)
    {
        if (Adapter->Flags & NV_USER_SPEED_100)
        {
            *LinkSpeed = NVREG_LINKSPEED_100;
        }
        else
        {
            *LinkSpeed = NVREG_LINKSPEED_10;
        }
        *FullDuplex = !!(Adapter->Flags & NV_FORCE_FULL_DUPLEX);
        return TRUE;
    }

    /* Check auto-negotiation is complete */
    if (!(MiiStatus & MII_SR_AUTONEG_COMPLETE))
    {
        /* Still in auto-negotiation - configure NIC for 10 MBit HD and wait */
        *LinkSpeed = NVREG_LINKSPEED_10;
        *FullDuplex = FALSE;
        return FALSE;
    }

    MiiRead(Adapter, Adapter->PhyAddress, MII_AUTONEG_ADVERTISE, MiiAdvertise);
    MiiRead(Adapter, Adapter->PhyAddress, MII_AUTONEG_LINK_PARTNER, MiiLinkPartnerAbility);

    /* Gigabit ethernet */
    if (Adapter->Flags & NV_GIGABIT_PHY)
    {
        ULONG MiiControl1000, MiiStatus1000;

        MiiRead(Adapter, Adapter->PhyAddress, MII_MASTER_SLAVE_CONTROL, &MiiControl1000);
        MiiRead(Adapter, Adapter->PhyAddress, MII_MASTER_SLAVE_STATUS, &MiiStatus1000);

        if ((MiiControl1000 & MII_MS_CR_1000T_FD) && (MiiStatus1000 & MII_MS_SR_1000T_FD))
        {
            *LinkSpeed = NVREG_LINKSPEED_1000;
            *FullDuplex = TRUE;
            return TRUE;
        }
    }

    AdvLpa = (*MiiAdvertise) & (*MiiLinkPartnerAbility);
    if (AdvLpa & MII_LP_100T_FD)
    {
        *LinkSpeed = NVREG_LINKSPEED_100;
        *FullDuplex = TRUE;
    }
    else if (AdvLpa & MII_LP_100T_HD)
    {
        *LinkSpeed = NVREG_LINKSPEED_100;
        *FullDuplex = FALSE;
    }
    else if (AdvLpa & MII_LP_10T_FD)
    {
        *LinkSpeed = NVREG_LINKSPEED_10;
        *FullDuplex = TRUE;
    }
    else if (AdvLpa & MII_LP_10T_HD)
    {
        *LinkSpeed = NVREG_LINKSPEED_10;
        *FullDuplex = FALSE;
    }
    else
    {
        *LinkSpeed = NVREG_LINKSPEED_10;
        *FullDuplex = FALSE;
    }

    return TRUE;
}

static
VOID
NvNetSetSpeedAndDuplex(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG MiiAdvertise,
    _In_ ULONG MiiLinkPartnerAbility)
{
    ULONG PhyRegister, TxDeferral, PauseFlags, MiiExpansion;
    BOOLEAN RestartTransmitter = FALSE, RestartReceiver = FALSE;

    /* The transmitter and receiver must be restarted for safe update */
    if (NV_READ(Adapter, NvRegTransmitterControl) & NVREG_XMITCTL_START)
    {
        RestartTransmitter = TRUE;
        NvNetStopTransmitter(Adapter);
    }
    if (NV_READ(Adapter, NvRegReceiverControl) & NVREG_RCVCTL_START)
    {
        RestartReceiver = TRUE;
        NvNetStopReceiver(Adapter);
    }

    if (Adapter->Flags & NV_GIGABIT_PHY)
    {
        PhyRegister = NV_READ(Adapter, NvRegSlotTime);
        PhyRegister &= ~NVREG_SLOTTIME_1000_FULL;
        if ((Adapter->LinkSpeed == NVREG_LINKSPEED_10) ||
            (Adapter->LinkSpeed == NVREG_LINKSPEED_100))
        {
            PhyRegister |= NVREG_SLOTTIME_10_100_FULL;
        }
        else if (Adapter->LinkSpeed == NVREG_LINKSPEED_1000)
        {
            PhyRegister |= NVREG_SLOTTIME_1000_FULL;
        }
        NV_WRITE(Adapter, NvRegSlotTime, PhyRegister);
    }

    PhyRegister = NV_READ(Adapter, NvRegPhyInterface);
    PhyRegister &= ~(PHY_HALF | PHY_100 | PHY_1000);
    if (!Adapter->FullDuplex)
    {
        PhyRegister |= PHY_HALF;
    }
    if (Adapter->LinkSpeed == NVREG_LINKSPEED_100)
        PhyRegister |= PHY_100;
    else if (Adapter->LinkSpeed == NVREG_LINKSPEED_1000)
        PhyRegister |= PHY_1000;
    NV_WRITE(Adapter, NvRegPhyInterface, PhyRegister);

    /* Setup the deferral register */
    MiiRead(Adapter, Adapter->PhyAddress, MII_AUTONEG_EXPANSION, &MiiExpansion);
    if (PhyRegister & PHY_RGMII)
    {
        if (Adapter->LinkSpeed == NVREG_LINKSPEED_1000)
        {
            TxDeferral = NVREG_TX_DEFERRAL_RGMII_1000;
        }
        else
        {
            if (!(MiiExpansion & MII_EXP_LP_AUTONEG) && !Adapter->FullDuplex &&
                (Adapter->Features & DEV_HAS_COLLISION_FIX))
            {
                TxDeferral = NVREG_TX_DEFERRAL_RGMII_STRETCH_10;
            }
            else
            {
                TxDeferral = NVREG_TX_DEFERRAL_RGMII_STRETCH_100;
            }
        }
    }
    else
    {
        if (!(MiiExpansion & MII_EXP_LP_AUTONEG) && !Adapter->FullDuplex &&
            (Adapter->Features & DEV_HAS_COLLISION_FIX))
        {
            TxDeferral = NVREG_TX_DEFERRAL_MII_STRETCH;
        }
        else
        {
            TxDeferral = NVREG_TX_DEFERRAL_DEFAULT;
        }
    }
    NV_WRITE(Adapter, NvRegTxDeferral, TxDeferral);

    /* Setup the watermark register */
    if (Adapter->Features & (DEV_HAS_HIGH_DMA | DEV_HAS_LARGEDESC))
    {
        if (Adapter->LinkSpeed == NVREG_LINKSPEED_1000)
            NV_WRITE(Adapter, NvRegTxWatermark, NVREG_TX_WM_DESC2_3_1000);
        else
            NV_WRITE(Adapter, NvRegTxWatermark, NVREG_TX_WM_DESC2_3_DEFAULT);
    }
    else
    {
        NV_WRITE(Adapter, NvRegTxWatermark, NVREG_TX_WM_DESC1_DEFAULT);
    }

    NV_WRITE(Adapter, NvRegMisc1, NVREG_MISC1_FORCE | (Adapter->FullDuplex ? 0 : NVREG_MISC1_HD));
    NV_WRITE(Adapter, NvRegLinkSpeed, Adapter->LinkSpeed | NVREG_LINKSPEED_FORCE);

    PauseFlags = 0;

    /* Setup pause frames */
    if (Adapter->FullDuplex)
    {
        if (!(Adapter->Flags & NV_FORCE_SPEED_AND_DUPLEX) &&
            (Adapter->PauseFlags & NV_PAUSEFRAME_AUTONEG))
        {
            ULONG AdvPause = MiiAdvertise & (MII_ADV_PAUSE_SYM | MII_ADV_PAUSE_ASYM);
            ULONG LpaPause = MiiLinkPartnerAbility & (MII_LP_PAUSE_SYM | MII_LP_PAUSE_ASYM);

            switch (AdvPause)
            {
                case MII_ADV_PAUSE_SYM:
                {
                    if (LpaPause & MII_LP_PAUSE_SYM)
                    {
                        PauseFlags |= NV_PAUSEFRAME_RX_ENABLE;

                        if (Adapter->PauseFlags & NV_PAUSEFRAME_TX_REQ)
                            PauseFlags |= NV_PAUSEFRAME_TX_ENABLE;
                    }
                    break;
                }
                case MII_ADV_PAUSE_ASYM:
                {
                    if (LpaPause == (MII_LP_PAUSE_SYM | MII_LP_PAUSE_ASYM))
                    {
                        PauseFlags |= NV_PAUSEFRAME_TX_ENABLE;
                    }
                    break;
                }
                case (MII_ADV_PAUSE_SYM | MII_ADV_PAUSE_ASYM):
                {
                    if (LpaPause & MII_LP_PAUSE_SYM)
                    {
                        PauseFlags |= NV_PAUSEFRAME_RX_ENABLE;

                        if (Adapter->PauseFlags & NV_PAUSEFRAME_TX_REQ)
                            PauseFlags |= NV_PAUSEFRAME_TX_ENABLE;
                    }
                    if (LpaPause == MII_LP_PAUSE_ASYM)
                    {
                        PauseFlags |= NV_PAUSEFRAME_RX_ENABLE;
                    }
                    break;
                }

                default:
                    break;
            }
        }
        else
        {
            PauseFlags = Adapter->PauseFlags;
        }
    }
    NvNetUpdatePauseFrame(Adapter, PauseFlags);

    if (RestartTransmitter)
    {
        NvNetStartTransmitter(Adapter);
    }
    if (RestartReceiver)
    {
        NvNetStartReceiver(Adapter);
    }
}

BOOLEAN
NvNetUpdateLinkSpeed(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG MiiAdvertise, MiiLinkPartnerAbility, LinkSpeed;
    BOOLEAN FullDuplex, LinkUp;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    LinkUp = MiiGetSpeedAndDuplex(Adapter,
                                  &MiiAdvertise,
                                  &MiiLinkPartnerAbility,
                                  &LinkSpeed,
                                  &FullDuplex);
    if (Adapter->FullDuplex == FullDuplex && Adapter->LinkSpeed == LinkSpeed)
    {
        return LinkUp;
    }

    NDIS_DbgPrint(MIN_TRACE, ("Configuring MAC from '%lx %s-duplex' to '%lx %s-duplex'\n",
                              Adapter->LinkSpeed,
                              Adapter->FullDuplex ? "full" : "half",
                              LinkSpeed,
                              FullDuplex ? "full" : "half"));

    Adapter->FullDuplex = FullDuplex;
    Adapter->LinkSpeed = LinkSpeed;

    if (Adapter->Flags & NV_ACTIVE)
    {
        NdisDprAcquireSpinLock(&Adapter->Send.Lock);
        NdisDprAcquireSpinLock(&Adapter->Receive.Lock);
    }

    NvNetSetSpeedAndDuplex(Adapter, MiiAdvertise, MiiLinkPartnerAbility);

    if (Adapter->Flags & NV_ACTIVE)
    {
        NdisDprReleaseSpinLock(&Adapter->Receive.Lock);
        NdisDprReleaseSpinLock(&Adapter->Send.Lock);
    }

    return LinkUp;
}

CODE_SEG("PAGE")
NDIS_STATUS
NvNetPhyInit(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG PhyState;
    BOOLEAN RestorePhyState = FALSE, PhyInitialized = FALSE;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    /* Take PHY and NIC out of low power mode */
    if (Adapter->Features & DEV_HAS_POWER_CNTRL)
    {
        ULONG PowerState = NV_READ(Adapter, NvRegPowerState2);

        PowerState &= ~NVREG_POWERSTATE2_POWERUP_MASK;
        if ((Adapter->Features & DEV_NEED_LOW_POWER_FIX) && Adapter->RevisionId >= 0xA3)
        {
            PowerState |= NVREG_POWERSTATE2_POWERUP_REV_A3;
        }
        NV_WRITE(Adapter, NvRegPowerState2, PowerState);
    }

    /* Clear PHY state and temporarily halt PHY interrupts */
    NV_WRITE(Adapter, NvRegMIIMask, 0);
    PhyState = NV_READ(Adapter, NvRegAdapterControl);
    if (PhyState & NVREG_ADAPTCTL_RUNNING)
    {
        RestorePhyState = TRUE;

        PhyState &= ~NVREG_ADAPTCTL_RUNNING;
        NV_WRITE(Adapter, NvRegAdapterControl, PhyState);
    }
    NV_WRITE(Adapter, NvRegMIIStatus, NVREG_MIISTAT_MASK_ALL);

    if (Adapter->Features & DEV_HAS_MGMT_UNIT)
    {
        ULONG UnitVersion;

        /* Management unit running on the MAC? */
        if ((NV_READ(Adapter, NvRegTransmitterControl) & NVREG_XMITCTL_MGMT_ST) &&
            (NV_READ(Adapter, NvRegTransmitterControl) & NVREG_XMITCTL_SYNC_PHY_INIT) &&
            SidebandUnitAcquireSemaphore(Adapter) &&
            SidebandUnitGetVersion(Adapter, &UnitVersion))
        {
            if (UnitVersion > 0)
            {
                if (NV_READ(Adapter, NvRegMgmtUnitControl) & NVREG_MGMTUNITCONTROL_INUSE)
                    Adapter->Flags |= NV_MAC_IN_USE;
                else
                    Adapter->Flags &= ~NV_MAC_IN_USE;
            }
            else
            {
                Adapter->Flags |= NV_MAC_IN_USE;
            }

            NDIS_DbgPrint(MIN_TRACE, ("Management unit is running. MAC in use\n"));

            /* Management unit setup the PHY already? */
            if ((Adapter->Flags & NV_MAC_IN_USE) &&
                ((NV_READ(Adapter, NvRegTransmitterControl) & NVREG_XMITCTL_SYNC_MASK) ==
                 NVREG_XMITCTL_SYNC_PHY_INIT))
            {
                /* PHY is inited by management unit */
                PhyInitialized = TRUE;

                NDIS_DbgPrint(MIN_TRACE, ("PHY already initialized by management unit\n"));
            }
        }
    }

    /* Find a suitable PHY */
    if (!FindPhyDevice(Adapter))
    {
        NDIS_DbgPrint(MAX_TRACE, ("Could not find a valid PHY\n"));
        goto Failure;
    }

    /* We need to init the PHY */
    if (!PhyInitialized)
    {
        if (!PhyInit(Adapter))
        {
            /* It's not critical for init, continue */
        }
    }
    else
    {
        ULONG MiiStatus;

        /* See if it is a gigabit PHY */
        MiiRead(Adapter, Adapter->PhyAddress, MII_STATUS, &MiiStatus);
        if (MiiStatus & PHY_GIGABIT)
        {
            Adapter->Flags |= NV_GIGABIT_PHY;
        }
    }

    return NDIS_STATUS_SUCCESS;

Failure:
    if (RestorePhyState)
    {
        NV_WRITE(Adapter, NvRegAdapterControl, PhyState | NVREG_ADAPTCTL_RUNNING);
    }

    return NDIS_STATUS_FAILURE;
}
