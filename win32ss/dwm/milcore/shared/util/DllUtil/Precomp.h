// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       Precomp.h
//------------------------------------------------------------------------------
#include <wpfsdl.h>

#define AVALON_NOMEMSYSTEM
#include "always.h"

#ifdef __cplusplus
extern "C" {
#endif


// Ensure that DBG is defined when being compiled 
// under dbg/chk mode
#if !defined(DBG) && (defined(DEBUG) || defined(_DEBUG))
#define DBG 
#endif 

BOOL WINAPI _DllMainStartupImpl
(
    __in_ecount(1) HANDLE hDllHandle,
    DWORD dwReason,
    __in_ecount_opt(1) LPVOID lpreserved
);


#if !defined(DBG)

BOOL WINAPI _DllMainStartup
(
    __in_ecount(1) HANDLE hDllHandle,
    DWORD dwReason,
    __in_ecount_opt(1) LPVOID lpreserved
);

#else

extern __declspec(dllexport) BOOL g_fNoMeterChecks;

void WINAPI InitDebugLib(
        __in_ecount_opt(1) HANDLE,
        __in_ecount_opt(1) BOOL(WINAPI *)(HANDLE, DWORD, LPVOID),
        BOOL fExe
);

void TermDebugLib(__in_ecount(1) HANDLE, BOOL);

BOOL WINAPI _DllMainStartupDebug
(
    __in_ecount(1) HANDLE hDllHandle,
    DWORD dwReason,
    __in_ecount_opt(1) LPVOID lpreserved
);

#endif 

extern int avalonutil_proc_attached;
extern BOOL g_fAlwaysDetach;

extern BOOL WINAPI _DllMainCRTStartup(
    __in_ecount(1) HANDLE hDllHandle,
    DWORD dwReason,
    __in_ecount_opt(1) LPVOID lpreserved
    );

extern HRESULT AvCreateProcessHeap();
extern HRESULT AvDestroyProcessHeap();

#ifdef __cplusplus
}
#endif


