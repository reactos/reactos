/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/service/rpc.c
 * PURPOSE:         RPC support routines
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <advapi32.h>

static RPC_BINDING_HANDLE LocalBindingHandle = NULL;

RPC_STATUS
EvtBindRpc(LPCWSTR pszMachine,
           RPC_BINDING_HANDLE* BindingHandle)
{
    PWSTR pszStringBinding = NULL;
    RPC_STATUS Status;

    Status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      (LPWSTR)pszMachine,
                                      L"\\pipe\\EventLog",
                                      NULL,
                                      &pszStringBinding);
    if (Status != RPC_S_OK)
        return Status;

    Status = RpcBindingFromStringBindingW(pszStringBinding,
                                          BindingHandle);

    RpcStringFreeW(&pszStringBinding);

    return Status;
}

RPC_STATUS
EvtUnbindRpc(RPC_BINDING_HANDLE *BindingHandle)
{
    if (BindingHandle != NULL)
    {
        RpcBindingFree(*BindingHandle);
        *BindingHandle = NULL;
    }

    return RPC_S_OK;
}

BOOL
EvtGetLocalHandle(RPC_BINDING_HANDLE *BindingHandle)
{
    if (LocalBindingHandle != NULL)
    {
        if (BindingHandle != NULL)
            *BindingHandle = LocalBindingHandle;

        return TRUE;
    }

    if (EvtBindRpc(NULL, &LocalBindingHandle) != RPC_S_OK)
        return FALSE;

    if (BindingHandle != NULL)
        *BindingHandle = LocalBindingHandle;

    return TRUE;
}

RPC_STATUS
EvtUnbindLocalHandle(VOID)
{
    return EvtUnbindRpc(&LocalBindingHandle);
}

void __RPC_FAR * __RPC_USER
midl_user_allocate(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER
midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

/* EOF */
