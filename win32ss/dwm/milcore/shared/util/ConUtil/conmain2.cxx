// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       conmain2.cxx
//------------------------------------------------------------------------------
#include "Precomp.h"

extern "C" void InitDebugLib(
    __in_ecount_opt(1) HANDLE, 
    __in_ecount_opt(1) BOOL (WINAPI *)(HANDLE, DWORD, LPVOID), 
    BOOL fExe
    );

extern "C" void TermDebugLib(__in_ecount(1) HANDLE, BOOL);

#define CON_MAIN_FUNCTION_NAME  _ConMainStartupDebug
#define CON_MAIN_PRE_MAIN       { InitDebugLib(NULL, NULL, TRUE); }
#include "conmain.cxx"



