/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TDI library
 * FILE:        include/net/tdi.h
 * PURPOSE:     TDI definitions for user mode clients
 */
#ifndef __TDI_H
#define __TDI_H

/* FIXME: Missed some definitions in ntddk.h */

/* Could be defined in ndis.h */
#ifndef __NDIS_H
typedef signed int INT, *PINT;
#endif



/* Basic types */

typedef LONG    TDI_STATUS;
typedef PVOID   CONNECTION_CONTEXT;



/* TDI status codes */

#define TDI_SUCCESS             STATUS_SUCCESS
#define TDI_NO_RESOURCES        STATUS_INSUFFICIENT_RESOURCES
#define TDI_ADDR_IN_USE         STATUS_ADDRESS_ALREADY_EXISTS
#define TDI_BAD_ADDR            STATUS_INVALID_ADDRESS_COMPONENT
#define TDI_NO_FREE_ADDR        STATUS_TOO_MANY_ADDRESSES
#define TDI_ADDR_INVALID        STATUS_INVALID_ADDRESS
#define TDI_ADDR_DELETED        STATUS_ADDRESS_CLOSED
#define TDI_BUFFER_OVERFLOW     STATUS_BUFFER_OVERFLOW
#define TDI_BAD_EVENT_TYPE      STATUS_INVALID_PARAMETER
#define TDI_BAD_OPTION          STATUS_INVALID_PARAMETER
#define TDI_CONN_REFUSED        STATUS_CONNECTION_REFUSED
#define TDI_INVALID_CONNECTION  STATUS_CONNECTION_INVALID
#define TDI_ALREADY_ASSOCIATED  STATUS_ADDRESS_ALREADY_ASSOCIATED
#define TDI_NOT_ASSOCIATED      STATUS_ADDRESS_NOT_ASSOCIATED
#define TDI_CONNECTION_ACTIVE   STATUS_CONNECTION_ACTIVE
#define TDI_CONNECTION_ABORTED  STATUS_CONNECTION_ABORTED
#define TDI_CONNECTION_RESET    STATUS_CONNECTION_RESET
#define TDI_TIMED_OUT           STATUS_IO_TIMEOUT
#define TDI_GRACEFUL_DISC       STATUS_GRACEFUL_DISCONNECT
#define TDI_NOT_ACCEPTED        STATUS_DATA_NOT_ACCEPTED
#define TDI_MORE_PROCESSING     STATUS_MORE_PROCESSING_REQUIRED
#define TDI_INVALID_STATE       STATUS_INVALID_DEVICE_STATE
#define TDI_INVALID_PARAMETER   STATUS_INVALID_PARAMETER
#define TDI_DEST_NET_UNREACH    STATUS_NETWORK_UNREACHABLE
#define TDI_DEST_HOST_UNREACH   STATUS_HOST_UNREACHABLE
#define TDI_DEST_UNREACHABLE    TDI_DEST_HOST_UNREACH
#define TDI_DEST_PROT_UNREACH   STATUS_PROTOCOL_UNREACHABLE
#define TDI_DEST_PORT_UNREACH   STATUS_PORT_UNREACHABLE
#define TDI_INVALID_QUERY       STATUS_INVALID_DEVICE_REQUEST
#define TDI_REQ_ABORTED         STATUS_REQUEST_ABORTED
#define TDI_BUFFER_TOO_SMALL    STATUS_BUFFER_TOO_SMALL
#define TDI_CANCELLED           STATUS_CANCELLED
#define	TDI_BUFFER_TOO_BIG		STATUS_INVALID_BUFFER_SIZE
#define TDI_INVALID_REQUEST     STATUS_INVALID_DEVICE_REQUEST
#define TDI_PENDING             STATUS_PENDING
#define TDI_ITEM_NOT_FOUND      STATUS_OBJECT_NAME_NOT_FOUND

#define	TDI_OPTION_EOL              0
#define	TDI_ADDRESS_OPTION_REUSE    1
#define	TDI_ADDRESS_OPTION_DHCP     2



/* TDI IOCTL codes */

#define TDI_CONTROL_CODE(Request, Method) \
            CTL_CODE(FILE_DEVICE_TRANSPORT, Request, Method, FILE_ANY_ACCESS)

#define IOCTL_TDI_ACCEPT                TDI_CONTROL_CODE(0,  METHOD_BUFFERED)
#define IOCTL_TDI_CONNECT               TDI_CONTROL_CODE(1,  METHOD_BUFFERED)
#define IOCTL_TDI_DISCONNECT            TDI_CONTROL_CODE(2,  METHOD_BUFFERED)
#define IOCTL_TDI_LISTEN                TDI_CONTROL_CODE(3,  METHOD_BUFFERED)
#define IOCTL_TDI_QUERY_INFORMATION     TDI_CONTROL_CODE(4,  METHOD_OUT_DIRECT)
#define IOCTL_TDI_RECEIVE               TDI_CONTROL_CODE(5,  METHOD_OUT_DIRECT)
#define IOCTL_TDI_RECEIVE_DATAGRAM      TDI_CONTROL_CODE(6,  METHOD_OUT_DIRECT)
#define IOCTL_TDI_SEND                  TDI_CONTROL_CODE(7,  METHOD_IN_DIRECT)
#define IOCTL_TDI_SEND_DATAGRAM         TDI_CONTROL_CODE(8,  METHOD_IN_DIRECT)
#define IOCTL_TDI_SET_EVENT_HANDLER     TDI_CONTROL_CODE(9,  METHOD_BUFFERED)
#define IOCTL_TDI_SET_INFORMATION       TDI_CONTROL_CODE(10, METHOD_IN_DIRECT)
#define IOCTL_TDI_ASSOCIATE_ADDRESS     TDI_CONTROL_CODE(11, METHOD_BUFFERED)
#define IOCTL_TDI_DISASSOCIATE_ADDRESS  TDI_CONTROL_CODE(12, METHOD_BUFFERED)
#define IOCTL_TDI_ACTION                TDI_CONTROL_CODE(13, METHOD_OUT_DIRECT)



/* TDI transport driver dispatch routines */

NTSTATUS TdiDispatchCleanup(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp);

NTSTATUS TdiDispatchClose(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp);

NTSTATUS TdiDispatchCreate(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp);

NTSTATUS TdiDispatchDeviceControl(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PIO_STACK_LOCATION  IrpSp);

BOOLEAN TdiDispatchFastDeviceControl(
    IN PFILE_OBJECT  FileObject,
    IN BOOLEAN  Wait,
    IN PVOID  InputBuffer,
    IN ULONG  InputBufferLength,
    IN OUT PVOID  OutputBuffer,
    IN ULONG  OutputBufferLength,
    IN ULONG  IoControlCode,
    OUT PIO_STATUS_BLOCK  IoStatus);

NTSTATUS TdiDispatchInternalDeviceControl(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp);



/* TDI structures */

typedef struct _TDI_CONNECTION_INFORMATION
{
    LONG   UserDataLength;
    PVOID  UserData;
    LONG   OptionsLength;
    PVOID  Options;
    LONG   RemoteAddressLength;
    PVOID  RemoteAddress;
} TDI_CONNECTION_INFORMATION, *PTDI_CONNECTION_INFORMATION;

typedef struct _TDI_REQUEST
{
    union {
        HANDLE              AddressHandle;
        CONNECTION_CONTEXT  ConnectionContext;
        HANDLE              ControlChannel;
    } Handle;
    PVOID      RequestNotifyObject;
    PVOID      RequestContext;
    TDI_STATUS TdiStatus;
} TDI_REQUEST, *PTDI_REQUEST;

typedef struct _TDI_REQUEST_STATUS
{
    TDI_STATUS  Status;
    PVOID  RequestContext;
    ULONG  BytesTransferred;
} TDI_REQUEST_STATUS, *PTDI_REQUEST_STATUS;


typedef struct _TA_ADDRESS
{
    USHORT  AddressLength;
    USHORT  AddressType;
    UCHAR   Address[1];
} TA_ADDRESS, *PTA_ADDRESS;

#define TDI_ADDRESS_TYPE_UNSPEC      0
#define TDI_ADDRESS_TYPE_UNIX        1
#define TDI_ADDRESS_TYPE_IP          2
#define TDI_ADDRESS_TYPE_IMPLINK     3
#define TDI_ADDRESS_TYPE_PUP         4
#define TDI_ADDRESS_TYPE_CHAOS       5
#define TDI_ADDRESS_TYPE_NS          6
#define TDI_ADDRESS_TYPE_IPX         6
#define TDI_ADDRESS_TYPE_NBS         7
#define TDI_ADDRESS_TYPE_ECMA        8
#define TDI_ADDRESS_TYPE_DATAKIT     9
#define TDI_ADDRESS_TYPE_CCITT      10
#define TDI_ADDRESS_TYPE_SNA        11
#define TDI_ADDRESS_TYPE_DECnet     12
#define TDI_ADDRESS_TYPE_DLI        13
#define TDI_ADDRESS_TYPE_LAT        14
#define TDI_ADDRESS_TYPE_HYLINK     15
#define TDI_ADDRESS_TYPE_APPLETALK  16
#define TDI_ADDRESS_TYPE_NETBIOS    17
#define TDI_ADDRESS_TYPE_8022       18
#define TDI_ADDRESS_TYPE_OSI_TSAP   19
#define TDI_ADDRESS_TYPE_NETONE     20
#define TDI_ADDRESS_TYPE_VNS        21
#define TDI_ADDRESS_TYPE_NETBIOS_EX 22


#define TdiTransportAddress             "TransportAddress"
#define TdiConnectionContext            "ConnectionContext"
#define TDI_TRANSPORT_ADDRESS_LENGTH    (sizeof(TdiTransportAddress) - 1)
#define TDI_CONNECTION_CONTEXT_LENGTH   (sizeof(TdiConnectionContext) - 1)


typedef struct _TRANSPORT_ADDRESS
{
    LONG        TAAddressCount;
    TA_ADDRESS  Address[1];
} TRANSPORT_ADDRESS, *PTRANSPORT_ADDRESS;

typedef struct _TDI_ADDRESS_INFO
{
    ULONG              ActivityCount;
    TRANSPORT_ADDRESS  Address;
} TDI_ADDRESS_INFO, *PTDI_ADDRESS_INFO;

typedef struct _TDI_ACTION_HEADER
{
    ULONG   TransportId;
    USHORT  ActionCode;
    USHORT  Reserved;
} TDI_ACTION_HEADER, *PTDI_ACTION_HEADER;

typedef struct _TDI_ADDRESS_8022
{
    UCHAR MACAddress[6];
} TDI_ADDRESS_8022, *PTDI_ADDRESS_8022;

typedef struct _TDI_ADDRESS_APPLETALK
{
    USHORT  Network;
    UCHAR   Node;
    UCHAR   Socket;
} TDI_ADDRESS_APPLETALK, *PTDI_ADDRESS_APPLETALK;

#define TDI_ADDRESS_LENGTH_APPLETALK    sizeof(TDI_ADDRESS_APPLETALK)

typedef struct _TDI_ADDRESS_IP
{
    USHORT  sin_port;
    ULONG   in_addr;
    UCHAR   sin_zero[8];
} TDI_ADDRESS_IP, *PTDI_ADDRESS_IP;

#define TDI_ADDRESS_LENGTH_IP   sizeof(TDI_ADDRESS_IP)

typedef struct _TDI_ADDRESS_IPX
{
    ULONG   NetworkAddress;
    UCHAR   NodeAddress[6];
    USHORT  Socket;
} TDI_ADDRESS_IPX, *PTDI_ADDRESS_IPX;

#define TDI_ADDRESS_LENGTH_IPX  sizeof(TDI_ADDRESS_IPX)

typedef struct _TDI_ADDRESS_NETBIOS
{
    USHORT  NetbiosNameType;
    UCHAR   NetbiosName[16];
} TDI_ADDRESS_NETBIOS, *PTDI_ADDRESS_NETBIOS;

#define TDI_ADDRESS_NETBIOS_TYPE_UNIQUE         0x0000
#define TDI_ADDRESS_NETBIOS_TYPE_GROUP          0x0001
#define TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE   0x0002
#define TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP    0x0003

#define TDI_ADDRESS_LENGTH_NETBIOS  sizeof(TDI_ADDRESS_NETBIOS)

typedef struct _TDI_ADDRESS_NETBIOS_EX
{
    UCHAR               EndpointName[16];
    TDI_ADDRESS_NETBIOS NetbiosAddress;
} TDI_ADDRESS_NETBIOS_EX, *PTDI_ADDRESS_NETBIOS_EX;

#define TDI_ADDRESS_LENGTH_NETBIOS_EX   sizeof(TDI_ADDRESS_NETBIOS_EX)

typedef struct _TDI_ADDRESS_NETONE
{
    USHORT  NetoneNameType;
    UCHAR   NetoneName[20];
} TDI_ADDRESS_NETONE, *PTDI_ADDRESS_NETONE;

#define TDI_ADDRESS_NETONE_TYPE_UNIQUE  0x0000
#define TDI_ADDRESS_NETONE_TYPE_ROTORED 0x0001

#define TDI_ADDRESS_LENGTH_NETONE   sizeof(TDI_ADDRESS_NETONE)

typedef struct _TDI_ADDRESS_NS
{
    ULONG   NetworkAddress;
    UCHAR   NodeAddress[6];
    USHORT  Socket;
} TDI_ADDRESS_NS, *PTDI_ADDRESS_NS;

#define TDI_ADDRESS_LENGTH_NS   sizeof(TDI_ADDRESS_NS)

#define ISO_MAX_ADDR_LENGTH 64

#define ISO_HIERARCHICAL        0
#define ISO_NON_HIERARCHICAL    1

typedef struct _TDI_ADDRESS_OSI_TSAP
{
    USHORT  tp_addr_type;
    USHORT  tp_taddr_len;
    USHORT  tp_tsel_len;
    UCHAR   tp_addr[ISO_MAX_ADDR_LENGTH];
} TDI_ADDRESS_OSI_TSAP, *PTDI_ADDRESS_OSI_TSAP;

#define TDI_ADDRESS_LENGTH_OSI_TSAP sizeof(TDI_ADDRESS_OSI_TSAP)

typedef struct _TDI_ADDRESS_VNS
{
    UCHAR  net_address[4];
    UCHAR  subnet_addr[2];
    UCHAR  port[2];
    UCHAR  hops;
    UCHAR  filler[5];
} TDI_ADDRESS_VNS, *PTDI_ADDRESS_VNS;

#define TDI_ADDRESS_LENGTH_VNS  sizeof(TDI_ADDRESS_VNS)


/* TDI request structures */

typedef struct _TDI_CONNECT_REQUEST
{
    TDI_REQUEST     Request;
    PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
    PTDI_CONNECTION_INFORMATION ReturnConnectionInformation;
    LARGE_INTEGER   Timeout;
} TDI_REQUEST_CONNECT, *PTDI_REQUEST_CONNECT;

typedef struct _TDI_REQUEST_ACCEPT
{
    TDI_REQUEST Request;
    PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
    PTDI_CONNECTION_INFORMATION ReturnConnectionInformation;
} TDI_REQUEST_ACCEPT, *PTDI_REQUEST_ACCEPT;

typedef struct _TDI_REQUEST_LISTEN
{
    TDI_REQUEST Request;
    PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
    PTDI_CONNECTION_INFORMATION ReturnConnectionInformation;
    USHORT      ListenFlags;
} TDI_REQUEST_LISTEN, *PTDI_REQUEST_LISTEN;

typedef struct _TDI_DISCONNECT_REQUEST
{
    TDI_REQUEST     Request;
    LARGE_INTEGER   Timeout;
} TDI_REQUEST_DISCONNECT, *PTDI_REQUEST_DISCONNECT;

typedef struct _TDI_REQUEST_SEND
{
    TDI_REQUEST Request;
    USHORT      SendFlags;
} TDI_REQUEST_SEND, *PTDI_REQUEST_SEND;

typedef struct _TDI_REQUEST_RECEIVE
{
    TDI_REQUEST Request;
    USHORT      ReceiveFlags;
} TDI_REQUEST_RECEIVE, *PTDI_REQUEST_RECEIVE;

typedef struct _TDI_REQUEST_SEND_DATAGRAM
{
    TDI_REQUEST Request;
    PTDI_CONNECTION_INFORMATION SendDatagramInformation;
} TDI_REQUEST_SEND_DATAGRAM, *PTDI_REQUEST_SEND_DATAGRAM;

typedef struct _TDI_REQUEST_RECEIVE_DATAGRAM
{
    TDI_REQUEST Request;
    PTDI_CONNECTION_INFORMATION ReceiveDatagramInformation;
    PTDI_CONNECTION_INFORMATION ReturnInformation;
    USHORT      ReceiveFlags;
} TDI_REQUEST_RECEIVE_DATAGRAM, *PTDI_REQUEST_RECEIVE_DATAGRAM;

typedef struct _TDI_REQUEST_SET_EVENT
{
    TDI_REQUEST Request;
    LONG        EventType;
    PVOID       EventHandler;
    PVOID       EventContext;
} TDI_REQUEST_SET_EVENT_HANDLER, *PTDI_REQUEST_SET_EVENT_HANDLER;

#define TDI_RECEIVE_BROADCAST           0x00000004
#define TDI_RECEIVE_MULTICAST           0x00000008
#define TDI_RECEIVE_PARTIAL             0x00000010
#define TDI_RECEIVE_NORMAL              0x00000020
#define TDI_RECEIVE_EXPEDITED           0x00000040
#define TDI_RECEIVE_PEEK                0x00000080
#define TDI_RECEIVE_NO_RESPONSE_EXP     0x00000100
#define TDI_RECEIVE_COPY_LOOKAHEAD      0x00000200
#define TDI_RECEIVE_ENTIRE_MESSAGE      0x00000400
#define TDI_RECEIVE_AT_DISPATCH_LEVEL   0x00000800

/* Listen flags */
#define TDI_QUERY_ACCEPT    0x00000001

/* Options used for both SendOptions and ReceiveIndicators */
#define TDI_SEND_EXPEDITED              0x0020
#define TDI_SEND_PARTIAL                0x0040
#define TDI_SEND_NO_RESPONSE_EXPECTED   0x0080
#define TDI_SEND_NON_BLOCKING           0x0100

/* Disconnect Flags */
#define TDI_DISCONNECT_WAIT     0x0001
#define TDI_DISCONNECT_ABORT    0x0002
#define TDI_DISCONNECT_RELEASE  0x0004

/* TdiRequest structure for TdiQueryInformation request */
typedef struct _TDI_REQUEST_QUERY_INFORMATION
{
    TDI_REQUEST Request;
    ULONG       QueryType;
    PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
} TDI_REQUEST_QUERY_INFORMATION, *PTDI_REQUEST_QUERY_INFORMATION;

/* TdiRequest structure for TdiSetInformation request */
typedef struct _TDI_REQUEST_SET_INFORMATION
{
    TDI_REQUEST Request;
    ULONG       SetType;
    PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
} TDI_REQUEST_SET_INFORMATION, *PTDI_REQUEST_SET_INFORMATION;

typedef union _TDI_REQUEST_TYPE
{
    TDI_REQUEST_ACCEPT              TdiAccept;
    TDI_REQUEST_CONNECT             TdiConnect;
    TDI_REQUEST_DISCONNECT          TdiDisconnect;
    TDI_REQUEST_LISTEN              TdiListen;
    TDI_REQUEST_QUERY_INFORMATION   TdiQueryInformation;
    TDI_REQUEST_RECEIVE             TdiReceive;
    TDI_REQUEST_RECEIVE_DATAGRAM    TdiReceiveDatagram;
    TDI_REQUEST_SEND                TdiSend;
    TDI_REQUEST_SEND_DATAGRAM       TdiSendDatagram;
    TDI_REQUEST_SET_EVENT_HANDLER   TdiSetEventHandler;
    TDI_REQUEST_SET_INFORMATION     TdiSetInformation;
} TDI_REQUEST_TYPE, *PTDI_REQUEST_TYPE;



/* Query information types */

/* Generic query info types that must be supported by all transports */
#define TDI_QUERY_BROADCAST_ADDRESS     0x00000001
#define TDI_QUERY_PROVIDER_INFO         0x00000002
#define TDI_QUERY_ADDRESS_INFO          0x00000003
#define TDI_QUERY_CONNECTION_INFO       0x00000004
#define TDI_QUERY_PROVIDER_STATISTICS   0x00000005
#define TDI_QUERY_DATAGRAM_INFO         0x00000006
#define TDI_QUERY_DATA_LINK_ADDRESS     0x00000007
#define TDI_QUERY_NETWORK_ADDRESS       0x00000008
#define TDI_QUERY_MAX_DATAGRAM_INFO     0x00000009

/* Netbios specific query information types */
#define TDI_QUERY_ADAPTER_STATUS        0x00000100
#define TDI_QUERY_SESSION_STATUS        0x00000200
#define TDI_QUERY_FIND_NAME             0x00000300

typedef struct _TA_APPLETALK_ADDR
{
    LONG    TAAddressCount;
    struct  _AddrAtalk {
        USHORT                  AddressLength;
        USHORT                  AddressType;
        TDI_ADDRESS_APPLETALK   Address[1];
    } Address[1];
} TA_APPLETALK_ADDRESS, *PTA_APPLETALK_ADDRESS;

typedef struct _TA_ADDRESS_IP
{
    LONG    TAAddressCount;
    struct  _AddrIp {
        USHORT          AddressLength;
        USHORT          AddressType;
        TDI_ADDRESS_IP  Address[1];
    } Address[1];
} TA_ADDRESS_IP, *PTA_ADDRESS_IP;

typedef struct _TA_ADDRESS_IPX
{
    LONG    TAAddressCount;
    struct  _AddrIpx {
        USHORT          AddressLength;
        USHORT          AddressType;
        TDI_ADDRESS_IP  Address[1];
    } Address[1];
} TA_IPX_ADDRESS, *PTA_IPX_ADDRESS;

typedef struct _TA_NETBIOS_ADDRESS
{
    LONG    TAAddressCount;
    struct _Addr{
        USHORT              AddressLength;
        USHORT              AddressType;
        TDI_ADDRESS_NETBIOS Address[1];
    } Address[1];
} TA_NETBIOS_ADDRESS, *PTA_NETBIOS_ADDRESS;

typedef struct _TA_ADDRESS_NS
{
    LONG    TAAddressCount;
    struct  _AddrNs {
        USHORT          AddressLength;
        USHORT          AddressType;
        TDI_ADDRESS_NS  Address[1];
    } Address[1];
} TA_NS_ADDRESS, *PTA_NS_ADDRESS;

typedef struct _TA_ADDRESS_VNS
{
    LONG    TAAddressCount;
    struct  _AddrVns {
        USHORT          AddressLength;
        USHORT          AddressType;
        TDI_ADDRESS_VNS Address[1];
    } Address[1];
} TA_VNS_ADDRESS, *PTA_VNS_ADDRESS;


/* Structures used for TdiQueryInformation and TdiSetInformation */

typedef struct _TDI_ENDPOINT_INFO
{
    ULONG   State;
    ULONG   Event;
    ULONG   TransmittedTsdus;
    ULONG   ReceivedTsdus;
    ULONG   TransmissionErrors;
    ULONG   ReceiveErrors;
    ULONG   MinimumLookaheadData;
    ULONG   MaximumLookaheadData;
    ULONG   PriorityLevel;
    ULONG   SecurityLevel;
    ULONG   SecurityCompartment;
} TDI_ENDPOINT_INFO, *PTDI_ENDPOINT_INFO;

typedef struct _TDI_CONNECTION_INFO
{
    ULONG           State;
    ULONG           Event; 
    ULONG           TransmittedTsdus;
    ULONG           ReceivedTsdus;
    ULONG           TransmissionErrors;
    ULONG           ReceiveErrors;
    LARGE_INTEGER   Throughput;
    LARGE_INTEGER   Delay;
    ULONG           SendBufferSize;
    ULONG           ReceiveBufferSize;
    BOOLEAN         Unreliable;
} TDI_CONNECTION_INFO, *PTDI_CONNECTION_INFO;

typedef struct _TDI_DATAGRAM_INFO
{
    ULONG   MaximumDatagramBytes;
    ULONG   MaximumDatagramCount;
} TDI_DATAGRAM_INFO, *PTDI_DATAGRAM_INFO;

typedef struct _TDI_MAX_DATAGRAM_INFO
{
    ULONG   MaxDatagramSize;
} TDI_MAX_DATAGRAM_INFO, *PTDI_MAX_DATAGRAM_INFO;

typedef struct _TDI_PROVIDER_INFO
{
    ULONG           Version;
    ULONG           MaxSendSize;
    ULONG           MaxConnectionUserData;
    ULONG           MaxDatagramSize;
    ULONG           ServiceFlags;
    ULONG           MinimumLookaheadData;
    ULONG           MaximumLookaheadData;
    ULONG           NumberOfResources;
    LARGE_INTEGER   StartTime;
} TDI_PROVIDER_INFO, *PTDI_PROVIDER_INFO;

#define TDI_SERVICE_CONNECTION_MODE     0x00000001
#define TDI_SERVICE_ORDERLY_RELEASE     0x00000002
#define TDI_SERVICE_CONNECTIONLESS_MODE 0x00000004
#define TDI_SERVICE_ERROR_FREE_DELIVERY 0x00000008
#define TDI_SERVICE_SECURITY_LEVEL      0x00000010
#define TDI_SERVICE_BROADCAST_SUPPORTED 0x00000020
#define TDI_SERVICE_MULTICAST_SUPPORTED 0x00000040
#define TDI_SERVICE_DELAYED_ACCEPTANCE  0x00000080
#define TDI_SERVICE_EXPEDITED_DATA      0x00000100
#define TDI_SERVICE_INTERNAL_BUFFERING  0x00000200
#define TDI_SERVICE_ROUTE_DIRECTED      0x00000400
#define TDI_SERVICE_NO_ZERO_LENGTH      0x00000800
#define TDI_SERVICE_POINT_TO_POINT      0x00001000
#define TDI_SERVICE_MESSAGE_MODE        0x00002000
#define TDI_SERVICE_HALF_DUPLEX         0x00004000

typedef struct _TDI_PROVIDER_RESOURCE_STATS
{
    ULONG   ResourceId;
    ULONG   MaximumResourceUsed;
    ULONG   AverageResourceUsed;
    ULONG   ResourceExhausted;
} TDI_PROVIDER_RESOURCE_STATS, *PTDI_PROVIDER_RESOURCE_STATS;

typedef struct _TDI_PROVIDER_STATISTICS
{
    ULONG           Version;
    ULONG           OpenConnections;
    ULONG           ConnectionsAfterNoRetry;
    ULONG           ConnectionsAfterRetry;
    ULONG           LocalDisconnects;
    ULONG           RemoteDisconnects;
    ULONG           LinkFailures;
    ULONG           AdapterFailures;
    ULONG           SessionTimeouts;
    ULONG           CancelledConnections;
    ULONG           RemoteResourceFailures;
    ULONG           LocalResourceFailures;
    ULONG           NotFoundFailures;
    ULONG           NoListenFailures;
    ULONG           DatagramsSent;
    LARGE_INTEGER   DatagramBytesSent;
    ULONG           DatagramsReceived;
    LARGE_INTEGER   DatagramBytesReceived;
    ULONG           PacketsSent;
    ULONG           PacketsReceived;
    ULONG           DataFramesSent;
    LARGE_INTEGER   DataFrameBytesSent;
    ULONG           DataFramesReceived;
    LARGE_INTEGER   DataFrameBytesReceived;
    ULONG           DataFramesResent;
    LARGE_INTEGER   DataFrameBytesResent;
    ULONG           DataFramesRejected;
    LARGE_INTEGER   DataFrameBytesRejected;
    ULONG           ResponseTimerExpirations;
    ULONG           AckTimerExpirations;
    ULONG           MaximumSendWindow;
    ULONG           AverageSendWindow;
    ULONG           PiggybackAckQueued;
    ULONG           PiggybackAckTimeouts;
    LARGE_INTEGER   WastedPacketSpace;
    ULONG           WastedSpacePackets;
    ULONG           NumberOfResources;
    TDI_PROVIDER_RESOURCE_STATS Resources[1];
} TDI_PROVIDER_STATISTICS, *PTDI_PROVIDER_STATISTICS;

#endif /* __TDI_H */

/* EOF */
