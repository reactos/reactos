/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            dll/win32/netapi32/browser.c
 * PURPOSE:         Computer Browser service interface code
 * PROGRAMMERS:     Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"

#include <rpc.h>
#include <lmbrowsr.h>
#include "browser_c.h"


WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
BROWSER_IDENTIFY_HANDLE_bind(BROWSER_IDENTIFY_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("BROWSER_IDENTIFY_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\browser",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
BROWSER_IDENTIFY_HANDLE_unbind(BROWSER_IDENTIFY_HANDLE pszSystemName,
                               handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("BROWSER_IDENTIFY_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NET_API_STATUS
WINAPI
I_BrowserQueryEmulatedDomains(
    _In_opt_ LPWSTR ServerName,
    _Out_ PBROWSER_EMULATED_DOMAIN *EmulatedDomains,
    _Out_ LPDWORD EntriesRead)
{
    FIXME("I_BrowserQueryEmulatedDomains(%s %p %p)\n",
          debugstr_w(ServerName), EmulatedDomains, EntriesRead);

    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
I_BrowserSetNetlogonState(
    _In_ LPWSTR ServerName,
    _In_ LPWSTR DomainName,
    _In_ LPWSTR EmulatedServerName,
    _In_ DWORD Role)
{
    FIXME("I_BrowserSetNetlogonState(%s %s %s %lu)\n",
          debugstr_w(ServerName), debugstr_w(ServerName),
          debugstr_w(EmulatedServerName), Role);

    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetServerEnum(
    _In_opt_ LMCSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _In_ DWORD servertype,
    _In_opt_ LMCSTR domain,
    _Inout_opt_ LPDWORD resume_handle)
{
    FIXME("NetServerEnum(%s %lu %p %lu %p %p %lu %s %p)\n",
          debugstr_w(servername), level, bufptr, prefmaxlen, entriesread,
          totalentries, servertype, debugstr_w(domain), resume_handle);

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}


NET_API_STATUS
WINAPI
NetServerEnumEx(
    _In_opt_ LMCSTR ServerName,
    _In_ DWORD Level,
    _Out_ LPBYTE *Bufptr,
    _In_ DWORD PrefMaxlen,
    _Out_ LPDWORD EntriesRead,
    _Out_ LPDWORD totalentries,
    _In_ DWORD servertype,
    _In_opt_ LMCSTR domain,
    _In_opt_ LMCSTR FirstNameToReturn)
{
    FIXME("NetServerEnumEx(%s %lu %p %lu %p %p %lu %s %s)\n",
           debugstr_w(ServerName), Level, Bufptr, PrefMaxlen, EntriesRead, totalentries,
           servertype, debugstr_w(domain), debugstr_w(FirstNameToReturn));

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

/* EOF */
