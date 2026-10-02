// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Visual cache resource header.
//
//      A visual cache repesents a cached texture for a given Visual as
//      described by its CacheMode.  There may be multiple VisualCaches sharing
//      a single CacheMode, and more than one VisualCache in a VisualCacheSet
//      for the given Visual.
//
//-----------------------------------------------------------------------------

MtExtern(CMilVisualCache);

class CMilCacheModeDuce;
class CMilBitmapCacheDuce;

class CMilVisualCache : public CMilSlaveResource
{

public:
    
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilVisualCache));

    static HRESULT Create(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilVisual *pVisual,
        __deref_out_ecount(1) CMilVisualCache **ppCache
        );

    ~CMilVisualCache();
  
    CMilBitmapCacheDuce* GetCacheMode()
    {
        return m_pCacheMode;
    }

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const;

    override virtual BOOL OnChanged(
        CMilSlaveResource *pSender,
        NotificationEventArgs::Flags e
        );
    
    void SetCacheMode(__in_opt CMilCacheModeDuce* pCacheMode);

    float GetScaleInflation();

    void Invalidate(
        bool fFullInvalidate,
        __in MilRectF const *prcLocalBounds
        );

    bool IsValid() const;

    void NotifyDeviceLost();

    HRESULT Update(
        __in IRenderTargetInternal* pIRTInternal,
        __in_opt CDirtyRegion2 *pDirtyRegion
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );

    HRESULT Render(
        __in CDrawingContext *pDC,
        __in IRenderTargetInternal *pDestRT,
        float opacity
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );

    HRESULT GetRenderTargetBitmap (
        __deref_out_opt IMILRenderTargetBitmap ** ppIRTB,
        __in IRenderTargetInternal *pDestRT
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );

    HRESULT GetBitmapSource (
        __deref_out_opt IWGXBitmapSource ** const ppIBitmapSource,
        __in IRenderTargetInternal *pDestRT
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );

private:
    
    CMilVisualCache(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilVisual *pVisual
        );

    void ReleaseDeviceResources();
      
    HRESULT GetRealizationDimensions(
            __in IRenderTargetInternal *pIRTInternal,
            __out MilRectF *pBounds
            );
    
    void GetLocalBounds(__out MilRectF *pBounds) const
    {
        *pBounds = m_rcLocalBounds;
    }    

    void GetLocalToSurfaceTransform(__out CMILMatrix *pTransform);
    
    HRESULT EnsureDisplaySet();

    static HRESULT DrawRectangleOverlay(
        __in_ecount(1) CDrawingContext *pDC,
        __in_ecount(1) CMilRectF const *renderBounds
        );

    inline bool HasRenderingModeChanged(DWORD parentType) const
    {
        return ((parentType == HWRasterRenderTarget) && m_isCachedInSoftware)
            || ((parentType == SWRasterRenderTarget) && !m_isCachedInSoftware);
    }
    
private:

    // The local space inner bounds of our cached element.
    CMilRectF m_rcLocalBounds;

    // The cached bounds of our intermediate surface.
    MilRectF m_cacheRealizationDimensions;
    
    // Scale transform amounts accounting for max texture limitations and DPI.
    double m_systemScaleX;
    double m_systemScaleY;

    CComposition* m_pCompositionNoRef;
    CMilBitmapCacheDuce *m_pCacheMode;

    // The visual we are caching.
    CMilVisual *m_pVisualNoRef;

    const CDisplaySet *m_pDisplaySet;
    
    // Our cached texture.
    IMILRenderTargetBitmap *m_pIRenderTargetBitmap;
    
    // Residency information.
    DynArray<bool> m_rgResidentDisplays;

    // The cache can either cache in software or in hardware, but not 
    // in both at the same time.
    bool m_isCachedInSoftware;
    
    // True if the cache needs to be updated.
    bool m_isDirty;

    // True if the cache needs to be fully redrawn.
    bool m_needsFullUpdate;
};

