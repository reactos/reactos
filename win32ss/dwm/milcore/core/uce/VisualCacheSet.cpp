// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(VisualCacheSetResource, MILRender, "BlurEffect Resource");
MtDefine(CMilVisualCacheSet, VisualCacheSetResource, "VisualCacheSet resource");
MtDefine(BrushCacheToken,  VisualCacheSetResource, "BrushCacheToken struct");


CMilBitmapCacheDuce* CMilVisualCacheSet::s_pDefaultCacheMode = NULL;

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::CMilVisualCacheSet
//
//-----------------------------------------------------------------------------

CMilVisualCacheSet::CMilVisualCacheSet(
    __in_ecount(1) CComposition* pComposition,
    __in_ecount(1) CMilVisual *pVisual
    )
{
    m_pCompositionNoRef = pComposition;
    m_pVisualNoRef = pVisual;
    m_cUnspecifiedBrushes = 0;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::~CMilVisualCacheSet
//
//-----------------------------------------------------------------------------

CMilVisualCacheSet::~CMilVisualCacheSet()
{    
    #if DBG
    bool result = 
    #endif
    m_pCompositionNoRef->GetVisualCacheManagerNoRef()->UnregisterVisualCache(this);
    #if DBG // This is here to stop a PreFast error about 'result' being undeclared, it's not strictly necessary otherwise.
    Assert(result);
    #endif

    // Clean up node cache.
    UnRegisterNotifier(m_pNodeCache);

    // Clean-up any brush caches.
    for (UINT i = 0; i < m_arrBrushCaches.GetCount(); i++)
    {
        BrushCacheToken *pCacheToken = m_arrBrushCaches[i];
        
        UnRegisterNotifier(pCacheToken->pCache);
        WPFFree(ProcessHeap, pCacheToken);
    }
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::IsOfType
//
// Synopsis:
//    Since this class is a wrapper for a BitmapCache resource, returns the
//    resource's type.
//
//-----------------------------------------------------------------------------

__override
bool
CMilVisualCacheSet::IsOfType(MIL_RESOURCE_TYPE type) const
{
    FreAssert(FALSE);
    return false;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::GetCount
//
// Synopsis:
//    Returns the total number of caches in this cache set.
//
//-----------------------------------------------------------------------------

UINT 
CMilVisualCacheSet::GetCount() const
{
    UINT count = 0;

    if (m_pNodeCache != NULL)
    {
        count = 1;
    }

    count += m_arrBrushCaches.GetCount();

    return count;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::OnChanged
//
// Synopsis: 
//    Changed handler.  If the wrapped BitmapCache resource changes, we need
//    to ensure the caches are walked again in precompute.
//
//-----------------------------------------------------------------------------

BOOL CMilVisualCacheSet::OnChanged(
    CMilSlaveResource *pSender, 
    NotificationEventArgs::Flags e
    )
{
    m_pVisualNoRef->MarkDirtyForPrecompute();
    return TRUE;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::Create
//
// Synopsis: 
//    Factory method for creating CMilVisuaCacheSets.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCacheSet::Create(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilVisual *pVisual,
        __deref_out_ecount(1) CMilVisualCacheSet **ppCache
        )
{
    Assert(ppCache);
    
    HRESULT hr = S_OK;
    
    CMilVisualCacheSet *pNewInstance = NULL;
        
    // Instantiate the wrapper
    pNewInstance = new CMilVisualCacheSet(pComposition, pVisual);
    IFCOOM(pNewInstance);

    pNewInstance->AddRef();
    
    // Register the cache to receive device lost notifications.
    IFC(pComposition->GetVisualCacheManagerNoRef()->RegisterVisualCache(pNewInstance));
    
    // Transfer ref to out argument
    *ppCache = pNewInstance;
    pNewInstance = NULL;

Cleanup:
    // Free any allocations that weren't set to NULL due to failure
    ReleaseInterface(pNewInstance);

    RRETURN(hr);    
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::GetNodeCacheMode
//
// Synopsis: 
//    Returns the cache mode for the node's cache, if it exists.
//
//-----------------------------------------------------------------------------

__out_opt
CMilBitmapCacheDuce* 
CMilVisualCacheSet::GetNodeCacheMode()
{
    if (m_pNodeCache == NULL)
    {
        return NULL;
    }
    else
    {
        return m_pNodeCache->GetCacheMode();
    }
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::SetNodeCacheMode
//
// Synopsis: 
//    Sets the cache mode on the node's cache.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilVisualCacheSet::SetNodeCacheMode(
    __in_opt CMilCacheModeDuce* pCacheMode
    )
{
    HRESULT hr = S_OK;

    CMilVisualCache *pNodeCache = NULL;

    // If we are changing from not having a node cache to having one (or vice versa),
    // then any cache brushes that did not specify a cache mode will change from 
    // using the default cache to using the node cache (or vice versa).
    // We'll handle this by removing their references to the old cache and adding
    // them back to the new one.
    UINT cUnspecifiedBrushes = m_cUnspecifiedBrushes;
    bool fHandleUnspecifiedBrushes =    cUnspecifiedBrushes >= 1
                                     &&   ((m_pNodeCache != NULL && pCacheMode == NULL)
                                        || (m_pNodeCache == NULL && pCacheMode != NULL));

    // Remove references from unspecified brushes to their old cache.
    if (fHandleUnspecifiedBrushes)
    {
        #if DBG
        bool fRemoved =
        #endif
        RemoveCacheInternal(NULL, cUnspecifiedBrushes);
        #if DBG
        Assert(fRemoved);
        #endif
    }

    // If the node no longer has a cache mode, release the node cache
    // otherwise we can keep using the current m_pNodeCache instance. 
    if (pCacheMode == NULL)
    {
        UnRegisterNotifier(m_pNodeCache);
    }
    else
    {
        // If we haven't yet created a node cache, do so.
        if (m_pNodeCache == NULL)
        {
            IFC(CMilVisualCache::Create(m_pCompositionNoRef, m_pVisualNoRef, &pNodeCache));
            m_pNodeCache = pNodeCache;
            IFC(RegisterNotifier(m_pNodeCache));
        }

        m_pNodeCache->SetCacheMode(pCacheMode);
    }

    // Add back references from unspecified brushes to new cache instead.
    if (fHandleUnspecifiedBrushes)
    {
        IFC(AddCacheInternal(NULL, cUnspecifiedBrushes));
    }

Cleanup:
    // Release the ref from Create, we still hold one from registering as a listener.
    ReleaseInterface(pNodeCache);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::IsNodeCacheValid
//
// Synopsis: 
//    Returns false if the contents of the cache are stale.  IsValid does not 
//    check device state, that's handled by NotifyDeviceLost.
//
//-----------------------------------------------------------------------------

bool
CMilVisualCacheSet::IsNodeCacheValid() const
{
    if (m_pNodeCache == NULL)
    {
        return false;
    }
    else
    {
        return m_pNodeCache->IsValid();
    }
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::GetNodeCacheRenderTargetBitmap
//
// Synopsis: 
//    Returns the valid, up-to-date render target bitmap for the node cache.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilVisualCacheSet::GetNodeCacheRenderTargetBitmap (
    __deref_out_opt IMILRenderTargetBitmap ** ppIRTB,
    __in IRenderTargetInternal *pDestRT
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    Assert(m_pNodeCache);
    RRETURN(m_pNodeCache->GetRenderTargetBitmap(
        ppIRTB,
        pDestRT 
        DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)
        ));
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::GetNodeCacheScaleInflation
//
// Synopsis: 
//    Returns the amount to inflate a dirty rect in world space to account
//    for the scaled size of the cache in local space.
//
//-----------------------------------------------------------------------------
float
CMilVisualCacheSet::GetNodeCacheScaleInflation()
{
    float inflation = 1.0f;
    if (m_pNodeCache != NULL)
    {
        inflation = m_pNodeCache->GetScaleInflation();
    }

    return inflation;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::RenderNodeCache
//
// Synopsis: 
//    Draws this node's cache into the argument DrawingContext.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCacheSet::RenderNodeCache(
    __in CDrawingContext *pDC,
    __in IRenderTargetInternal *pDestRT,
    float opacity
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    Assert(m_pNodeCache);
    RRETURN(m_pNodeCache->Render(
        pDC, 
        pDestRT,
        opacity
        DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)
        ));
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::IsValid
//
// Synopsis: 
//    Returns true if all caches are valid, false otherwise.
//
//-----------------------------------------------------------------------------

bool 
CMilVisualCacheSet::IsValid() const
{
    // The cache set should always contain at least one cache.
    Assert(m_pNodeCache != NULL || m_arrBrushCaches.GetCount() > 0);
    
    bool isValid = true;

    if (m_pNodeCache != NULL)
    {
        isValid = m_pNodeCache->IsValid();
    }

    for (UINT i = 0; i < m_arrBrushCaches.GetCount(); i++)
    {
        const BrushCacheToken *pCacheToken = m_arrBrushCaches[i];
        isValid = isValid && pCacheToken->pCache->IsValid();
    }

    return isValid;
}
    
//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::BeginPartialInvalidate
//
// Synopsis: 
//    Returns the dirty region accumulator to pick up new invalid regions.
//
//-----------------------------------------------------------------------------

void
CMilVisualCacheSet::BeginPartialInvalidate(
    __in float allowedDirtyRegionOverhead,
    __deref_out CDirtyRegion2 **ppDirtyRegionsNoRef
    )
{        
    CMilRectF rect = CMilRectF::sc_rcInfinite;
    m_dirtyRegion.Initialize(&rect, allowedDirtyRegionOverhead);
    *ppDirtyRegionsNoRef = &m_dirtyRegion;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::EndPartialInvalidate
//
// Synopsis: 
//    Marks the cache as dirty for update for the given regions and bounds.  The precompute
//    walk has updated m_pDirtyRegion for us.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCacheSet::EndPartialInvalidate(
    __in MilRectF const *prcLocalBounds
    )
{
    HRESULT hr = S_OK;
    
    //  There are 3 ways our cache node will be walked in precompute.
    //          A. Our cached node was dirty.  See FullInvalidate.
    //          B. Nothing was dirty, but we walked to our node to redraw something above or below the 
    //             cached node.  Do nothing (the check below).
    //          C. Our subtree was dirty.  Here we set m_isDirty and draw the dirty regions.
    // If a dirty region was added we need to update the cache.
    // Call GetDirtyRegion so we can call GetRegionCount().
    m_dirtyRegion.GetUninflatedDirtyRegions();
    if (m_dirtyRegion.GetRegionCount() > 0)
    {
        // If we had some dirty regions, partially invalidate all the caches.
        if (m_pNodeCache != NULL)
        {
            m_pNodeCache->Invalidate(false, prcLocalBounds);
        }

        for (UINT i = 0; i < m_arrBrushCaches.GetCount(); i++)
        {
            BrushCacheToken *pCacheToken = m_arrBrushCaches[i];
            pCacheToken->pCache->Invalidate(false, prcLocalBounds);
        }

        // We add our cache to the VisualCacheManager's list so it can update the cache before it is 
        // needed by the Render pass.
        IFC(m_pCompositionNoRef->GetVisualCacheManagerNoRef()->MarkCacheForUpdate(this));
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::FullInvalidate
//
// Synopsis: 
//    Marks the cache as dirty for update for the given regions and bounds.  
//    The precompute walk has NOT updated m_pDirtyRegion for us, but it will be 
//    ignored since each cache needs to fully redraw anyway (so they won't use dirty 
//    regions).
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCacheSet::FullInvalidate(
    __in MilRectF const *prcLocalBounds
    )
{
    HRESULT hr = S_OK;
    
    if (m_pNodeCache != NULL)
    {
        m_pNodeCache->Invalidate(true, prcLocalBounds);
    }

    for (UINT i = 0; i < m_arrBrushCaches.GetCount(); i++)
    {
        BrushCacheToken *pCacheToken = m_arrBrushCaches[i];
        pCacheToken->pCache->Invalidate(true, prcLocalBounds);
    }

    // We add our cache to the VisualCacheManager's list so it can update the cache before it is 
    // needed by the Render pass.
    IFC(m_pCompositionNoRef->GetVisualCacheManagerNoRef()->MarkCacheForUpdate(this));

Cleanup:
    RRETURN(hr);
 }

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::NotifyDeviceLost
//
// Synopsis: 
//    Marks the cached visual dirty for precompute to ensure that the cache
//    will be recreated and re-rendered.
//
//-----------------------------------------------------------------------------

void
CMilVisualCacheSet::NotifyDeviceLost()
{
    m_pVisualNoRef->MarkDirtyForPrecompute();

    if (m_pNodeCache)
    {
        m_pNodeCache->NotifyDeviceLost();
    }

    for (UINT i = 0; i < m_arrBrushCaches.GetCount(); i++)
    {
        BrushCacheToken *pCacheToken = m_arrBrushCaches[i];
        pCacheToken->pCache->NotifyDeviceLost();
    }
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::Update
//
// Synopsis: 
//    Brings the rendered content of the cache up to date.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCacheSet::Update(
    __in IRenderTargetInternal* pIRTInternal
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    HRESULT hr = S_OK;

    if (m_pNodeCache != NULL)
    {
        IFC(m_pNodeCache->Update(
            pIRTInternal,
            &m_dirtyRegion
            DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)
            ));
    }

    for (UINT i = 0; i < m_arrBrushCaches.GetCount(); i++)
    {
        BrushCacheToken *pCacheToken = m_arrBrushCaches[i];
        IFC(pCacheToken->pCache->Update(
            pIRTInternal,
            &m_dirtyRegion
            DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)
            ));
    }
    
Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::AddCache
//
// Synopsis: 
//    Adds a cache reference for the specified cache mode.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCacheSet::AddCache(
    __in_opt CMilBitmapCacheDuce *pBitmapCacheMode
    )
{
    RRETURN(AddCacheInternal(pBitmapCacheMode, 1));
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::AddCacheInternal
//
// Synopsis: 
//    Adds refCount references for the specified cache mode.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCacheSet::AddCacheInternal(
    __in_opt CMilBitmapCacheDuce *pBitmapCacheMode,
    __in UINT refCount
    )
{
    HRESULT hr = S_OK;

    CMilVisualCache *pBrushCache = NULL;

    // Our cache mode for lookup will either be the specified cache mode or the default cache.
    CMilBitmapCacheDuce *pCacheModeForLookup = pBitmapCacheMode;

    // Should only be called with a positive number of cache references.
    Assert(refCount >= 1);
    
    // Handle unspecified cache modes separately.
    if (pCacheModeForLookup == NULL)
    {
        m_cUnspecifiedBrushes += refCount;
        
        if (m_pNodeCache != NULL)
        {
            goto Cleanup;
        }
        else
        {
            // Use the default cache if there is no node cache.
            if (s_pDefaultCacheMode == NULL)
            {
                // Lazily create the static default cache mode.
                IFC(CMilBitmapCacheDuce::Create(
                    m_pCompositionNoRef,
                    1.0,
                    false,
                    false,
                    &s_pDefaultCacheMode
                    ));
            }

            pCacheModeForLookup = s_pDefaultCacheMode;
        }
    }

    // Try to find an existing cache to re-use.  We can re-use a cache if the specified
    // cache mode is identical.
    BrushCacheToken *pCacheToken = LookupCache(pCacheModeForLookup);
    if (pCacheToken != NULL)
    {
        // Re-use the cache, increment use counter.
        // The cache set registered as a listener when this cache was created, no need
        // to do it again.
        Assert(pCacheToken->pCache);
        pCacheToken->refCount += refCount;
    }
    else
    {
        // Create a new cache for this cache mode, add the token to our collection, and
        // register as a listener.
        IFC(CMilVisualCache::Create(m_pCompositionNoRef, m_pVisualNoRef, &pBrushCache));

        pBrushCache->SetCacheMode(pCacheModeForLookup);

        BrushCacheToken *pNewCacheToken = reinterpret_cast<BrushCacheToken*>WPFAlloc(ProcessHeap, Mt(BrushCacheToken), sizeof(BrushCacheToken));
        pNewCacheToken->pCache = pBrushCache;
        pNewCacheToken->pCacheModeNoRef = pCacheModeForLookup;
        pNewCacheToken->refCount = refCount;
        IFC(m_arrBrushCaches.Add(pNewCacheToken));
        
        IFC(RegisterNotifier(pBrushCache));

        // Since we've added a new cache we need to ensure it is updated.
        m_pVisualNoRef->MarkDirtyForPrecompute();
    }

Cleanup:
    // Release the ref from Create, we still hold one from registering as a listener.
    ReleaseInterface(pBrushCache);
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::RemoveCache
//
// Synopsis: 
//    Removes a cache reference for the specified cache mode.
//
//-----------------------------------------------------------------------------

bool
CMilVisualCacheSet::RemoveCache(
    __in_opt CMilBitmapCacheDuce const *pBitmapCacheMode
    )
{
    return RemoveCacheInternal(pBitmapCacheMode, 1);
}


//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::RemoveCacheInternal
//
// Synopsis: 
//    Removes refCount cache references for the specified cache mode.
//
//-----------------------------------------------------------------------------
    
bool
CMilVisualCacheSet::RemoveCacheInternal(
    __in_opt CMilBitmapCacheDuce const *pBitmapCacheMode,
    __in UINT refCount
    )
{
    bool fFoundCache = false;

    // Our cache mode for lookup will either be the specified cache mode or the default cache.
    const CMilBitmapCacheDuce *pCacheModeForLookup = pBitmapCacheMode;

    // Should only be called with a positive number of cache references.
    Assert(refCount > 0);
    
    if (pCacheModeForLookup == NULL)
    {
        Assert(m_cUnspecifiedBrushes >= refCount);
        m_cUnspecifiedBrushes -= refCount;
        
        if (m_pNodeCache != NULL)
        {
            return true;
        }
        else
        {
            // Remove reference to the default cache.
            Assert(s_pDefaultCacheMode != NULL);
            pCacheModeForLookup = s_pDefaultCacheMode;
        }
    }
    
    // Look-up the corresponding cache token.
    BrushCacheToken *pCacheToken = LookupCache(pCacheModeForLookup);
    if (pCacheToken != NULL)
    {
        fFoundCache = true;
        
        // Found the cache, decrement use counter.
        Assert(pCacheToken->pCache);
        Assert(pCacheToken->refCount >= refCount);
        pCacheToken->refCount -= refCount;

        // If this was the last reference for this cache, unregister it and delete its token.
        if (pCacheToken->refCount == 0)
        {
            UnRegisterNotifier(pCacheToken->pCache);

            m_arrBrushCaches.Remove(pCacheToken);

            WPFFree(ProcessHeap, pCacheToken);
        }

    }
    
    // We should always find a cache since we should only call Remove after a matching Add.
    Assert(fFoundCache);
    
    return fFoundCache;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::GetBitmapSource
//
// Synopsis: 
//    Returns the bitmap source for the associated cache mode's cache.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilVisualCacheSet::GetBitmapSource (
    __in_opt CMilBitmapCacheDuce const *pCacheMode,
    __in IRenderTargetInternal *pIRT,
    __deref_out_opt IWGXBitmapSource ** const ppIBitmapSource
    )
{
    HRESULT hr = S_OK;

    // Our cache mode for lookup will either be the specified cache mode or the default cache.
    const CMilBitmapCacheDuce *pCacheModeForLookup = pCacheMode;
    
    // If the caller doesn't specify a cache mode, return
    // the cache on this node if it exists, otherwise return
    // the default brush cache.
    if (pCacheModeForLookup == NULL)
    {
        Assert(m_cUnspecifiedBrushes > 0);

        if (m_pNodeCache != NULL)
        {
            // Use the node cache if it exists.
            IFC(m_pNodeCache->GetBitmapSource(
                ppIBitmapSource,
                pIRT
                DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::PageInPixels)
                ));
            
            goto Cleanup;
        }
        else
        {
            Assert(s_pDefaultCacheMode != NULL);
            pCacheModeForLookup = s_pDefaultCacheMode;
        }
    }

    // Look-up the corresponding cache token.
    BrushCacheToken *pCacheToken = LookupCache(pCacheModeForLookup);
    if (pCacheToken != NULL)
    {
        IFC(pCacheToken->pCache->GetBitmapSource(
            ppIBitmapSource,
            pIRT
            DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::PageInPixels)
            ));
    }
    else
    {
        // We shouldn't try to get a bitmap from a non-existent cache.
        Assert(false);
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::GetBitmapSource
//
// Synopsis: 
//    Returns the render target texture for the associated cache mode's cache.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilVisualCacheSet::GetRenderTargetBitmap (
    __in_opt CMilBitmapCacheDuce const *pCacheMode,
    __deref_out_opt IMILRenderTargetBitmap ** ppIRTB,
    __in IRenderTargetInternal *pDestRT
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    HRESULT hr = S_OK;

    // Our cache mode for lookup will either be the specified cache mode or the default cache.
    const CMilBitmapCacheDuce *pCacheModeForLookup = pCacheMode;
    
    // If the caller doesn't specify a cache mode, return
    // the cache on this node if it exists, otherwise return
    // the default brush cache.
    if (pCacheModeForLookup == NULL)
    {
        Assert(m_cUnspecifiedBrushes > 0);

        if (m_pNodeCache != NULL)
        {
            // Use the node cache if it exists.
            IFC(m_pNodeCache->GetRenderTargetBitmap(
                ppIRTB,
                pDestRT 
                DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)
                ));
            
            goto Cleanup;
        }
        else
        {
            Assert(s_pDefaultCacheMode != NULL);
            pCacheModeForLookup = s_pDefaultCacheMode;
        }
    }

    // Look-up the corresponding cache token.
    BrushCacheToken *pCacheToken = LookupCache(pCacheModeForLookup);
    if (pCacheToken != NULL)
    {
        IFC(pCacheToken->pCache->GetRenderTargetBitmap(
            ppIRTB,
            pDestRT 
            DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)
            ));
    }
    else
    {
        // We shouldn't try to get a bitmap from a non-existent cache.
        Assert(false);
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCacheSet::LookupCache
//
// Synopsis: 
//    Returns cache token for the argument cache mode.
//
//-----------------------------------------------------------------------------

__out_opt BrushCacheToken*
CMilVisualCacheSet::LookupCache (
    __in CMilBitmapCacheDuce const *pCacheModeForLookup
    )
{
    for (UINT i = 0; i < m_arrBrushCaches.GetCount(); i++)
    {
        BrushCacheToken *pCacheToken = m_arrBrushCaches[i];
        if (pCacheModeForLookup == pCacheToken->pCacheModeNoRef)
        {
            return pCacheToken;
        }
    }

    return NULL;
}

