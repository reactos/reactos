/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/stubs.c
 * PURPOSE:     Stubs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */

#include "ws2_32.h"

#include <ws2tcpip.h>
#include <strsafe.h>

/*
 * @implemented
 */
INT
EXPORT
getpeername(IN     SOCKET s,
            OUT    LPSOCKADDR name,
            IN OUT INT FAR* namelen)
{
    int Error;
    INT Errno;
    PCATALOG_ENTRY Provider;

    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }

    if (!ReferenceProviderByHandle((HANDLE)(ULONG_PTR)(s), &Provider))
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
getsockname(IN     SOCKET s,
            OUT    LPSOCKADDR name,
            IN OUT INT FAR* namelen)
{
    int Error;
    INT Errno;
    PCATALOG_ENTRY Provider;

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
getsockopt(IN      SOCKET s,
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

    if (!ReferenceProviderByHandle((HANDLE)s,
                                   &Provider))
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
EXPORT __stdcall
setsockopt(IN  SOCKET s,
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

    if(IS_INTRESOURCE(optval))
    {
        SetLastError(WSAEFAULT);
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
shutdown(IN  SOCKET s,
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
WSAAsyncSelect(IN  SOCKET s,
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

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSADuplicateSocketA(IN  SOCKET s,
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
        UnicodeToAnsi(lpProtocolInfo,
                      ProtocolInfoU,
                      sizeof(

    }

    return Error;
#endif

    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}



/*
 * @implemented
 */
INT
EXPORT
WSADuplicateSocketW(IN  SOCKET s,
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
WSAEnumProtocolsA(IN      LPINT lpiProtocols,
                  OUT     LPWSAPROTOCOL_INFOA lpProtocolBuffer,
                  IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAEnumProtocolsW(IN      LPINT lpiProtocols,
                  OUT     LPWSAPROTOCOL_INFOW lpProtocolBuffer,
                  IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @implemented
 */
BOOL
EXPORT
WSAGetOverlappedResult(IN  SOCKET s,
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
WSAGetQOSByName(IN      SOCKET s,
                IN OUT  LPWSABUF lpQOSName,
                OUT     LPQOS lpQOS)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return FALSE;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAHtonl(IN  SOCKET s,
         IN  ULONG hostLONG,
         OUT ULONG FAR* lpnetlong)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAHtons(IN  SOCKET s,
         IN  USHORT hostshort,
         OUT USHORT FAR* lpnetshort)
{
    PCATALOG_ENTRY provider;
    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }

    if (!ReferenceProviderByHandle((HANDLE)s, &provider))
    {
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }

    switch (provider->ProtocolInfo.iNetworkByteOrder)
    {
    case BIGENDIAN:
        *lpnetshort = htons(hostshort);
        break;
    case LITTLEENDIAN:
#ifdef LE
        *lpnetshort = hostshort;
#else
        *lpnetshort = (((hostshort & 0xFF00) >> 8) | ((hostshort & 0x00FF) << 8));
#endif
        break;
    }
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
WSAJoinLeaf(IN  SOCKET s,
            IN  CONST struct sockaddr *name,
            IN  INT namelen,
            IN  LPWSABUF lpCallerData,
            OUT LPWSABUF lpCalleeData,
            IN  LPQOS lpSQOS,
            IN  LPQOS lpGQOS,
            IN  DWORD dwFlags)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSANtohl(IN  SOCKET s,
         IN  ULONG netlong,
         OUT ULONG FAR* lphostlong)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSANtohs(IN  SOCKET s,
         IN  USHORT netshort,
         OUT USHORT FAR* lphostshort)
{
    PCATALOG_ENTRY provider;
    if (!WSAINITIALIZED)
    {
        WSASetLastError(WSANOTINITIALISED);
        return SOCKET_ERROR;
    }

    if (!ReferenceProviderByHandle((HANDLE)s, &provider))
    {
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }

    switch (provider->ProtocolInfo.iNetworkByteOrder)
    {
    case BIGENDIAN:
        *lphostshort = ntohs(netshort);
        break;
    case LITTLEENDIAN:
#ifdef LE
        *lphostshort = netshort;
#else
        *lphostshort = (((netshort & 0xFF00) >> 8) | ((netshort & 0x00FF) << 8));
#endif
        break;
    }
    return 0;
}


/*
 * @unimplemented
 */
FARPROC
EXPORT
WSASetBlockingHook(IN  FARPROC lpBlockFunc)
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
WSAProviderConfigChange(IN OUT  LPHANDLE lpNotificationHandle,
                        IN      LPWSAOVERLAPPED lpOverlapped,
                        IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSACancelAsyncRequest(IN  HANDLE hAsyncTaskHandle)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}

/* WinSock Service Provider support functions */

/*
 * @unimplemented
 */
INT
EXPORT
WPUCompleteOverlappedRequest(IN  SOCKET s,
                             IN  LPWSAOVERLAPPED lpOverlapped,
                             IN  DWORD dwError,
                             IN  DWORD cbTransferred,
                             OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCDeinstallProvider(IN  LPGUID lpProviderId,
                     OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCEnumProtocols(IN      LPINT lpiProtocols,
                 OUT     LPWSAPROTOCOL_INFOW lpProtocolBuffer,
                 IN OUT  LPDWORD lpdwBufferLength,
                 OUT     LPINT lpErrno)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCGetProviderPath(IN      LPGUID lpProviderId,
                   OUT     LPWSTR lpszProviderDllPath,
                   IN OUT  LPINT lpProviderDllPathLen,
                   OUT     LPINT lpErrno)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCInstallProvider(IN  LPGUID lpProviderId,
                   IN  CONST WCHAR* lpszProviderDllPath,
                   IN  CONST LPWSAPROTOCOL_INFOW lpProtocolInfoList,
                   IN  DWORD dwNumberOfEntries,
                   OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCEnableNSProvider(IN  LPGUID lpProviderId,
                    IN  BOOL fEnable)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCInstallNameSpace(IN  LPWSTR lpszIdentifier,
                    IN  LPWSTR lpszPathName,
                    IN  DWORD dwNameSpace,
                    IN  DWORD dwVersion,
                    IN  LPGUID lpProviderId)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCUnInstallNameSpace(IN  LPGUID lpProviderId)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCWriteProviderOrder(IN  LPDWORD lpwdCatalogEntryId,
                      IN  DWORD dwNumberOfEntries)
{
    UNIMPLEMENTED

    return WSASYSCALLFAILURE;
}

/*
 * @unimplemented
 */
INT
EXPORT
WSANSPIoctl(HANDLE           hLookup,
            DWORD            dwControlCode,
            LPVOID           lpvInBuffer,
            DWORD            cbInBuffer,
            LPVOID           lpvOutBuffer,
            DWORD            cbOutBuffer,
            LPDWORD          lpcbBytesReturned,
            LPWSACOMPLETION  lpCompletion)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSCUpdateProvider(LPGUID lpProviderId,
                  const WCHAR FAR * lpszProviderDllPath,
                  const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
                  DWORD dwNumberOfEntries,
                  LPINT lpErrno)
{
    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
EXPORT
WSCWriteNameSpaceOrder(LPGUID lpProviderId,
                       DWORD dwNumberOfEntries)
{
    UNIMPLEMENTED

    return WSASYSCALLFAILURE;
}

/*
 * @unimplemented
 */
INT
EXPORT
getnameinfo(const struct sockaddr FAR * sa,
            socklen_t       salen,
            char FAR *      host,
            DWORD           hostlen,
            char FAR *      serv,
            DWORD           servlen,
            INT             flags)
{
    if (!host && serv && flags & NI_NUMERICSERV)
    {
        const struct sockaddr_in *sa_in = (const struct sockaddr_in *)sa;
        if (salen >= sizeof(*sa_in) && sa->sa_family == AF_INET)
        {
            StringCbPrintfA(serv, servlen, "%u", sa_in->sin_port);
            return 0;
        }
    }

    UNIMPLEMENTED

    WSASetLastError(WSASYSCALLFAILURE);
    return SOCKET_ERROR;
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

    return FALSE;
}

/*
 * @unimplemented
 */
INT
EXPORT
GetAddrInfoW(IN PCWSTR pszNodeName,
             IN PCWSTR pszServiceName,
             IN const ADDRINFOW *ptHints,
             OUT PADDRINFOW *pptResult)
{
    UNIMPLEMENTED

    WSASetLastError(EAI_FAIL);
    return EAI_FAIL;
}

/*
 * @unimplemented
 */
INT
EXPORT
GetNameInfoW(IN CONST SOCKADDR *pSockaddr,
             IN socklen_t SockaddrLength,
             OUT PWCHAR pNodeBuffer,
             IN DWORD NodeBufferSize,
             OUT PWCHAR pServiceBuffer,
             IN DWORD ServiceBufferSize,
             IN INT Flags)
{
    UNIMPLEMENTED

    WSASetLastError(EAI_FAIL);
    return EAI_FAIL;
}

/* EOF */
