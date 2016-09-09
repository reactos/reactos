/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            reactos/dll/win32/netapi32/schedule.c
 * PURPOSE:         Server service interface code
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"
#include "srvsvc_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
SRVSVC_HANDLE_bind(SRVSVC_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("SRVSVC_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      (RPC_WSTR)pszSystemName,
                                      L"\\pipe\\srvsvc",
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
SRVSVC_HANDLE_unbind(SRVSVC_HANDLE pszSystemName,
                     handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("SRVSVC_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NET_API_STATUS
WINAPI
NetRemoteTOD(
    LPCWSTR UncServerName,
    LPBYTE *BufferPtr)
{
    NET_API_STATUS status;

    TRACE("NetRemoteTOD(%s, %p)\n", debugstr_w(UncServerName),
          BufferPtr);

    *BufferPtr = NULL;

    RpcTryExcept
    {
        status = NetrRemoteTOD((SRVSVC_HANDLE)UncServerName,
                               (LPTIME_OF_DAY_INFO *)BufferPtr);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}

/* EOF */
