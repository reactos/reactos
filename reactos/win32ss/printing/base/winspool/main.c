/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Global Variables
HANDLE hProcessHeap;


handle_t __RPC_USER
WINSPOOL_HANDLE_bind(WINSPOOL_HANDLE wszName)
{
    handle_t hBinding;
    PWSTR wszStringBinding;
    RPC_STATUS Status;

    // Get us a string binding handle from the supplied connection information
    Status = RpcStringBindingComposeW(NULL, L"ncalrpc", NULL, L"spoolss", NULL, &wszStringBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcStringBindingComposeW failed with status %ld!\n", Status);
        return NULL;
    }

    // Get a handle_t binding handle from the string binding handle
    Status = RpcBindingFromStringBindingW(wszStringBinding, &hBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcBindingFromStringBindingW failed with status %ld!\n", Status);
        return NULL;
    }

    // Free the string binding handle
    Status = RpcStringFreeW(&wszStringBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcStringFreeW failed with status %ld!\n", Status);
        return NULL;
    }

    return hBinding;
}

void __RPC_USER
WINSPOOL_HANDLE_unbind(WINSPOOL_HANDLE wszName, handle_t hBinding)
{
    RPC_STATUS Status;

    Status = RpcBindingFree(&hBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcBindingFree failed with status %ld!\n", Status);
    }
}

void __RPC_FAR* __RPC_USER
midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(hProcessHeap, HEAP_ZERO_MEMORY, len);
}

void __RPC_USER
midl_user_free(void __RPC_FAR* ptr)
{
    HeapFree(hProcessHeap, 0, ptr);
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hProcessHeap = GetProcessHeap();
            break;
    }

    return TRUE;
}

BOOL WINAPI
SpoolerInit()
{
    BOOL bReturnValue = FALSE;
    DWORD dwErrorCode;

    // Nothing to initialize here yet, but pass this call to the Spool Service as well.
    RpcTryExcept
    {
        dwErrorCode = _RpcSpoolerInit();
        SetLastError(dwErrorCode);
        bReturnValue = (dwErrorCode == ERROR_SUCCESS);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("_RpcSpoolerInit failed with exception code %lu!\n", RpcExceptionCode());
    }
    RpcEndExcept;

    return bReturnValue;
}
