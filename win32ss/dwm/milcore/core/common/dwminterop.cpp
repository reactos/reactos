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

#include "precomp.hpp"

#include "osversionhelper.h"
#include "DelayLoadedProc.h"


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
    struct CDWMAPIModuleInfo
    {
        static const TCHAR sc_szFileName[];
        static HRESULT CheckLoadAvailability()
        {
            return CheckOS() ? S_OK : WGXERR_UNSUPPORTEDVERSION;
        }
    };

    const TCHAR CDWMAPIModuleInfo::sc_szFileName[] = _T("DWMAPI.dll");

    CDelayLoadedModule<CDWMAPIModuleInfo> m_Module;


    // Public Methods

    HRESULT Load()
    {
        return m_Module.Load();
    }

    FARPROC
    GetProcAddress(
        __in PCSTR pProcName
        )
    {
        return m_Module.GetProcAddress(pProcName);
    }

    bool CheckOS()
    {
        return WPFUtils::OSVersionHelper::IsWindowsVistaOrGreater();
    }


    #define DLP_MODULE_VARIABLE m_Module
    #define DLP_PFN_VARIABLE_PREFIX m_

    DELAY_LOAD_PROC(HRESULT, DwmIsCompositionEnabled, (__out_ecount(1) BOOL *pfEnabled), (pfEnabled))

    HRESULT WINAPI STUB_DwmIsCompositionEnabled(
        __out_ecount(1) BOOL *pfEnabled
        )
    {
        *pfEnabled = FALSE;
        return S_OK;
    }

}



