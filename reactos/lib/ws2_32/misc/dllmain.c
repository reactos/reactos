/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/dllmain.c
 * PURPOSE:     DLL entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ws2_32.h>
#include <catalog.h>
#include <handle.h>
#include <upcall.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;
//DWORD DebugTraceLevel = MAX_TRACE;

#endif /* DBG */

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}


HANDLE GlobalHeap;
WSPUPCALLTABLE UpcallTable;


INT
EXPORT
WSAGetLastError(VOID)
{
  PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

  if (p) {
    return p->LastErrorValue;
  } else {
    /* FIXME: What error code should we use here? Can this even happen? */
    return ERROR_BAD_ENVIRONMENT;
  }
}


VOID
EXPORT
WSASetLastError(
    IN  INT iError)
{
  PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

  if (p)
      p->LastErrorValue = iError;
}


INT
EXPORT
WSAStartup(
    IN  WORD wVersionRequested,
    OUT LPWSADATA lpWSAData)
{
  WS_DbgPrint(MAX_TRACE, ("WSAStartup of ws2_32.dll\n"));

  lpWSAData->wVersion     = wVersionRequested;
  lpWSAData->wHighVersion = 2;
  lstrcpyA(lpWSAData->szDescription, "WinSock 2.0");
  lstrcpyA(lpWSAData->szSystemStatus, "Running");
  lpWSAData->iMaxSockets  = 0;
  lpWSAData->iMaxUdpDg    = 0;
  lpWSAData->lpVendorInfo = NULL;

  WSASETINITIALIZED;

  return NO_ERROR;
}


INT
EXPORT
WSACleanup(VOID)
{
  WS_DbgPrint(MAX_TRACE, ("WSACleanup of ws2_32.dll\n"));

  if (!WSAINITIALIZED) {
    WSASetLastError(WSANOTINITIALISED);
    return WSANOTINITIALISED;
  }

  return NO_ERROR;
}


SOCKET
EXPORT
WSASocketA(
    IN  INT af,
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

  if (lpProtocolInfo) {
    memcpy(&ProtocolInfoW,
      lpProtocolInfo,
      sizeof(WSAPROTOCOL_INFOA) -
      sizeof(CHAR) * (WSAPROTOCOL_LEN + 1));
    RtlInitAnsiString(&StringA, (LPSTR)lpProtocolInfo->szProtocol);
    RtlInitUnicodeString(&StringU, (LPWSTR)&ProtocolInfoW.szProtocol);
    RtlAnsiStringToUnicodeString(&StringU, &StringA, FALSE);
    p = &ProtocolInfoW;
  } else {
    p = NULL;
  }

  return WSASocketW(af,
    type,
    protocol,
    p,
    g,
    dwFlags);
}


SOCKET
EXPORT
WSASocketW(
    IN  INT af,
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

  if (!WSAINITIALIZED) {
      WSASetLastError(WSANOTINITIALISED);
      return INVALID_SOCKET;
  }

  if (!lpProtocolInfo) {
    lpProtocolInfo = &ProtocolInfo;
    ZeroMemory(&ProtocolInfo, sizeof(WSAPROTOCOL_INFOW));

    ProtocolInfo.iAddressFamily = af;
    ProtocolInfo.iSocketType    = type;
    ProtocolInfo.iProtocol      = protocol;
  }

  Provider = LocateProvider(lpProtocolInfo);
  if (!Provider) {
    WSASetLastError(WSAEAFNOSUPPORT);
    return INVALID_SOCKET;
  }

  Status = LoadProvider(Provider, lpProtocolInfo);
  if (Status != NO_ERROR) {
    WSASetLastError(Status);
    return INVALID_SOCKET;
  }

  WS_DbgPrint(MAX_TRACE, ("Calling WSPSocket at (0x%X).\n",
    Provider->ProcTable.lpWSPSocket));

  assert(Provider->ProcTable.lpWSPSocket);

  Socket = Provider->ProcTable.lpWSPSocket(
    af,
    type,
    protocol,
    lpProtocolInfo,
    g,
    dwFlags,
    &Status);
	if (Status != NO_ERROR) {
    WSASetLastError(Status);
    return INVALID_SOCKET;
  }

  return Socket;
}


INT
EXPORT
closesocket(
    IN  SOCKET s)
/*
 * FUNCTION: Closes a socket descriptor
 * ARGUMENTS:
 *     s = Socket descriptor
 * RETURNS:
 *     0, or SOCKET_ERROR if an error ocurred
 */
{
  PCATALOG_ENTRY Provider;
  INT Errno;
  INT Code;

  WS_DbgPrint(MAX_TRACE, ("s (0x%X).\n", s));

  if (!WSAINITIALIZED) {
    WSASetLastError(WSANOTINITIALISED);
    return SOCKET_ERROR;
  }

  if (!ReferenceProviderByHandle((HANDLE)s, &Provider)) {
    WSASetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
  }

  CloseProviderHandle((HANDLE)s);

  DereferenceProviderByPointer(Provider);

  Code = Provider->ProcTable.lpWSPCloseSocket(s, &Errno);
  if (Code == SOCKET_ERROR)
    WSASetLastError(Errno);

  return 0;
}


INT
EXPORT
select(
    IN      INT nfds, 
    IN OUT  LPFD_SET readfds, 
    IN OUT  LPFD_SET writefds, 
    IN OUT  LPFD_SET exceptfds, 
    IN      CONST LPTIMEVAL timeout)
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
  PCATALOG_ENTRY Provider;
  INT Count;
  INT Errno;
  ULONG i;

  WS_DbgPrint(MAX_TRACE, ("readfds (0x%X)  writefds (0x%X)  exceptfds (0x%X).\n",
    readfds, writefds, exceptfds));

  if (!WSAINITIALIZED) {
    WSASetLastError(WSANOTINITIALISED);
    return SOCKET_ERROR;
  }

  /* FIXME: Sockets in FD_SETs should be sorted by their provider */

  /* FIXME: For now, assume only one service provider */
  if ((readfds != NULL) && (readfds->fd_count > 0)) {
    if (!ReferenceProviderByHandle((HANDLE)readfds->fd_array[0], &Provider)) {
      WSASetLastError(WSAENOTSOCK);
      return SOCKET_ERROR;
    }
  } else if ((writefds != NULL) && (writefds->fd_count > 0)) {
    if (!ReferenceProviderByHandle((HANDLE)writefds->fd_array[0], &Provider)) {
      WSASetLastError(WSAENOTSOCK);
      return SOCKET_ERROR;
    }
  } else if ((exceptfds != NULL) && (exceptfds->fd_count > 0)) {
    if (!ReferenceProviderByHandle((HANDLE)exceptfds->fd_array[0], &Provider)) {
      WSASetLastError(WSAENOTSOCK);
      return SOCKET_ERROR;
    }
  } else {
    WSASetLastError(WSAEINVAL);
    return SOCKET_ERROR;
  }

  Count = Provider->ProcTable.lpWSPSelect(
    nfds, readfds, writefds, exceptfds, timeout, &Errno);

  WS_DbgPrint(MAX_TRACE, ("Provider (0x%X).\n", Provider));

  DereferenceProviderByPointer(Provider);

  WSASetLastError(Errno);

  if (Errno != NO_ERROR)
    return SOCKET_ERROR;

  return Count;
}


BOOL
STDCALL
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        LPVOID lpReserved)
{
  PWINSOCK_THREAD_BLOCK p;

  WS_DbgPrint(MAX_TRACE, ("DllMain of ws2_32.dll.\n"));

  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    GlobalHeap = GetProcessHeap();

    CreateCatalog();

    InitProviderHandleTable();

    UpcallTable.lpWPUCloseEvent         = WPUCloseEvent;
    UpcallTable.lpWPUCloseSocketHandle  = WPUCloseSocketHandle;
    UpcallTable.lpWPUCreateEvent        = WPUCreateEvent;
    UpcallTable.lpWPUCreateSocketHandle = WPUCreateSocketHandle;
    UpcallTable.lpWPUFDIsSet            = WPUFDIsSet;
    UpcallTable.lpWPUGetProviderPath    = WPUGetProviderPath;
    UpcallTable.lpWPUModifyIFSHandle    = WPUModifyIFSHandle;
    UpcallTable.lpWPUPostMessage        = WPUPostMessage;
    UpcallTable.lpWPUQueryBlockingCallback    = WPUQueryBlockingCallback;
    UpcallTable.lpWPUQuerySocketHandleContext = WPUQuerySocketHandleContext;
    UpcallTable.lpWPUQueueApc           = WPUQueueApc;
    UpcallTable.lpWPUResetEvent         = WPUResetEvent;
    UpcallTable.lpWPUSetEvent           = WPUSetEvent;
    UpcallTable.lpWPUOpenCurrentThread  = WPUOpenCurrentThread;
    UpcallTable.lpWPUCloseThread        = WPUCloseThread;

    /* Fall through to thread attachment handler */

  case DLL_THREAD_ATTACH:
    p = HeapAlloc(GlobalHeap, 0, sizeof(WINSOCK_THREAD_BLOCK));

    WS_DbgPrint(MAX_TRACE, ("Thread block at 0x%X.\n", p));
        
    if (!p) {
      return FALSE;
    }

    p->LastErrorValue = NO_ERROR;
    p->Initialized    = FALSE;

    NtCurrentTeb()->WinSockData = p;
    break;

  case DLL_PROCESS_DETACH:
    p = NtCurrentTeb()->WinSockData;

    if (p)
      HeapFree(GlobalHeap, 0, p);

    DestroyCatalog();

    FreeProviderHandleTable();
    break;

  case DLL_THREAD_DETACH:
    p = NtCurrentTeb()->WinSockData;

    if (p)
      HeapFree(GlobalHeap, 0, p);
    break;
  }

  WS_DbgPrint(MAX_TRACE, ("DllMain of ws2_32.dll. Leaving.\n"));

  return TRUE;
}

/* EOF */
