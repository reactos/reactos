/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/stubs.c
 * PURPOSE:     Stubs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ws2_32.h>
#include <catalog.h>
#include <handle.h>


/*
 * @implemented
 */
INT
EXPORT
getpeername(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  INT FAR* namelen)
{
  int Error;
  INT Errno;
  PCATALOG_ENTRY Provider;

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
  
  Error = Provider->ProcTable.lpWSPGetPeerName(s,
                                               name,
                                               namelen,
                                               &Errno);
                                              
  DereferenceProviderByPointer(Provider);                                              
  
  if (Error == SOCKET_ERROR)
  {
    WSASetLastError(Errno);
  }
 
  return Error;
}



/*
 * @implemented
 */
INT
EXPORT
getsockname(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  INT FAR* namelen)
{
  int Error;
  INT Errno;
  PCATALOG_ENTRY Provider;

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
  
  Error = Provider->ProcTable.lpWSPGetSockName(s,
                                               name,
                                               namelen,
                                               &Errno);
                                              
  DereferenceProviderByPointer(Provider);                                              
  
  if (Error == SOCKET_ERROR)
  {
    WSASetLastError(Errno);
  }
 
  return Error;
}


/*
 * @implemented
 */
INT
EXPORT
getsockopt(
    IN      SOCKET s,
    IN      INT level,
    IN      INT optname,
    OUT     CHAR FAR* optval,
    IN OUT  INT FAR* optlen)
{
  PCATALOG_ENTRY Provider;
  INT Errno;
  int Error;

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
  
  Error = Provider->ProcTable.lpWSPGetSockOpt(s,
                                              level,
                                              optname,
                                              optval,
                                              optlen,
                                              &Errno);
                                              
  DereferenceProviderByPointer(Provider);                                              
  
  if (Error == SOCKET_ERROR)
  {
    WSASetLastError(Errno);
  }
 
  return Error;
}


/*
 * @implemented
 */
INT
EXPORT
setsockopt(
    IN  SOCKET s,
    IN  INT level,
    IN  INT optname,
    IN  CONST CHAR FAR* optval,
    IN  INT optlen)
{
  PCATALOG_ENTRY Provider;
  INT Errno;
  int Error;

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
  
  Error = Provider->ProcTable.lpWSPSetSockOpt(s,
                                              level,
                                              optname,
                                              optval,
                                              optlen,
                                              &Errno);
                                              
  DereferenceProviderByPointer(Provider);                                              
  
  if (Error == SOCKET_ERROR)
  {
    WSASetLastError(Errno);
  }
 
  return Error;
}


/*
 * @implemented
 */
INT
EXPORT
shutdown(
    IN  SOCKET s,
    IN  INT how)
{
  PCATALOG_ENTRY Provider;
  INT Errno;
  int Error;

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
  
  Error = Provider->ProcTable.lpWSPShutdown(s,
                                            how,
                                            &Errno);
                                              
  DereferenceProviderByPointer(Provider);                                              
  
  if (Error == SOCKET_ERROR)
  {
    WSASetLastError(Errno);
  }
 
  return Error;
}


/*
 * @implemented
 */
INT
EXPORT
WSAAsyncSelect(
    IN  SOCKET s,
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  LONG lEvent)
{
  PCATALOG_ENTRY Provider;
  INT Errno;
  int Error;

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
  
  Error = Provider->ProcTable.lpWSPAsyncSelect(s,
                                               hWnd,
                                               wMsg,
                                               lEvent,
                                               &Errno);
                                              
  DereferenceProviderByPointer(Provider);                                              
  
  if (Error == SOCKET_ERROR)
  {
    WSASetLastError(Errno);
  }
 
  return Error;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSACancelBlockingCall(VOID)
{
#if 0
  INT Errno;
  int Error;
  PCATALOG_ENTRY Provider;

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
  
  Error = Provider->ProcTable.lpWSPCancelBlockingCall(&Errno);
                                              
  DereferenceProviderByPointer(Provider);                                              
  
  if (Error == SOCKET_ERROR)
  {
    WSASetLastError(Errno);
  }
 
  return Error;
#endif

  UNIMPLEMENTED
  
  return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSADuplicateSocketA(
    IN  SOCKET s,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOA lpProtocolInfo)
{
#if 0
  WSAPROTOCOL_INFOA ProtocolInfoU;
  
  Error  = WSADuplicateSocketW(s,
                               dwProcessId,
                               &ProtocolInfoU);
                               
  if (Error == NO_ERROR)
  {
    UnicodeToAnsi(lpProtocolInfo, ProtocolInfoU, sizeof( 
    
  }
  
  return Error;
#endif
  
  UNIMPLEMENTED
  
  return 0;
}



/*
 * @implemented
 */
INT
EXPORT
WSADuplicateSocketW(
    IN  SOCKET s,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
  INT Errno;
  int Error;
  PCATALOG_ENTRY Provider;

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
  
  Error = Provider->ProcTable.lpWSPDuplicateSocket(s,
                                                   dwProcessId,
                                                   lpProtocolInfo,
                                                   &Errno);
  DereferenceProviderByPointer(Provider);                                              
  
  if (Error == SOCKET_ERROR)
  {
    WSASetLastError(Errno);
  }
 
  return Error;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAEnumProtocolsA(
    IN      LPINT lpiProtocols,
    OUT     LPWSAPROTOCOL_INFOA lpProtocolBuffer,
    IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAEnumProtocolsW(
    IN      LPINT lpiProtocols,
    OUT     LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @implemented
 */
BOOL
EXPORT
WSAGetOverlappedResult(
    IN  SOCKET s,
    IN  LPWSAOVERLAPPED lpOverlapped,
    OUT LPDWORD lpcbTransfer,
    IN  BOOL fWait,
    OUT LPDWORD lpdwFlags)
{
  INT Errno;
  BOOL Success;
  PCATALOG_ENTRY Provider;

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
  
  Success = Provider->ProcTable.lpWSPGetOverlappedResult(s,
                                                        lpOverlapped,
                                                        lpcbTransfer,
                                                        fWait,
                                                        lpdwFlags,
                                                        &Errno);
  DereferenceProviderByPointer(Provider);                                              
  
  if (Success == FALSE)
  {
    WSASetLastError(Errno);
  }
 
  return Success;
}


/*
 * @unimplemented
 */
BOOL
EXPORT
WSAGetQOSByName(
    IN      SOCKET s, 
    IN OUT  LPWSABUF lpQOSName, 
    OUT     LPQOS lpQOS)
{
    UNIMPLEMENTED

    return FALSE;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAHtonl(
    IN  SOCKET s,
    IN  ULONG hostLONG,
    OUT ULONG FAR* lpnetlong)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAHtons(
    IN  SOCKET s,
    IN  USHORT hostshort,
    OUT USHORT FAR* lpnetshort)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
BOOL
EXPORT
WSAIsBlocking(VOID)
{
    UNIMPLEMENTED

    return FALSE;
}


/*
 * @unimplemented
 */
SOCKET
EXPORT
WSAJoinLeaf(
    IN  SOCKET s,
    IN  CONST struct sockaddr *name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS,
    IN  DWORD dwFlags)
{
    UNIMPLEMENTED

    return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSANtohl(
    IN  SOCKET s,
    IN  ULONG netlong,
    OUT ULONG FAR* lphostlong)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSANtohs(
    IN  SOCKET s,
    IN  USHORT netshort,
    OUT USHORT FAR* lphostshort)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
FARPROC
EXPORT
WSASetBlockingHook(
    IN  FARPROC lpBlockFunc)
{
    UNIMPLEMENTED

    return (FARPROC)0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAUnhookBlockingHook(VOID)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAProviderConfigChange(
    IN OUT  LPHANDLE lpNotificationHandle,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSACancelAsyncRequest(
    IN  HANDLE hAsyncTaskHandle)
{
    UNIMPLEMENTED

    return 0;
}

/*
 * @unimplemented
 */
INT
#if 0
PASCAL FAR
#else
EXPORT
#endif
__WSAFDIsSet(SOCKET s, LPFD_SET set)
{
    UNIMPLEMENTED

    return 0;
}


/* WinSock Service Provider support functions */

/*
 * @unimplemented
 */
INT
EXPORT
WPUCompleteOverlappedRequest(
    IN  SOCKET s,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  DWORD dwError,
    IN  DWORD cbTransferred,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSPStartup(
    IN  WORD wVersionRequested,
    OUT LPWSPDATA lpWSPData,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  WSPUPCALLTABLE UpcallTable,
    OUT LPWSPPROC_TABLE lpProcTable)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCDeinstallProvider(
    IN  LPGUID lpProviderId,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCEnumProtocols(
    IN      LPINT lpiProtocols,
    OUT     LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCGetProviderPath(
    IN      LPGUID lpProviderId,
    OUT     LPWSTR lpszProviderDllPath,
    IN OUT  LPINT lpProviderDllPathLen,
    OUT     LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCInstallProvider(
    IN  CONST LPGUID lpProviderId,
    IN  CONST LPWSTR lpszProviderDllPath,
    IN  CONST LPWSAPROTOCOL_INFOW lpProtocolInfoList,
    IN  DWORD dwNumberOfEntries,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCEnableNSProvider(
    IN  LPGUID lpProviderId,
    IN  BOOL fEnable)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCInstallNameSpace(
    IN  LPWSTR lpszIdentifier,
    IN  LPWSTR lpszPathName,
    IN  DWORD dwNameSpace,
    IN  DWORD dwVersion,
    IN  LPGUID lpProviderId)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCUnInstallNameSpace(
    IN  LPGUID lpProviderId)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCWriteProviderOrder(
    IN  LPDWORD lpwdCatalogEntryId,
    IN  DWORD dwNumberOfEntries)
{
    UNIMPLEMENTED

    return 0;
}

/*
 * @unimplemented
 */
INT
EXPORT
WSANSPIoctl(
    HANDLE           hLookup,
    DWORD            dwControlCode,
    LPVOID           lpvInBuffer,
    DWORD            cbInBuffer,
    LPVOID           lpvOutBuffer,
    DWORD            cbOutBuffer,
    LPDWORD          lpcbBytesReturned,
    LPWSACOMPLETION  lpCompletion
    )
{
    //UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCUpdateProvider(
    LPGUID lpProviderId,
    const WCHAR FAR * lpszProviderDllPath,
    const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
    DWORD dwNumberOfEntries,
    LPINT lpErrno
    )
{
    UNIMPLEMENTED

    return 0;
}

/*
 * @unimplemented
 */
INT
EXPORT
WSCWriteNameSpaceOrder (
    LPGUID lpProviderId,
    DWORD dwNumberOfEntries
    )
{
    UNIMPLEMENTED

    return 0;
}

/*
 * @unimplemented
 */
VOID
EXPORT
freeaddrinfo(
    struct addrinfo *pAddrInfo
    )
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
INT
EXPORT
getaddrinfo(
    const char FAR * nodename,
    const char FAR * servname,
    const struct addrinfo FAR * hints,
    struct addrinfo FAR * FAR * res
    )
{
    UNIMPLEMENTED

    return 0;
}

/*
 * @unimplemented
 */
INT
EXPORT
getnameinfo(
    const struct sockaddr FAR * sa,
    socklen_t       salen,
    char FAR *      host,
    DWORD           hostlen,
    char FAR *      serv,
    DWORD           servlen,
    INT             flags
    )
{
    UNIMPLEMENTED

    return 0;
}

/*
 * @unimplemented
 */
VOID EXPORT WEP()
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
BOOL EXPORT WSApSetPostRoutine(PVOID Routine)
{
    UNIMPLEMENTED

    return 0;
}

/* EOF */
