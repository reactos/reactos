// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       Misc.cxx
//  Contents:   Useful helpers
//------------------------------------------------------------------------------
#include "Pch.h"

//------------------------------------------------------------------------------
//  Function:   GetLastWin32Error
//  Synopsis:   Returns the last Win32 error, converted to an HRESULT.
//  Returns:    HRESULT
//------------------------------------------------------------------------------
HRESULT
GetLastWin32Error( )
{
    // Win 95 can return 0, even when there's an error.
    DWORD dw = GetLastError();
    return dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
}



