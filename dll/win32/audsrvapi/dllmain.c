/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             dll\win32\audsrvapi\dllmain.c
 * PURPOSE:          Audio Server
 * COPYRIGHT:        Copyright 2011 Neeraj Yadav

 */

#include "audsrvapi.h"


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{   
    RPC_STATUS status;
    unsigned short * pszStringBinding    = NULL;
    
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        status = RpcStringBindingComposeW(NULL,
                                          L"ncacn_np",
                                          NULL,
                                          L"\\pipe\\audsrv",
                                          NULL,
                                          &pszStringBinding);

        status = RpcBindingFromStringBindingW(pszStringBinding,
                                              &audsrv_v0_0_c_ifspec);

        if (status)
        {
            /*Connection Problem*/
        }

        status = RpcStringFree(&pszStringBinding);

        if (status)
        {
            /*problem*/
        }
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        status = RpcBindingFree(audsrv_v0_0_c_ifspec);
         if (status == RPC_S_INVALID_BINDING)
            OutputDebugStringA("Error Closing RPC Connection\n");
    break;
    }
    return TRUE;
}
