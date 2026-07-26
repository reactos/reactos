/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Packet sending
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
VOID
EeStartTransmit(
    _In_ PE100_ADAPTER Adapter,
    _In_ PE100_TX_CONTEXT TxContext)
{
    if (Adapter->CommandUnitActive)
    {
        TRACE("Resume CU %02X\n", FXP_SCB_CUS(CSR_READ_8(Adapter, FXP_CSR_SCB_RUSCUS)));

        /* Errata: LAN Microcontroller PCI Protocol Violation with 10Mbps half-duplex link */
        if (!(Adapter->CurrentMedia & E100_MEDIA_100T) &&
            !(Adapter->CurrentMedia & E100_MEDIA_FD) &&
            (Adapter->Flags & E100_FLAG_IS_ICH))
        {
            /* Issue a CU_NOP command before CU_RESUME and wait */
            EeCommandUnitExecuteCommand(Adapter, FXP_SCB_COMMAND_CU_NOP);
            NdisStallExecution(1);
        }

        EeCommandUnitExecuteCommand(Adapter, FXP_SCB_COMMAND_CU_RESUME);
    }
    else
    {
        TRACE("Starting CU %02X\n", FXP_SCB_CUS(CSR_READ_8(Adapter, FXP_CSR_SCB_RUSCUS)));

        Adapter->CommandUnitActive = TRUE;

        EeCommandUnitExecuteCommandAddr(Adapter, FXP_SCB_COMMAND_CU_START, TxContext->TcbPhys);
    }
}

static
VOID
EeInsertVlanTag(
    _In_ PE100_ADAPTER Adapter,
    _In_ PNDIS_PACKET Packet,
    _In_ PFXP_CB_TRANSMIT Tcb)
{
    NDIS_PACKET_8021Q_INFO VlanTag;

    VlanTag.Value = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, Ieee8021QInfo);

    if (Adapter->Flags & E100_FLAG_VLAN_TAGGING)
    {
        if (!VlanTag.TagHeader.VlanId)
            VlanTag.TagHeader.VlanId = Adapter->VlanId;
    }
    else
    {
        VlanTag.TagHeader.VlanId = 0;
    }

    if (!(Adapter->Flags & E100_FLAG_PACKET_PRIORITY))
    {
        VlanTag.TagHeader.UserPriority = 0;
    }

    if (VlanTag.TagHeader.VlanId || VlanTag.TagHeader.UserPriority)
    {
        USHORT Control;

        Control = VlanTag.TagHeader.VlanId;
        Control |= VlanTag.TagHeader.CanonicalFormatId << 12;
        Control |= VlanTag.TagHeader.UserPriority << 13;

        Tcb->Ipcb.VlanId = htons(Control);
        Tcb->Ipcb.IpActivationHigh |= FXP_IPCB_INSERTVLAN_ENABLE;
    }
}

static
VOID
EeSetChecksumOperation(
    _In_ PE100_ADAPTER Adapter,
    _In_ PNDIS_PACKET Packet,
    _In_ PFXP_CB_TRANSMIT Tcb)
{
    NDIS_TCP_IP_CHECKSUM_PACKET_INFO CsInfo;

    CsInfo.Value = PtrToUlong(NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, TcpIpChecksumPacketInfo));

    if (CsInfo.Transmit.NdisPacketChecksumV4)
    {
        if (CsInfo.Transmit.NdisPacketIpChecksum && Adapter->Offload.SendIpChecksum)
        {
            Tcb->Ipcb.IpSchedule |= FXP_IPCB_IP_CHECKSUM_ENABLE;
        }

        if ((CsInfo.Transmit.NdisPacketTcpChecksum && Adapter->Offload.SendTcpChecksum) ||
            (CsInfo.Transmit.NdisPacketUdpChecksum && Adapter->Offload.SendUdpChecksum))
        {
            Tcb->Ipcb.IpSchedule |= FXP_IPCB_TCPUDP_CHECKSUM_ENABLE;
        }
    }
}

static
VOID
EeTransmitPacket(
    _In_ E100_ADAPTER* __restrict Adapter,
    _In_ NDIS_PACKET* __restrict Packet,
    _In_ FXP_CB_TRANSMIT* __restrict Tcb,
    _In_ SCATTER_GATHER_LIST* __restrict SgList)
{
    PFXP_TBD Tbd;
    ULONG i;

    Tbd = Tcb->Tbd;

    if (Adapter->Flags & E100_FLAG_EXT_RFA)
    {
        Tcb->Ipcb.IpActivationHigh = FXP_IPCB_HARDWAREPARSING_ENABLE;

        if ((Adapter->Flags & E100_FLAG_VLAN_TAGGING) ||
            (Adapter->Flags & E100_FLAG_PACKET_PRIORITY))
        {
            EeInsertVlanTag(Adapter, Packet, Tcb);
        }

        EeSetChecksumOperation(Adapter, Packet, Tcb);

        /* Skip IPCB block */
        Tbd++;
    }

    for (i = 0; i < SgList->NumberOfElements; ++i)
    {
        /* 32-bit DMA */
        ASSERT(SgList->Elements[i].Address.HighPart == 0);

        Tbd[i].Address = htole32(SgList->Elements[i].Address.LowPart);
        Tbd[i].Size = htole32(SgList->Elements[i].Length);
    }

    if (Adapter->Flags & E100_FLAG_EXT_RFA)
    {
        /* Dynamic TBD mode, required in order to use LSO */
        Tcb->TbdNumber = 0xFF;
        Tcb->Tbd[i].Size |= htole32(FXP_TBD_DYNTBD_EL);
    }
    else
    {
        Tcb->TbdNumber = i;
    }

    Tcb->ByteCount = 0;
    Tcb->Header.Status = 0;
    Tcb->Header.Command = Adapter->TransmitCommand;
    Tcb->TxThreshold = Adapter->TransmitThreshold;
}

static
BOOLEAN
EeCopyPacket(
    _In_ PNDIS_PACKET Packet,
    _In_ PE100_COALESCE_BUFFER Buffer)
{
    PNDIS_BUFFER CurrentBuffer;
    PVOID Address;
    UINT CurrentLength, PacketLength;
    PUCHAR Destination;

    NdisGetFirstBufferFromPacketSafe(Packet,
                                     &CurrentBuffer,
                                     &Address,
                                     &CurrentLength,
                                     &PacketLength,
                                     HighPagePriority);
    if (!Address)
        return FALSE;

    Destination = Buffer->VirtualAddress;

    while (TRUE)
    {
        NdisMoveMemory(Destination, Address, CurrentLength);
        Destination += CurrentLength;

        NdisGetNextBuffer(CurrentBuffer, &CurrentBuffer);
        if (!CurrentBuffer)
            break;

        NdisQueryBufferSafe(CurrentBuffer,
                            &Address,
                            &CurrentLength,
                            HighPagePriority);
        if (!Address)
            return FALSE;
    }

    return TRUE;
}

static
NDIS_STATUS
EeSendPacket(
    _In_ PE100_ADAPTER Adapter,
    _In_ PNDIS_PACKET Packet)
{
    PSCATTER_GATHER_LIST SgList;
    PE100_TX_CONTEXT TxContext;

    if (Adapter->TxPending >= Adapter->TcbCount)
        return NDIS_STATUS_RESOURCES;

    TxContext = Adapter->TxLast->Next;

    SgList = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo);
    if (SgList->NumberOfElements > Adapter->MaxTbdCount)
    {
        PE100_COALESCE_BUFFER CoalesceBuffer;
        UINT PacketLength;
        PSINGLE_LIST_ENTRY Entry;

        Entry = Adapter->SendBufferList.Next;
        if (!Entry)
            return NDIS_STATUS_RESOURCES;

        CoalesceBuffer = CONTAINING_RECORD(Entry, E100_COALESCE_BUFFER, ListEntry);

        NdisQueryPacketLength(Packet, &PacketLength);
        if (!EeCopyPacket(Packet, CoalesceBuffer))
            return NDIS_STATUS_RESOURCES;

        NT_VERIFY(PopEntryList(&Adapter->SendBufferList) != NULL);

        SgList = &Adapter->LocalSgList;
        SgList->Elements[0].Address.LowPart = CoalesceBuffer->PhysicalAddress;
        SgList->Elements[0].Length = PacketLength;
        SgList->NumberOfElements = 1;

        TxContext->Buffer = CoalesceBuffer;
    }

    ASSERT(Adapter->TxPending < Adapter->TcbCount);
    ++Adapter->TxPending;

    TxContext->Packet = Packet;

    EeTransmitPacket(Adapter, Packet, TxContext->Tcb, SgList);

    /* Reset suspend bit in the previous TCB */
    EE_WRITE_BARRIER();
    Adapter->TxLast->Tcb->Header.Command &= htole16(~FXP_CB_COMMAND_S);
    Adapter->TxLast = TxContext;

    return NDIS_STATUS_PENDING;
}

VOID
EeSendPackets(
    _In_ PE100_ADAPTER Adapter,
    _In_ PPNDIS_PACKET PacketArray,
    _In_ UINT NumberOfPackets)
{
    PE100_TX_CONTEXT FirstTxContext;
    ULONG i, PrevTxPending;
    PLIST_ENTRY Entry;
    NDIS_STATUS Status;
    PNDIS_PACKET Packet;

    TRACE("Send packets %lu\n", NumberOfPackets);

    FirstTxContext = Adapter->TxLast->Next;
    PrevTxPending = Adapter->TxPending;

    if (NumberOfPackets == 0)
    {
        ASSERT(!IsListEmpty(&Adapter->SendQueueList));

        /* Try to start more packets transmitting */
        do
        {
            Entry = RemoveHeadList(&Adapter->SendQueueList);

            Packet = E100_PACKET_FROM_LIST_ENTRY(Entry);

            Status = EeSendPacket(Adapter, Packet);
            if (Status == NDIS_STATUS_RESOURCES)
            {
                InsertHeadList(&Adapter->SendQueueList, E100_LIST_ENTRY_FROM_PACKET(Packet));
                break;
            }
        }
        while (!IsListEmpty(&Adapter->SendQueueList));
    }
    else
    {
        for (i = 0; i < NumberOfPackets; ++i)
        {
            Packet = PacketArray[i];

            Status = EeSendPacket(Adapter, Packet);
            if (Status == NDIS_STATUS_RESOURCES)
            {
                InsertTailList(&Adapter->SendQueueList, E100_LIST_ENTRY_FROM_PACKET(Packet));
            }
        }
    }

    /* Issue a CU_RESUME command in case the CU was suspended */
    if (Adapter->TxPending != PrevTxPending)
    {
        Adapter->TxTimeOut = 3;

        EeStartTransmit(Adapter, FirstTxContext);
    }
}

VOID
NTAPI
MiniportSendPackets(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PPNDIS_PACKET PacketArray,
    _In_ UINT NumberOfPackets)
{
    PE100_ADAPTER Adapter = MiniportAdapterContext;
    ULONG i;

    NdisAcquireSpinLock(&Adapter->SendLock);

    if (!Adapter->AdapterActive)
    {
        NdisReleaseSpinLock(&Adapter->SendLock);

        for (i = 0; i < NumberOfPackets; ++i)
        {
            NdisMSendComplete(Adapter->AdapterHandle, PacketArray[i], NDIS_STATUS_NOT_ACCEPTED);
        }
        return;
    }

    EeSendPackets(Adapter, PacketArray, NumberOfPackets);

    NdisReleaseSpinLock(&Adapter->SendLock);
}

VOID
NTAPI
MiniportCancelSendPackets(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PVOID CancelId)
{
    PE100_ADAPTER Adapter = MiniportAdapterContext;
    LIST_ENTRY DoneList;
    PLIST_ENTRY Entry, NextEntry;

    TRACE("Called\n");

    InitializeListHead(&DoneList);

    NdisAcquireSpinLock(&Adapter->SendLock);

    NextEntry = Adapter->SendQueueList.Flink;
    while (NextEntry != &Adapter->SendQueueList)
    {
        PNDIS_PACKET Packet;

        Entry = NextEntry;
        NextEntry = NextEntry->Flink;

        Packet = E100_PACKET_FROM_LIST_ENTRY(Entry);

        if (NDIS_GET_PACKET_CANCEL_ID(Packet) == CancelId)
        {
            RemoveEntryList(E100_LIST_ENTRY_FROM_PACKET(Packet));

            InsertTailList(&DoneList, E100_LIST_ENTRY_FROM_PACKET(Packet));
        }
    }

    NdisReleaseSpinLock(&Adapter->SendLock);

    while (!IsListEmpty(&DoneList))
    {
        Entry = RemoveHeadList(&DoneList);

        NdisMSendComplete(Adapter->AdapterHandle,
                          E100_PACKET_FROM_LIST_ENTRY(Entry),
                          NDIS_STATUS_REQUEST_ABORTED);
    }
}
