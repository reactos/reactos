/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/audsrv/rpc.c
 * PURPOSE:          Audio Server
 * COPYRIGHT:        Copyright 2011 Neeraj Yadav
  */

#include "audsrv.h"

LIST_ENTRY LogHandleListHead;

/*RPC Listener Thread,Returns values less than 0 in failures*/
DWORD WINAPI RunRPCThread(LPVOID lpParameter)
{
    RPC_STATUS Status;

    InitializeListHead(&LogHandleListHead);

    Status = RpcServerUseProtseqEp(L"ncacn_np",
                                   20,
                                   L"\\pipe\\audsrv",
                                   NULL);
    if (Status != RPC_S_OK)
    {
        return -1;
    }

    Status = RpcServerRegisterIfEx(audsrv_v0_0_s_ifspec,
                                   NULL,
                                   NULL,
                                   0,
                                   RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                                   NULL );
    if (Status != RPC_S_OK)
    {
        return -2;
    }

    Status = RpcServerListen(1,
                             20,
                             FALSE);

    if (Status != RPC_S_OK)
    {
        return -3;
    }

    return 0;
}

/*************************RPC Functions**********************************/

long AUDInitStream(    IN RPC_BINDING_HANDLE hBinding,
                       LONG frequency,
                       int channels,
                       int bitspersample,
                       int datatype, 
                       ULONG channelmask,
                       int volume,
                       int mute,
                       float balance )
{
    long stream;
    
    stream = AddStream(frequency,
                       channels,
                       bitspersample,
                       datatype,
                       channelmask,
                       volume,
                       mute,
                       balance);

    if( stream != 0 )
    {
        /*ERROR*/
    }
    return stream;
}

long AUDPlayBuffer(    IN RPC_BINDING_HANDLE hBinding,
                       LONG streamid,
					   LONG length,
					   char* buffer)
{
    WriteBuffer(streamid,length,buffer);
    return 0;
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
