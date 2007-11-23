/* $Id$
 *
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

#include <ntddk.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#include <string.h>
#define _WINBASE_
#define _WINDOWS_H
#define _INC_WINDOWS
#include <windef.h>
#include <winsock2.h>
#include <afd/shared.h>

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif

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
#define FUNCTION_CLOSE                  5
#define MAX_FUNCTIONS                   6

#define IN_FLIGHT_REQUESTS              3

#define EXTRA_LOCK_BUFFERS              2 /* Number of extra buffers needed
					   * for ancillary data on packet
					   * requests. */

#define DEFAULT_SEND_WINDOW_SIZE        16384
#define DEFAULT_RECEIVE_WINDOW_SIZE     16384

#define SGID_CONNECTIONLESS             1 /* XXX Find this flag */

/* XXX This is a hack we should clean up later
 * We do this in order to get some storage for the locked handle table
 * Maybe I'll use some tail item in the irp instead */
#define AFD_HANDLES(x) ((PAFD_HANDLE)(x)->Exclusive)
#define SET_AFD_HANDLES(x,y) (((x)->Exclusive) = (ULONG)(y))

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
    IO_STATUS_BLOCK Iosb;
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
    BOOLEAN Locked, Critical, Overread;
    UINT State, Flags;
    KIRQL OldIrql;
    UINT LockCount;
    PVOID CurrentThread;
    KSPIN_LOCK SpinLock;
    PFILE_OBJECT FileObject;
    PAFD_DEVICE_EXTENSION DeviceExt;
    BOOLEAN DelayedAccept, NeedsNewListen;
    UINT ConnSeq;
    PTRANSPORT_ADDRESS LocalAddress, RemoteAddress;
    PTDI_CONNECTION_INFORMATION AddressFrom;
    AFD_TDI_OBJECT AddressFile, Connection;
    AFD_IN_FLIGHT_REQUEST ConnectIrp, ListenIrp, ReceiveIrp, SendIrp;
    AFD_DATA_WINDOW Send, Recv;
    FAST_MUTEX Mutex;
    KEVENT StateLockedEvent;
    PKEVENT EventSelect;
    DWORD EventSelectTriggers;
    DWORD EventsFired;
    UNICODE_STRING TdiDeviceName;
    PVOID Context;
    DWORD PollState;
    UINT ContextSize;
    PIRP PendingClose;
    LIST_ENTRY PendingIrpList[MAX_FUNCTIONS];
    LIST_ENTRY DatagramList;
    LIST_ENTRY PendingConnections;
} AFD_FCB, *PAFD_FCB;

/* bind.c */

NTSTATUS WarmSocketForBind( PAFD_FCB FCB );
NTSTATUS STDCALL
AfdBindSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	      PIO_STACK_LOCATION IrpSp);

/* connect.c */

NTSTATUS MakeSocketIntoConnection( PAFD_FCB FCB );
NTSTATUS WarmSocketForConnection( PAFD_FCB FCB );
NTSTATUS STDCALL
AfdStreamSocketConnect(PDEVICE_OBJECT DeviceObject, PIRP Irp,
		       PIO_STACK_LOCATION IrpSp);

/* context.c */

NTSTATUS STDCALL
AfdGetContext( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	       PIO_STACK_LOCATION IrpSp );
NTSTATUS STDCALL
AfdSetContext( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	       PIO_STACK_LOCATION IrpSp );

/* info.c */

NTSTATUS STDCALL
AfdGetInfo( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	    PIO_STACK_LOCATION IrpSp );

NTSTATUS STDCALL
AfdGetSockOrPeerName( PDEVICE_OBJECT DeviceObject, PIRP Irp,
                      PIO_STACK_LOCATION IrpSp, BOOLEAN Local );

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
			 BOOLEAN Write, BOOLEAN LockAddress );
VOID UnlockBuffers( PAFD_WSABUF Buf, UINT Count, BOOL Address );
UINT SocketAcquireStateLock( PAFD_FCB FCB );
NTSTATUS NTAPI UnlockAndMaybeComplete
( PAFD_FCB FCB, NTSTATUS Status, PIRP Irp,
  UINT Information,
  PIO_COMPLETION_ROUTINE Completion,
  BOOL ShouldUnlockIrp );
VOID SocketStateUnlock( PAFD_FCB FCB );
NTSTATUS LostSocket( PIRP Irp, BOOL ShouldUnlockIrp );
PAFD_HANDLE LockHandles( PAFD_HANDLE HandleArray, UINT HandleCount );
VOID UnlockHandles( PAFD_HANDLE HandleArray, UINT HandleCount );
PVOID LockRequest( PIRP Irp, PIO_STACK_LOCATION IrpSp );
VOID UnlockRequest( PIRP Irp, PIO_STACK_LOCATION IrpSp );
VOID SocketCalloutEnter( PAFD_FCB FCB );
VOID SocketCalloutLeave( PAFD_FCB FCB );

/* main.c */

VOID OskitDumpBuffer( PCHAR Buffer, UINT Len );
NTSTATUS LeaveIrpUntilLater( PAFD_FCB FCB, PIRP Irp, UINT Function );
VOID DestroySocket( PAFD_FCB FCB );

/* read.c */

NTSTATUS NTAPI ReceiveComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context );

NTSTATUS NTAPI PacketSocketRecvComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context );

NTSTATUS STDCALL
AfdConnectedSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			   PIO_STACK_LOCATION IrpSp, BOOLEAN Short);
NTSTATUS STDCALL
AfdPacketSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			PIO_STACK_LOCATION IrpSp );

/* select.c */

NTSTATUS STDCALL
AfdSelect( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	   PIO_STACK_LOCATION IrpSp );
NTSTATUS STDCALL
AfdEventSelect( PDEVICE_OBJECT DeviceObject, PIRP Irp,
		PIO_STACK_LOCATION IrpSp );
NTSTATUS STDCALL
AfdEnumEvents( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	       PIO_STACK_LOCATION IrpSp );
VOID PollReeval( PAFD_DEVICE_EXTENSION DeviceObject, PFILE_OBJECT FileObject );
VOID KillSelectsForFCB( PAFD_DEVICE_EXTENSION DeviceExt,
                        PFILE_OBJECT FileObject, BOOLEAN ExclusiveOnly );

/* tdi.c */

NTSTATUS TdiOpenAddressFile(
    PUNICODE_STRING DeviceName,
    PTRANSPORT_ADDRESS Name,
    PHANDLE AddressHandle,
    PFILE_OBJECT *AddressObject);

NTSTATUS TdiAssociateAddressFile(
  HANDLE AddressHandle,
  PFILE_OBJECT ConnectionObject);

NTSTATUS TdiListen
( PIRP *Irp,
  PFILE_OBJECT ConnectionObject,
  PTDI_CONNECTION_INFORMATION *RequestConnectionInfo,
  PTDI_CONNECTION_INFORMATION *ReturnConnectionInfo,
  PIO_STATUS_BLOCK Iosb,
  PIO_COMPLETION_ROUTINE  CompletionRoutine,
  PVOID CompletionContext);

NTSTATUS TdiReceive
( PIRP *Irp,
  PFILE_OBJECT ConnectionObject,
  USHORT Flags,
  PCHAR Buffer,
  UINT BufferLength,
  PIO_STATUS_BLOCK Iosb,
  PIO_COMPLETION_ROUTINE  CompletionRoutine,
  PVOID CompletionContext);

NTSTATUS TdiSend
( PIRP *Irp,
  PFILE_OBJECT ConnectionObject,
  USHORT Flags,
  PCHAR Buffer,
  UINT BufferLength,
  PIO_STATUS_BLOCK Iosb,
  PIO_COMPLETION_ROUTINE  CompletionRoutine,
  PVOID CompletionContext);

NTSTATUS TdiReceiveDatagram(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION From,
    PIO_STATUS_BLOCK Iosb,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

NTSTATUS TdiSendDatagram(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION To,
    PIO_STATUS_BLOCK Iosb,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

/* write.c */

NTSTATUS STDCALL
AfdConnectedSocketWriteData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			    PIO_STACK_LOCATION IrpSp, BOOLEAN Short);
NTSTATUS STDCALL
AfdPacketSocketWriteData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			 PIO_STACK_LOCATION IrpSp);

#endif/*_AFD_H*/
