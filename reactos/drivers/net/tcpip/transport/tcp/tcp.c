/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/tcp.c
 * PURPOSE:     Transmission Control Protocol
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"


LONG TCP_IPIdentification = 0;
static BOOLEAN TCPInitialized = FALSE;
static NPAGED_LOOKASIDE_LIST TCPSegmentList;
LIST_ENTRY SleepingThreadsList;
FAST_MUTEX SleepingThreadsLock;

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
    TI_DbgPrint(MID_TRACE,("Sending packet %d (%d) to oskit\n", 
			   IPPacket->TotalSize,
			   IPPacket->HeaderSize));
    
    OskitTCPReceiveDatagram( IPPacket->Header, 
			     IPPacket->TotalSize, 
			     IPPacket->HeaderSize );
}

/* event.c */
int TCPSocketState( void *ClientData,
		    void *WhichSocket,
		    void *WhichConnection,
		    OSK_UINT NewState );

int TCPPacketSend( void *ClientData,
		   OSK_PCHAR Data,
		   OSK_UINT Len );

POSK_IFADDR TCPFindInterface( void *ClientData,
			      OSK_UINT AddrType,
			      OSK_UINT FindType,
			      OSK_SOCKADDR *ReqAddr );

void *TCPMalloc( void *ClientData,
		 OSK_UINT bytes, OSK_PCHAR file, OSK_UINT line );
void TCPFree( void *ClientData,
	      void *data, OSK_PCHAR file, OSK_UINT line );

int TCPSleep( void *ClientData, void *token, int priority, char *msg,
	      int tmio );

void TCPWakeup( void *ClientData, void *token );

OSKITTCP_EVENT_HANDLERS EventHandlers = {
    NULL,             /* Client Data */
    TCPSocketState,   /* SocketState */
    TCPPacketSend,    /* PacketSend */
    TCPFindInterface, /* FindInterface */
    TCPMalloc,        /* Malloc */
    TCPFree,          /* Free */
    TCPSleep,         /* Sleep */
    TCPWakeup         /* Wakeup */
};

NTSTATUS TCPStartup(VOID)
/*
 * FUNCTION: Initializes the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    ExInitializeFastMutex( &SleepingThreadsLock );
    InitializeListHead( &SleepingThreadsList );    

    RegisterOskitTCPEventHandlers( &EventHandlers );
    InitOskitTCP();
    
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

NTSTATUS TCPTranslateError( int OskitError ) {
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    switch( OskitError ) {
    case 0: Status = STATUS_SUCCESS; break;
    case OSK_EADDRNOTAVAIL:
    case OSK_EAFNOSUPPORT: Status = STATUS_INVALID_CONNECTION; break;
    case OSK_ECONNREFUSED:
    case OSK_ECONNRESET: Status = STATUS_REMOTE_NOT_LISTENING; break;
    case OSK_EINPROGRESS:
    case OSK_EAGAIN: Status = STATUS_PENDING; break;
    default: Status = STATUS_INVALID_CONNECTION; break;
    }

    TI_DbgPrint(MID_TRACE,("Error %d -> %x\n", OskitError, Status));
    return Status;
}

#if 0
NTSTATUS TCPBind
( PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo ) {
    NTSTATUS Status;
    PCONNECTION_ENDPOINT Connection = Request->Handle.ConnectionContext;
    SOCKADDR_IN AddressToConnect;
    PIP_ADDRESS LocalAddress;
    USHORT LocalPort;

    TI_DbgPrint(MID_TRACE,("Called\n"));

    Status = AddrBuildAddress
	((PTA_ADDRESS)ConnInfo->LocalAddress,
	 &LocalAddress,
	 &LocalPort);

    AddressToBind.sin_family = AF_INET;
    memcpy( &AddressToBind.sin_addr, 
	    &LocalAddress->Address.IPv4Address,
	    sizeof(AddressToBind.sin_addr) );
    AddressToBind.sin_port = LocalPort;

    Status = OskitTCPBind( Connection->SocketContext,
			   Connection,
			   &AddressToBind, 
			   sizeof(AddressToBind));

    TI_DbgPrint(MID_TRACE,("Leaving %x\n", Status));

    return Status;
}
#endif

NTSTATUS TCPConnect
( PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo ) {
    KIRQL OldIrql;
    NTSTATUS Status;
    SOCKADDR_IN AddressToConnect = { 0 }, AddressToBind = { 0 };
    PCONNECTION_ENDPOINT Connection = Request->Handle.ConnectionContext;
    PIP_ADDRESS RemoteAddress;
    USHORT RemotePort;
    PTDI_BUCKET Bucket;

    DbgPrint("TCPConnect: Called\n");

    Bucket = ExAllocatePool( NonPagedPool, sizeof(*Bucket) );
    if( !Bucket ) return STATUS_NO_MEMORY;

    KeAcquireSpinLock(&Connection->Lock, &OldIrql);

    /* Freed in TCPSocketState */
    Bucket->Request = *Request;
    InsertHeadList( &Connection->ConnectRequest, &Bucket->Entry );

    Status = AddrBuildAddress
	((PTA_ADDRESS)ConnInfo->RemoteAddress,
	 &RemoteAddress,
	 &RemotePort);

    DbgPrint("Connecting to address %x:%x\n",
	     RemoteAddress->Address.IPv4Address,
	     RemotePort);

    if (!NT_SUCCESS(Status)) {
	TI_DbgPrint(MID_TRACE, ("Could not AddrBuildAddress in TCPConnect\n"));
	KeReleaseSpinLock(&Connection->Lock, OldIrql);
	return Status;
    }
    
    AddressToConnect.sin_family = AF_INET;
    AddressToBind = AddressToConnect;

    OskitTCPBind( Connection->SocketContext,
		  Connection,
		  &AddressToBind,
		  sizeof(AddressToBind) );

    memcpy( &AddressToConnect.sin_addr, 
	    &RemoteAddress->Address.IPv4Address,
	    sizeof(AddressToConnect.sin_addr) );
    AddressToConnect.sin_port = RemotePort;
    KeReleaseSpinLock(&Connection->Lock, OldIrql);
    
    Status = OskitTCPConnect(Connection->SocketContext,
			     Connection,
			     &AddressToConnect, 
			     sizeof(AddressToConnect));

    if( Status == OSK_EINPROGRESS || Status == STATUS_SUCCESS ) 
	return STATUS_PENDING;
    else
	return Status;
}

NTSTATUS TCPClose
( PTDI_REQUEST Request ) {
    PCONNECTION_ENDPOINT Connection;
    NTSTATUS Status;
    Connection = Request->Handle.ConnectionContext;
    
    TI_DbgPrint(MID_TRACE,("TCPClose started\n"));

    Status = TCPTranslateError( OskitTCPClose( Connection->SocketContext ) );
    
    TI_DbgPrint(MID_TRACE,("TCPClose finished %x\n", Status));

    return Status;
}

NTSTATUS TCPListen
( PTDI_REQUEST Request,
  UINT Backlog ) {
    PCONNECTION_ENDPOINT Connection;

    Connection = Request->Handle.ConnectionContext;

    return TCPTranslateError( OskitTCPListen( Connection->SocketContext,
					      Backlog ) );
}

NTSTATUS TCPAccept
( PTDI_REQUEST Request,
  VOID **NewSocketContext ) {
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS TCPReceiveData
( PTDI_REQUEST Request,
  PNDIS_BUFFER Buffer,
  ULONG ReceiveLength,
  ULONG ReceiveFlags,
  PULONG BytesReceived ) {
    KIRQL OldIrql;
    PCONNECTION_ENDPOINT Connection;
    PCHAR DataBuffer;
    UINT DataLen, Received = 0;
    NTSTATUS Status;
    PTDI_BUCKET Bucket;

    TI_DbgPrint(MID_TRACE,("Called for %d bytes\n", ReceiveLength));

    Connection = Request->Handle.ConnectionContext;

    KeAcquireSpinLock(&Connection->Lock, &OldIrql);

    NdisQueryBuffer( Buffer, &DataBuffer, &DataLen );

    TI_DbgPrint(MID_TRACE,("TCP>|< Got an MDL %x (%x:%d)\n", Buffer, DataBuffer, DataLen));

    Status = TCPTranslateError
	( OskitTCPRecv
	  ( Connection->SocketContext,
	    DataBuffer,
	    DataLen,
	    &Received,
	    ReceiveFlags ) );

    TI_DbgPrint(MID_TRACE,("OskitTCPReceive: %x, %d\n", Status, Received));

    /* Keep this request around ... there was no data yet */
    if( Status == STATUS_PENDING || 
	(Status == STATUS_SUCCESS && Received == 0) ) {
	/* Freed in TCPSocketState */
	Bucket = ExAllocatePool( NonPagedPool, sizeof(*Bucket) );
	if( !Bucket ) return STATUS_NO_MEMORY;
	
	Bucket->Request = *Request;
	InsertHeadList( &Connection->ReceiveRequest, &Bucket->Entry );
	Status = STATUS_PENDING;
    } else {
	TI_DbgPrint(MID_TRACE,("Got status %x, bytes %d\n", Status, Received));
    }

    KeReleaseSpinLock(&Connection->Lock, OldIrql);

    TI_DbgPrint(MID_TRACE,("Status %x\n", Status));

    return Status;
}

NTSTATUS TCPSendData
( PTDI_REQUEST Request,
  PNDIS_BUFFER Buffer,
  ULONG DataSize,
  ULONG Flags,
  PULONG DataUsed ) {
    KIRQL OldIrql;
    NTSTATUS Status;
    PCONNECTION_ENDPOINT Connection;
    PCHAR BufferData;
    ULONG PacketSize;

    Connection = Request->Handle.ConnectionContext;

    KeAcquireSpinLock(&Connection->Lock, &OldIrql);

    NdisQueryBuffer( Buffer, &BufferData, &PacketSize );
    
    TI_DbgPrint(MID_TRACE,("Connection = %x\n", Connection));
    TI_DbgPrint(MID_TRACE,("Connection->SocketContext = %x\n",
			   Connection->SocketContext));

    OskitDumpBuffer( BufferData, PacketSize );

    Status = OskitTCPSend( Connection->SocketContext, 
			   BufferData, PacketSize, (PUINT)DataUsed, 0 );

    KeReleaseSpinLock(&Connection->Lock, OldIrql);

    return Status;
}

VOID TCPTimeout(VOID) { 
    static int Times = 0;
    if( (Times++ % 5) == 0 ) {
	TimerOskitTCP();
    }
}

/* EOF */
