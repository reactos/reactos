// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Visual cache set resource header.
//
//      A visual cache set contains all cached textures containing a given
//      Visual's content and subtree.  The Visual may have one cache attached to
//      it directly (via its VisualCacheMode property), and it can have any
//      number of additional caches targeting it via BitmapCacheBrush.
//
//-----------------------------------------------------------------------------

MtExtern(CMilVisualCacheSet);

struct BrushCacheToken
{
    CMilBitmapCacheDuce *pCacheModeNoRef;
    CMilVisualCache *pCache;
    UINT refCount;
};

class CMilVisualCacheSet : public CMilSlaveResource
{

public:
    
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilVisualCacheSet));

    static HRESULT Create(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilVisual *pVisual,
        __deref_out_ecount(1) CMilVisualCacheSet **ppCache
        );

    ~CMilVisualCacheSet();

    //
    // CMilSlaveResource methods
    //
    
    override BOOL OnChanged(
        CMilSlaveResource *pSender, 
        NotificationEventArgs::Flags e
        );

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const;

    UINT GetCount() const;

    //
    // Node cache methods.  These are used by CMilVisual and CDrawingContext to interact with
    // the cache specified by m_pVisualNoRef's managed CacheMode property.
    //
    
    __out_opt CMilBitmapCacheDuce* GetNodeCacheMode();

    HRESULT SetNodeCacheMode(__in_opt CMilCacheModeDuce* pCacheMode);

    HRESULT GetNodeCacheRenderTargetBitmap (
        __deref_out_opt IMILRenderTargetBitmap ** ppIRTB,
        __in IRenderTargetInternal *pDestRT
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );
    
    bool IsNodeCacheValid() const;
    
    HRESULT RenderNodeCache(
        __in CDrawingContext *pDC,
        __in IRenderTargetInternal *pDestRT,
        float opacity
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );

    float GetNodeCacheScaleInflation();
    
    //
    // Cache invalidation methods.  These methods affect all caches interacting with this node,
    // including the node's cache and any brush caches targeting the node.
    //

    bool IsValid() const;
    
    HRESULT FullInvalidate(
        __in MilRectF const *prcLocalBounds
        );

    void BeginPartialInvalidate(
        __in float allowedDirtyRegionOverhead,
        __deref_out CDirtyRegion2 **ppDirtyRegionsNoRef
        );

    HRESULT EndPartialInvalidate(
        __in MilRectF const *prcLocalBounds
        );

    void NotifyDeviceLost();

    HRESULT Update(
        __in IRenderTargetInternal* pIRTInternal
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );

    //
    //  BitmapCacheBrush supporting methods.  Used to register, get, and remove caches for a 
    //  bitmap cache brush targeted at m_pVisualNoRef.
    //
    HRESULT AddCache(
        __in_opt CMilBitmapCacheDuce *pBitmapCacheMode
        );

    bool RemoveCache(
        __in_opt CMilBitmapCacheDuce const *pBitmapCacheMode
        );
    
    HRESULT GetBitmapSource (
        __in_opt CMilBitmapCacheDuce const *pCacheMode,
        __in IRenderTargetInternal *pIRT,
        __deref_out_opt IWGXBitmapSource ** const ppIBitmapSource
        );

    HRESULT GetRenderTargetBitmap (
        __in_opt CMilBitmapCacheDuce const *pCacheMode,
        __deref_out_opt IMILRenderTargetBitmap ** ppIRTB,
        __in IRenderTargetInternal *pDestRT
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );
       
private:
    
    CMilVisualCacheSet(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilVisual *pVisual
        );

    HRESULT AddCacheInternal(
        __in_opt CMilBitmapCacheDuce *pBitmapCacheMode,
        __in UINT refCount
        );

    bool RemoveCacheInternal(
        __in_opt CMilBitmapCacheDuce const *pBitmapCacheMode,
        __in UINT refCount
        );

    __out_opt BrushCacheToken* LookupCache (
        __in CMilBitmapCacheDuce const *pCacheMode
        );
    
    
private:

    // The cache for m_pVisualNoRef, specified by setting the CacheMode property
    // on that Visual in managed code.
    CMilVisualCache *m_pNodeCache;

    // A count of brushes that are using the node cache or default cache.
    UINT m_cUnspecifiedBrushes;
    
    // The caches for any BitmapCacheBrushes targeting m_pVisualNoRef.
    DynArray<BrushCacheToken*> m_arrBrushCaches;
    
    CComposition* m_pCompositionNoRef;

    // The dirty region tracker for this cache set.
    CDirtyRegion2 m_dirtyRegion;
    
    // The visual we are caching.
    CMilVisual *m_pVisualNoRef;

    // The default cache mode specifier.
    static CMilBitmapCacheDuce *s_pDefaultCacheMode;
};

