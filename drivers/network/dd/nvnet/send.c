/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Packet sending
 * COPYRIGHT:   Copyright 2021-2022 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "nvnet.h"

#define NDEBUG
#include "debug.h"

/* FUNCTIONS ******************************************************************/

VOID
NvNetTransmitPacket32(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNVNET_TCB Tcb,
    _In_ PSCATTER_GATHER_LIST SgList)
{
    NVNET_TBD Tbd, LastTbd;
    ULONG i, Flags;
    ULONG Slots;

    Flags = 0;
    Slots = 0;
    Tbd = Adapter->Send.CurrentTbd;

    for (i = 0; i < SgList->NumberOfElements; ++i)
    {
        ULONG Address = NdisGetPhysicalAddressLow(SgList->Elements[i].Address);
        ULONG Length = SgList->Elements[i].Length;

        if (Length > NV_MAXIMUM_SG_SIZE)
        {
            ULONG ImplicitEntries = NV_IMPLICIT_ENTRIES(Length);

            do
            {
                ++Slots;

                Tbd.x32->Address = Address;
                Tbd.x32->FlagsLength = Flags | (NV_MAXIMUM_SG_SIZE - 1);
                LastTbd = Tbd;
                Tbd = NV_NEXT_TBD_32(Adapter, Tbd);

                Flags = NV_TX_VALID;

                Length -= NV_MAXIMUM_SG_SIZE;
                Address += NV_MAXIMUM_SG_SIZE;

                --ImplicitEntries;
            }
            while (ImplicitEntries);
        }

        ++Slots;

        Tbd.x32->Address = Address;
        Tbd.x32->FlagsLength = Flags | (Length - 1);
        LastTbd = Tbd;
        Tbd = NV_NEXT_TBD_32(Adapter, Tbd);

        Flags = NV_TX_VALID;
    }

    Tcb->Slots = Slots;
    Tcb->Tbd = LastTbd;

    if (Adapter->Features & DEV_HAS_LARGEDESC)
    {
        LastTbd.x32->FlagsLength |= NV_TX2_LASTPACKET;
    }
    else
    {
        LastTbd.x32->FlagsLength |= NV_TX_LASTPACKET;
    }

    if (Tcb->Flags & NV_TCB_LARGE_SEND)
    {
        Flags |= (Tcb->Mss << NV_TX2_TSO_SHIFT) | NV_TX2_TSO;
    }
    else
    {
        if (Tcb->Flags & NV_TCB_CHECKSUM_IP)
        {
            Flags |= NV_TX2_CHECKSUM_L3;
        }
        if (Tcb->Flags & (NV_TCB_CHECKSUM_TCP | NV_TCB_CHECKSUM_UDP))
        {
            Flags |= NV_TX2_CHECKSUM_L4;
        }
    }

    Adapter->Send.CurrentTbd.x32->FlagsLength |= Flags;
    Adapter->Send.CurrentTbd = Tbd;

    NV_WRITE(Adapter, NvRegTxRxControl, Adapter->TxRxControl | NVREG_TXRXCTL_KICK);
}

VOID
NvNetTransmitPacket64(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNVNET_TCB Tcb,
    _In_ PSCATTER_GATHER_LIST SgList)
{
    NVNET_TBD Tbd, LastTbd;
    ULONG i, Flags;
    ULONG Slots;

    Flags = 0;
    Slots = 0;
    Tbd = Adapter->Send.CurrentTbd;

    for (i = 0; i < SgList->NumberOfElements; ++i)
    {
        ULONG64 Address = SgList->Elements[i].Address.QuadPart;
        ULONG Length = SgList->Elements[i].Length;

        if (Length > NV_MAXIMUM_SG_SIZE)
        {
            ULONG ImplicitEntries = NV_IMPLICIT_ENTRIES(Length);

            do
            {
                ++Slots;

                Tbd.x64->AddressLow = (ULONG)Address;
                Tbd.x64->AddressHigh = Address >> 32;
                Tbd.x64->VlanTag = 0;
                Tbd.x64->FlagsLength = Flags | (NV_MAXIMUM_SG_SIZE - 1);
                LastTbd = Tbd;
                Tbd = NV_NEXT_TBD_64(Adapter, Tbd);

                Flags = NV_TX2_VALID;

                Length -= NV_MAXIMUM_SG_SIZE;
                Address += NV_MAXIMUM_SG_SIZE;

                --ImplicitEntries;
            }
            while (ImplicitEntries);
        }

        ++Slots;

        Tbd.x64->AddressLow = (ULONG)Address;
        Tbd.x64->AddressHigh = Address >> 32;
        Tbd.x64->VlanTag = 0;
        Tbd.x64->FlagsLength = Flags | (Length - 1);
        LastTbd = Tbd;
        Tbd = NV_NEXT_TBD_64(Adapter, Tbd);

        Flags = NV_TX2_VALID;
    }

    Tcb->Slots = Slots;
    Tcb->Tbd = LastTbd;

    LastTbd.x64->FlagsLength |= NV_TX2_LASTPACKET;

    if (Adapter->Flags & NV_SEND_ERRATA_PRESENT)
    {
        if (Adapter->Send.PacketsCount == NV_TX_LIMIT_COUNT)
        {
            Tcb->DeferredTbd = Adapter->Send.CurrentTbd;

            if (!Adapter->Send.DeferredTcb)
            {
                Adapter->Send.DeferredTcb = Tcb;
            }

            Flags = 0;
        }
        else
        {
            ++Adapter->Send.PacketsCount;
        }
    }

    if (Tcb->Flags & NV_TCB_LARGE_SEND)
    {
        Flags |= (Tcb->Mss << NV_TX2_TSO_SHIFT) | NV_TX2_TSO;
    }
    else
    {
        if (Tcb->Flags & NV_TCB_CHECKSUM_IP)
        {
            Flags |= NV_TX2_CHECKSUM_L3;
        }
        if (Tcb->Flags & (NV_TCB_CHECKSUM_TCP | NV_TCB_CHECKSUM_UDP))
        {
            Flags |= NV_TX2_CHECKSUM_L4;
        }
    }

    // Adapter->Send.CurrentTbd.x64->VlanTag = NV_TX3_VLAN_TAG_PRESENT; TODO
    Adapter->Send.CurrentTbd.x64->FlagsLength |= Flags;
    Adapter->Send.CurrentTbd = Tbd;

    NV_WRITE(Adapter, NvRegTxRxControl, Adapter->TxRxControl | NVREG_TXRXCTL_KICK);
}

static
DECLSPEC_NOINLINE
ULONG
NvNetQueryTcpIpHeaders(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNDIS_PACKET Packet)
{
    PNDIS_BUFFER CurrentBuffer;
    PVOID Address;
    UINT CurrentLength;
    UINT PacketLength;
    PIPv4_HEADER IpHeader;
    PTCPv4_HEADER TcpHeader;
    ULONG BytesCopied = 0;
    UCHAR Buffer[136];

    NdisGetFirstBufferFromPacketSafe(Packet,
                                     &CurrentBuffer,
                                     &Address,
                                     &CurrentLength,
                                     &PacketLength,
                                     HighPagePriority);
    if (!Address)
        return 0;

    while (TRUE)
    {
        CurrentLength = min(CurrentLength, sizeof(Buffer) - BytesCopied);

        NdisMoveMemory(&Buffer[BytesCopied], Address, CurrentLength);
        BytesCopied += CurrentLength;

        if (BytesCopied >= sizeof(Buffer))
            break;

        NdisGetNextBuffer(CurrentBuffer, &CurrentBuffer);

        if (!CurrentBuffer)
            return 0;

        NdisQueryBufferSafe(CurrentBuffer,
                            &Address,
                            &CurrentLength,
                            HighPagePriority);
    }

    IpHeader = (PIPv4_HEADER)&Buffer[Adapter->IpHeaderOffset];
    TcpHeader = (PTCPv4_HEADER)((PUCHAR)IpHeader + IP_HEADER_LENGTH(IpHeader));

    return IP_HEADER_LENGTH(IpHeader) + TCP_HEADER_LENGTH(TcpHeader);
}

static
BOOLEAN
NvNetCopyPacket(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNDIS_PACKET Packet,
    _In_ PNVNET_TX_BUFFER Buffer)
{
    PNDIS_BUFFER CurrentBuffer;
    PVOID Address;
    UINT CurrentLength;
    UINT PacketLength;
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
NvNetSendPacketLargeSend(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNDIS_PACKET Packet,
    _In_ ULONG TotalLength)
{
    PSCATTER_GATHER_LIST SgList;
    ULONG Mss, Length;
    PNVNET_TCB Tcb;

    if (!Adapter->Send.TcbSlots)
    {
        return NDIS_STATUS_RESOURCES;
    }

    SgList = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo);

    /* Make sure we have room to setup all fragments */
    C_ASSERT(NVNET_TRANSMIT_DESCRIPTORS > ((NVNET_MAXIMUM_LSO_FRAME_SIZE / PAGE_SIZE) + 3));
    ASSERT(SgList->NumberOfElements +
           (NVNET_MAXIMUM_LSO_FRAME_SIZE / (NV_MAXIMUM_SG_SIZE + 1)) <=
           NVNET_TRANSMIT_DESCRIPTORS);

    if (SgList->NumberOfElements +
        (NVNET_MAXIMUM_LSO_FRAME_SIZE / (NV_MAXIMUM_SG_SIZE + 1)) < Adapter->Send.TbdSlots)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Length = NvNetQueryTcpIpHeaders(Adapter, Packet);
    if (!Length)
    {
        return NDIS_STATUS_RESOURCES;
    }

    NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, TcpLargeSendPacketInfo) =
        UlongToPtr(TotalLength - Adapter->IpHeaderOffset - Length);

    --Adapter->Send.TcbSlots;

    Mss = PtrToUlong(NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, TcpLargeSendPacketInfo));

    Tcb = Adapter->Send.CurrentTcb;
    Tcb->Mss = Mss;
    Tcb->Packet = Packet;
    Tcb->Flags = NV_TCB_LARGE_SEND;

    Adapter->TransmitPacket(Adapter, Tcb, SgList);

    ASSERT(Adapter->Send.TbdSlots >= Tcb->Slots);
    Adapter->Send.TbdSlots -= Tcb->Slots;

    Adapter->Send.CurrentTcb = NV_NEXT_TCB(Adapter, Tcb);

    return NDIS_STATUS_SUCCESS;
}

static
ULONG
NvNetGetChecksumInfo(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNDIS_PACKET Packet)
{
    ULONG Flags;
    NDIS_TCP_IP_CHECKSUM_PACKET_INFO ChecksumInfo;

    if (NDIS_GET_PACKET_PROTOCOL_TYPE(Packet) != NDIS_PROTOCOL_ID_TCP_IP)
        return 0;

    ChecksumInfo.Value = PtrToUlong(NDIS_PER_PACKET_INFO_FROM_PACKET(Packet,
                                                                     TcpIpChecksumPacketInfo));

    Flags = 0;
    if (ChecksumInfo.Transmit.NdisPacketChecksumV4)
    {
        if (ChecksumInfo.Transmit.NdisPacketTcpChecksum && Adapter->Offload.SendTcpChecksum)
        {
            Flags |= NV_TCB_CHECKSUM_TCP;
        }
        if (ChecksumInfo.Transmit.NdisPacketUdpChecksum && Adapter->Offload.SendUdpChecksum)
        {
            Flags |= NV_TCB_CHECKSUM_UDP;
        }
        if (ChecksumInfo.Transmit.NdisPacketIpChecksum && Adapter->Offload.SendIpChecksum)
        {
            Flags |= NV_TCB_CHECKSUM_IP;
        }
    }

    return Flags;
}

static
NDIS_STATUS
NvNetSendPacket(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNDIS_PACKET Packet,
    _In_ ULONG TotalLength)
{
    PSCATTER_GATHER_LIST SgList;
    SCATTER_GATHER_LIST LocalSgList;
    PNVNET_TCB Tcb;
    ULONG Flags;

    ASSERT(TotalLength <= Adapter->MaximumFrameSize);

    if (!Adapter->Send.TcbSlots)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Flags = NvNetGetChecksumInfo(Adapter, Packet);

    SgList = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo);

    if (SgList->NumberOfElements > NVNET_FRAGMENTATION_THRESHOLD)
    {
        if (!Adapter->Send.TbdSlots || !Adapter->Send.BufferList.Next)
        {
            return NDIS_STATUS_RESOURCES;
        }
        else
        {
            PNVNET_TX_BUFFER CoalesceBuffer;
            BOOLEAN Success;

            --Adapter->Send.TcbSlots;

            CoalesceBuffer = (PNVNET_TX_BUFFER)PopEntryList(&Adapter->Send.BufferList);

            NdisDprReleaseSpinLock(&Adapter->Send.Lock);

            Success = NvNetCopyPacket(Adapter, Packet, CoalesceBuffer);

            NdisDprAcquireSpinLock(&Adapter->Send.Lock);

            if (!Success || !Adapter->Send.TbdSlots || !(Adapter->Flags & NV_ACTIVE))
            {
                PushEntryList(&Adapter->Send.BufferList, &CoalesceBuffer->Link);

                ++Adapter->Send.TcbSlots;

                return NDIS_STATUS_RESOURCES;
            }

            Flags |= NV_TCB_COALESCE;

            LocalSgList.NumberOfElements = 1;
            LocalSgList.Elements[0].Address = CoalesceBuffer->PhysicalAddress;
            LocalSgList.Elements[0].Length = TotalLength;
            SgList = &LocalSgList;

            Tcb = Adapter->Send.CurrentTcb;
            Tcb->Buffer = CoalesceBuffer;
        }
    }
    else
    {
        if (SgList->NumberOfElements +
            (NVNET_MAXIMUM_FRAME_SIZE_JUMBO / (NV_MAXIMUM_SG_SIZE + 1)) > Adapter->Send.TbdSlots)
        {
            return NDIS_STATUS_RESOURCES;
        }

        --Adapter->Send.TcbSlots;

        Tcb = Adapter->Send.CurrentTcb;
    }

    Tcb->Packet = Packet;
    Tcb->Flags = Flags;

    Adapter->TransmitPacket(Adapter, Tcb, SgList);

    ASSERT(Adapter->Send.TbdSlots >= Tcb->Slots);
    Adapter->Send.TbdSlots -= Tcb->Slots;

    Adapter->Send.CurrentTcb = NV_NEXT_TCB(Adapter, Tcb);

    return NDIS_STATUS_PENDING;
}

/* FIXME: Use the proper send function (MiniportSendPackets) */
NDIS_STATUS
NTAPI
MiniportSend(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_PACKET Packet,
    _In_ UINT Flags)
{
    PNVNET_ADAPTER Adapter = (PNVNET_ADAPTER)MiniportAdapterContext;
    UINT TotalLength;
    NDIS_STATUS Status;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    NdisQueryPacketLength(Packet, &TotalLength);

    NdisDprAcquireSpinLock(&Adapter->Send.Lock);

    if (!(Adapter->Flags & NV_ACTIVE))
    {
        NdisDprReleaseSpinLock(&Adapter->Send.Lock);

        return NDIS_STATUS_FAILURE;
    }

    if (Adapter->Flags & NV_SEND_LARGE_SEND &&
        PtrToUlong(NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, TcpLargeSendPacketInfo)))
    {
        Status = NvNetSendPacketLargeSend(Adapter, Packet, TotalLength);
    }
    else
    {
        Status = NvNetSendPacket(Adapter, Packet, TotalLength);
    }

    NdisDprReleaseSpinLock(&Adapter->Send.Lock);

    return Status;
}
