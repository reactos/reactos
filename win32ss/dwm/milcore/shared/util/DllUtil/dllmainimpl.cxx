// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       dllmainimpl.cxx
//------------------------------------------------------------------------------
#include "Precomp.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI _DllMainStartupImpl
(
    __in_ecount(1) HANDLE hDllHandle,
    DWORD dwReason,
    __in_ecount_opt(1) LPVOID lpreserved
)
{
    BOOL retcode = TRUE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        avalonutil_proc_attached++;

        if (!retcode)
            break;


        if (FAILED(AvCreateProcessHeap())) {
            retcode = FALSE;
            break;
        }

#if defined(DBG)
        // DLL_MAIN_PRE_CINIT
        g_fNoMeterChecks = TRUE;
        InitDebugLib(hDllHandle, _DllMainStartupDebug, FALSE);
#endif 

        // Initialize the CRT and have it call into our DllMain for us

        retcode = _DllMainCRTStartup(hDllHandle, dwReason, lpreserved);

#if defined(DBG)
        // DLL_MAIN_POST_CINIT
        g_fNoMeterChecks = FALSE;
#endif 
        break;
    }

    case DLL_PROCESS_DETACH:
    {
        //
        // Check for process exit
        //
        //  If the process is being exited there is nothing to we need to
        //  do, but many things we are not allowed to do.
        //
        if (!g_fAlwaysDetach && lpreserved != NULL)
        {
            return TRUE;
        }

        if (avalonutil_proc_attached <= 0)
        {
            /*
            * no prior process attach notification. just return
            * without doing anything.
            */
            return FALSE;
        }

#if defined(DBG)
        // DLL_MAIN_PRE_CEXIT
        TermDebugLib(hDllHandle, FALSE);
#endif 

        avalonutil_proc_attached--;

        retcode = _DllMainCRTStartup(hDllHandle, dwReason, lpreserved);

#if defined(DBG)
        // DLL_MAIN_POST_CEXIT
        TermDebugLib(hDllHandle, TRUE);
#endif 

        if (FAILED(AvDestroyProcessHeap())) {
            retcode = FALSE;
        }

        break;
    }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    {
        retcode = _DllMainCRTStartup(hDllHandle, dwReason, lpreserved);
        break;
    }
    }

    return retcode;
}

#ifdef __cplusplus
}
#endif


