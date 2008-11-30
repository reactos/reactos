/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/dllmain.c
 * PURPOSE:     DLL entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */

#include <w32api.h>
#include <ws2_32.h>
#include <catalog.h>
#include <handle.h>
#include <upcall.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
//DWORD DebugTraceLevel = MIN_TRACE;
//DWORD DebugTraceLevel = MAX_TRACE;
//DWORD DebugTraceLevel = DEBUG_ULTRA;
DWORD DebugTraceLevel = 0;
#endif /* DBG */

/* To make the linker happy */
VOID WINAPI KeBugCheck (ULONG BugCheckCode) {}

HINSTANCE g_hInstDll;
HANDLE GlobalHeap;
BOOL WsaInitialized = FALSE;    /* TRUE if WSAStartup() has been successfully called */
WSPUPCALLTABLE UpcallTable;


/*
 * @implemented
 */
INT
EXPORT
WSAGetLastError(VOID)
{
    PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

    if (p)
    {
        return p->LastErrorValue;
    }
    else
    {
        /* FIXME: What error code should we use here? Can this even happen? */
        return ERROR_BAD_ENVIRONMENT;
    }
}


/*
 * @implemented
 */
VOID
EXPORT
WSASetLastError(IN INT iError)
{
    PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

    if (p)
        p->LastErrorValue = iError;
}


/*
 * @implemented
 */
INT
EXPORT
WSAStartup(IN  WORD wVersionRequested,
           OUT LPWSADATA lpWSAData)
{
    BYTE Low, High;

    WS_DbgPrint(MAX_TRACE, ("WSAStartup of ws2_32.dll\n"));

    if (!g_hInstDll)
        return WSASYSNOTREADY;

    if (lpWSAData == NULL)
        return WSAEFAULT;

    Low = LOBYTE(wVersionRequested);
    High  = HIBYTE(wVersionRequested);

    if (Low < 1)
    {
        WS_DbgPrint(MAX_TRACE, ("Bad winsock version requested, %d,%d", Low, High));
        return WSAVERNOTSUPPORTED;
    }

    if (Low == 1)
    {
        if (High == 0)
        {
            lpWSAData->wVersion = wVersionRequested;
        }
        else
        {
            lpWSAData->wVersion = MAKEWORD(1, 1);
        }
    }
    else if (Low == 2)
    {
        if (High <= 2)
        {
            lpWSAData->wVersion = MAKEWORD(2, High);
        }
        else
        {
            lpWSAData->wVersion = MAKEWORD(2, 2);
        }
    }
    else
    {
        lpWSAData->wVersion = MAKEWORD(2, 2);
    }

    lpWSAData->wVersion     = wVersionRequested;
    lpWSAData->wHighVersion = MAKEWORD(2,2);
    lstrcpyA(lpWSAData->szDescription, "WinSock 2.2");
    lstrcpyA(lpWSAData->szSystemStatus, "Running");
    lpWSAData->iMaxSockets  = 0;
    lpWSAData->iMaxUdpDg    = 0;
    lpWSAData->lpVendorInfo = NULL;

    /*FIXME: increment internal counter */

    WSASETINITIALIZED;

    return NO_ERROR;
}


/*
 * @implemented
 */
INT
EXPORT
WSACleanup(VOID)
{
    WS_DbgPrint(MAX_TRACE, ("WSACleanup of ws2_32.dll\n"));

    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        return WSANOTINITIALISED;
    }

    return NO_ERROR;
}


/*
 * @implemented
 */
SOCKET
EXPORT
socket(IN  INT af,
       IN  INT type,
       IN  INT protocol)
{
    return WSASocketW(af,
                      type,
                      protocol,
                      NULL,
                      0,
                      0);
}


/*
 * @implemented
 */
SOCKET
EXPORT
WSASocketA(IN  INT af,
           IN  INT type,
           IN  INT protocol,
           IN  LPWSAPROTOCOL_INFOA lpProtocolInfo,
           IN  GROUP g,
           IN  DWORD dwFlags)
/*
 * FUNCTION: Creates a new socket
 */
{
    WSAPROTOCOL_INFOW ProtocolInfoW;
    LPWSAPROTOCOL_INFOW p;
    UNICODE_STRING StringU;
    ANSI_STRING StringA;

    WS_DbgPrint(MAX_TRACE, ("af (%d)  type (%d)  protocol (%d).\n",
    af, type, protocol));

    if (lpProtocolInfo)
    {
        memcpy(&ProtocolInfoW,
               lpProtocolInfo,
               sizeof(WSAPROTOCOL_INFOA) - sizeof(CHAR) * (WSAPROTOCOL_LEN + 1));
        RtlInitAnsiString(&StringA, (LPSTR)lpProtocolInfo->szProtocol);
        RtlInitUnicodeString(&StringU, (LPWSTR)&ProtocolInfoW.szProtocol);
        RtlAnsiStringToUnicodeString(&StringU, &StringA, FALSE);
        p = &ProtocolInfoW;
    }
    else
    {
        p = NULL;
    }

    return WSASocketW(af,
                      type,
                      protocol,
                      p,
                      g,
                      dwFlags);
}


/*
 * @implemented
 */
SOCKET
EXPORT
WSASocketW(IN  INT af,
           IN  INT type,
           IN  INT protocol,
           IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
           IN  GROUP g,
           IN  DWORD dwFlags)
/*
 * FUNCTION: Creates a new socket descriptor
 * ARGUMENTS:
 *     af             = Address family
 *     type           = Socket type
 *     protocol       = Protocol type
 *     lpProtocolInfo = Pointer to protocol information
 *     g              = Reserved
 *     dwFlags        = Socket flags
 * RETURNS:
 *     Created socket descriptor, or INVALID_SOCKET if it could not be created
 */
{
    INT Status;
    SOCKET Socket;
    PCATALOG_ENTRY Provider;
    WSAPROTOCOL_INFOW ProtocolInfo;

    WS_DbgPrint(MAX_TRACE, ("af (%d)  type (%d)  protocol (%d).\n",
                af, type, protocol));

    if (!WSAINITIALIZED)
    {
        WS_DbgPrint(MAX_TRACE, ("af (%d)  type (%d)  protocol (%d) = WSANOTINITIALISED.\n",
                  af, type, protocol));
        WSASetLastError(WSANOTINITIALISED);
        return INVALID_SOCKET;
    }

    if (!lpProtocolInfo)
    {
        lpProtocolInfo = &ProtocolInfo;
        ZeroMemory(&ProtocolInfo, sizeof(WSAPROTOCOL_INFOW));

        ProtocolInfo.iAddressFamily = af;
        ProtocolInfo.iSocketType    = type;
        ProtocolInfo.iProtocol      = protocol;
    }

    Provider = LocateProvider(lpProtocolInfo);
    if (!Provider)
    {
        WS_DbgPrint(MAX_TRACE, ("af (%d)  type (%d)  protocol (%d) = WSAEAFNOSUPPORT.\n",
                    af, type, protocol));
        WSASetLastError(WSAEAFNOSUPPORT);
        return INVALID_SOCKET;
    }

    Status = LoadProvider(Provider, lpProtocolInfo);
    if (Status != NO_ERROR)
    {
        WS_DbgPrint(MAX_TRACE, ("af (%d)  type (%d)  protocol (%d) = %d.\n",
                    af, type, protocol, Status));
        WSASetLastError(Status);
        return INVALID_SOCKET;
    }

    WS_DbgPrint(MAX_TRACE, ("Calling WSPSocket at (0x%X).\n",
                Provider->ProcTable.lpWSPSocket));

    assert(Provider->ProcTable.lpWSPSocket);

    WS_DbgPrint(MAX_TRACE,("About to call provider socket fn\n"));

    Socket = Provider->ProcTable.lpWSPSocket(af,
                                             type,
                                             protocol,
                                             lpProtocolInfo,
                                             g,
                                             dwFlags,
                                             &Status);

    WS_DbgPrint(MAX_TRACE,("Socket: %x, Status: %x\n", Socket, Status));

    if (Status != NO_ERROR)
    {
        WSASetLastError(Status);
        return INVALID_SOCKET;
    }

    WS_DbgPrint(MAX_TRACE,("Status: %x\n", Status));

    return Socket;
}


/*
 * @implemented
 */
INT
EXPORT
closesocket(IN  SOCKET s)
/*
 * FUNCTION: Closes a socket descriptor
 * ARGUMENTS:
 *     s = Socket descriptor
 * RETURNS:
 *     0, or SOCKET_ERROR if an error ocurred
 */
{
    PCATALOG_ENTRY Provider;
    INT Status;
    INT Errno;

    WS_DbgPrint(MAX_TRACE, ("s (0x%X).\n", s));

    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }

    if (!ReferenceProviderByHandle((HANDLE)s, &Provider))
    {
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }

    CloseProviderHandle((HANDLE)s);

    WS_DbgPrint(MAX_TRACE,("DereferenceProviderByHandle\n"));

    DereferenceProviderByPointer(Provider);

    WS_DbgPrint(MAX_TRACE,("DereferenceProviderByHandle Done\n"));

    Status = Provider->ProcTable.lpWSPCloseSocket(s, &Errno);

    WS_DbgPrint(MAX_TRACE,("Provider Close Done\n"));

    if (Status == SOCKET_ERROR)
        WSASetLastError(Errno);

    WS_DbgPrint(MAX_TRACE,("Returning success\n"));

    return 0;
}


/*
 * @implemented
 */
INT
EXPORT
select(IN      INT nfds,
       IN OUT  LPFD_SET readfds,
       IN OUT  LPFD_SET writefds,
       IN OUT  LPFD_SET exceptfds,
       IN      CONST struct timeval *timeout)
/*
 * FUNCTION: Returns status of one or more sockets
 * ARGUMENTS:
 *     nfds      = Always ignored
 *     readfds   = Pointer to socket set to be checked for readability (optional)
 *     writefds  = Pointer to socket set to be checked for writability (optional)
 *     exceptfds = Pointer to socket set to be checked for errors (optional)
 *     timeout   = Pointer to a TIMEVAL structure indicating maximum wait time
 *                 (NULL means wait forever)
 * RETURNS:
 *     Number of ready socket descriptors, or SOCKET_ERROR if an error ocurred
 */
{
    PCATALOG_ENTRY Provider = NULL;
    INT Count;
    INT Errno;

    WS_DbgPrint(MAX_TRACE, ("readfds (0x%X)  writefds (0x%X)  exceptfds (0x%X).\n",
                readfds, writefds, exceptfds));

    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        WS_DbgPrint(MID_TRACE,("Not initialized\n"));
        return SOCKET_ERROR;
    }

    /* FIXME: Sockets in FD_SETs should be sorted by their provider */

    /* FIXME: For now, assume only one service provider */
    if ((readfds != NULL) && (readfds->fd_count > 0))
    {
        if (!ReferenceProviderByHandle((HANDLE)readfds->fd_array[0],
                                       &Provider))
        {
            WSASetLastError(WSAENOTSOCK);
            WS_DbgPrint(MID_TRACE,("No provider (read)\n"));
            return SOCKET_ERROR;
        }
    }
    else if ((writefds != NULL) && (writefds->fd_count > 0))
    {
        if (!ReferenceProviderByHandle((HANDLE)writefds->fd_array[0],
                                       &Provider))
        {
            WSASetLastError(WSAENOTSOCK);
            WS_DbgPrint(MID_TRACE,("No provider (write)\n"));
            return SOCKET_ERROR;
        }
    }
    else if ((exceptfds != NULL) && (exceptfds->fd_count > 0))
    {
        if (!ReferenceProviderByHandle((HANDLE)exceptfds->fd_array[0], &Provider))
        {
            WSASetLastError(WSAENOTSOCK);
            WS_DbgPrint(MID_TRACE,("No provider (err)\n"));
            return SOCKET_ERROR;
        }
#if 0 /* XXX empty select is not an error */
    }
    else
    {
        WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
#endif
    }

    if ( !Provider )
    {
        if ( timeout )
        {
            WS_DbgPrint(MID_TRACE,("Select: used as timer\n"));
            Sleep( timeout->tv_sec * 1000 + (timeout->tv_usec / 1000) );
        }
        return 0;
    }
    else if (Provider->ProcTable.lpWSPSelect)
    {
        WS_DbgPrint(MID_TRACE,("Calling WSPSelect:%x\n", Provider->ProcTable.lpWSPSelect));
        Count = Provider->ProcTable.lpWSPSelect(nfds,
                                                readfds,
                                                writefds,
                                                exceptfds,
                                                (LPTIMEVAL)timeout,
                                                &Errno);

        WS_DbgPrint(MAX_TRACE, ("[%x] Select: Count %d Errno %x\n",
                    Provider, Count, Errno));

        DereferenceProviderByPointer(Provider);

        if (Errno != NO_ERROR)
        {
            WSASetLastError(Errno);
            return SOCKET_ERROR;
        }
    }
    else
    {
        WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    return Count;
}


/*
 * @implemented
 */
INT
EXPORT
bind(IN SOCKET s,
     IN CONST struct sockaddr *name,
     IN INT namelen)
{
    PCATALOG_ENTRY Provider;
    INT Status;
    INT Errno;

    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }

    if (!ReferenceProviderByHandle((HANDLE)s,
                                   &Provider))
    {
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)
    Status = Provider->ProcTable.lpWSPBind(s,
                                           (CONST LPSOCKADDR)name,
                                           namelen,
                                           &Errno);
#else
    Status = Provider->ProcTable.lpWSPBind(s,
                                           name,
                                           namelen,
                                           &Errno);
#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

    DereferenceProviderByPointer(Provider);

    if (Status == SOCKET_ERROR)
        WSASetLastError(Errno);

  return Status;
}


/*
 * @implemented
 */
INT
EXPORT
listen(IN SOCKET s,
       IN INT backlog)
{
    PCATALOG_ENTRY Provider;
    INT Status;
    INT Errno;

    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }

    if (!ReferenceProviderByHandle((HANDLE)s,
                                   &Provider))
    {
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }

    Status = Provider->ProcTable.lpWSPListen(s,
                                             backlog,
                                             &Errno);

    DereferenceProviderByPointer(Provider);

    if (Status == SOCKET_ERROR)
        WSASetLastError(Errno);

    return Status;
}


/*
 * @implemented
 */
SOCKET
EXPORT
accept(IN  SOCKET s,
       OUT LPSOCKADDR addr,
       OUT INT FAR* addrlen)
{
  return WSAAccept(s,
                   addr,
                   addrlen,
                   NULL,
                   0);
}


/*
 * @implemented
 */
INT
EXPORT
ioctlsocket(IN     SOCKET s,
            IN     LONG cmd,
            IN OUT ULONG FAR* argp)
{
    return WSAIoctl(s,
                    cmd,
                    argp,
                    sizeof(ULONG),
                    argp,
                    sizeof(ULONG),
                    argp,
                    0,
                    0);
}


/*
 * @implemented
 */
SOCKET
EXPORT
WSAAccept(IN     SOCKET s,
          OUT    LPSOCKADDR addr,
          IN OUT LPINT addrlen,
          IN     LPCONDITIONPROC lpfnCondition,
          IN     DWORD dwCallbackData)
{
    PCATALOG_ENTRY Provider;
    SOCKET Socket;
    INT Errno;

    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }

    if (!ReferenceProviderByHandle((HANDLE)s, &Provider))
    {
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }

    WS_DbgPrint(MAX_TRACE,("Calling provider accept\n"));

    Socket = Provider->ProcTable.lpWSPAccept(s,
                                             addr,
                                             addrlen,
                                             lpfnCondition,
                                             dwCallbackData,
                                             &Errno);

    WS_DbgPrint(MAX_TRACE,("Calling provider accept -> Socket %x, Errno %x\n",
                Socket, Errno));

    DereferenceProviderByPointer(Provider);

    if (Socket == INVALID_SOCKET)
        WSASetLastError(Errno);

    if ( addr )
    {
#ifdef DBG
        LPSOCKADDR_IN sa = (LPSOCKADDR_IN)addr;
        WS_DbgPrint(MAX_TRACE,("Returned address: %d %s:%d (len %d)\n",
                               sa->sin_family,
                               inet_ntoa(sa->sin_addr),
                               ntohs(sa->sin_port),
                               *addrlen));
#endif
    }

    return Socket;
}


/*
 * @implemented
 */
INT
EXPORT
connect(IN  SOCKET s,
        IN  CONST struct sockaddr *name,
        IN  INT namelen)
{
  return WSAConnect(s,
                    name,
                    namelen,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
}


/*
 * @implemented
 */
INT
EXPORT
WSAConnect(IN  SOCKET s,
           IN  CONST struct sockaddr *name,
           IN  INT namelen,
           IN  LPWSABUF lpCallerData,
           OUT LPWSABUF lpCalleeData,
           IN  LPQOS lpSQOS,
           IN  LPQOS lpGQOS)
{
    PCATALOG_ENTRY Provider;
    INT Status;
    INT Errno;

    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }

    if (!ReferenceProviderByHandle((HANDLE)s, &Provider))
    {
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)
    Status = Provider->ProcTable.lpWSPConnect(s,
                                              (CONST LPSOCKADDR)name,
                                              namelen,
                                              lpCallerData,
                                              lpCalleeData,
                                              lpSQOS,
                                              lpGQOS,
                                              &Errno);
#else
    Status = Provider->ProcTable.lpWSPConnect(s,
                                              name,
                                              namelen,
                                              lpCallerData,
                                              lpCalleeData,
                                              lpSQOS,
                                              lpGQOS,
                                              &Errno);
#endif

    DereferenceProviderByPointer(Provider);

    if (Status == SOCKET_ERROR)
        WSASetLastError(Errno);

    return Status;
}


/*
 * @implemented
 */
INT
EXPORT
WSAIoctl(IN  SOCKET s,
         IN  DWORD dwIoControlCode,
         IN  LPVOID lpvInBuffer,
         IN  DWORD cbInBuffer,
         OUT LPVOID lpvOutBuffer,
         IN  DWORD cbOutBuffer,
         OUT LPDWORD lpcbBytesReturned,
         IN  LPWSAOVERLAPPED lpOverlapped,
         IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    PCATALOG_ENTRY Provider;
    INT Status;
    INT Errno;

    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }

    if (!ReferenceProviderByHandle((HANDLE)s, &Provider))
    {
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }

    Status = Provider->ProcTable.lpWSPIoctl(s,
                                            dwIoControlCode,
                                            lpvInBuffer,
                                            cbInBuffer,
                                            lpvOutBuffer,
                                            cbOutBuffer,
                                            lpcbBytesReturned,
                                            lpOverlapped,
                                            lpCompletionRoutine,
                                            NULL /* lpThreadId */,
                                            &Errno);

    DereferenceProviderByPointer(Provider);

    if (Status == SOCKET_ERROR)
        WSASetLastError(Errno);

    return Status;
}

/*
 * @implemented
 */
INT
EXPORT
__WSAFDIsSet(SOCKET s, LPFD_SET set)
{
    unsigned int i;

    for ( i = 0; i < set->fd_count; i++ )
    if ( set->fd_array[i] == s ) return TRUE;

    return FALSE;
}

void free_winsock_thread_block(PWINSOCK_THREAD_BLOCK p)
{
    if (p)
    {
        if (p->Hostent) { free_hostent(p->Hostent); p->Hostent = 0; }
        if (p->Getservbyname){}
        if (p->Getservbyport) {}
    }
}

BOOL
STDCALL
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        LPVOID lpReserved)
{
    PWINSOCK_THREAD_BLOCK p;

    WS_DbgPrint(MAX_TRACE, ("DllMain of ws2_32.dll.\n"));

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            GlobalHeap = GetProcessHeap();

            g_hInstDll = hInstDll;

            CreateCatalog();

            InitProviderHandleTable();

            UpcallTable.lpWPUCloseEvent         = WPUCloseEvent;
            UpcallTable.lpWPUCloseSocketHandle  = WPUCloseSocketHandle;
            UpcallTable.lpWPUCreateEvent        = WPUCreateEvent;
            UpcallTable.lpWPUCreateSocketHandle = WPUCreateSocketHandle;
            UpcallTable.lpWPUFDIsSet            = WPUFDIsSet;
            UpcallTable.lpWPUGetProviderPath    = WPUGetProviderPath;
            UpcallTable.lpWPUModifyIFSHandle    = WPUModifyIFSHandle;
            UpcallTable.lpWPUPostMessage        = PostMessageW;
            UpcallTable.lpWPUQueryBlockingCallback    = WPUQueryBlockingCallback;
            UpcallTable.lpWPUQuerySocketHandleContext = WPUQuerySocketHandleContext;
            UpcallTable.lpWPUQueueApc           = WPUQueueApc;
            UpcallTable.lpWPUResetEvent         = WPUResetEvent;
            UpcallTable.lpWPUSetEvent           = WPUSetEvent;
            UpcallTable.lpWPUOpenCurrentThread  = WPUOpenCurrentThread;
            UpcallTable.lpWPUCloseThread        = WPUCloseThread;

            /* Fall through to thread attachment handler */
        }
        case DLL_THREAD_ATTACH:
        {
            p = HeapAlloc(GlobalHeap, 0, sizeof(WINSOCK_THREAD_BLOCK));

            WS_DbgPrint(MAX_TRACE, ("Thread block at 0x%X.\n", p));

            if (!p) {
              return FALSE;
            }

            p->Hostent = NULL;
            p->LastErrorValue = NO_ERROR;
            p->Getservbyname  = NULL;
            p->Getservbyport  = NULL;

            NtCurrentTeb()->WinSockData = p;
        }
        break;

        case DLL_PROCESS_DETACH:
        {
            p = NtCurrentTeb()->WinSockData;

            if (p)
              HeapFree(GlobalHeap, 0, p);

            DestroyCatalog();

            FreeProviderHandleTable();
        }
        break;

        case DLL_THREAD_DETACH:
        {
            p = NtCurrentTeb()->WinSockData;

            if (p)
              HeapFree(GlobalHeap, 0, p);
        }
        break;
    }

    WS_DbgPrint(MAX_TRACE, ("DllMain of ws2_32.dll. Leaving.\n"));

    return TRUE;
}

/* EOF */
