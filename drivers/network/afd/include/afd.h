/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/include/afd.h
 * PURPOSE:          Ancillary functions driver -- constants and structures
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040630 Created
 */

#ifndef _AFD_H
#define _AFD_H

#include <ntifs.h>
#include <ndk/obtypes.h>
#include <tdi.h>
#include <tcpioctl.h>
#define _WINBASE_
#define _WINDOWS_H
#define _INC_WINDOWS
#include <windef.h>
#include <winsock2.h>
#include <afd/shared.h>
#include <pseh/pseh2.h>

#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif

#define TL_INSTANCE 0
#define	IP_MIB_STATS_ID 1
#define	IP_MIB_ADDRTABLE_ENTRY_ID 0x102

#define TAG_AFD_DATA_BUFFER                'BdfA'
#define TAG_AFD_TRANSPORT_ADDRESS          'tdfA'
#define TAG_AFD_SOCKET_CONTEXT             'XdfA'
#define TAG_AFD_CONNECT_DATA               'cdfA'
#define TAG_AFD_DISCONNECT_DATA            'ddfA'

#define TAG_AFD_CONNECT_OPTIONS            'ocfA'
#define TAG_AFD_DISCONNECT_OPTIONS         'odfA'
#define TAG_AFD_ACCEPT_QUEUE               'qafA'
#define TAG_AFD_POLL_HANDLE                'hpfA'
#define TAG_AFD_FCB                        'cffA'
#define TAG_AFD_ACTIVE_POLL                'pafA'
#define TAG_AFD_EA_INFO                    'aefA'
#define TAG_AFD_STORED_DATAGRAM            'gsfA'
#define TAG_AFD_SNMP_ADDRESS_INFO          'asfA'
#define TAG_AFD_TDI_CONNECTION_INFORMATION 'cTfA'
#define TAG_AFD_WSA_BUFFER                 'bWfA'

typedef struct IPADDR_ENTRY {
	ULONG  Addr;
	ULONG  Index;
	ULONG  Mask;
	ULONG  BcastAddr;
	ULONG  ReasmSize;
	USHORT Context;
	USHORT Pad;
} IPADDR_ENTRY, *PIPADDR_ENTRY;

#define DN2H(dw) \
    ((((dw) & 0xFF000000L) >> 24) | \
	 (((dw) & 0x00FF0000L) >> 8) | \
	 (((dw) & 0x0000FF00L) << 8) | \
	 (((dw) & 0x000000FFL) << 24))

#define SOCKET_STATE_INVALID_TRANSITION ((DWORD)-1)
#define SOCKET_STATE_CREATED            0
#define SOCKET_STATE_BOUND              1
#define SOCKET_STATE_CONNECTING         2
#define SOCKET_STATE_CONNECTED          3
#define SOCKET_STATE_LISTENING          4
#define SOCKET_STATE_MASK               0x0000ffff
#define SOCKET_STATE_EOF_READ           0x20000000
#define SOCKET_STATE_LOCKED             0x40000000
#define SOCKET_STATE_NEW                0x80000000
#define SOCKET_STATE_CLOSED             0x00000100

#define FUNCTION_CONNECT                0
#define FUNCTION_RECV                   1
#define FUNCTION_SEND                   2
#define FUNCTION_PREACCEPT              3
#define FUNCTION_ACCEPT                 4
#define FUNCTION_DISCONNECT             5
#define FUNCTION_CLOSE                  6
#define MAX_FUNCTIONS                   7

#define IN_FLIGHT_REQUESTS              5

#define EXTRA_LOCK_BUFFERS              2 /* Number of extra buffers needed
					   * for ancillary data on packet
					   * requests. */

/* XXX This is a hack we should clean up later
 * We do this in order to get some storage for the locked handle table
 * Maybe I'll use some tail item in the irp instead */
#define AFD_HANDLES(x) ((PAFD_HANDLE)(x)->Exclusive)
#define SET_AFD_HANDLES(x,y) (((x)->Exclusive) = (ULONG_PTR)(y))

typedef struct _AFD_MAPBUF {
    PVOID BufferAddress;
    PMDL  Mdl;
} AFD_MAPBUF, *PAFD_MAPBUF;

typedef struct _AFD_DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;
    LIST_ENTRY Polls;
    KSPIN_LOCK Lock;
} AFD_DEVICE_EXTENSION, *PAFD_DEVICE_EXTENSION;

typedef struct _AFD_ACTIVE_POLL {
    LIST_ENTRY ListEntry;
    PIRP Irp;
    PAFD_DEVICE_EXTENSION DeviceExt;
    KDPC TimeoutDpc;
    KTIMER Timer;
    PKEVENT EventObject;
    BOOLEAN Exclusive;
} AFD_ACTIVE_POLL, *PAFD_ACTIVE_POLL;

typedef struct _IRP_LIST {
    LIST_ENTRY ListEntry;
    PIRP Irp;
} IRP_LIST, *PIRP_LIST;

typedef struct _AFD_TDI_OBJECT {
    PFILE_OBJECT Object;
    HANDLE Handle;
} AFD_TDI_OBJECT, *PAFD_TDI_OBJECT;

typedef struct _AFD_TDI_OBJECT_QELT {
    LIST_ENTRY ListEntry;
    UINT Seq;
    PTDI_CONNECTION_INFORMATION ConnInfo;
    AFD_TDI_OBJECT Object;
} AFD_TDI_OBJECT_QELT, *PAFD_TDI_OBJECT_QELT;

typedef struct _AFD_IN_FLIGHT_REQUEST {
    PIRP InFlightRequest;
    PTDI_CONNECTION_INFORMATION ConnectionCallInfo;
    PTDI_CONNECTION_INFORMATION ConnectionReturnInfo;
} AFD_IN_FLIGHT_REQUEST, *PAFD_IN_FLIGHT_REQUEST;

typedef struct _AFD_DATA_WINDOW {
    PCHAR Window;
    UINT BytesUsed, Size, Content;
} AFD_DATA_WINDOW, *PAFD_DATA_WINDOW;

typedef struct _AFD_STORED_DATAGRAM {
    LIST_ENTRY ListEntry;
    UINT Len;
    PTRANSPORT_ADDRESS Address;
    CHAR Buffer[1];
} AFD_STORED_DATAGRAM, *PAFD_STORED_DATAGRAM;

typedef struct _AFD_FCB {
    BOOLEAN Locked, Critical, Overread, NonBlocking, OobInline, TdiReceiveClosed, SendClosed;
    UINT State, Flags, GroupID, GroupType;
    KIRQL OldIrql;
    UINT LockCount;
    PVOID CurrentThread;
    PFILE_OBJECT FileObject;
    PAFD_DEVICE_EXTENSION DeviceExt;
    BOOLEAN DelayedAccept;
    UINT ConnSeq;
    USHORT DisconnectFlags;
    BOOLEAN DisconnectPending;
    LARGE_INTEGER DisconnectTimeout;
    PTRANSPORT_ADDRESS LocalAddress, RemoteAddress;
    PTDI_CONNECTION_INFORMATION AddressFrom, ConnectCallInfo, ConnectReturnInfo;
    AFD_TDI_OBJECT AddressFile, Connection;
    AFD_IN_FLIGHT_REQUEST ConnectIrp, ListenIrp, ReceiveIrp, SendIrp, DisconnectIrp;
    AFD_DATA_WINDOW Send, Recv;
    KMUTEX Mutex;
    PKEVENT EventSelect;
    DWORD EventSelectTriggers;
    DWORD EventSelectDisabled;
    UNICODE_STRING TdiDeviceName;
    PVOID Context;
    DWORD PollState;
    NTSTATUS PollStatus[FD_MAX_EVENTS];
    NTSTATUS LastReceiveStatus;
    UINT ContextSize;
    PVOID ConnectData;
    UINT FilledConnectData;
    UINT ConnectDataSize;
    PVOID DisconnectData;
    UINT FilledDisconnectData;
    UINT DisconnectDataSize;
    PVOID ConnectOptions;
    UINT FilledConnectOptions;
    UINT ConnectOptionsSize;
    PVOID DisconnectOptions;
    UINT FilledDisconnectOptions;
    UINT DisconnectOptionsSize;
    LIST_ENTRY PendingIrpList[MAX_FUNCTIONS];
    LIST_ENTRY DatagramList;
    LIST_ENTRY PendingConnections;
} AFD_FCB, *PAFD_FCB;

/* bind.c */

NTSTATUS WarmSocketForBind( PAFD_FCB FCB, ULONG ShareType );
NTSTATUS NTAPI
AfdBindSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	      PIO_STACK_LOCATION IrpSp);

/* connect.c */

NTSTATUS MakeSocketIntoConnection( PAFD_FCB FCB );
NTSTATUS WarmSocketForConnection( PAFD_FCB FCB );
NTSTATUS NTAPI
AfdStreamSocketConnect(PDEVICE_OBJECT DeviceObject, PIRP Irp,
		       PIO_STACK_LOCATION IrpSp);
NTSTATUS NTAPI
AfdGetConnectData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	          PIO_STACK_LOCATION IrpSp);
NTSTATUS NTAPI
AfdSetConnectData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                  PIO_STACK_LOCATION IrpSp);
NTSTATUS NTAPI
AfdSetConnectDataSize(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                      PIO_STACK_LOCATION IrpSp);
NTSTATUS NTAPI
AfdGetConnectOptions(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	             PIO_STACK_LOCATION IrpSp);
NTSTATUS NTAPI
AfdSetConnectOptions(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                     PIO_STACK_LOCATION IrpSp);
NTSTATUS NTAPI
AfdSetConnectOptionsSize(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                         PIO_STACK_LOCATION IrpSp);

/* context.c */

NTSTATUS NTAPI
AfdGetContext( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	       PIO_STACK_LOCATION IrpSp );
NTSTATUS NTAPI
AfdGetContextSize( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	           PIO_STACK_LOCATION IrpSp );
NTSTATUS NTAPI
AfdSetContext( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	       PIO_STACK_LOCATION IrpSp );

/* info.c */

NTSTATUS NTAPI
AfdGetInfo( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	    PIO_STACK_LOCATION IrpSp );

NTSTATUS NTAPI
AfdSetInfo( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	    PIO_STACK_LOCATION IrpSp );

NTSTATUS NTAPI
AfdGetSockName( PDEVICE_OBJECT DeviceObject, PIRP Irp,
                PIO_STACK_LOCATION IrpSp );

NTSTATUS NTAPI
AfdGetPeerName( PDEVICE_OBJECT DeviceObject, PIRP Irp,
                PIO_STACK_LOCATION IrpSp );

/* listen.c */
NTSTATUS AfdWaitForListen( PDEVICE_OBJECT DeviceObject, PIRP Irp,
			   PIO_STACK_LOCATION IrpSp );

NTSTATUS AfdListenSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			 PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdAccept( PDEVICE_OBJECT DeviceObject, PIRP Irp,
		    PIO_STACK_LOCATION IrpSp );

/* lock.c */

PAFD_WSABUF LockBuffers( PAFD_WSABUF Buf, UINT Count,
			 PVOID AddressBuf, PINT AddressLen,
			 BOOLEAN Write, BOOLEAN LockAddress,
             KPROCESSOR_MODE LockMode );
VOID UnlockBuffers( PAFD_WSABUF Buf, UINT Count, BOOL Address );
BOOLEAN SocketAcquireStateLock( PAFD_FCB FCB );
NTSTATUS NTAPI UnlockAndMaybeComplete
( PAFD_FCB FCB, NTSTATUS Status, PIRP Irp,
  UINT Information );
VOID SocketStateUnlock( PAFD_FCB FCB );
NTSTATUS LostSocket( PIRP Irp );
PAFD_HANDLE LockHandles( PAFD_HANDLE HandleArray, UINT HandleCount );
VOID UnlockHandles( PAFD_HANDLE HandleArray, UINT HandleCount );
PVOID LockRequest( PIRP Irp, PIO_STACK_LOCATION IrpSp, BOOLEAN Output, KPROCESSOR_MODE *LockMode );
VOID UnlockRequest( PIRP Irp, PIO_STACK_LOCATION IrpSp );
PVOID GetLockedData( PIRP Irp, PIO_STACK_LOCATION IrpSp );
NTSTATUS LeaveIrpUntilLater( PAFD_FCB FCB, PIRP Irp, UINT Function );
NTSTATUS QueueUserModeIrp(PAFD_FCB FCB, PIRP Irp, UINT Function);

/* main.c */

VOID OskitDumpBuffer( PCHAR Buffer, UINT Len );
VOID DestroySocket( PAFD_FCB FCB );
DRIVER_CANCEL AfdCancelHandler;
VOID RetryDisconnectCompletion(PAFD_FCB FCB);
BOOLEAN CheckUnlockExtraBuffers(PAFD_FCB FCB, PIO_STACK_LOCATION IrpSp);

/* read.c */

IO_COMPLETION_ROUTINE ReceiveComplete;

IO_COMPLETION_ROUTINE PacketSocketRecvComplete;

NTSTATUS NTAPI
AfdConnectedSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION IrpSp, BOOLEAN Short);
NTSTATUS NTAPI
AfdPacketSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			PIO_STACK_LOCATION IrpSp );

/* select.c */

NTSTATUS NTAPI
AfdSelect( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	   PIO_STACK_LOCATION IrpSp );
NTSTATUS NTAPI
AfdEventSelect( PDEVICE_OBJECT DeviceObject, PIRP Irp,
		PIO_STACK_LOCATION IrpSp );
NTSTATUS NTAPI
AfdEnumEvents( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	       PIO_STACK_LOCATION IrpSp );
VOID PollReeval( PAFD_DEVICE_EXTENSION DeviceObject, PFILE_OBJECT FileObject );
VOID KillSelectsForFCB( PAFD_DEVICE_EXTENSION DeviceExt,
                        PFILE_OBJECT FileObject, BOOLEAN ExclusiveOnly );
VOID ZeroEvents( PAFD_HANDLE HandleArray,
		 UINT HandleCount );
VOID SignalSocket(
   PAFD_ACTIVE_POLL Poll OPTIONAL, PIRP _Irp OPTIONAL,
   PAFD_POLL_INFO PollReq, NTSTATUS Status);

/* tdi.c */

NTSTATUS TdiOpenAddressFile(
    PUNICODE_STRING DeviceName,
    PTRANSPORT_ADDRESS Name,
    ULONG ShareType,
    PHANDLE AddressHandle,
    PFILE_OBJECT *AddressObject);

NTSTATUS TdiAssociateAddressFile(
  HANDLE AddressHandle,
  PFILE_OBJECT ConnectionObject);

NTSTATUS TdiDisassociateAddressFile(
  PFILE_OBJECT ConnectionObject);

NTSTATUS TdiListen
( PIRP *Irp,
  PFILE_OBJECT ConnectionObject,
  PTDI_CONNECTION_INFORMATION *RequestConnectionInfo,
  PTDI_CONNECTION_INFORMATION *ReturnConnectionInfo,
  PIO_COMPLETION_ROUTINE  CompletionRoutine,
  PVOID CompletionContext);

NTSTATUS TdiReceive
( PIRP *Irp,
  PFILE_OBJECT ConnectionObject,
  USHORT Flags,
  PCHAR Buffer,
  UINT BufferLength,
  PIO_COMPLETION_ROUTINE  CompletionRoutine,
  PVOID CompletionContext);

NTSTATUS TdiSend
( PIRP *Irp,
  PFILE_OBJECT ConnectionObject,
  USHORT Flags,
  PCHAR Buffer,
  UINT BufferLength,
  PIO_COMPLETION_ROUTINE  CompletionRoutine,
  PVOID CompletionContext);

NTSTATUS TdiReceiveDatagram(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION From,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

NTSTATUS TdiSendDatagram(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION To,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

NTSTATUS TdiQueryMaxDatagramLength(
        PFILE_OBJECT FileObject,
        PUINT MaxDatagramLength);

/* write.c */

NTSTATUS NTAPI
AfdConnectedSocketWriteData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			    PIO_STACK_LOCATION IrpSp, BOOLEAN Short);
NTSTATUS NTAPI
AfdPacketSocketWriteData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			 PIO_STACK_LOCATION IrpSp);

#endif /* _AFD_H */
