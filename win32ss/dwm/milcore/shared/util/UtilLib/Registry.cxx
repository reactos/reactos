// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description: Simple registry utilities
//
//------------------------------------------------------------------------
#include "pch.h"

//+-----------------------------------------------------------------------
//
//  Member:     RegGetDword 
//
//  Synopsis:   Reads a REG_DWORD from the registy
//
//  Returns:    true if the value was read
//
//------------------------------------------------------------------------

bool RegGetDword(
    HKEY hKey,
    const LPCTSTR pszValueName, 
    __out_ecount(1) LPDWORD pdwValue
    )
{
    DWORD dwValue;
    Assert(pszValueName);
    Assert(pdwValue);

    bool fRead = false;
    DWORD cbValue = sizeof(*pdwValue);
    DWORD dwType;
    
    if (hKey && 
        (RegQueryValueEx(
            hKey,
            pszValueName,
            NULL,
            &dwType,
            reinterpret_cast<LPBYTE>(&dwValue),
            &cbValue) == ERROR_SUCCESS))
    {
        fRead = (dwType == REG_DWORD);
        *pdwValue = dwValue;
    }
    
    return fRead;
}
//+-----------------------------------------------------------------------
//
//  Member:     RegGetHKLMDword 
//
//  Synopsis:   Reads a REG_DWORD from the registy
//
//  Returns:    true if the value was read
//
//------------------------------------------------------------------------

bool RegGetHKLMDword(
    const LPCTSTR pszKeyName, 
    const LPCTSTR pszValueName, 
    __out_ecount(1) LPDWORD pdwValue
    )
{
    Assert(pszKeyName);
    Assert(pszValueName);
    Assert(pdwValue);

    bool fRead = false;
    HKEY hKey  = NULL;

    if (RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, 
        pszKeyName,
        NULL, 
        KEY_READ,
        &hKey) == ERROR_SUCCESS)
    {
        fRead = RegGetDword(hKey,
                            pszValueName,
                            pdwValue);
        RegCloseKey(hKey);
    }
    
    return fRead;
}
 
//+-----------------------------------------------------------------------
//
//  Member:     RegGetString 
//
//  Synopsis:   Reads an REG_SZ from the registy
//
//  Returns:    true if the value was read
//
//------------------------------------------------------------------------

bool RegGetString(
    HKEY hKey,
    const LPCTSTR pszValueName, 
    __out_bcount(cbValue) LPTSTR pszValue,
    DWORD cbValue
    )
{
    Assert(pszValueName);
    Assert(pszValue);

    bool fRead = false;
    DWORD dwType;
 
    if (hKey &&
        (RegQueryValueEx(
            hKey,
            pszValueName,
            NULL,
            &dwType,
            reinterpret_cast<LPBYTE>(pszValue),
            &cbValue) == ERROR_SUCCESS))
    {
        fRead = (dwType == REG_SZ);
    }
    
    return fRead;
}
//+-----------------------------------------------------------------------
//
//  Member:     RegGetHKLMString 
//
//  Synopsis:   Reads an REG_SZ from the registy
//
//  Returns:    true if the value was read
//
//------------------------------------------------------------------------

bool RegGetHKLMString(
    const LPCTSTR pszKeyName, 
    const LPCTSTR pszValueName, 
    __out_bcount(cbValue) LPTSTR pszValue,
    DWORD cbValue
    )
{
    Assert(pszKeyName);
    Assert(pszValueName);
    Assert(pszValue);

    bool fRead = false;
    HKEY hKey  = NULL;

    if (RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, 
        pszKeyName,
        NULL, 
        KEY_READ,
        &hKey) == ERROR_SUCCESS)
    {
        fRead = RegGetString(
            hKey,
            pszValueName,
            pszValue,
            cbValue);
        
        RegCloseKey(hKey);
    }
    
    return fRead;
}


