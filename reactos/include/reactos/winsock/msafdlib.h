/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 NSP
 * FILE:        include/reactos/winsock/msafdlib.h
 * PURPOSE:     Winsock 2 SPI Utility Header
 */

#define NO_BLOCKING_HOOK        0
#define MAYBE_BLOCKING_HOOK     1
#define ALWAYS_BLOCKING_HOOK    2

#define NO_TIMEOUT              0
#define SEND_TIMEOUT            1
#define RECV_TIMEOUT            2

#define MAX_TDI_ADDRESS_LENGTH 32

#define WSA_FLAG_MULTIPOINT_ALL (WSA_FLAG_MULTIPOINT_C_ROOT |\
                                 WSA_FLAG_MULTIPOINT_C_LEAF |\
                                 WSA_FLAG_MULTIPOINT_D_ROOT |\
                                 WSA_FLAG_MULTIPOINT_D_LEAF)


/* Socket State */
typedef enum _SOCKET_STATE
{
    SocketUndefined = -1,
    SocketOpen,
    SocketBound,
    SocketBoundUdp,
    SocketConnected,
    SocketClosed
} SOCKET_STATE, *PSOCKET_STATE;

/* 
 * Shared Socket Information.
 * It's called shared because we send it to Kernel-Mode for safekeeping
 */
typedef struct _SOCK_SHARED_INFO {
    SOCKET_STATE                State;
    INT                            AddressFamily;
    INT                            SocketType;
    INT                            Protocol;
    INT                            SizeOfLocalAddress;
    INT                            SizeOfRemoteAddress;
    struct linger                LingerData;
    ULONG                        SendTimeout;
    ULONG                        RecvTimeout;
    ULONG                        SizeOfRecvBuffer;
    ULONG                        SizeOfSendBuffer;
    struct {
        BOOLEAN                    Listening:1;
        BOOLEAN                    Broadcast:1;
        BOOLEAN                    Debug:1;
        BOOLEAN                    OobInline:1;
        BOOLEAN                    ReuseAddresses:1;
        BOOLEAN                    ExclusiveAddressUse:1;
        BOOLEAN                    NonBlocking:1;
        BOOLEAN                    DontUseWildcard:1;
        BOOLEAN                    ReceiveShutdown:1;
        BOOLEAN                    SendShutdown:1;
        BOOLEAN                    UseDelayedAcceptance:1;
        BOOLEAN                    UseSAN:1;
    }; // Flags
    DWORD                        CreateFlags;
    DWORD                        CatalogEntryId;
    DWORD                        ServiceFlags1;
    DWORD                        ProviderFlags;
    GROUP                        GroupID;
    DWORD                        GroupType;
    INT                            GroupPriority;
    INT                            SocketLastError;
    HWND                        hWnd;
    LONG                        Unknown;
    DWORD                        SequenceNumber;
    UINT                        wMsg;
    LONG                        AsyncEvents;
    LONG                        AsyncDisabledEvents;
} SOCK_SHARED_INFO, *PSOCK_SHARED_INFO;

/* Socket Helper Data. Holds information about the WSH Libraries */
typedef struct _HELPER_DATA {
    LIST_ENTRY                        Helpers;
    LONG                            RefCount;
    HANDLE                            hInstance;
    INT                                MinWSAddressLength;
    INT                                MaxWSAddressLength;
    INT                                MinTDIAddressLength;
    INT                                MaxTDIAddressLength;
    BOOLEAN                            UseDelayedAcceptance;
    PWINSOCK_MAPPING                Mapping;
    PWSH_OPEN_SOCKET                WSHOpenSocket;
    PWSH_OPEN_SOCKET2                WSHOpenSocket2;
    PWSH_JOIN_LEAF                    WSHJoinLeaf;
    PWSH_NOTIFY                        WSHNotify;
    PWSH_GET_SOCKET_INFORMATION        WSHGetSocketInformation;
    PWSH_SET_SOCKET_INFORMATION        WSHSetSocketInformation;
    PWSH_GET_SOCKADDR_TYPE            WSHGetSockaddrType;
    PWSH_GET_WILDCARD_SOCKADDR        WSHGetWildcardSockaddr;
    PWSH_GET_BROADCAST_SOCKADDR        WSHGetBroadcastSockaddr;
    PWSH_ADDRESS_TO_STRING            WSHAddressToString;
    PWSH_STRING_TO_ADDRESS            WSHStringToAddress;
    PWSH_IOCTL                        WSHIoctl;
    WCHAR                            TransportName[1];
} HELPER_DATA, *PHELPER_DATA;

typedef struct _ASYNC_DATA
{
    struct _SOCKET_INFORMATION *ParentSocket;
    DWORD SequenceNumber;
    IO_STATUS_BLOCK IoStatusBlock;
    AFD_POLL_INFO AsyncSelectInfo;
} ASYNC_DATA, *PASYNC_DATA;

/* The actual Socket Structure represented by a handle. Internal to us */
typedef struct _SOCKET_INFORMATION {
    union {
        WSH_HANDLE WshContext;
        struct {
            LONG RefCount;
            SOCKET Handle;
        };
    };
    SOCK_SHARED_INFO SharedData;
    GUID ProviderId;
    DWORD HelperEvents;
    PHELPER_DATA HelperData;
    PVOID HelperContext;
    PSOCKADDR LocalAddress;
    PSOCKADDR RemoteAddress;
    HANDLE TdiAddressHandle;
    HANDLE TdiConnectionHandle;
    PASYNC_DATA AsyncData;
    HANDLE EventObject;
    LONG NetworkEvents;
    CRITICAL_SECTION Lock;
    BOOL DontUseSan;
    PVOID SanData;
} SOCKET_INFORMATION, *PSOCKET_INFORMATION;

/* The blob of data we send to Kernel-Mode for safekeeping */
typedef struct _SOCKET_CONTEXT {
    SOCK_SHARED_INFO SharedData;
    ULONG SizeOfHelperData;
    ULONG Padding;
    SOCKADDR LocalAddress;
    SOCKADDR RemoteAddress;
    /* Plus Helper Data */
} SOCKET_CONTEXT, *PSOCKET_CONTEXT;

typedef struct _SOCK_RW_LOCK
{
    volatile LONG ReaderCount;
    HANDLE WriterWaitEvent;
    RTL_CRITICAL_SECTION Lock;
} SOCK_RW_LOCK, *PSOCK_RW_LOCK;

typedef struct _WINSOCK_TEB_DATA
{
    HANDLE EventHandle;
    SOCKET SocketHandle;
    PAFD_ACCEPT_DATA AcceptData;
    LONG PendingAPCs;
    BOOLEAN CancelIo;
    ULONG Unknown;
    PVOID RnrThreadData;
} WINSOCK_TEB_DATA, *PWINSOCK_TEB_DATA;

typedef INT
(WINAPI *PICF_CONNECT)(PVOID IcfData);

typedef struct _SOCK_ICF_DATA
{
    HANDLE IcfHandle;
    PVOID IcfOpenDynamicFwPort;
    PICF_CONNECT IcfConnect;
    PVOID IcfDisconnect;
    HINSTANCE DllHandle;
} SOCK_ICF_DATA, *PSOCK_ICF_DATA;

typedef PVOID
(NTAPI *PRTL_HEAP_ALLOCATE)(
    IN HANDLE Heap,
    IN ULONG Flags,
    IN ULONG Size
);

extern HANDLE SockPrivateHeap;
extern PRTL_HEAP_ALLOCATE SockAllocateHeapRoutine;
extern SOCK_RW_LOCK SocketGlobalLock;
extern PWAH_HANDLE_TABLE SockContextTable;
extern LPWSPUPCALLTABLE SockUpcallTable;
extern BOOL SockProcessTerminating;
extern LONG SockWspStartupCount;
extern DWORD SockSendBufferWindow;
extern DWORD SockReceiveBufferWindow;
extern HANDLE SockAsyncQueuePort;
extern BOOLEAN SockAsyncSelectCalled;
extern LONG SockProcessPendingAPCCount;
extern HINSTANCE SockModuleHandle;
extern LONG gWSM_NSPStartupRef;
extern LONG gWSM_NSPCallRef;
extern LIST_ENTRY SockHelperDllListHead;
extern CRITICAL_SECTION MSWSOCK_SocketLock;
extern HINSTANCE NlsMsgSourcemModuleHandle;
extern PVOID SockBufferKeyTable;
extern ULONG SockBufferKeyTableSize;
extern LONG SockAsyncThreadReferenceCount;
extern BOOLEAN g_fRnrLockInit;
extern CRITICAL_SECTION g_RnrLock;

BOOL
WSPAPI
MSWSOCK_Initialize(VOID);

BOOL
WSPAPI
MSAFD_SockThreadInitialize(VOID);

INT
WSPAPI
SockCreateAsyncQueuePort(VOID);

PVOID
WSPAPI
SockInitializeHeap(IN HANDLE Heap,
                   IN ULONG Flags,
                   IN ULONG Size);

NTSTATUS
WSPAPI
SockInitializeRwLockAndSpinCount(
    IN PSOCK_RW_LOCK Lock,
    IN ULONG SpinCount
);

VOID
WSPAPI
SockAcquireRwLockExclusive(IN PSOCK_RW_LOCK Lock);

VOID
WSPAPI
SockAcquireRwLockShared(IN PSOCK_RW_LOCK Lock);

VOID
WSPAPI
SockReleaseRwLockExclusive(IN PSOCK_RW_LOCK Lock);

VOID
WSPAPI
SockReleaseRwLockShared(IN PSOCK_RW_LOCK Lock);

NTSTATUS
WSPAPI
SockDeleteRwLock(IN PSOCK_RW_LOCK Lock);

INT
WSPAPI
SockGetConnectData(IN PSOCKET_INFORMATION Socket,
                   IN ULONG Ioctl,
                   IN PVOID Buffer,
                   IN ULONG BufferLength,
                   OUT PULONG BufferReturned);

INT
WSPAPI
SockIsAddressConsistentWithConstrainedGroup(IN PSOCKET_INFORMATION Socket,
                                            IN GROUP Group,
                                            IN PSOCKADDR SocketAddress,
                                            IN INT SocketAddressLength);

BOOL
WSPAPI
SockWaitForSingleObject(IN HANDLE Handle,
                        IN SOCKET SocketHandle,
                        IN DWORD BlockingFlags,
                        IN DWORD TimeoutFlags);

BOOLEAN
WSPAPI
SockIsSocketConnected(IN PSOCKET_INFORMATION Socket);

INT
WSPAPI
SockNotifyHelperDll(IN PSOCKET_INFORMATION Socket,
                    IN DWORD Event);

INT
WSPAPI
SockUpdateWindowSizes(IN PSOCKET_INFORMATION Socket,
                      IN BOOLEAN Force);

INT
WSPAPI
SockBuildTdiAddress(OUT PTRANSPORT_ADDRESS TdiAddress,
                    IN PSOCKADDR Sockaddr,
                    IN INT SockaddrLength);

INT
WSPAPI
SockBuildSockaddr(OUT PSOCKADDR Sockaddr,
                  OUT PINT SockaddrLength,
                  IN PTRANSPORT_ADDRESS TdiAddress);

INT
WSPAPI
SockGetTdiHandles(IN PSOCKET_INFORMATION Socket);

VOID
WSPAPI
SockIoCompletion(IN PVOID ApcContext,
                 IN PIO_STATUS_BLOCK IoStatusBlock,
                 DWORD Reserved);

VOID
WSPAPI
SockCancelIo(IN SOCKET Handle);

INT
WSPAPI
SockGetInformation(IN PSOCKET_INFORMATION Socket, 
                   IN ULONG AfdInformationClass, 
                   IN PVOID ExtraData OPTIONAL,
                   IN ULONG ExtraDataSize,
                   IN OUT PBOOLEAN Boolean OPTIONAL,
                   IN OUT PULONG Ulong OPTIONAL, 
                   IN OUT PLARGE_INTEGER LargeInteger OPTIONAL);

INT
WSPAPI
SockSetInformation(IN PSOCKET_INFORMATION Socket, 
                   IN ULONG AfdInformationClass, 
                   IN PBOOLEAN Boolean OPTIONAL,
                   IN PULONG Ulong OPTIONAL, 
                   IN PLARGE_INTEGER LargeInteger OPTIONAL);

INT
WSPAPI
SockSetHandleContext(IN PSOCKET_INFORMATION Socket);

VOID
WSPAPI
SockDereferenceSocket(IN PSOCKET_INFORMATION Socket);

VOID
WSPAPI
SockFreeHelperDll(IN PHELPER_DATA Helper);

PSOCKET_INFORMATION
WSPAPI
SockFindAndReferenceSocket(IN SOCKET Handle,
                           IN BOOLEAN Import);

INT
WSPAPI
SockEnterApiSlow(OUT PWINSOCK_TEB_DATA *ThreadData);

VOID
WSPAPI
SockSanInitialize(VOID);

VOID
WSPAPI
SockSanGetTcpipCatalogId(VOID);

VOID
WSPAPI
CloseIcfConnection(IN PSOCK_ICF_DATA IcfData);

VOID
WSPAPI
InitializeIcfConnection(IN PSOCK_ICF_DATA IcfData);

VOID
WSPAPI
NewIcfConnection(IN PSOCK_ICF_DATA IcfData);

INT
WSPAPI
NtStatusToSocketError(IN NTSTATUS Status);

INT
WSPAPI
SockSocket(INT AddressFamily, 
           INT SocketType, 
           INT Protocol, 
           LPGUID ProviderId, 
           GROUP g,
           DWORD dwFlags,
           DWORD ProviderFlags,
           DWORD ServiceFlags,
           DWORD CatalogEntryId,
           PSOCKET_INFORMATION *NewSocket);

INT
WSPAPI
SockCloseSocket(IN PSOCKET_INFORMATION Socket);

FORCEINLINE
INT
WSPAPI
SockEnterApiFast(OUT PWINSOCK_TEB_DATA *ThreadData)
{
    /* Make sure we aren't terminating and get our thread data */
    if (!(SockProcessTerminating) &&
        (SockWspStartupCount > 0) &&
        ((*ThreadData = NtCurrentTeb()->WinSockData)))
    {
        /* Everything is good, return */
        return NO_ERROR;
    }

    /* Something didn't work out, use the slow path */
    return SockEnterApiSlow(ThreadData);
}

FORCEINLINE
VOID
WSPAPI
SockDereferenceHelperDll(IN PHELPER_DATA Helper)
{
    /* Dereference and see if it's the last count */
    if (!InterlockedDecrement(&Helper->RefCount))
    {
        /* Destroy the Helper DLL */
        SockFreeHelperDll(Helper);
    }
}

#define MSAFD_IS_DGRAM_SOCK(s) \
    (s->SharedData.ServiceFlags1 & XP1_CONNECTIONLESS)

/* Global data that we want to share access with */
extern HANDLE SockSanCleanUpCompleteEvent;
extern BOOLEAN SockSanEnabled;
extern WSAPROTOCOL_INFOW SockTcpProviderInfo;

typedef VOID
(WSPAPI *PASYNC_COMPLETION_ROUTINE)(
    PVOID Context,
    PIO_STATUS_BLOCK IoStatusBlock
);

/* Internal Helper Functions */
INT
WSPAPI
SockLoadHelperDll(
    PWSTR TransportName, 
    PWINSOCK_MAPPING Mapping, 
    PHELPER_DATA *HelperDllData
);

INT
WSPAPI
SockLoadTransportMapping(
    PWSTR TransportName, 
    PWINSOCK_MAPPING *Mapping
);

INT
WSPAPI
SockLoadTransportList(
    PWSTR *TransportList
);

BOOL
WSPAPI
SockIsTripleInMapping(IN PWINSOCK_MAPPING Mapping, 
                      IN INT AddressFamily,
                      OUT PBOOLEAN AfMatch,
                      IN INT SocketType, 
                      OUT PBOOLEAN SockMatch,
                      IN INT Protocol,
                      OUT PBOOLEAN ProtoMatch);

INT
WSPAPI
SockAsyncSelectHelper(IN PSOCKET_INFORMATION Socket,
                      IN HWND hWnd,
                      IN UINT wMsg,
                      IN LONG Events);

INT
WSPAPI
SockEventSelectHelper(IN PSOCKET_INFORMATION Socket,
                      IN WSAEVENT EventObject,
                      IN LONG Events);

BOOLEAN 
WSPAPI
SockCheckAndReferenceAsyncThread(VOID);

BOOLEAN 
WSPAPI
SockCheckAndInitAsyncSelectHelper(VOID);

INT 
WSPAPI
SockGetTdiName(PINT AddressFamily, 
               PINT SocketType, 
               PINT Protocol,
               LPGUID ProviderId,
               GROUP Group, 
               DWORD Flags, 
               PUNICODE_STRING TransportName, 
               PVOID *HelperDllContext, 
               PHELPER_DATA *HelperDllData, 
               PDWORD Events);

INT
WSPAPI
SockAsyncThread(
    PVOID ThreadParam
);

VOID
WSPAPI
SockProcessAsyncSelect(PSOCKET_INFORMATION Socket,
                       PASYNC_DATA AsyncData);

VOID
WSPAPI
SockHandleAsyncIndication(IN PASYNC_COMPLETION_ROUTINE Callback,
                          IN PVOID Context,
                          IN PIO_STATUS_BLOCK IoStatusBlock);

INT
WSPAPI
SockReenableAsyncSelectEvent(IN PSOCKET_INFORMATION Socket,
                             IN ULONG Event);

VOID
WSPAPI
SockProcessQueuedAsyncSelect(PVOID Context,
                             PIO_STATUS_BLOCK IoStatusBlock);

VOID
WSPAPI
SockAsyncSelectCompletion(
    PVOID Context,
    PIO_STATUS_BLOCK IoStatusBlock
);

/* Public functions, but not exported! */
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
    IN    SOCKET s,
    OUT    LPINT lpErrno);

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
    OUT        CHAR FAR* optval,
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
    IN OUT  LPFD_SET readfds,
    IN OUT  LPFD_SET writefds,
    IN OUT  LPFD_SET exceptfds,
    IN      CONST LPTIMEVAL timeout,
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
