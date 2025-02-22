/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware specific functions
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
DcDisableHw(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG OpMode;

    /* Disable interrupts */
    DC_WRITE(Adapter, DcCsr7_IrqMask, 0);

    /* Stop DMA */
    OpMode = Adapter->OpMode;
    OpMode &= ~(DC_OPMODE_RX_ENABLE | DC_OPMODE_TX_ENABLE);
    DC_WRITE(Adapter, DcCsr6_OpMode, OpMode);

    /* Put the adapter to snooze mode */
    DcPowerSave(Adapter, TRUE);

    /* Perform a software reset */
    DC_WRITE(Adapter, DcCsr0_BusMode, DC_BUS_MODE_SOFT_RESET);
}

VOID
DcStopTxRxProcess(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG i, OpMode, Status;

    OpMode = Adapter->OpMode;
    OpMode &= ~(DC_OPMODE_RX_ENABLE | DC_OPMODE_TX_ENABLE);
    DC_WRITE(Adapter, DcCsr6_OpMode, OpMode);

    for (i = 0; i < 5000; ++i)
    {
        Status = DC_READ(Adapter, DcCsr5_Status);

        if (((Status & DC_STATUS_TX_STATE_MASK) == DC_STATUS_TX_STATE_STOPPED) &&
            ((Status & DC_STATUS_RX_STATE_MASK) == DC_STATUS_RX_STATE_STOPPED))
        {
            return;
        }

        NdisStallExecution(10);
    }

    WARN("Failed to stop the TX/RX process 0x%08lx\n", Status);
}

VOID
DcWriteGpio(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG Value)
{
    ULONG Data, Register;

    /* Some chips don't have a separate GPIO register */
    if (Adapter->Features & DC_SIA_GPIO)
    {
        Data = Adapter->SiaSetting;
        Data &= 0x0000FFFF;  /* SIA */
        Data |= Value << 16; /* GPIO */
        Adapter->SiaSetting = Data;

        Register = DcCsr15_SiaGeneral;
    }
    else
    {
        Data = Value;
        Register = DcCsr12_Gpio;
    }
    DC_WRITE(Adapter, Register, Data);
}

VOID
DcWriteSia(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG Csr13,
    _In_ ULONG Csr14,
    _In_ ULONG Csr15)
{
    ULONG SiaConn, SiaGen;

    TRACE("CSR13 %08lx, CSR14 %08lx, CSR15 %08lx\n", Csr13, Csr14, Csr15);

    SiaConn = 0;

    /* The 21145 comes with 16 new bits in CSR13 */
    if (Adapter->Features & DC_SIA_ANALOG_CONTROL)
    {
        SiaConn = Adapter->AnalogControl;
    }

    /* Reset the transceiver */
    DC_WRITE(Adapter, DcCsr13_SiaConnectivity, SiaConn | DC_SIA_CONN_RESET);
    NdisStallExecution(20);

    /* Some chips don't have a separate GPIO register */
    if (Adapter->Features & DC_SIA_GPIO)
    {
        SiaGen = Adapter->SiaSetting;
        SiaGen &= 0xFFFF0000; /* GPIO */
        SiaGen |= Csr15;      /* SIA */
        Adapter->SiaSetting = SiaGen;
    }
    else
    {
        SiaGen = Csr15;
    }

    DC_WRITE(Adapter, DcCsr14_SiaTxRx, Csr14);
    DC_WRITE(Adapter, DcCsr15_SiaGeneral, SiaGen);

    /* Don't reset the transceiver twice */
    if (Csr13 == DC_SIA_CONN_RESET)
        return;

    DC_WRITE(Adapter, DcCsr13_SiaConnectivity, SiaConn | Csr13);
}

VOID
DcTestPacket(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_TCB Tcb;
    PDC_TBD Tbd;
    ULONG FrameNumber;

    Adapter->MediaTestStatus = FALSE;
    Adapter->ModeFlags |= DC_MODE_TEST_PACKET;

    if (!Adapter->LoopbackFrameSlots)
    {
        ERR("Failed to complete test packets, CSR12 %08lx, CSR5 %08lx\n",
            DC_READ(Adapter, DcCsr12_SiaStatus),
            DC_READ(Adapter, DcCsr5_Status));

        /* Try to recover the lost TX buffers */
        NdisScheduleWorkItem(&Adapter->TxRecoveryWorkItem);
        return;
    }

    --Adapter->LoopbackFrameSlots;

    FrameNumber = (Adapter->LoopbackFrameNumber++) % DC_LOOPBACK_FRAMES;

    Tbd = Adapter->CurrentTbd;
    Adapter->CurrentTbd = DC_NEXT_TBD(Adapter, Tbd);

    Tcb = Adapter->CurrentTcb;
    Adapter->CurrentTcb = DC_NEXT_TCB(Adapter, Tcb);

    Tcb->Tbd = Tbd;
    Tcb->Packet = NULL;

    ASSERT(!(Tbd->Status & DC_TBD_STATUS_OWNED));

    /* Put the loopback frame on the transmit ring */
    Tbd->Address1 = Adapter->LoopbackFramePhys[FrameNumber];
    Tbd->Address2 = 0;
    Tbd->Control &= DC_TBD_CONTROL_END_OF_RING;
    Tbd->Control |= DC_LOOPBACK_FRAME_SIZE |
                    DC_TBD_CONTROL_FIRST_FRAGMENT |
                    DC_TBD_CONTROL_LAST_FRAGMENT |
                    DC_TBD_CONTROL_REQUEST_INTERRUPT;
    DC_WRITE_BARRIER();
    Tbd->Status = DC_TBD_STATUS_OWNED;

    /* Send the loopback packet to verify connectivity of a media */
    DC_WRITE(Adapter, DcCsr1_TxPoll, DC_TX_POLL_DOORBELL);
}

BOOLEAN
DcSetupFrameDownload(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN WaitForCompletion)
{
    PDC_TCB Tcb;
    PDC_TBD Tbd;
    ULONG i, Control;

    Tbd = Adapter->CurrentTbd;

    /* Ensure correct setup frame processing */
    if (Tbd != Adapter->HeadTbd)
    {
        ASSERT(!(Tbd->Status & DC_TBD_STATUS_OWNED));

        /* Put the null frame on the transmit ring */
        Tbd->Control &= DC_TBD_CONTROL_END_OF_RING;
        Tbd->Address1 = 0;
        Tbd->Address2 = 0;
        DC_WRITE_BARRIER();
        Tbd->Status = DC_TBD_STATUS_OWNED;

        Tbd = DC_NEXT_TBD(Adapter, Tbd);
    }

    Adapter->CurrentTbd = DC_NEXT_TBD(Adapter, Tbd);

    Tcb = Adapter->CurrentTcb;
    Adapter->CurrentTcb = DC_NEXT_TCB(Adapter, Tcb);

    Tcb->Tbd = Tbd;
    Tcb->Packet = NULL;

    ASSERT(!(Tbd->Status & DC_TBD_STATUS_OWNED));

    /* Prepare the setup frame */
    Tbd->Address1 = Adapter->SetupFramePhys;
    Tbd->Address2 = 0;
    Tbd->Control &= DC_TBD_CONTROL_END_OF_RING;
    Control = DC_SETUP_FRAME_SIZE | DC_TBD_CONTROL_SETUP_FRAME;
    if (!WaitForCompletion)
        Control |= DC_TBD_CONTROL_REQUEST_INTERRUPT;
    if (Adapter->ProgramHashPerfectFilter)
        Control |= DC_TBD_CONTROL_HASH_PERFECT_FILTER;
    Tbd->Control |= Control;
    DC_WRITE_BARRIER();
    Tbd->Status = DC_TBD_STATUS_OWNED;

    DC_WRITE(Adapter, DcCsr1_TxPoll, DC_TX_POLL_DOORBELL);

    if (!WaitForCompletion)
        return TRUE;

    /* Wait up to 500 ms for the chip to process the setup frame */
    for (i = 50000; i > 0; --i)
    {
        NdisStallExecution(10);

        KeMemoryBarrierWithoutFence();
        if (!(Tbd->Status & DC_TBD_STATUS_OWNED))
            break;
    }
    if (i == 0)
    {
        ERR("Failed to complete setup frame %08lx\n", Tbd->Status);
        return FALSE;
    }

    return TRUE;
}

CODE_SEG("PAGE")
VOID
DcSetupFrameInitialize(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PULONG SetupFrame, SetupFrameStart;
    PUSHORT MacAddress;
    ULONG i;

    PAGED_CODE();

    SetupFrame = Adapter->SetupFrame;

    /* Add the physical address entry */
    MacAddress = (PUSHORT)Adapter->CurrentMacAddress;
    *SetupFrame++ = DC_SETUP_FRAME_ENTRY(MacAddress[0]);
    *SetupFrame++ = DC_SETUP_FRAME_ENTRY(MacAddress[1]);
    *SetupFrame++ = DC_SETUP_FRAME_ENTRY(MacAddress[2]);

    /* Pad to 16 addresses */
    SetupFrameStart = Adapter->SetupFrame;
    for (i = 1; i < DC_SETUP_FRAME_PERFECT_FILTER_ADDRESSES; ++i)
    {
        *SetupFrame++ = SetupFrameStart[0];
        *SetupFrame++ = SetupFrameStart[1];
        *SetupFrame++ = SetupFrameStart[2];
    }
}

static
VOID
DcSetupFramePerfectFiltering(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PULONG SetupFrame, SetupFrameStart;
    PUSHORT MacAddress;
    ULONG i;

    SetupFrame = Adapter->SetupFrame;

    /* Add the physical address entry */
    MacAddress = (PUSHORT)Adapter->CurrentMacAddress;
    *SetupFrame++ = DC_SETUP_FRAME_ENTRY(MacAddress[0]);
    *SetupFrame++ = DC_SETUP_FRAME_ENTRY(MacAddress[1]);
    *SetupFrame++ = DC_SETUP_FRAME_ENTRY(MacAddress[2]);

    /* Store multicast addresses */
    for (i = 0; i < Adapter->MulticastCount; ++i)
    {
        MacAddress = (PUSHORT)Adapter->MulticastList[i].MacAddress;

        *SetupFrame++ = DC_SETUP_FRAME_ENTRY(MacAddress[0]);
        *SetupFrame++ = DC_SETUP_FRAME_ENTRY(MacAddress[1]);
        *SetupFrame++ = DC_SETUP_FRAME_ENTRY(MacAddress[2]);
    }

    ++i;

    /* Add the broadcast address entry */
    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
    {
        *SetupFrame++ = DC_SETUP_FRAME_ENTRY(0x0000FFFF);
        *SetupFrame++ = DC_SETUP_FRAME_ENTRY(0x0000FFFF);
        *SetupFrame++ = DC_SETUP_FRAME_ENTRY(0x0000FFFF);

        ++i;
    }

    /* Pad to 16 addresses */
    SetupFrameStart = Adapter->SetupFrame;
    while (i < DC_SETUP_FRAME_PERFECT_FILTER_ADDRESSES)
    {
        *SetupFrame++ = SetupFrameStart[0];
        *SetupFrame++ = SetupFrameStart[1];
        *SetupFrame++ = SetupFrameStart[2];

        ++i;
    }
}

static
VOID
DcSetupFrameImperfectFiltering(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PULONG SetupFrame = Adapter->SetupFrame;
    PUSHORT MacAddress;
    ULONG Hash, i;

    RtlZeroMemory(SetupFrame, DC_SETUP_FRAME_SIZE);

    /* Fill up the 512-bit multicast hash table */
    for (i = 0; i < Adapter->MulticastCount; ++i)
    {
        MacAddress = (PUSHORT)Adapter->MulticastList[i].MacAddress;

        /* Only need lower 9 bits of the hash */
        Hash = DcEthernetCrc(MacAddress, ETH_LENGTH_OF_ADDRESS);
        Hash &= 512 - 1;
        SetupFrame[Hash / 16] |= 1 << (Hash % 16);
    }

    /* Insert the broadcast address hash to the bin */
    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
    {
        Hash = DC_SETUP_FRAME_BROADCAST_HASH;
        SetupFrame[Hash / 16] |= 1 << (Hash % 16);
    }

    /* Add the physical address entry */
    MacAddress = (PUSHORT)Adapter->CurrentMacAddress;
    SetupFrame[39] = DC_SETUP_FRAME_ENTRY(MacAddress[0]);
    SetupFrame[40] = DC_SETUP_FRAME_ENTRY(MacAddress[1]);
    SetupFrame[41] = DC_SETUP_FRAME_ENTRY(MacAddress[2]);
}

NDIS_STATUS
DcUpdateMulticastList(
    _In_ PDC21X4_ADAPTER Adapter)
{
    BOOLEAN UsePerfectFiltering;

    /* If more than 14 addresses are requested, switch to hash filtering mode */
    UsePerfectFiltering = (Adapter->MulticastCount <= DC_SETUP_FRAME_ADDRESSES);

    Adapter->ProgramHashPerfectFilter = UsePerfectFiltering;
    Adapter->OidPending = TRUE;

    if (UsePerfectFiltering)
        DcSetupFramePerfectFiltering(Adapter);
    else
        DcSetupFrameImperfectFiltering(Adapter);

    NdisAcquireSpinLock(&Adapter->SendLock);

    DcSetupFrameDownload(Adapter, FALSE);

    NdisReleaseSpinLock(&Adapter->SendLock);

    return NDIS_STATUS_PENDING;
}

NDIS_STATUS
DcApplyPacketFilter(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG PacketFilter)
{
    ULONG OpMode, OldPacketFilter;

    INFO("Packet filter value 0x%lx\n", PacketFilter);

    NdisAcquireSpinLock(&Adapter->ModeLock);

    /* Update the filtering mode */
    OpMode = Adapter->OpMode;
    OpMode &= ~(DC_OPMODE_RX_PROMISCUOUS | DC_OPMODE_RX_ALL_MULTICAST);
    if (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
    {
        OpMode |= DC_OPMODE_RX_PROMISCUOUS;
    }
    else if (PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST)
    {
        OpMode |= DC_OPMODE_RX_ALL_MULTICAST;
    }
    Adapter->OpMode = OpMode;
    DC_WRITE(Adapter, DcCsr6_OpMode, OpMode);

    NdisReleaseSpinLock(&Adapter->ModeLock);

    OldPacketFilter = Adapter->PacketFilter;
    Adapter->PacketFilter = PacketFilter;

    /* Program the NIC to receive or reject broadcast frames */
    if ((OldPacketFilter ^ PacketFilter) & NDIS_PACKET_TYPE_BROADCAST)
    {
        return DcUpdateMulticastList(Adapter);
    }

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
DcSoftReset(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PAGED_CODE();

    /* Linux driver does this */
    if (Adapter->Features & DC_HAS_MII)
    {
        /* Select the MII/SYM port */
        DC_WRITE(Adapter, DcCsr6_OpMode, DC_OPMODE_PORT_SELECT);
    }

    /* Perform a software reset */
    DC_WRITE(Adapter, DcCsr0_BusMode, DC_BUS_MODE_SOFT_RESET);
    NdisMSleep(100);
    DC_WRITE(Adapter, DcCsr0_BusMode, Adapter->BusMode);
}

CODE_SEG("PAGE")
NDIS_STATUS
DcSetupAdapter(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PAGED_CODE();

    DcInitTxRing(Adapter);
    DcInitRxRing(Adapter);

    /* Initial values */
    if (!MEDIA_IS_FIXED(Adapter))
    {
        Adapter->LinkSpeedMbps = 10;
    }
    Adapter->MediaNumber = Adapter->DefaultMedia;
    Adapter->ModeFlags &= ~(DC_MODE_PORT_AUTOSENSE | DC_MODE_AUI_FAILED | DC_MODE_BNC_FAILED |
                            DC_MODE_TEST_PACKET | DC_MODE_AUTONEG_MASK);

    DcSoftReset(Adapter);

    /* Receive descriptor ring buffer */
    DC_WRITE(Adapter, DcCsr3_RxRingAddress, Adapter->RbdPhys);

    /* Transmit descriptor ring buffer */
    DC_WRITE(Adapter, DcCsr4_TxRingAddress, Adapter->TbdPhys);

    switch (Adapter->ChipType)
    {
        case DC21040:
        {
            DcWriteSia(Adapter,
                       Adapter->Media[Adapter->MediaNumber].Csr13,
                       Adapter->Media[Adapter->MediaNumber].Csr14,
                       Adapter->Media[Adapter->MediaNumber].Csr15);

            /* Explicitly specifed by user */
            if (Adapter->MediaNumber == MEDIA_10T_FD)
            {
                Adapter->OpMode |= DC_OPMODE_FULL_DUPLEX;
            }
            break;
        }

        case DC21041:
        {
            MediaSiaSelect(Adapter);
            break;
        }

        case DC21140:
        {
            if (Adapter->MediaNumber == MEDIA_MII)
            {
                MediaSelectMiiPort(Adapter, !(Adapter->Flags & DC_FIRST_SETUP));
                MediaMiiSelect(Adapter);
            }
            else
            {
                /* All media use the same GPIO directon */
                DC_WRITE(Adapter, DcCsr12_Gpio, Adapter->Media[Adapter->MediaNumber].GpioCtrl);
                NdisStallExecution(10);

                MediaGprSelect(Adapter);
            }
            break;
        }

        case DC21143:
        case DC21145:
        {
            /* Init the HPNA PHY */
            if ((Adapter->MediaBitmap & (1 << MEDIA_HMR)) && Adapter->HpnaInitBitmap)
            {
                HpnaPhyInit(Adapter);
            }

            if (Adapter->MediaNumber == MEDIA_MII)
            {
                MediaSelectMiiPort(Adapter, !(Adapter->Flags & DC_FIRST_SETUP));
                MediaMiiSelect(Adapter);
                break;
            }

            /* If the current media is FX, assume we have a link */
            if (MEDIA_IS_FX(Adapter->MediaNumber))
            {
                Adapter->LinkUp = TRUE;

                NdisMIndicateStatus(Adapter->AdapterHandle,
                                    NDIS_STATUS_MEDIA_CONNECT,
                                    NULL,
                                    0);
                NdisMIndicateStatusComplete(Adapter->AdapterHandle);
            }

            MediaSiaSelect(Adapter);
            break;
        }

        default:
            ASSERT(FALSE);
            UNREACHABLE;
            break;
    }

    /* Start the TX process */
    Adapter->OpMode |= DC_OPMODE_TX_ENABLE;
    DC_WRITE(Adapter, DcCsr6_OpMode, Adapter->OpMode);

    /* Load the address recognition RAM */
    if (!DcSetupFrameDownload(Adapter, TRUE))
    {
        /* This normally should not happen */
        ASSERT(FALSE);

        NdisWriteErrorLogEntry(Adapter->AdapterHandle,
                               NDIS_ERROR_CODE_HARDWARE_FAILURE,
                               1,
                               __LINE__);

        return NDIS_STATUS_HARD_ERRORS;
    }

    return NDIS_STATUS_SUCCESS;
}
