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

typedef VOID 
(*PTCP_COMPLETION_ROUTINE)( PVOID Context, NTSTATUS Status, ULONG Count );

int TCPSocketState(void *ClientData,
		   void *WhichSocket, 
		   void *WhichConnection,
		   OSK_UINT NewState ) {
    PCONNECTION_ENDPOINT Connection = WhichConnection;
    PTCP_COMPLETION_ROUTINE Complete;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;

    TI_DbgPrint(MID_TRACE,("Called: NewState %x\n", NewState));

    if( !Connection ) {
	TI_DbgPrint(MID_TRACE,("Socket closing.\n"));
	return 0;
    }

    if( (NewState & SEL_CONNECT) && 
	!(Connection->State & SEL_CONNECT) ) {
	while( !IsListEmpty( &Connection->ConnectRequest ) ) {
	    Connection->State |= SEL_CONNECT;
	    Entry = RemoveHeadList( &Connection->ConnectRequest );
	    Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
	    Complete = Bucket->Request.RequestNotifyObject;
	    TI_DbgPrint(MID_TRACE,
			("Completing Connect Request %x\n", Bucket->Request));
	    Complete( Bucket->Request.RequestContext, STATUS_SUCCESS, 0 );
	    /* Frees the bucket allocated in TCPConnect */
	    ExFreePool( Bucket );
	}
    } else if( NewState & SEL_READ ) {
	while( !IsListEmpty( &Connection->ReceiveRequest ) ) {
	    PIRP Irp;
	    OSK_UINT RecvLen = 0, Received = 0;
	    OSK_PCHAR RecvBuffer = 0;
	    PMDL Mdl;
	    NTSTATUS Status;

	    Entry = RemoveHeadList( &Connection->ReceiveRequest );
	    Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
	    Complete = Bucket->Request.RequestNotifyObject;

	    TI_DbgPrint(MID_TRACE,
			("Readable, Completing read request %x\n", 
			 Bucket->Request));

	    Irp = Bucket->Request.RequestContext;
	    Mdl = Irp->MdlAddress;

	    TI_DbgPrint(MID_TRACE,
			("Getting the user buffer from %x\n", Mdl));

	    NdisQueryBuffer( Mdl, &RecvBuffer, &RecvLen );

	    TI_DbgPrint(MID_TRACE,
			("Reading %d bytes to %x\n", RecvLen, RecvBuffer));

	    Status = TCPTranslateError
		( OskitTCPRecv( Connection->SocketContext,
				RecvBuffer,
				RecvLen,
				&Received,
				0 ) );

	    TI_DbgPrint(MID_TRACE,("TCP Bytes: %d\n", Received));

	    if( Status == STATUS_SUCCESS && Received != 0 ) {
		TI_DbgPrint(MID_TRACE,("Received %d bytes with status %x\n",
				       Received, Status));
		
		TI_DbgPrint(MID_TRACE,
			    ("Completing Receive Request: %x\n", 
			     Bucket->Request));
		
		Complete( Bucket->Request.RequestContext, 
			  STATUS_SUCCESS, 
			  Received );
	    } else {
		InsertHeadList( &Connection->ReceiveRequest,
				&Bucket->Entry );
	    }
	}
    }

    return 0;
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

    DbgPrint("OSKIT SENDING PACKET *** %x:%d -> %x:%d\n",
	     LocalAddress.Address.IPv4Address,
	     LocalPort,
	     RemoteAddress.Address.IPv4Address,
	     RemotePort);

    NCE = RouterGetRoute( &RemoteAddress, NULL );

    if( !NCE ) return OSK_EADDRNOTAVAIL;

    GetInterfaceIPv4Address(NCE->Interface, 
			    ADE_UNICAST, 
			    &LocalAddress.Address.IPv4Address );

    KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );

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
    KeLowerIrql( OldIrql );

    if( !NT_SUCCESS(NdisStatus) ) return OSK_EINVAL;
    else return 0;
}

