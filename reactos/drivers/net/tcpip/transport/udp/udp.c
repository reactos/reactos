/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/udp/udp.c
 * PURPOSE:     User Datagram Protocol routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <udp.h>
#include <routines.h>
#include <transmit.h>
#include <datagram.h>
#include <checksum.h>
#include <address.h>
#include <pool.h>


BOOLEAN UDPInitialized = FALSE;


NTSTATUS AddUDPHeaderIPv4(
    PDATAGRAM_SEND_REQUEST SendRequest,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Adds an IPv4 and UDP header to an IP packet
 * ARGUMENTS:
 *     SendRequest  = Pointer to send request
 *     LocalAddress = Pointer to our local address
 *     LocalPort    = The port we send this datagram from
 *     IPPacket     = Pointer to IP packet
 * RETURNS:
 *     Status of operation
 */
{
    PIPv4_HEADER IPHeader;
    PUDP_HEADER UDPHeader;
    PVOID Header;
	ULONG BufferSize;
    NDIS_STATUS NdisStatus;
    PNDIS_BUFFER HeaderBuffer;

	BufferSize = MaxLLHeaderSize + sizeof(IPv4_HEADER) + sizeof(UDP_HEADER);
    Header     = PoolAllocateBuffer(BufferSize);

    if (!Header) {
        TI_DbgPrint(MIN_TRACE, ("Cannot allocate memory for packet headers.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate NDIS buffer for maximum Link level, IP and UDP header */
    NdisAllocateBuffer(&NdisStatus,
                       &HeaderBuffer,
                       GlobalBufferPool,
                       Header,
                       BufferSize);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TI_DbgPrint(MIN_TRACE, ("Cannot allocate NDIS buffer for packet headers. NdisStatus = (0x%X)\n", NdisStatus));
        PoolFreeBuffer(Header);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Chain header at front of NDIS packet */
    NdisChainBufferAtFront(IPPacket->NdisPacket, HeaderBuffer);
    
	IPPacket->Header     = (PVOID)((ULONG_PTR)Header + MaxLLHeaderSize);
    IPPacket->HeaderSize = 20;

    /* Build IPv4 header */
    IPHeader = (PIPv4_HEADER)IPPacket->Header;
    /* Version = 4, Length = 5 DWORDs */
    IPHeader->VerIHL = 0x45;
    /* Normal Type-of-Service */
    IPHeader->Tos = 0;
    /* Length of header and data */
    IPHeader->TotalLength = WH2N((USHORT)IPPacket->TotalSize);
    /* Identification */
    IPHeader->Id = 0;
    /* One fragment at offset 0 */
    IPHeader->FlagsFragOfs = 0;
    /* Time-to-Live is 128 */
    IPHeader->Ttl = 128;
    /* User Datagram Protocol */
    IPHeader->Protocol = IPPROTO_UDP;
    /* Checksum is 0 (for later calculation of this) */
    IPHeader->Checksum = 0;
    /* Source address */
    IPHeader->SrcAddr = LocalAddress->Address.IPv4Address;
    /* Destination address. FIXME: IPv4 only */
    IPHeader->DstAddr = SendRequest->RemoteAddress->Address.IPv4Address;

    /* Build UDP header */
    UDPHeader = (PUDP_HEADER)((ULONG_PTR)IPHeader + sizeof(IPv4_HEADER));
    /* Port values are already big-endian values */
    UDPHeader->SourcePort = LocalPort;
    UDPHeader->DestPort   = SendRequest->RemotePort;
    /* FIXME: Calculate UDP checksum and put it in UDP header */
    UDPHeader->Checksum   = 0;
    /* Length of UDP header and data */
    UDPHeader->Length     = WH2N((USHORT)IPPacket->TotalSize - IPPacket->HeaderSize);

    return STATUS_SUCCESS;
}


NTSTATUS BuildUDPPacket(
    PVOID Context,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PIP_PACKET *IPPacket)
/*
 * FUNCTION: Builds an UDP packet
 * ARGUMENTS:
 *     Context      = Pointer to context information (DATAGRAM_SEND_REQUEST)
 *     LocalAddress = Pointer to our local address
 *     LocalPort    = The port we send this datagram from
 *     IPPacket     = Address of pointer to IP packet
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;
    PIP_PACKET Packet;
    NDIS_STATUS NdisStatus;
    PDATAGRAM_SEND_REQUEST SendRequest = (PDATAGRAM_SEND_REQUEST)Context;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Prepare packet */
    Packet = PoolAllocateBuffer(sizeof(IP_PACKET));
    if (!Packet) {
        TI_DbgPrint(MIN_TRACE, ("Cannot allocate memory for packet.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Packet, sizeof(IP_PACKET));
    Packet->RefCount   = 1;
    Packet->TotalSize  = sizeof(IPv4_HEADER) +
                         sizeof(UDP_HEADER)  +
                         SendRequest->BufferSize;

    /* Allocate NDIS packet */
    NdisAllocatePacket(&NdisStatus, &Packet->NdisPacket, GlobalPacketPool);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TI_DbgPrint(MIN_TRACE, ("Cannot allocate NDIS packet. NdisStatus = (0x%X)\n", NdisStatus));
        PoolFreeBuffer(Packet);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    switch (SendRequest->RemoteAddress->Type) {
    case IP_ADDRESS_V4:
        Status = AddUDPHeaderIPv4(SendRequest, LocalAddress, LocalPort, Packet);
        break;
    case IP_ADDRESS_V6:
        /* FIXME: Support IPv6 */
        TI_DbgPrint(MIN_TRACE, ("IPv6 UDP datagrams are not supported.\n"));
    default:
        Status = STATUS_UNSUCCESSFUL;
        break;
    }
    if (!NT_SUCCESS(Status)) {
        TI_DbgPrint(MIN_TRACE, ("Cannot add UDP header. Status = (0x%X)\n", Status));
        NdisFreePacket(Packet->NdisPacket);
        PoolFreeBuffer(Packet);
        return Status;
    }

    /* Chain data after header */
    NdisChainBufferAtBack(Packet->NdisPacket, SendRequest->Buffer);

    *IPPacket = Packet;

    return STATUS_SUCCESS;
}


VOID DeliverUDPData(
    PADDRESS_FILE AddrFile,
    PIP_ADDRESS Address,
    PIP_PACKET IPPacket,
    UINT DataSize)
/*
 * FUNCTION: Delivers UDP data to a user
 * ARGUMENTS:
 *     AddrFile = Address file to deliver data to
 *     Address  = Remote address the packet came from
 *     IPPacket = Pointer to IP packet to deliver
 *     DataSize = Number of bytes in data area
 * NOTES:
 *     If there is a receive request, then we copy the data to the
 *     buffer supplied by the user and complete the receive request.
 *     If no suitable receive request exists, then we call the event
 *     handler if it exists, otherwise we drop the packet.
 */
{
    KIRQL OldIrql;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

    if (!IsListEmpty(&AddrFile->ReceiveQueue)) {
        PLIST_ENTRY CurrentEntry;
        PDATAGRAM_RECEIVE_REQUEST Current;
        BOOLEAN Found;

        /* Search receive request list to find a match */
        Found = FALSE;
        CurrentEntry = AddrFile->ReceiveQueue.Flink;
        while ((CurrentEntry != &AddrFile->ReceiveQueue) && (!Found)) {
            Current = CONTAINING_RECORD(CurrentEntry, DATAGRAM_RECEIVE_REQUEST, ListEntry);
            if (!Current->RemoteAddress)
                Found = TRUE;
            else if (AddrIsEqual(Address, Current->RemoteAddress))
                Found = TRUE;

            if (Found) {
                /* FIXME: Maybe we should check if the buffer of this
                   receive request is large enough and if not, search
                   for another. Also a 'best fit' strategy could be used. */

                /* Remove the request from the queue */
                RemoveEntryList(&Current->ListEntry);
                AddrFile->RefCount--;
                break;
            }
            CurrentEntry = CurrentEntry->Flink;
        }

        KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

        if (Found) {
            /* Copy the data into buffer provided by the user */
            CopyBufferToBufferChain(Current->Buffer,
                                    0,
                                    IPPacket->Data,
                                    DataSize);

            /* Complete the receive request */
            (*Current->Complete)(Current->Context, STATUS_SUCCESS, DataSize);

            /* Finally free the receive request */
            if (Current->RemoteAddress)
                PoolFreeBuffer(Current->RemoteAddress);
            PoolFreeBuffer(Current);
        }
    } else {
        KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

        /* FIXME: Call event handler */
        TI_DbgPrint(MAX_TRACE, ("Calling receive event handler.\n"));
    }

    TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS UDPSendDatagram(
    PTDI_REQUEST Request,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PNDIS_BUFFER Buffer,
    ULONG DataSize)
/*
 * FUNCTION: Sends an UDP datagram to a remote address
 * ARGUMENTS:
 *     Request   = Pointer to TDI request
 *     ConnInfo  = Pointer to connection information
 *     Buffer    = Pointer to NDIS buffer with data
 *     DataSize  = Size in bytes of data to be sent
 * RETURNS:
 *     Status of operation
 */
{
    return DGSendDatagram(Request,
                          ConnInfo,
                          Buffer,
                          DataSize,
                          BuildUDPPacket);
}


NTSTATUS UDPReceiveDatagram(
    PTDI_REQUEST Request,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PNDIS_BUFFER Buffer,
    ULONG ReceiveLength,
    ULONG ReceiveFlags,
    PTDI_CONNECTION_INFORMATION ReturnInfo,
    PULONG BytesReceived)
/*
 * FUNCTION: Attempts to receive an UDP datagram from a remote address
 * ARGUMENTS:
 *     Request       = Pointer to TDI request
 *     ConnInfo      = Pointer to connection information
 *     Buffer        = Pointer to NDIS buffer chain to store received data
 *     ReceiveLength = Maximum size to use of buffer, 0 if all can be used
 *     ReceiveFlags  = Receive flags (None, Normal, Peek)
 *     ReturnInfo    = Pointer to structure for return information
 *     BytesReceive  = Pointer to structure for number of bytes received
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     This is the high level interface for receiving UDP datagrams
 */
{
    return DGReceiveDatagram(Request,
                             ConnInfo,
                             Buffer,
                             ReceiveLength,
                             ReceiveFlags,
                             ReturnInfo,
                             BytesReceived);
}


VOID UDPReceive(
    PNET_TABLE_ENTRY NTE,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives and queues a UDP datagram
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
 *     IPPacket = Pointer to an IP packet that was received
 * NOTES:
 *     This is the low level interface for receiving UDP datagrams. It strips
 *     the UDP header from a packet and delivers the data to anyone that wants it
 */
{
    AF_SEARCH SearchContext;
    PIPv4_HEADER IPv4Header;
    PADDRESS_FILE AddrFile;
    PUDP_HEADER UDPHeader;
    PIP_ADDRESS DstAddress;
    UINT DataSize, i;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    switch (IPPacket->Type) {
    /* IPv4 packet */
    case IP_ADDRESS_V4:
        IPv4Header = IPPacket->Header;
        DstAddress = &IPPacket->DstAddr;
        break;

    /* IPv6 packet */
    case IP_ADDRESS_V6:
        TI_DbgPrint(MIN_TRACE, ("Discarded IPv6 UDP datagram (%i bytes).\n", IPPacket->TotalSize));

        /* FIXME: IPv6 is not supported */
        return;

    default:
        return;
    }

    UDPHeader = (PUDP_HEADER)IPPacket->Data;

    /* FIXME: Calculate and validate UDP checksum */

    /* Sanity checks */
    i = WH2N(UDPHeader->Length);
    if ((i < sizeof(UDP_HEADER)) || (i > IPPacket->TotalSize - IPPacket->Position)) {
        /* Incorrect or damaged packet received, discard it */
        TI_DbgPrint(MIN_TRACE, ("Incorrect or damaged UDP packet received.\n"));
        return;
    }

    DataSize = i - sizeof(UDP_HEADER);

    /* Go to UDP data area */
    (ULONG_PTR)IPPacket->Data += sizeof(UDP_HEADER);

    /* Locate a receive request on destination address file object
       and deliver the packet if one is found. If there is no receive
       request on the address file object, call the associated receive
       handler. If no receive handler is registered, drop the packet */

    AddrFile = AddrSearchFirst(DstAddress,
                               UDPHeader->DestPort,
                               IPPROTO_UDP,
                               &SearchContext);
    if (AddrFile) {
        do {
            DeliverUDPData(AddrFile,
                           DstAddress,
                           IPPacket,
                           DataSize);
        } while ((AddrFile = AddrSearchNext(&SearchContext)) != NULL);
    } else {
        /* There are no open address files that will take this datagram */
        /* FIXME: IPv4 only */
        TI_DbgPrint(MID_TRACE, ("Cannot deliver IPv4 UDP datagram to address (0x%X).\n",
            DN2H(DstAddress->Address.IPv4Address)));

        /* FIXME: Send ICMP reply */
    }
    TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS UDPStartup(
    VOID)
/*
 * FUNCTION: Initializes the UDP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    RtlZeroMemory(&UDPStats, sizeof(UDP_STATISTICS));

    /* Register this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_UDP, UDPReceive);

    UDPInitialized = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS UDPShutdown(
    VOID)
/*
 * FUNCTION: Shuts down the UDP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    if (!UDPInitialized)
        return STATUS_SUCCESS;

    /* Deregister this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_UDP, NULL);

    return STATUS_SUCCESS;
}

/* EOF */
