/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/event.c
 * PURPOSE:     Transmission Control Protocol -- Events from oskittcp
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <roscfg.h>
#include <limits.h>
#include <tcpip.h>
#include <tcp.h>
#include <pool.h>
#include <route.h>
#include <router.h>
#include <address.h>
#include <neighbor.h>
#include <datagram.h>
#include <checksum.h>
#include <routines.h>
#include <oskittcp.h>

extern ULONG TCP_IPIdentification;

int TCPBindEvent( void *ClientData,
		  void *WhichSocket, 
		  void *WhichConnection,
		  LPSOCKADDR address, OSK_UINT addrlen,
		  OSK_UINT reuseport ) {
    PCONNECTION_ENDPOINT Connection = 
	(PCONNECTION_ENDPOINT)WhichConnection;
    /* Select best interface */
    LPSOCKADDR_IN addr_in = (struct sockaddr_in *)address;
    KIRQL OldIrql;
    PNEIGHBOR_CACHE_ENTRY NCE = 0;
    IP_ADDRESS RemoteAddress, LocalAddress;
    USHORT RemotePort, LocalPort;
    
    KeAcquireSpinLock( &Connection->Lock, &OldIrql );

    addr_in->sin_family = htons(addr_in->sin_family);

    TI_DbgPrint(MID_TRACE,("Binding %08x (sin_family = %d)\n", address,
			   addr_in->sin_family));

    if( addr_in->sin_family != AF_INET ) return OSK_EPROTONOSUPPORT;

    RemoteAddress.Type = LocalAddress.Type = IP_ADDRESS_V4;
    CP;
    OskitTCPGetAddress( Connection->SocketContext,
			&LocalAddress.Address.IPv4Address,
			&LocalPort,
			&RemoteAddress.Address.IPv4Address,
			&RemotePort );
    CP;

    NCE = RouterGetRoute(&RemoteAddress, NULL);

    if( !NCE ) return OSK_EADDRNOTAVAIL;

    GetInterfaceIPv4Address(NCE->Interface, 
			    ADE_UNICAST, 
			    &LocalAddress.Address.IPv4Address );

    /* XXX arty IPv4 */
    if( addr_in ) 
	memcpy( &addr_in->sin_addr, 
		&LocalAddress.Address.IPv4Address, 
		sizeof( addr_in->sin_addr ) );

    addr_in->sin_port = htons(LocalPort);

    CP;
    OskitTCPSetAddress( Connection->SocketContext,
			&LocalAddress.Address.IPv4Address,
			LocalPort,
			&RemoteAddress.Address.IPv4Address,
			RemotePort );
    CP;

    KeReleaseSpinLock( &Connection->Lock, OldIrql );

    return 0;
}

void TCPPacketSendComplete( PVOID Context,
			    NDIS_STATUS NdisStatus,
			    DWORD BytesSent ) {
    PDATAGRAM_SEND_REQUEST Send = (PDATAGRAM_SEND_REQUEST)Context;
    if( Send->Packet.NdisPacket )
	FreeNdisPacket( Send->Packet.NdisPacket );
    exFreePool( Send );
}

NTSTATUS AddHeaderIPv4(
    PDATAGRAM_SEND_REQUEST SendRequest,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PIP_ADDRESS RemoteAddress,
    USHORT RemotePort) {
/*
 * FUNCTION: Adds an IPv4 and TCP header to an IP packet
 * ARGUMENTS:
 *     SendRequest      = Pointer to send request
 *     Connection       = Pointer to connection endpoint
 *     LocalAddress     = Pointer to our local address
 *     LocalPort        = The port we send this segment from
 *     IPPacket         = Pointer to IP packet
 * RETURNS:
 *     Status of operation
 */
    PIPv4_HEADER IPHeader;
    PIP_PACKET IPPacket;
    PVOID Header;
    NDIS_STATUS NdisStatus;
    PNDIS_BUFFER HeaderBuffer;
    PCHAR BufferContent;
    ULONG BufferSize;
    ULONG PayloadBufferSize;
    
    IPPacket = &SendRequest->Packet;

    BufferSize = MaxLLHeaderSize + sizeof(IPv4_HEADER);
    Header     = exAllocatePool(NonPagedPool, BufferSize);
    if (!Header)
	return STATUS_INSUFFICIENT_RESOURCES;
    
    TI_DbgPrint(MAX_TRACE, ("Allocated %d bytes for headers at 0x%X.\n", BufferSize, Header));
    
    NdisQueryPacketLength( IPPacket->NdisPacket, &PayloadBufferSize );

    /* Allocate NDIS buffer for maximum Link level, IP and TCP header */
    NdisAllocateBuffer(&NdisStatus,
		       &HeaderBuffer,
		       GlobalBufferPool,
		       Header,
		       BufferSize);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
	exFreePool(Header);
	TI_DbgPrint(MAX_TRACE, ("Error from NDIS: %08x\n", NdisStatus));
	return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    /* Chain header at front of NDIS packet */
    NdisChainBufferAtFront(IPPacket->NdisPacket, HeaderBuffer);
    IPPacket->HeaderSize = 20;
    IPPacket->ContigSize = BufferSize;
    IPPacket->TotalSize  = IPPacket->HeaderSize + PayloadBufferSize;
    IPPacket->Header     = (PVOID)((ULONG_PTR)Header + MaxLLHeaderSize);
    IPPacket->Flags      = 0;
    
    /* Build IPv4 header */
    IPHeader = (PIPv4_HEADER)IPPacket->Header;
    /* Version = 4, Length = 5 DWORDs */
    IPHeader->VerIHL = 0x45;
    /* Normal Type-of-Service */
    IPHeader->Tos = 0;
    /* Length of header and data */
    IPHeader->TotalLength = WH2N((USHORT)IPPacket->TotalSize);
    /* Identification */
    IPHeader->Id = WH2N((USHORT)InterlockedIncrement(&TCP_IPIdentification));
    /* One fragment at offset 0 */
    IPHeader->FlagsFragOfs = WH2N((USHORT)IPv4_DF_MASK);
    /* Time-to-Live is 128 */
    IPHeader->Ttl = 128;
    /* Transmission Control Protocol */
    IPHeader->Protocol = IPPROTO_TCP;
    /* Checksum is 0 (for later calculation of this) */
    IPHeader->Checksum = 0;
    /* Source address */
    IPHeader->SrcAddr = LocalAddress->Address.IPv4Address;
    /* Destination address. FIXME: IPv4 only */
    IPHeader->DstAddr = RemoteAddress->Address.IPv4Address;

    return STATUS_SUCCESS;
}

int TCPPacketSend(void *ClientData,
		  void *WhichSocket, 
		  void *WhichConnection,
		  OSK_PCHAR data,
		  OSK_UINT len ) {
    PADDRESS_FILE AddrFile;
    PNDIS_BUFFER NdisPacket;
    NDIS_STATUS NdisStatus;
    KIRQL OldIrql;
    PDATAGRAM_SEND_REQUEST SendRequest;
    PNEIGHBOR_CACHE_ENTRY NCE = 0;
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)WhichConnection;
    IP_ADDRESS RemoteAddress, LocalAddress;
    USHORT RemotePort, LocalPort;
    PULONG AckNumber = (PULONG)data;

    TI_DbgPrint(MID_TRACE,("TCP OUTPUT:\n"));
    OskitDumpBuffer( data, len );

    SendRequest = 
	(PDATAGRAM_SEND_REQUEST)
	exAllocatePool( NonPagedPool, sizeof( DATAGRAM_SEND_REQUEST ) );
    /* if( !SendRequest || !Connection ) return OSK_EINVAL; */

    RemoteAddress.Type = LocalAddress.Type = IP_ADDRESS_V4;
    CP;
    OskitTCPGetAddress( WhichSocket,
			&LocalAddress.Address.IPv4Address,
			&LocalPort,
			&RemoteAddress.Address.IPv4Address,
			&RemotePort );
    CP;

    NCE = RouterGetRoute( &RemoteAddress, NULL );

    if( !NCE ) return OSK_EADDRNOTAVAIL;

    GetInterfaceIPv4Address(NCE->Interface, 
			    ADE_UNICAST, 
			    &LocalAddress.Address.IPv4Address );

    if( Connection ) 
	KeAcquireSpinLock( &Connection->Lock, &OldIrql );

    NdisStatus = 
	AllocatePacketWithBuffer( &SendRequest->PacketToSend, data, len );
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
	TI_DbgPrint(MAX_TRACE, ("Error from NDIS: %08x\n", NdisStatus));
	goto end;
    }

    SendRequest->Packet.NdisPacket = SendRequest->PacketToSend;

    SendRequest->Complete = TCPPacketSendComplete;
    SendRequest->Context = Connection;
    SendRequest->RemoteAddress = RemoteAddress;
    SendRequest->RemotePort = RemotePort;
    NdisQueryPacketLength( SendRequest->Packet.NdisPacket,
			   &SendRequest->BufferSize );

    AddHeaderIPv4( SendRequest, 
		   &LocalAddress, 
		   LocalPort,
		   &RemoteAddress,
		   RemotePort );

    if( Connection ) 
	DGTransmit( Connection->AddressFile, SendRequest );
    else
	DbgPrint("Transmit called without connection.\n");

end:
    if( Connection )
	KeReleaseSpinLock( &Connection->Lock, OldIrql );

    if( !NT_SUCCESS(NdisStatus) ) return OSK_EINVAL;
    else return 0;
}

