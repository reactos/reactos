/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            dll/win32/netapi32/netlogon.c
 * PURPOSE:         Netlogon service interface code
 * PROGRAMMERS:     Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

NTSTATUS
WINAPI
NetEnumerateTrustedDomains(
    _In_ LPWSTR ServerName,
    _Out_ LPWSTR *DomainNames)
{
    FIXME("NetEnumerateTrustedDomains(%s, %p)\n",
          debugstr_w(ServerName), DomainNames);

    return STATUS_NOT_IMPLEMENTED;
}


NET_API_STATUS
WINAPI
NetGetAnyDCName(
    _In_ LPCWSTR servername,
    _In_ LPCWSTR domainname,
    _Out_ LPBYTE *bufptr)
{
    FIXME("NetGetAnyDCName(%s, %s, %p)\n",
          debugstr_w(servername), debugstr_w(domainname), bufptr);

    return ERROR_NO_LOGON_SERVERS;
}


NET_API_STATUS
WINAPI
NetGetDCName(
    _In_ LPCWSTR servername,
    _In_ LPCWSTR domainname,
    _Out_ LPBYTE *bufptr)
{
    FIXME("NetGetDCName(%s, %s, %p)\n",
          debugstr_w(servername), debugstr_w(domainname), bufptr);

    return NERR_DCNotFound;
}

/* EOF */
