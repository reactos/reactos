/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/tcp.c
 * PURPOSE:     Transmission Control Protocol
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <roscfg.h>
#include <limits.h>
#include <tcpip.h>
#include <tcp.h>
#include <pool.h>
#include <address.h>
#include <datagram.h>
#include <checksum.h>
#include <routines.h>
#include <oskittcp.h>

LONG TCP_IPIdentification = 0;
static BOOLEAN TCPInitialized = FALSE;
static NPAGED_LOOKASIDE_LIST TCPSegmentList;

VOID TCPReceive(PNET_TABLE_ENTRY NTE, PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives and queues TCP data
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
 *     IPPacket = Pointer to an IP packet that was received
 * NOTES:
 *     This is the low level interface for receiving TCP data
 */
{
    PCHAR BufferData = exAllocatePool( NonPagedPool, IPPacket->TotalSize );

    if( BufferData ) {
	TI_DbgPrint(MID_TRACE,("Sending packet %d (%d) to oskit\n", 
			       IPPacket->TotalSize,
			       IPPacket->HeaderSize));

	memcpy( BufferData, IPPacket->Header, IPPacket->HeaderSize );
	memcpy( BufferData + IPPacket->HeaderSize, IPPacket->Data,
		IPPacket->TotalSize - IPPacket->HeaderSize );
	
	OskitTCPReceiveDatagram( BufferData, 
				 IPPacket->TotalSize, 
				 IPPacket->HeaderSize );

	/*exFreePool( BufferData );*/
    }
}

/* event.c */
int TCPBindEvent( void *ClientData,
		  void *WhichSocket, 
		  void *WhichConnection,
		  LPSOCKADDR address, 
		  OSK_UINT addrlen,
		  OSK_UINT reuseport );

int TCPPacketSend( void *ClientData,
		   void *WhichSocket,
		   void *WhichConnection,
		   OSK_PCHAR Data,
		   OSK_UINT Len );

OSKITTCP_EVENT_HANDLERS EventHandlers = {
    NULL, /* Client Data */
    NULL, /* SocketDataAvailable */
    NULL, /* SocketConnectIndication */
    NULL, /* SocketCloseIndication */
    NULL, /* SocketPendingConnectIndication */
    NULL, /* SocketResetIndication */
    TCPPacketSend, /* PacketSend */
    TCPBindEvent /* Bind */
};

NTSTATUS TCPStartup(VOID)
/*
 * FUNCTION: Initializes the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    InitOskitTCP();
    RegisterOskitTCPEventHandlers( &EventHandlers );
    
    /* Register this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_TCP, TCPReceive);
    
    ExInitializeNPagedLookasideList(
	&TCPSegmentList,                /* Lookaside list */
	NULL,                           /* Allocate routine */
	NULL,                           /* Free routine */
	0,                              /* Flags */
	sizeof(TCP_SEGMENT),            /* Size of each entry */
	TAG('T','C','P','S'),           /* Tag */
	0);                             /* Depth */
    
    TCPInitialized = TRUE;
    
    return STATUS_SUCCESS;
}


NTSTATUS TCPShutdown(VOID)
/*
 * FUNCTION: Shuts down the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    if (!TCPInitialized)
	return STATUS_SUCCESS;
    
    /* Deregister this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_TCP, NULL);
    
    ExDeleteNPagedLookasideList(&TCPSegmentList);
    
    TCPInitialized = FALSE;

    DeinitOskitTCP();
    
    return STATUS_SUCCESS;
}

NTSTATUS TCPConnect
( PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo ) {
    KIRQL OldIrql;
    NTSTATUS Status;
    SOCKADDR_IN AddressToConnect;
    PCONNECTION_ENDPOINT Connection;

    Connection = Request->Handle.ConnectionContext;

    KeAcquireSpinLock(&Connection->Lock, &OldIrql);
    
    TI_DbgPrint(MID_TRACE, ("AF: %08x\n", 
			    Connection->AddressFile));
    TI_DbgPrint(MID_TRACE, ("ADE: %08x\n", 
			    Connection->AddressFile->ADE));
    TI_DbgPrint(MID_TRACE, ("ADEA: %08x\n", 
			    Connection->AddressFile->ADE->Address));

    PIP_ADDRESS RemoteAddress;
    USHORT RemotePort;

    Status = AddrBuildAddress(
	(PTA_ADDRESS)(&((PTRANSPORT_ADDRESS)ConnInfo->RemoteAddress)->
		      Address[0]),
	&RemoteAddress,
	&RemotePort);

    if (!NT_SUCCESS(Status)) {
	TI_DbgPrint(MID_TRACE, ("Could not AddrBuildAddress in TCPConnect\n"));
	KeReleaseSpinLock(&Connection->Lock, OldIrql);
	return Status;
    }
    
    AddressToConnect.sin_family = AF_INET;

    memcpy( &AddressToConnect.sin_addr, 
	    &RemoteAddress->Address.IPv4Address,
	    sizeof(AddressToConnect.sin_addr) );
    AddressToConnect.sin_port = RemotePort;
    KeReleaseSpinLock(&Connection->Lock, OldIrql);

    Status = OskitTCPConnect(Connection->SocketContext,
			     Connection,
			     &AddressToConnect, 
			     sizeof(AddressToConnect));

    if( Status == 0 ) return STATUS_PENDING; else return Status;
}

NTSTATUS TCPListen
( PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo ) {
}

NTSTATUS TCPReceiveData
( PTDI_REQUEST Request,
  PNDIS_BUFFER Buffer,
  ULONG ReceiveLength,
  ULONG ReceiveFlags,
  PULONG BytesReceived ) {
}

NTSTATUS TCPSendData
( PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PNDIS_BUFFER Buffer,
  ULONG DataSize ) {
    PCONNECTION_ENDPOINT Connection;
    PCHAR BufferData;
    ULONG PacketSize;
    int error;

    NdisQueryBuffer( Buffer, &BufferData, &PacketSize );
    
    Connection = Request->Handle.ConnectionContext;
    /* XXX flags */
    error = OskitTCPSend( Connection->SocketContext, 
			  BufferData, PacketSize, 0 );
    if( error ) return STATUS_UNSUCCESSFUL;
    else return STATUS_SUCCESS;
}

NTSTATUS TCPTimeout(VOID) { 
    TimerOskitTCP();
}

/* EOF */
