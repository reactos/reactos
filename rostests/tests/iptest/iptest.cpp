#include <iostream>
#include <list>
#include <string>
#include <sstream>
extern "C" {
    typedef unsigned short u_short;
#include <stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <ddk/tdi.h>
#include <ddk/tdikrnl.h>
#include <ddk/tdiinfo.h>
#include <ddk/ndis.h>
#include <titypes.h>
#include <ip.h>
#include <tcp.h>
#include <receive.h>
#include <lan.h>
#include <routines.h>
};

/* Undis */
extern "C" VOID ExpInitLookasideLists();

std::list<std::string> output_packets;
DWORD DebugTraceLevel = 0x7fffffff;
PVOID GlobalBufferPool, GlobalPacketPool;

#define MAX_DG_SIZE 16384

char hwaddr[6] = { 0x08, 0x00, 0x20, 0x0b, 0xb7, 0xbb };

char hdr[14] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0x08, 0x00 };

#define STRINGIFY(x) #x

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

void connect_complete( void *context, NTSTATUS status, unsigned long count ) {
    printf( "Connection: status %x\n", status );
}

void receive_complete( void *context, NTSTATUS status, unsigned long count ) {
    printf( "Receive: status %s (bytes %d)\n", status, count );
    if( !status && count ) {
	for( int off = 0; off < count; off += 16 ) {
	    display_row( (char *)context, off, count );
	}
	printf( "\n" );
    }
}

class SocketObject {
public:
    virtual ~SocketObject() { }
    virtual int send( char *buf, int len, int *bytes,
		      struct sockaddr_in *si ) = 0;
    virtual int recv( char *buf, int len, int *bytes,
		      struct sockaddr_in *si ) = 0;
};

UINT TdiAddressSizeFromType( UINT AddressType ) {
    switch( AddressType ) {
    case TDI_ADDRESS_TYPE_IP:
	return sizeof(TA_IP_ADDRESS);
    default:
	KeBugCheck( 0 );
    }
    return 0;
}

NTSTATUS TdiBuildNullConnectionInfoInPlace
( PTDI_CONNECTION_INFORMATION ConnInfo,
  ULONG Type )
/*
 * FUNCTION: Builds a NULL TDI connection information structure
 * ARGUMENTS:
 *     ConnectionInfo = Address of buffer to place connection information
 *     Type           = TDI style address type (TDI_ADDRESS_TYPE_XXX).
 * RETURNS:
 *     Status of operation
 */
{
  ULONG TdiAddressSize;

  TdiAddressSize = TdiAddressSizeFromType(Type);

  RtlZeroMemory(ConnInfo,
		sizeof(TDI_CONNECTION_INFORMATION) +
		TdiAddressSize);

  ConnInfo->OptionsLength = sizeof(ULONG);
  ConnInfo->RemoteAddressLength = 0;
  ConnInfo->RemoteAddress = NULL;

  return STATUS_SUCCESS;
}

NTSTATUS TdiBuildNullConnectionInfo
( PTDI_CONNECTION_INFORMATION *ConnectionInfo,
  ULONG Type )
/*
 * FUNCTION: Builds a NULL TDI connection information structure
 * ARGUMENTS:
 *     ConnectionInfo = Address of buffer pointer to allocate connection
 *                      information in
 *     Type           = TDI style address type (TDI_ADDRESS_TYPE_XXX).
 * RETURNS:
 *     Status of operation
 */
{
  PTDI_CONNECTION_INFORMATION ConnInfo;
  ULONG TdiAddressSize;
  NTSTATUS Status;

  TdiAddressSize = TdiAddressSizeFromType(Type);

  ConnInfo = (PTDI_CONNECTION_INFORMATION)
    ExAllocatePool(NonPagedPool,
		   sizeof(TDI_CONNECTION_INFORMATION) +
		   TdiAddressSize);
  if (!ConnInfo)
    return STATUS_INSUFFICIENT_RESOURCES;

  Status = TdiBuildNullConnectionInfoInPlace( ConnInfo, Type );

  if (!NT_SUCCESS(Status))
      ExFreePool( ConnInfo );
  else
      *ConnectionInfo = ConnInfo;

  ConnInfo->RemoteAddress = (PTA_ADDRESS)&ConnInfo[1];
  ConnInfo->RemoteAddressLength = TdiAddressSize;

  return Status;
}


UINT TaLengthOfTransportAddress( PTRANSPORT_ADDRESS Addr ) {
    UINT AddrLen = 2 * sizeof( ULONG ) + Addr->Address[0].AddressLength;
    printf("AddrLen %x\n", AddrLen);
    return AddrLen;
}

NTSTATUS
TdiBuildConnectionInfoInPlace
( PTDI_CONNECTION_INFORMATION ConnectionInfo,
  PTA_ADDRESS Address ) {
    NTSTATUS Status = STATUS_SUCCESS;

    RtlCopyMemory( ConnectionInfo->RemoteAddress,
		   Address,
		   ConnectionInfo->RemoteAddressLength );

    return Status;
}

NTSTATUS
TdiBuildConnectionInfo
( PTDI_CONNECTION_INFORMATION *ConnectionInfo,
  PTA_ADDRESS Address ) {
  NTSTATUS Status = TdiBuildNullConnectionInfo( ConnectionInfo,
						Address->AddressType );

  if( NT_SUCCESS(Status) )
      TdiBuildConnectionInfoInPlace( *ConnectionInfo, Address );

  return Status;
}

class TCPSocketObject : public SocketObject {
public:
    TCPSocketObject( std::string host, int port, NTSTATUS *status ) {
	TA_IP_ADDRESS ConnectTo;
	PTDI_CONNECTION_INFORMATION ConnInfo;

	ConnectTo.TAAddressCount = 1;
	ConnectTo.Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
	ConnectTo.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
	ConnectTo.Address[0].Address[0].sin_port = htons(port);
	ConnectTo.Address[0].Address[0].in_addr = 0x6a020a0a;

	TdiBuildConnectionInfo( &ConnInfo, (PTA_ADDRESS)&ConnectTo );

	Connection = TCPAllocateConnectionEndpoint( NULL );
	*status = TCPSocket( Connection,
			     AF_INET,
			     SOCK_STREAM, IPPROTO_TCP );
	if( !*status )
	    *status = TCPConnect( Connection,
				  ConnInfo,
				  NULL,
				  connect_complete,
				  NULL );
    }

    ~TCPSocketObject() {
	TCPClose( Connection );
	if( Connection ) TCPFreeConnectionEndpoint( Connection );
    }

    int send( char *buf, int len, int *bytes, struct sockaddr_in *si ) {
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if( Connection )
	    Status = TCPSendData( Connection,
				  buf,
				  len,
				  (PULONG)bytes,
				  0 );
	return Status;
    }

    int recv( char *buf, int len, int *bytes, struct sockaddr_in *si ) {
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if( Connection )
	    Status = TCPSendData( Connection,
				  buf,
				  len,
				  (PULONG)bytes,
				  0 );
	return Status;
    }

private:
    PCONNECTION_ENDPOINT Connection;
};

VOID SendPacket( PVOID Context,
		 PNDIS_PACKET NdisPacket,
		 UINT Offset,
		 PVOID LinkAddress,
		 USHORT Type ) {
    PCHAR DataOut;
    PUCHAR Addr = (PUCHAR)LinkAddress;
    UINT Size;
    std::string output_packet;

    printf( "Sending packet: %02x:%02x:%02x:%02x:%02x:%02x\n",
	    Addr[0], Addr[1], Addr[2], Addr[3], Addr[4], Addr[5] );

    GetDataPtr( NdisPacket, Offset, &DataOut, &Size );
    for( int off = 0; off < Size; off += 16 ) {
	display_row( DataOut, off, Size );
    }
    printf( "\n" );

    output_packet += std::string( hwaddr, sizeof(hwaddr) );
    output_packet += std::string( (char *)LinkAddress, sizeof(hwaddr) );
    output_packet += (char)(Type >> 8);
    output_packet += (char)Type;
    output_packet += std::string( DataOut + Offset, Size - Offset );

    output_packets.push_back( output_packet );
}

#if 0
UINT CopyBufferToBufferChain
( PNDIS_BUFFER DstBuffer, UINT DstOffset, PCHAR SrcData, UINT Length ) {
    assert( 0 );
}
#endif

int main( int argc, char **argv ) {
    int asock = INVALID_SOCKET, selret, dgrecv, fromsize, err, port = 5001;
    int bytes, adapter_id, mtu, speed;
    char datagram[MAX_DG_SIZE];
    struct fd_set readf;
    struct timeval tv;
    struct sockaddr_in addr_from = { AF_INET }, addr_to;
    std::string word, cmdin, host;
    std::list<std::string>::iterator i;
    WSADATA wsadata;
    NTSTATUS Status;
    UNICODE_STRING RegistryUnicodePath;
    PCONNECTION_ENDPOINT Connection;
    PIP_INTERFACE Interface;
    IP_PACKET IPPacket;
    LLIP_BIND_INFO BindInfo;
    SocketObject *S = NULL;

    RtlInitUnicodeString
	( &RegistryUnicodePath,
	  L"\\SYSTEM\\CurrentControlSet\\Services"
	  L"\\Tcpip" );

    ExpInitLookasideLists();

    WSAStartup( 0x101, &wsadata );

    if( argc > 1 ) port = atoi(argv[1]);

    IPStartup( &RegistryUnicodePath );

    BindInfo.Context = NULL;
    BindInfo.HeaderSize = sizeof(ETH_HEADER);
    BindInfo.MTU = 1500; /* MTU for ethernet */
    BindInfo.Address = (PUCHAR)hwaddr;
    BindInfo.AddressLength = sizeof(hwaddr);
    BindInfo.Transmit = SendPacket;

    IPCreateInterface( &BindInfo );

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

	    if( datagram[0] == 'C' && datagram[1] == 'M' &&
		datagram[2] == 'D' && datagram[3] == ' ' ) {
		int theport, bytes, recvret, off, bytin;
		struct sockaddr_in nam;
		std::string faddr, word;
		std::istringstream
		    cmdin( std::string( datagram + 4, dgrecv - 4 ) );

		cmdin >> word;

/* UDP Section */
		if( word == "udpsocket" ) {
/* TCP Section */
		} else if( word == "tcpsocket" ) {
		    cmdin >> host >> port;
		    S = new TCPSocketObject( host, port, &Status );
		    fprintf( stderr, "Socket: Result %x\n", Status );
		} else if( word == "close" ) {
		    TCPClose( Connection );
		    TCPFreeConnectionEndpoint( Connection );
		} else if( word == "type" ) {
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
		    if( S )
			err = S->send( p, strlen(p), &bytes, NULL );
		    if( err > 0 ) { bytin = err; err = 0; }

		    if( err )
			fprintf ( stderr, "OskitTCPConnect: error %d\n",
				  err );
		    else {
			printf ( "wrote %d bytes\n", bytin );
		    }
		} else if( word == "send" ) {
		    off = 0;
		    while( cmdin >> word ) {
			datagram[off++] =
			    atoi( (std::string("0x") + word).c_str() );
		    }

		    if( (err = S->send( datagram, off, &bytin, NULL )) != 0 ) {
			fprintf( stderr, "OskitTCPConnect: error %d\n", err );
		    } else {
			printf( "wrote %d bytes\n", bytin );
		    }
		} else if( word == "recv" ) {
		    cmdin >> bytes;

		    if( (err = S->recv( datagram,
					sizeof(datagram),
					&bytes,
					NULL )) != 0 ) {
			fprintf( stderr, "OskitTCPRecv: error %d\n", err );
		    }

/* Misc section */
		} else if( word == "end" ) {
		    return 0;
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
		    IPPacket.Header = datagram;
		    IPPacket.Data = datagram + 14;
		    IPPacket.TotalSize = dgrecv;
		    IPReceive( Interface, &IPPacket );
		}
	    }
	}

	IPTimeout(NULL, NULL, NULL, NULL);

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

