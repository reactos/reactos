/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/event.c
 * PURPOSE:     Transmission Control Protocol -- Events from oskittcp
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

extern ULONG TCP_IPIdentification;
extern LIST_ENTRY SleepingThreadsList;
extern FAST_MUTEX SleepingThreadsLock;
extern RECURSIVE_MUTEX TCPLock;

int TCPSocketState(void *ClientData,
		   void *WhichSocket, 
		   void *WhichConnection,
		   OSK_UINT NewState ) {
    PCONNECTION_ENDPOINT Connection = WhichConnection;
    PTCP_COMPLETION_ROUTINE Complete;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;

    TI_DbgPrint(MID_TRACE,("Called: NewState %x (Conn %x)\n", 
			   NewState, Connection));

    if( !Connection ) {
	TI_DbgPrint(MID_TRACE,("Socket closing.\n"));
	return 0;
    }

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

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
	    PoolFreeBuffer( Bucket );
	}
    } else if( (NewState & SEL_READ) || (NewState & SEL_FIN) ) {
	TI_DbgPrint(MID_TRACE,("Readable (or closed): irp list %s\n",
			       IsListEmpty(&Connection->ReceiveRequest) ?
			       "empty" : "nonempty"));

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

	    if( (NewState & SEL_FIN) && !RecvLen ) {
		Status = STATUS_END_OF_FILE;
		Received = 0;
	    } else {
		TI_DbgPrint(MID_TRACE, ("Connection: %x\n", Connection));
		TI_DbgPrint
		    (MID_TRACE, 
		     ("Connection->SocketContext: %x\n", 
		      Connection->SocketContext));
		TI_DbgPrint(MID_TRACE, ("RecvBuffer: %x\n", RecvBuffer));

		Status = TCPTranslateError
		    ( OskitTCPRecv( Connection->SocketContext,
				    RecvBuffer,
				    RecvLen,
				    &Received,
				    0 ) );
	    }

	    TI_DbgPrint(MID_TRACE,("TCP Bytes: %d\n", Received));

	    if( Status == STATUS_SUCCESS && Received != 0 ) {
		TI_DbgPrint(MID_TRACE,("Received %d bytes with status %x\n",
				       Received, Status));
		
		TI_DbgPrint(MID_TRACE,
			    ("Completing Receive Request: %x\n", 
			     Bucket->Request));

		Complete( Bucket->Request.RequestContext,
			  STATUS_SUCCESS, Received );
	    } else if( Status == STATUS_PENDING || 
		       (Status == STATUS_SUCCESS && Received == 0) ) {
		InsertHeadList( &Connection->ReceiveRequest,
				&Bucket->Entry );
		break;
	    } else {
		TI_DbgPrint(MID_TRACE,
			    ("Completing Receive request: %x %x\n",
			     Bucket->Request, Status));
		Complete( Bucket->Request.RequestContext, Status, 0 );
	    }
	}
    } 

    TcpipRecursiveMutexLeave( &TCPLock );

    return 0;
}

void TCPPacketSendComplete( PVOID Context,
			    PNDIS_PACKET NdisPacket,
			    NDIS_STATUS NdisStatus ) {
    TI_DbgPrint(MID_TRACE,("called %x\n", NdisPacket));
    FreeNdisPacket(NdisPacket);
    TI_DbgPrint(MID_TRACE,("done\n"));
}

#define STRINGIFY(x) #x

int TCPPacketSend(void *ClientData, OSK_PCHAR data, OSK_UINT len ) {
    NTSTATUS Status;
    NDIS_STATUS NdisStatus;
    ROUTE_CACHE_NODE *RCN;
    IP_PACKET Packet = { 0 };
    IP_ADDRESS RemoteAddress, LocalAddress;
    PIPv4_HEADER Header;

    TI_DbgPrint(MID_TRACE,("TCP OUTPUT (%x:%d):\n", data, len));
    OskitDumpBuffer( data, len );

    if( *data == 0x45 ) { /* IPv4 */
	Header = (PIPv4_HEADER)data;
	LocalAddress.Type = IP_ADDRESS_V4;
	LocalAddress.Address.IPv4Address = Header->SrcAddr;
	RemoteAddress.Type = IP_ADDRESS_V4;
	RemoteAddress.Address.IPv4Address = Header->DstAddr;
    } else {
	DbgPrint("Don't currently handle IPv6\n");
	KeBugCheck(4);
    }

    RemoteAddress.Type = LocalAddress.Type = IP_ADDRESS_V4;

    DbgPrint("OSKIT SENDING PACKET *** %x -> %x\n",
	     LocalAddress.Address.IPv4Address,
	     RemoteAddress.Address.IPv4Address);
    
    Status = RouteGetRouteToDestination( &RemoteAddress, NULL, &RCN );
    
    if( !NT_SUCCESS(Status) || !RCN ) return OSK_EADDRNOTAVAIL;

    NdisStatus = AllocatePacketWithBuffer( &Packet.NdisPacket, NULL, 
					   MaxLLHeaderSize + len );
    
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
	TI_DbgPrint(MAX_TRACE, ("Error from NDIS: %08x\n", NdisStatus));
	return STATUS_NO_MEMORY;
    }

    GetDataPtr( Packet.NdisPacket, MaxLLHeaderSize, 
		(PCHAR *)&Packet.Header, &Packet.ContigSize );

    RtlCopyMemory( Packet.Header, data, len );

    Packet.HeaderSize = sizeof(IPv4_HEADER);
    Packet.TotalSize = len;
    Packet.SrcAddr = LocalAddress;
    Packet.DstAddr = RemoteAddress;

    IPSendDatagram( &Packet, RCN, TCPPacketSendComplete, NULL );

    if( !NT_SUCCESS(NdisStatus) ) return OSK_EINVAL;
    else return 0;
}

void *TCPMalloc( void *ClientData,
		 OSK_UINT Bytes, OSK_PCHAR File, OSK_UINT Line ) {
    void *v = PoolAllocateBuffer( Bytes );
    if( v ) TrackWithTag( FOURCC('f','b','s','d'), v, File, Line );
    return v;
}

void TCPFree( void *ClientData,
	      void *data, OSK_PCHAR File, OSK_UINT Line ) {
    UntrackFL( File, Line, data );
    PoolFreeBuffer( data );
}

int TCPSleep( void *ClientData, void *token, int priority, char *msg,
	      int tmio ) {
    PSLEEPING_THREAD SleepingThread;
    
    TI_DbgPrint(MID_TRACE,
		("Called TSLEEP: tok = %x, pri = %d, wmesg = %s, tmio = %x\n",
		 token, priority, msg, tmio));

    SleepingThread = PoolAllocateBuffer( sizeof( *SleepingThread ) );
    if( SleepingThread ) {
	KeInitializeEvent( &SleepingThread->Event, NotificationEvent, FALSE );
	SleepingThread->SleepToken = token;

	TcpipAcquireFastMutex( &SleepingThreadsLock );
	InsertTailList( &SleepingThreadsList, &SleepingThread->Entry );
	TcpipReleaseFastMutex( &SleepingThreadsLock );

	TI_DbgPrint(MID_TRACE,("Waiting on %x\n", token));
	KeWaitForSingleObject( &SleepingThread->Event,
			       WrSuspended,
			       KernelMode,
			       TRUE,
			       NULL );

	TcpipAcquireFastMutex( &SleepingThreadsLock );
	RemoveEntryList( &SleepingThread->Entry );
	TcpipReleaseFastMutex( &SleepingThreadsLock );

	PoolFreeBuffer( SleepingThread );
    }
    TI_DbgPrint(MID_TRACE,("Waiting finished: %x\n", token));
    return 0;
}

void TCPWakeup( void *ClientData, void *token ) {
    PLIST_ENTRY Entry;
    PSLEEPING_THREAD SleepingThread;

    TcpipAcquireFastMutex( &SleepingThreadsLock );
    Entry = SleepingThreadsList.Flink;
    while( Entry != &SleepingThreadsList ) {
	SleepingThread = CONTAINING_RECORD(Entry, SLEEPING_THREAD, Entry);
	TI_DbgPrint(MID_TRACE,("Sleeper @ %x\n", SleepingThread));
	if( SleepingThread->SleepToken == token ) {
	    TI_DbgPrint(MID_TRACE,("Setting event to wake %x\n", token));
	    KeSetEvent( &SleepingThread->Event, IO_NETWORK_INCREMENT, FALSE );
	}
	Entry = Entry->Flink;
    }
    TcpipReleaseFastMutex( &SleepingThreadsLock );
}
