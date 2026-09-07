// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilBitmapCacheDuce, MILRender, "BitmapCache Resource");


//+----------------------------------------------------------------------------
//
// CMilBitmapCacheDuce::CMilBitmapCacheDuce
//
// Synopsis: 
//    Private constructor.
//
//-----------------------------------------------------------------------------

CMilBitmapCacheDuce::CMilBitmapCacheDuce(
    __in CComposition *pComposition,
    __in double renderAtScale,
    __in bool snapsToDevicePixels,
    __in bool enableClearType
    )
{
    m_pCompositionNoRef = pComposition;
    m_data.m_RenderAtScale = renderAtScale;
    m_data.m_pRenderAtScaleAnimation = NULL;
    m_data.m_SnapsToDevicePixels = snapsToDevicePixels;
    m_data.m_EnableClearType = enableClearType;
}

//+----------------------------------------------------------------------------
//
// CMilBitmapCacheDuce::Create
//
// Synopsis: 
//    Factory method for creating bitmap cache resources in native code.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilBitmapCacheDuce::Create(
    __in CComposition *pComposition,
    __in double renderAtScale,
    __in bool snapsToDevicePixels,
    __in bool enableClearType,
    __deref_out CMilBitmapCacheDuce **ppCacheMode
    )
{
    Assert(ppCacheMode);
    
    HRESULT hr = S_OK;
    
    CMilBitmapCacheDuce *pNewInstance = NULL;
        
    // Instantiate the instance
    pNewInstance = new CMilBitmapCacheDuce(pComposition, renderAtScale, snapsToDevicePixels, enableClearType);
    IFCOOM(pNewInstance);

    pNewInstance->AddRef();
    
    // Transfer ref to out argument
    *ppCacheMode = pNewInstance;

    // Avoid deletion during Cleanup
    pNewInstance = NULL;

Cleanup:
    // Free any allocations that weren't set to NULL due to failure
    ReleaseInterface(pNewInstance);

    RRETURN(hr);        
}

//+----------------------------------------------------------------------------
//
// CMilBitmapCacheDuce::GetScale
//
// Synopsis: 
//    Returns the current value of the RenderAtScale property.
//
//-----------------------------------------------------------------------------

double
CMilBitmapCacheDuce::GetScale()
{
    // Determine the current scale.
    double scale = m_data.m_RenderAtScale;
    if (m_data.m_pRenderAtScaleAnimation != NULL)
    {
        scale = *(m_data.m_pRenderAtScaleAnimation->GetValue());
    }
    
    // Scale must be non-negative
    return max(scale, 0.0);
}


