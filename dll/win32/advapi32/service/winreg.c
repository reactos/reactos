/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/service/winreg.c
 * PURPOSE:         Remote registry functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <advapi32.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);


/* FUNCTIONS *****************************************************************/


handle_t __RPC_USER
PREGISTRY_SERVER_NAME_bind(PREGISTRY_SERVER_NAME pszServerName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("PREGISTRY_SERVER_NAME_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszServerName,
                                      L"\\pipe\\winreg",
                                      NULL,
                                      &pszStringBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
PREGISTRY_SERVER_NAME_unbind(PREGISTRY_SERVER_NAME pszServerName,
                             handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("PREGISTRY_SERVER_NAME_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFree returned 0x%x\n", status);
    }
}

/* EOF */
