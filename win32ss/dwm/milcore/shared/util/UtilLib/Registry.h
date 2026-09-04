// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description: Simple registry utilities
//
//------------------------------------------------------------------------
#pragma once

bool RegGetDword(
    const HKEY hKey,
    const LPCTSTR pszValueName, 
    __out_ecount(1) LPDWORD pdwValue
    );
bool RegGetHKLMDword(
    const LPCTSTR pszKeyName, 
    const LPCTSTR pszValueName, 
    __out_ecount(1) LPDWORD pdwValue
    );

bool RegGetString(
    const HKEY hKey,
    const LPCTSTR pszValueName, 
    __out_bcount(cbValue) LPTSTR pszValue,
    DWORD cbValue
    );


bool RegGetHKLMString(
    const LPCTSTR pszKeyName, 
    const LPCTSTR pszValueName, 
    __out_bcount(cbValue) LPTSTR pszValue,
    DWORD cbValue
    );


