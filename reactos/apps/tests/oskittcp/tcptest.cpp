#include <list>
#include <string>
extern "C" {
#include <stdio.h>
#include <oskittcp.h>
#include <windows.h>
#include <winsock2.h>
};

#undef malloc
#undef free

unsigned long TCP_IPIdentification;

#define MAX_DG_SIZE 0x10000
#define TI_DbgPrint(x,y) printf y

std::list<std::string> output_packets;

typedef struct _CONNECTION_ENDPOINT {
    OSK_UINT State;
} CONNECTION_ENDPOINT, *PCONNECTION_ENDPOINT;

int TCPSocketState(void *ClientData,
		   void *WhichSocket, 
		   void *WhichConnection,
		   OSK_UINT NewState ) {
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)WhichConnection;
    PLIST_ENTRY Entry;

    TI_DbgPrint(MID_TRACE,("Called: NewState %x\n", NewState));

    if( !Connection ) {
	TI_DbgPrint(MID_TRACE,("Socket closing.\n"));
	return 0;
    }

    if( (NewState & SEL_CONNECT) && 
	!(Connection->State & SEL_CONNECT) ) {
    } else if( (NewState & SEL_READ) || (NewState & SEL_FIN) ) {
    } 

    return 0;
}

#define STRINGIFY(x) #x

int TCPPacketSend(void *ClientData, OSK_PCHAR data, OSK_UINT len ) {
    output_packets.push_back( std::string( (char *)data, (int)len ) );
    return 0;
}

struct ifaddr *TCPFindInterface( void *ClientData,
				 OSK_UINT AddrType,
				 OSK_UINT FindType,
				 struct sockaddr *ReqAddr ) {
    return NULL;
}

void *TCPMalloc( void *ClientData,
		 OSK_UINT Bytes, OSK_PCHAR File, OSK_UINT Line ) {
    return malloc( Bytes );
}

void TCPFree( void *ClientData,
	      void *data, OSK_PCHAR File, OSK_UINT Line ) {
    free( data );
}

int TCPSleep( void *ClientData, void *token, int priority, char *msg,
	      int tmio ) {
#if 0
    PSLEEPING_THREAD SleepingThread;
    
    TI_DbgPrint(MID_TRACE,
		("Called TSLEEP: tok = %x, pri = %d, wmesg = %s, tmio = %x\n",
		 token, priority, msg, tmio));

    SleepingThread = ExAllocatePool( NonPagedPool, sizeof( *SleepingThread ) );
    if( SleepingThread ) {
	KeInitializeEvent( &SleepingThread->Event, NotificationEvent, FALSE );
	SleepingThread->SleepToken = token;

	ExAcquireFastMutex( &SleepingThreadsLock );
	InsertTailList( &SleepingThreadsList, &SleepingThread->Entry );
	ExReleaseFastMutex( &SleepingThreadsLock );

	TI_DbgPrint(MID_TRACE,("Waiting on %x\n", token));
	KeWaitForSingleObject( &SleepingThread->Event,
			       WrSuspended,
			       KernelMode,
			       TRUE,
			       NULL );

	ExAcquireFastMutex( &SleepingThreadsLock );
	RemoveEntryList( &SleepingThread->Entry );
	ExReleaseFastMutex( &SleepingThreadsLock );

	ExFreePool( SleepingThread );
    }
    TI_DbgPrint(MID_TRACE,("Waiting finished: %x\n", token));
#endif
    return 0;
}

void TCPWakeup( void *ClientData, void *token ) {
#if 0
    PLIST_ENTRY Entry;
    PSLEEPING_THREAD SleepingThread;

    ExAcquireFastMutex( &SleepingThreadsLock );
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
    ExReleaseFastMutex( &SleepingThreadsLock );
#endif
}

OSKITTCP_EVENT_HANDLERS EventHandlers = {
    NULL,
    TCPSocketState,
    TCPPacketSend,
    TCPFindInterface,
    TCPMalloc,
    TCPFree,
    TCPSleep,
    TCPWakeup
};

int main( int argc, char **argv ) {
    int asock = INVALID_SOCKET, selret, dgrecv, fromsize, err, port = 5000;
    char datagram[MAX_DG_SIZE];
    struct fd_set readf;
    struct timeval tv;
    struct sockaddr_in addr_from = { AF_INET };
    std::list<std::string>::iterator i;

    if( argc > 1 ) port = atoi(argv[1]);

    RegisterOskitTCPEventHandlers( &EventHandlers );
    InitOskitTCP();

    asock = socket( AF_INET, SOCK_DGRAM, 0 );

    addr_from.sin_port = htons( port );

    if( bind( asock, (struct sockaddr *)&addr_from, sizeof( addr_from ) ) ) {
	printf( "Bind error\n" );
	return 0;
    }

    while( true ) {
	FD_ZERO( &readf );
	FD_SET( asock, &readf );
	tv.tv_sec = 0; 
	tv.tv_usec = 10000;
	selret = select( asock + 1, &readf, NULL, NULL, &tv );

	if( FD_ISSET( asock, &readf ) ) {
	    fromsize = sizeof( addr_from );
	    dgrecv = recvfrom( asock, datagram, sizeof(datagram), 0,
			       (struct sockaddr *)&addr_from, &fromsize );

	    if( dgrecv > 0 ) {
		OskitTCPReceiveDatagram( (unsigned char *)datagram, 
					 dgrecv, 20 );
		if( err != 0 )
		    printf( "OskitTCPReceiveDatagram: %d\n", err );
	    }
	}

	TimerOskitTCP();

	for( i = output_packets.begin(); i != output_packets.end(); i++ ) {
	    err = sendto( asock, i->c_str(), i->size(), 0, 
			  (struct sockaddr *)&addr_from, sizeof(addr_from) );

	    if( err != 0 )
		printf( "sendto: %d\n", err );
	}

	output_packets.clear();
    }
}
