// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      See BrushIntermediateCache.cpp header
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CBrushIntermediateCache);

struct BrushCachingParameters;

//+-----------------------------------------------------------------------------
//
//  Structure:
//      CachedBrushRealizationState
//
//  Synopsis:
//      Contains all the state needed to determine whether or not a cached brush
//      realization can be re-used.
//
//------------------------------------------------------------------------------
struct CachedBrushRealizationState
{    
    // Bounds of content the intermediate was created for, with only
    // the scale portion of the Viewport->SampleSpace transform applied
    CMilRectF rcContentBounds_SampleScaledViewportSpace;

    // Bounds of  intermediate surface, scaled by the Viewport->SampleSpace transform
    // that's in effect when the intermediate was created.
    CMilRectF rcIntermediateBounds_SampleScaledViewportSpace;

    // Integer bounds of the intermediate surface without any transformation
    CMilRectF rcIntermediateBounds_SurfaceSpace;
};

class CBrushIntermediateCache
{
protected:
    
    CBrushIntermediateCache()
    {
        m_pCachedIntermediate = NULL;        
        m_uRealizationCacheIndex = CMILResourceCache::InvalidToken;       
    }   

public:    

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CBrushIntermediateCache));    

    virtual ~CBrushIntermediateCache()
    {
        ReleaseInterfaceNoNULL(m_pCachedIntermediate);
    }     

    static HRESULT Create(
        __deref_out_ecount(1) CBrushIntermediateCache **ppBrushIntermediateCache
        );

    VOID FindIntermediate(
        UINT uAdapterIndex,
        __in_ecount(1) const BrushCachingParameters *pCachingParams,
        __in_ecount_opt(1) const CMILMatrix *pmatScaleOfViewportToWorld,
        __in_ecount(1) const CMILMatrix &matScaleOfWorldToSampleSpace,
        __in_ecount(1) const CMILMatrix &matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace,
        __in_ecount(1) const CMilRectF &rcIntermediateBounds_SurfaceSpace,
        __deref_out_ecount_opt(1) IWGXBitmapSource **ppBitmapSource,
        __out_ecount(1) CachedBrushRealizationState *pCachedRealizationState
        );

    VOID StoreIntermediate(
        __in_ecount(1) IWGXBitmapSource *pCacheableIntermediate,        
        UINT uRealizationCacheIndex,
        __in_ecount(1) CachedBrushRealizationState &cachedState
        );

    VOID InvalidateCache()
    {
        m_uRealizationCacheIndex = CMILResourceCache::InvalidToken;
        ReleaseInterface(m_pCachedIntermediate);
    }    

private:    

    VOID FindValidIntermediate(
        UINT uAdapterIndex,
        __deref_out_ecount_opt(1) IWGXBitmapSource **ppValidIntermediate
        );

    VOID CalculateCachedBrushRealizationState(
        __in_ecount(1) const CMilRectF &rcCurrentContentBounds_ViewportSpace,
        __in_ecount(1) const CMilRectF &rcIntermediateBounds_SurfaceSpace,
        __in_ecount_opt(1) const CMILMatrix *pmatScaleOfViewportToWorld,
        __in_ecount(1) const CMILMatrix &matScaleOfWorldToSampleSpace,
        __in_ecount(1) const CMILMatrix &matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace,
        __out_ecount(1) CachedBrushRealizationState *pCachedRealizationState    
        );

    bool CanIntermediateBeReused(
        __in_ecount(1) CMilRectF &rcCurrentContentBounds_SampleScaledViewportSpace,
        __in_ecount(1) CMilRectF &rcCurrentIntermediateBounds_SampleScaledViewportSpace,
        float rCacheInvalidationThresholdMinimum,
        float rCacheInvalidationThresholdMaximum
        );

    static bool IsDimensionWithinCachingThreshold(
        float originalValue, 
        float newValue, 
        float shrinkThreshold,
        float expandThreshold
        );

private:
    IWGXBitmapSource *m_pCachedIntermediate;
    UINT m_uRealizationCacheIndex;   
    CachedBrushRealizationState m_cachedState;    
};



