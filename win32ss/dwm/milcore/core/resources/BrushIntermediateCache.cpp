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
//      - TileBrush intermediate caching support
//
//      This class is responsible for maintaining references to cached
//      intermediate surfaces, and determining whether or not they can be
//      reused.  Currently this class supports only one cached intermediate for
//      all adapters, but that could be expanded in the future to one
//      intermediate per adapter, or even multiple intermediates per adapter in
//      multi- use scenarios.
//
//      When enabled, we attempt to re-use the intermediate surfaces whenever
//      possible. Re-use is allowed when a cached surface exists (i.e.,
//      FindValidIntermediate determines that a surface was cached & hasn't been
//      invalidated), and the content we are rendering to a surface doesn't
//      change (i.e., the Image/Drawing/Visual doesn't change at all, and
//      neither do any brush properties).  When both of those are true,
//      CanIntermediateBeReused determines whether or not the 'world' or
//      'context' the brush is used has changed.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#include "precomp.hpp"

MtDefine(CBrushIntermediateCache, TileBrushResource, "CBrushIntermediateCache");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushIntermediateCache::Create
//
//  Synopsis:
//      Static factory method which instantiates a fully-constructed
//      CBrushIntermediateCache, or returns failure if it can't.
//
//------------------------------------------------------------------------------
HRESULT 
CBrushIntermediateCache::Create(
    __deref_out_ecount(1) CBrushIntermediateCache **ppBrushIntermediateCache
        // Output newly-instantiated intermediate surface
    )
{
    HRESULT hr = S_OK;
    
    *ppBrushIntermediateCache = new CBrushIntermediateCache();
    IFCOOM(*ppBrushIntermediateCache);
    
Cleanup:
    RRETURN(hr);                
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushIntermediateCache::StoreIntermediate
//
//  Synopsis:
//      Public method which calculates the ideal cache-reuse parameters for the
//      current world/context state, searches for a cached intermediate
//      associated with uAdapterIndex, and if one exists, compares the ideal
//      parameters against the cached parameters to determine if the cached
//      intermediate can be reused.
//
//------------------------------------------------------------------------------
VOID 
CBrushIntermediateCache::StoreIntermediate(
    __in_ecount(1) IWGXBitmapSource *pCacheableIntermediate,
        // Intermediate to cache
    UINT uRealizationCacheIndex,
        // Adapter index pCachableIntermediate was created for 
    __in_ecount(1) CachedBrushRealizationState &cachedState
        // Cached re-use parameters calculated during FindIntermediate
    )
{
    Assert(pCacheableIntermediate);

    // InvalidToken is used to denote an invalid cache by this class, so it
    // shouldn't also be used as an index.  This is guaranteed by the caller.
    Assert(uRealizationCacheIndex != CMILResourceCache::InvalidToken);
    
#if DBG
    if (uRealizationCacheIndex != CMILResourceCache::SwRealizationCacheIndex)
    {
        CDeviceBitmap *pDeviceBitmap = DYNCAST(CDeviceBitmap, pCacheableIntermediate);
        
        Assert(pDeviceBitmap);
        Assert(pDeviceBitmap->HasValidDeviceBitmap());
    }
#endif

    ReplaceInterface(m_pCachedIntermediate, pCacheableIntermediate);
    m_uRealizationCacheIndex = uRealizationCacheIndex;
    m_cachedState = cachedState;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushIntermediateCache::FindIntermediate
//
//  Synopsis:
//      Public method which calculates the ideal cache-reuse parameters for the
//      current world/context state, searches for a cached intermediate
//      associated with uAdapterIndex, and if one exists, compares the ideal
//      parameters against the cached parameters to determine if the cached
//      intermediate can be reused.
//
//------------------------------------------------------------------------------
VOID 
CBrushIntermediateCache::FindIntermediate(
    UINT uAdapterIndex,
        // Adapter index to find a intermediate for
    __in_ecount(1) const BrushCachingParameters *pCachingParams,
        // In-parameters needed to determine if the saved intermediate is still valid
    __in_ecount_opt(1) const CMILMatrix *pmatScaleOfViewportToWorld,
        // Scale portion of the Viewport->World transform
    __in_ecount(1) const CMILMatrix &matScaleOfWorldToSampleSpace,
        // Scale portion of the World->SampleSpace transform
    __in_ecount(1) const CMILMatrix &matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace,
        // Matrix which transforms that calculated intermediate surface to the ideal surface
    __in_ecount(1) const CMilRectF &rcIntermediateBounds_SurfaceSpace,
        // Bounds of the calculated intermediate
    __deref_out_ecount_opt(1) IWGXBitmapSource **ppBitmapSource,
        // Output cached intermediate.  Will be NULL if no bitmap source was found.
    __out_ecount(1) CachedBrushRealizationState *pCachedRealizationState
        // Output state needed to cache this realization.  If the *ppBitmapSource
        // is non-NULL, then pCachedRealizationState == pCachingParams.  Otherwise,
        // it's set to the new re-use parameters calculated for the current context.
    )
{
    *ppBitmapSource = NULL;

    //
    // Setup pCachedRealizationState for the case that the current intermediate cannot be used.
    //
    // If the intermediate can be reused, pCachedRealizationState will be changed to the 
    // cached intermediate's state before returning.
    // 
    
    CalculateCachedBrushRealizationState(
        IN pCachingParams->rcCurrentContentBounds_ViewportSpace,
        IN rcIntermediateBounds_SurfaceSpace,
        IN pmatScaleOfViewportToWorld,
        IN matScaleOfWorldToSampleSpace,
        IN matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace,
        OUT pCachedRealizationState
        );

    //
    // Perform a cache lookup for a valid intermediate
    //

    IWGXBitmapSource *pValidIntermediate = NULL;
    FindValidIntermediate(uAdapterIndex, &pValidIntermediate);

    //
    // Try to re-use the intermediate, if one was found
    //
    
    if (NULL != pValidIntermediate)
    {
        if (CanIntermediateBeReused(
            pCachedRealizationState->rcContentBounds_SampleScaledViewportSpace,
            pCachedRealizationState->rcIntermediateBounds_SampleScaledViewportSpace,
            pCachingParams->rCacheInvalidationThresholdMinimum,
            pCachingParams->rCacheInvalidationThresholdMaximum
            ))
        {
            //
            // Success!  We can re-use the cached intermediate.  
            // Set the bitmap pointer & cached state to those of the cached intermediate
            //
            *ppBitmapSource = pValidIntermediate;
            pValidIntermediate = NULL;   // Steal ref

            *pCachedRealizationState = m_cachedState;
        }
    }

    // Release the reference obtained for pValidIntermediate if it's still valid 
    ReleaseInterfaceNoNULL(pValidIntermediate);    
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushIntermediateCache::FindValidIntermediate
//
//  Synopsis:
//      Determines whether or not a intermediate for the adapter index exists in
//      the cache
//
//  Future Consideration:  Add content protection support if the brush
//  intermediate cache is ever used to cache protected content. To do this we'd
//  need the ability to store and retrieve intermediates by display index
//  instead of cache index.
//
//------------------------------------------------------------------------------
VOID
CBrushIntermediateCache::FindValidIntermediate(
    UINT uAdapterIndex,
        // Adapter index to find a intermediate for
    __deref_out_ecount_opt(1) IWGXBitmapSource **ppValidIntermediate
        // Output cached valid intermediate.  Will be NULL if no valid intermediatewas found.
        )
{
    *ppValidIntermediate = NULL;

    if (m_uRealizationCacheIndex == CMILResourceCache::SwRealizationCacheIndex)
    {
        // Software intermediates can be used by any device, and don't
        // need to be checked for validity.  
        //
        // In single-mon scenarios, m_uRealizationCacheIndex will typically
        // be the actual HW index and not SwRealizationCacheIndex.  But 
        // one case where this does happen is 3D over TS, when a HW-render 
        // target creates (then caches) a SW intermediate.  In multi-
        // mon scenarios this can happen when the brush spans multiple
        // monitors, and at least one of those is SW.
        SetInterface(*ppValidIntermediate, m_pCachedIntermediate);              
    }
    // Do we have an intermediate for this adapter?
    else if (m_uRealizationCacheIndex == uAdapterIndex)
    {
        //
        // Ensure the cached intermediate is still valid
        //
        CDeviceBitmap *pDeviceBitmap = DYNCAST(CDeviceBitmap, m_pCachedIntermediate);
        Assert(pDeviceBitmap);

        if (pDeviceBitmap->HasValidDeviceBitmap())
        {
            // We currently do not support restricted content even in the case
            // where we could- when it would work on the current display.
            SetInterface(*ppValidIntermediate, m_pCachedIntermediate);   
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushIntermediateCache::CalculateCachedBrushRealizationState
//
//  Synopsis:
//      Calculates the CachedBrushRealizationState state (3 rectangles) needed
//      by CanIntermediateBeReused to determine whether or not an intermediate
//      can be re-used.
//
//------------------------------------------------------------------------------
VOID 
CBrushIntermediateCache::CalculateCachedBrushRealizationState(
    __in_ecount(1) const CMilRectF &rcCurrentContentBounds_ViewportSpace,
        // Current content bounds in Viewport space
    __in_ecount(1) const CMilRectF &rcIntermediateBounds_SurfaceSpace,
        // Bounds of the intermediate in surface space (i.e., 0, 0, Width, Height)
    __in_ecount_opt(1) const CMILMatrix *pmatScaleOfViewportToWorld,
        // Scale portion of the Viewport->World transform
    __in_ecount(1) const CMILMatrix &matScaleOfWorldToSampleSpace,
        // Scale portion of the World->SampleSpace transform
    __in_ecount(1) const CMILMatrix &matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace,
        // Matrix which transforms that calculated intermediate surface to the ideal surface
    __out_ecount(1) CachedBrushRealizationState *pCachedRealizationState
        // Output state needed to cache a new realization based on the the current brush state
    )    
{        
    // 
    // Transform the current content bounds into sample-scaled space
    //
    
    CMilRectF rcCurrentContentBounds_WorldScaledViewportSpace;

    // First, apply scale of viewport->world transform, if one exists
    if (pmatScaleOfViewportToWorld)
    {
        pmatScaleOfViewportToWorld->Transform2DBounds(                
            IN rcCurrentContentBounds_ViewportSpace,
            OUT rcCurrentContentBounds_WorldScaledViewportSpace
            );
    }
    else
    {
        rcCurrentContentBounds_WorldScaledViewportSpace = rcCurrentContentBounds_ViewportSpace;
    }

    // Then apply scale of world->sample space transform
    matScaleOfWorldToSampleSpace.Transform2DBounds(
        IN rcCurrentContentBounds_WorldScaledViewportSpace,
        OUT pCachedRealizationState->rcContentBounds_SampleScaledViewportSpace
        );

    //
    // Transform current intermediate bounds into sample-scaled viewport space.
    //

    matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace.Transform2DBounds(
        IN rcIntermediateBounds_SurfaceSpace,
        OUT pCachedRealizationState->rcIntermediateBounds_SampleScaledViewportSpace
        );

    // Set the last pCachedRealizationState member
    pCachedRealizationState->rcIntermediateBounds_SurfaceSpace = rcIntermediateBounds_SurfaceSpace;    
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushIntermediateCache::CanIntermediateBeReused
//
//  Synopsis:
//      Determines whether or not the world/context has changed so much that a
//      cached intermediate cannot be re-used, either because re-using the
//      intermediate would be functionally incorrect (e.g., because the
//      intermediate doesn't have the right content in it), or would result in
//      too large of a quality loss (i.e., the cached surface would be stretched
//      too much).  An acceptable threshold for quality loss due to stretching
//      is subjective and scenario-dependent, so that option has been exposed
//      via API.   This method is only called after we've determine that the
//      brush hasn't changed, and that a cached intermediate actually exists.
//
//      There are 2 situations where a previously cached intermediate surface
//      cannot be re-used due to changes in the world/context. They are detected
//      in the following order:
//
//      1)  Stretching/shrinking the intermediate such that the quality loss is too great.
//
//      2)  The visible portion of the content doesn't currently reside in the intermediate 
//          because it was clipped out previously.  
//
//          For non-tiled cases, the content which resides in the cached
//          intermediate must be a superset of the content that is currently
//          visible.  This is even more constrained when tiling -- the visible
//          portion of the content has to be exactly the same since the surface
//          must be tiled in it's entirety (non-tiled cases can get around this
//          by applying a source-clip to unneeded portions of the surface).   To
//          simplify implementation, both the non-tiled and tiled cases are
//          currently constrained to equality (super-set re-use isn't
//          supported).
//
//      - #1 Implementing scaling threshold detection
//
//      What we're trying to determine here is how much the content (i.e., the
//      Image, Drawing, etc.) would be scaled if we were to re-use the
//      intermediate.  To do that, we need to compare the content bounds scaled
//      into sample/device-space from when the intermediate was created, versus
//      now.  If their width or height is beyond the user-specified maximum, we
//      won't re-use the cache.
//
//      - #2 Implementing detection of clipped content changes
//
//      Once we've determined that we are within the stretching/shrinking
//      theshold, we have to detect differences caused by clipping. What we're
//      trying to determine here is if the content inside the cached
//      intermediate surface is equivalent-to the content that would exist in a
//      re-created surface.  That is, is the bounds of content in the cached
//      intermediate surface (I'll call this the 'clipped content') unchanged?
//
//      The first key is to realize what the 'clipped content' is.  It is the
//      entire intermediate surface.  That is, the bounds of the intermediate
//      surface completely describe the bounds of the clipped content we need to
//      compare.  But to compare the bounds of the two intermediate surfaces
//      (the cached surface, and one we may re-create), we need to transform
//      both sets of bounds into a common coordinate space.
//
//      The challenge there is to do the comparison in the device units the
//      surface will be re-used in (the current world->device/sample-space
//      transform) , even though the world->device transform the surface was
//      originally created for is different.  As a sidenote, the reason we do
//      this comparison in device-units is to use an epsilon which allows for
//      small differences caused by floating-point error, while failing (i.e.,
//      evaluating to false) when the difference would be perceivable.
//
//      To obtain a coordinate space common to both surfaces, we create a
//      scaling/translation transformation from the previous sample-scaled
//      content bounds to the current sample- scaled content bounds. This works
//      because these bounds are a common link between the two coordindate
//      spaces (i.e., they contain the same thing, with their only difference
//      being their position & width/height, or translate & scale).
//      Additionally, because skews & rotations have been factored out, we can
//      infer a transform between these bounds using a simple rectangle mapping,
//      and the transformed bounds will remain axis-aligned.
//
//      Once both sets of bounds are within the same sample-scaled coordinate
//      space, they are compared using a fuzzy equality tolerance based on
//      device pixels.
//
//------------------------------------------------------------------------------
bool 
CBrushIntermediateCache::CanIntermediateBeReused(
    __in_ecount(1) CMilRectF &rcCurrentContentBounds_SampleScaledViewportSpace,
        // Bounds of content with only the scale portion of the Viewport->SampleSpace 
        // transform applied
    __in_ecount(1) CMilRectF &rcCurrentIntermediateBounds_SampleScaledViewportSpace,
        // Bounds of  intermediate surface, scaled by the Viewport->SampleSpace transform       
    float rCacheInvalidationThresholdMinimum,
        // Shrink threshold beyond which the intermediate cannot be re-used
    float rCacheInvalidationThresholdMaximum
        // Expand threshold beyond which the intermediate cannot be re-used    
    )    
{
    bool fCanReuseIntermediate = false;

    //
    // #1 Implementation of scaling threshold detection
    //   
    // Compare the previous sample-space scaled content bounds to the current bounds
    //
    
    if (IsDimensionWithinCachingThreshold(
            m_cachedState.rcContentBounds_SampleScaledViewportSpace.Width(),
            rcCurrentContentBounds_SampleScaledViewportSpace.Width(), 
            rCacheInvalidationThresholdMinimum,
            rCacheInvalidationThresholdMaximum
            ) &&           
        IsDimensionWithinCachingThreshold(
            m_cachedState.rcContentBounds_SampleScaledViewportSpace.Height(),
            rcCurrentContentBounds_SampleScaledViewportSpace.Height(), 
            rCacheInvalidationThresholdMinimum,
            rCacheInvalidationThresholdMaximum
            )
       )
    {
        //
        // #2 Detection of clipped content changes 
        //    
        
        CMilRectF rcCachedIntermediateBounds_SampleScaledViewportSpace;
        
        //
        // Transform old intermediate bounds from the previous sample-scaled
        // viewport space to the current sample-scaled viewport space.  This is
        // neccessary because the World->SampleSpace transform between now and
        // when the intermediate was cached can be, and often are, different. 
        // See the full description of this operation in the banner comment.
        //
        // General linear transform from old to new:
        //      I.new = (I.old-C.old) * (new/old) + C.new
        // where I = Intermediate and C = Content
        //

        {
            float rScaleXCachedToCurrent =
                rcCurrentContentBounds_SampleScaledViewportSpace.Width() /
                m_cachedState.rcContentBounds_SampleScaledViewportSpace.Width();

            rcCachedIntermediateBounds_SampleScaledViewportSpace.left =
                (  (m_cachedState.rcIntermediateBounds_SampleScaledViewportSpace.left
                    - m_cachedState.rcContentBounds_SampleScaledViewportSpace.left)
                 * rScaleXCachedToCurrent)
                + rcCurrentContentBounds_SampleScaledViewportSpace.left;

            rcCachedIntermediateBounds_SampleScaledViewportSpace.right =
                (  (m_cachedState.rcIntermediateBounds_SampleScaledViewportSpace.right
                    - m_cachedState.rcContentBounds_SampleScaledViewportSpace.left)
                 * rScaleXCachedToCurrent)
                + rcCurrentContentBounds_SampleScaledViewportSpace.left;
        }

        {
            float rScaleYCachedToCurrent =
                rcCurrentContentBounds_SampleScaledViewportSpace.Height() /
                m_cachedState.rcContentBounds_SampleScaledViewportSpace.Height();

            rcCachedIntermediateBounds_SampleScaledViewportSpace.top =
                (  (m_cachedState.rcIntermediateBounds_SampleScaledViewportSpace.top
                    - m_cachedState.rcContentBounds_SampleScaledViewportSpace.top)
                 * rScaleYCachedToCurrent)
                + rcCurrentContentBounds_SampleScaledViewportSpace.top;

            rcCachedIntermediateBounds_SampleScaledViewportSpace.bottom =
                (  (m_cachedState.rcIntermediateBounds_SampleScaledViewportSpace.bottom
                    - m_cachedState.rcContentBounds_SampleScaledViewportSpace.top)
                 * rScaleYCachedToCurrent)
                + rcCurrentContentBounds_SampleScaledViewportSpace.top;
        }


        //
        // Determines whether or not the clipped content has changed beyond the
        // INSIGNIFICANT_PIXEL_COVERAGE_SRGB threshold in sample-space
        //

        // Future Consideration:  - Consider allowing the cached intermediate
        // bounds to be a super-set of the needed bounds when not tiling.
        //
        // We could alter the source clip to allow the cached intermediate
        // bounds to be a superset of the needed bounds.  This isn't being done
        // now because it isn't a common case, and would further complicate
        // this logic.

        if (AreTransformedRectanglesClose(
            &rcCachedIntermediateBounds_SampleScaledViewportSpace,
            &rcCurrentIntermediateBounds_SampleScaledViewportSpace,
            NULL,
            INSIGNIFICANT_PIXEL_COVERAGE_SRGB
            ))
        {
            fCanReuseIntermediate = true;
        }
    }

    return fCanReuseIntermediate;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushIntermediateCache::IsDimensionWithinCachingThreshold
//
//  Synopsis:
//      Determines whether or not a dimension (e.g., width/height) is within a
//      factor of the originalValue.
//
//      shrinkThreshold is only valid in the following range:
//          0.0 <= shrinkThreshold <= 1.0.
//      If set to 0.0, true is always returned when the dimension
//          shrinks.
//
//      expandThreshold is only valid in the following range:
//          1.0 <= expandThreshold.  
//      If set to +INF, true is always returned when the dimension
//          expands.
//
//      true will always be returned if orginalValue == newValue
//
//------------------------------------------------------------------------------
bool 
CBrushIntermediateCache::IsDimensionWithinCachingThreshold(
    float originalValue, 
        // Original dimension to compare against
    float newValue, 
        // New dimension to compare to originalValue
    float shrinkThreshold,
        // Threshold to apply when shrinking (i.e., when newValue < originalValue)
    float expandThreshold
        // Threshold to apply when expanding (i.e., when newValue > originalValue)
    )
{
    // originalValue & newValue must be positive, 0.0, or NaN.
    Assert(!(originalValue < 0.0f));
    Assert(!(newValue < 0.0f));    

    Assert(shrinkThreshold >= 0.0f);
    Assert(shrinkThreshold <= 1.0f);   

    Assert(expandThreshold >= 1.0f);
        
    bool fWithinThreshold = false;    
    float differenceFactor;

    //
    // Calculate the difference factor
    //

    if (0.0f == originalValue)
    {
        // Support infinite expand thresholds.  This is neccessary to support a expandThreshold of +INF.
        //
        // Dividing by 0.0f would produce a NaN.  Unlike other causes of NaN, this value has an
        // actual meaning -- that we are expanding by a factor of infinity.  Dividing by FLT_MIN, 
        // the next representable value after 0.0, results in +INF, so we don't need to check
        // for values other than 0.0.
        differenceFactor = FLT_MIN;
    }
    
    differenceFactor = newValue / originalValue;

    //
    // Determine whether or not the factor is within the threshold
    //

    if (differenceFactor < 1.0f)
    {   
        fWithinThreshold = (differenceFactor >= shrinkThreshold);
    }
    else if (differenceFactor >= 1.0f)
    {
        fWithinThreshold = (differenceFactor <= expandThreshold);
    }
    else
    {
        Assert(_isnan(differenceFactor));
        // If the difference factor is a NaN because of some singularily, we should re-populate the cache.
        fWithinThreshold = false;       
    }
    
    return fWithinThreshold;
}



