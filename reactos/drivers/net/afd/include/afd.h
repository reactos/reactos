/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        include/afd.h
 * PURPOSE:     Main driver header
 */
#ifndef __AFD_H
#define __AFD_H

#define NTOS_KERNEL_MODE
#include <ntos.h>
#include <ddk/tdikrnl.h>
#include <ddk/tdiinfo.h>
#include <winsock2.h>
#include <afd/shared.h>
#include <debug.h>

/* Forward declarations */
struct _AFDFCB;

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT StorageDevice;
    KSPIN_LOCK FCBListLock;
    LIST_ENTRY FCBListHead;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* Context Control Block structure */
typedef struct _AFDCCB {
	struct _AFDFCB *FCB;
	LIST_ENTRY      ListEntry;
	PFILE_OBJECT    FileObject;
	ULONG           Flags;
	LARGE_INTEGER   CurrentByteOffset;
} AFDCCB, *PAFDCCB;

/* Flags for CCB structure */
#define	CCB_CLEANED     0x00000001

typedef struct _FsdNTRequiredFCB {
    FSRTL_COMMON_FCB_HEADER CommonFCBHeader;
    SECTION_OBJECT_POINTERS SectionObject;
    ERESOURCE               MainResource;
    ERESOURCE               PagingIoResource;
} FsdNTRequiredFCB, *PFsdNTRequiredFCB;

typedef struct _AFDFCB {
    FsdNTRequiredFCB    NTRequiredFCB;
    LIST_ENTRY          ListEntry;
    BOOL                CommandChannel;
    PDEVICE_EXTENSION   DeviceExt;
    SHARE_ACCESS        ShareAccess;
    ULONG               ReferenceCount;
    ULONG               OpenHandleCount;
    HANDLE              TdiAddressObjectHandle;
    PFILE_OBJECT        TdiAddressObject;
    HANDLE              TdiConnectionObjectHandle;
    PFILE_OBJECT        TdiConnectionObject;
    LIST_ENTRY          CCBListHead;
    INT                 AddressFamily;
    INT                 SocketType;
    INT                 Protocol;
    SOCKADDR            SocketName;
    PVOID               HelperContext;
    DWORD               NotificationEvents;
    UNICODE_STRING      TdiDeviceName;
    DWORD               State;
    PVOID               SendBuffer;
    LIST_ENTRY          ReceiveQueue;
    KSPIN_LOCK          ReceiveQueueLock;
    LIST_ENTRY          ReadRequestQueue;
    KSPIN_LOCK          ReadRequestQueueLock;
    /* For WSAEventSelect() */
    WSANETWORKEVENTS    NetworkEvents;
    WSAEVENT            EventObjects[FD_MAX_EVENTS];
} AFDFCB, *PAFDFCB;

/* Socket states */
#define SOCKET_STATE_CREATED    0
#define SOCKET_STATE_BOUND      1
#define SOCKET_STATE_LISTENING  2
#define SOCKET_STATE_CONNECTED  3

typedef struct _AFD_BUFFER {
  LIST_ENTRY ListEntry;
  WSABUF Buffer;
} AFD_BUFFER, *PAFD_BUFFER;

typedef struct _AFD_READ_REQUEST {
  LIST_ENTRY ListEntry;
  PIRP Irp;
  PFILE_REQUEST_RECVFROM RecvFromRequest;
  PFILE_REPLY_RECVFROM RecvFromReply;
} AFD_READ_REQUEST, *PAFD_READ_REQUEST;

typedef struct IPSNMP_INFO {
	ULONG Forwarding;
	ULONG DefaultTTL;
	ULONG InReceives;
	ULONG InHdrErrors;
	ULONG InAddrErrors;
	ULONG ForwDatagrams;
	ULONG InUnknownProtos;
	ULONG InDiscards;
	ULONG InDelivers;
	ULONG OutRequests;
	ULONG RoutingDiscards;
	ULONG OutDiscards;
	ULONG OutNoRoutes;
	ULONG ReasmTimeout;
	ULONG ReasmReqds;
	ULONG ReasmOks;
	ULONG ReasmFails;
	ULONG FragOks;
	ULONG FragFails;
	ULONG FragCreates;
	ULONG NumIf;
	ULONG NumAddr;
	ULONG NumRoutes;
} IPSNMP_INFO, *PIPSNMP_INFO;

typedef struct IPADDR_ENTRY {
	ULONG  Addr;
	ULONG  Index;
	ULONG  Mask;
	ULONG  BcastAddr;
	ULONG  ReasmSize;
	USHORT Context;
	USHORT Pad;
} IPADDR_ENTRY, *PIPADDR_ENTRY;


#define TL_INSTANCE 0

#define IP_MIB_STATS_ID             0x1
#define IP_MIB_ADDRTABLE_ENTRY_ID   0x102


/* IPv4 header format */
typedef struct IPv4_HEADER {
    UCHAR VerIHL;                /* 4-bit version, 4-bit Internet Header Length */
    UCHAR Tos;                   /* Type of Service */
    USHORT TotalLength;          /* Total Length */
    USHORT Id;                   /* Identification */
    USHORT FlagsFragOfs;         /* 3-bit Flags, 13-bit Fragment Offset */
    UCHAR Ttl;                   /* Time to Live */
    UCHAR Protocol;              /* Protocol */
    USHORT Checksum;             /* Header Checksum */
    ULONG SrcAddr;               /* Source Address */
    ULONG DstAddr;               /* Destination Address */
} IPv4_HEADER, *PIPv4_HEADER;


/* IOCTL codes */

#define IOCTL_TCP_QUERY_INFORMATION_EX \
	    CTL_CODE(FILE_DEVICE_NETWORK, 0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_TCP_SET_INFORMATION_EX  \
	    CTL_CODE(FILE_DEVICE_NETWORK, 1, METHOD_BUFFERED, FILE_WRITE_ACCESS)


#ifdef i386

/* DWORD network to host byte order conversion for i386 */
#define DN2H(dw) \
    ((((dw) & 0xFF000000L) >> 24) | \
	 (((dw) & 0x00FF0000L) >> 8) | \
	 (((dw) & 0x0000FF00L) << 8) | \
	 (((dw) & 0x000000FFL) << 24))

/* DWORD host to network byte order conversion for i386 */
#define DH2N(dw) \
	((((dw) & 0xFF000000L) >> 24) | \
	 (((dw) & 0x00FF0000L) >> 8) | \
	 (((dw) & 0x0000FF00L) << 8) | \
	 (((dw) & 0x000000FFL) << 24))

/* WORD network to host order conversion for i386 */
#define WN2H(w) \
	((((w) & 0xFF00) >> 8) | \
	 (((w) & 0x00FF) << 8))

/* WORD host to network byte order conversion for i386 */
#define WH2N(w) \
	((((w) & 0xFF00) >> 8) | \
	 (((w) & 0x00FF) << 8))

#else /* i386 */

/* DWORD network to host byte order conversion for other architectures */
#define DN2H(dw) \
    (dw)

/* DWORD host to network byte order conversion for other architectures */
#define DH2N(dw) \
    (dw)

/* WORD network to host order conversion for other architectures */
#define WN2H(w) \
    (w)

/* WORD host to network byte order conversion for other architectures */
#define WH2N(w) \
    (w)

#endif /* i386 */


extern NPAGED_LOOKASIDE_LIST BufferLookasideList;
extern NPAGED_LOOKASIDE_LIST ReadRequestLookasideList;


/* Prototypes from dispatch.c */

NTSTATUS AfdDispBind(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispListen(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispSendTo(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispRecvFrom(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispSelect(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispEventSelect(
  PIRP Irp,
  PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispEnumNetworkEvents(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispRecv(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispSend(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispConnect(
  PIRP Irp,
  PIO_STACK_LOCATION IrpSp);

/* Prototypes from event.c */

NTSTATUS AfdRegisterEventHandlers(
    PAFDFCB FCB);

NTSTATUS AfdDeregisterEventHandlers(
    PAFDFCB FCB);

/* Prototypes from opnclose.c */

NTSTATUS STDCALL AfdCreate(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS STDCALL AfdClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

/* Prototypes from rdwr.c */

NTSTATUS AfdEventReceiveDatagramHandler(
    IN PVOID TdiEventContext,
    IN LONG SourceAddressLength,
    IN PVOID SourceAddress,
    IN LONG OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG * BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP * IoRequestPacket);

NTSTATUS STDCALL AfdRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS STDCALL AfdWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

/* Prototypes from routines.c */

ULONG WSABufferSize(
    LPWSABUF Buffers,
    DWORD BufferCount);

NTSTATUS MergeWSABuffers(
    LPWSABUF Buffers,
    DWORD BufferCount,
    PVOID Destination,
    ULONG MaxLength,
    PULONG BytesCopied);

NTSTATUS FillWSABuffers(
    PAFDFCB FCB,
    LPWSABUF Buffers,
    DWORD BufferCount,
    PULONG BytesCopied);

VOID BuildIPv4Header(
    PIPv4_HEADER IPHeader,
    ULONG TotalSize,
    ULONG Protocol,
    PSOCKADDR SourceAddress,
    PSOCKADDR DestinationAddress);

/* Prototypes from tdi.c */

NTSTATUS TdiCloseDevice(
  HANDLE Handle,
  PFILE_OBJECT FileObject);

NTSTATUS TdiOpenAddressFileIPv4(
  PUNICODE_STRING DeviceName,
  LPSOCKADDR Name,
  PHANDLE AddressHandle,
  PFILE_OBJECT *AddressObject);

NTSTATUS TdiOpenAddressFile(
  PUNICODE_STRING DeviceName,
  LPSOCKADDR Name,
  PHANDLE AddressHandle,
  PFILE_OBJECT *AddressObject);

NTSTATUS TdiOpenConnectionEndpointFile(
  PUNICODE_STRING DeviceName,
  PHANDLE ConnectionHandle,
  PFILE_OBJECT *ConnectionObject);

NTSTATUS TdiConnect(
  PFILE_OBJECT ConnectionObject,
  LPSOCKADDR RemoteAddress);

NTSTATUS TdiAssociateAddressFile(
  HANDLE AddressHandle,
  PFILE_OBJECT ConnectionObject);

NTSTATUS TdiSetEventHandler(
  PFILE_OBJECT FileObject,
  LONG EventType,
  PVOID Handler,
  PVOID Context);

NTSTATUS TdiQueryDeviceControl(
  PFILE_OBJECT FileObject,
  ULONG IoControlCode,
  PVOID InputBuffer,
  ULONG InputBufferLength,
  PVOID OutputBuffer,
  ULONG OutputBufferLength,
  PULONG Return);

NTSTATUS TdiQueryInformationEx(
  PFILE_OBJECT FileObject,
  ULONG Entity,
  ULONG Instance,
  ULONG Class,
  ULONG Type,
  ULONG Id,
  PVOID OutputBuffer,
  PULONG OutputLength);

NTSTATUS TdiQueryAddress(
  PFILE_OBJECT FileObject,
  PULONG Address);

NTSTATUS TdiSend(
  PFILE_OBJECT TransportObject,
  PVOID Buffer,
  ULONG BufferSize);

NTSTATUS TdiSendDatagram(
  PFILE_OBJECT TransportObject,
  LPSOCKADDR Address,
  PMDL Mdl,
  ULONG BufferSize);

#endif /*__AFD_H */

/* EOF */
