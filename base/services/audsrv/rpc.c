/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/audsrv/rpc.c
 * PURPOSE:          Audio Server
 * COPYRIGHT:        Copyright 2011 Pankaj Yadav
  */

/* INCLUDES *****************************************************************/

#include "audsrv.h"

LIST_ENTRY LogHandleListHead;

/* FUNCTIONS ****************************************************************/

DWORD WINAPI RunRPCThread(LPVOID lpParameter)
{
    RPC_STATUS Status;

    InitializeListHead(&LogHandleListHead);
printf("reached\n");
    Status = RpcServerUseProtseqEpW(L"ncacn_np", 20, L"\\pipe\\EventLog", NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(audsrv_v0_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerListen() failed (Status %lx)\n", Status);
    }

    return 0;
}


/*************************RPC Functions**********************************/

NTSTATUS AUDInitStream(	AUDSRV_HANDLE *streamthread)
{
    UNIMPLEMENTED;/*Coolest Macro I have ever seen*/
    return STATUS_NOT_IMPLEMENTED;
}
/*************************************************************************/
void __RPC_FAR *__RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


void __RPC_USER IELF_HANDLE_rundown(AUDSRV_HANDLE LogHandle)
{
}
