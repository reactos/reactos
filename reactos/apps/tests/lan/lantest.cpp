#include <windows.h>
#include <net/lan.h>
#include <iostream>
#include <string>
#include <ddk/ntddk.h>
#include <rosrtl/string.h>

using std::string;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

void display_row( char *data, int off, int len ) {
    int i;

    printf( "%08x:", off );
    for( i = off; i < len && i < off + 16; i++ ) {
	printf( " %02x", data[i] & 0xff );
    }

    for( ; i < off + 16; i++ )
	printf("   ");

    printf( " -- " );

    for( i = off; i < len && i < off + 16; i++ ) {
	printf( "%c", (data[i] >= ' ' && data[i] <= '~') ? data[i] : '.' );
    }

    printf( "\n" );
}

void display_buffer( char *Packet, int ReadLen ) {
    UINT PktLen;
    for( PktLen = 0; PktLen < ReadLen; PktLen += 16 )
	display_row( Packet, PktLen, ReadLen );
}

int byte_till_end( char *Packet, int PktLen ) {
    int byte;
    std::string word;

    cin >> word;
    while( word != "end" ) {
	byte = strtoul( (string("0x") + word).c_str(), 0, 0 );
	fprintf( stderr, "Byte[%d]: %x\n", PktLen, byte & 0xff );
	Packet[PktLen++] = byte;
	cin >> word;
    }

    return PktLen;
}

/* Ethernet types. We swap constants so we can compare values at runtime
   without swapping them there */
#define ETYPE_IPv4 WH2N(0x0800)
#define ETYPE_IPv6 WH2N(0x86DD)
#define ETYPE_ARP  WH2N(0x0806)

extern "C"
NTSTATUS STDCALL NtCreateFile(
    PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    PLARGE_INTEGER AllocationSize,
    ULONG FileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    PVOID EaBuffer,
    ULONG EaLength);

int main( int argc, char **argv ) {
    string word;
    HANDLE LanFile;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING LanDevice;
    IO_STATUS_BLOCK Iosb;
    HANDLE Event;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    NTSTATUS Status;
    DWORD On = 1, PktLen;
    CHAR Packet[1600];
    PLAN_PACKET_HEADER Hdr = (PLAN_PACKET_HEADER)Packet;
    PLAN_ADDRESS Addr = (PLAN_ADDRESS)Packet;
    USHORT TypesToListen[] = { ETYPE_IPv4, ETYPE_IPv6, ETYPE_ARP };
    UINT EaLength = LAN_EA_INFO_SIZE(sizeof(TypesToListen)/sizeof(USHORT));

    Status = NtCreateEvent(&Event,
			   EVENT_ALL_ACCESS,
			   NULL,
			   0,
			   0 );
    
    RtlInitUnicodeString( &LanDevice, L"\\Device\\Lan" );

    InitializeObjectAttributes( &Attributes, 
				&LanDevice,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL );

    EaBuffer = (PFILE_FULL_EA_INFORMATION)calloc( EaLength, 1 );
    LAN_FILL_EA_INFO(EaBuffer,sizeof(TypesToListen)/sizeof(USHORT),
		     TypesToListen);

    Status = ZwCreateFile( &LanFile, 
			   SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | 
			   GENERIC_EXECUTE, 
			   &Attributes,
			   &Iosb,
			   NULL,
			   FILE_ATTRIBUTE_NORMAL,
			   FILE_SHARE_READ | FILE_SHARE_WRITE,
			   FILE_OPEN_IF,
			   FILE_SYNCHRONOUS_IO_NONALERT,
			   EaBuffer,
			   EaLength );

    if( !NT_SUCCESS(Status) ) {
	cerr << "Could not open lan device " << Status << "\n";
	return 1;
    }
    
    Status = DeviceIoControl( LanFile,
			      IOCTL_IF_BUFFERED_MODE,
			      &On,
			      sizeof(On),
			      0,
			      0,
			      &PktLen,
			      NULL );
    
    if( !Status ) {
	cerr << "Could not turn on buffered mode " << Status << "\n";
	return 1;
    }

    while( cin >> word ) {
	if( word == "end" ) {
	    NtClose( LanFile );
	    return 0;
	} else if( word == "enum" ) {
	    Status = DeviceIoControl( LanFile,
				      IOCTL_IF_ENUM_ADAPTERS,
				      NULL,
				      0,
				      Packet,
				      sizeof(Packet),
				      &PktLen,
				      NULL );

	    cout << "EnumAdapters: " << Status << "\n";
	    if( Status ) 
		display_buffer( Packet, PktLen );
	} else if( word == "query" ) {
	    cin >> PktLen;

	    Status = DeviceIoControl( LanFile,
				      IOCTL_IF_ADAPTER_INFO,
				      &PktLen,
				      sizeof(PktLen),
				      Packet,
				      sizeof(Packet),
				      &PktLen,
				      NULL );
	    
	    cout << "QueryAdapterInfo: " << Status << "\n";
	    if( Status )
		display_buffer( Packet, PktLen );
	} else if( word == "send" ) {
	    cin >> Hdr->Fixed.Adapter 
		>> Hdr->Fixed.AddressType 
		>> Hdr->Fixed.AddressLen 
		>> Hdr->Fixed.PacketType;
	    Hdr->Fixed.Mdl = NULL;
	    PktLen = byte_till_end( Packet, Hdr->Address - (PCHAR)Hdr );
	    Status = NtWriteFile( LanFile, 
				  NULL,
				  NULL,
				  NULL,
				  &Iosb,
				  Packet,
				  PktLen,
				  NULL,
				  NULL );

	    cout << "NtWriteFile: " << Status << "\n";
	} else if( word == "recv" ) {
	    ULONG ReadLen;
	    Status = NtReadFile( LanFile,
				 Event,
				 NULL,
				 NULL,
				 &Iosb,
				 Packet,
				 sizeof(Packet),
				 NULL,
				 NULL );
	    cout << "NtReadFile: " << Status << "\n";
	    if( Status == STATUS_PENDING ) {
		LARGE_INTEGER Timeout = { 0 };
		Status = NtWaitForSingleObject( Event, 1, &Timeout );
	    } 

	    ReadLen = Iosb.Information;

	    if( Status == STATUS_SUCCESS ) {
		cout << "Read " << ReadLen << " bytes\n";
		display_buffer( Packet, ReadLen );
	    }
	}
    }

    return 0;
}
