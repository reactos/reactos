// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************\
*

*
* Abstract:
*
*   Contains miscellaneous engine helper functions.
*
* Revision History:
*
*   12/13/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

MtDefine(MILApi, Mem, "MIL Api objects");
MtDefine(MILRender, Mem, "MIL Rendering objects");

DeclareTag(tagMILStepRendering, "MIL", "MIL Step Rendering");
DeclareTag(tagMILStepRenderingDisableBreak, "MIL", "MIL Step Rendering - Disable Break");

UINT CCommonRegistryData::m_uResCheckInSeconds = 15 * 60;
bool CCommonRegistryData::m_fGPUThrottlingDisabled = false;

//can be overriden by HKLM\Software\Microsoft\Avalon.Graphics\DisableInstrumentationBreaking(DWORD) = !0

/**************************************************************************\
*
* Function Description:
*
*   Opens the global registry key. Used to save settings information. The
*   caller is responsible for closing this key.
*
\**************************************************************************/

HRESULT GetAvalonRegistrySettingsKey(
    __out HKEY *phRegSettings, 
    BOOL fCurrentUser
    )
{
    return (RegOpenKeyEx(
        fCurrentUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
        _T("Software\\Microsoft\\Avalon.Graphics"),
        0,
        KEY_QUERY_VALUE,
        phRegSettings) == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

#if PRERELEASE
/**************************************************************************\
*
* Function Description:
*
*   Opens the DWM registry key. Used to save settings information. The
*   caller is responsible for closing this key.
*
\**************************************************************************/

HRESULT GetDWMRegistrySettingsKey(
    __out HKEY *phRegSettings
    )
{
    return (RegOpenKeyEx(
        HKEY_CURRENT_USER,
        _T("Software\\Microsoft\\Windows\\DWM"),
        0,
        KEY_QUERY_VALUE,
        phRegSettings) == ERROR_SUCCESS) ? S_OK : E_FAIL;
}
#endif

/**************************************************************************\
*
* Function Description:
*
*   Helper to read single dword value from registry.
*   If there is no such a record in the registry, then *pValue remains unchanged.
*
\**************************************************************************/

bool RegReadDWORD(
    __in HKEY hKey, 
    __in PCWSTR pName, 
    __inout_ecount(1) DWORD *pValue
    )
{
    DWORD dwType = 0;
    DWORD dwValue = 0;
    DWORD dwDataSize = sizeof(dwValue);

    Assert(pName);
    Assert(pValue);

    if (RegQueryValueEx(
        hKey,
        pName,
        NULL,
        &dwType,
        (LPBYTE)&dwValue,
        &dwDataSize
        ) == ERROR_SUCCESS)
    {
        if (dwType == REG_DWORD)
        {
            *pValue = dwValue;
            return true;
        }
    }

    return false;
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize globals for the MIL render engine.
*
*   NOTE: Initialization should not be extremely expensive!
*         Do NOT put a lot of gratuitous junk into here; consider instead
*         doing lazy initialization.
*
\**************************************************************************/

HRESULT Startup()
{
    HRESULT hr = S_OK;

    HKEY hRegAvalonGraphics = NULL;

    // Initializes CPU caps 
    CCPUInfo::Initialize();

    // Assert CPU features that we only use in DBG mode
    Assert(MILInterlockedAvailable());

    hr = CCommonRegistryData::InitializeFromRegistry();

    //
    // MACHINE-WIDE SETTINGS
    //

    if (SUCCEEDED(hr))
    {
        hr = (GetAvalonRegistrySettingsKey(&hRegAvalonGraphics, FALSE /* machine-wide */));
    }
    
    if (SUCCEEDED(hr))
    {
        if (hRegAvalonGraphics != NULL)
        {
            RegCloseKey(hRegAvalonGraphics);
        }
    }
    else
    {
        // This means that we didn't find the registry key. Everything is still fine.
        hr = S_OK;
    } 

    IFC(CD3DModuleLoader::Startup());
    IFC(g_DisplayManager.Init());
    IFC(g_DWriteLoader.Startup());
    IFC(CAVLoader::Startup());

    TraceTag((tagMILVerbose, "Startup completed successfully"));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  Shutdown
//
//  Synopsis:  Release global resources needed by MIL render engine
//
//-------------------------------------------------------------------------
void
Shutdown()
{
    CAVLoader::Shutdown();
    CD3DModuleLoader::Shutdown();
    g_DWriteLoader.Shutdown();
}

//+------------------------------------------------------------------------
//
//  Function:  CCommonRegistryData::InitializeFromRegistry
//
//  Synopsis:  Initialize the values
//
//-------------------------------------------------------------------------
HRESULT
CCommonRegistryData::InitializeFromRegistry()
{
    HRESULT hr = S_OK;
    HKEY hRegAvalonGraphicsLocalMachine = NULL;

#if PRERELEASE
    IFC(InitializeDWMKeysFromRegistry());
#endif

    hr = THR(GetAvalonRegistrySettingsKey(
        &hRegAvalonGraphicsLocalMachine,
        FALSE   // fCurrentUser
        ));

    if (FAILED(hr))
    {
        hr = S_OK;
        goto Cleanup;
    }

    {
        DWORD dwTemp = 0;

        if (   RegReadDWORD(hRegAvalonGraphicsLocalMachine, _T("DisableGpuThrottling"), &dwTemp)
            && dwTemp != 0
               )
        {
            m_fGPUThrottlingDisabled = true;
        }
    }

    // NOTICE-2006/07/19-milesc  Given that most of the registry keys previously 
    // in the class were not registry keys we wanted to ship, this class no longer
    // accesses the registry for all keys. Instead default values are returned 
    // from the public functions for those keys we didn't want to ship with.

Cleanup:
    if (hRegAvalonGraphicsLocalMachine != NULL)
    {
        RegCloseKey(hRegAvalonGraphicsLocalMachine);
    }

    return S_OK;
}


#ifdef PRERELEASE
//+------------------------------------------------------------------------
//
//  Function:  CCommonRegistryData::InitializeDWMKeysFromRegistry
//
//  Synopsis:  Initialize DWM Keys
//
//-------------------------------------------------------------------------
HRESULT
CCommonRegistryData::InitializeDWMKeysFromRegistry()
{
    HRESULT hr = S_OK;
    HKEY hRegDWMGraphics = NULL;

    //
    // Check for global DWM registry hooks
    //

    hr = THR(GetDWMRegistrySettingsKey(&hRegDWMGraphics));
    if (FAILED(hr))
    {
        // If we can't open the root key, assume everything is default
        // and ignore the error
        hr = S_OK;
        goto Cleanup;
    }


    //
    // We don't want this registry key check in the final build.  So we wrap it
    // in this #ifdef PRERELEASE.
    //

    //
    // How long we should wait, in seconds, before checking the residency of 
    // video memory resources
    //

    {
        DWORD dwTemp = 0;

        if (RegReadDWORD(hRegDWMGraphics, _T("ResourceResidencyCheckIntervalInSeconds"), &dwTemp))
        {
            m_uResCheckInSeconds = dwTemp;
        }
    }  


Cleanup:
    if (hRegDWMGraphics != NULL)
    {
        RegCloseKey(hRegDWMGraphics);
    }

    RRETURN(hr);
}
#endif




