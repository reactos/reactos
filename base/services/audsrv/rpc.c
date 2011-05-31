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
    Status = RpcServerUseProtseqEp(L"ncacn_np", 20, L"\\pipe\\audsrv", NULL);
    if (Status != RPC_S_OK)
    {
        printf("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIfEx(audsrv_v0_0_s_ifspec, NULL, NULL, 0, RPC_C_LISTEN_MAX_CALLS_DEFAULT, NULL );
    if (Status != RPC_S_OK)
    {
        printf("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, 20, FALSE);
    if (Status != RPC_S_OK)
    {
        printf("RpcServerListen() failed (Status %lx)\n", Status);
    }

    return 0;
}


/*************************RPC Functions**********************************/

int AUDInitStream(	IN RPC_BINDING_HANDLE hBinding,LONG frequency,int channels,int bitspersample, ULONG channelmask,int volume,int mute,float balance)
{
	HANDLE stream;
	printf("Client Connected and Initiated Stream Freq: %ld,Channle: %d,Bitspersample: %d,Mask: %ld,Volume: %d,Mute: %d,Balance: %f\n",frequency,channels,bitspersample,channelmask,volume,mute,balance);
	stream = addstream(frequency,channels,bitspersample,channelmask,volume,mute,balance);
	if( stream == NULL ){return 0;}else{printf("Stream added\n");}
    return (int)stream;
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
