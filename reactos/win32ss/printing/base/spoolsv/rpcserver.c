/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     RPC Server Thread
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD WINAPI
LrpcThreadProc(LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncalrpc", 20, L"spoolss", NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerUseProtseqEpW failed with status %ld!\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(winspool_v1_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerRegisterIf failed with status %ld!\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, 0);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerListen() failed with status %ld!\n", Status);
    }

    return 0;
}

void __RPC_FAR* __RPC_USER
midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

void __RPC_USER
midl_user_free(void __RPC_FAR* ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

void __RPC_USER
WINSPOOL_GDI_HANDLE_rundown(WINSPOOL_GDI_HANDLE hGdiHandle)
{
}

void __RPC_USER
WINSPOOL_PRINTER_HANDLE_rundown(WINSPOOL_PRINTER_HANDLE hPrinter)
{
}
