/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    type.h

Abstract:

    This module contains global type definitions for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#ifndef _TYPE_H_
#define _TYPE_H_


//
// An IO status block, used mainly for overlapped IO.
//

typedef struct _SOCK_IO_STATUS {

    DWORD Status;
    DWORD Information;

} SOCK_IO_STATUS, *PSOCK_IO_STATUS;


//
// Per-thread data.
//

typedef struct _SOCK_TLS_DATA {

    //
    // Reentrancy flag. Keeps us from getting reentered on the same thread.
    //

    BOOL ReentrancyFlag;

    //
    // Set if the current IO request has been cancelled.
    //

    BOOL IoCancelled;

    //
    // Blocking hook support.
    //

    BOOL BlockingHookInstalled;
    BOOL IsBlocking;
    LPBLOCKINGCALLBACK BlockingCallback;
    ULONG_PTR BlockingContext;
    struct _SOCKET_INFORMATION * BlockingSocketInfo;

} SOCK_TLS_DATA, *PSOCK_TLS_DATA;


//
// Overlapped IO managment data. Each socket has two of these structures:
// one for overlapped receives, and one for overlapped sends.
//

typedef struct _SOCK_OVERLAPPED_DATA {

    HANDLE WakeupEvent;
    LIST_ENTRY QueueListHead;

} SOCK_OVERLAPPED_DATA, *PSOCK_OVERLAPPED_DATA;


//
// Socket state identifiers.
//

typedef enum _SOCK_STATE {

    SocketStateOpen,
    SocketStateClosing

} SOCK_STATE, *PSOCK_STATE;


//
// Group types.
//

typedef enum _SOCK_GROUP_TYPE {

    GroupTypeNeither = 0,
    GroupTypeUnconstrained = SG_UNCONSTRAINED_GROUP,
    GroupTypeConstrained = SG_CONSTRAINED_GROUP

} SOCK_GROUP_TYPE, *PSOCK_GROUP_TYPE;


//
// Per-socket data.
//

typedef struct _SOCKET_INFORMATION {

    //
    // Linked-list linkage.
    //

    LIST_ENTRY SocketListEntry;

    //
    // Reference count.
    //

    LONG ReferenceCount;

    //
    // The 2.x handle.
    //

    SOCKET WS2Handle;

    //
    // The 1.1 handle.
    //

    SOCKET WS1Handle;

    //
    // Socket state.
    //

    SOCK_STATE State;

    //
    // Socket Attributes.
    //

    INT AddressFamily;
    INT SocketType;
    INT Protocol;

    //
    // Lock protecting this socket.
    //

    CRITICAL_SECTION SocketLock;

    //
    // Shutdown event. This event is signalled whenever closesocket()
    // is called on this socket so that any worker threads can cleanup
    // and exit cleanly.
    //

    HANDLE ShutdownEvent;

    //
    // Overlapped IO data.
    //

    SOCK_OVERLAPPED_DATA OverlappedRecv;
    SOCK_OVERLAPPED_DATA OverlappedSend;

    //
    // The hooker that owns this socket.
    //

    struct _HOOKER_INFORMATION * Hooker;

    //
    // Snapshot of several parameters passed into WSPSocket() when
    // creating this socket. We need these when creating accept()ed
    // sockets.
    //

    INT MaxSockaddrLength;
    INT MinSockaddrLength;
    DWORD CreationFlags;
    DWORD CatalogEntryId;
    DWORD ServiceFlags1;
    DWORD ServiceFlags2;
    DWORD ServiceFlags3;
    DWORD ServiceFlags4;
    DWORD ProviderFlags;

    //
    // WSA{Async|Event}Select stuff.
    //

    BOOL AsyncSelectIssuedToHooker;

    LONG SelectEventsEnabled;
    LONG SelectEventsActive;

    HWND AsyncSelectWindow;
    UINT AsyncSelectMessage;

    HANDLE EventSelectEvent;
    INT EventSelectStatus[FD_MAX_EVENTS];

    //
    // Socket grouping.
    //

    GROUP GroupID;
    SOCK_GROUP_TYPE GroupType;
    INT GroupPriority;

    //
    // Deferred accept() stuff.
    //

    SOCKET DeferSocket;
    INT DeferAddressLength;
//    CHAR DeferAddress[];

} SOCKET_INFORMATION, *PSOCKET_INFORMATION;


//
// Per-hooker data.
//

typedef struct _HOOKER_INFORMATION {

    //
    // Linked-list linkage.
    //

    LIST_ENTRY HookerListEntry;

    //
    // Reference count.
    //

    LONG ReferenceCount;

    //
    // This boolean gets set to TRUE only after the hooker is fully
    // initialized (meaning that the target DLL's WSAStartup() has
    // been called). This is to prevent a nasty recursion when DLLs
    // such as RWS insist on creating sockets from *within* their
    // WSAStartup() routines...
    //

    BOOL Initialized;

    //
    // Handle to the hooker's DLL.
    //

    HINSTANCE DllHandle;

    //
    // The provider GUID generated for this hooker.
    //

    GUID ProviderId;

    //
    // Pointers to the DLL entrypoints.
    //

    LPFN_ACCEPT accept;
    LPFN_BIND bind;
    LPFN_CLOSESOCKET closesocket;
    LPFN_CONNECT connect;
    LPFN_IOCTLSOCKET ioctlsocket;
    LPFN_GETPEERNAME getpeername;
    LPFN_GETSOCKNAME getsockname;
    LPFN_GETSOCKOPT getsockopt;
    LPFN_LISTEN listen;
    LPFN_RECV recv;
    LPFN_RECVFROM recvfrom;
    LPFN_SELECT select;
    LPFN_SEND send;
    LPFN_SENDTO sendto;
    LPFN_SETSOCKOPT setsockopt;
    LPFN_SHUTDOWN shutdown;
    LPFN_SOCKET socket;
    LPFN_WSASTARTUP WSAStartup;
    LPFN_WSACLEANUP WSACleanup;
    LPFN_WSAGETLASTERROR WSAGetLastError;
    LPFN_WSAISBLOCKING WSAIsBlocking;
    LPFN_WSAUNHOOKBLOCKINGHOOK WSAUnhookBlockingHook;
    LPFN_WSASETBLOCKINGHOOK WSASetBlockingHook;
    LPFN_WSACANCELBLOCKINGCALL WSACancelBlockingCall;
    LPFN_WSAASYNCSELECT WSAAsyncSelect;

    LPFN_GETHOSTNAME gethostname;
    LPFN_GETHOSTBYNAME gethostbyname;
    LPFN_GETHOSTBYADDR gethostbyaddr;
    LPFN_GETSERVBYPORT getservbyport;
    LPFN_GETSERVBYNAME getservbyname;

    //
    // An "anchor" socket. This socket is created & bound immediately
    // after loading the hooker to ensure the hooker's internal state
    // is fully initialized.
    //

    SOCKET AnchorSocket;

    //
    // The maximum datagram size, retrieved from the WSADATA structure
    // returned from WSAStartup().
    //

    INT MaxUdpDatagramSize;

} HOOKER_INFORMATION, *PHOOKER_INFORMATION;


#endif // _TYPE_H_

