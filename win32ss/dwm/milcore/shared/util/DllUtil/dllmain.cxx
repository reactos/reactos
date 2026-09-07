// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       dllmain.cxx
//------------------------------------------------------------------------------
#include "Precomp.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DBG)

BOOL WINAPI _DllMainStartup
(
    __in_ecount(1) HANDLE hDllHandle,
    DWORD dwReason,
    __in_ecount_opt(1) LPVOID lpreserved
)
{
	return _DllMainStartupImpl(hDllHandle, dwReason, lpreserved);
}

#else

__declspec(dllexport) BOOL g_fNoMeterChecks;

BOOL WINAPI _DllMainStartupDebug
(
    __in_ecount(1) HANDLE hDllHandle,
    DWORD dwReason,
    __in_ecount_opt(1) LPVOID lpreserved
)
{
	return _DllMainStartupImpl(hDllHandle, dwReason, lpreserved);
}

#endif 

#ifdef __cplusplus
}
#endif



