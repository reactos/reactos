/*++

Copyright (c) 1992-1996 Microsoft Corporation

Module Name:

    SockProc.h

Abstract:

    This module contains prototypes for WinSock support routines.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#ifndef _SOCKPROC_
#define _SOCKPROC_

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
    DWORD_PTR dwCallbackData,
    LPINT lpErrno
    );

int
WSPAPI
WSPAddressToString(
    LPSOCKADDR lpsaAddress,
    DWORD dwAddressLength,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPWSTR lpszAddressString,
    LPDWORD lpdwAddressStringLength,
    LPINT lpErrno
    );

int
WSPAPI
WSPAsyncSelect(
    SOCKET s,
    HWND hWnd,
    unsigned int wMsg,
    long lEvent,
    LPINT lpErrno
    );

int
WSPAPI
WSPBind(
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPINT lpErrno
    );

int
WSPAPI
WSPCancelBlockingCall(
    LPINT lpErrno
    );

int
WSPAPI
WSPCleanup(
    LPINT lpErrno
    );

int
WSPAPI
WSPCloseSocket(
    SOCKET s,
    LPINT lpErrno
    );

int
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

int
WSPAPI
WSPDuplicateSocket(
    SOCKET s,
    DWORD dwProcessId,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPINT lpErrno
    );

int
WSPAPI
WSPEnumNetworkEvents(
    SOCKET s,
    WSAEVENT hEventObject,
    LPWSANETWORKEVENTS lpNetworkEvents,
    LPINT lpErrno
    );

int
WSPAPI
WSPEventSelect(
    SOCKET s,
    WSAEVENT hEventObject,
    long lNetworkEvents,
    LPINT lpErrno
    );

int
WSPAPI
WSPGetOverlappedResult(
    SOCKET s,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD lpcbTransfer,
    BOOL fWait,
    LPDWORD lpdwFlags,
    LPINT lpErrno
    );

int
WSPAPI
WSPGetPeerName(
    SOCKET s,
    struct sockaddr FAR * name,
    LPINT namelen,
    LPINT lpErrno
    );

int
WSPAPI
WSPGetSockName(
    SOCKET s,
    struct sockaddr FAR * name,
    LPINT namelen,
    LPINT lpErrno
    );

int
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

int
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

int
WSPAPI
WSPListen(
    SOCKET s,
    int backlog,
    LPINT lpErrno
    );

int
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

int
WSPAPI
WSPRecvDisconnect(
    SOCKET s,
    LPWSABUF lpInboundDisconnectData,
    LPINT lpErrno
    );

int
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

int
WSPAPI
WSPSelect(
    int nfds,
    fd_set FAR * readfds,
    fd_set FAR * writefds,
    fd_set FAR * exceptfds,
    const struct timeval FAR * timeout,
    LPINT lpErrno
    );

int
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

int
WSPAPI
WSPSendDisconnect(
    SOCKET s,
    LPWSABUF lpOutboundDisconnectData,
    LPINT lpErrno
    );

int
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

int
WSPAPI
WSPSetSockOpt(
    SOCKET s,
    int level,
    int optname,
    const char FAR * optval,
    int optlen,
    LPINT lpErrno
    );

int
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

int
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
// Reference/dereference routines for socket blocks.
//

VOID
SockDestroySocket (
    IN PSOCKET_INFORMATION Socket
    );
#define SockDereferenceSocket(_s) {								\
    if ( WahDereferenceHandleContext (&((_s)->HContext)) == 0 )	\
		SockDestroySocket ((_s));								\
	}


PSOCKET_INFORMATION
SockFindAndReferenceSocket (
    IN SOCKET Handle,
    IN BOOLEAN AttemptImport
    );


#define SockReferenceSocket(_S)					\
    WahReferenceHandleContext (&((_S)->HContext))

INT
SockIsAddressConsistentWithConstrainedGroup(
    IN PSOCKET_INFORMATION Socket,
    IN GROUP Group,
    IN PSOCKADDR SocketAddress,
    IN INT SocketAddressLength
    );

//
// Standard routine for blocking on a handle.
//

BOOL
SockWaitForSingleObject (
    IN HANDLE Handle,
    IN SOCKET SocketHandle,
    IN DWORD BlockingHookUsage,
    IN DWORD TimeoutUsage
    );

//
// Standard I/O completion routine.
//

VOID
WINAPI
SockIoCompletion (
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    );

//
// Error translation routines.
//

int
SockNtStatusToSocketError (
    IN NTSTATUS Status
    );

//
// Routines for interacting with the asynchronous processing thread.
//

BOOLEAN
SockCheckAndReferenceAsyncThread (
    VOID
    );

#define SockDereferenceAsyncThread()        \
        InterlockedDecrement (&SockAsyncThreadReferenceCount)

VOID
SockTerminateAsyncThread (
    VOID
    );

VOID
SockAsyncConnectCompletion (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

VOID
SockAsyncSelectCompletion (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

VOID
SockProcessQueuedAsyncSelect (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

VOID
SockHandleAsyncIndication(
    PASYNC_COMPLETION_PROC  Proc,
	PVOID					Context,
	PIO_STATUS_BLOCK        IoStatus
	);


BOOLEAN
SockIsSocketConnected (
    IN PSOCKET_INFORMATION Socket
    );

INT
SockPostProcessConnect (
    IN PSOCKET_INFORMATION Socket
    );

BOOL
SockCheckAndInitAsyncConnectHelper (
    VOID
    );
//
// Routine to handle reenabling of async select events.
//

VOID
SockReenableAsyncSelectEvent (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG Event
    );


int
SockEnterApiSlow (
	OUT PWINSOCK_TLS_DATA	*tlsData
    );

//
// Routine called at every entrypoint of the winsock DLL.
//
//int
//SockEnterApi (
//	  OUT PWINSOCK_TLS_DATA	*tlsData
//    );
#define SockEnterApi(_t)								\
		((!SockProcessTerminating						\
				&& (SockWspStartupCount>0)				\
				&& ((*(_t)=GET_THREAD_DATA())!=NULL))	\
			? NO_ERROR									\
			: SockEnterApiSlow(_t)						\
		)


//
// Routines for interacting with helper DLLs.
//

VOID
SockFreeHelperDlls (
    VOID
    );

INT
SockGetTdiHandles (
    IN PSOCKET_INFORMATION Socket
    );

INT
SockGetTdiName (
    IN PINT AddressFamily,
    IN PINT SocketType,
    IN PINT Protocol,
    IN GROUP Group,
    IN DWORD Flags,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID *HelperDllSocketContext,
    OUT PWINSOCK_HELPER_DLL_INFO *HelperDll,
    OUT PDWORD NotificationEvents
    );

VOID
SockFreeHelperDll (
    PWINSOCK_HELPER_DLL_INFO helperDll
	);

#define SockReferenceHelper(_hdll)		\
			InterlockedIncrement(&(_hdll)->RefCount)

#define SockDereferenceHelper(_hdll)	{					\
			if (InterlockedDecrement(&(_hdll)->RefCount)==0)\
				SockFreeHelperDll((_hdll));					\
	}

BOOL
SockIsTripleInMapping (
    IN PWINSOCK_MAPPING Mapping,
    IN INT AddressFamily,
    OUT PBOOLEAN AddressFamilyFound,
    IN INT SocketType,
    OUT PBOOLEAN SocketTypeFound,
    IN INT Protocol,
    OUT PBOOLEAN ProtocolFound,
    OUT PBOOLEAN InvalidProtocolMatch
    );

INT
SockLoadHelperDll (
    IN PWSTR TransportName,
    IN PWINSOCK_MAPPING Mapping,
    OUT PWINSOCK_HELPER_DLL_INFO *HelperDll
    );

INT
SockLoadTransportList (
    OUT PWSTR *TransportList
    );

INT
SockLoadTransportMapping (
    IN PWSTR TransportName,
    OUT PWINSOCK_MAPPING *Mapping
    );

INT
SockNotifyHelperDll (
    IN PSOCKET_INFORMATION Socket,
    IN DWORD Event
    );


INT
SockSetHandleContext (
    IN PSOCKET_INFORMATION Socket
    );

INT
SockUpdateWindowSizes (
    IN PSOCKET_INFORMATION Socket,
    IN BOOLEAN AlwaysUpdate
    );

BOOL
SockDefaultValidateAddressForConstrainedGroup(
    IN PSOCKADDR Sockaddr1,
    IN PSOCKADDR Sockaddr2,
    IN INT SockaddrLength
    );

//
// Routines for building addresses.
//

INT
SockBuildSockaddr (
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength,
    IN PTRANSPORT_ADDRESS TdiAddress
    );

INT
SockBuildTdiAddress (
    OUT PTRANSPORT_ADDRESS TdiAddress,
    IN PSOCKADDR Sockaddr,
    IN INT SockaddrLength
    );

//
// The default blocking function we'll use.
//

BOOL
SockDefaultBlockingHook (
    VOID
    );

//
// DLL initialization routines.
//

BOOL
SockInitialize (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    );

BOOL
SockThreadInitialize(
    VOID
    );

//
// Routine for getting and setting socket information from/to AFD.
//

INT
SockGetInformation (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG InformationType,
    IN PVOID AdditionalInputInfo OPTIONAL,
    IN ULONG AdditionalInputInfoLength,
    IN OUT PBOOLEAN Boolean OPTIONAL,
    IN OUT PULONG Ulong OPTIONAL,
    IN OUT PLARGE_INTEGER LargeInteger OPTIONAL
    );

INT
SockSetInformation (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG InformationType,
    IN PBOOLEAN Boolean OPTIONAL,
    IN PULONG Ulong OPTIONAL,
    IN PLARGE_INTEGER LargeInteger OPTIONAL
    );

INT
SockBuildProtocolInfoForSocket(
    IN PSOCKET_INFORMATION Socket,
    OUT LPWSAPROTOCOL_INFOW ProtocolInfo
    );

//
// Routine for passing connect data and options to and from AFD.
//

INT
SockSetConnectData(
    IN PSOCKET_INFORMATION Socket,
    IN ULONG AfdIoctl,
    IN PCHAR Buffer,
    IN INT BufferLength,
    OUT PINT OutBufferLength
    );

INT
SockGetConnectData(
    IN PSOCKET_INFORMATION Socket,
    IN ULONG AfdIoctl,
    IN PCHAR Buffer,
    IN INT BufferLength,
    OUT PINT OutBufferLength
    );

//
// Core accept code.
//

INT
SockCoreAccept (
    IN PSOCKET_INFORMATION ListenSocket,
    IN PSOCKET_INFORMATION AcceptSocket
    );

//
// Helper routines for WSPAsyncSelect and WSPEventSelect.
//

int
SockAsyncSelectHelper(
    PSOCKET_INFORMATION Socket,
    HWND hWnd,
    unsigned int wMsg,
    long lEvent
    );

int
SockEventSelectHelper(
    PSOCKET_INFORMATION Socket,
    WSAEVENT hEventObject,
    long lNetworkEvents
    );

//
// Routine for handling async (nonblocking) connects.
//

DWORD
SockDoAsyncConnect (
    IN PSOCK_ASYNC_CONNECT_CONTEXT ConnectContext
    );

//
// Routine for cancelling I/O.
//

VOID
SockCancelIo(
    IN SOCKET Socket
    );

//
// Routines to manage reader-writer lock.
// Exclusive access to this lock is only used to perform one-time
// initialization tasks (e.g., startup, loading of helper DLLs,
// creation of async thread, etc).
// Shared access is also used sparingly to get access to global data.
//
NTSTATUS
SockInitializeRwLockAndSpinCount (
    PSOCK_RW_LOCK   Lock,
    ULONG           SpinCount
    );

NTSTATUS
SockDeleteRwLock (
    PSOCK_RW_LOCK   Lock
    );

VOID
SockAcquireRwLockShared(
    PSOCK_RW_LOCK   Lock
    );

VOID
SockReleaseRwLockShared(
    PSOCK_RW_LOCK   Lock
    );

VOID
SockAcquireRwLockExclusive(
    PSOCK_RW_LOCK   Lock
    );

VOID
SockReleaseRwLockExclusive(
    PSOCK_RW_LOCK   Lock
    );

#ifdef _AFD_SAN_SWITCH_
INT
SockSanSend(
    PSOCKET_INFORMATION socket,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    HANDLE ThreadHandle
    );

INT
SockSanRecv(
    PSOCKET_INFORMATION socket,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    LPDWORD lpFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    HANDLE ThreadHandle
    );

int
WSPAPI
WSPSanRecvFrom(
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
SockSanRecvDisconnect(
    PSOCKET_INFORMATION Socket
	);

INT
SockpGetProtocolInfoArray (VOID);

NTSTATUS
SockSanInitialize (
    VOID
    );

VOID
SockSanCleanup (
    VOID
    );

INT
SockSanGetTcpipCatalogId(
    );

VOID
SockSanWaitForSockDupToComplete(
    );

PSOCK_SAN_PROVIDER
SockSanFindProvider (
    IN LPSOCKADDR_IN   Address,
    OUT LPSOCKADDR_IN   IntfAddress OPTIONAL
    );

VOID
SockSanProviderAddressUpdate (
	VOID
    );

VOID
SockSanEnableActiveUpdates (
    VOID
    );

NTSTATUS
SockSanActivate (
    VOID
    );

VOID
SockSanProviderChange (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

VOID
SockSanScanListenSocksForProviderChange (
    VOID
    );

#ifndef DEBUG_TRACING

// PROVIDER is an empty macro when not doing DEBUG tracing
#define PROVIDER(_provider)	
#define SAN_CALL_SPI(_prov, _func, _args)   \
    (_prov)->SpiProcTable.lp##_func _args

#else

#define PROVIDER(_provider)	(_provider),
#define SAN_CALL_SPI(_prov, _func, _args) DTHOOK_##_func _args

#endif //DEBUG_TRACING

#define SAN_CALL_EPI(_prov, _func, _args)  \
    (_prov)->SanExtProcTable.lp##_func _args



VOID
SockSanAsyncFreeProvider (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

VOID
SockSanFreeProvider (
    PSOCK_SAN_PROVIDER  Provider
    );

#define SockSanDereferenceProvider(_prov)                   \
    if (InterlockedDecrement (&(_prov)->RefCount)==0) {     \
        SockSanFreeProvider (_prov);                       \
    }

BOOLEAN
SockSanConnect (
    IN PSOCKET_INFORMATION Socket,
    IN PAFD_SAN_CONNECT_JOIN_INFO ConnectInfo,
    OUT LPINT lpErrNo
    );

void
HandleSanConnectIndication (
	 PSOCK_SAN_INFORMATION switchSocket,
	 int errorCode
     );

PSOCK_SAN_INFORMATION
SockSanCreateSwitchSocket(
    VOID
    );

SOCKET
WSPAPI
SockSanCreateSocketHandle (
    IN DWORD dwCatalogEntryId,
    IN DWORD_PTR dwContext,
    OUT LPINT lpErrno
    );

int
WSPAPI
SockSanCloseSocketHandle(
    IN SOCKET s,
    OUT LPINT lpErrno
    );

int
WSPAPI
SockSanQuerySocketHandleContext(
    IN SOCKET s,
    OUT PDWORD_PTR lpContext,
    OUT LPINT lpErrno
    );

int
WSPAPI
SockSanCompleteOverlappedRequest (
    SOCKET s,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwError,
    DWORD cbTransferred,
    LPINT lpErrno
    );

VOID
SockSanCompletion (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

VOID
SockSanRedirectRequest (
    IN PVOID Key,
	IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

INT
SockSanCreateSocket (
    PSOCK_SAN_PROVIDER  Provider,
    PSOCKET_INFORMATION Socket,
    LPSOCKADDR_IN       Address,
	BOOLEAN				Always
    );

INT
SockSanDeleteSocket (
    PSOCKET_INFORMATION Socket
    );

INT
SockSanCloseSocket (
    PSOCKET_INFORMATION Socket
    );

VOID
SockSanDeleteHalfAcceptedSocket(
	PSOCK_SAN_INFORMATION SwitchSocket
	);

#define SockSanReferenceAcceptSock(_swSock) 					\
    InterlockedIncrement (&(_swSock)->HalfAcceptRefCount)

#define SockSanDereferenceAcceptSock(_swSock)                   \
    if (InterlockedDecrement (&(_swSock)->HalfAcceptRefCount)==0) {     \
        SockSanDeleteHalfAcceptedSocket (_swSock);                       \
    }
int
WSPAPI
SockSanListen(
    IN PSOCKET_INFORMATION Socket,
    int Backlog
    );

void
SockSanCleanupListeningContext(
    PSOCK_SAN_INFORMATION SwitchSocket
	);

int
SockSanShutdown(
    IN PSOCKET_INFORMATION Socket,
    IN int HowTo
    );

BOOL
PASCAL FAR
SanTransmitFile (
    SOCKET hSocket,                             
	HANDLE hFile,                               
	DWORD nNumberOfBytesToWrite,                
	DWORD nNumberOfBytesPerSend,                
	LPOVERLAPPED lpOverlapped,                  
	LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,  
	DWORD dwFlags
	);

NTSTATUS
AfdRegisterSwitch (
    IN HANDLE   CompletionPort,
    IN HANDLE   CompletionEvent,
    OUT PHANDLE Handle
    );

NTSTATUS
AfdCementSocketToSan (
    IN HANDLE   Handle,
    IN HANDLE   SocketHandle,
    IN PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdReadAvailable (
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdReadMore (
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdNothingToRead (
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdReadAvailableOob (
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdReadMoreOob (
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdNothingToReadOob (
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdWriteWouldBlock (
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdCanWrite (
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdConnectFail (
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  NTSTATUS            Status
    );

NTSTATUS
AfdRemoteDisconnected(
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdRemoteAborted(
    IN  HANDLE              Handle,
    IN  HANDLE              SocketHandle,
    IN  PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdConnectIndication (
    IN HANDLE                   Handle,
	IN HANDLE					Event,
    IN PAFD_SWITCH_CONNECT_INFO ConnectInfo,
    IN ULONG                    ConnectInfoLength,
    IN PVOID                    Context,
    OUT PAFD_SWITCH_ACCEPT_INFO AcceptInfo,
    OUT PIO_STATUS_BLOCK        IoStatus
    );

NTSTATUS
AfdCompleteAccept (
    IN HANDLE                   Handle,
    IN HANDLE                   SocketHandle,
    IN PAFD_SWITCH_CONTEXT      SwitchContext,
    IN PVOID                    ReceiveBuffer,
    IN ULONG                    ReceiveLength
    );

NTSTATUS
AfdCompleteRequest (
    IN HANDLE           SocketHandle,
    IN LPOVERLAPPED     lpOverlapped,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

NTSTATUS
AfdDeregisterSwitch (
    IN HANDLE       Handle
    );

NTSTATUS
AfdCompleteRedirectedRequest (
    IN HANDLE                   Handle,
    IN HANDLE                   SocketHandle,
    IN PAFD_SWITCH_CONTEXT      SwitchContext,
    IN PVOID                    RequestContext,
    IN NTSTATUS                 RequestStatus,
    IN ULONG                    DataOffset,
    IN PVOID                    Buffer,
    IN ULONG                    BufferLength,
	OUT PULONG					RetBufferLength
    );

NTSTATUS
AfdReuseSocket (
    IN HANDLE       Handle,
    IN HANDLE       Socket,
    IN PAFD_SWITCH_CONTEXT SwitchContext
    );

NTSTATUS
AfdGetPhysicalAddr (
    IN HANDLE       	Handle,
	IN PVOID			Va,
	ULONG				AccessMode,
    IN PPHYSICAL_ADDRESS Pa
    );

NTSTATUS
AfdAcquireContext (
    IN HANDLE       Handle,
    IN HANDLE       Socket,
    IN PAFD_SWITCH_CONTEXT SwitchContext,
    IN PVOID        SocketCtx,
    IN ULONG        SocketCtxSize,
    IN PVOID        RcvBuffer,
    IN ULONG        RcvBufferSize,
    IN HANDLE       Event,
    IN PVOID        Context,
    OUT PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
AfdTransferContext (
    IN HANDLE       Handle,
    IN HANDLE       Socket,
	IN PAFD_SWITCH_CONTEXT SwitchContext,
	IN PVOID       	RequestContext,
    IN PVOID        SocketCtx,
    IN ULONG        SocketCtxSize,
    IN LPWSABUF     RcvBufferArray,
    IN ULONG        RcvBufferCount,
    IN NTSTATUS     Status
    );

NTSTATUS
AfdGetServicePid (
    IN HANDLE       Handle,
    OUT ULONG       *ProcessId
    );

NTSTATUS
AfdSetServiceProcess (
    IN HANDLE       Handle
    );

NTSTATUS
AfdProviderChange (
    IN HANDLE       Handle
    );

NTSTATUS
AfdSwitchAddrListChange (
    IN HANDLE       Handle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID           Context
    );

//
// Internal SAN routines
//

int
InitFlowAndConnectThreads (
    VOID
    );

int
InitializeFlowControl(
    PSOCK_SAN_INFORMATION SwitchSocket,
    BOOL IsServer
    );


void
SockSanCleanupFlowControl(
    PSOCK_SAN_INFORMATION SwitchSocket
    );

VOID
SockSanDeallocateFCMemory(
    PSOCK_SAN_INFORMATION SwitchSocket
    );

void
CompleteAppRecvsUponClose(
	PSOCK_SAN_INFORMATION SwitchSocket,
	BOOL Graceful,
	int Error
	);

int
ContinueGracefulDisconnect(
	PSOCK_SAN_INFORMATION SwitchSocket
	);

int
AbortSanConnection(
    PSOCK_SAN_INFORMATION SwitchSocket
    );

void
NotifyAfdOfApplicationReceive(
    PSOCK_SAN_INFORMATION SwitchSocket,
    BOOL         UserOob		// MSG_OOB flag with WSPRecv
);

void
NotifyAfdOfReceiveComplete(
    PSOCK_SAN_INFORMATION SwitchSocket,
    BOOL SignalReadEvent,
	BOOL SignalOobEvent
);

void
NotifyAfdOfSendNotAvail(
    PSOCK_SAN_INFORMATION SwitchSocket
);

void
NotifyAfdOfSendAvail(
    PSOCK_SAN_INFORMATION SwitchSocket
);

void
NotifyAfdOfClose(
    PSOCK_SAN_INFORMATION SwitchSocket,
	BOOL Graceful
	);

//
// Socket duplication functions
//
VOID
SockSanDuplicateSock (
	IN PSOCK_SAN_INFORMATION SwitchSocket,
    IN PVOID RequestContext,
    IN DWORD ProcessId
    );

VOID
HandleSuspendCommunicationRequest (
    IN PSOCK_SAN_INFORMATION SwitchSocket
    );

VOID
ResumeSockDuplication (
    IN PSOCK_SAN_INFORMATION SwitchSocket
    );

INT
SockSanImportSocket (
    IN PSOCKET_INFORMATION Socket
    );

VOID
SockSanDupSockCtrlMsg (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

VOID
SockSanMigrateSocketsToService(
    );

NTSTATUS
HandleSanAcceptIndication (
	 PSOCK_SAN_INFORMATION switchSocket
     );

BOOL
SockSanFlowInitialize(
    void
    );

void
SockSanFlowTerminate(
    void
    );

void
SockSanConnectTerminate(
    void
    );

void
SockSanRemoveFromWaitList (
	 PSOCK_SAN_INFORMATION switchSocket,
	 LONG waitId
     );

INT
SockSanInsertInWaitList (
    IN PSOCK_SAN_INFORMATION   SwitchSocket,
    OUT LONG   *waitId
    );

VOID
SockSanIncludeWait (
    LONG waitId
    );

void 
AcceptExRecvCompletionRoutine ( 
	DWORD dwError, 
	DWORD cbTransferred,  
	LPWSAOVERLAPPED lpOverlapped, 
	DWORD dwFlags 
	);

int
AcceptRejectConditionCallback(
    IN LPWSABUF lpCallerId,
    IN LPWSABUF lpCallerData,
    IN OUT LPQOS lpSQOS,
    IN OUT LPQOS lpGQOS,
    IN LPWSABUF lpCalleeId,
    IN LPWSABUF lpCalleeData,
    OUT GROUP FAR * g,
    IN DWORD_PTR dwCallbackData
    );
int
AcceptDeferConditionCallback(
    IN LPWSABUF lpCallerId,
    IN LPWSABUF lpCallerData,
    IN OUT LPQOS lpSQOS,
    IN OUT LPQOS lpGQOS,
    IN LPWSABUF lpCalleeId,
    IN LPWSABUF lpCalleeData,
    OUT GROUP FAR * g,
    IN DWORD_PTR dwCallbackData
    );

VOID
CleanupTFSock(
	PSOCKET_INFORMATION Socket,
	PSOCK_SAN_INFORMATION AcceptSwitchSocket
	);

VOID
ContinueAcceptProcessing(
	PSOCK_SAN_INFORMATION SwitchSocket
	);

int
StartGracefulDisconnect(
	PSOCK_SAN_INFORMATION SwitchSocket
	);

int
SendFIN(
	PSOCK_SAN_INFORMATION SwitchSocket
	);

VOID
DontSendAnyMore(
    IN PSOCK_SAN_INFORMATION SwitchSocket
    );

int
CloseSocketExt1(
    PSOCK_SAN_INFORMATION SwitchSocket
    );


#if DBG
BOOL
VerifySwitchState(
    PSOCK_SAN_INFORMATION SwitchSocket
    );
#endif

//
//  Utility functions
//

int
CopyToUserBuffer(
    LPWSABUF WsaBuffer,
    DWORD BufferCount,
	DWORD StartOffset,
    PVOID Data,
    DWORD ByteCount
    );

int
CopyFromUserBuffer(
    PVOID Data,
    LPDWORD ByteCount,
    LPWSABUF WsaBuffer,
    DWORD BufferCount,
    DWORD StartOffset
    );

NTSTATUS
SocketErrorToNtStatus (
    IN int Error
    );

INT
SockSanGetSocketBack (
	PSOCK_SAN_INFORMATION SwitchSocket
    );

//
// Handle same-process duplication.
// Also, if socket has been given away to some other process, then
// get it back. BUT, if this is WSPCloseSocket (State==SocketStateClosing)
// then no need to get socket back; graceful disconnect (if needed) will
// be done by whichever process has the socket
//
#define SockSanCheckSocketUponEntry(_swSock, _sock)							\
    if ((_swSock)->Socket != (_sock)) {										\
		PSOCKET_INFORMATION tmpSocket = (_swSock)->Socket;					\
		SockReferenceSocket(_sock);											\
		(_swSock)->Socket = _sock;											\
		SockDereferenceSocket(tmpSocket);									\
	}                                                                       \
	if ((_swSock)->SockDupState && (_sock)->State!=SocketStateClosing) {    \
        SockSanGetSocketBack (_swSock);                                     \
    }                                                                       \

#endif //_AFD_SAN_SWITCH_


#endif // ndef _SOCKPROC_

