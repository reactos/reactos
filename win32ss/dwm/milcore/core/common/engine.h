// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains simple engine-wide prototypes and helper functions and
//      compile time flags.
//
//------------------------------------------------------------------------------

#pragma once


MtExtern(MILApi);
MtExtern(MILRender);

ExternTag(tagMILStepRendering);
ExternTag(tagMILStepRenderingDisableBreak);


//------------------------------------------------------------------------
// FillMemoryInt32 - Fill an INT32 array with the specified value
//------------------------------------------------------------------------

inline VOID
FillMemoryInt32(
    __typefix(INT32 *) __out_ecount(count) VOID *buf,
    INT count,
    INT32 val
    )
{
    INT32 *p = reinterpret_cast<INT32 *>(buf);

    while (count-- > 0)
    {
        *p++ = val;
    }
}

HRESULT GetAvalonRegistrySettingsKey(__out HKEY *phRegSettings, BOOL fCurrentUser = TRUE);
#if PRERELEASE
HRESULT GetDWMRegistrySettingsKey(__out HKEY *phRegSettings);
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
    );

HRESULT Startup();
void Shutdown();

//
// Consolidate Registry Keys
//
// The other registry keys set in these files should be moved into the
// CCommonRegistryData class.
//

//
// Common Registry Data
//
// NOTICE-2006/07/19-milesc  Given that most of the registry keys previously 
// in the class were not registry keys we wanted to ship, this class no longer
// accesses the registry for all keys. Instead default values are returned 
// from the public functions for those keys we didn't want to ship with.
//
class CCommonRegistryData
{
public:
    static HRESULT InitializeFromRegistry();

    static UINT GetResidencyCheckIntervalInSeconds()
    {
        return m_uResCheckInSeconds;
    }

    static bool GPUThrottlingDisabled()
    {
        return m_fGPUThrottlingDisabled;
    }

private:
#if PRERELEASE
    static HRESULT InitializeDWMKeysFromRegistry();    
#endif

private:
    static UINT m_uResCheckInSeconds;
    static bool m_fGPUThrottlingDisabled;
};



