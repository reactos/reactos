/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        datalink/arp.c
 * PURPOSE:     Address Resolution Protocol routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <arp.h>
#include <routines.h>
#include <neighbor.h>
#include <address.h>
#include <pool.h>
#include <lan.h>


PNDIS_PACKET PrepareARPPacket(
    USHORT HardwareType,
    USHORT ProtocolType,
    UCHAR LinkAddressLength,
    UCHAR ProtoAddressLength,
    PVOID SenderLinkAddress,
    PVOID SenderProtoAddress,
    PVOID TargetLinkAddress,
    PVOID TargetProtoAddress,
    USHORT Opcode)
/*
 * FUNCTION: Prepares an ARP packet
 * ARGUMENTS:
 *     HardwareType       = Hardware type (in network byte order)
 *     ProtocolType       = Protocol type (in network byte order)
 *     LinkAddressLength  = Length of link address fields
 *     ProtoAddressLength = Length of protocol address fields
 *     SenderLinkAddress  = Sender's link address
 *     SenderProtoAddress = Sender's protocol address
 *     TargetLinkAddress  = Target's link address (NULL if don't care)
 *     TargetProtoAddress = Target's protocol address
 *     Opcode             = ARP opcode (in network byte order)
 * RETURNS:
 *     Pointer to NDIS packet, NULL if there is not enough free resources
 */
{
    PNDIS_PACKET NdisPacket;
    PNDIS_BUFFER NdisBuffer;
    NDIS_STATUS NdisStatus;
    PARP_HEADER Header;
    PVOID DataBuffer;
    ULONG Size;

    TI_DbgPrint(MID_TRACE, ("Called.\n"));

    /* Prepare ARP packet */
    Size = MaxLLHeaderSize + sizeof(ARP_HEADER) + 
        2 * LinkAddressLength + /* Hardware address length */
        2 * ProtoAddressLength; /* Protocol address length */
    Size = MAX(Size, MinLLFrameSize);

    DataBuffer = ExAllocatePool(NonPagedPool, Size);
    if (!DataBuffer)
        return NULL;

    /* Allocate NDIS packet */
    NdisAllocatePacket(&NdisStatus, &NdisPacket, GlobalPacketPool);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        ExFreePool(DataBuffer);
        return NULL;
    }

    /* Allocate NDIS buffer for maximum link level header and ARP packet */
    NdisAllocateBuffer(&NdisStatus, &NdisBuffer, GlobalBufferPool,
        DataBuffer, Size);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NdisFreePacket(NdisPacket);
        ExFreePool(DataBuffer);
        return NULL;
    }

    /* Link NDIS buffer into packet */
    NdisChainBufferAtFront(NdisPacket, NdisBuffer);
    RtlZeroMemory(DataBuffer, Size);
    Header = (PARP_HEADER)((ULONG_PTR)DataBuffer + MaxLLHeaderSize);
    Header->HWType       = HardwareType;
    Header->ProtoType    = ProtocolType;
    Header->HWAddrLen    = LinkAddressLength;
    Header->ProtoAddrLen = ProtoAddressLength;
    Header->Opcode       = Opcode; /* Already swapped */
    DataBuffer = (PVOID)((ULONG_PTR)Header + sizeof(ARP_HEADER));

    /* Our hardware address */
    RtlCopyMemory(DataBuffer, SenderLinkAddress, LinkAddressLength);
    (ULONG_PTR)DataBuffer += LinkAddressLength;

    /* Our protocol address */
    RtlCopyMemory(DataBuffer, SenderProtoAddress, ProtoAddressLength);

    if (TargetLinkAddress) {
        (ULONG_PTR)DataBuffer += ProtoAddressLength;
        /* Target hardware address */
        RtlCopyMemory(DataBuffer, TargetLinkAddress, LinkAddressLength);
        (ULONG_PTR)DataBuffer += LinkAddressLength;
    } else
        /* Don't care about target hardware address */
        (ULONG_PTR)DataBuffer += (ProtoAddressLength + LinkAddressLength);

    /* Target protocol address */
    RtlCopyMemory(DataBuffer, TargetProtoAddress, ProtoAddressLength);

    return NdisPacket;
}


VOID ARPTransmitComplete(
    PVOID Context,
    PNDIS_PACKET NdisPacket,
    NDIS_STATUS NdisStatus)
/*
 * FUNCTION: ARP request transmit completion handler
 * ARGUMENTS:
 *     Context    = Pointer to context information (IP_INTERFACE)
 *     Packet     = Pointer to NDIS packet that was sent
 *     NdisStatus = NDIS status of operation
 * NOTES:
 *    This routine is called when an ARP request has been sent
 */
{
    TI_DbgPrint(MID_TRACE, ("Called.\n"));

    FreeNdisPacket(NdisPacket);
}


BOOLEAN ARPTransmit(
    PIP_ADDRESS Address,
    PNET_TABLE_ENTRY NTE)
/*
 * FUNCTION: Creates an ARP request and transmits it on a network
 * ARGUMENTS:
 *     Address = Pointer to IP address to resolve
 *     NTE     = Pointer to net table entru to use for transmitting request
 * RETURNS:
 *     TRUE if the request was successfully sent, FALSE if not
 */
{
    PIP_INTERFACE Interface;
    PNDIS_PACKET NdisPacket;
    UCHAR ProtoAddrLen;
    USHORT ProtoType;

    TI_DbgPrint(MID_TRACE, ("Called.\n"));

    Interface = NTE->Interface;

    switch (Address->Type) {
        case IP_ADDRESS_V4:
            ProtoType    = (USHORT)ETYPE_IPv4; /* IPv4 */
            ProtoAddrLen = 4;                  /* Length of IPv4 address */
            break;
        case IP_ADDRESS_V6:
            ProtoType    = (USHORT)ETYPE_IPv6; /* IPv6 */
            ProtoAddrLen = 16;                 /* Length of IPv6 address */
            break;
        default:
            /* Should not happen */
            return FALSE;
    }

    NdisPacket = PrepareARPPacket(
        WN2H(0x0001),                    /* FIXME: Ethernet only */
        ProtoType,                       /* Protocol type */
        (UCHAR)Interface->AddressLength, /* Hardware address length */
        (UCHAR)ProtoAddrLen,             /* Protocol address length */
        Interface->Address,              /* Sender's (local) hardware address */
        &NTE->Address->Address,          /* Sender's (local) protocol address */
        NULL,                            /* Don't care */
        &Address->Address,               /* Target's (remote) protocol address */
        ARP_OPCODE_REQUEST);             /* ARP request */

    PC(NdisPacket)->DLComplete = ARPTransmitComplete;

    (*Interface->Transmit)(Interface->Context, NdisPacket,
        MaxLLHeaderSize, NULL, LAN_PROTO_ARP);

    return TRUE;
}


VOID ARPReceive(
    PVOID Context,
    PIP_PACKET Packet)
/*
 * FUNCTION: Receives an ARP packet
 * ARGUMENTS:
 *     Context = Pointer to context information (IP_INTERFACE)
 *     Packet  = Pointer to packet
 */
{
    PARP_HEADER Header;
    PIP_ADDRESS Address;
    PVOID SenderHWAddress;
    PVOID SenderProtoAddress;
    PVOID TargetProtoAddress;
    PADDRESS_ENTRY ADE;
    PNEIGHBOR_CACHE_ENTRY NCE;
    PNDIS_PACKET NdisPacket;
    PIP_INTERFACE Interface = (PIP_INTERFACE)Context;

    TI_DbgPrint(MID_TRACE, ("Called.\n"));

    Header = (PARP_HEADER)Packet->Header;

    /* FIXME: Ethernet only */
    if (WN2H(Header->HWType) != 1)
        return;

    /* Check protocol type */
    if (Header->ProtoType != ETYPE_IPv4)
        return;

    SenderHWAddress    = (PVOID)((ULONG_PTR)Header + sizeof(ARP_HEADER));
    SenderProtoAddress = (PVOID)((ULONG_PTR)SenderHWAddress + Header->HWAddrLen);

    /* Check if we have the target protocol address */

    TargetProtoAddress = (PVOID)((ULONG_PTR)SenderProtoAddress +
        Header->ProtoAddrLen + Header->HWAddrLen);

    Address = AddrBuildIPv4(*(PULONG)(TargetProtoAddress));
    ADE = IPLocateADE(Address, ADE_UNICAST);
    if (!ADE) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return;
    }

    /* Check if we know the sender */

    AddrInitIPv4(Address, *(PULONG)(SenderProtoAddress));
    NCE = NBLocateNeighbor(Address);
    if (NCE) {
        DereferenceObject(Address);
        /* We know the sender. Update the hardware address 
           and state in our neighbor address cache */
        NBUpdateNeighbor(NCE, SenderHWAddress, NUD_REACHABLE);
    } else {
        /* The packet had our protocol address as target. The sender
           may want to communicate with us soon, so add his address
           to our address cache */
        NCE = NBAddNeighbor(Interface, Address, SenderHWAddress,
            Header->HWAddrLen, NUD_REACHABLE);
    }
    if (NCE)
        DereferenceObject(NCE)

    if (Header->Opcode != ARP_OPCODE_REQUEST)
        return;
    
    /* This is a request for our address. Swap the addresses and
       send an ARP reply back to the sender */
    NdisPacket = PrepareARPPacket(
        Header->HWType,                  /* Hardware type */
        Header->ProtoType,               /* Protocol type */
        (UCHAR)Interface->AddressLength, /* Hardware address length */
        (UCHAR)Header->ProtoAddrLen,     /* Protocol address length */
        Interface->Address,              /* Sender's (local) hardware address */
        &ADE->Address->Address,          /* Sender's (local) protocol address */
        SenderHWAddress,                 /* Target's (remote) hardware address */
        SenderProtoAddress,              /* Target's (remote) protocol address */
        ARP_OPCODE_REPLY);               /* ARP reply */
    if (NdisPacket) {
        PC(NdisPacket)->DLComplete = ARPTransmitComplete;
        (*Interface->Transmit)(Interface->Context, NdisPacket,
            MaxLLHeaderSize, SenderHWAddress, LAN_PROTO_ARP);
    }
}

/* EOF */
