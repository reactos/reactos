// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      Contains SW Startup routine implementations
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

bool g_fUseMMX = false;
bool g_fUseSSE2 = false;

//+-----------------------------------------------------------------------------
//
//  Function:
//      SwStartup
//
//  Synopsis:
//      Initialize commen data needed by SW code
//
//------------------------------------------------------------------------------
HRESULT
SwStartup()
{
    HRESULT hr = S_OK;
    DWORD dwDisableMMX = 0;
    DWORD dwDisableSSE2 = 0;

#if PRERELEASE
    HKEY hKeyAvalonGraphics = NULL;

    LONG r = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        _T("Software\\Microsoft\\Avalon.Graphics"),
        0,
        KEY_QUERY_VALUE,
        &hKeyAvalonGraphics
        );

    if (r == ERROR_SUCCESS)
    {
        DWORD dwValue = 0;
        DWORD dwDataSize = sizeof(dwValue);

        LONG r = RegQueryValueEx(
            hKeyAvalonGraphics,
            _T("DisableMMXForSwRast"),
            NULL,
            NULL,
            (LPBYTE)&dwValue,
            &dwDataSize
            );

        if (r == ERROR_SUCCESS && dwDataSize == sizeof(dwValue))
        {
            dwDisableMMX = dwValue;
        }

        dwDataSize = sizeof(dwValue);        

        r = RegQueryValueEx(
            hKeyAvalonGraphics,
            _T("DisableSSE2ForSwRast"),
            NULL,
            NULL,
            (LPBYTE)&dwValue,
            &dwDataSize
            );

        if (r == ERROR_SUCCESS && dwDataSize == sizeof(dwValue))
        {
            dwDisableSSE2 = dwValue;
        }

        RegCloseKey(hKeyAvalonGraphics);
    }
#endif

    if (dwDisableMMX == 0 && CCPUInfo::HasMMX())
    {
        g_fUseMMX = true;
    }

    if (dwDisableSSE2 == 0 && CCPUInfo::HasSSE2())
    {
        g_fUseSSE2 = true;
    }

    IFC(CMilShaderEffectDuce::InitializeJitterLock());

Cleanup:
    return hr;
}

void
SwShutdown()
{
    CMilShaderEffectDuce::DeInitializeJitterLock();
}



