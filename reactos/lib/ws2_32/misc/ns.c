/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/ns.c
 * PURPOSE:     Namespace APIs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ws2_32.h>

/* Name resolution APIs */

INT
EXPORT
WSAAddressToStringA(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOA lpProtocolInfo,
    OUT     LPSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAAddressToStringW(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPWSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAEnumNameSpaceProvidersA(
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSANAMESPACE_INFOA lpnspBuffer)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAEnumNameSpaceProvidersW(
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSANAMESPACE_INFOW lpnspBuffer)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAGetServiceClassInfoA(
    IN      LPGUID lpProviderId,
    IN      LPGUID lpServiceClassId,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAGetServiceClassInfoW(
    IN      LPGUID lpProviderId,
    IN      LPGUID lpServiceClassId,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAGetServiceClassNameByClassIdA(
    IN      LPGUID lpServiceClassId,
    OUT     LPSTR lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAGetServiceClassNameByClassIdW(
    IN      LPGUID lpServiceClassId,
    OUT     LPWSTR lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAInstallServiceClassA(
    IN  LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAInstallServiceClassW(
    IN  LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSALookupServiceBeginA(
    IN  LPWSAQUERYSETA lpqsRestrictions,
    IN  DWORD dwControlFlags,
    OUT LPHANDLE lphLookup)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSALookupServiceBeginW(
    IN  LPWSAQUERYSETW lpqsRestrictions,
    IN  DWORD dwControlFlags,
    OUT LPHANDLE lphLookup)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSALookupServiceEnd(
    IN  HANDLE hLookup)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSALookupServiceNextA(
    IN      HANDLE hLookup,
    IN      DWORD dwControlFlags,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSAQUERYSETA lpqsResults)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSALookupServiceNextW(
    IN      HANDLE hLookup,
    IN      DWORD dwControlFlags,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSAQUERYSETW lpqsResults)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSARemoveServiceClass(
    IN  LPGUID lpServiceClassId)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSASetServiceA(
    IN  LPWSAQUERYSETA lpqsRegInfo,
    IN  WSAESETSERVICEOP essOperation,
    IN  DWORD dwControlFlags)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSASetServiceW(
    IN  LPWSAQUERYSETW lpqsRegInfo,
    IN  WSAESETSERVICEOP essOperation,
    IN  DWORD dwControlFlags)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAStringToAddressA(
    IN      LPSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOA lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength)
{
    UNIMPLEMENTED

    return 0;
}


INT
EXPORT
WSAStringToAddressW(
    IN      LPWSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength)
{
    UNIMPLEMENTED

    return 0;
}


/* WinSock 1.1 compatible name resolution APIs */

LPHOSTENT
EXPORT
gethostbyaddr(
    IN  CONST CHAR FAR* addr,
    IN  INT len,
    IN  INT type)
{
    UNIMPLEMENTED

    return (LPHOSTENT)NULL;
}

LPHOSTENT
EXPORT
gethostbyname(
    IN  CONST CHAR FAR* name)
{
    UNIMPLEMENTED

    return (LPHOSTENT)NULL;
}


INT
EXPORT
gethostname(
    OUT CHAR FAR* name,
    IN  INT namelen)
{
    UNIMPLEMENTED

    return 0;
}


LPPROTOENT
EXPORT
getprotobyname(
    IN  CONST CHAR FAR* name)
{
    UNIMPLEMENTED

    return (LPPROTOENT)NULL;
}


LPPROTOENT
EXPORT
getprotobynumber(
    IN  INT number)
{
    UNIMPLEMENTED

    return (LPPROTOENT)NULL;
}

LPSERVENT
EXPORT
getservbyname(
    IN  CONST CHAR FAR* name, 
    IN  CONST CHAR FAR* proto)
{
    UNIMPLEMENTED

    return (LPSERVENT)NULL;
}


LPSERVENT
EXPORT
getservbyport(
    IN  INT port, 
    IN  CONST CHAR FAR* proto)
{
    UNIMPLEMENTED

    return (LPSERVENT)NULL;
}


ULONG
EXPORT
inet_addr(
    IN  CONST CHAR FAR* cp)
/*
 * FUNCTION: Converts a string containing an IPv4 address to an unsigned long
 * ARGUMENTS:
 *     cp = Pointer to string with address to convert
 * RETURNS:
 *     Binary representation of IPv4 address, or INADDR_NONE
 */
{
    UINT i;
    PCHAR p;
    ULONG u = 0;

    /* FIXME: Little endian version only */

    p = (PCHAR)cp;

    if (strlen(p) == 0)
        return INADDR_NONE;

    if (strcmp(p, " ") == 0)
        return 0;

    for (i = 3; i >= 0; i--) {
        u += (strtoul(p, &p, 0) << (i * 8));

        if (strlen(p) == 0)
            return u;

        if (p[0] != '.')
            return INADDR_NONE;

        p++;
    }

    return u;
}


CHAR FAR*
EXPORT
inet_ntoa(
    IN  IN_ADDR in)
{
    CHAR b[10];
    PCHAR p;

    p = ((PWINSOCK_THREAD_BLOCK)NtCurrentTeb()->WinSockData)->Intoa;
    _itoa(in.S_un.S_addr >> 24, b, 10);
    strcpy(p, b);
    _itoa(in.S_un.S_addr >> 16, b, 10);
    strcat(p, b);
    _itoa(in.S_un.S_addr >> 8, b, 10);
    strcat(p, b);
    _itoa(in.S_un.S_addr & 0xFF, b, 10);
    strcat(p, b);
    return (CHAR FAR*)p;
}


HANDLE
EXPORT
WSAAsyncGetHostByAddr(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* addr, 
    IN  INT len,
    IN  INT type, 
    OUT CHAR FAR* buf, 
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


HANDLE
EXPORT
WSAAsyncGetHostByName(
    IN  HWND hWnd, 
    IN  UINT wMsg,  
    IN  CONST CHAR FAR* name, 
    OUT CHAR FAR* buf, 
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


HANDLE
EXPORT
WSAAsyncGetProtoByName(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* name,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


HANDLE
EXPORT
WSAAsyncGetProtoByNumber(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  INT number,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


HANDLE
EXPORT
WSAAsyncGetServByName(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* name,
    IN  CONST CHAR FAR* proto,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


HANDLE
EXPORT
WSAAsyncGetServByPort(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  INT port,
    IN  CONST CHAR FAR* proto,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}

/* EOF */
