// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilVisualCache, MILRender, "VisualCache resource");

// If the following flag is set, the cache content updates are highlighted by 
// rendering the dirty region in a translucent color.
BOOL g_fDirtyRegion_ShowCacheDirtyRegions = FALSE;

const UINT g_CacheDirtyRegionColorCount = 3;

MilColorF g_CacheDirtyRegionColors[g_CacheDirtyRegionColorCount] =
    { /* {r, g, b, a} */
    { 0, 0.7f, 0, 0.5f },
    { 0, 0.7f, 0.7f, 0.5f },
    { 0.7f, 0.7f, 0, 0.5f }};

UINT g_CacheDirtyRegionColor = 0;

//+----------------------------------------------------------------------------
//
// CMilVisualCache::CMilVisualCache
//
//-----------------------------------------------------------------------------

CMilVisualCache::CMilVisualCache(
    __in_ecount(1) CComposition* pComposition,
    __in_ecount(1) CMilVisual *pVisual
    )
{
    m_pCompositionNoRef = pComposition;
    m_pVisualNoRef = pVisual;
    m_isDirty = true;
    m_needsFullUpdate = true;
    m_systemScaleX = m_systemScaleY = 1.0;
    Assert(m_isCachedInSoftware == false);
    Assert(m_pDisplaySet == NULL);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::~CMilVisualCache
//
//-----------------------------------------------------------------------------

CMilVisualCache::~CMilVisualCache()
{    
    ReleaseInterface(m_pDisplaySet);

    ReleaseDeviceResources();
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::IsOfType
//
// Synopsis:
//    Since this class is a wrapper for a BitmapCache resource, returns the
//    resource's type.
//
//-----------------------------------------------------------------------------
__override
bool
CMilVisualCache::IsOfType(MIL_RESOURCE_TYPE type) const
{
    FreAssert(FALSE);
    return false;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::OnChanged
//
// Synopsis: 
//    Changed handler.  If the wrapped BitmapCache resource changes, we need
//    to re-create the cache texture and propagate the changed flag.
//
//-----------------------------------------------------------------------------

BOOL
CMilVisualCache::OnChanged(
    CMilSlaveResource *pSender,
    NotificationEventArgs::Flags e
    )
{
    ReleaseDeviceResources();
    return TRUE;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::Create
//
// Synopsis: 
//    Factory method for creating CMilVisuaCaches.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCache::Create(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilVisual *pVisual,
        __deref_out_ecount(1) CMilVisualCache **ppCache
        )
{
    Assert(ppCache);
    
    HRESULT hr = S_OK;
    
    CMilVisualCache *pNewInstance = NULL;
        
    // Instantiate the wrapper
    pNewInstance = new CMilVisualCache(pComposition, pVisual);
    IFCOOM(pNewInstance);

    pNewInstance->AddRef();
    
    // Transfer ref to out argument
    *ppCache = pNewInstance;

    // Avoid deletion during Cleanup
    pNewInstance = NULL;

Cleanup:
    // Free any allocations that weren't set to NULL due to failure
    ReleaseInterface(pNewInstance);

    RRETURN(hr);    
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::ReleaseDeviceResources
//
// Synopsis: 
//    Cleans up device-dependent resources and resets the cache state.
//
//-----------------------------------------------------------------------------

void
CMilVisualCache::ReleaseDeviceResources()
{    
    // Throw away our cached texture.
    ReleaseInterface(m_pIRenderTargetBitmap);

    // Mark the cache as needing a full update.
    m_isDirty = true;
    m_needsFullUpdate = true;

    // Reset other flags.
    m_isCachedInSoftware = false;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::SetCacheMode
//
// Synopsis: 
//    Sets a new cache mode for this cache.
//
//-----------------------------------------------------------------------------

void 
CMilVisualCache::SetCacheMode(__in_opt CMilCacheModeDuce* pCacheMode)
{
    // Decrease the refcount for the previous cache mode.
    UnRegisterNotifier(m_pCacheMode);

    if (pCacheMode)
    {
        Assert(pCacheMode->IsOfType(TYPE_BITMAPCACHE));
        CMilBitmapCacheDuce *pBitmapCacheMode = static_cast<CMilBitmapCacheDuce*>(pCacheMode);
        m_pCacheMode = pBitmapCacheMode;

        // Add count for the new cache mode.
        if (m_pCacheMode)
        {
            RegisterNotifier(m_pCacheMode);
        }
    }

    // Since the cache mode changed we should ensure we reset the cache state.
    ReleaseDeviceResources();
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::GetScaleInflation
//
// Synopsis: 
//    Returns the amount to inflate a dirty rect in world space to account
//    for the scaled size of the cache in local space.
//
//-----------------------------------------------------------------------------
float
CMilVisualCache::GetScaleInflation()
{
    float inflation = 1.0f;
    if (m_pCacheMode != NULL)
    {
        auto scale = static_cast<float const>(m_pCacheMode->GetScale());
        if (!IsCloseReal(scale, 0.0f))
        {
            inflation = 1.0f / static_cast<float>(m_pCacheMode->GetScale());
        }
    }

    // Always inflate by at least one pixel in world space.
    return max(1.0f, inflation);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::GetRealizationDimensions
//
// Synopsis: 
//    Returns the surface bounds for the cache (after RenderScale).
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCache::GetRealizationDimensions(
    __in IRenderTargetInternal *pIRTInternal,
    __out MilRectF *pBounds
    )
{
    HRESULT hr = S_OK;

    // If the cached content has changed, update the cached surface bounds.
    if (!IsValid())
    {    
         double scale = m_pCacheMode->GetScale();

        // Caches are not clipped to the window bounds, they use local space bounds,
        // so (especially in combination with RenderScale) a very large intermediate
        // surface could be requested.  Instead of failing in this case, we clamp the 
        // surface to the max texture size, which can cause some pixelation but will 
        // allow the app to render in hardware and still benefit from a cache.
        DWORD renderTargetType = 0;
        IFC(pIRTInternal->GetType(&renderTargetType));
        UINT uMaxWidth;
        UINT uMaxHeight;
        if (renderTargetType == HWRasterRenderTarget)
        {            
            Assert(m_pDisplaySet);
            MilGraphicsAccelerationCaps caps;
            m_pDisplaySet->GetGraphicsAccelerationCaps(true, NULL, &caps);
            
            uMaxWidth = caps.MaxTextureWidth;
            uMaxHeight = caps.MaxTextureHeight;
        }
        else
        {
            Assert(renderTargetType == SWRasterRenderTarget || renderTargetType == DummyRenderTarget);
            // The width and height are converted to floats when clipping,
            // so we clamp to the largest value allowed for a cache SW intermediate.
            uMaxWidth = uMaxHeight = MAX_CACHE_SW_INTERMEDIATE_SIZE;
        }

        // Since the cache relies only on local space bounds, the DPI isn't taken into account (as it's the root
        // transform of the visual tree).  Scale for DPI if needed here.
        m_systemScaleX = DpiScale::PrimaryDisplayDpi().DpiScaleX;
        m_systemScaleY = DpiScale::PrimaryDisplayDpi().DpiScaleY;
        
        // We round our bounds up to integral values for consistency here, since we need to do so when creating the surface anyway.
        // This also ensures that our content will always be drawn in its entirety in the texture.
        //  Future Consideration:  Note that if we want to use the cache texture for TextureBrush or as input to Effects, we'll
        //          need to be able to toggle this "snap-out" behavior to avoid seams since Effects by default
        //          do NOT snap the size out, they round down to integral bounds.
        auto fWidth = m_rcLocalBounds.Width() * scale * m_systemScaleX;
        UINT uWidth = static_cast<UINT>(fWidth);
        // If our width was non-integer, round up.
        if (!IsCloseReal(static_cast<float>(uWidth), static_cast<float>(fWidth)))
        {
            uWidth += 1;
        }

        auto fHeight = m_rcLocalBounds.Height() * scale * m_systemScaleY;
        UINT uHeight = static_cast<UINT>(fHeight);
        // If our height was non-integer, round up.
        if (!IsCloseReal(static_cast<float>(uHeight), static_cast<float>(fHeight)))
        {
            uHeight += 1;
        }
        
        // Limit the size of the intermediate if necessary.
        if (uWidth > uMaxWidth)
        {
            m_systemScaleX *= static_cast<double>(uMaxWidth) / static_cast<double>(uWidth);
            uWidth = uMaxWidth;
        }
        if (uHeight > uMaxHeight)
        {
            m_systemScaleY *= static_cast<double>(uMaxHeight) / static_cast<double>(uHeight);
            uHeight = uMaxHeight;
        }

        m_cacheRealizationDimensions.left = m_cacheRealizationDimensions.top = 0.0f;
        m_cacheRealizationDimensions.right = static_cast<float>(uWidth);
        m_cacheRealizationDimensions.bottom = static_cast<float>(uHeight);
    }

    *pBounds = m_cacheRealizationDimensions;
    
Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::GetBounds
//
// Synopsis: 
//    Returns the transform from local space to the scaled surface space.
//
//-----------------------------------------------------------------------------

void
CMilVisualCache::GetLocalToSurfaceTransform(__out CMILMatrix *pTransform)
{   
    double scale = m_pCacheMode->GetScale();

    // The offset to the bounding box is important, for example, when the cache is placed on 
    // a panel like a Canvas, but its only visual content is offset inside it.
    // We need to un-offset to ensure our content is drawn into our texture starting at the
    // upper-left corner.
    // We scale the inverse offset since we want to render all our content scaled to size in the texture,
    // and when we come upon the offset walking the tree to render it will be under the scale transform.
    pTransform->SetToIdentity();
    pTransform->SetTranslation(-m_rcLocalBounds.left, -m_rcLocalBounds.top);
    pTransform->Scale(static_cast<float>(scale * m_systemScaleX), static_cast<float>(scale * m_systemScaleY));
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::Invalidate
//
// Synopsis: 
//    Marks the cache as dirty for update for the given regions and bounds.  
//    If full invalidation, we will ignore dirty regions in Update.
//
//-----------------------------------------------------------------------------

void
CMilVisualCache::Invalidate(
    bool fFullInvalidate,
    __in MilRectF const *prcLocalBounds
    )
{
    // A cache should always be updated in the cache-render pass immediately after the precompute pass.
    // The only other way it can be dirty here is if we need to do a full update (either this is the first time we've
    // drawn the cache, or we lost the device resource).
    Assert(m_needsFullUpdate || !m_isDirty);

    // Store the bounds.
    m_rcLocalBounds = *prcLocalBounds;
    
    // We need to update the cache (if it's not static).
    if (!m_pCacheMode->IsStatic())
    {
        if (fFullInvalidate)
        {
            m_needsFullUpdate = true;
        }
        
        m_isDirty = true;
    }
 }

//+----------------------------------------------------------------------------
//
// CMilVisualCache::IsValid
//
// Synopsis: 
//    Returns false if the contents of the cache are stale.  IsValid does not 
//    check device state, that's handled by NotifyDeviceLost.
//
//-----------------------------------------------------------------------------

bool
CMilVisualCache::IsValid() const
{
    return !m_isDirty && !m_needsFullUpdate;
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::NotifyDeviceLost
//
// Synopsis: 
//    The cache set marks the visual as dirty for pre-compute to ensure it is
//    re-rendered.  The cache needs to release its resources.
//
//-----------------------------------------------------------------------------

void
CMilVisualCache::NotifyDeviceLost()
{
    ReleaseInterface(m_pDisplaySet);
    
    ReleaseDeviceResources();
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::Update
//
// Synopsis: 
//    Brings the rendered content of the cache up to date.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCache::Update(
    __in IRenderTargetInternal* pIRTInternal,
    __in_opt CDirtyRegion2 *pDirtyRegion
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    HRESULT hr = S_OK;
    
    CDrawingContext* pDrawingContext = NULL;
    CMilRectF realizationDimensions;

    IRenderTargetInternal* pCacheIRT = NULL;

    // Check for cyclic update calls.  Since Update might cause something (like a Visual
    // Brush) to run another precompute and cache update pass, we want to prevent trying
    // to update this cache from within an Update call.
    if (EnterResource())
    {
        // We use the m_*Display* variables so we need to ensure they are valid.
        IFC(EnsureDisplaySet());
        
        // We should only call update if the cache is dirty.
        Assert(m_isDirty);

        // We need to pass dirty regions in if we're partially updating the cache.
        Assert(pDirtyRegion != NULL || m_needsFullUpdate);

        //
        // If we are rendering in hardware and have a resident display for the cache,
        // or we are rendering in software, we need to update the cache texture as it
        // might be used.
        //
        
        bool hasResidentDisplay = false;
        for (UINT i = 0; i < m_rgResidentDisplays.GetCount(); i++)
        {
            if (m_rgResidentDisplays[i])
            {
                hasResidentDisplay = true;
                break;
            }
        }

        DWORD parentRTType;
        IFC(pIRTInternal->GetType(&parentRTType));

        //
        // If we have not drawn to a display yet and the parent rt is a hardware rt, we
        // do not know yet on which display (think multi-mon/multi-adapter) to create the cache. 
        // Therefore we will wait with realizing the cache until the render pass. 
        // However, if the parent is a sw rt, we can update the software cache right away.
        // Also, if the cache has been realized before on a hw display, we update it right here too 
        // because it allows more time for the GPU to produce the texture before it is being used. 
        //
        
        if (hasResidentDisplay || 
            parentRTType == SWRasterRenderTarget) 
        {        
            IFC(GetRealizationDimensions(pIRTInternal, &realizationDimensions));
            
            // Compare bounds after scale.  We need to recreate our texture if they've changed.
            // We'll also need to do this if we're caching in the wrong rendering mode (hw vs sw).
            if (m_pIRenderTargetBitmap != NULL)
            {
                MilRectF currentCacheBounds;
                m_pIRenderTargetBitmap->GetBounds(&currentCacheBounds);

                if (!realizationDimensions.IsEquivalentTo(currentCacheBounds)
                    || HasRenderingModeChanged(parentRTType))
                {
                    // The cache can only cache in software or in hardware, but not in both. 
                    // If the WPF appliation runs in hardware on anything, the cache will be produced in hardware.
                    // For software RTs the bits are being pulled back from video memory. 
                    // This code is responsible for switching between the modes. 
                    ReleaseDeviceResources();
                }
            }
            
            if (!realizationDimensions.IsEmpty())
            {
                if (m_pIRenderTargetBitmap == NULL)
                {
                    IntermediateRTUsage rtUsage;
                    rtUsage.flags = IntermediateRTUsage::ForBlending;
                    rtUsage.wrapMode = MilBitmapWrapMode::Extend;

                    IFC(pIRTInternal->CreateRenderTargetBitmap(
                        static_cast<UINT>(realizationDimensions.Width()),
                        static_cast<UINT>(realizationDimensions.Height()),
                        rtUsage,
                        MilRTInitialization::Default,
                        &m_pIRenderTargetBitmap,
                        &m_rgResidentDisplays
                        ));

                    // ETW cache creation event.
                    EventWriteVisualCacheAlloc(
                        static_cast<const unsigned int>(realizationDimensions.left), 
                        static_cast<const unsigned int>(realizationDimensions.top),
                        static_cast<const unsigned int>(realizationDimensions.Width()),
                        static_cast<const unsigned int>(realizationDimensions.Height())
                        );
                    
                    // We must be able to cast this in order to render into it later, since DrawingContext does the same cast.
                    IFC(m_pIRenderTargetBitmap->QueryInterface(IID_IRenderTargetInternal, (void **) &pCacheIRT));
                    Assert(pCacheIRT);

                    DWORD rtType;
                    IFC(pCacheIRT->GetType(&rtType));
                    m_isCachedInSoftware = (rtType == SWRasterRenderTarget);
                    
                    // Force ClearType in our intermediate texture if the CacheMode so specifies.
                    if (m_pCacheMode->IsClearTypeEnabled())
                    {
                        IFC(pCacheIRT->SetClearTypeHint(true));
                    }
                }

                IFC(CDrawingContext::Create(m_pCompositionNoRef, &pDrawingContext));

                IFC(pDrawingContext->BeginFrame(
                    m_pIRenderTargetBitmap
                    DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)
                    ));

                //
                // We have already done the precompute walk, that's where the cache was invalidated.
                // Now we draw the visual tree into the render target.  
                //
                Assert(m_pVisualNoRef);

                MilColorF colBlank = { 0, 0, 0, 0 };

                // If we need to fully update the cache, ignore any dirty region information.
                //  Future Consideration:  We want to enable some control over the dirty regions and display information
                //         about caches and cache updates for the WPF Perf tools.
                if (m_needsFullUpdate)
                {   
                    // Push the transform for rendering into the cache.  The node's properties will be applied when the node itself
                    // is drawn in a separate render walk.
                    CMILMatrix matTransformToSurface;
                    GetLocalToSurfaceTransform(&matTransformToSurface);
                    IFC(pDrawingContext->PushTransform(&matTransformToSurface));
                    
                    // Draw the entire surface.
                    IFC(pDrawingContext->DrawCacheVisualTree(
                        m_pVisualNoRef,
                        &colBlank,
                        realizationDimensions
                        ));

                    // Pop the transform for drawing into the cache surface.
                    pDrawingContext->PopTransform();
                    
                    if (g_fDirtyRegion_ShowCacheDirtyRegions)
                    {
                        IFC(DrawRectangleOverlay(pDrawingContext, &realizationDimensions));
                    }

                    // ETW cache update event.
                    EventWriteVisualCacheUpdate(
                        static_cast<const unsigned int>(realizationDimensions.left),
                        static_cast<const unsigned int>(realizationDimensions.top),
                        static_cast<const unsigned int>(realizationDimensions.Width()),
                        static_cast<const unsigned int>(realizationDimensions.Height())
                        );
                    
                }
                else
                {
                    const MilRectF *arrDirtyRegions = pDirtyRegion->GetUninflatedDirtyRegions();

                    // Otherwise, only update the parts of the cache that are dirty.
                    const UINT dirtyRegionCount = pDirtyRegion->GetRegionCount();

                    // We transform dirty rects from local to surface space for drawing the visual tree
                    // directly into our cached texture.
                    CMILMatrix matLocalToSurface;
                    GetLocalToSurfaceTransform(&matLocalToSurface);
                    
                    for (UINT i = 0; i < dirtyRegionCount; i++)
                    {
                        CMilRectF renderBounds(arrDirtyRegions[i]);
                        
                        // Dirty regions are tracked in local space.  We need to render in our texture's space.
                        matLocalToSurface.Transform2DBounds(renderBounds, renderBounds);

                        // Inflate to ensure we did not round-off a pixel on any side when scaling the dirty rect.
                        InflateRectF_InPlace(&renderBounds);
                        
                        // Intersect the dirty region with the surface bounds.
                        if (renderBounds.Intersect(realizationDimensions))
                        {
                            // Push the transform for rendering into the cache.  The node's properties will be applied when the node itself
                            // is drawn in a separate render walk.
                            CMILMatrix matTransformToSurface;
                            GetLocalToSurfaceTransform(&matTransformToSurface);
                            IFC(pDrawingContext->PushTransform(&matTransformToSurface));                        

                            IFC(pDrawingContext->DrawCacheVisualTree(
                                m_pVisualNoRef,
                                &colBlank,
                                renderBounds
                                ));

                            // Pop the transform for drawing into the cache surface.
                            pDrawingContext->PopTransform();
                            
                            if (g_fDirtyRegion_ShowCacheDirtyRegions)
                            {
                                IFC(DrawRectangleOverlay(pDrawingContext, &renderBounds));
                            }
                            
                            // ETW cache update event.
                            EventWriteVisualCacheUpdate(
                                static_cast<const unsigned int>(renderBounds.left),
                                static_cast<const unsigned int>(renderBounds.top),
                                static_cast<const unsigned int>(renderBounds.Width()),
                                static_cast<const unsigned int>(renderBounds.Height())
                                );
                        }
                    }
                }
                
                pDrawingContext->EndFrame();
            }
        }
            
        m_isDirty = false;
        m_needsFullUpdate = false;
    }

Cleanup:
    // Leave the resource in a good state.
    LeaveResource();
    
    ReleaseInterface(pCacheIRT);
    ReleaseInterface(pDrawingContext);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::GetRenderTargetBitmap
//
// Synopsis: 
//    Returns the valid, up-to-date cache render target.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCache::GetRenderTargetBitmap (
    __deref_out_opt IMILRenderTargetBitmap ** ppIRTB,
    __in IRenderTargetInternal *pDestRT
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    HRESULT hr = S_OK;
    
    IRenderTargetInternal *pBaseIRT = NULL;
    DynArray<bool> arrActiveDisplays;
    
    // If we are rendering in hardware anywhere, we choose to render caches only in hardware.  
    // This means that in some scenarios (a VisualBrush in a software HWnd, certain cases of 
    // tiled TileBrushes) we might always render the cache bitmap in software 
    // which requires pulling the bits back from video memory each time the cache is updated.
    //
    // To conserve video memory, hardware caches are created for each display on demand.
    //
    // If we are rendering in software only, we'll cache in a software texture.  We are then
    // free to share that texture across displays.
    //
    // Ensure the cache is instantiated on the correct display.
    //  Future Consideration:  We may want to add a heuristic to free the hardware cache textures on displays 
    //         that haven't been rendered to in awhile.
    IFC(EnsureDisplaySet());

    // Get target display(s) from the destination RT.
    UINT cDisplays = m_pDisplaySet->GetDisplayCount();
    IFC(arrActiveDisplays.AddAndSet(cDisplays, false));
    IFC(pDestRT->ReadEnabledDisplays(&arrActiveDisplays));

    bool fNeedsDisplayUpdate = false;
    for (UINT i = 0; i < cDisplays; i++)
    {
        // If we need to render to a display we haven't created a backing texture for, 
        // we'll need to re-create our cached meta RT.
        if (arrActiveDisplays[i] && !m_rgResidentDisplays[i])
        {
            fNeedsDisplayUpdate = true;
            // Set residency for the new display.
            m_rgResidentDisplays[i] = true;
        }
    }

    // Get the base render interface, which determines whether we cache in hardware
    // or software.  If the base changed from one to the other and the cache hasn't
    // yet, update the cache.
    WHEN_DBG_ANALYSIS(CoordinateSpaceId::Enum dbgCoordSpaceId;)
    IFC(m_pCompositionNoRef->GetVisualCacheManagerNoRef()->GetBaseRenderInterface(
        &pBaseIRT 
        DBG_ANALYSIS_COMMA_PARAM(&dbgCoordSpaceId)
        ));

    DWORD rtType;
    IFC(pBaseIRT->GetType(&rtType));
    if (HasRenderingModeChanged(rtType))
    {
        fNeedsDisplayUpdate = true;
    }
    
    if (fNeedsDisplayUpdate)
    {
        // Mark the cache dirty and release its old RT.
        ReleaseDeviceResources();

        // Update our cache.
        IFC(Update(
            pBaseIRT,
            NULL // We released the device resources so we need to fully update, no need for dirty regions
            DBG_ANALYSIS_COMMA_PARAM(dbgCoordSpaceId)
            ));
    }
    
    if (m_pIRenderTargetBitmap != NULL)
    {
        m_pIRenderTargetBitmap->AddRef();
    }

    *ppIRTB = m_pIRenderTargetBitmap; // Pass ref or null to out arg

Cleanup:
    ReleaseInterface(pBaseIRT);
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::GetBitmapSource
//
// Synopsis: 
//    Returns the valid, up-to-date cache bitmap.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCache::GetBitmapSource (
    __deref_out_opt IWGXBitmapSource ** const ppIBitmapSource,
    __in IRenderTargetInternal *pDestRT
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    HRESULT hr = S_OK;

    IMILRenderTargetBitmap *pRTB = NULL;
    IWGXBitmapSource *pIBitmapSource = NULL;

    IFC(GetRenderTargetBitmap(
        &pRTB, 
        pDestRT
        DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)
        ));

    if (pRTB != NULL)
    {
        IFC(pRTB->GetBitmapSource(&pIBitmapSource));
    }

    *ppIBitmapSource = pIBitmapSource;
        
Cleanup:
    ReleaseInterface(pRTB);
    RRETURN(hr);
}
    
//+----------------------------------------------------------------------------
//
// CMilVisualCache::Render
//
// Synopsis: 
//    Draws this cache into the argument DrawingContext.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCache::Render(
    __in CDrawingContext *pDC,
    __in IRenderTargetInternal *pDestRT,
    float opacity
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    HRESULT hr = S_OK;

    IWGXBitmapSource *pBitmapSource = NULL;
    
    // We should never attempt to render a cache that is invalid.
    Assert(IsValid());

    // If our scale is zero we don't render anything.
    if (!IsCloseReal(static_cast<const float>(m_pCacheMode->GetScale()), 0.0f))
    {
        IFC(GetBitmapSource(
            &pBitmapSource,
            pDestRT
            DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)
            ));

        // If our bitmap source is null, we have nothing to render.
        if (pBitmapSource == NULL)
        {
            goto Cleanup;
        }
        
        MilRectF rcLocalBounds;
        GetLocalBounds(&rcLocalBounds);
        
        MilRectF rcSourceBounds;

        CMILMatrix matLocalToSurface;
        GetLocalToSurfaceTransform(&matLocalToSurface);

        matLocalToSurface.Transform2DBounds(rcLocalBounds, rcSourceBounds);

        // We handle snapping to device pixels by pushing an offset to snap to pixels 
        // in world space after the world transform has been applied.
        if (m_pCacheMode->SnapsToDevicePixels())
        {
            CMILMatrix matWorldTransform;
            pDC->GetWorldTransform(&matWorldTransform);
            
            MilRectF rcWorldBounds;
            matWorldTransform.Transform2DBounds(rcLocalBounds, rcWorldBounds);

            float snapOffsetX, snapOffsetY;
            snapOffsetX = rcWorldBounds.left - CFloatFPU::Floor(rcWorldBounds.left);
            snapOffsetY = rcWorldBounds.top - CFloatFPU::Floor(rcWorldBounds.top);
            IFC(pDC->PushTransformPostOffset(-snapOffsetX, -snapOffsetY));
        }
        
        // We will only update the dirty region here, since the Render pass this method is called
        // from will have our dirty rect pushed at the bottom of the clip stack, so there is no 
        // need to recalculate it here.
        IFC(pDC->DrawBitmap(pBitmapSource, &rcSourceBounds, &rcLocalBounds, opacity));

        if (m_pCacheMode->SnapsToDevicePixels())
        {
            pDC->PopTransform();
        }
    }

Cleanup:
    ReleaseInterface(pBitmapSource);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilVisualCache::EnsureDisplaySet
//
// Synopsis: 
//    After first cache update after creation or device lost, this call
//    initializes the cache to exist on zero displays - we will lazily
//    create the cache on each display when we encounter it.
//
//-----------------------------------------------------------------------------

HRESULT
CMilVisualCache::EnsureDisplaySet()
{
    HRESULT hr = S_OK;

    if (m_pDisplaySet == NULL)
    {
        g_DisplayManager.GetCurrentDisplaySet(&m_pDisplaySet);

        // Initialize our cache residency to false for each display.
        UINT cDisplays = m_pDisplaySet->GetDisplayCount();
        m_rgResidentDisplays.Reset(FALSE);
        IFC(m_rgResidentDisplays.AddAndSet(cDisplays, false));
    }

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CMilVisualCache::DrawRectangleOverlay
//
// Description:
//      This function overlays alternating transparent colored windows on the
//      parameter rectangle. Designed to be used with the debug tools allowing
//      display of the dirty regions being re-rendered.
//---------------------------------------------------------------------------------

HRESULT 
CMilVisualCache::DrawRectangleOverlay(
    __in_ecount(1) CDrawingContext *pDC,
    __in_ecount(1) CMilRectF const *renderBounds
    )
{
    HRESULT hr = S_OK;

    CMilPointAndSizeF renderBoundsXYWH(
        renderBounds->left,
        renderBounds->top,
        renderBounds->right - renderBounds->left,
        renderBounds->bottom - renderBounds->top
    );

    g_CacheDirtyRegionColor = g_CacheDirtyRegionColor % g_CacheDirtyRegionColorCount;
    
    IFC(pDC->DrawRectangle(
        &(g_CacheDirtyRegionColors[g_CacheDirtyRegionColor]),
        &renderBoundsXYWH
        ));

    g_CacheDirtyRegionColor++;
    
Cleanup:
    RRETURN(hr);
}


