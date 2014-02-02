/*
 * wsk.h
 *
 * Windows Sockets Kernel-Mode Interface
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
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
#pragma once
#define _WSK_

#ifdef __cplusplus
extern "C" {
#endif

#include <netioddk.h>
#include <ws2def.h>
#include <mswsockdef.h>

extern CONST NPIID NPI_WSK_INTERFACE_ID;

#define WSKAPI NTAPI

#define MAKE_WSK_VERSION(Mj, Mn) ((USHORT)((Mj) << 8) | (USHORT)((Mn) & 0xff))
#define WSK_MAJOR_VERSION(V) ((UCHAR)((V) >> 8))
#define WSK_MINOR_VERSION(V) ((UCHAR)(V))
#define WSK_FLAG_AT_DISPATCH_LEVEL 0x00000008
#define WSK_FLAG_RELEASE_ASAP 0x00000002
#define WSK_FLAG_ENTIRE_MESSAGE 0x00000004
#define WSK_FLAG_ABORTIVE 0x00000001
#define WSK_FLAG_BASIC_SOCKET 0x00000000
#define WSK_FLAG_LISTEN_SOCKET 0x00000001
#define WSK_FLAG_CONNECTION_SOCKET 0x00000002
#define WSK_FLAG_DATAGRAM_SOCKET 0x00000004
#define WSK_TRANSPORT_LIST_QUERY 2
#define WSK_TRANSPORT_LIST_CHANGE 3
#define WSK_CACHE_SD 4
#define WSK_RELEASE_SD 5
#define WSK_TDI_DEVICENAME_MAPPING 6
#define WSK_SET_STATIC_EVENT_CALLBACKS 7
#define WSK_TDI_BEHAVIOR 8
#define WSK_TDI_BEHAVIOR_BYPASS_TDI 0x00000001
#define SO_WSK_SECURITY (WSK_SO_BASE+1)
#define SO_WSK_EVENT_CALLBACK (WSK_SO_BASE+2)
#define WSK_EVENT_RECEIVE_FROM 0x00000100
#define WSK_EVENT_ACCEPT 0x00000200
#define WSK_EVENT_SEND_BACKLOG 0x00000010
#define WSK_EVENT_RECEIVE 0x00000040
#define WSK_EVENT_DISCONNECT 0x00000080
#define WSK_EVENT_DISABLE 0x80000000
#define SIO_WSK_SET_REMOTE_ADDRESS _WSAIOW(IOC_WSK,0x1)
#define SIO_WSK_REGISTER_EXTENSION _WSAIORW(IOC_WSK,0x2)
#define SIO_WSK_QUERY_IDEAL_SEND_BACKLOG _WSAIOR(IOC_WSK,0x3)
#define SIO_WSK_QUERY_RECEIVE_BACKLOG _WSAIOR(IOC_WSK,0x4)
#define SIO_WSK_QUERY_INSPECT_ID _WSAIOR(IOC_WSK,0x5)
#define SIO_WSK_SET_SENDTO_ADDRESS _WSAIOW(IOC_WSK,0x6)
#define WSK_FLAG_NODELAY 0x00000002
#define WSK_FLAG_WAITALL 0x00000002
#define WSK_FLAG_DRAIN 0x00000004
#define WSK_NO_WAIT 0
#define WSK_INFINITE_WAIT 0xffffffff

typedef enum
{
    WskInspectReject,
    WskInspectAccept,
    WskInspectPend,
    WskInspectMax
} WSK_INSPECT_ACTION, *PWSK_INSPECT_ACTION;

typedef enum
{
    WskSetOption,
    WskGetOption,
    WskIoctl,
    WskControlMax
} WSK_CONTROL_SOCKET_TYPE, *PWSK_CONTROL_SOCKET_TYPE;

typedef PVOID PWSK_CLIENT;

typedef struct _WSK_SOCKET
{
    const VOID *Dispatch;
} WSK_SOCKET, *PWSK_SOCKET;

typedef struct _WSK_BUF
{
    PMDL Mdl;
    ULONG Offset;
    SIZE_T Length;
} WSK_BUF, *PWSK_BUF;

typedef struct _WSK_INSPECT_ID
{
    ULONG_PTR Key;
    ULONG SerialNumber;
} WSK_INSPECT_ID, *PWSK_INSPECT_ID;

typedef struct _WSK_DATAGRAM_INDICATION
{
    struct _WSK_DATAGRAM_INDICATION *Next;
    WSK_BUF Buffer;
    _Field_size_bytes_(ControlInfoLength) PCMSGHDR ControlInfo;
    ULONG ControlInfoLength;
    PSOCKADDR RemoteAddress;
} WSK_DATAGRAM_INDICATION, *PWSK_DATAGRAM_INDICATION;

typedef
_Must_inspect_result_
NTSTATUS
(WSKAPI * PFN_WSK_RECEIVE_FROM_EVENT)(
    _In_opt_ PVOID SocketContext,
    _In_ ULONG Flags,
    _In_opt_ PWSK_DATAGRAM_INDICATION DataIndication);

typedef struct _WSK_DATA_INDICATION
{
    struct _WSK_DATA_INDICATION *Next;
    WSK_BUF Buffer;
} WSK_DATA_INDICATION, *PWSK_DATA_INDICATION;

typedef
_Must_inspect_result_
NTSTATUS
(WSKAPI * PFN_WSK_RECEIVE_EVENT)(
    _In_opt_ PVOID SocketContext,
    _In_ ULONG Flags,
    _In_opt_ PWSK_DATA_INDICATION DataIndication,
    _In_ SIZE_T BytesIndicated,
    _Inout_ SIZE_T *BytesAccepted);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_DISCONNECT_EVENT)(
    _In_opt_ PVOID SocketContext,
    _In_ ULONG Flags);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_SEND_BACKLOG_EVENT)(
    _In_opt_ PVOID SocketContext,
    _In_ SIZE_T IdealBacklogSize);

typedef struct _WSK_CLIENT_CONNECTION_DISPATCH
{
    PFN_WSK_RECEIVE_EVENT WskReceiveEvent;
    PFN_WSK_DISCONNECT_EVENT WskDisconnectEvent;
    PFN_WSK_SEND_BACKLOG_EVENT WskSendBacklogEvent;
} WSK_CLIENT_CONNECTION_DISPATCH, *PWSK_CLIENT_CONNECTION_DISPATCH;

typedef
_Must_inspect_result_
_At_(AcceptSocket, __drv_aliasesMem)
NTSTATUS
(WSKAPI * PFN_WSK_ACCEPT_EVENT)(
    _In_opt_ PVOID SocketContext,
    _In_ ULONG Flags,
    _In_ PSOCKADDR LocalAddress,
    _In_ PSOCKADDR RemoteAddress,
    _In_opt_ PWSK_SOCKET AcceptSocket,
    _Outptr_result_maybenull_ PVOID *AcceptSocketContext,
    _Outptr_result_maybenull_ const WSK_CLIENT_CONNECTION_DISPATCH **AcceptSocketDispatch);

typedef
_At_(Irp->IoStatus.Information, __drv_allocatesMem(Mem))
NTSTATUS
(WSKAPI * PFN_WSK_SOCKET_CONNECT)(
    _In_ PWSK_CLIENT Client,
    _In_ USHORT SocketType,
    _In_ ULONG Protocol,
    _In_ PSOCKADDR LocalAddress,
    _In_ PSOCKADDR RemoteAddress,
    _Reserved_ ULONG Flags,
    _In_opt_ PVOID SocketContext,
    _In_opt_ const WSK_CLIENT_CONNECTION_DISPATCH *Dispatch,
    _In_opt_ PEPROCESS OwningProcess,
    _In_opt_ PETHREAD OwningThread,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_CONTROL_SOCKET)(
    _In_ PWSK_SOCKET Socket,
    _In_ WSK_CONTROL_SOCKET_TYPE RequestType,
    _In_ ULONG ControlCode,
    _In_ ULONG Level,
    _In_ SIZE_T InputSize,
    _In_reads_bytes_opt_(InputSize) PVOID InputBuffer,
    _In_ SIZE_T OutputSize,
    _Out_writes_bytes_opt_(OutputSize) PVOID  OutputBuffer,
    _Out_opt_ SIZE_T *OutputSizeReturned,
    _Inout_opt_ PIRP Irp);

typedef
_At_(Socket, __drv_freesMem(Mem))
NTSTATUS
(WSKAPI * PFN_WSK_CLOSE_SOCKET)(
    _In_ PWSK_SOCKET Socket,
    _Inout_ PIRP Irp);

typedef struct _WSK_PROVIDER_BASIC_DISPATCH
{
    PFN_WSK_CONTROL_SOCKET WskControlSocket;
    PFN_WSK_CLOSE_SOCKET WskCloseSocket;
} WSK_PROVIDER_BASIC_DISPATCH, *PWSK_PROVIDER_BASIC_DISPATCH;

typedef
NTSTATUS
(WSKAPI * PFN_WSK_BIND) (
    _In_ PWSK_SOCKET Socket,
    _In_ PSOCKADDR LocalAddress,
    _Reserved_ ULONG Flags,
    _Inout_ PIRP Irp);

typedef
_At_(Irp->IoStatus.Information, __drv_allocatesMem(Mem))
NTSTATUS
(WSKAPI * PFN_WSK_ACCEPT)(
    _In_ PWSK_SOCKET ListenSocket,
    _Reserved_ ULONG Flags,
    _In_opt_ PVOID AcceptSocketContext,
    _In_opt_ const WSK_CLIENT_CONNECTION_DISPATCH *AcceptSocketDispatch,
    _Out_opt_ PSOCKADDR LocalAddress,
    _Out_opt_ PSOCKADDR RemoteAddress,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_INSPECT_COMPLETE)(
    _In_ PWSK_SOCKET ListenSocket,
    _In_ PWSK_INSPECT_ID InspectID,
    _In_ WSK_INSPECT_ACTION Action,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_GET_LOCAL_ADDRESS)(
    _In_ PWSK_SOCKET Socket,
    _Out_ PSOCKADDR LocalAddress,
    _Inout_ PIRP Irp);

typedef struct _WSK_PROVIDER_LISTEN_DISPATCH
{
#ifdef __cplusplus
    WSK_PROVIDER_BASIC_DISPATCH Basic;
#else
    WSK_PROVIDER_BASIC_DISPATCH;
#endif
    PFN_WSK_BIND WskBind;
    PFN_WSK_ACCEPT WskAccept;
    PFN_WSK_INSPECT_COMPLETE WskInspectComplete;
    PFN_WSK_GET_LOCAL_ADDRESS WskGetLocalAddress;
} WSK_PROVIDER_LISTEN_DISPATCH, *PWSK_PROVIDER_LISTEN_DISPATCH;

#if (NTDDI_VERSION >= NTDDI_WIN8)
typedef struct _WSK_BUF_LIST {
    struct _WSK_BUF_LIST *Next;
    WSK_BUF Buffer;
} WSK_BUF_LIST, *PWSK_BUF_LIST;

typedef
NTSTATUS
(WSKAPI * PFN_WSK_SEND_MESSAGES)(
    _In_ PWSK_SOCKET Socket,
    _In_ PWSK_BUF_LIST BufferList,
    _Reserved_ ULONG Flags,
    _In_opt_ PSOCKADDR RemoteAddress,
    _In_ ULONG ControlInfoLength,
    _In_reads_bytes_opt_(ControlInfoLength) PCMSGHDR ControlInfo,
    _Inout_ PIRP Irp);
#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

typedef struct _WSK_PROVIDER_DATAGRAM_DISPATCH
{
#ifdef __cplusplus
    WSK_PROVIDER_BASIC_DISPATCH Basic;
#else
    WSK_PROVIDER_BASIC_DISPATCH;
#endif
    PFN_WSK_BIND WskBind;
    PFN_WSK_SEND_TO WskSendTo;
    PFN_WSK_RECEIVE_FROM WskReceiveFrom;
    PFN_WSK_RELEASE_DATAGRAM_INDICATION_LIST WskRelease;
    PFN_WSK_GET_LOCAL_ADDRESS WskGetLocalAddress;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    PFN_WSK_SEND_MESSAGES WskSendMessages;
#endif
} WSK_PROVIDER_DATAGRAM_DISPATCH, *PWSK_PROVIDER_DATAGRAM_DISPATCH;

typedef
NTSTATUS
(WSKAPI * PFN_WSK_CONNECT) (
    _In_ PWSK_SOCKET Socket,
    _In_ PSOCKADDR RemoteAddress,
    _Reserved_ ULONG Flags,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_GET_REMOTE_ADDRESS)(
    _In_ PWSK_SOCKET Socket,
    _Out_ PSOCKADDR RemoteAddress,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_SEND)(
    _In_ PWSK_SOCKET Socket,
    _In_ PWSK_BUF Buffer,
    _In_ ULONG Flags,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_RECEIVE)(
    _In_ PWSK_SOCKET Socket,
    _In_ PWSK_BUF Buffer,
    _In_ ULONG Flags,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_DISCONNECT)(
    _In_ PWSK_SOCKET Socket,
    _In_opt_ PWSK_BUF Buffer,
    _In_ ULONG Flags,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_RELEASE_DATA_INDICATION_LIST)(
    _In_ PWSK_SOCKET Socket,
    _In_ PWSK_DATA_INDICATION DataIndication);

typedef struct _WSK_PROVIDER_CONNECTION_DISPATCH
{
#ifdef __cplusplus
    WSK_PROVIDER_BASIC_DISPATCH Basic;
#else
    WSK_PROVIDER_BASIC_DISPATCH;
#endif
    PFN_WSK_BIND WskBind;
    PFN_WSK_CONNECT WskConnect;
    PFN_WSK_GET_LOCAL_ADDRESS WskGetLocalAddress;
    PFN_WSK_GET_REMOTE_ADDRESS WskGetRemoteAddress;
    PFN_WSK_SEND WskSend;
    PFN_WSK_RECEIVE WskReceive;
    PFN_WSK_DISCONNECT WskDisconnect;
    PFN_WSK_RELEASE_DATA_INDICATION_LIST WskRelease;
} WSK_PROVIDER_CONNECTION_DISPATCH, *PWSK_PROVIDER_CONNECTION_DISPATCH;

typedef
_Must_inspect_result_
WSK_INSPECT_ACTION
(WSKAPI * PFN_WSK_INSPECT_EVENT)(
    _In_opt_ PVOID SocketContext,
    _In_ PSOCKADDR LocalAddress,
    _In_ PSOCKADDR RemoteAddress,
    _In_opt_ PWSK_INSPECT_ID InspectID);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_ABORT_EVENT) (
    _In_opt_ PVOID SocketContext,
    _In_ PWSK_INSPECT_ID InspectID);

typedef
_At_(Irp->IoStatus.Information, __drv_allocatesMem(Mem))
NTSTATUS
(WSKAPI * PFN_WSK_SOCKET)(
    _In_ PWSK_CLIENT Client,
    _In_ ADDRESS_FAMILY AddressFamily,
    _In_ USHORT SocketType,
    _In_ ULONG Protocol,
    _In_ ULONG Flags,
    _In_opt_ PVOID SocketContext,
    _In_opt_ const VOID *Dispatch,
    _In_opt_ PEPROCESS OwningProcess,
    _In_opt_ PETHREAD OwningThread,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_ PIRP Irp);

typedef struct _WSK_TDI_MAP
{
    USHORT SocketType;
    ADDRESS_FAMILY AddressFamily;
    ULONG Protocol;
    PCWSTR TdiDeviceName;
} WSK_TDI_MAP, *PWSK_TDI_MAP;

typedef struct _WSK_TDI_MAP_INFO
{
    const ULONG ElementCount;
    _Field_size_(ElementCount) const WSK_TDI_MAP *Map;
} WSK_TDI_MAP_INFO, *PWSK_TDI_MAP_INFO;

typedef
NTSTATUS
(WSKAPI * PFN_WSK_CONTROL_CLIENT)(
    _In_ PWSK_CLIENT Client,
    _In_ ULONG ControlCode,
    _In_ SIZE_T InputSize,
    _In_reads_bytes_opt_(InputSize) PVOID InputBuffer,
    _In_ SIZE_T OutputSize,
    _Out_writes_bytes_opt_(OutputSize) PVOID OutputBuffer,
    _Out_opt_ SIZE_T *OutputSizeReturned,
    _Inout_opt_ PIRP Irp);

#if (NTDDI_VERSION >= NTDDI_WIN7)

typedef
_At_(*Result, __drv_allocatesMem(Mem))
NTSTATUS
(WSKAPI * PFN_WSK_GET_ADDRESS_INFO)(
    _In_ PWSK_CLIENT Client,
    _In_opt_ PUNICODE_STRING NodeName,
    _In_opt_ PUNICODE_STRING ServiceName,
    _In_opt_ ULONG NameSpace,
    _In_opt_ GUID *Provider,
    _In_opt_ PADDRINFOEXW Hints,
    _Outptr_ PADDRINFOEXW *Result,
    _In_opt_ PEPROCESS OwningProcess,
    _In_opt_ PETHREAD OwningThread,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_GET_NAME_INFO)(
    _In_ PWSK_CLIENT Client,
    _In_ PSOCKADDR SockAddr,
    _In_ ULONG SockAddrLength,
    _Out_opt_ PUNICODE_STRING NodeName,
    _Out_opt_ PUNICODE_STRING ServiceName,
    _In_ ULONG Flags,
    _In_opt_ PEPROCESS OwningProcess,
    _In_opt_ PETHREAD OwningThread,
    _Inout_ PIRP Irp);

typedef
_At_(AddrInfo, __drv_freesMem(Mem))
VOID
(WSKAPI * PFN_WSK_FREE_ADDRESS_INFO)(
    _In_ PWSK_CLIENT Client,
    _In_ PADDRINFOEXW AddrInfo);

#endif /* if (NTDDI_VERSION >= NTDDI_WIN7) */

typedef struct _WSK_EVENT_CALLBACK_CONTROL
{
    PNPIID NpiId;
    ULONG  EventMask;
} WSK_EVENT_CALLBACK_CONTROL, *PWSK_EVENT_CALLBACK_CONTROL;

typedef struct _WSK_EXTENSION_CONTROL_IN
{
    PNPIID NpiId;
    PVOID ClientContext;
    const VOID* ClientDispatch;
} WSK_EXTENSION_CONTROL_IN, *PWSK_EXTENSION_CONTROL_IN;

typedef struct _WSK_EXTENSION_CONTROL_OUT
{
    PVOID ProviderContext;
    const VOID* ProviderDispatch;
} WSK_EXTENSION_CONTROL_OUT, *PWSK_EXTENSION_CONTROL_OUT;

typedef
NTSTATUS
(WSKAPI * PFN_WSK_SEND_TO)(
    _In_ PWSK_SOCKET Socket,
    _In_ PWSK_BUF Buffer,
    _Reserved_ ULONG Flags,
    _In_opt_ PSOCKADDR RemoteAddress,
    _In_ ULONG ControlInfoLength,
    _In_reads_bytes_opt_(ControlInfoLength) PCMSGHDR ControlInfo,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_RECEIVE_FROM)(
    _In_ PWSK_SOCKET Socket,
    _In_ PWSK_BUF Buffer,
    _Reserved_ ULONG Flags,
    _Out_opt_ PSOCKADDR RemoteAddress,
    _Inout_ PULONG ControlLength,
    _Out_writes_bytes_opt_(*ControlLength) PCMSGHDR ControlInfo,
    _Out_opt_ PULONG ControlFlags,
    _Inout_ PIRP Irp);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_RELEASE_DATAGRAM_INDICATION_LIST)(
    _In_ PWSK_SOCKET Socket,
    _In_ PWSK_DATAGRAM_INDICATION DatagramIndication);

typedef
NTSTATUS
(WSKAPI * PFN_WSK_CLIENT_EVENT)(
    _In_opt_ PVOID ClientContext,
    _In_ ULONG EventType,
    _In_reads_bytes_opt_(InformationLength) PVOID Information,
    _In_ SIZE_T InformationLength);

typedef struct _WSK_CLIENT_DISPATCH
{
    USHORT Version;
    USHORT Reserved;
    PFN_WSK_CLIENT_EVENT WskClientEvent;
} WSK_CLIENT_DISPATCH, *PWSK_CLIENT_DISPATCH;

typedef struct _WSK_CLIENT_LISTEN_DISPATCH
{
    PFN_WSK_ACCEPT_EVENT WskAcceptEvent;
    PFN_WSK_INSPECT_EVENT WskInspectEvent;
    PFN_WSK_ABORT_EVENT WskAbortEvent;
} WSK_CLIENT_LISTEN_DISPATCH, *PWSK_CLIENT_LISTEN_DISPATCH;

typedef struct _WSK_CLIENT_DATAGRAM_DISPATCH
{
    PFN_WSK_RECEIVE_FROM_EVENT WskReceiveFromEvent;
} WSK_CLIENT_DATAGRAM_DISPATCH, *PWSK_CLIENT_DATAGRAM_DISPATCH;

typedef struct _WSK_PROVIDER_DISPATCH
{
    USHORT Version;
    USHORT Reserved;
    PFN_WSK_SOCKET WskSocket;
    PFN_WSK_SOCKET_CONNECT WskSocketConnect;
    PFN_WSK_CONTROL_CLIENT WskControlClient;
#if (NTDDI_VERSION >= NTDDI_WIN7)
    PFN_WSK_GET_ADDRESS_INFO WskGetAddressInfo;
    PFN_WSK_FREE_ADDRESS_INFO WskFreeAddressInfo;
    PFN_WSK_GET_NAME_INFO WskGetNameInfo;
#endif
} WSK_PROVIDER_DISPATCH, *PWSK_PROVIDER_DISPATCH;


typedef struct _WSK_CLIENT_NPI
{
    PVOID ClientContext;
    const WSK_CLIENT_DISPATCH *Dispatch;
} WSK_CLIENT_NPI, *PWSK_CLIENT_NPI;

typedef struct _WSK_PROVIDER_NPI
{
    PWSK_CLIENT Client;
    const WSK_PROVIDER_DISPATCH *Dispatch;
} WSK_PROVIDER_NPI, *PWSK_PROVIDER_NPI;

typedef struct _WSK_REGISTRATION
{
    ULONGLONG ReservedRegistrationState;
    PVOID ReservedRegistrationContext;
    KSPIN_LOCK ReservedRegistrationLock;
} WSK_REGISTRATION, *PWSK_REGISTRATION;

typedef struct _WSK_PROVIDER_CHARACTERISTICS
{
    USHORT HighestVersion;
    USHORT LowestVersion;
} WSK_PROVIDER_CHARACTERISTICS, *PWSK_PROVIDER_CHARACTERISTICS;

typedef struct _WSK_TRANSPORT
{
    USHORT Version;
    USHORT SocketType;
    ULONG Protocol;
    ADDRESS_FAMILY AddressFamily;
    GUID ProviderId;
} WSK_TRANSPORT, *PWSK_TRANSPORT;

_Must_inspect_result_
NTSTATUS
WskRegister(
    _In_ PWSK_CLIENT_NPI WskClientNpi,
    _Out_ PWSK_REGISTRATION WskRegistration);

_Must_inspect_result_
NTSTATUS
WskCaptureProviderNPI(
    _In_ PWSK_REGISTRATION WskRegistration,
    _In_ ULONG WaitTimeout,
    _Out_ PWSK_PROVIDER_NPI WskProviderNpi);

VOID
WskReleaseProviderNPI(
    _In_ PWSK_REGISTRATION WskRegistration);

_Must_inspect_result_
NTSTATUS
WskQueryProviderCharacteristics(
    _In_ PWSK_REGISTRATION WskRegistration,
    _Out_ PWSK_PROVIDER_CHARACTERISTICS WskProviderCharacteristics);

VOID
WskDeregister(
    _In_ PWSK_REGISTRATION WskRegistration);

#ifdef __cplusplus
}
#endif


