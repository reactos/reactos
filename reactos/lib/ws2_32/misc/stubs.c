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

SOCKET
EXPORT
accept(
    IN  SOCKET s,
    OUT LPSOCKADDR addr,
    OUT INT FAR* addrlen)
{
    UNIMPLEMENTED

    return INVALID_SOCKET;
}

INT
EXPORT
bind(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
connect(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
getpeername(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  INT FAR* namelen)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
getsockname(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  INT FAR* namelen)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
getsockopt(
    IN      SOCKET s,
    IN      INT level,
    IN      INT optname,
    OUT     CHAR FAR* optval,
    IN OUT  INT FAR* optlen)
{
    UNIMPLEMENTED

    return 0;
}

ULONG
EXPORT
htonl(
    IN  ULONG hostlong)
{
    UNIMPLEMENTED

    return 0;
}

USHORT
EXPORT
htons(
    IN  USHORT hostshort)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
ioctlsocket(
    IN      SOCKET s,
    IN      LONG cmd,
    IN OUT  ULONG FAR* argp)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
listen(
    IN  SOCKET s,
    IN  INT backlog)
{
    UNIMPLEMENTED

    return 0;
}

ULONG
EXPORT
ntohl(
    IN  ULONG netlong)
{
    UNIMPLEMENTED

    return 0;
}

USHORT
EXPORT
ntohs(
    IN  USHORT netshort)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
setsockopt(
    IN  SOCKET s,
    IN  INT level,
    IN  INT optname,
    IN  CONST CHAR FAR* optval,
    IN  INT optlen)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
shutdown(
    IN  SOCKET s,
    IN  INT how)
{
    UNIMPLEMENTED

    return 0;
}

SOCKET
EXPORT
socket(
    IN  INT af,
    IN  INT type,
    IN  INT protocol)
{
    UNIMPLEMENTED

    return INVALID_SOCKET;
}

SOCKET
EXPORT
WSAAccept(
    IN      SOCKET s,
    OUT     LPSOCKADDR addr,
    IN OUT  LPINT addrlen,
    IN      LPCONDITIONPROC lpfnCondition,
    IN      DWORD dwCallbackData)
{
    UNIMPLEMENTED

    return INVALID_SOCKET;
}

INT
EXPORT
WSAAsyncSelect(
    IN  SOCKET s,
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  LONG lEvent)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WSACancelBlockingCall(VOID)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WSAConnect(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WSADuplicateSocketA(
    IN  SOCKET s,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOA lpProtocolInfo)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WSADuplicateSocketW(
    IN  SOCKET s,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WSAEnumNetworkEvents(
    IN  SOCKET s,
    IN  WSAEVENT hEventObject,
    OUT LPWSANETWORKEVENTS lpNetworkEvents)
{
    UNIMPLEMENTED

    return 0;
}

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

INT
EXPORT
WSAEventSelect(
    IN  SOCKET s,
    IN  WSAEVENT hEventObject,
    IN  LONG lNetworkEvents)
{
    UNIMPLEMENTED

    return 0;
}

BOOL
EXPORT
WSAGetOverlappedResult(
    IN  SOCKET s,
    IN  LPWSAOVERLAPPED lpOverlapped,
    OUT LPDWORD lpcbTransfer,
    IN  BOOL fWait,
    OUT LPDWORD lpdwFlags)
{
    UNIMPLEMENTED

    return 0;
}

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

INT
EXPORT
WSAIoctl(
    IN  SOCKET s,
    IN  DWORD dwIoControlCode,
    IN  LPVOID lpvInBuffer,
    IN  DWORD cbInBuffer,
    OUT LPVOID lpvOutBuffer,
    IN  DWORD cbOutBuffer,
    OUT LPDWORD lpcbBytesReturned,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    UNIMPLEMENTED

    return 0;
}

BOOL
EXPORT
WSAIsBlocking(VOID)
{
    UNIMPLEMENTED

    return FALSE;
}

SOCKET
EXPORT
WSAJoinLeaf(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
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

FARPROC
EXPORT
WSASetBlockingHook(
    IN  FARPROC lpBlockFunc)
{
    UNIMPLEMENTED

    return (FARPROC)0;
}

INT
EXPORT
WSAUnhookBlockingHook(VOID)
{
    UNIMPLEMENTED

    return 0;
}

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

INT
EXPORT
WSACancelAsyncRequest(
    IN  HANDLE hAsyncTaskHandle)
{
    UNIMPLEMENTED

    return 0;
}

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

INT
EXPORT
WSCDeinstallProvider(
    IN  LPGUID lpProviderId,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}

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

INT
EXPORT
WSCEnableNSProvider(
    IN  LPGUID lpProviderId,
    IN  BOOL fEnable)
{
    UNIMPLEMENTED

    return 0;
}

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

INT
EXPORT
WSCUnInstallNameSpace(
    IN  LPGUID lpProviderId)
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WSCWriteProviderOrder(
    IN  LPDWORD lpwdCatalogEntryId,
    IN  DWORD dwNumberOfEntries)
{
    UNIMPLEMENTED

    return 0;
}

/* EOF */
