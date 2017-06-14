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
NetServerEnum(
    LMCSTR servername,
    DWORD level,
    LPBYTE *bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    DWORD servertype,
    LMCSTR domain,
    LPDWORD resume_handle)
{
    FIXME("NetServerEnum(%s %lu %p %lu %p %p %lu %s %p)\n",
          debugstr_w(servername), level, bufptr, prefmaxlen, entriesread,
          totalentries, servertype, debugstr_w(domain), resume_handle);

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}


NET_API_STATUS
WINAPI
NetServerEnumEx(
    LMCSTR ServerName,
    DWORD Level,
    LPBYTE *Bufptr,
    DWORD PrefMaxlen,
    LPDWORD EntriesRead,
    LPDWORD totalentries,
    DWORD servertype,
    LMCSTR domain,
    LMCSTR FirstNameToReturn)
{
    FIXME("NetServerEnumEx(%s %lu %p %lu %p %p %lu %s %s)\n",
           debugstr_w(ServerName), Level, Bufptr, PrefMaxlen, EntriesRead, totalentries,
           servertype, debugstr_w(domain), debugstr_w(FirstNameToReturn));

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

/* EOF */
