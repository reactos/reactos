/*
 * tdikrnl.h
 *
 * TDI kernel mode definitions
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __TDIKRNL_H
#define __TDIKRNL_H

#include "tdi.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_TDI_)
#define TDIKRNLAPI
#else
#define TDIKRNLAPI DECLSPEC_IMPORT
#endif


typedef struct _TDI_REQUEST_KERNEL {
  ULONG RequestFlags;
  PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
  PTDI_CONNECTION_INFORMATION ReturnConnectionInformation;
  PVOID RequestSpecific;
} TDI_REQUEST_KERNEL, *PTDI_REQUEST_KERNEL;

/* Request codes */
#define TDI_ASSOCIATE_ADDRESS             0x01
#define TDI_DISASSOCIATE_ADDRESS          0x02
#define TDI_CONNECT                       0x03
#define TDI_LISTEN                        0x04
#define TDI_ACCEPT                        0x05
#define TDI_DISCONNECT                    0x06
#define TDI_SEND                          0x07
#define TDI_RECEIVE                       0x08
#define TDI_SEND_DATAGRAM                 0x09
#define TDI_RECEIVE_DATAGRAM              0x0A
#define TDI_SET_EVENT_HANDLER             0x0B
#define TDI_QUERY_INFORMATION             0x0C
#define TDI_SET_INFORMATION               0x0D
#define TDI_ACTION                        0x0E

#define TDI_DIRECT_SEND                   0x27
#define TDI_DIRECT_SEND_DATAGRAM          0x29

#define TDI_TRANSPORT_ADDRESS_FILE        1
#define TDI_CONNECTION_FILE               2
#define TDI_CONTROL_CHANNEL_FILE          3

/* Internal TDI IOCTLS */
#define IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER   _TDI_CONTROL_CODE(0x80, METHOD_NEITHER)
#define IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER _TDI_CONTROL_CODE(0x81, METHOD_NEITHER)

/* TdiAssociateAddress */
typedef struct _TDI_REQUEST_KERNEL_ASSOCIATE {
  HANDLE AddressHandle;
} TDI_REQUEST_KERNEL_ASSOCIATE, *PTDI_REQUEST_KERNEL_ASSOCIATE;

/* TdiDisassociateAddress */
typedef TDI_REQUEST_KERNEL TDI_REQUEST_KERNEL_DISASSOCIATE,
  *PTDI_REQUEST_KERNEL_DISASSOCIATE;

/* TdiAccept */
typedef struct _TDI_REQUEST_KERNEL_ACCEPT {
  PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
  PTDI_CONNECTION_INFORMATION ReturnConnectionInformation;
} TDI_REQUEST_KERNEL_ACCEPT, *PTDI_REQUEST_KERNEL_ACCEPT;

/* TdiConnect */
typedef TDI_REQUEST_KERNEL TDI_REQUEST_KERNEL_CONNECT,
  *PTDI_REQUEST_KERNEL_CONNECT;

/* TdiDisconnect */
typedef TDI_REQUEST_KERNEL TDI_REQUEST_KERNEL_DISCONNECT,
  *PTDI_REQUEST_KERNEL_DISCONNECT;

/* TdiListen */
typedef TDI_REQUEST_KERNEL TDI_REQUEST_KERNEL_LISTEN,
  *PTDI_REQUEST_KERNEL_LISTEN;

/* TdiReceive */
typedef struct _TDI_REQUEST_KERNEL_RECEIVE {
  ULONG ReceiveLength;
  ULONG ReceiveFlags;
} TDI_REQUEST_KERNEL_RECEIVE, *PTDI_REQUEST_KERNEL_RECEIVE;

/* TdiReceiveDatagram */
typedef struct _TDI_REQUEST_KERNEL_RECEIVEDG {
  ULONG ReceiveLength;
  PTDI_CONNECTION_INFORMATION ReceiveDatagramInformation;
  PTDI_CONNECTION_INFORMATION ReturnDatagramInformation;
  ULONG ReceiveFlags;
} TDI_REQUEST_KERNEL_RECEIVEDG, *PTDI_REQUEST_KERNEL_RECEIVEDG;

/* TdiSend */
typedef struct _TDI_REQUEST_KERNEL_SEND {
  ULONG SendLength;
  ULONG SendFlags;
} TDI_REQUEST_KERNEL_SEND, *PTDI_REQUEST_KERNEL_SEND;

/* TdiSendDatagram */
typedef struct _TDI_REQUEST_KERNEL_SENDDG {
  ULONG SendLength;
  _Field_size_bytes_(SendLength) PTDI_CONNECTION_INFORMATION SendDatagramInformation;
} TDI_REQUEST_KERNEL_SENDDG, *PTDI_REQUEST_KERNEL_SENDDG;

/* TdiSetEventHandler */
typedef struct _TDI_REQUEST_KERNEL_SET_EVENT {
  LONG EventType;
  PVOID EventHandler;
  PVOID EventContext;
} TDI_REQUEST_KERNEL_SET_EVENT, *PTDI_REQUEST_KERNEL_SET_EVENT;

/* TdiQueryInformation */
typedef struct _TDI_REQUEST_KERNEL_QUERY_INFO {
  LONG QueryType;
  PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
} TDI_REQUEST_KERNEL_QUERY_INFORMATION, *PTDI_REQUEST_KERNEL_QUERY_INFORMATION;

/* TdiSetInformation */
typedef struct _TDI_REQUEST_KERNEL_SET_INFO {
  LONG SetType;
  PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
} TDI_REQUEST_KERNEL_SET_INFORMATION, *PTDI_REQUEST_KERNEL_SET_INFORMATION;

/* Event types */
#define TDI_EVENT_CONNECT                   0
#define TDI_EVENT_DISCONNECT                1
#define TDI_EVENT_ERROR                     2
#define TDI_EVENT_RECEIVE                   3
#define TDI_EVENT_RECEIVE_DATAGRAM          4
#define TDI_EVENT_RECEIVE_EXPEDITED         5
#define TDI_EVENT_SEND_POSSIBLE             6
#define TDI_EVENT_CHAINED_RECEIVE           7
#define TDI_EVENT_CHAINED_RECEIVE_DATAGRAM  8
#define TDI_EVENT_CHAINED_RECEIVE_EXPEDITED 9
#define TDI_EVENT_ERROR_EX                  10

typedef NTSTATUS
(NTAPI *PTDI_IND_CONNECT)(
  _In_opt_ PVOID TdiEventContext,
  _In_ LONG RemoteAddressLength,
  _In_reads_bytes_(RemoteAddressLength) PVOID RemoteAddress,
  _In_ LONG UserDataLength,
  _In_reads_bytes_opt_(UserDataLength) PVOID UserData,
  _In_ LONG OptionsLength,
  _In_reads_bytes_opt_(OptionsLength) PVOID Options,
  _Out_ CONNECTION_CONTEXT *ConnectionContext,
  _Out_ PIRP *AcceptIrp);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiDefaultConnectHandler(
  _In_opt_ PVOID TdiEventContext,
  _In_ LONG RemoteAddressLength,
  _In_reads_bytes_(RemoteAddressLength) PVOID RemoteAddress,
  _In_ LONG UserDataLength,
  _In_reads_bytes_opt_(UserDataLength) PVOID UserData,
  _In_ LONG OptionsLength,
  _In_reads_bytes_opt_(OptionsLength) PVOID Options,
  _Out_ CONNECTION_CONTEXT *ConnectionContext,
  _Out_ PIRP *AcceptIrp);

typedef NTSTATUS
(NTAPI *PTDI_IND_DISCONNECT)(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ CONNECTION_CONTEXT ConnectionContext,
  _In_ LONG DisconnectDataLength,
  _In_reads_bytes_opt_(DisconnectDataLength) PVOID DisconnectData,
  _In_ LONG DisconnectInformationLength,
  _In_reads_bytes_opt_(DisconnectInformationLength) PVOID DisconnectInformation,
  _In_ ULONG DisconnectFlags);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiDefaultDisconnectHandler(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ CONNECTION_CONTEXT ConnectionContext,
  _In_ LONG DisconnectDataLength,
  _In_reads_bytes_opt_(DisconnectDataLength) PVOID DisconnectData,
  _In_ LONG DisconnectInformationLength,
  _In_reads_bytes_opt_(DisconnectInformationLength) PVOID DisconnectInformation,
  _In_ ULONG DisconnectFlags);

typedef NTSTATUS
(NTAPI *PTDI_IND_ERROR)(
  _In_opt_ PVOID TdiEventContext,
  _In_ NTSTATUS Status);

typedef NTSTATUS
(NTAPI *PTDI_IND_ERROR_EX)(
  _In_opt_ PVOID TdiEventContext,
  _In_ NTSTATUS Status,
  _In_ PVOID Buffer);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiDefaultErrorHandler(
  _In_opt_ PVOID TdiEventContext,
  _In_ NTSTATUS Status);

typedef NTSTATUS
(NTAPI *PTDI_IND_RECEIVE)(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ CONNECTION_CONTEXT ConnectionContext,
  _In_ ULONG ReceiveFlags,
  _In_ ULONG BytesIndicated,
  _In_ ULONG BytesAvailable,
  _Out_ ULONG *BytesTaken,
  _In_ PVOID Tsdu,
  _Out_opt_ PIRP *IoRequestPacket);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiDefaultReceiveHandler(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ CONNECTION_CONTEXT ConnectionContext,
  _In_ ULONG ReceiveFlags,
  _In_ ULONG BytesIndicated,
  _In_ ULONG BytesAvailable,
  _Out_ ULONG *BytesTaken,
  _In_ PVOID Tsdu,
  _Out_opt_ PIRP *IoRequestPacket);

typedef NTSTATUS
(NTAPI *PTDI_IND_RECEIVE_DATAGRAM)(
  _In_opt_ PVOID TdiEventContext,
  _In_ LONG SourceAddressLength,
  _In_reads_bytes_(SourceAddressLength) PVOID SourceAddress,
  _In_ LONG OptionsLength,
  _In_reads_bytes_opt_(OptionsLength) PVOID Options,
  _In_ ULONG ReceiveDatagramFlags,
  _In_ ULONG BytesIndicated,
  _In_ ULONG BytesAvailable,
  _Out_ ULONG *BytesTaken,
  _In_ PVOID Tsdu,
  _Out_opt_ PIRP *IoRequestPacket);

TDIKRNLAPI
NTSTATUS NTAPI
TdiDefaultRcvDatagramHandler(
  _In_opt_ PVOID TdiEventContext,
  _In_ LONG SourceAddressLength,
  _In_reads_bytes_(SourceAddressLength) PVOID SourceAddress,
  _In_ LONG OptionsLength,
  _In_reads_bytes_opt_(OptionsLength) PVOID Options,
  _In_ ULONG ReceiveDatagramFlags,
  _In_ ULONG BytesIndicated,
  _In_ ULONG BytesAvailable,
  _Out_ ULONG *BytesTaken,
  _In_ PVOID Tsdu,
  _Out_opt_ PIRP *IoRequestPacket);

typedef NTSTATUS
(NTAPI *PTDI_IND_RECEIVE_EXPEDITED)(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ CONNECTION_CONTEXT ConnectionContext,
  _In_ ULONG ReceiveFlags,
  _In_ ULONG BytesIndicated,
  _In_ ULONG BytesAvailable,
  _Out_ ULONG *BytesTaken,
  _In_ PVOID Tsdu,
  _Out_opt_ PIRP *IoRequestPacket);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiDefaultRcvExpeditedHandler(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ CONNECTION_CONTEXT ConnectionContext,
  _In_ ULONG ReceiveFlags,
  _In_ ULONG BytesIndicated,
  _In_ ULONG BytesAvailable,
  _Out_ ULONG *BytesTaken,
  _In_ PVOID Tsdu,
  _Out_opt_ PIRP *IoRequestPacket);

typedef NTSTATUS
(NTAPI *PTDI_IND_CHAINED_RECEIVE)(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ CONNECTION_CONTEXT ConnectionContext,
  _In_ ULONG ReceiveFlags,
  _In_ ULONG ReceiveLength,
  _In_ ULONG StartingOffset,
  _In_ PMDL Tsdu,
  _In_ PVOID TsduDescriptor);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiDefaultChainedReceiveHandler(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ CONNECTION_CONTEXT ConnectionContext,
  _In_ ULONG ReceiveFlags,
  _In_ ULONG ReceiveLength,
  _In_ ULONG StartingOffset,
  _In_ PMDL Tsdu,
  _In_ PVOID TsduDescriptor);

typedef NTSTATUS
(NTAPI *PTDI_IND_CHAINED_RECEIVE_DATAGRAM)(
  _In_opt_ PVOID TdiEventContext,
  _In_ LONG SourceAddressLength,
  _In_reads_bytes_(SourceAddressLength) PVOID SourceAddress,
  _In_ LONG OptionsLength,
  _In_reads_bytes_opt_(OptionsLength) PVOID Options,
  _In_ ULONG ReceiveDatagramFlags,
  _In_ ULONG ReceiveDatagramLength,
  _In_ ULONG StartingOffset,
  _In_ PMDL Tsdu,
  _In_ PVOID TsduDescriptor);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiDefaultChainedRcvDatagramHandler(
  _In_opt_ PVOID TdiEventContext,
  _In_ LONG SourceAddressLength,
  _In_reads_bytes_(SourceAddressLength) PVOID SourceAddress,
  _In_ LONG OptionsLength,
  _In_reads_bytes_opt_(OptionsLength) PVOID Options,
  _In_ ULONG ReceiveDatagramFlags,
  _In_ ULONG ReceiveDatagramLength,
  _In_ ULONG StartingOffset,
  _In_ PMDL Tsdu,
  _In_ PVOID TsduDescriptor);

typedef NTSTATUS
(NTAPI *PTDI_IND_CHAINED_RECEIVE_EXPEDITED)(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ CONNECTION_CONTEXT ConnectionContext,
  _In_ ULONG ReceiveFlags,
  _In_ ULONG ReceiveLength,
  _In_ ULONG StartingOffset,
  _In_ PMDL Tsdu,
  _In_ PVOID TsduDescriptor);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiDefaultChainedRcvExpeditedHandler(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ CONNECTION_CONTEXT ConnectionContext,
  _In_ ULONG ReceiveFlags,
  _In_ ULONG ReceiveLength,
  _In_ ULONG StartingOffset,
  _In_ PMDL Tsdu,
  _In_ PVOID TsduDescriptor);

typedef NTSTATUS
(NTAPI *PTDI_IND_SEND_POSSIBLE)(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ PVOID ConnectionContext,
  _In_ ULONG BytesAvailable);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiDefaultSendPossibleHandler(
  _In_opt_ PVOID TdiEventContext,
  _In_opt_ PVOID ConnectionContext,
  _In_ ULONG BytesAvailable);



/* Macros and functions to build IRPs */

#define TdiBuildBaseIrp(                                                  \
  bIrp, bDevObj, bFileObj, bCompRoutine, bContxt, bIrpSp, bMinor)         \
{                                                                         \
  bIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;                 \
  bIrpSp->MinorFunction = (bMinor);                                       \
  bIrpSp->DeviceObject  = (bDevObj);                                      \
  bIrpSp->FileObject    = (bFileObj);                                     \
  if (bCompRoutine)                                                       \
  {                                                                       \
    IoSetCompletionRoutine(bIrp, bCompRoutine, bContxt, TRUE, TRUE, TRUE);\
  }                                                                       \
  else                                                                    \
    IoSetCompletionRoutine(bIrp, NULL, NULL, FALSE, FALSE, FALSE);        \
}

/*
 * VOID
 * TdiBuildAccept(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN PTDI_CONNECTION_INFORMATION  RequestConnectionInfo,
 *   OUT PTDI_CONNECTION_INFORMATION  ReturnConnectionInfo);
 */
#define TdiBuildAccept(                                             \
  Irp, DevObj, FileObj, CompRoutine, Contxt,                        \
  RequestConnectionInfo, ReturnConnectionInfo)                      \
{                                                                   \
  PTDI_REQUEST_KERNEL_ACCEPT _Request;                              \
  PIO_STACK_LOCATION _IrpSp;                                        \
                                                                    \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                          \
                                                                    \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                \
                  Contxt, _IrpSp, TDI_ACCEPT);                      \
                                                                    \
  _Request = (PTDI_REQUEST_KERNEL_ACCEPT)&_IrpSp->Parameters;       \
  _Request->RequestConnectionInformation = (RequestConnectionInfo); \
  _Request->ReturnConnectionInformation  = (ReturnConnectionInfo);  \
}

/*
 * VOID
 * TdiBuildAction(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN PMDL  MdlAddr);
 */
#define TdiBuildAction(                               \
  Irp, DevObj, FileObj, CompRoutine, Contxt, MdlAddr) \
{                                                     \
  PIO_STACK_LOCATION _IrpSp;                          \
                                                      \
  _IrpSp = IoGetNextIrpStackLocation(Irp);            \
                                                      \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,  \
                  Contxt, _IrpSp, TDI_ACTION);        \
                                                      \
  (Irp)->MdlAddress = (MdlAddr);                      \
}

/*
 * VOID
 * TdiBuildAssociateAddress(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN HANDLE  AddrHandle);
 */
#define TdiBuildAssociateAddress(                                \
  Irp, DevObj, FileObj, CompRoutine, Contxt, AddrHandle)         \
{                                                                \
  PTDI_REQUEST_KERNEL_ASSOCIATE _Request;                        \
  PIO_STACK_LOCATION _IrpSp;                                     \
                                                                 \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                       \
                                                                 \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,             \
                  Contxt, _IrpSp, TDI_ASSOCIATE_ADDRESS);        \
                                                                 \
  _Request = (PTDI_REQUEST_KERNEL_ASSOCIATE)&_IrpSp->Parameters; \
  _Request->AddressHandle = (HANDLE)(AddrHandle);                \
}

/*
 * VOID
 * TdiBuildConnect(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN PLARGE_INTEGER  Time,
 *   IN PTDI_CONNECTION_INFORMATION  RequestConnectionInfo,
 *   OUT PTDI_CONNECTION_INFORMATION  ReturnConnectionInfo);
 */
#define TdiBuildConnect(                                            \
  Irp, DevObj, FileObj, CompRoutine, Contxt,                        \
  Time, RequestConnectionInfo, ReturnConnectionInfo)                \
{                                                                   \
  PTDI_REQUEST_KERNEL _Request;                                     \
  PIO_STACK_LOCATION _IrpSp;                                        \
                                                                    \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                          \
                                                                    \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                \
                  Contxt, _IrpSp, TDI_CONNECT);                     \
                                                                    \
  _Request = (PTDI_REQUEST_KERNEL)&_IrpSp->Parameters;              \
  _Request->RequestConnectionInformation = (RequestConnectionInfo); \
  _Request->ReturnConnectionInformation  = (ReturnConnectionInfo);  \
  _Request->RequestSpecific              = (PVOID)(Time);           \
}

/*
 * VOID
 * TdiBuildDisassociateAddress(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt);
 */
#define TdiBuildDisassociateAddress(                                \
  Irp, DevObj, FileObj, CompRoutine, Contxt)                        \
{                                                                   \
  PIO_STACK_LOCATION _IrpSp;                                        \
                                                                    \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                          \
                                                                    \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                \
                  Contxt, _IrpSp, TDI_DISASSOCIATE_ADDRESS);        \
}

/*
 * VOID
 * TdiBuildDisconnect(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN PLARGE_INTEGER  Time,
 *   IN PULONG  Flags,
 *   IN PTDI_CONNECTION_INFORMATION  RequestConnectionInfo,
 *   OUT PTDI_CONNECTION_INFORMATION  ReturnConnectionInfo);
 */
#define TdiBuildDisconnect(                                         \
  Irp, DevObj, FileObj, CompRoutine, Contxt, Time,                  \
  Flags, RequestConnectionInfo, ReturnConnectionInfo)               \
{                                                                   \
  PTDI_REQUEST_KERNEL _Request;                                     \
  PIO_STACK_LOCATION _IrpSp;                                        \
                                                                    \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                          \
                                                                    \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                \
                  Contxt, _IrpSp, TDI_DISCONNECT);                  \
                                                                    \
  _Request = (PTDI_REQUEST_KERNEL)&_IrpSp->Parameters;              \
  _Request->RequestConnectionInformation = (RequestConnectionInfo); \
  _Request->ReturnConnectionInformation  = (ReturnConnectionInfo);  \
  _Request->RequestSpecific = (PVOID)(Time);                        \
  _Request->RequestFlags    = (Flags);                              \
}

/*
 * PIRP
 * TdiBuildInternalDeviceControlIrp(
 *   IN CCHAR IrpSubFunction,
 *   IN PDEVICE_OBJECT DeviceObject,
 *   IN PFILE_OBJECT FileObject,
 *   IN PKEVENT Event,
 *   IN PIO_STATUS_BLOCK IoStatusBlock);
 */
#define TdiBuildInternalDeviceControlIrp( \
  IrpSubFunction, DeviceObject,           \
  FileObject, Event, IoStatusBlock)       \
  IoBuildDeviceIoControlRequest(          \
		IrpSubFunction, DeviceObject,             \
		NULL, 0, NULL, 0,                     \
		TRUE, Event, IoStatusBlock)

/*
 * VOID
 * TdiBuildListen(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN ULONG  Flags,
 *   IN PTDI_CONNECTION_INFORMATION  RequestConnectionInfo,
 *   OUT PTDI_CONNECTION_INFORMATION  ReturnConnectionInfo);
 */
#define TdiBuildListen(                                             \
  Irp, DevObj, FileObj, CompRoutine, Contxt,                        \
  Flags, RequestConnectionInfo, ReturnConnectionInfo)               \
{                                                                   \
  PTDI_REQUEST_KERNEL _Request;                                     \
  PIO_STACK_LOCATION _IrpSp;                                        \
                                                                    \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                          \
                                                                    \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                \
                  Contxt, _IrpSp, TDI_LISTEN);                      \
                                                                    \
  _Request = (PTDI_REQUEST_KERNEL)&_IrpSp->Parameters;              \
  _Request->RequestConnectionInformation = (RequestConnectionInfo); \
  _Request->ReturnConnectionInformation  = (ReturnConnectionInfo);  \
  _Request->RequestFlags = (Flags);                                 \
}

TDIKRNLAPI
VOID
NTAPI
TdiBuildNetbiosAddress(
  _In_ PUCHAR NetbiosName,
  _In_ BOOLEAN IsGroupName,
  _Inout_ PTA_NETBIOS_ADDRESS NetworkName);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiBuildNetbiosAddressEa(
  _Out_ PUCHAR Buffer,
  _In_ BOOLEAN IsGroupName,
  _In_ PUCHAR NetbiosName);

/*
 * VOID
 * TdiBuildQueryInformation(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN UINT  QType,
 *   IN PMDL  MdlAddr);
 */
#define TdiBuildQueryInformation(                                        \
  Irp, DevObj, FileObj, CompRoutine, Contxt, QType, MdlAddr)             \
{                                                                        \
  PTDI_REQUEST_KERNEL_QUERY_INFORMATION _Request;                        \
  PIO_STACK_LOCATION _IrpSp;                                             \
                                                                         \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                               \
                                                                         \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                     \
                  Contxt, _IrpSp, TDI_QUERY_INFORMATION);                \
                                                                         \
  _Request = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&_IrpSp->Parameters; \
  _Request->RequestConnectionInformation = NULL;                         \
  _Request->QueryType = (ULONG)(QType);                                  \
  (Irp)->MdlAddress   = (MdlAddr);                                       \
}

/*
 * VOID
 * TdiBuildReceive(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN PMDL  MdlAddr,
 *   IN ULONG  InFlags,
 *   IN ULONG  ReceiveLen);
 */
#define TdiBuildReceive(                                       \
  Irp, DevObj, FileObj, CompRoutine, Contxt,                   \
  MdlAddr, InFlags, ReceiveLen)                                \
{                                                              \
  PTDI_REQUEST_KERNEL_RECEIVE _Request;                        \
  PIO_STACK_LOCATION _IrpSp;                                   \
                                                               \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                     \
                                                               \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,           \
                  Contxt, _IrpSp, TDI_RECEIVE);                \
                                                               \
  _Request = (PTDI_REQUEST_KERNEL_RECEIVE)&_IrpSp->Parameters; \
  _Request->ReceiveFlags  = (InFlags);                         \
  _Request->ReceiveLength = (ReceiveLen);                      \
  (Irp)->MdlAddress       = (MdlAddr);                         \
}

/*
 * VOID
 * TdiBuildReceiveDatagram(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN PMDL  MdlAddr,
 *   IN ULONG  ReceiveLen,
 *   IN PTDI_CONNECTION_INFORMATION  ReceiveDatagramInfo,
 *   OUT PTDI_CONNECTION_INFORMATION  ReturnInfo,
 *   ULONG InFlags);
 */
#define TdiBuildReceiveDatagram(                                 \
  Irp, DevObj, FileObj, CompRoutine, Contxt, MdlAddr,            \
  ReceiveLen, ReceiveDatagramInfo, ReturnInfo, InFlags)          \
{                                                                \
  PTDI_REQUEST_KERNEL_RECEIVEDG _Request;                        \
  PIO_STACK_LOCATION _IrpSp;                                     \
                                                                 \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                       \
                                                                 \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,             \
                  Contxt, _IrpSp, TDI_RECEIVE_DATAGRAM);         \
                                                                 \
  _Request = (PTDI_REQUEST_KERNEL_RECEIVEDG)&_IrpSp->Parameters; \
  _Request->ReceiveDatagramInformation = (ReceiveDatagramInfo);  \
  _Request->ReturnDatagramInformation  = (ReturnInfo);           \
  _Request->ReceiveLength = (ReceiveLen);                        \
  _Request->ReceiveFlags  = (InFlags);                           \
  (Irp)->MdlAddress       = (MdlAddr);                           \
}

/*
 * VOID
 * TdiBuildSend(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN PMDL  MdlAddr,
 *   IN ULONG  InFlags,
 *   IN ULONG  SendLen);
 */
#define TdiBuildSend(                                       \
  Irp, DevObj, FileObj, CompRoutine, Contxt,                \
  MdlAddr, InFlags, SendLen)                                \
{                                                           \
  PTDI_REQUEST_KERNEL_SEND _Request;                        \
  PIO_STACK_LOCATION _IrpSp;                                \
                                                            \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                  \
                                                            \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,        \
                  Contxt, _IrpSp, TDI_SEND);                \
                                                            \
  _Request = (PTDI_REQUEST_KERNEL_SEND)&_IrpSp->Parameters; \
  _Request->SendFlags  = (InFlags);                         \
  _Request->SendLength = (SendLen);                         \
  (Irp)->MdlAddress    = (MdlAddr);                         \
}

/*
 * VOID
 * TdiBuildSendDatagram(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN PMDL  MdlAddr,
 *   IN ULONG  SendLen,
 *   IN PTDI_CONNECTION_INFORMATION  SendDatagramInfo);
 */
#define TdiBuildSendDatagram(                                 \
  Irp, DevObj, FileObj, CompRoutine, Contxt,                  \
  MdlAddr, SendLen, SendDatagramInfo)                         \
{                                                             \
  PTDI_REQUEST_KERNEL_SENDDG _Request;                        \
  PIO_STACK_LOCATION _IrpSp;                                  \
                                                              \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                    \
                                                              \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,          \
                  Contxt, _IrpSp, TDI_SEND_DATAGRAM);         \
                                                              \
  _Request = (PTDI_REQUEST_KERNEL_SENDDG)&_IrpSp->Parameters; \
  _Request->SendDatagramInformation = (SendDatagramInfo);     \
  _Request->SendLength = (SendLen);                           \
  (Irp)->MdlAddress    = (MdlAddr);                           \
}

/*
 * VOID
 * TdiBuildSetEventHandler(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN INT  InEventType,
 *   IN PVOID  InEventHandler,
 *   IN PVOID  InEventContext);
 */
#define TdiBuildSetEventHandler(                                 \
  Irp, DevObj, FileObj, CompRoutine, Contxt,                     \
  InEventType, InEventHandler, InEventContext)                   \
{                                                                \
  PTDI_REQUEST_KERNEL_SET_EVENT _Request;                        \
  PIO_STACK_LOCATION _IrpSp;                                     \
                                                                 \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                       \
                                                                 \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,             \
                  Contxt, _IrpSp, TDI_SET_EVENT_HANDLER);        \
                                                                 \
  _Request = (PTDI_REQUEST_KERNEL_SET_EVENT)&_IrpSp->Parameters; \
  _Request->EventType    = (InEventType);                        \
  _Request->EventHandler = (PVOID)(InEventHandler);              \
  _Request->EventContext = (PVOID)(InEventContext);              \
}

/*
 * VOID
 * TdiBuildSetInformation(
 *   IN PIRP  Irp,
 *   IN PDEVICE_OBJECT  DevObj,
 *   IN PFILE_OBJECT  FileObj,
 *   IN PVOID  CompRoutine,
 *   IN PVOID  Contxt,
 *   IN UINT  SType,
 *   IN PMDL  MdlAddr);
 */
#define TdiBuildSetInformation(                                        \
  Irp, DevObj, FileObj, CompRoutine, Contxt, SType, MdlAddr)           \
{                                                                      \
  PTDI_REQUEST_KERNEL_SET_INFORMATION _Request;                        \
  PIO_STACK_LOCATION _IrpSp;                                           \
                                                                       \
  _IrpSp = IoGetNextIrpStackLocation(Irp);                             \
                                                                       \
  TdiBuildBaseIrp(Irp, DevObj, FileObj, CompRoutine,                   \
                  Contxt, _IrpSp, TDI_SET_INFORMATION);                \
                                                                       \
  _Request = (PTDI_REQUEST_KERNEL_SET_INFORMATION)&_IrpSp->Parameters; \
  _Request->RequestConnectionInformation = NULL;                       \
  _Request->SetType = (ULONG)(SType);                                  \
  (Irp)->MdlAddress = (MdlAddr);                                       \
}

/* TDI20_CLIENT_INTERFACE_INFO.TdiVersion constants */
#define TDI_CURRENT_MAJOR_VERSION         2
#define TDI_CURRENT_MINOR_VERSION         0

#define TDI_CURRENT_VERSION ((TDI_CURRENT_MINOR_VERSION) << 8 \
  | (TDI_CURRENT_MAJOR_VERSION))

#define TDI_VERSION_ONE                   0x0001

typedef enum _TDI_PNP_OPCODE {
  TDI_PNP_OP_MIN,
  TDI_PNP_OP_ADD,
  TDI_PNP_OP_DEL,
  TDI_PNP_OP_UPDATE,
  TDI_PNP_OP_PROVIDERREADY,
  TDI_PNP_OP_NETREADY,
  TDI_PNP_OP_ADD_IGNORE_BINDING,
  TDI_PNP_OP_DELETE_IGNORE_BINDING,
  TDI_PNP_OP_MAX
} TDI_PNP_OPCODE;

/* TDI_PNP_CONTEXT.ContextType */
#define TDI_PNP_CONTEXT_TYPE_IF_NAME            0x1
#define TDI_PNP_CONTEXT_TYPE_IF_ADDR            0x2
#define TDI_PNP_CONTEXT_TYPE_PDO                0x3
#define TDI_PNP_CONTEXT_TYPE_FIRST_OR_LAST_IF   0x4

typedef struct _TDI_PNP_CONTEXT {
  USHORT  ContextSize;
  USHORT  ContextType;
  UCHAR  ContextData[1];
} TDI_PNP_CONTEXT, *PTDI_PNP_CONTEXT;

typedef VOID
(NTAPI *TDI_ADD_ADDRESS_HANDLER)(
  _In_ PTA_ADDRESS Address);

typedef VOID
(NTAPI *TDI_ADD_ADDRESS_HANDLER_V2)(
  _In_ PTA_ADDRESS Address,
  _In_ PUNICODE_STRING DeviceName,
  _In_ PTDI_PNP_CONTEXT Context);

typedef VOID
(NTAPI *TDI_BINDING_HANDLER)(
  _In_ TDI_PNP_OPCODE PnPOpcode,
  _In_ PUNICODE_STRING DeviceName,
  _In_ PWSTR MultiSZBindList);

typedef VOID
(NTAPI *TDI_BIND_HANDLER)(
  _In_ PUNICODE_STRING DeviceName);

typedef VOID
(NTAPI *TDI_DEL_ADDRESS_HANDLER)(
  _In_ PTA_ADDRESS Address);

typedef VOID
(NTAPI *TDI_DEL_ADDRESS_HANDLER_V2)(
  _In_ PTA_ADDRESS Address,
  _In_ PUNICODE_STRING DeviceName,
  _In_ PTDI_PNP_CONTEXT Context);

typedef NTSTATUS
(NTAPI *TDI_PNP_POWER_HANDLER)(
  _In_ PUNICODE_STRING DeviceName,
  _In_ PNET_PNP_EVENT PowerEvent,
  _In_ PTDI_PNP_CONTEXT Context1,
  _In_ PTDI_PNP_CONTEXT Context2);

typedef VOID
(NTAPI *TDI_UNBIND_HANDLER)(
  _In_ PUNICODE_STRING DeviceName);

typedef VOID
(NTAPI *ProviderPnPPowerComplete)(
  _In_ PNET_PNP_EVENT NetEvent,
  _In_ NTSTATUS ProviderStatus);

typedef struct _TDI20_CLIENT_INTERFACE_INFO {
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      UCHAR  MajorTdiVersion;
      UCHAR  MinorTdiVersion;
    } DUMMYSTRUCTNAME;
    USHORT TdiVersion;
  } DUMMYUNIONNAME;
  USHORT  Unused;
  PUNICODE_STRING  ClientName;
  TDI_PNP_POWER_HANDLER  PnPPowerHandler;
  _ANONYMOUS_UNION union {
    TDI_BINDING_HANDLER  BindingHandler;
    _ANONYMOUS_STRUCT struct {
      TDI_BIND_HANDLER  BindHandler;
      TDI_UNBIND_HANDLER  UnBindHandler;
    } DUMMYSTRUCTNAME;
  }DUMMYUNIONNAME2;
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      TDI_ADD_ADDRESS_HANDLER_V2  AddAddressHandlerV2;
      TDI_DEL_ADDRESS_HANDLER_V2  DelAddressHandlerV2;
    } DUMMYSTRUCTNAME;
    _ANONYMOUS_STRUCT struct {
      TDI_ADD_ADDRESS_HANDLER  AddAddressHandler;
      TDI_DEL_ADDRESS_HANDLER  DelAddressHandler;
    } DUMMYSTRUCTNAME2;
  } DUMMYUNIONNAME3;
} TDI20_CLIENT_INTERFACE_INFO, *PTDI20_CLIENT_INTERFACE_INFO;

typedef TDI20_CLIENT_INTERFACE_INFO TDI_CLIENT_INTERFACE_INFO;
typedef TDI_CLIENT_INTERFACE_INFO *PTDI_CLIENT_INTERFACE_INFO;


/* TDI functions */

/*
 * VOID
 * TdiCompleteRequest(
 *   IN PIRP Irp,
 *   IN NTSTATUS Status);
 */
#define TdiCompleteRequest(Irp, Status)           \
{                                                 \
  (Irp)->IoStatus.Status = (Status);              \
  IoCompleteRequest((Irp), IO_NETWORK_INCREMENT); \
}

_IRQL_requires_max_(DISPATCH_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiCopyBufferToMdl(
  _In_ PVOID SourceBuffer,
  _In_ ULONG SourceOffset,
  _In_ ULONG SourceBytesToCopy,
  _In_ PMDL DestinationMdlChain,
  _In_ ULONG DestinationOffset,
  _Out_ PULONG BytesCopied);

/*
 * VOID
 * TdiCopyLookaheadData(
 *   IN PVOID  Destination,
 *   IN PVOID  Source,
 *   IN ULONG  Length,
 *   IN ULONG  ReceiveFlags);
 */
#define TdiCopyLookaheadData(Destination, Source, Length, ReceiveFlags) \
  RtlCopyMemory(Destination, Source, Length)

_IRQL_requires_max_(DISPATCH_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiCopyMdlChainToMdlChain(
  _In_ PMDL SourceMdlChain,
  _In_ ULONG SourceOffset,
  _In_ PMDL DestinationMdlChain,
  _In_ ULONG DestinationOffset,
  _Out_ PULONG BytesCopied);

_IRQL_requires_max_(DISPATCH_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiCopyMdlToBuffer(
  _In_ PMDL SourceMdlChain,
  _In_ ULONG SourceOffset,
  _Out_writes_bytes_(DestinationBufferSize) PVOID DestinationBuffer,
  _In_ ULONG DestinationOffset,
  _In_ ULONG DestinationBufferSize,
  _Out_ PULONG BytesCopied);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiDeregisterAddressChangeHandler(
  _In_ HANDLE BindingHandle);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiDeregisterDeviceObject(
  _In_ HANDLE DevRegistrationHandle);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiDeregisterNetAddress(
  _In_ HANDLE AddrRegistrationHandle);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiDeregisterPnPHandlers(
  _In_ HANDLE BindingHandle);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiDeregisterProvider(
  _In_ HANDLE ProviderHandle);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiEnumerateAddresses(
  _In_ HANDLE BindingHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
TDIKRNLAPI
VOID
NTAPI
TdiInitialize(VOID);

TDIKRNLAPI
VOID
NTAPI
TdiMapBuffer(
  _In_ PMDL MdlChain);

_IRQL_requires_max_(PASSIVE_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiMapUserRequest(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp,
  _In_ PIO_STACK_LOCATION IrpSp);

TDIKRNLAPI
BOOLEAN
NTAPI
TdiMatchPdoWithChainedReceiveContext(
  _In_ PVOID TsduDescriptor,
  _In_ PVOID PDO);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
VOID
NTAPI
TdiPnPPowerComplete(
  _In_ HANDLE BindingHandle,
  _In_ PNET_PNP_EVENT PowerEvent,
  _In_ NTSTATUS Status);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiPnPPowerRequest(
  _In_ PUNICODE_STRING DeviceName,
  _In_ PNET_PNP_EVENT PowerEvent,
  _In_ PTDI_PNP_CONTEXT Context1,
  _In_ PTDI_PNP_CONTEXT Context2,
  _In_ ProviderPnPPowerComplete ProtocolCompletionHandler);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiProviderReady(
  _In_ HANDLE ProviderHandle);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiRegisterAddressChangeHandler(
  _In_ TDI_ADD_ADDRESS_HANDLER AddHandler,
  _In_ TDI_DEL_ADDRESS_HANDLER DeleteHandler,
  _Out_ HANDLE *BindingHandle);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiRegisterDeviceObject(
  _In_ PUNICODE_STRING DeviceName,
  _Out_ HANDLE *DevRegistrationHandle);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiRegisterNetAddress(
  _In_ PTA_ADDRESS Address,
  _In_ PUNICODE_STRING DeviceName,
  _In_ PTDI_PNP_CONTEXT Context,
  _Out_ HANDLE *AddrRegistrationHandle);

TDIKRNLAPI
NTSTATUS
NTAPI
TdiRegisterNotificationHandler(
  _In_ TDI_BIND_HANDLER BindHandler,
  _In_ TDI_UNBIND_HANDLER UnbindHandler,
  _Out_ HANDLE *BindingHandle);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiRegisterPnPHandlers(
  _In_reads_bytes_(InterfaceInfoSize) PTDI_CLIENT_INTERFACE_INFO ClientInterfaceInfo,
  _In_ ULONG InterfaceInfoSize,
  _Out_ HANDLE *BindingHandle);

_IRQL_requires_max_(APC_LEVEL)
TDIKRNLAPI
NTSTATUS
NTAPI
TdiRegisterProvider(
  _In_ PUNICODE_STRING ProviderName,
  _Out_ HANDLE *ProviderHandle);

_IRQL_requires_max_(DISPATCH_LEVEL)
TDIKRNLAPI
VOID
NTAPI
TdiReturnChainedReceives(
  _In_ PVOID *TsduDescriptors,
  _In_ ULONG NumberOfTsdus);

TDIKRNLAPI
VOID
NTAPI
TdiUnmapBuffer(
  _In_ PMDL MdlChain);

#ifdef __cplusplus
}
#endif

#endif /* __TDIKRNL_H */
