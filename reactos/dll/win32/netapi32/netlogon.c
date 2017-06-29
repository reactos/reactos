/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            dll/win32/netapi32/netlogon.c
 * PURPOSE:         Netlogon service interface code
 * PROGRAMMERS:     Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"
#include <rpc.h>
#include "netlogon_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
LOGONSRV_HANDLE_bind(LOGONSRV_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("LOGONSRV_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\netlogon",
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
LOGONSRV_HANDLE_unbind(LOGONSRV_HANDLE pszSystemName,
                       handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("LOGONSRV_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


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
    _In_opt_ LPCWSTR ServerName,
    _In_opt_ LPCWSTR DomainName,
    _Out_ LPBYTE *BufPtr)
{
    NET_API_STATUS status;

    TRACE("NetGetAnyDCName(%s, %s, %p)\n",
          debugstr_w(ServerName), debugstr_w(DomainName), BufPtr);

    *BufPtr = NULL;

    RpcTryExcept
    {
        status = NetrGetAnyDCName((PWSTR)ServerName,
                                  (PWSTR)DomainName,
                                  (PWSTR*)BufPtr);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
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
