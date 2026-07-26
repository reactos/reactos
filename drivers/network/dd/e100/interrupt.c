/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Interrupt handling
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
VOID
EeStartReceive(
    _In_ PE100_ADAPTER Adapter)
{
    PE100_RX_CONTEXT RxContext;
    UCHAR RuState;

    /*
     * Since we also check the FXP_RFD_STATUS_RNR bit to prevent the restart latency
     * we cannot assume the RNR state here.
     * Thus, make sure the RU has entered the no resource condition.
     */
    RuState = FXP_SCB_RUS(CSR_READ_8(Adapter, FXP_CSR_SCB_RUSCUS));
    TRACE("Starting RU %02X\n", RuState);
    if (RuState != FXP_SCB_RUS_NORESOURCES)
        return;

    ASSERT(!IsListEmpty(&Adapter->RxContextList));
    RxContext = CONTAINING_RECORD(Adapter->RxContextList.Flink,
                                  E100_RX_CONTEXT,
                                  ListEntry);
    EeCommandUnitExecuteCommandAddr(Adapter, FXP_SCB_COMMAND_RU_START, RxContext->RfdPhys);
}

static
VOID
EeReleaseRfd(
    _In_ PE100_ADAPTER Adapter,
    _In_ PE100_RX_CONTEXT RxContext)
{
    PFXP_RFD Rfd;

    RxContext->Flags = 0;

    Rfd = RxContext->Rfd;
    Rfd->Header.Status = 0;
    Rfd->Header.Command = htole16(FXP_RFD_CONTROL_EL);
    le32enc(&Rfd->Header.LinkAddress, 0xFFFFFFFF);
    le32enc(&Rfd->RbdAddress, 0xFFFFFFFF);
    Rfd->ActualSize = 0; // Clear the F and EOF bits

    NdisFlushBuffer(RxContext->RfdMdl, TRUE);

    /* Attach the RFD to the end of receive chain */
    if (!IsListEmpty(&Adapter->RxContextList))
    {
        PE100_RX_CONTEXT LastRxContext;
        PFXP_RFD LastRfd;

        LastRxContext = CONTAINING_RECORD(Adapter->RxContextList.Blink,
                                          E100_RX_CONTEXT,
                                          ListEntry);
        LastRfd = LastRxContext->Rfd;

        EE_WRITE_BARRIER();
        le32enc(&LastRfd->Header.LinkAddress, RxContext->RfdPhys);
        NdisFlushBuffer(LastRxContext->RfdMdl, TRUE);

        EE_WRITE_BARRIER();
        LastRfd->Header.Command = 0;
        NdisFlushBuffer(LastRxContext->RfdMdl, TRUE);
    }

    InsertTailList(&Adapter->RxContextList, &RxContext->ListEntry);
}

VOID
NTAPI
MiniportReturnPacket(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_PACKET Packet)
{
    PE100_ADAPTER Adapter = MiniportAdapterContext;
    PE100_RX_CONTEXT RxContext;

    RxContext = *E100_RX_CONTEXT_FROM_PACKET(Packet);

    NdisAcquireSpinLock(&Adapter->ReceiveLock);

    ASSERT(Adapter->RxPending > 0);
    --Adapter->RxPending;

    EeReleaseRfd(Adapter, RxContext);

    NdisReleaseSpinLock(&Adapter->ReceiveLock);
}

static
VOID
EeIndicateReceivePackets(
    _In_ PE100_ADAPTER Adapter,
    _In_reads_(PacketsToIndicate) PNDIS_PACKET* ReceiveArray,
    _In_ ULONG PacketsToIndicate)
{
    PNDIS_PACKET Packet;
    PE100_RX_CONTEXT RxContext;
    ULONG i;

    NdisDprReleaseSpinLock(&Adapter->ReceiveLock);

    NdisMIndicateReceivePacket(Adapter->AdapterHandle,
                               ReceiveArray,
                               PacketsToIndicate);

    NdisDprAcquireSpinLock(&Adapter->ReceiveLock);

    for (i = 0; i < PacketsToIndicate; ++i)
    {
        Packet = ReceiveArray[i];
        RxContext = *E100_RX_CONTEXT_FROM_PACKET(Packet);

        /* Reuse the RFD immediately */
        if (RxContext->Flags & E100_RX_FLAG_RECLAIM)
        {
            EeReleaseRfd(Adapter, RxContext);
        }
    }
}

static
USHORT
EeIpv4PseudoHeaderChecksum(
    _In_ ULONG SourceAddress,
    _In_ ULONG DestinationAddress,
    _In_ ULONG TcpSegmentLength)
{
    ULONG Crc;

    Crc = TcpSegmentLength;
    Crc += (SourceAddress >> 16) & 0xFFFF;
    Crc += (SourceAddress >> 0) & 0xFFFF;
    Crc += (DestinationAddress >> 16) & 0xFFFF;
    Crc += (DestinationAddress >> 0) & 0xFFFF;

    Crc = ((Crc >> 16) & 0xFFFF) + (Crc & 0xFFFF);

    if (Crc > 0xFFFF)
        Crc -= 0xFFFF;

    return Crc;
}

static
VOID
EeHandleRxChecksum82559(
    _In_ PE100_ADAPTER Adapter,
    _In_ PUCHAR PacketData,
    _In_ ULONG PacketLength,
    _Out_ PNDIS_PACKET Packet)
{
    NDIS_TCP_IP_CHECKSUM_PACKET_INFO CsInfo;
    PIPv4_HEADER IpHeader;
    USHORT HeaderAndPayloadLength, Crc;
    PETH_HEADER EthHeader;

    CsInfo.Value = 0;

    if (PacketLength < (E100_ETHERNET_HEADER_SIZE + sizeof(*IpHeader)))
        goto Exit;

    EthHeader = (PVOID)PacketData;
    if (EthHeader->PayloadType != htons(ETHERTYPE_IP))
        goto Exit;

    IpHeader = (PVOID)(PacketData + E100_ETHERNET_HEADER_SIZE);
    if (((IpHeader->VersionLength & 0xF0) >> 4) != 4)
        goto Exit;

    if (IpHeader->Offset & htons(IP_MF | IP_OFFMASK))
        goto Exit;

    if (!((IpHeader->Protocol == IPPROTO_TCP && Adapter->Offload.ReceiveTcpChecksum) ||
          (IpHeader->Protocol == IPPROTO_UDP && Adapter->Offload.ReceiveUdpChecksum)))
    {
        goto Exit;
    }

    HeaderAndPayloadLength = ntohs(IpHeader->TotalLength) - ((IpHeader->VersionLength & 0x0F) << 2);

    /* The checksum word is appended at the end of the packet */
    Crc = be16dec(PacketData + PacketLength);

    Crc = EeIpv4PseudoHeaderChecksum(IpHeader->Source,
                                     IpHeader->Destination,
                                     htons(Crc + HeaderAndPayloadLength + IpHeader->Protocol));

    if (IpHeader->Protocol == IPPROTO_TCP)
    {
        if (Crc == 0xFFFF)
            CsInfo.Receive.NdisPacketTcpChecksumSucceeded = 1;
        else
            CsInfo.Receive.NdisPacketTcpChecksumFailed = 1;
    }
    else
    {
        if (Crc == 0xFFFF)
            CsInfo.Receive.NdisPacketUdpChecksumSucceeded = 1;
        else
            CsInfo.Receive.NdisPacketUdpChecksumFailed = 1;
    }

Exit:
    NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, TcpIpChecksumPacketInfo) = UlongToPtr(CsInfo.Value);
}

static
VOID
EeHandleRxChecksum82550(
    _In_ PE100_ADAPTER Adapter,
    _In_ USHORT RfdStatus,
    _In_ UCHAR ParserStatus,
    _In_ UCHAR ChecksumStatus,
    _Out_ PNDIS_PACKET Packet)
{
    NDIS_TCP_IP_CHECKSUM_PACKET_INFO CsInfo;

    CsInfo.Value = 0;

    if (RfdStatus & FXP_RFD_STATUS_PARSE_OK)
    {
        if (Adapter->Offload.ReceiveIpChecksum)
        {
            if (ChecksumStatus & FXP_RFDX_CS_IP_CSUM_BIT_VALID)
            {
                if (ChecksumStatus & FXP_RFDX_CS_IP_CSUM_VALID)
                    CsInfo.Receive.NdisPacketIpChecksumSucceeded = 1;
                else
                    CsInfo.Receive.NdisPacketIpChecksumFailed = 1;
            }
        }

        if (ChecksumStatus & FXP_RFDX_CS_TCPUDP_CSUM_BIT_VALID)
        {
            if ((ParserStatus & FXP_RFDX_P_CSUM_PROTOCOL_MASK) == FXP_RFDX_P_TCP_PACKET)
            {
                if (Adapter->Offload.ReceiveTcpChecksum)
                {
                    if (ChecksumStatus & FXP_RFDX_CS_TCPUDP_CSUM_VALID)
                        CsInfo.Receive.NdisPacketTcpChecksumSucceeded = 1;
                    else
                        CsInfo.Receive.NdisPacketTcpChecksumFailed = 1;
                }
            }
            else if ((ParserStatus & FXP_RFDX_P_CSUM_PROTOCOL_MASK) == FXP_RFDX_P_UDP_PACKET)
            {
                if (Adapter->Offload.ReceiveUdpChecksum)
                {
                    if (ChecksumStatus & FXP_RFDX_CS_TCPUDP_CSUM_VALID)
                        CsInfo.Receive.NdisPacketUdpChecksumSucceeded = 1;
                    else
                        CsInfo.Receive.NdisPacketUdpChecksumFailed = 1;
                }
            }
        }
    }

    NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, TcpIpChecksumPacketInfo) = UlongToPtr(CsInfo.Value);
}

static
VOID
EeHandleRxVlan(
    _In_ USHORT RfdStatus,
    _In_ USHORT RfdVlanId,
    _Out_ PNDIS_PACKET Packet)
{
    NDIS_PACKET_8021Q_INFO VlanTag;

    VlanTag.Value = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, Ieee8021QInfo);

    if (RfdStatus & FXP_RFD_STATUS_VLAN)
    {
        USHORT Control = ntohs(RfdVlanId);

        VlanTag.TagHeader.VlanId = Control & 0xFFF;
        VlanTag.TagHeader.CanonicalFormatId = (Control >> 12) & 1;
        VlanTag.TagHeader.UserPriority = (Control >> 13) & 7;
    }
    else
    {
        VlanTag.Value = NULL;
    }

    NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, Ieee8021QInfo) = VlanTag.Value;
}

static
BOOLEAN
EeHandleRxReceivedFrames(
    _In_ PE100_ADAPTER Adapter,
    _Out_ PBOOLEAN OutOfBuffers)
{
    BOOLEAN CallAgain = TRUE;
    ULONG PacketsToIndicate = 0;
    PNDIS_PACKET ReceiveArray[E100_RECEIVE_ARRAY_SIZE];

    while (PacketsToIndicate < RTL_NUMBER_OF(ReceiveArray))
    {
        PE100_RX_CONTEXT RxContext;
        PFXP_RFD Rfd;
        USHORT Status;
        ULONG PacketLength;
        PNDIS_PACKET Packet;

        ASSERT(!IsListEmpty(&Adapter->RxContextList));
        RxContext = CONTAINING_RECORD(Adapter->RxContextList.Flink,
                                      E100_RX_CONTEXT,
                                      ListEntry);

        /* RFD headers are in cached memory */
        NdisFlushBuffer(RxContext->RfdMdl, FALSE);

        Rfd = RxContext->Rfd;

        Status = letoh16(Rfd->Header.Status);
        if (!(Status & FXP_RFD_STATUS_C))
        {
            CallAgain = FALSE;
            break;
        }

        NT_VERIFY(RemoveHeadList(&Adapter->RxContextList) != NULL);

        if (Status & FXP_RFD_STATUS_RNR)
        {
            *OutOfBuffers = TRUE;
        }

        if (!(Status & FXP_RFD_STATUS_OK))
        {
            /*
             * We enable saving of bad packets on the 82557 in order to receive VLAN frames.
             * However, this requires additional checks.
             */
            if (Adapter->Flags & E100_FLAG_SAVE_BAD_PACKETS)
            {
                if (Status & (FXP_RFD_STATUS_OVERRUN |
                              FXP_RFD_STATUS_RNR |
                              FXP_RFD_STATUS_ALIGN |
                              FXP_RFD_STATUS_CRC))
                {
                    EeReleaseRfd(Adapter, RxContext);
                    continue;
                }
            }
            else
            {
                EeReleaseRfd(Adapter, RxContext);
                continue;
            }
        }

        PacketLength = letoh16(Rfd->ActualSize) & FXP_RFD_FRAME_LENGTH_MASK;

        if (((Adapter->Offload.ReceiveTcpChecksum) ||
             (Adapter->Offload.ReceiveUdpChecksum)) &&
            (Adapter->Flags & E100_FLAG_82559_RXCSUM))
        {
            /* Omit the checksum word */
            ASSERT(PacketLength >= 2);
            PacketLength -= 2;
        }

        TRACE("RX packet (len %lu)\n", PacketLength);

        NdisAdjustBufferLength(RxContext->ReceiveBufferMdl, PacketLength);

        /* Receive buffer is a part of RFD */
        NdisFlushBuffer(RxContext->ReceiveBufferMdl, FALSE);

        Packet = RxContext->Packet;

        if (Adapter->Flags & E100_FLAG_EXT_RFA)
        {
            /* Check VLAN tag stripping */
            if ((Adapter->Flags & E100_FLAG_VLAN_TAGGING) ||
                (Adapter->Flags & E100_FLAG_PACKET_PRIORITY))
            {
                EeHandleRxVlan(Status, Rfd->Ex.VlanId, Packet);
            }

            if (Adapter->Offload.ReceiveTcpChecksum ||
                Adapter->Offload.ReceiveUdpChecksum ||
                Adapter->Offload.ReceiveIpChecksum)
            {
                EeHandleRxChecksum82550(Adapter,
                                        Status,
                                        Rfd->Ex.ParserStatus,
                                        Rfd->Ex.ChecksumStatus,
                                        Packet);
            }
        }
        else
        {
            if (Adapter->Offload.ReceiveTcpChecksum ||
                Adapter->Offload.ReceiveUdpChecksum)
            {
                ULONG RfdSize = RTL_SIZEOF_THROUGH_FIELD(FXP_RFD, Size);

                EeHandleRxChecksum82559(Adapter,
                                        (PUCHAR)((ULONG_PTR)RxContext->Rfd + RfdSize),
                                        PacketLength,
                                        Packet);
            }
        }

        ReceiveArray[PacketsToIndicate++] = Packet;

        if (IsListEmpty(&Adapter->RxContextList))
        {
            RxContext->Flags |= E100_RX_FLAG_RECLAIM;
            NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_RESOURCES);

            CallAgain = FALSE;
            break;
        }
        else
        {
            NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_SUCCESS);

            ++Adapter->RxPending;
        }
    }

    /* Pass the packets up */
    if (PacketsToIndicate)
    {
        EeIndicateReceivePackets(Adapter, ReceiveArray, PacketsToIndicate);
    }

    return CallAgain;
}

static
VOID
EeHandleRx(
    _In_ PE100_ADAPTER Adapter,
    _In_ UCHAR InterruptStatus)
{
    ULONG i;
    BOOLEAN OutOfBuffers = FALSE;

    NdisDprAcquireSpinLock(&Adapter->ReceiveLock);

    for (i = 0; i < E100_RECEIVE_PROCESSING_LIMIT; ++i)
    {
        if (!EeHandleRxReceivedFrames(Adapter, &OutOfBuffers))
            break;
    }

    if (OutOfBuffers || (InterruptStatus & FXP_SCB_STATACK_RNR))
    {
        NdisDprAcquireSpinLock(&Adapter->SendLock);
        EeStartReceive(Adapter);
        NdisDprReleaseSpinLock(&Adapter->SendLock);
    }

    NdisDprReleaseSpinLock(&Adapter->ReceiveLock);
}

static
VOID
EeHandleTxCompletedFrames(
    _In_ PE100_ADAPTER Adapter,
    _Inout_ PLIST_ENTRY SendReadyList)
{
    PE100_TX_CONTEXT TxContext;

    for (TxContext = Adapter->TxFirst; Adapter->TxPending > 0; TxContext = TxContext->Next)
    {
        PFXP_CB_TRANSMIT Tcb = TxContext->Tcb;

        if (!(Tcb->Header.Status & letoh16(FXP_CB_STATUS_C)))
            break;

        InsertTailList(SendReadyList, E100_LIST_ENTRY_FROM_PACKET(TxContext->Packet));

        E100_RELEASE_TX_CONTEXT(Adapter, TxContext, Tcb);
    }

    Adapter->TxFirst = TxContext;
}

static
VOID
EeHandleTx(
    _In_ PE100_ADAPTER Adapter)
{
    LIST_ENTRY SendReadyList;

    TRACE("Handle TX\n");

    InitializeListHead(&SendReadyList);

    NdisDprAcquireSpinLock(&Adapter->SendLock);

    EeHandleTxCompletedFrames(Adapter, &SendReadyList);

    if (!IsListEmpty(&Adapter->SendQueueList))
    {
        EeSendPackets(Adapter, NULL, 0);
    }

    NdisDprReleaseSpinLock(&Adapter->SendLock);

    while (!IsListEmpty(&SendReadyList))
    {
        PLIST_ENTRY Entry = RemoveHeadList(&SendReadyList);

        TRACE("Complete TX packet %p\n", E100_PACKET_FROM_LIST_ENTRY(Entry));

        NdisMSendComplete(Adapter->AdapterHandle,
                          E100_PACKET_FROM_LIST_ENTRY(Entry),
                          NDIS_STATUS_SUCCESS);
    }
}

VOID
NTAPI
MiniportHandleInterrupt(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PE100_ADAPTER Adapter = MiniportAdapterContext;
    UCHAR InterruptStatus;
    ULONG IoLimit;

    if (!Adapter->AdapterActive)
        return;

    IoLimit = E100_INTERRUPT_PROCESSING_LIMIT;
    InterruptStatus = Adapter->InterruptStatus;

    TRACE("DPC Enter %02X\n", InterruptStatus);

    while (TRUE)
    {
        /* Handling transmit interrupts */
        if (InterruptStatus & (FXP_SCB_STATACK_CXTNO | FXP_SCB_STATACK_CNA))
        {
            EeHandleTx(Adapter);
        }

        /* Handling receive interrupts */
        if (InterruptStatus & (FXP_SCB_STATACK_FR | FXP_SCB_STATACK_RNR))
        {
            EeHandleRx(Adapter, InterruptStatus);
        }

        /* Limit in order to avoid doing too much work at DPC level */
        if (!--IoLimit)
            break;

        /* Check if new events have occurred */
        InterruptStatus = CSR_READ_8(Adapter, FXP_CSR_SCB_STATACK);
        if (InterruptStatus == 0xFF || InterruptStatus == 0)
            break;

        /* Acknowledge the events */
        CSR_WRITE_8(Adapter, FXP_CSR_SCB_STATACK, InterruptStatus);

        TRACE("DPC Continue %02X\n", InterruptStatus);
    }

    /* Reenable interrupts (clear the FXP_SCB_INTR_DISABLE bit) */
    CSR_WRITE_8(Adapter, FXP_CSR_SCB_INTRCNTL, 0);
}

VOID
NTAPI
MiniportIsr(
    _Out_ PBOOLEAN InterruptRecognized,
    _Out_ PBOOLEAN QueueMiniportHandleInterrupt,
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PE100_ADAPTER Adapter = MiniportAdapterContext;
    ULONG InterruptStatus;

    /*
     * Read both interrupt registers at once. This eliminates having
     * to call NdisMSynchronizeWithInterrupt() when re-enabling device interrupts
     * in MiniportHandleInterrupt().
     */
    InterruptStatus = CSR_READ_32(Adapter, FXP_CSR_SCB_RUSCUS);

    /* Interrupts have been disabled previously */
    if (InterruptStatus & (FXP_SCB_INTR_DISABLE << 24))
        goto NotOurs;

    /* Extract the FXP_CSR_SCB_STATACK bits */
    InterruptStatus = (InterruptStatus >> 8) & 0xFF;
    if (InterruptStatus == 0xFF || InterruptStatus == 0)
        goto NotOurs;

    TRACE("ISR %02lX\n", InterruptStatus);

    /* Disable further interrupts */
    CSR_WRITE_8(Adapter, FXP_CSR_SCB_INTRCNTL, FXP_SCB_INTR_DISABLE);

    /* Acknowledge interrupts */
    CSR_WRITE_8(Adapter, FXP_CSR_SCB_STATACK, InterruptStatus);

    Adapter->InterruptStatus = InterruptStatus;

    *InterruptRecognized = TRUE;
    *QueueMiniportHandleInterrupt = TRUE;
    return;

NotOurs:
    *InterruptRecognized = FALSE;
    *QueueMiniportHandleInterrupt = FALSE;
}
