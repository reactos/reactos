// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Helper for loading DWMApi.dll and calling methods
//
//------------------------------------------------------------------------------

#pragma once

#include "osversionhelper.h"

#if defined(DWMAPI)
   #undef DWMAPI
#endif

//+-----------------------------------------------------------------------------
//
//  Namespace: DWMAPI
//
//  Description: Local helper "class" to ensure that dwmapi.dll is loaded once
//               and unloaded only when milcore is unloaded. 
//
//               NOTE: It is critical that we unload dwmapi.dll only when 
//               milcore is unloaded, because otherwise it will result in LPC 
//               port disconnect which will lead to premature closing of the 
//               graphics stream. 
//
//------------------------------------------------------------------------------
namespace DWMAPI
{
    HRESULT Load();
    
    FARPROC GetProcAddress(__in PCSTR pProcName);

    bool CheckOS();

    HRESULT DwmIsCompositionEnabled(__out_ecount(1) BOOL *pfEnabled);

    inline bool IsWindows8OrGreater()
    {
        static auto fIsWindows8OrGreater = WPFUtils::OSVersionHelper::IsWindows8OrGreater(); 

        return fIsWindows8OrGreater;
    }

    inline HRESULT OSCheckedIsCompositionEnabled(BOOL *pfEnabled)
    {
        HRESULT hr = S_OK;

        if (IsWindows8OrGreater())
        {
            *pfEnabled = TRUE;
        }
        else
        {
            hr = DwmIsCompositionEnabled(pfEnabled);
        }

        return hr;
    }
}


