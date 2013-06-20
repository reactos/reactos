#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <list>
#include <string>
#include <sstream>
#include <malloc.h>
extern "C" {
    typedef unsigned short u_short;
#include <stdio.h>
#include <oskittcp.h>
#include <windows.h>
#ifndef _MSC_VER
#include <winsock2.h>
#endif//_MSC_VER
};

unsigned char hwaddr[6] = { 0x08, 0x00, 0x20, 0x0b, 0xb7, 0xbb };

#undef malloc
#undef free

unsigned long TCP_IPIdentification;

#define MAX_DG_SIZE 0x10000
#define TI_DbgPrint(x,y) printf y

std::list<std::string> output_packets;

typedef struct _CONNECTION_ENDPOINT {
    OSK_UINT State;
} CONNECTION_ENDPOINT, *PCONNECTION_ENDPOINT;

extern "C" int is_stack_ptr ( const void* p )
{
	MEMORY_BASIC_INFORMATION mbi1, mbi2;
	VirtualQuery ( p, &mbi1, sizeof(mbi1) );
	VirtualQuery ( _alloca(1), &mbi2, sizeof(mbi2) );
	return mbi1.AllocationBase == mbi2.AllocationBase;
}

int TCPSocketState(void *ClientData,
		   void *WhichSocket,
		   void *WhichConnection,
		   OSK_UINT NewState ) {
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)WhichConnection;
    //PLIST_ENTRY Entry;

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

char hdr[14] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0x08, 0x00 };

int TCPPacketSend(void *ClientData, OSK_PCHAR data, OSK_UINT len ) {
    output_packets.push_back( std::string( hdr, 14 ) +
			      std::string( (char *)data, (int)len ) );
    return 0;
}

struct ifaddr *TCPFindInterface( void *ClientData,
				 OSK_UINT AddrType,
				 OSK_UINT FindType,
				 struct sockaddr *ReqAddr ) {
    static struct sockaddr_in ifa = { AF_INET }, nm = { AF_INET };
    static struct ifaddr a = {
	(struct sockaddr *)&ifa,
	NULL,
	(struct sockaddr *)&nm,
	0,
	0,
	1,
	1500
    };
    ifa.sin_addr.s_addr = inet_addr( "10.10.2.115" );
    nm.sin_addr.s_addr  = inet_addr( "255.255.255.0" );
    return &a;
}

void *TCPMalloc( void *ClientData,
		 OSK_UINT Bytes, OSK_PCHAR File, OSK_UINT Line ) {
    void *v = malloc( Bytes );
    fprintf( stderr, "(%s:%d) malloc( %d ) => %x\n", File, Line, Bytes, v );
    return v;
}

void TCPFree( void *ClientData,
	      void *data, OSK_PCHAR File, OSK_UINT Line ) {
    fprintf( stderr, "(%s:%d) free( %x )\n", File, Line, data );
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

void display_row( char *data, int off, int len ) {
    int i;

    printf( "%08x:", off );
    for( i = off; i < len && i < off + 16; i++ ) {
	printf( " %02x", data[i] & 0xff );
    }

    printf( " -- " );

    for( i = off; i < len && i < off + 16; i++ ) {
	printf( "%c", (data[i] >= ' ') ? data[i] : '.' );
    }

    printf( "\n" );
}

int main( int argc, char **argv ) {
    int asock = INVALID_SOCKET, selret, dgrecv, fromsize, err, port = 5001;
    char datagram[MAX_DG_SIZE];
    void *conn = 0;
    struct fd_set readf;
    struct timeval tv;
    struct sockaddr_in addr_from = { AF_INET }, addr_to = { AF_INET };
    std::list<std::string>::iterator i;
    WSADATA wsadata;

    WSAStartup( 0x101, &wsadata );

    if( argc > 1 ) port = atoi(argv[1]);

    RegisterOskitTCPEventHandlers( &EventHandlers );
    InitOskitTCP();

    asock = socket( AF_INET, SOCK_DGRAM, 0 );

    addr_from.sin_port = htons( port );

    if( bind( asock, (struct sockaddr *)&addr_from, sizeof( addr_from ) ) ) {
	printf( "Bind error\n" );
	return 0;
    }

    addr_to.sin_port = htons( port & (~1) );
    addr_to.sin_addr.s_addr = inet_addr("127.0.0.1");

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

	    if( datagram[0] == 'C' && datagram[1] == 'M' &&
		datagram[2] == 'D' && datagram[3] == ' ' ) {
		int theport, bytes, /*recvret,*/ off, bytin;
		struct sockaddr_in nam;
		std::string faddr, word;
		std::istringstream
		    cmdin( std::string( datagram + 4, dgrecv - 4 ) );

		cmdin >> word;

		if( word == "socket" ) {
		    cmdin >> faddr >> theport;

		    nam.sin_family = AF_INET;
		    nam.sin_addr.s_addr = inet_addr(faddr.c_str());
		    nam.sin_port = htons(theport);

		    if( (err = OskitTCPSocket( NULL, &conn, AF_INET,
					      SOCK_STREAM, 0 )) != 0 ) {
			fprintf( stderr, "OskitTCPSocket: error %d\n", err );
		    }

		    if( (err = OskitTCPConnect( conn, NULL, &nam,
						sizeof(nam) )) != 0 ) {
			fprintf( stderr, "OskitTCPConnect: error %d\n", err );
		    } else {
			printf( "Socket created\n" );
		    }
		}

		/* The rest of the commands apply only to an open socket */
		if( !conn ) continue;

		if( word == "recv" ) {
		    cmdin >> bytes;

		    if( (err = OskitTCPRecv( conn, (OSK_PCHAR)datagram,
					     sizeof(datagram),
					     (unsigned int *)&bytin, 0 )) != 0 ) {
			fprintf( stderr, "OskitTCPRecv: error %d\n", err );
		    } else {
			for( off = 0; off < bytin; off += 16 ) {
			    display_row( datagram, off, bytin );
			}
			printf( "\n" );
		    }
		} else if ( word == "type" ) {
			std::string therest = &cmdin.str()[word.size()];
			char* p = &therest[0];
			p += strspn ( p, " \t" );
			char* src = p;
			char* dst = p;
			while ( *src )
			{
				char c = *src++;
				if ( c == '\r' || c == '\n' ) break;
				if ( c == '\\' )
				{
					c = *src++;
					switch ( c )
					{
					case 'b': c = '\b'; break;
					case 'n': c = '\n'; break;
					case 'r': c = '\r'; break;
					case 't': c = '\t'; break;
					case 'v': c = '\v'; break;
					}
				}
				*dst++ = c;
			}
			*dst = '\0';
			if ( (err = OskitTCPSend ( conn, (OSK_PCHAR)p, strlen(p), (OSK_UINT*)&bytin, 0 ))
				!= 0 ) {
				fprintf ( stderr, "OskitTCPConnect: error %d\n", err );
			} else {
				printf ( "wrote %d bytes\n", bytin );
			}
		} else if( word == "send" ) {
		    off = 0;
		    while( cmdin >> word ) {
			datagram[off++] =
			    atoi( (std::string("0x") + word).c_str() );
		    }

		    if( (err = OskitTCPSend( conn, (OSK_PCHAR)datagram,
					     off, (OSK_UINT *)&bytin, 0 ))
			!= 0 ) {
			fprintf( stderr, "OskitTCPConnect: error %d\n", err );
		    } else {
			printf( "wrote %d bytes\n", bytin );
		    }
		} else if( word == "close" ) {
		    OskitTCPClose( conn );
		    conn = NULL;
		}
	    } else if( dgrecv > 14 ) {
		addr_to = addr_from;

		if( datagram[12] == 8 && datagram[13] == 6 ) {
		    /* Answer arp query */
		    char laddr[4];
		    /* Mark patch as to the previous sender */
		    memcpy( datagram + 32, datagram + 6, 6 );
		    memcpy( datagram, datagram + 6, 6 );
		    /* Mark packet as from us */
		    memcpy( datagram + 22, hwaddr, 6 );
		    memcpy( datagram + 6, hwaddr, 6 );
		    /* Swap inet addresses */
		    memcpy( laddr, datagram + 28, 4 );
		    memcpy( datagram + 28, datagram + 38, 4 );
		    memcpy( datagram + 38, laddr, 4 );
		    /* Set reply opcode */
		    datagram[21] = 2;

		    err = sendto( asock, datagram, dgrecv, 0,
				  (struct sockaddr *)&addr_to,
				  sizeof(addr_to) );

		    if( err != 0 )
			printf( "sendto: %d\n", err );
		} else {
		    memcpy( hdr, datagram + 6, 6 );
		    memcpy( hdr + 6, datagram, 6 );
		    memcpy( hdr + 12, datagram + 12, 2 );
		    OskitTCPReceiveDatagram
			( (unsigned char *)datagram + 14,
			  dgrecv - 14, 20 );
		}
	    }
	}

	TimerOskitTCP();

	for( i = output_packets.begin(); i != output_packets.end(); i++ ) {
	    err = sendto( asock, i->c_str(), i->size(), 0,
			  (struct sockaddr *)&addr_to, sizeof(addr_to) );

	    fprintf( stderr, "** SENDING PACKET %d bytes **\n", i->size() );

	    if( err != 0 )
		printf( "sendto: %d\n", err );
	}

	output_packets.clear();
    }
}

