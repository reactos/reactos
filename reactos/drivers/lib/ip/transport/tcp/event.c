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

extern VOID DrainSignals();

int TCPSocketState(void *ClientData,
		   void *WhichSocket, 
		   void *WhichConnection,
		   OSK_UINT NewState ) {
    PCONNECTION_ENDPOINT Connection = WhichConnection;

    TI_DbgPrint(DEBUG_TCP,("Called: NewState %x (Conn %x) (Change %x)\n", 
			   NewState, Connection,
			   Connection ? Connection->State ^ NewState : 
			   NewState));

    if( !Connection ) {
	TI_DbgPrint(DEBUG_TCP,("Socket closing.\n"));
	Connection = FileFindConnectionByContext( WhichSocket );
	if( !Connection ) {
	    TcpipRecursiveMutexLeave( &TCPLock );
	    return 0;
	} else 
	    TI_DbgPrint(DEBUG_TCP,("Found socket %x\n", Connection));
    }

    if( !Connection->Signalled ) {
	Connection->Signalled = TRUE;
	Connection->SignalState = NewState;
	InsertTailList( &SignalledConnections, &Connection->SignalList );
    }

    return 0;
}

void TCPPacketSendComplete( PVOID Context,
			    PNDIS_PACKET NdisPacket,
			    NDIS_STATUS NdisStatus ) {
    TI_DbgPrint(DEBUG_TCP,("called %x\n", NdisPacket));
    FreeNdisPacket(NdisPacket);
    TI_DbgPrint(DEBUG_TCP,("done\n"));
}

#define STRINGIFY(x) #x

int TCPPacketSend(void *ClientData, OSK_PCHAR data, OSK_UINT len ) {
    NDIS_STATUS NdisStatus;
    PNEIGHBOR_CACHE_ENTRY NCE;
    IP_PACKET Packet = { 0 };
    IP_ADDRESS RemoteAddress, LocalAddress;
    PIPv4_HEADER Header;

    if( *data == 0x45 ) { /* IPv4 */
	Header = (PIPv4_HEADER)data;
	LocalAddress.Type = IP_ADDRESS_V4;
	LocalAddress.Address.IPv4Address = Header->SrcAddr;
	RemoteAddress.Type = IP_ADDRESS_V4;
	RemoteAddress.Address.IPv4Address = Header->DstAddr;
    } else {
	TI_DbgPrint(MIN_TRACE,("Outgoing packet is not IPv4\n"));
	OskitDumpBuffer( data, len );
	return OSK_EINVAL;
    }

    RemoteAddress.Type = LocalAddress.Type = IP_ADDRESS_V4;

    if(!(NCE = RouteGetRouteToDestination( &RemoteAddress ))) {
	TI_DbgPrint(MIN_TRACE,("No route to %s\n", A2S(&RemoteAddress)));
	return OSK_EADDRNOTAVAIL;
    }

    NdisStatus = AllocatePacketWithBuffer( &Packet.NdisPacket, NULL, 
					   MaxLLHeaderSize + len );
    
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
	TI_DbgPrint(DEBUG_TCP, ("Error from NDIS: %08x\n", NdisStatus));
	return STATUS_NO_MEMORY;
    }

    GetDataPtr( Packet.NdisPacket, MaxLLHeaderSize, 
		(PCHAR *)&Packet.Header, &Packet.ContigSize );

    RtlCopyMemory( Packet.Header, data, len );

    Packet.HeaderSize = sizeof(IPv4_HEADER);
    Packet.TotalSize = len;
    Packet.SrcAddr = LocalAddress;
    Packet.DstAddr = RemoteAddress;

    IPSendDatagram( &Packet, NCE, TCPPacketSendComplete, NULL );
    
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
    
    TI_DbgPrint(DEBUG_TCP,
		("Called TSLEEP: tok = %x, pri = %d, wmesg = %s, tmio = %x\n",
		 token, priority, msg, tmio));

    SleepingThread = PoolAllocateBuffer( sizeof( *SleepingThread ) );
    if( SleepingThread ) {
	KeInitializeEvent( &SleepingThread->Event, NotificationEvent, FALSE );
	SleepingThread->SleepToken = token;

	TcpipAcquireFastMutex( &SleepingThreadsLock );
	InsertTailList( &SleepingThreadsList, &SleepingThread->Entry );
	TcpipReleaseFastMutex( &SleepingThreadsLock );

	TI_DbgPrint(DEBUG_TCP,("Waiting on %x\n", token));
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
    TI_DbgPrint(DEBUG_TCP,("Waiting finished: %x\n", token));
    return 0;
}

void TCPWakeup( void *ClientData, void *token ) {
    PLIST_ENTRY Entry;
    PSLEEPING_THREAD SleepingThread;

    TcpipAcquireFastMutex( &SleepingThreadsLock );
    Entry = SleepingThreadsList.Flink;
    while( Entry != &SleepingThreadsList ) {
	SleepingThread = CONTAINING_RECORD(Entry, SLEEPING_THREAD, Entry);
	TI_DbgPrint(DEBUG_TCP,("Sleeper @ %x\n", SleepingThread));
	if( SleepingThread->SleepToken == token ) {
	    TI_DbgPrint(DEBUG_TCP,("Setting event to wake %x\n", token));
	    KeSetEvent( &SleepingThread->Event, IO_NETWORK_INCREMENT, FALSE );
	}
	Entry = Entry->Flink;
    }
    TcpipReleaseFastMutex( &SleepingThreadsLock );
}
