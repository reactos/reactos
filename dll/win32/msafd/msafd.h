/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        include/msafd.h
 * PURPOSE:     Ancillary Function Driver DLL header
 */

#ifndef __MSAFD_H
#define __MSAFD_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <ws2spi.h>
#define NTOS_MODE_USER
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

/* This includes ntsecapi.h so it needs to come after the NDK */
#include <wsahelp.h>
#include <tdi.h>
#include <afd/shared.h>
#include "include/helpers.h"

extern HANDLE GlobalHeap;
extern WSPUPCALLTABLE Upcalls;
extern LPWPUCOMPLETEOVERLAPPEDREQUEST lpWPUCompleteOverlappedRequest;
extern LIST_ENTRY SockHelpersListHead;
extern HANDLE SockEvent;
extern HANDLE SockAsyncCompletionPort;
extern BOOLEAN SockAsyncSelectCalled;

typedef enum _SOCKET_STATE {
    SocketOpen,
    SocketBound,
    SocketBoundUdp,
    SocketConnected,
    SocketClosed
} SOCKET_STATE, *PSOCKET_STATE;

typedef struct _SOCK_SHARED_INFO {
    SOCKET_STATE				State;
    INT							AddressFamily;
    INT							SocketType;
    INT							Protocol;
    INT							SizeOfLocalAddress;
    INT							SizeOfRemoteAddress;
    struct linger				LingerData;
    ULONG						SendTimeout;
    ULONG						RecvTimeout;
    ULONG						SizeOfRecvBuffer;
    ULONG						SizeOfSendBuffer;
    struct {
        BOOLEAN					Listening:1;
        BOOLEAN					Broadcast:1;
        BOOLEAN					Debug:1;
        BOOLEAN					OobInline:1;
        BOOLEAN					ReuseAddresses:1;
        BOOLEAN					ExclusiveAddressUse:1;
        BOOLEAN					NonBlocking:1;
        BOOLEAN					DontUseWildcard:1;
        BOOLEAN					ReceiveShutdown:1;
        BOOLEAN					SendShutdown:1;
        BOOLEAN					UseDelayedAcceptance:1;
		BOOLEAN					UseSAN:1;
    }; // Flags
    DWORD						CreateFlags;
    DWORD						CatalogEntryId;
    DWORD						ServiceFlags1;
    DWORD						ProviderFlags;
    GROUP						GroupID;
    DWORD						GroupType;
    INT							GroupPriority;
    INT							SocketLastError;
    HWND						hWnd;
    LONG						Unknown;
    DWORD						SequenceNumber;
    UINT						wMsg;
    LONG						AsyncEvents;
    LONG						AsyncDisabledEvents;
} SOCK_SHARED_INFO, *PSOCK_SHARED_INFO;

typedef struct _SOCKET_INFORMATION {
	ULONG RefCount;
	SOCKET Handle;
	SOCK_SHARED_INFO SharedData;
	DWORD HelperEvents;
	PHELPER_DATA HelperData;
	PVOID HelperContext;
	PSOCKADDR LocalAddress;
	PSOCKADDR RemoteAddress;
	HANDLE TdiAddressHandle;
	HANDLE TdiConnectionHandle;
	PVOID AsyncData;
	HANDLE EventObject;
	LONG NetworkEvents;
	CRITICAL_SECTION Lock;
	PVOID SanData;
	BOOL TrySAN;
	SOCKADDR WSLocalAddress;
	SOCKADDR WSRemoteAddress;
	struct _SOCKET_INFORMATION *NextSocket;
} SOCKET_INFORMATION, *PSOCKET_INFORMATION;


typedef struct _SOCKET_CONTEXT {
	SOCK_SHARED_INFO SharedData;
	ULONG SizeOfHelperData;
	ULONG Padding;
	SOCKADDR LocalAddress;
	SOCKADDR RemoteAddress;
	/* Plus Helper Data */
} SOCKET_CONTEXT, *PSOCKET_CONTEXT;

typedef struct _ASYNC_DATA {
	PSOCKET_INFORMATION ParentSocket;
	DWORD SequenceNumber;
	IO_STATUS_BLOCK IoStatusBlock;
	AFD_POLL_INFO AsyncSelectInfo;
} ASYNC_DATA, *PASYNC_DATA;

SOCKET
WSPAPI
WSPAccept(
    IN      SOCKET s,
    OUT     LPSOCKADDR addr,
    IN OUT  LPINT addrlen,
    IN      LPCONDITIONPROC lpfnCondition,
    IN      DWORD dwCallbackData,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPAddressToString(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPWSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPAsyncSelect(
    IN  SOCKET s,
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  LONG lEvent,
    OUT LPINT lpErrno);

INT
WSPAPI WSPBind(
    IN  SOCKET s,
    IN  CONST SOCKADDR *name,
    IN  INT namelen,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPCancelBlockingCall(
    OUT LPINT lpErrno);

INT
WSPAPI
WSPCleanup(
    OUT LPINT lpErrno);

INT
WSPAPI
WSPCloseSocket(
    IN	SOCKET s,
    OUT	LPINT lpErrno);

INT
WSPAPI
WSPConnect(
    IN  SOCKET s,
    IN  CONST SOCKADDR *name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPDuplicateSocket(
    IN  SOCKET s,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPEnumNetworkEvents(
    IN  SOCKET s,
    IN  WSAEVENT hEventObject,
    OUT LPWSANETWORKEVENTS lpNetworkEvents,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPEventSelect(
    IN  SOCKET s,
    IN  WSAEVENT hEventObject,
    IN  LONG lNetworkEvents,
    OUT LPINT lpErrno);

BOOL
WSPAPI
WSPGetOverlappedResult(
    IN  SOCKET s,
    IN  LPWSAOVERLAPPED lpOverlapped,
    OUT LPDWORD lpcbTransfer,
    IN  BOOL fWait,
    OUT LPDWORD lpdwFlags,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPGetPeerName(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  LPINT namelen,
    OUT     LPINT lpErrno);

BOOL
WSPAPI
WSPGetQOSByName(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpQOSName,
    OUT     LPQOS lpQOS,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPGetSockName(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  LPINT namelen,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPGetSockOpt(
    IN      SOCKET s,
    IN      INT level,
    IN      INT optname,
    OUT	    CHAR FAR* optval,
    IN OUT  LPINT optlen,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPIoctl(
    IN  SOCKET s,
    IN  DWORD dwIoControlCode,
    IN  LPVOID lpvInBuffer,
    IN  DWORD cbInBuffer,
    OUT LPVOID lpvOutBuffer,
    IN  DWORD cbOutBuffer,
    OUT LPDWORD lpcbBytesReturned,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

SOCKET
WSPAPI
WSPJoinLeaf(
    IN  SOCKET s,
    IN  CONST SOCKADDR *name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS,
    IN  DWORD dwFlags,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPListen(
    IN  SOCKET s,
    IN  INT backlog,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPRecv(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpBuffers,
    IN      DWORD dwBufferCount,
    OUT     LPDWORD lpNumberOfBytesRecvd,
    IN OUT  LPDWORD lpFlags,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN      LPWSATHREADID lpThreadId,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPRecvDisconnect(
    IN  SOCKET s,
    OUT LPWSABUF lpInboundDisconnectData,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPRecvFrom(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpBuffers,
    IN      DWORD dwBufferCount,
    OUT     LPDWORD lpNumberOfBytesRecvd,
    IN OUT  LPDWORD lpFlags,
    OUT     LPSOCKADDR lpFrom,
    IN OUT  LPINT lpFromlen,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN      LPWSATHREADID lpThreadId,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPSelect(
    IN      INT nfds,
    IN OUT  fd_set *readfds,
    IN OUT  fd_set *writefds,
    IN OUT  fd_set *exceptfds,
    IN      CONST struct timeval *timeout,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPSend(
    IN  SOCKET s,
    IN  LPWSABUF lpBuffers,
    IN  DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN  DWORD dwFlags,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPSendDisconnect(
    IN  SOCKET s,
    IN  LPWSABUF lpOutboundDisconnectData,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPSendTo(
    IN  SOCKET s,
    IN  LPWSABUF lpBuffers,
    IN  DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN  DWORD dwFlags,
    IN  CONST SOCKADDR *lpTo,
    IN  INT iTolen,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPSetSockOpt(
    IN  SOCKET s,
    IN  INT level,
    IN  INT optname,
    IN  CONST CHAR FAR* optval,
    IN  INT optlen,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPShutdown(
    IN  SOCKET s,
    IN  INT how,
    OUT LPINT lpErrno);

SOCKET
WSPAPI
WSPSocket(
    IN  INT af,
    IN  INT type,
    IN  INT protocol,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  GROUP g,
    IN  DWORD dwFlags,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPStringToAddress(
    IN      LPWSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength,
    OUT     LPINT lpErrno);


PSOCKET_INFORMATION GetSocketStructure(
	SOCKET Handle
);

INT TranslateNtStatusError( NTSTATUS Status );

VOID DeleteSocketStructure( SOCKET Handle );

int GetSocketInformation(
	PSOCKET_INFORMATION Socket,
	ULONG				AfdInformationClass,
    PBOOLEAN            Boolean      OPTIONAL,
	PULONG              Ulong        OPTIONAL,
	PLARGE_INTEGER		LargeInteger OPTIONAL
);

int SetSocketInformation(
	PSOCKET_INFORMATION Socket,
	ULONG				AfdInformationClass,
    PBOOLEAN            Boolean      OPTIONAL,
	PULONG				Ulong		 OPTIONAL,
	PLARGE_INTEGER		LargeInteger OPTIONAL
);

int CreateContext(
	PSOCKET_INFORMATION Socket
);

int SockAsyncThread(
	PVOID ThreadParam
);

VOID
SockProcessAsyncSelect(
	PSOCKET_INFORMATION Socket,
	PASYNC_DATA AsyncData
);

VOID
SockAsyncSelectCompletionRoutine(
	PVOID Context,
	PIO_STATUS_BLOCK IoStatusBlock
);

BOOLEAN
SockCreateOrReferenceAsyncThread(
	VOID
);

BOOLEAN SockGetAsyncSelectHelperAfdHandle(
	VOID
);

VOID SockProcessQueuedAsyncSelect(
	PVOID Context,
	PIO_STATUS_BLOCK IoStatusBlock
);

VOID
SockReenableAsyncSelectEvent (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG Event
    );

typedef VOID (*PASYNC_COMPLETION_ROUTINE)(PVOID Context, PIO_STATUS_BLOCK IoStatusBlock);

FORCEINLINE
DWORD
MsafdReturnWithErrno(NTSTATUS Status,
                     LPINT Errno,
                     DWORD Received,
                     LPDWORD ReturnedBytes)
{
    if (Errno)
    {
        *Errno = TranslateNtStatusError(Status);

        if (ReturnedBytes)
            *ReturnedBytes = (*Errno == 0) ? Received : 0;

        return (*Errno == 0) ? 0 : SOCKET_ERROR;
    }
    else
    {
        DbgPrint("%s: Received invalid lpErrno pointer!\n", __FUNCTION__);

        if (ReturnedBytes)
            *ReturnedBytes = (Status == STATUS_SUCCESS) ? Received : 0;

        return (Status == STATUS_SUCCESS) ? 0 : SOCKET_ERROR;
    }
}

#endif /* __MSAFD_H */
