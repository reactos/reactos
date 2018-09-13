/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    proc.h

Abstract:

    This module contains global function prototype definitions for the
    Winsock 2 to Winsock 1.1 Mapper Service Provider.

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#ifndef _PROC_H_
#define _PROC_H_


//
// SPI entrypoints.
//

SOCKET
WSPAPI
WSPAccept(
    SOCKET s,
    struct sockaddr FAR * addr,
    LPINT addrlen,
    LPCONDITIONPROC lpfnCondition,
    ULONG_PTR dwCallbackData,
    LPINT lpErrno
    );

INT
WSPAPI
WSPAddressToString(
    LPSOCKADDR lpsaAddress,
    DWORD dwAddressLength,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPWSTR lpszAddressString,
    LPDWORD lpdwAddressStringLength,
    LPINT lpErrno
    );

INT
WSPAPI
WSPAsyncSelect(
    SOCKET s,
    HWND hWnd,
    unsigned int wMsg,
    long lEvent,
    LPINT lpErrno
    );

INT
WSPAPI
WSPBind(
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPINT lpErrno
    );

INT
WSPAPI
WSPCancelBlockingCall(
    LPINT lpErrno
    );

INT
WSPAPI
WSPCleanup(
    LPINT lpErrno
    );

INT
WSPAPI
WSPCloseSocket(
    SOCKET s,
    LPINT lpErrno
    );

INT
WSPAPI
WSPConnect(
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    LPINT lpErrno
    );

INT
WSPAPI
WSPDuplicateSocket(
    SOCKET s,
    DWORD dwProcessId,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPINT lpErrno
    );

INT
WSPAPI
WSPEnumNetworkEvents(
    SOCKET s,
    WSAEVENT hEventObject,
    LPWSANETWORKEVENTS lpNetworkEvents,
    LPINT lpErrno
    );

INT
WSPAPI
WSPEventSelect(
    SOCKET s,
    WSAEVENT hEventObject,
    long lNetworkEvents,
    LPINT lpErrno
    );

INT
WSPAPI
WSPGetOverlappedResult(
    SOCKET s,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD lpcbTransfer,
    BOOL fWait,
    LPDWORD lpdwFlags,
    LPINT lpErrno
    );

INT
WSPAPI
WSPGetPeerName(
    SOCKET s,
    struct sockaddr FAR * name,
    LPINT namelen,
    LPINT lpErrno
    );

INT
WSPAPI
WSPGetSockName(
    SOCKET s,
    struct sockaddr FAR * name,
    LPINT namelen,
    LPINT lpErrno
    );

INT
WSPAPI
WSPGetSockOpt(
    SOCKET s,
    int level,
    int optname,
    char FAR * optval,
    LPINT optlen,
    LPINT lpErrno
    );

BOOL
WSPAPI
WSPGetQOSByName(
    SOCKET s,
    LPWSABUF lpQOSName,
    LPQOS lpQOS,
    LPINT lpErrno
    );

INT
WSPAPI
WSPIoctl(
    SOCKET s,
    DWORD dwIoControlCode,
    LPVOID lpvInBuffer,
    DWORD cbInBuffer,
    LPVOID lpvOutBuffer,
    DWORD cbOutBuffer,
    LPDWORD lpcbBytesReturned,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    );

SOCKET
WSPAPI
WSPJoinLeaf(
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    DWORD dwFlags,
    LPINT lpErrno
    );

INT
WSPAPI
WSPListen(
    SOCKET s,
    int backlog,
    LPINT lpErrno
    );

INT
WSPAPI
WSPRecv(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd,
    LPDWORD lpFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    );

INT
WSPAPI
WSPRecvDisconnect(
    SOCKET s,
    LPWSABUF lpInboundDisconnectData,
    LPINT lpErrno
    );

INT
WSPAPI
WSPRecvFrom(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd,
    LPDWORD lpFlags,
    struct sockaddr FAR * lpFrom,
    LPINT lpFromlen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    );

INT
WSPAPI
WSPSelect(
    int nfds,
    fd_set FAR * readfds,
    fd_set FAR * writefds,
    fd_set FAR * exceptfds,
    const struct timeval FAR * timeout,
    LPINT lpErrno
    );

INT
WSPAPI
WSPSend(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    );

INT
WSPAPI
WSPSendDisconnect(
    SOCKET s,
    LPWSABUF lpOutboundDisconnectData,
    LPINT lpErrno
    );

INT
WSPAPI
WSPSendTo(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    const struct sockaddr FAR * lpTo,
    int iTolen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    );

INT
WSPAPI
WSPSetSockOpt(
    SOCKET s,
    int level,
    int optname,
    const char FAR * optval,
    int optlen,
    LPINT lpErrno
    );

INT
WSPAPI
WSPShutdown(
    SOCKET s,
    int how,
    LPINT lpErrno
    );

SOCKET
WSPAPI
WSPSocket(
    int af,
    int type,
    int protocol,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    GROUP g,
    DWORD dwFlags,
    LPINT lpErrno
    );

INT
WSPAPI
WSPStartup(
    IN WORD wVersionRequested,
    OUT LPWSPDATA lpWSPData,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN WSPUPCALLTABLE UpcallTable,
    OUT LPWSPPROC_TABLE lpProcTable
    );

INT
WSPAPI
WSPStringToAddress(
    LPWSTR AddressString,
    INT AddressFamily,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPSOCKADDR lpAddress,
    LPINT lpAddressLength,
    LPINT lpErrno
    );


//
// Initialization & termination routines.
//

BOOL
SockInitializeDll(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    );

PSOCK_TLS_DATA
SockInitializeThread(
    VOID
    );


//
// API helpers.
//

INT
SockEnterApi(
    IN BOOL MustBeStarted,
    IN BOOL AllowReentrancy
    );


//
// Thread data accessors.
//

#define SOCK_GET_THREAD_DATA() TlsGetValue( SockTlsSlot )
#define SOCK_SET_THREAD_DATA(p) TlsSetValue( SockTlsSlot, (LPVOID)(p) )


//
// GUID management.
//

#define GUIDS_ARE_EQUAL(a,b) \
            (!memcmp( (LPVOID)(a), (LPVOID)(b), sizeof(GUID) ))


//
// Hooker management.
//

PHOOKER_INFORMATION
SockFindAndReferenceHooker(
    IN LPGUID ProviderId
    );

VOID
SockReferenceHooker(
    IN PHOOKER_INFORMATION HookerInfo
    );

VOID
SockDereferenceHooker(
    IN PHOOKER_INFORMATION HookerInfo
    );

VOID
SockFreeAllHookers(
    VOID
    );


//
// Socket management.
//

PSOCKET_INFORMATION
SockFindAndReferenceWS2Socket(
    IN SOCKET Socket
    );

PSOCKET_INFORMATION
SockFindAndReferenceWS1Socket(
    IN SOCKET Socket
    );

VOID
SockReferenceSocket(
    IN PSOCKET_INFORMATION SocketInfo
    );

VOID
SockDereferenceSocket(
    IN PSOCKET_INFORMATION SocketInfo
    );

PSOCKET_INFORMATION
SockCreateSocket(
    IN PHOOKER_INFORMATION HookerInfo,
    IN INT AddressFamily,
    IN INT SocketType,
    IN INT Protocol,
    IN INT MaxSockaddrLength,
    IN INT MinSockaddrLength,
    IN DWORD Flags,
    IN DWORD CatalogEntryId,
    IN DWORD ServiceFlags1,
    IN DWORD ServiceFlags2,
    IN DWORD ServiceFlags3,
    IN DWORD ServiceFlags4,
    IN DWORD ProviderFlags,
    IN GROUP Group,
    IN SOCK_GROUP_TYPE GroupType,
    IN SOCKET WS1Handle,
    OUT LPINT Error
    );

VOID
SockPrepareForBlockingHook(
    IN PSOCKET_INFORMATION SocketInfo
    );

VOID
SockPreApiCallout(
    VOID
    );

VOID
SockPostApiCallout(
    VOID
    );

VOID
SockBuildProtocolInfoForSocket(
    IN PSOCKET_INFORMATION SocketInfo,
    IN LPWSAPROTOCOL_INFOW ProtocolInfo
    );


//
// Thread management routines.
//

HANDLE
WINAPI
SockCreateWorkerThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    );

HANDLE
WINAPI
SockCreateHookerThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    );


//
// Shared data manager routines.
//

BOOL
SockInitializeSharedData(
    VOID
    );

VOID
SockTerminateSharedData(
    VOID
    );

VOID
SockAcquireSharedDataLock(
    VOID
    );

VOID
SockReleaseSharedDataLock(
    VOID
    );


//
// Overlapped IO routines.
//

VOID
SockCompleteRequest(
    PSOCKET_INFORMATION SocketInfo,
    DWORD Status,
    DWORD Information,
    LPWSAOVERLAPPED Overlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
    LPWSATHREADID ThreadId
    );

BOOL
SockInitializeOverlappedThread(
    PSOCKET_INFORMATION SocketInfo,
    PSOCK_OVERLAPPED_DATA OverlappedData,
    LPTHREAD_START_ROUTINE ThreadStartAddress
    );


//
// Group management routines.
//

BOOL
SockInitializeGroupManager(
    VOID
    );

VOID
SockTerminateGroupManager(
    VOID
    );

VOID
SockAcquireGroupLock(
    VOID
    );

VOID
SockReleaseGroupLock(
    VOID
    );

BOOL
SockReferenceGroup(
    GROUP Group,
    PSOCK_GROUP_TYPE GroupType
    );

BOOL
SockDereferenceGroup(
    GROUP Group
    );

BOOL
SockGetGroup(
    GROUP * Group,
    PSOCK_GROUP_TYPE GroupType
    );

BOOL
SockValidateConstrainedGroup(
    GROUP Group,
    SOCK_GROUP_TYPE GroupType,
    SOCKADDR * Address,
    INT AddressLength
    );


//
// Buffer management routines.
//

DWORD
SockCalcBufferArrayByteLength(
    LPWSABUF BufferArray,
    DWORD BufferCount
    );

DWORD
SockCopyFlatBufferToBufferArray(
    LPWSABUF BufferArray,
    DWORD BufferCount,
    PVOID FlatBuffer,
    DWORD FlatBufferLength
    );

DWORD
SockCopyBufferArrayToFlatBuffer(
    PVOID FlatBuffer,
    DWORD FlatBufferLength,
    LPWSABUF BufferArray,
    DWORD BufferCount
    );


//
// Select() helpers.
//

VOID
SockDestroyAsyncThread(
    VOID
    );

VOID
SockIndicateEvent(
    PSOCKET_INFORMATION SocketInfo,
    INT NetworkEvent,
    INT Status
    );


#endif // _PROC_H_

