// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      CD3DRegistryDatabase implementation.  
//

#include "precomp.hpp"

MtDefine(CD3DRegistryDatabase, MILRender, "CD3DRegistryDatabase");

//
// Statics
//

bool CD3DRegistryDatabase::m_fInitialized = false;
UINT CD3DRegistryDatabase::m_cNumAdapters = 0;
UINT *CD3DRegistryDatabase::m_prguErrorCount = NULL;
bool CD3DRegistryDatabase::m_fSkipDriverCheck = false;

// Maximum number of internal errors on the D3DDevice before we disable it.
// Set m_prguErrorCount[adapter] to this to disable.
const UINT c_uMaxErrorCount = 5;

//+------------------------------------------------------------------------
//
//  Function:  CD3DRegistryDatabase::IsAdapterEnabled
//
//  Synopsis:  1. Ensure we initialized our status
//             2. Look up adapter in our list
//
//-------------------------------------------------------------------------
HRESULT 
CD3DRegistryDatabase::IsAdapterEnabled(
    UINT uAdapter, 
    __out_ecount(1) bool *pfEnabled
    )
{
    HRESULT hr = S_OK;

    Assert(pfEnabled);

    *pfEnabled = false;

    //
    // Make sure that we have initialized our list from the registry
    //

    Assert(m_fInitialized);

    //
    // Ensure parameters are valid
    //

    if (uAdapter >= m_cNumAdapters)
    {
        IFC(E_INVALIDARG);
    }

    //
    // Return status
    //

    *pfEnabled = m_prguErrorCount[uAdapter] < c_uMaxErrorCount;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DRegistryDatabase::DisableAdapter
//
//  Synopsis:  Mark a given adapter as unusable
//
//-------------------------------------------------------------------------
HRESULT 
CD3DRegistryDatabase::DisableAdapter(
    UINT uAdapter
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that we have initialized our list from the registry
    //

    Assert(m_fInitialized);

    //
    // Ensure parameters are valid
    //

    if (uAdapter >= m_cNumAdapters)
    {
        IFC(E_INVALIDARG);
    }

    //
    // Set status
    //

    m_prguErrorCount[uAdapter] = c_uMaxErrorCount;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DRegistryDatabase::HandleAdapterUnexpectedError
//
//  Synopsis:  Handle an unexpected error from an adapter, possibly
//             disabling the adapter.
//
//-------------------------------------------------------------------------
HRESULT 
CD3DRegistryDatabase::HandleAdapterUnexpectedError(
    UINT uAdapter
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that we have initialized our list from the registry
    //

    Assert(m_fInitialized);

    //
    // Ensure parameters are valid
    //

    if (uAdapter >= m_cNumAdapters)
    {
        IFC(E_INVALIDARG);
    }

    //
    // Increment errors
    //

    if (m_prguErrorCount[uAdapter] < c_uMaxErrorCount)
    {
        ++m_prguErrorCount[uAdapter];
        if (m_prguErrorCount[uAdapter] >= c_uMaxErrorCount)
        {
            TraceTag((tagError, "MIL-HW(adapter=%d): Too many d3d internal errors-- switching to software rendering.", uAdapter));
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DRegistryDatabase::ShouldSkipDriverCheck
//
//  Synopsis:  Should we skip driver/vendor checks?  This flag enables 
//             IHV's to investigate issues after we've disabled their 
//             card.
//
//-------------------------------------------------------------------------
bool
CD3DRegistryDatabase::ShouldSkipDriverCheck()
{
    return m_fSkipDriverCheck;
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DRegistryDatabase::EnableAllAdapters
//
//  Synopsis:  Either enable or disable all adapters
//
//-------------------------------------------------------------------------
void 
CD3DRegistryDatabase::EnableAllAdapters(bool fEnabled)
{
    for (UINT i = 0; i < m_cNumAdapters; i++)
    {
        m_prguErrorCount[i] = fEnabled ? 0 : c_uMaxErrorCount;
    }
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DRegistryDatabase::InitializeFromRegistry
//
//  Synopsis:  Initialize our database from the driver list
//
//-------------------------------------------------------------------------

MtDefine(CD3DRegistryData, MILRender, "CD3DRegistryDatabase enable array")

HRESULT 
CD3DRegistryDatabase::InitializeFromRegistry(
    __in_ecount(1) IDirect3D9 *pD3D
    )
{
    HRESULT hr = S_OK;

    Assert(!m_fInitialized);

    IFC(InitializeDriversFromRegistry(pD3D));

Cleanup:
    m_fInitialized = SUCCEEDED(hr);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DRegistryDatabase::InitializeDriversFromRegistry
//
//  Synopsis:  Initialize Drivers based on registry key settings
//
//-------------------------------------------------------------------------
HRESULT
CD3DRegistryDatabase::InitializeDriversFromRegistry(
    __inout_ecount(1) IDirect3D9 *pD3D
    )
{
    HRESULT hr = S_OK;
    HKEY hRegAvalonGraphics = NULL;
    DWORD dwType;
    DWORD dwDisableHWAccleration;
    DWORD dwDataSize;

    Assert(pD3D);

    //
    // Get number of adaptors
    //

    m_cNumAdapters = pD3D->GetAdapterCount();

    //
    // Allocate adapter enable array
    //

    {
        UINT cAllocSize = 0;
        IFC(MultiplyUINT(m_cNumAdapters, sizeof(*m_prguErrorCount), OUT cAllocSize));

        m_prguErrorCount = WPFAllocType(UINT *,
            ProcessHeap,
            Mt(CD3DRegistryData),
            cAllocSize
            );
        IFCOOM(m_prguErrorCount);
    }

    //
    // Check for global Avalon registry hooks
    //

    hr = GetAvalonRegistrySettingsKey(&hRegAvalonGraphics);
    if (FAILED(hr))
    {
        // If we can't open the root key, assume everything is enabled
        // and ignore the error
        EnableAllAdapters(true /* fEnabled */);
        hr = S_OK;
        goto Cleanup;
    }

    //
    // Check if HW acceleration is disabled
    //

    dwDataSize = 4;
    if (RegQueryValueEx(
        hRegAvalonGraphics,
        _T("DisableHWAcceleration"),
        NULL,
        &dwType,
        (LPBYTE)&dwDisableHWAccleration,
        &dwDataSize
        ) == ERROR_SUCCESS)
    {
        if (dwType != REG_DWORD || dwDisableHWAccleration)
        {
            EnableAllAdapters(false /* fEnabled */);
            goto Cleanup;
        }
    }

    EnableAllAdapters(true /* fEnabled */);

Cleanup:
    if (FAILED(hr))
    {
        WPFFree(ProcessHeap, m_prguErrorCount);
        m_prguErrorCount = NULL;
    }

    if (hRegAvalonGraphics != NULL)
    {
        RegCloseKey(hRegAvalonGraphics);
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DRegistryDatabase::Cleanup
//
//  Synopsis:  Reset to uninitialized state
//
//-------------------------------------------------------------------------
void 
CD3DRegistryDatabase::Cleanup(
    )
{
    WPFFree(ProcessHeap, m_prguErrorCount);

    m_fInitialized = false;
    m_cNumAdapters = 0;
    m_prguErrorCount = NULL;
}





