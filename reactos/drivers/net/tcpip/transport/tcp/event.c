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

void TCPRecvNotify( PCONNECTION_ENDPOINT Connection, UINT Flags ) {
    int error = 0;
    NTSTATUS Status = 0;
    CHAR DataBuffer[1024];
    UINT BytesRead = 0, BytesTaken = 0;
    PTDI_IND_RECEIVE ReceiveHandler;
    PTDI_IND_DISCONNECT DisconnectHandler;
    PVOID HandlerContext;
    SOCKADDR Addr;

    TI_DbgPrint(MID_TRACE,("XX> Called\n"));
    
    do { 
	error = OskitTCPRecv( Connection->SocketContext,
			      &Addr,
			      DataBuffer,
			      1024,
			      &BytesRead,
			      Flags | OSK_MSG_DONTWAIT | OSK_MSG_PEEK );

	switch( error ) {
	case 0:
	    ReceiveHandler = Connection->AddressFile->ReceiveHandler;
	    HandlerContext = Connection->AddressFile->ReceiveHandlerContext;
	    
	    TI_DbgPrint(MID_TRACE,("Received %d bytes\n", BytesRead));
	    
	    if( Connection->AddressFile->RegisteredReceiveHandler ) 
		Status = ReceiveHandler( HandlerContext,
					 NULL,
					 TDI_RECEIVE_NORMAL,
					 BytesRead,
					 BytesRead,
					 &BytesTaken,
					 DataBuffer,
					 NULL );
	    else
		Status = STATUS_UNSUCCESSFUL;
	    
	    if( Status == STATUS_SUCCESS ) {
		OskitTCPRecv( Connection->SocketContext,
			      &Addr,
			      DataBuffer,
			      BytesTaken,
			      &BytesRead,
			      Flags | OSK_MSG_DONTWAIT );
	    }
	    break;

	case OSK_ESHUTDOWN:
	case OSK_ECONNRESET:
	    DisconnectHandler = Connection->AddressFile->DisconnectHandler;
	    HandlerContext = Connection->AddressFile->DisconnectHandlerContext;
	    
	    if( Connection->AddressFile->RegisteredDisconnectHandler )
		Status = DisconnectHandler( HandlerContext,
					    NULL,
					    0,
					    NULL,
					    0,
					    NULL,
					    (error == OSK_ESHUTDOWN) ? 
					    TDI_DISCONNECT_RELEASE :
					    TDI_DISCONNECT_ABORT );
	    else
		Status = STATUS_UNSUCCESSFUL;
	    break;
	    
	default:
	    assert( 0 );
	    break;
	}
    } while( error == 0 && BytesRead > 0 && BytesTaken > 0 );

    TI_DbgPrint(MID_TRACE,("XX> Leaving\n"));
}

void TCPCloseNotify( PCONNECTION_ENDPOINT Connection ) {
    TCPRecvNotify( Connection, 0 );
}

char *FlagNames[] = { "SEL_CONNECT",
		      "SEL_FIN",
		      "SEL_ACCEPT",
		      "SEL_OOB",
		      "SEL_READ",
		      "SEL_WRITE",
		       0 };
int FlagValues[]   = { SEL_CONNECT,
		       SEL_FIN,
		       SEL_ACCEPT,
		       SEL_OOB,
		       SEL_READ,
		       SEL_WRITE,
		       0 };

void TCPSocketState( void *ClientData,
		     void *WhichSocket,
		     void *WhichConnection,
		     OSK_UINT Flags,
		     OSK_UINT SocketState ) {
    int i;
    PCONNECTION_ENDPOINT Connection = 
	(PCONNECTION_ENDPOINT)WhichConnection;

    TI_DbgPrint(MID_TRACE,("TCPSocketState: (socket %x) %x %x\n",
			   WhichSocket, Flags, SocketState));

    for( i = 0; FlagValues[i]; i++ ) {
	if( Flags & FlagValues[i] ) 
	    TI_DbgPrint(MID_TRACE,("Flag %s\n", FlagNames[i]));
    }

    if( Flags & SEL_CONNECT ) 
	/* TCPConnectNotify( Connection ); */ ;
    if( Flags & SEL_FIN )
	TCPCloseNotify( Connection );
    if( Flags & SEL_ACCEPT )
	/* TCPAcceptNotify( Connection ); */ ;
    if( Flags & SEL_OOB ) 
	TCPRecvNotify( Connection, MSG_OOB );
    if( Flags & SEL_WRITE )
	/* TCPSendNotify( Connection ); */ ;
    if( Flags & SEL_READ )
	TCPRecvNotify( Connection, 0 );
}

void TCPPacketSendComplete( PVOID Context,
			    NDIS_STATUS NdisStatus,
			    DWORD BytesSent ) {
    TI_DbgPrint(MID_TRACE,("called\n"));
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

    OskitTCPGetAddress( WhichSocket,
			&LocalAddress.Address.IPv4Address,
			&LocalPort,
			&RemoteAddress.Address.IPv4Address,
			&RemotePort );

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

