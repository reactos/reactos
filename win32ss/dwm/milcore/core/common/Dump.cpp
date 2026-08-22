// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+------------------------------------------------------------------------------------------------
//
//  Microsoft

//
//
//  Contents:  Debug dumping utilities
//
//-------------------------------------------------------------------------------------------------
#include "precomp.hpp"

#include <strsafe.h>

#if DBG

#define MAX_DUMP_SIZE 500

void _cdecl MILDebugOutput(__in PCWSTR pFormat, ...)
{
    WCHAR buffer[MAX_DUMP_SIZE];
    va_list arglist;
    
    va_start(arglist, pFormat);  
    // Debug spew -- ignore errors.
    IGNORE_HR(StringCchVPrintfW(buffer, MAX_DUMP_SIZE, pFormat, arglist)); 
    va_end(arglist);

    OutputDebugString(buffer);
}

#endif // DBG


