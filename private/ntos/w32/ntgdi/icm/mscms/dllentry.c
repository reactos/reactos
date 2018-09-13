/****************************Module*Header******************************\
* Module Name: DLLENTRY.C
*
* Module Descripton: This file has dll management functions and global
*   variables used by MSCMS.DLL
*
* Warnings:
*
* Issues:
*
* Public Routines:
*
* Created:  18 March 1996
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include "mscms.h"

BOOL WINAPI
DllEntryPoint(
    HINSTANCE hInstance,
    DWORD     fdwReason,
    LPVOID    lpvReserved
    )
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        //
        // Not nessesary to call me for DLL_THREAD_ATTACH and DLL_THREAD_DETACH
        //
        DisableThreadLibraryCalls(hInstance);
        InitializeCriticalSection(&critsec);
        break;

    case DLL_PROCESS_DETACH:
        DeleteCriticalSection(&critsec);
        break;

    default:
        break;
    }

    return(TRUE);
}
