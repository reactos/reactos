// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Visual cache manager.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CVisualCacheManager, MILRender, "CVisualCacheManager");

//+-----------------------------------------------------------------------------
//
//    Member:
//        CVisualCacheManager constructor
//
//------------------------------------------------------------------------------

CVisualCacheManager::CVisualCacheManager(
    __in CComposition *pComposition, 
    __in CMILFactory *pFactory
    )
{
    m_pCompositionNoRef = pComposition;
    m_pFactoryNoRef = pFactory;
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CVisualCacheManager destructor
//
//------------------------------------------------------------------------------

CVisualCacheManager::~CVisualCacheManager()
{
    ReleaseInterface(m_pSoftwareRenderInterface);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CVisualCacheManager::Create
//
//    Synopsis:
//        Static.  Visual cache manager factory.
//
//------------------------------------------------------------------------------

HRESULT 
CVisualCacheManager::Create(
    __in_ecount(1) CComposition *pComposition,
    __in_ecount(1) CMILFactory *pFactory,
    __deref_out_ecount(1) CVisualCacheManager **ppVisualCacheManager
    )
{
    HRESULT hr = S_OK;

    CVisualCacheManager *pVisualCacheManager = new CVisualCacheManager(pComposition, pFactory);
    IFCOOM(pVisualCacheManager);   

    SetInterface(*ppVisualCacheManager, pVisualCacheManager); // add a reference
    pVisualCacheManager = NULL;

Cleanup:
    ReleaseInterface(pVisualCacheManager);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:
//      CVisualCacheManager::MarkCacheForUpdate
//
//  Synopsis:
//      Notification that this cache will be used for rendering this frame.
//
//------------------------------------------------------------------------

HRESULT 
CVisualCacheManager::MarkCacheForUpdate(CMilVisualCacheSet *pCache)
{
    HRESULT hr = S_OK;
    
    IFC(m_arrCachesToUpdateNoRef.Add(pCache));
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:
//      CVisualCacheManager::UpdateCaches
//
//  Synopsis:
//      Ensures that each cache marked for use this frame is valid.
//
//------------------------------------------------------------------------

HRESULT 
CVisualCacheManager::UpdateCaches()
{
    HRESULT hr = S_OK;

    IRenderTargetInternal *pIRT = NULL;
    
    WHEN_DBG_ANALYSIS(CoordinateSpaceId::Enum dbgCoordSpaceId;)

    UINT cCaches = m_arrCachesToUpdateNoRef.GetCount();
    UINT cacheIndex = 0;

    if (cCaches > 0)
    {
        IFC(GetBaseRenderInterface(
            &pIRT
            DBG_ANALYSIS_COMMA_PARAM(&dbgCoordSpaceId)
            ));
        
        // Ensure each cache marked dirty this frame by precompute is up-to-date.
        for ( ; cacheIndex < cCaches; cacheIndex += 1)
        {
            CMilVisualCacheSet *pVisualCaches = m_arrCachesToUpdateNoRef[cacheIndex];
            Assert(!pVisualCaches->IsValid());

            Assert(pIRT != NULL);
            IFC(pVisualCaches->Update(
                pIRT
                DBG_ANALYSIS_COMMA_PARAM(dbgCoordSpaceId)
                ));

            // We can't assert that the cache is valid after updating, since the UpdateCaches()
            // walk could have been kicked off by a VisualBrush/BitmapCacheBrush within an
            // updating cache's subtree.  Update() protects against such cycles.
        }
    }
    
Cleanup:
    if (FAILED(hr))
    {
        // If a cache update failed we'll bail out on this render pass.  However, the
        // changes to the cached node were already pre-computed, so the cache needs to be marked
        // as changed to ensure it is precomputed again next frame so that an update is again
        // processed.  The cache can't just be left in this list since next frame it (or its
        // Visual) could be disconnected from the tree when batches are processed.
        for ( ; cacheIndex < cCaches; cacheIndex += 1)
        {
            CMilVisualCacheSet *pVisualCaches = m_arrCachesToUpdateNoRef[cacheIndex];
            pVisualCaches->OnChanged(NULL, NotificationEventArgs::None);
        }
    }

    // Clear the list of caches to update.
    m_arrCachesToUpdateNoRef.Reset(FALSE);
    ReleaseInterface(pIRT);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CVisualCacheManager::RegisterVisualCache
// 
//  Synopsis:
//      Adds a visual cache to the global list of weak references.
// 
//------------------------------------------------------------------------------

HRESULT
CVisualCacheManager::RegisterVisualCache(
    __in_ecount(1) CMilVisualCacheSet *pVisualCaches
    )
{
    HRESULT hr = S_OK;

    IFC((m_arrAllDeviceCachesNoRef.Add(pVisualCaches)));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CVisualCacheManager::UnregisterVisualCache
// 
//  Synopsis:
//      Removes a visual cache from the global list of weak references.
// 
//------------------------------------------------------------------------------

bool
CVisualCacheManager::UnregisterVisualCache(
    __in_ecount(1) CMilVisualCacheSet *pVisualCaches
    )
{
    return !!m_arrAllDeviceCachesNoRef.Remove(pVisualCaches);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CVisualCacheManager::NotifyDeviceLost
// 
//  Synopsis:
//      Notifies the cache manager that an underlying HWndTarget has lost its device.
//      We use this notification to mark nodes with caches as dirty for precompute 
//      to ensure they are recreated.
// 
//------------------------------------------------------------------------------

void
CVisualCacheManager::NotifyDeviceLost()
{
    for (UINT i = 0; i < m_arrAllDeviceCachesNoRef.GetCount(); i++)
    {
        m_arrAllDeviceCachesNoRef[i]->NotifyDeviceLost();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CVisualCacheManager::GetBaseRenderInterface
// 
//  Synopsis:
//      Returns the base render interface for use creating cache textures this
//      frame.
// 
//------------------------------------------------------------------------------
HRESULT
CVisualCacheManager::GetBaseRenderInterface(
    __deref_out_xcount(1) IRenderTargetInternal **ppIRT
    DBG_ANALYSIS_COMMA_PARAM(__out CoordinateSpaceId::Enum *pdbgTargetCoordSpaceId)
    )
{
    HRESULT hr = S_OK;

    IRenderTargetInternal *pHardwareIRT = NULL;
    IMILRenderTargetBitmap *pSoftwareIRT = NULL;

    Assert(m_pCompositionNoRef);
    IFC(m_pCompositionNoRef->GetRenderTargetManagerNoRef()->GetHardwareRenderInterface(&pHardwareIRT));

    // If we found a hardware RT, use that.  Otherwise use the default software RT.
    if (pHardwareIRT != NULL)
    {
        *ppIRT = pHardwareIRT; // transfer ref
        pHardwareIRT = NULL;
        
        WHEN_DBG_ANALYSIS(*pdbgTargetCoordSpaceId = CoordinateSpaceId::PageInPixels;)
    }
    else
    {
        if (m_pSoftwareRenderInterface == NULL)
        {
            // Create a default software render interface for creating software caches.
            IFC(m_pFactoryNoRef->CreateBitmapRenderTarget(
                    1, 
                    1,  
                    MilPixelFormat::PBGRA32bpp, 
                    96.0f, 
                    96.0f, 
                    MilRTInitialization::Default, 
                    &pSoftwareIRT
                    ));

            // Accumulated two refs, pSoftwareIRT will release one in Cleanup
            IFC(pSoftwareIRT->QueryInterface(IID_IRenderTargetInternal, (void**) &m_pSoftwareRenderInterface));
        }

        m_pSoftwareRenderInterface->AddRef();
        *ppIRT = m_pSoftwareRenderInterface; // transfer ref
        
        WHEN_DBG_ANALYSIS(*pdbgTargetCoordSpaceId = CoordinateSpaceId::Device;)
    }
    
Cleanup:
    ReleaseInterface(pSoftwareIRT);
    RRETURN(hr);
}


