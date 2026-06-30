// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwBitmapColorSource implementation
//

#include "precomp.hpp"


//   Why is g_dwTextureUpdatesPerFrame volatile
// when we expect to have a single threaded access to device and realizations?
volatile DWORD g_dwTextureUpdatesPerFrame = 0;


#if DBG
//+----------------------------------------------------------------------------
//
//  Member:    DbgTintDirtyRectangle
//
//  Synopsis:  Tint bitmap dirty rectangles in debug to show update regions
//
//-----------------------------------------------------------------------------

void
DbgTintDirtyRectangle(
    __inout_xcount(
        iPitch * (prcDirty->bottom - 1) +
        sizeof(GpCC) * (prcDirty->right)
        ) void *pvDbgTintBits,
    INT iPitch,
    D3DFORMAT d3dFmt,
    const CMilRectU *prcDirty
    );

#endif DBG


DeclareTag(tagShowBitmapDirtyRectangles, "MIL-HW", "Show bitmap dirty rectangles");


MtDefine(CHwBitmapColorSource, MILRender, "CHwBitmapColorSource");


//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::CacheContextParameters::CacheContextParameters
//
//  Synopsis:
//      Constructor that intentionally does not initialize any members.
//
//  Notes:
//      There is one parameter to force all users to intentionally select a
//      constructor.  True must always be passed here.
//
//-----------------------------------------------------------------------------

CHwBitmapColorSource::CacheContextParameters::CacheContextParameters(
    bool fInitializeNoMembers
    )
{
    Verify(fInitializeNoMembers);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::CacheContextParameters::CacheContextParameters
//
//  Synopsis:
//      Initializes the realization parameters based on context state and brush
//

CHwBitmapColorSource::CacheContextParameters::CacheContextParameters(
    __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) const CContextState *pContextState,
    __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
    MilPixelFormat::Enum fmtTargetSurface
    )
{
    pBitmapBrushNoRef = pBitmapBrush;
    fPrefilterEnable = pContextState->RenderState->PrefilterEnable;
    interpolationMode = pContextState->RenderState->InterpolationMode;
    fmtRenderTarget = fmtTargetSurface;
    nBitmapBrushUniqueness = pBitmapBrush->GetUniqueCount();
    wrapMode = pBitmapBrush->GetWrapMode();

    //
    // Check for media control of prefiltering
    //

    if (fPrefilterEnable)
    {
        if (g_pMediaControl && g_pMediaControl->GetDataPtr()->FantScalerDisabled)
        {
            fPrefilterEnable = false;
        }
    }

    if (DoesUseMipMapping(interpolationMode))
    {
        //
        // We don't want to mipmap if we're in a Sw device or we don't have
        // enough Hw support.
        // The check is needed here to make sure
        //  cache look up is most correct.
        //
        if (   pDevice->IsSWDevice()
            || !(   pDevice->CanAutoGenMipMap()
                 || pDevice->CanStretchRectGenMipMap())
           )
        {
            interpolationMode = MilBitmapInterpolationMode::Linear;
        }
    }


    //
    // Future Consideration:  Could calculate the destination rect for prefiltering, but the source
    // rect code adds complication here that I don't want to deal with now.
    //

    //
    // NOTICE-2005/10/12-chrisra ContextParameters Don't track subregion
    //
    // When a bitmap is realized, we may only realize a subregion into a
    // texture because of texture size limits on the hardware.  But we
    // don't know this until the full realization code has been run.
    //
    // Until the ContextParameters can properly track this information we
    // simply avoid setting a "Last Used" color source in the cache.
    //
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::CacheContextParameters::CacheContextParameters
//
//  Synopsis:
//      Initializes the realization parameters based on explicit settings.  For
//      use without context and/or brush.
//
//-----------------------------------------------------------------------------

CHwBitmapColorSource::CacheContextParameters::CacheContextParameters(
    MilBitmapInterpolationMode::Enum _interpolationMode,
    bool _fPrefilterEnable,
    MilPixelFormat::Enum _fmtRenderTarget,
    MilBitmapWrapMode::Enum _wrapMode
    ) :
    pBitmapBrushNoRef(NULL),
    interpolationMode(_interpolationMode),
    fPrefilterEnable(_fPrefilterEnable),
    fmtRenderTarget(_fmtRenderTarget),
    nBitmapBrushUniqueness(0),
    wrapMode(_wrapMode)
{
}



//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::DeriveFromBrushAndContext
//
//  Synopsis:  Gets a CHwTexturedColorSource from the bitmap brush
//
//-----------------------------------------------------------------------------
HRESULT CHwBitmapColorSource::DeriveFromBrushAndContext(
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
    __in_ecount(1) const CHwBrushContext &hwBrushContext,
    __deref_out_ecount(1) CHwTexturedColorSource **ppHwTexturedColorSource
    )
{
    HRESULT hr = S_OK;

    CHwBitmapColorSource *pHwBitmapColorSource = NULL;
    CHwBitmapColorSource *pReusableRealizationSourcesList = NULL;

    IWGXBitmap *pBitmapNoRef = NULL;
    CHwBitmapCache *pHwBitmapCache = NULL;

    BitmapToXSpaceTransform bitmapToXSpaceTransform;

    Assert(hwBrushContext.GetContextStatePtr()->RenderState);

    //
    // Compute Bitmap to sample space transform
    //

    CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> matBitmapToIdealRealization;

    CDelayComputedBounds<CoordinateSpace::RealizationSampling> rcRealizationBounds;

    hwBrushContext.GetRealizationBoundsAndTransforms(
        IN pBitmapBrush,
        OUT matBitmapToIdealRealization,
        OUT bitmapToXSpaceTransform,
        OUT rcRealizationBounds
        );

    CHwBitmapColorSource::CacheContextParameters oContextCacheParameters(
        pDevice,
        hwBrushContext.GetContextStatePtr(),
        pBitmapBrush,
        hwBrushContext.GetFormat()
        );

    IWGXBitmapSource *pIBitmapNoRef = pBitmapBrush->GetTextureNoAddRef();

    if (!pIBitmapNoRef)
    {
        IFC(E_INVALIDARG);
    }

    //
    // Look for an existing cached resource and extract IWGXBitmap if possible.
    //

    MIL_THR(CHwBitmapCache::RetrieveFromBitmapSource(
        pIBitmapNoRef,
        pDevice,
        OUT &pBitmapNoRef,
        OUT &pHwBitmapCache
        ));

    if (SUCCEEDED(hr))
    {
        if (pBitmapNoRef && pBitmapNoRef->SourceState() == IWGXBitmap::SourceState::DeviceBitmap)
        {
            // Disable prefiltering for device bitmaps. Since we already have a realization with
            // the device bitmap, prefiltering is usually ignored. The one time we copy the device
            // bitmap to software and realize again is cross adapter and then prefiltering kills
            // performance.
            oContextCacheParameters.fPrefilterEnable = false;
            
            //
            // First time encoutering a device bitmap on this device. Let's try to create our 
            // secondary device bitmap if possible. The new color source will be retrieved by 
            // TryForDeviceBitmapOrLastUsedBitmapColorSource below if it were actually created.
            //
            if (!pHwBitmapCache)
            {
                MIL_THR(CHwBitmapCache::GetCache(
                    pDevice,
                    pBitmapNoRef,
                    /* pICacheAlternate = */ NULL,
                    /* fSetResourceRequired = */ true,
                    &pHwBitmapCache
                    ));

                if (SUCCEEDED(hr))
                {
                    CDeviceBitmap *pDeviceBitmap = 
                        DYNCAST(CDeviceBitmap, pBitmapNoRef);
                    Assert(pDeviceBitmap);

                    pDeviceBitmap->TryCreateDependentDeviceColorSource(
                        pDevice->GetD3DAdapterLUID(),
                        pHwBitmapCache
                        );
                }
            }
            
            //
            // Prevent mip-map realization of CDeviceBitmap
            //
            // Checking the SourceState is a bit of a kludge. There may be other
            // source states which also contain useful cached device bitmaps in the
            // future. For now though, the only time we can receive a mip-mapped
            // interpolation mode and a device bitmap is with an intermediate
            // render target in 3D. These render targets use
            // CBitmapsOfDeviceBitmaps to store their bitmap source.
            //
            // NOTICE-2006/12/20-MilesC Video avoids using CBitmapofDeviceBitmaps,
            // yet its only entry point is through DrawVideo (which does not use
            // mip-mapping). There will be an issue with this code if we ever
            // support video brush in 3D.
            //

            if (DoesUseMipMapping(oContextCacheParameters.interpolationMode))
            {
                //
                // Replace the interpolation mode used to lookup entries in the
                // cache. There are some types of bitmaps which are very expensive
                // to generate mip-maps. By altering our cache lookup logic to stop
                // looking for mip-mapped interpolation modes, this saves us that
                // expensive re-realization step which follows a cache miss.
                //
                oContextCacheParameters.interpolationMode = MilBitmapInterpolationMode::Linear;
            }
        }

        //
        // Is there a cached bitmap cache?
        //

        if (pHwBitmapCache)
        {
            //
            // Try to quickly reuse a shared or the last bitmap color source.
            //

            pHwBitmapCache->TryForDeviceBitmapOrLastUsedBitmapColorSource(
                oContextCacheParameters,
                rcRealizationBounds,
                pBitmapBrush,
                pHwBitmapColorSource,
                pReusableRealizationSourcesList
                );
        }
    }
    
    if (pHwBitmapColorSource == NULL)
    {
        //
        // We weren't able to reuse the last bitmap source, go through standard
        // realization process.
        //

        IFC(DeriveFromBitmapAndContext(
            pDevice,
            pIBitmapNoRef,
            pBitmapNoRef,
            pHwBitmapCache,
            rcRealizationBounds,
            &matBitmapToIdealRealization,
            &bitmapToXSpaceTransform,
            hwBrushContext.GetContextStatePtr()->RenderState->PrefilterThreshold,
            hwBrushContext.CanFallback(),
            pBitmapBrush,
            oContextCacheParameters,
            ppHwTexturedColorSource
            ));
    }
    else
    {
        //
        // We're able to reuse a hw bitmap source, all we have to do is update
        // context specific settings: 1) interpolation mode, 2) reusable sources 
        // and 3) the bitmap to device transform.
        //
        
        pHwBitmapColorSource->SetFilterMode(
            oContextCacheParameters.interpolationMode
            );

        pHwBitmapColorSource->CheckAndSetReusableSources(
            pReusableRealizationSourcesList
            );

        IFC(pHwBitmapColorSource->CalcTextureTransform(&bitmapToXSpaceTransform));

        *ppHwTexturedColorSource = pHwBitmapColorSource;
        pHwBitmapColorSource = NULL;
    }

    const CParallelogram *pWorldSpaceMaskParallelogramNoRef = NULL;
    //
    // When bitmap is to be source clipped and the current mode is 3D then we
    // have no mechanism to trim our geometry; so, use mask texture instead.
    //
    // Future Consideration:   Consider context state to indicate src clip
    //  so that text and 3D can have a common path.  This is assuming we
    //  don't get a better solution of actually trimming geometry.
    //
    if (   pBitmapBrush->HasSourceClip()
        && hwBrushContext.GetContextStatePtr()->In3D)
    {
        // If the bounds of the mesh in texture space are contained
        // within the source clip then we don't need the source clip.
        // This same optimization for 2D is in ShapeClipperForFEB.cpp.
        //
        // Because of the TileBrush defaults this is a very common case
        // and this optimization can also avoid artifacts coming from
        // having texture coordinates precisely on the boundary of the
        // source clip edge.
        
        CParallelogram paraMaskSampleSpace, paraTextureBoundsSampleSpace;
        CRectF<CoordinateSpace::RealizationSampling> rcBitmapBounds;
        
        rcRealizationBounds.GetBounds(OUT rcBitmapBounds);
        
        paraTextureBoundsSampleSpace.Set(rcBitmapBounds);
        paraTextureBoundsSampleSpace.Transform(&matBitmapToIdealRealization);

        paraMaskSampleSpace.Set(*(pBitmapBrush->GetSourceClipWorldSpace()),
                                &hwBrushContext.GetWorld2DToIdealSamplingSpace());

        if (!paraMaskSampleSpace.Contains(
                paraTextureBoundsSampleSpace,
                INSIGNIFICANT_PIXEL_COVERAGE_SRGB))
        {
            pWorldSpaceMaskParallelogramNoRef = pBitmapBrush->GetSourceClipWorldSpace();
        }
    }
    IFC((**ppHwTexturedColorSource).SetMaskClipWorldSpace(pWorldSpaceMaskParallelogramNoRef));

Cleanup:
    ReleaseInterfaceNoNULL(pHwBitmapCache);
    ReleaseInterfaceNoNULL(pReusableRealizationSourcesList);
    ReleaseInterfaceNoNULL(pHwBitmapColorSource);

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::DeriveFromBitmapAndContext
//
//  Synopsis:  Gets a CHwTexturedColorSource from the bitmap brush data
//             The color source is realized if it cannot be found in a cache
//

HRESULT
CHwBitmapColorSource::DeriveFromBitmapAndContext(
    __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    __inout_ecount(1) IWGXBitmapSource *pIBitmap,
    __inout_ecount_opt(1) IWGXBitmap *pBitmapNoRef,
    __inout_ecount_opt(1) CHwBitmapCache *pHwBitmapCache,
    __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcRealizationBounds,
    __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> *pmatBitmapToIdealRealization,
    __in_ecount(1) const BitmapToXSpaceTransform *pBitmapToXSpaceTransform,
    REAL rPrefilterThreshold,
    BOOL fCanFallback,
    __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate,
    __in_ecount(1) CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,
    __deref_out_ecount(1) CHwTexturedColorSource **ppHwTexturedColorSource
    )
{
    HRESULT hr = S_OK;

    *ppHwTexturedColorSource = NULL;

    CHwBitmapColorSource *pHwBitmapColorSource = NULL;

    // pbcsWithReusableRealizationSource if set points to a HwBitmapColorSource
    // that has been previously cached and may provide source that is already
    // loaded into video memory.
    CHwBitmapColorSource *pbcsWithReusableRealizationSource = NULL;

    //
    // Look up cached resource if one isn't already specified.
    //
    if (pHwBitmapCache)
    {
        // AddRef the one we have because we'll Release in cleanup.
        pHwBitmapCache->AddRef();

        // pBitmapNoRef must be accurately set when pHwBitmapCache is not NULL.
        
        #if DBG
        {
            IWGXBitmap *pBitmapAnalysis = NULL;
            if (FAILED(pIBitmap->QueryInterface(IID_IWGXBitmap, reinterpret_cast<void**>(&pBitmapAnalysis))))
            {
                pBitmapAnalysis = NULL;
            }
            Assert(pBitmapNoRef == pBitmapAnalysis);
            ReleaseInterface(pBitmapAnalysis);
        }
        #endif
    }
    else
    {
        hr = THR(CHwBitmapCache::RetrieveFromBitmapSource(
            pIBitmap,
            pD3DDevice,
            OUT &pBitmapNoRef,
            OUT &pHwBitmapCache
            ));
    }

    //
    // Get realization parameters
    //

    CHwBitmapColorSource::RealizationParameters oRealizationParams;

    //
    // Set main realization parameters
    //

    IFC(CHwBitmapColorSource::ComputeRealizationParameters(
        pD3DDevice,
        pIBitmap,
        rcRealizationBounds,
        pmatBitmapToIdealRealization,
        oContextCacheParameters.fmtRenderTarget,
        oContextCacheParameters.wrapMode,
        oContextCacheParameters.interpolationMode,
        oContextCacheParameters.fPrefilterEnable,
        rPrefilterThreshold,
        fCanFallback,
        oRealizationParams
        ));

    //
    // Get a color source
    //

    IFC(CHwBitmapCache::GetBitmapColorSource(
         pD3DDevice,
         pIBitmap,
         pBitmapNoRef,
         IN OUT oRealizationParams,
         oContextCacheParameters,
         pHwBitmapCache,
         OUT pHwBitmapColorSource,
         OUT pbcsWithReusableRealizationSource,
         pICacheAlternate
         ));

    //
    // Set context and bitmap.  They may be the first to be set, the same as
    // currently set, or different than what was set previously.
    //

    IFC(pHwBitmapColorSource->SetBitmapAndContext(
        pIBitmap,
        rcRealizationBounds,
        pBitmapToXSpaceTransform,
        oRealizationParams,
        pbcsWithReusableRealizationSource
        ));

    //
    // Update our color source
    //

    *ppHwTexturedColorSource = pHwBitmapColorSource; // steal ref
    pHwBitmapColorSource = NULL;

    Assert(*ppHwTexturedColorSource);

Cleanup:
    ReleaseInterfaceNoNULL(pHwBitmapCache);
    ReleaseInterfaceNoNULL(pbcsWithReusableRealizationSource);
    ReleaseInterfaceNoNULL(pHwBitmapColorSource);

    RRETURN(hr);
}



//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::ComputeMinimumRealizationBounds
//
//  Synopsis:
//      Compute minimum realization bounds in RealizationSampling coordinate
//      space from the given context
//
//-----------------------------------------------------------------------------

bool
CHwBitmapColorSource::ComputeMinimumRealizationBounds(
    __in_ecount(1) IWGXBitmapSource *pIBitmap,
    __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcRealizationBounds,
    __in_ecount(1) CacheContextParameters const &oCacheContextParameters,
    __out_ecount(1) CMilRectU &rcMinBounds
    )
{
    bool fSuccess = false;
    HRESULT hr;

    InternalRealizationParameters oRealizationParams;

    IFC(pIBitmap->GetSize(
        &oRealizationParams.uBitmapWidth,
        &oRealizationParams.uBitmapHeight
        ));

    oRealizationParams.interpolationMode = oCacheContextParameters.interpolationMode;
    oRealizationParams.wrapMode = oCacheContextParameters.wrapMode;

    rcMinBounds.left = 0;
    rcMinBounds.top = 0;
    rcMinBounds.right = oRealizationParams.uBitmapWidth;
    rcMinBounds.bottom = oRealizationParams.uBitmapHeight;

    fSuccess = ComputeMinimumRealizationBounds(
        IN OUT rcRealizationBounds,
        IN     oRealizationParams,
        IN OUT rcMinBounds
        );

Cleanup:
    return fSuccess;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::ComputeMinimumRealizationBounds
//
//  Synopsis:
//      Compute minimum realization bounds for RealizationParameters structure
//      from the given context
//
//      Pass in/out bounds rect - rcMinBounds.  "In" it contains full
//      prefiltered rectangle of source and its width and height are used to
//      transform from original bitmap coordinate space to prefiltered bitmap
//      space.  "Out" it contains the minimum required bounds.
//

bool
CHwBitmapColorSource::ComputeMinimumRealizationBounds(
    __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcRealizationBounds,
    __in_ecount(1) InternalRealizationParameters const &oRealizationParams,
    __inout_ecount(1) CMilRectU &rcMinBounds
    )
{
    Assert(!DoesUseMipMapping(oRealizationParams.interpolationMode));

    Assert(rcMinBounds.left == 0);
    Assert(rcMinBounds.top == 0);
    Assert(!rcMinBounds.IsEmpty());

    CRectF<CoordinateSpace::RealizationSampling> rcBitmapBounds;

    bool fSuccess = rcRealizationBounds.GetBounds(
        OUT rcBitmapBounds
        );

    if (fSuccess)
    {
        UINT const uWidth = rcMinBounds.right;
        UINT const uHeight = rcMinBounds.bottom;

        if (uWidth != oRealizationParams.uBitmapWidth)
        {
            float rWidthPrefilterScale =
                static_cast<float>(uWidth) /
                static_cast<float>(oRealizationParams.uBitmapWidth);

            rcBitmapBounds.left *= rWidthPrefilterScale;
            rcBitmapBounds.right *= rWidthPrefilterScale;
        }

        if (uHeight != oRealizationParams.uBitmapHeight)
        {
            float rHeightPrefilterScale =
                static_cast<float>(uHeight) /
                static_cast<float>(oRealizationParams.uBitmapHeight);

            rcBitmapBounds.top *= rHeightPrefilterScale;
            rcBitmapBounds.bottom *= rHeightPrefilterScale;
        }

        //
        // Sample bounds are given in floating point and are inclusive-
        // inclusive.  Realization (texel) bounds are integer-based and are
        // inclusive-exclusive.  Both use half-pixel center convention.  The
        // net of this is that sample point to texel conversion must round in
        // some fashion to get integers.  Lower and upper texel bounds for
        // sample point n could most directly be caluculated by floor(n) and
        // ceiling(n), respectively.  However that produces no bound texel when
        // n is an integer (half-way between texels.)  Since actual sampling is
        // based on floating point data with limited precision, cases that
        // could break either way with a little such imprecision should be
        // protected by extending texel bounds to include possible
        // contributors.  This done done by calculating lower and upper bounds
        // as ceiling(n)-1 and floor(n)+1, respectively.
        //
        // When interpolation modes other than nearest are used multiple actual
        // sample points may be taken and the sampling bounds may be increased.
        // Each interpolation mode has its own inflation factor.
        //
        // For nearest sampling there is no extra inflation.  Inflation factor
        // is 0 texels. 
        //
        // For linear sampling another texel may contribute to if texel bound
        // is less than 0.5 texel away.  Inflation factor is 0.5 texels. 
        // Safety from floating point imprecision using wrong texels is not as
        // important with linear interpolation because the contribution of
        // those samples should be small.  Still there isn't a significant
        // known gain to optimize for that case; so, leave conversion code
        // general.
        //
        // For cubic the distance of contribution depends on the source scale,
        // but this mode is not currently supported or used; so, ignore that
        // case.
        Assert(oRealizationParams.interpolationMode != MilBitmapInterpolationMode::Cubic);
        //
        // The Fant interpolation mode is also ignored.  If Fant filtering were
        // employed it would already be accounted for with prefiltering and
        // linear would just be used.  See inflation factor for linear above.
        //
        // Combining interpolation inflation factor and float sample point n to
        // integer texel bounds produces the following formulas:
        //
        //   Lower bound = ceiling( n - interpolation_inflation_factor ) - 1
        //   Upper bound = floor( n + interpolation_inflation_factor ) + 1
        //
        // Discounting floating point precision loss at extreme values, which
        // will already exceed base texture bounds, reduces those formulas to:
        //
        //   Lower bound = ceiling( n - (interpolation_inflation_factor + 1) )
        //   Upper bound = floor( n + (interpolation_inflation_factor + 1) )
        //
        // Note: an additional benefit of including +/-1 before ceiling/floor
        //       is that overflow does become an issue.  Again only extreme
        //       values are impacted and falling to infinity is not an issue
        //       because saturating versions of floor and ceiling already need
        //       to be used.
        //
        // Summary table of interesting cases
        //
        //   Sample n     |     Nearest      |        Linear         |
        //                | lower    upper   | lower      upper      |
        //   N=Integer(n) | C(n-1)   F(n+1)  | C(n-1.5)   F(n+1.5)   |
        // ---------------+------------------+-----------------------+
        //   N.0  (ideal) |  N    to  N+1    |  N-1    to  N+1       |
        //     => (safe)  |  N-1  to  N+1    |    (same)             |
        //   N.0+epsilon  |  N    to  N+1    |  N-1    to  N+1       |
        //   N. ...       |  N    to  N+1    |  N-1    to  N+1       |
        //   N.5-epsilon  |  N    to  N+1    |  N-1    to  N+1       |
        //   N.5  (ideal) |  N    to  N+1    |  N      to  N+1       |
        //     => (safe)  |   (same)         |  N-1    to  N+2       |
        //   N.5+epsilon  |  N    to  N+1    |  N      to  N+2       |
        //   N. ...       |  N    to  N+1    |  N      to  N+2       |
        //   N+1-epsilon  |  N    to  N+1    |  N      to  N+2       |
        //

        FLOAT const rRoundingFactor =
            (oRealizationParams.interpolationMode ==
                MilBitmapInterpolationMode::NearestNeighbor) ? 1.0f : 1.5f;

        //
        // Compute horizontal expanse required
        //
        // Sampling points within natural (base) texel span are 0 to Width-1
        // inclusive.  Sample points beyond depend on wrap mode.
        //

        {
            Assert(rcMinBounds.left == 0);
            Assert(rcMinBounds.right == uWidth);

            INT LeftSampleBound =
                CFloatFPU::CeilingSat(rcBitmapBounds.left - rRoundingFactor);

            INT RightSampleBound =
                CFloatFPU::FloorSat(rcBitmapBounds.right + rRoundingFactor);

            if (LeftSampleBound < RightSampleBound)
            {
                if (oRealizationParams.wrapMode == MilBitmapWrapMode::Extend)
                {
                    if (LeftSampleBound > 0)
                    {
                        // Width-1 is rightmost edge; always include at least
                        // the rightmost edge.
                        if (LeftSampleBound < static_cast<INT>(uWidth))
                        {
                            rcMinBounds.left = static_cast<UINT>(LeftSampleBound);
                        }
                        else
                        {
                            rcMinBounds.left = uWidth - 1;
                        }
                    }
                    else
                    {
                        Assert(rcMinBounds.left == 0);
                    }
        
                    if (RightSampleBound < static_cast<INT>(uWidth))
                    {
                        // 1 is the lower limit for right to include at least
                        // leftmost edge; always inlude at least the leftmost
                        // edge.
                        if (RightSampleBound > 0)
                        {
                            rcMinBounds.right = static_cast<UINT>(RightSampleBound);
                        }
                        else
                        {
                            rcMinBounds.right = 1;
                        }
                    }
                    else
                    {
                        Assert(rcMinBounds.right == uWidth);
                    }
                }
                else
                {
                    // Check if sample bounds are all within base texel span.
                    if (   LeftSampleBound >= 0
                        && RightSampleBound <= static_cast<INT>(uWidth)
                       )
                    {
                        rcMinBounds.left = static_cast<UINT>(LeftSampleBound);
                        rcMinBounds.right = static_cast<UINT>(RightSampleBound);
                    }
                    else
                    {
                        // the entire span is needed
                        Assert(rcMinBounds.left == 0);
                        Assert(rcMinBounds.right == uWidth);
                    }
                }
            }
            else
            {
                // NaNs or empty bounds are in the mix.  Play it safe and use
                // the whole span.
                Assert(rcMinBounds.left == 0);
                Assert(rcMinBounds.right == uWidth);
            }

            Assert(rcMinBounds.left < rcMinBounds.right);
            Assert(rcMinBounds.left < uWidth);
            Assert(rcMinBounds.right > 0);
        }


        //
        // Compute vertical expanse required
        //
        // Sampling points within natural (base) texel span are 0 to Height-1
        // inclusive.  Sample points beyond depend on wrap mode.
        //

        {
            Assert(rcMinBounds.top == 0);
            Assert(rcMinBounds.bottom == uHeight);

            INT TopSampleBound =
                CFloatFPU::CeilingSat(rcBitmapBounds.top - rRoundingFactor);

            INT BottomSampleBound =
                CFloatFPU::FloorSat(rcBitmapBounds.bottom + rRoundingFactor);

            if (TopSampleBound < BottomSampleBound)
            {
                if (oRealizationParams.wrapMode == MilBitmapWrapMode::Extend)
                {
                    if (TopSampleBound > 0)
                    {
                        // Height-1 is bottommost edge; always include at least
                        // the bottommost edge.
                        if (TopSampleBound < static_cast<INT>(uHeight))
                        {
                            rcMinBounds.top = static_cast<UINT>(TopSampleBound);
                        }
                        else
                        {
                            rcMinBounds.top = uHeight - 1;
                        }
                    }
                    else
                    {
                        Assert(rcMinBounds.top == 0);
                    }
        
                    if (BottomSampleBound < static_cast<INT>(uHeight))
                    {
                        // 1 is the lower limit for bottom to include at least
                        // topmost edge; always inlude at least the topmost
                        // edge.
                        if (BottomSampleBound > 0)
                        {
                            rcMinBounds.bottom = static_cast<UINT>(BottomSampleBound);
                        }
                        else
                        {
                            rcMinBounds.bottom = 1;
                        }
                    }
                    else
                    {
                        Assert(rcMinBounds.bottom == uHeight);
                    }
                }
                else
                {
                    // Check if sample points are all within base texel span.
                    if (   TopSampleBound >= 0
                        && BottomSampleBound <= static_cast<INT>(uHeight)
                       )
                    {
                        rcMinBounds.top = static_cast<UINT>(TopSampleBound);
                        rcMinBounds.bottom = static_cast<UINT>(BottomSampleBound);
                    }
                    else
                    {
                        // the entire span is needed
                        Assert(rcMinBounds.top == 0);
                        Assert(rcMinBounds.bottom == uHeight);
                    }
                }
            }
            else
            {
                // NaNs or empty bounds are in the mix.  Play it safe and use
                // the whole span.
                Assert(rcMinBounds.top == 0);
                Assert(rcMinBounds.bottom == uHeight);
            }

            Assert(rcMinBounds.top < rcMinBounds.bottom);
            Assert(rcMinBounds.top < uHeight);
            Assert(rcMinBounds.bottom > 0);
        }

        Assert(!rcMinBounds.IsEmpty());
    }

    return fSuccess;
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::InitializeSubRectParameters
//
//  Synopsis:  Initialize the subrect to contain the entire source.
//
//-----------------------------------------------------------------------------
void
CHwBitmapColorSource::InitializeSubRectParameters(
    __inout_ecount(1) RealizationParameters &oRealizationParams
    )
{
    oRealizationParams.fOnlyContainsSubRectOfSource = false;
    oRealizationParams.rcSourceContained.left = 0;
    oRealizationParams.rcSourceContained.top  = 0;
    oRealizationParams.rcSourceContained.right  = oRealizationParams.uWidth;
    oRealizationParams.rcSourceContained.bottom = oRealizationParams.uHeight;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::ComputeRealizationSize
//
//  Synopsis:  Compose size portion of RealizationParameters structure from the
//             given context
//

HRESULT
CHwBitmapColorSource::ComputeRealizationSize(
    UINT uMaxTextureWidth,
    UINT uMaxTextureHeight,
    __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcRealizationBounds,
    __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> *pmatBitmapToIdealRealization,
    MilBitmapWrapMode::Enum wrapMode,
    BOOL fPrefilterEnabled,
    REAL rPrefilterThreshold,
    BOOL fCanFallback,
    __inout_ecount(1) RealizationParameters &oRealizationParams
    )
{
    HRESULT hr = S_OK;

    Assert(oRealizationParams.uBitmapWidth > 0);
    Assert(oRealizationParams.uBitmapHeight > 0);

    //
    // We don't want to prefilter to one size if we're using mipmapping.
    //
    if (DoesUseMipMapping(oRealizationParams.interpolationMode))
    {
        //
        // Currently the hardware only seems to be able to automatically
        // generate mipmaps if the texture size is a power of 2, and we need to
        // clamp the 3d textures to the maximum size available on the card.
        //

        if (oRealizationParams.uBitmapWidth >= uMaxTextureWidth)
        {
            Assert(IS_POWER_OF_2(uMaxTextureWidth));

            oRealizationParams.uWidth  = uMaxTextureWidth;
        }
        else if (fPrefilterEnabled || wrapMode != MilBitmapWrapMode::Extend)
        {
            // Scale up to a power of two (or stay at current power of two) to
            // completely fill the top mip-map level.  This will result in some
            // amount of blurring at 1:1 scale factor (original source :
            // destination).
            oRealizationParams.uWidth =  RoundToPow2(oRealizationParams.uBitmapWidth);
        }
        else
        {
            // Stay at natural resolution, which can actually improve quality
            // in addition to the boost in performance from avoiding filtering.
            //
            // Note: However no attempt is made later on to fill margins of
            //       texture not filled by natural source image.  This can lead
            //       to random colors bleeding into destination fill.
            oRealizationParams.uWidth  = oRealizationParams.uBitmapWidth;
        }

        if (oRealizationParams.uBitmapHeight >= uMaxTextureHeight)
        {
            Assert(IS_POWER_OF_2(uMaxTextureHeight));

            oRealizationParams.uHeight = uMaxTextureHeight;
        }
        else if (fPrefilterEnabled || wrapMode != MilBitmapWrapMode::Extend)
        {
            // Scale up to a power of two (or stay at current power of two) to
            // completely fill the top mip-map level.  This will result in some
            // amount of blurring at 1:1 scale factor (original source :
            // destination).
            oRealizationParams.uHeight = RoundToPow2(oRealizationParams.uBitmapHeight);
        }
        else
        {
            // Stay at natural resolution, which can actually improve quality
            // in addition to the boost in performance from avoiding filtering.
            //
            // Note: However no attempt is made later on to fill margins of
            //       texture not filled by natural source image.  This can lead
            //       to random colors bleeding into destination fill.
            oRealizationParams.uHeight = oRealizationParams.uBitmapHeight;
        }

        // Minimum realization bound calculations do not support mip maps.  So
        // effectively minimum bounds calculation is already complete.  Just
        // mark as such.
        oRealizationParams.fMinimumRealizationRectRequiredComputed = true;
    }
    else
    {
        if (fPrefilterEnabled)
        {
            pmatBitmapToIdealRealization->ComputePrefilteringDimensions(
                oRealizationParams.uBitmapWidth,
                oRealizationParams.uBitmapHeight,
                rPrefilterThreshold,
                OUT oRealizationParams.uWidth,
                OUT oRealizationParams.uHeight
                );
        }
        else
        {
            oRealizationParams.uWidth  = oRealizationParams.uBitmapWidth;
            oRealizationParams.uHeight = oRealizationParams.uBitmapHeight;
        }
    }

    Assert(oRealizationParams.uWidth > 0);
    Assert(oRealizationParams.uHeight > 0);

    InitializeSubRectParameters(oRealizationParams);

    //  Falling back for MAXSIZE-1 sized textures.
    // See Windows Client Task List # 42111
    // Under the following circumstances we'll fall back instead of
    // using the alternative minimum size code:
    //   1. we have a desired realization that is MAX-1
    //   2. the screen space bounds lie entirely within the base tile and
    //   3. the tile mode is not extend
    // We won't call ComputeAlternateMinimumRealizationSize because
    // the size isn't greater than the max texture size but later the
    // border code will try to increase the width by 2. It's not worth
    // fixing this corner case with additional complexity.

    if (   (oRealizationParams.uWidth  > uMaxTextureWidth)
        || (oRealizationParams.uHeight > uMaxTextureHeight))
    {
        Assert(   oRealizationParams.interpolationMode == MilBitmapInterpolationMode::NearestNeighbor
               || oRealizationParams.interpolationMode == MilBitmapInterpolationMode::Linear
               || oRealizationParams.interpolationMode == MilBitmapInterpolationMode::Cubic
               );

        // Independent of call result, minimum will be computed post call.
        oRealizationParams.fMinimumRealizationRectRequiredComputed = true;

        bool fFoundAlternate = ComputeMinimumRealizationBounds(
            rcRealizationBounds,
            oRealizationParams,
            IN OUT oRealizationParams.rcSourceContained
            );

        if (   !fFoundAlternate
            || (oRealizationParams.rcSourceContained.Width<UINT>() > uMaxTextureWidth )
            || (oRealizationParams.rcSourceContained.Height<UINT>() > uMaxTextureHeight))
        {
            if (fCanFallback && fPrefilterEnabled)
            {
                // In HighQuality mode this E_NOTIMPL will trigger fallback
                // to software rendering to complete the operation at high
                // quality.
                IFC(E_NOTIMPL);
            }
            else
            {
                // If we can't fallback to software or are in LowQuality mode,
                // just use a prefilter to get to a size within texture limits
                // even though sample resolution will not be ideal.

                if (oRealizationParams.rcSourceContained.Width<UINT>() > uMaxTextureWidth )
                {
                    oRealizationParams.uWidth  = uMaxTextureWidth;
                    oRealizationParams.rcSourceContained.left = 0;
                    oRealizationParams.rcSourceContained.right = oRealizationParams.uWidth;
                }
                else if (   (oRealizationParams.rcSourceContained.left > 0)
                         || (static_cast<UINT>(oRealizationParams.rcSourceContained.right) < oRealizationParams.uWidth))
                {
                    oRealizationParams.fOnlyContainsSubRectOfSource = true;
                }

                if (oRealizationParams.rcSourceContained.Height<UINT>() > uMaxTextureHeight)
                {
                    oRealizationParams.uHeight = uMaxTextureHeight;
                    oRealizationParams.rcSourceContained.top = 0;
                    oRealizationParams.rcSourceContained.bottom = oRealizationParams.uHeight;
                }
                else if (   (oRealizationParams.rcSourceContained.top > 0)
                         || (static_cast<UINT>(oRealizationParams.rcSourceContained.bottom) < oRealizationParams.uHeight))
                {
                    oRealizationParams.fOnlyContainsSubRectOfSource = true;
                }
            }
        }
        else
        {
            // Must now contain only a subrect because width and height are
            // within texture limits, but before call to
            // ComputeMinimumRealizationBounds at least one exceeded limits.
            oRealizationParams.fOnlyContainsSubRectOfSource = true;

            // Future Consideration:   Inflate minimum realization for scrolling
            //  Scrolling and moving windows scenarios can benefit from
            //  allocating a texture with some padding to avoid recreation with
            //  every change in view of source.
        }
    }

Cleanup:

    RRETURN(hr);

}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::ComputeRealizationParameters
//
//  Synopsis:  Compose a RealizationParameters structure from the given context
//

HRESULT
CHwBitmapColorSource::ComputeRealizationParameters(
    __in_ecount(1) CD3DDeviceLevel1 const *pDevice,
    __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
    __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcRealizationBounds,
    __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> *pmatBitmapToIdealRealization,
    MilPixelFormat::Enum fmtRenderTarget,
    MilBitmapWrapMode::Enum wrapMode,
    MilBitmapInterpolationMode::Enum interpolationMode,
    BOOL fPrefilterEnabled,
    REAL rPrefilterThreshold,
    BOOL fCanFallback,
    __out_ecount(1) RealizationParameters &oRealizationParams
    )
{
    HRESULT hr = S_OK;

    //
    // Determine texture properties
    //

    oRealizationParams.interpolationMode = interpolationMode;

    if (DoesUseMipMapping(interpolationMode))
    {
        Assert(!pDevice->IsSWDevice());

        Assert(   pDevice->CanAutoGenMipMap()
               || pDevice->CanStretchRectGenMipMap()
                  );

        oRealizationParams.eMipMapLevel = TMML_All;
    }
    else
    {
        oRealizationParams.eMipMapLevel = TMML_One;
    }

    oRealizationParams.wrapMode = wrapMode;
    oRealizationParams.fMinimumRealizationRectRequiredComputed = false;

    //
    // Determine texture format
    //

    MilPixelFormat::Enum fmtBitmapSource;
    IFC(pIBitmapSource->GetPixelFormat(&fmtBitmapSource));

    // The border color can have alpha even though the image doesn't.  Theoretically
    // D3D supports a border color of RGBA even when the texture is RGB but it
    // doesn't seem to work.
    // NOTE that we currently only use transparent border and we have no plans to
    // use any other border color so we don't bother to check whether the border
    // is opaque.
    bool fForceAlpha = (wrapMode == MilBitmapWrapMode::Border);
    
    MIL_THR(pDevice->GetSupportedTextureFormat(fmtBitmapSource,
                                               fmtRenderTarget,
                                               fForceAlpha,
                                               OUT oRealizationParams.fmtTexture));
    //
    // Any changes are unsupported
    //

    if (FAILED(hr))
    {
        // Change to E_NOTIMPL for fallback when available
        if (fCanFallback)
        {
            IFC(E_NOTIMPL);
        }
        goto Cleanup;
    }

    //
    // Determine texture size
    //

    IFC(pIBitmapSource->GetSize(&oRealizationParams.uBitmapWidth,
                                &oRealizationParams.uBitmapHeight));

    UINT uMaxTextureWidth = pDevice->GetMaxTextureWidth();
    UINT uMaxTextureHeight = pDevice->GetMaxTextureHeight();

    IFC(ComputeRealizationSize(
        uMaxTextureWidth,
        uMaxTextureHeight,
        rcRealizationBounds,
        pmatBitmapToIdealRealization,
        wrapMode,
        fPrefilterEnabled,
        rPrefilterThreshold,
        fCanFallback,
        IN OUT oRealizationParams
        ));


    //
    // Determine texture layout and wrapping support
    //

    //
    // Start with natural size and layout
    //

    oRealizationParams.dlU.uLength = oRealizationParams.rcSourceContained.Width<UINT>();
    oRealizationParams.dlU.eLayout = NaturalTexelLayout;

    oRealizationParams.dlV.uLength = oRealizationParams.rcSourceContained.Height<UINT>();
    oRealizationParams.dlV.eLayout = NaturalTexelLayout;

    //
    // Break down wrap mode into default texture addressing modes
    //  These may be adjusted later when texture layout is determined.
    //

    ConvertWrapModeToTextureAddressModes(
        wrapMode,
        &oRealizationParams.dlU.d3dta,
        &oRealizationParams.dlV.d3dta
        );

    //
    // Fix up layouts for non-power of two restrictions
    //

    if (DoesUseMipMapping(oRealizationParams.interpolationMode))
    {
        //
        // For mip-mapping the texture dimensions must be a power of two, but
        // when prefiltering is disabled the dimensions may not yet be a power
        // of two.  Handle that now.
        //
        // Note: FirstOnlyTexelLayout allows spare texels to be random and
        //       contribute garbage if ever sampled.
        //

        if (!IS_POWER_OF_2(oRealizationParams.dlU.uLength))
        {
            Assert(!fPrefilterEnabled);
            oRealizationParams.dlU.uLength = RoundToPow2(oRealizationParams.dlU.uLength);
            oRealizationParams.dlU.eLayout = FirstOnlyTexelLayout;
        }

        if (!IS_POWER_OF_2(oRealizationParams.dlV.uLength))
        {
            Assert(!fPrefilterEnabled);
            oRealizationParams.dlV.uLength = RoundToPow2(oRealizationParams.dlV.uLength);
            oRealizationParams.dlV.eLayout = FirstOnlyTexelLayout;
        }
    }
    else if (pDevice->SupportsTextureCap(D3DPTEXTURECAPS_POW2))
    {
        if (!IS_POWER_OF_2(oRealizationParams.dlU.uLength))
        {
            if (oRealizationParams.rcSourceContained.Width<UINT>() != oRealizationParams.uWidth)
            {
                //
                // If the Source Width and realization width aren't the same,
                // then we went through the alternative size logic, and don't
                // need to do anything here other than make sure wrap mode is
                // clamp (extend).
                //
                // Note that if the length is a power of two then wrap mode can
                // be left alone even if we are only dealing with a subportion
                // of the source.  ComputeAlternateMinimumRealizationSize makes
                // sure that all samples needed are included in the texture.
                //
                oRealizationParams.dlU.d3dta = D3DTADDRESS_CLAMP;
            }
            else
            {
                D3DTEXTUREADDRESS d3dtaOriginal = oRealizationParams.dlU.d3dta;

                IFC(AdjustLayoutForConditionalNonPowerOfTwo(
                        oRealizationParams.dlU,
                        uMaxTextureWidth
                        ));

                if (!fCanFallback && oRealizationParams.dlU.eLayout != NaturalTexelLayout)
                {
                    //
                    // Adjust to Natural Layout, the proper wrapping mode, and
                    // a power of 2 size.
                    //

                    oRealizationParams.uWidth = RoundToPow2(oRealizationParams.uWidth);
                    oRealizationParams.dlU.uLength = oRealizationParams.uWidth;

                    oRealizationParams.dlU.eLayout = NaturalTexelLayout;
                    oRealizationParams.dlU.d3dta = d3dtaOriginal;
                    Assert(oRealizationParams.rcSourceContained.left == 0);
                    oRealizationParams.rcSourceContained.right = oRealizationParams.uWidth;

                }
            }
        }

        if (!IS_POWER_OF_2(oRealizationParams.dlV.uLength))
        {
            if (oRealizationParams.rcSourceContained.Height<UINT>() != oRealizationParams.uHeight)
            {
                //
                // If the Source Height and realization height aren't the same,
                // then we went through the alternative size logic, and don't
                // need to do anything here other than make sure wrap mode is
                // clamp (extend).
                //
                // Note that if the length is a power of two then wrap mode can
                // be left alone even if we are only dealing with a subportion
                // of the source.  ComputeAlternateMinimumRealizationSize makes
                // sure that all samples needed are included in the texture.
                //
                oRealizationParams.dlV.d3dta = D3DTADDRESS_CLAMP;
            }
            else
            {
                D3DTEXTUREADDRESS d3dtaOriginal = oRealizationParams.dlV.d3dta;

                IFC(AdjustLayoutForConditionalNonPowerOfTwo(
                    oRealizationParams.dlV,
                    uMaxTextureHeight
                    ));

                if (!fCanFallback && oRealizationParams.dlV.eLayout != NaturalTexelLayout)
                {
                    //
                    // Adjust to Natural Layout, the proper wrapping mode, and
                    // a power of 2 size.
                    //

                    oRealizationParams.uHeight = RoundToPow2(oRealizationParams.uHeight);
                    oRealizationParams.dlV.uLength = oRealizationParams.uHeight;
                    oRealizationParams.dlV.eLayout = NaturalTexelLayout;
                    oRealizationParams.dlV.d3dta = d3dtaOriginal;
                        
                    Assert(oRealizationParams.rcSourceContained.top == 0);
                    oRealizationParams.rcSourceContained.bottom = oRealizationParams.uHeight;

                }
            }
        }

        IFC(ReconcileLayouts(oRealizationParams, uMaxTextureWidth, uMaxTextureHeight));
    }

    #if DBG
    {
        //
        // Assert that a texture may be created with the current requirements
        //  description.  S_FALSE indicates either width or height is too big;
        //  so we only accept S_OK.
        //

        D3DSURFACE_DESC d3dsdRequired;
        UINT uLevels;

        GetD3DSDRequired(
            pDevice,
            oRealizationParams,
            &d3dsdRequired,
            &uLevels
            );

        Assert(pDevice->GetMinimalTextureDesc(
            &d3dsdRequired,
            TRUE,
            (GMTD_CHECK_ALL |
             (TextureAddressingAllowsConditionalNonPower2Usage(
                 oRealizationParams.dlU.d3dta,
                 oRealizationParams.dlV.d3dta) ?
              GMTD_NONPOW2CONDITIONAL_OK : 0)
            )
            ) == S_OK);
    }
    #endif

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::GetD3DSDRequired
//
//  Synopsis:  Returns the surface description required for the realization
//             params.
//
//-----------------------------------------------------------------------------
VOID
CHwBitmapColorSource::GetD3DSDRequired(
    __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) const CHwBitmapColorSource::CacheParameters &oRealizationParams,
    __out_ecount(1) D3DSURFACE_DESC *pd3dsdRequired,
    __out_ecount(1) UINT *puLevels
    )
{
    Assert(   oRealizationParams.eMipMapLevel == TMML_One
           || oRealizationParams.eMipMapLevel == TMML_All
              );

    pd3dsdRequired->Format = PixelFormatToD3DFormat(oRealizationParams.fmtTexture);
    pd3dsdRequired->Type = D3DRTYPE_TEXTURE;

    CD3DTexture::DetermineUsageAndLevels(
        pDevice,
        oRealizationParams.eMipMapLevel,
        oRealizationParams.dlU.uLength,
        oRealizationParams.dlV.uLength,
        &pd3dsdRequired->Usage,
        puLevels
        );

    pd3dsdRequired->Pool = D3DPOOL_DEFAULT;
    pd3dsdRequired->MultiSampleType = D3DMULTISAMPLE_NONE;
    pd3dsdRequired->MultiSampleQuality = 0;
    pd3dsdRequired->Width = oRealizationParams.dlU.uLength;
    pd3dsdRequired->Height = oRealizationParams.dlV.uLength;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::AdjustLayoutForConditionalNonPowerOfTwo
//
//  Synopsis:  Adjust the given natural length and texture addressing mode to
//             accommodate conditional non-power of two support
//
//  Notes:     There are major considerations when deciding how to populate the
//             texture.  The first is whether the device supports non-power of
//             two textures unconditionally and the second is who to pad any
//             space not covered by the natural image samples.  (The natural
//             image samples are the samples within the bounds of the image.
//             Non-natural samples would be those outside the bounds which are
//             defined by the wrapping mode.)
//
//             Layout cases (per dimension):
//              1. Unconditional non-power of two support or power of two source
//                  - Populate 1:1
//                  - device will handle all wrap cases
//                  - Use direct conversion to DX wrap mode (texture addr mode)
//
//                  Source                     Texture
//                  +---+---+---+---+---+      +---+---+---+---+---+
//                  | I | m | a | g | e |  =>  | I | m | a | g | e |
//                  +---+---+---+---+---+      +---+---+---+---+---+
//
//
//              2. Extend Wrap Mode (Clamp in DX)
//                  - Populate 1:1
//                  - Device will handle case via conditional non-power of two
//                  - Use DX Clamp
//
//                  Source                     Texture
//                  +---+---+---+---+---+      +---+---+---+---+---+
//                  | I | m | a | g | e |  =>  | I | m | a | g | e |
//                  +---+---+---+---+---+      +---+---+---+---+---+
//
//
//              3. Tile (Wrap in DX)
//                  - Pad 1 texel on each end; fill with opposing texel
//                  - Use bump map to keep texture coordinates with in [0, 1]
//                  - Use DX Clamp to allow conditional non-power two support
//
//                  Source                     Texture
//                  +---+---+---+---+---+      +---+---+---+---+---+---+---+
//                  | I | m | a | g | e |  =>  | e | I | m | a | g | e | I |
//                  +---+---+---+---+---+      +---+---+---+---+---+---+---+
//
//              4. Mirror
//                  - Mirror source once and treat as source to tile; pad 1
//                    texel on each end; fill with adjacent (=opposing) texel
//                  - Use bump map to keep texture coordinates with in [0, 1]
//                  - Use DX Clamp to allow conditional non-power two support
//
//                  Source
//                  +---+---+---+---+---+
//                  | I | m | a | g | e |  =>
//                  +---+---+---+---+---+
//
//                  Texture
//                  +---+---+---+---+---+---+---+---+---+---+---+---+
//                  | I | I | m | a | g | e | e | g | a | m | I | I |
//                  +---+---+---+---+---+---+---+---+---+---+---+---+
//
//

HRESULT
CHwBitmapColorSource::AdjustLayoutForConditionalNonPowerOfTwo(
    IN OUT DimensionLayout &dl,
    IN UINT uMaxLength
    )
{
    HRESULT hr = S_OK;

    Assert(dl.uLength > 0);
    Assert(dl.uLength <= uMaxLength);

    switch (dl.d3dta)
    {
    case D3DTADDRESS_WRAP:
        if (dl.uLength + 2 <= uMaxLength)
        {
            dl.uLength += 2;
            dl.eLayout = EdgeWrappedTexelLayout;
            dl.d3dta = D3DTADDRESS_CLAMP;
        }
        else
        {
            hr = THR(E_NOTIMPL);
        }
        break;

    case D3DTADDRESS_MIRROR:
        if (dl.uLength + 2 <= uMaxLength)
        {
            dl.uLength += 2;
            dl.eLayout = EdgeMirroredTexelLayout;
            dl.d3dta = D3DTADDRESS_CLAMP;
        }
        else
        {
            hr = THR(E_NOTIMPL);
        }
        break;

    case D3DTADDRESS_CLAMP:
        // Conditional non-power of two support handles this case
        dl.eLayout = NaturalTexelLayout;
        break;
        
    case D3DTADDRESS_BORDER:
    default:
        //  This is only hit for 3D with trilinear
        // disabled which is just the dwm for now.
        break;
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::ReconcileLayouts
//
//  Synopsis:
//      If one dimension is using non-natural layout but the other is not
//      (because, for example one was pow 2 and the other wasn't) this forces
//      them to both be non-natural.
//
//  Notes:
//      Up until this point U and V are treated individually, but D3D does not
//      always treat them as such.  For conditional non-power of two support
//      both U and V must have a clamp wrapping mode, even if one dimension is
//      a power of two.
//
//      This routine is used to ensure that clamp mode, and therefore non-power
//      of two conditional support, can be used.
//
//      Technically if one direction was Edge* layout and the other is using
//      mirroring then clamp w/ natural layout would be fine, but still need
//      waffling.  To keep border update and waffling code simple we do not use
//      this optimization.
//
//      Another solution could be to bump the non-natural texture to a power of
//      two, but that can waste a lot of space.  Especially if the natural size
//      is 1 less than a power of two and tiling is requested.
//
//-----------------------------------------------------------------------------
HRESULT
CHwBitmapColorSource::ReconcileLayouts(
    __inout_ecount(1) RealizationParameters &rp,
    UINT uMaxWidth,
    UINT uMaxHeight
    )
{
    HRESULT hr = S_OK;

    if (rp.dlU.eLayout == NaturalTexelLayout && rp.dlV.eLayout != NaturalTexelLayout)
    {
        // Don't expect other layouts like FirstOnly here
        Assert(rp.dlV.eLayout == EdgeWrappedTexelLayout ||
               rp.dlV.eLayout == EdgeMirroredTexelLayout);
        IFC(AdjustLayoutForConditionalNonPowerOfTwo(rp.dlU, uMaxWidth));
    }
    else if (rp.dlV.eLayout == NaturalTexelLayout && rp.dlU.eLayout != NaturalTexelLayout)
    {
        // Don't expect other layouts like FirstOnly here
        Assert(rp.dlU.eLayout == EdgeWrappedTexelLayout ||
               rp.dlU.eLayout == EdgeMirroredTexelLayout);
        IFC(AdjustLayoutForConditionalNonPowerOfTwo(rp.dlV, uMaxHeight));
    }

  Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::Create
//
//  Synopsis:  Creates a HW bitmap color source
//
//-----------------------------------------------------------------------------

HRESULT
CHwBitmapColorSource::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount_opt(1) IWGXBitmap *pBitmap,
    __in_ecount(1) const CHwBitmapColorSource::CacheParameters &oRealizationDesc,
    bool fCreateAsRenderTarget,
    __deref_out_ecount(1) CHwBitmapColorSource **ppHwBitmapCS
    )
{
    HRESULT hr = S_OK;

    //
    // Underlying texture/surface description is not allowed to change over
    // time.  Compute it now and send to the constructor.
    //

    D3DSURFACE_DESC d3dsd;
    UINT uLevels;

    GetD3DSDRequired(
        pDevice,
        oRealizationDesc,
        &d3dsd,
        &uLevels
        );

    //
    // In the case there is an existing realization with reusable source, a
    // StretchRect may need to be performed to this new texture.  There are two
    // cases when a render target is required by StretchRect.
    //   1. The destination must be a render target.
    //   2. For DX8 hardware the source must also be a render target.  Caller
    //      is responsible for checking "can StretchRect from textures cap" and
    //      making a decision.
    //
    if (fCreateAsRenderTarget)
    {
        d3dsd.Usage |= D3DUSAGE_RENDERTARGET;
    }

    AssertMinimalTextureDesc(
        pDevice,
        oRealizationDesc.dlU.d3dta,
        oRealizationDesc.dlV.d3dta,
        &d3dsd
        );

    *ppHwBitmapCS = new CHwBitmapColorSource(
        pDevice,
        pBitmap,
        oRealizationDesc.fmtTexture,
        d3dsd,
        uLevels
        );
    IFCOOM(*ppHwBitmapCS);
    (*ppHwBitmapCS)->AddRef();

Cleanup:
    if (FAILED(hr))
    {
        Assert(*ppHwBitmapCS == NULL);
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::CHwBitmapColorSource
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------

CHwBitmapColorSource::CHwBitmapColorSource(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount_opt(1) IWGXBitmap *pBitmap,
    MilPixelFormat::Enum fmt,
    __in_ecount(1) const D3DSURFACE_DESC &d3dsd,
    UINT uLevels
    ) :
    CHwTexturedColorSource(pDevice),
    m_pBitmap(pBitmap),
    m_fmtTexture(fmt),
    m_d3dsdRequired(d3dsd),
    m_uLevels(uLevels)
{
    m_pVidMemOnlyTexture = NULL;
    m_pIBitmapSource = NULL;
    m_uCachedUniquenessToken = 0;   // Not required since rcCached is being set
                                    // to empty, but nice to set to zero for
                                    // debugging.
    m_rcCachedRealizationBounds.SetEmpty();
    m_rcRequiredRealizationBounds.SetEmpty();
    m_pvReferencedSystemBits = NULL;
    m_pD3DSysMemRefSurface = NULL;
    m_pbcsRealizationSources = NULL;

#if DBG
    // Set the source here to enable an assertion in SetBitmapAndContext that
    // the bitmap source doesn't change when there is a IWGXBitmap.
    m_pIBitmapSourceDBG = pBitmap;
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::~CHwBitmapColorSource
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------

CHwBitmapColorSource::~CHwBitmapColorSource()
{
    ReleaseInterfaceNoNULL(m_pVidMemOnlyTexture);
    ReleaseInterfaceNoNULL(m_pD3DSysMemRefSurface);
    ReleaseInterfaceNoNULL(m_pbcsRealizationSources);
    // No Reference held for m_pIBitmapSourceBrush
    //ReleaseInterfaceNoNULL(m_pIBitmapSourceBrush);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::CheckRequiredRealizationBounds
//
//  Synopsis:
//      Return true if color source has a realization of required sampling
//      bounds.
//
//-----------------------------------------------------------------------------

bool
CHwBitmapColorSource::CheckRequiredRealizationBounds(
    __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds,
    MilBitmapInterpolationMode::Enum interpolationMode,
    MilBitmapWrapMode::Enum wrapMode,
    RequiredBoundsCheck::Enum eCheckRequest
    )
{
    bool fCoversRequiredBounds = false;

    //
    // Make a quick check for common case of system memory bitmap. See if all
    // that could be required is already covered w/o computing actual bounds.
    //
    if (   (eCheckRequest == RequiredBoundsCheck::CheckRequired)
        && (m_uPrefilterWidth == m_rcRequiredRealizationBounds.Width<UINT>())
        && (m_uPrefilterHeight == m_rcRequiredRealizationBounds.Height<UINT>())
       )
    {
        fCoversRequiredBounds = true;
    }
    //
    // Actual bounds are needed or color source only has a partial realization.
    //
    else
    {
        //
        // Compute minimum required bounds
        //

        CMilRectU rcReqBounds(0,0,m_uPrefilterWidth,m_uPrefilterHeight,XYWH_Parameters);
        InternalRealizationParameters oRealizationParams;
        oRealizationParams.interpolationMode = interpolationMode;
        oRealizationParams.uBitmapWidth = m_uBitmapWidth;
        oRealizationParams.uBitmapHeight = m_uBitmapHeight;
        oRealizationParams.wrapMode = wrapMode;

        if (ComputeMinimumRealizationBounds(
            rcRealizationBounds,
            oRealizationParams,
            IN OUT rcReqBounds
            ))
        {
            //
            // Select bounds rect to compare against
            //

            CMilRectU const &rcCheckBounds =
                (eCheckRequest == RequiredBoundsCheck::CheckRequired) ?
                    m_rcRequiredRealizationBounds :
                (eCheckRequest == RequiredBoundsCheck::CheckCached) ?
                    m_rcCachedRealizationBounds :
                    m_rcPrefilteredBitmap;

            //
            // Check if bounds are covered.
            //

            if (rcCheckBounds.DoesContain(rcReqBounds))
            {
                fCoversRequiredBounds = true;

                // Update required bounds if requested
                if (eCheckRequest == RequiredBoundsCheck::CheckPossibleAndUpdateRequired)
                {
                    m_rcRequiredRealizationBounds = rcReqBounds;
                }
            }
        }
    }

    return fCoversRequiredBounds;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::CalcTextureTransform
//
//  Synopsis:  Sets the matrix which transforms points from device space
//             to source space.
//

HRESULT
CHwBitmapColorSource::CalcTextureTransform(
    __in_ecount(1) const BitmapToXSpaceTransform *pBitmapToXSpaceTransform
    )
{
    HRESULT hr = S_OK;

    //
    // Compute textured color source to device transform
    //
    // The color source space is the same as D3D texture space, which is
    // normalized.  However all other coordinate spaces employed here are not
    // normalized.
    //
    // The bitmap to device transform is given, so the texture to bitmap
    // transform needs to be calculated.  The texture to bitmap transform can be
    // broken down into the transforms:
    //   1. (normalized) Texture to (non-normalized) Texels
    //   2. Texels to Prefiltered
    //   3. Prefiltered to Bitmap
    //
    // These transforms are:
    //
    //  1. Texture to Texels - scale by texel count for width and height.  For
    //     Edge Wrapped/Mirrored Texel Layouts, texture is also offset +1 texel
    //     in from left and/or top edge, which would mean a -1 non-normalized
    //     texel translate.  That offset or a modified one to enable other
    //     special cases is handled by call to SetWaffling in
    //     CHwBitmapColorSource::SendVertexMapping.  In other cases the inset
    //     is 0.

    //      [ m_d3dsdRequired.Width         0                           0   ]
    //      [ 0                             m_d3dsdRequired.Height      0   ]
    //      [ 0                             0                           1   ]

    //  2. Texel to Prefiltered - translate by prefilter left-top location

    //      [ 1                             0                           0   ]
    //      [ 0                             1                           0   ]
    //      [ m_rcPrefilteredBitmap.left    m_rcPrefilteredBitmap.top   1   ]

    //  3. Prefiltered to Bitmap - scale by prefiltering scale

    //      [ m_uBitmapWidth                0                           0
    //         / m_uPrefilterWidth                                          ]
    //      [ 0                             m_uBitmapHeight             0
    //                                       / m_uPrefilterHeight           ]
    //      [ 0                             0                           1   ]

    // Texture to Prefiltered is a trivial matrix multiplication

    //      [ m_d3dsdRequired.Width         0                           0   ]
    //      [ 0                             m_d3dsdRequired.Height      0   ]
    //      [ m_rcPrefilteredBitmap.left    m_rcPrefilteredBitmap.top   1   ]

    //  Texture to Prefiltered x Prefiltered to Bitmap is then

    //      [ m_d3dsdRequired.Width         0                           0 
    //         * m_uBitmapWidth
    //         / m_uPrefilterWidth                                          ]
    //
    //      [ 0                             m_d3dsdRequired.Height      0 
    //                                       * m_uBitmapHeight
    //                                       / m_uPrefilterHeight           ]
    //
    //      [ m_rcPrefilteredBitmap.left    m_rcPrefilteredBitmap.top   1
    //         * m_uBitmapWidth              * m_uBitmapHeight
    //         / m_uPrefilterWidth           / m_uPrefilterHeight           ]

    //
    // There are several common, special cases when these calculations can be
    // simplified.
    //
    //   Case 1 - no prefiltering m_uPrefilter* == m_uBitmap*
    //   Case 2 - m_rcPrefilteredBitmap contains prefiltered bitmap source
    //
    // The X and Y transforms are mostly independent of one another so we
    // can apply these optimizations independently if we are careful about
    // translation component.
    //

    UINT uTextureWidth = m_d3dsdRequired.Width;
    UINT uTextureHeight = m_d3dsdRequired.Height;

    //
    // When waffling, the goal is not actually to compute transform to texture
    // space, but to a normalized space that clearly delineates waffle
    // boundaries.  So from texels convert to normalized texel span / base
    // tile, which is simply texel space divided by prefilter width and height
    // stored in texture.  This is directly done by substituting prefiltered
    // width/height for texture width/height for the two waffle modes:
    // EdgeWrappedTexelLayout and EdgeMirroredTexelLayout.
    //

    if (m_tlU == EdgeWrappedTexelLayout || m_tlU == EdgeMirroredTexelLayout)
    {
        uTextureWidth = m_rcPrefilteredBitmap.Width<UINT>();
    }
    if (m_tlV == EdgeWrappedTexelLayout || m_tlV == EdgeMirroredTexelLayout)
    {
        uTextureHeight = m_rcPrefilteredBitmap.Height<UINT>();
    }

    //
    // Expectations for relationship between actual texture texels and stored
    // texels based on texture layout:
    //

    switch (m_tlU)
    {
    case NaturalTexelLayout:
        Assert(m_d3dsdRequired.Width == m_rcPrefilteredBitmap.Width<UINT>()); break;
    case EdgeWrappedTexelLayout:
    case EdgeMirroredTexelLayout:
        Assert(m_d3dsdRequired.Width == m_rcPrefilteredBitmap.Width<UINT>() + 2); break;
    case FirstOnlyTexelLayout:
        Assert(m_d3dsdRequired.Width > m_rcPrefilteredBitmap.Width<UINT>()); break;
    default:
        NO_DEFAULT("Unrecognized U texel layout");
    }
    switch (m_tlV)
    {
    case NaturalTexelLayout:
        Assert(m_d3dsdRequired.Height == m_rcPrefilteredBitmap.Height<UINT>()); break;
    case EdgeWrappedTexelLayout:
    case EdgeMirroredTexelLayout:
        Assert(m_d3dsdRequired.Height == m_rcPrefilteredBitmap.Height<UINT>() + 2); break;
    case FirstOnlyTexelLayout:
        Assert(m_d3dsdRequired.Height > m_rcPrefilteredBitmap.Height<UINT>()); break;
    default:
        NO_DEFAULT("Unrecognized V texel layout");
    }


    MILMatrix3x2 matSourceToXSpace;

    MILMatrix3x2 matSourceToPrefiltered(
        static_cast<float>(uTextureWidth),   0,
        0,  static_cast<float>(uTextureHeight),

        static_cast<float>(m_rcPrefilteredBitmap.left),
        static_cast<float>(m_rcPrefilteredBitmap.top)
        );

    float rWidthPrefilterScale =
        static_cast<float>(m_uBitmapWidth)
         / static_cast<float>(m_uPrefilterWidth);

    float rHeightPrefilterScale =
        static_cast<float>(m_uBitmapHeight)
         / static_cast<float>(m_uPrefilterHeight);

    MILMatrix3x2 matPrefilteredToXSpace(
        pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.m[0][0] * rWidthPrefilterScale,
        pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.m[0][1] * rWidthPrefilterScale,
        
        pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.m[1][0] * rHeightPrefilterScale,
        pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.m[1][1] * rHeightPrefilterScale,

        pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.GetDx(),   pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.GetDy()
        );

    matSourceToXSpace.SetProduct(matSourceToPrefiltered, matPrefilteredToXSpace);

    if (!m_matXSpaceToTextureUV.SetInverse(
            //matSourceToDevice
            matSourceToXSpace.m_00,
            matSourceToXSpace.m_01,
            matSourceToXSpace.m_10,
            matSourceToXSpace.m_11,
            matSourceToXSpace.m_20,
            matSourceToXSpace.m_21
            ))
    {
        IFC(WGXERR_NONINVERTIBLEMATRIX);
    }

    // Reset shader handle for this context use
    ResetShaderTextureTransformHandle();

#if DBG
    DbgMarkXSpaceToTextureUVAsSet(pBitmapToXSpaceTransform->dbgXSpaceDefinition);
#endif

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::IsRealizationCurrent
//
//  Synopsis:
//      Checks whether cached content is current with source, independent of
//      whether enough area of required realization is present.
//
//-----------------------------------------------------------------------------

bool
CHwBitmapColorSource::IsRealizationCurrent() const
{
    bool fCurrentRealization = true;

    if (m_pBitmap)
    {
        UINT uBitmapUniquenessToken;
        m_pBitmap->GetUniquenessToken(&uBitmapUniquenessToken);
        if (m_uCachedUniquenessToken != uBitmapUniquenessToken)
        {
            fCurrentRealization = false;
        }
    }

    return fCurrentRealization;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::IsRealizationValid
//
//  Synopsis:
//      Checks whether cached content is realized for current requirements and
//      state of source, if source is a bitmap whose contents may change.
//

bool
CHwBitmapColorSource::IsRealizationValid() const
{
    return
        m_rcCachedRealizationBounds.DoesContain(m_rcRequiredRealizationBounds)
     && IsRealizationCurrent();
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::SetBitmapAndContextCacheParameters
//
//  Synopsis:
//      Set basic context parameters from pBitmapSournce and CacheParameters
//      struct
//

void
CHwBitmapColorSource::SetBitmapAndContextCacheParameters(
    __in_ecount(1) IWGXBitmapSource *pBitmapSource,
    __in_ecount(1) const CacheParameters &oRealizationParams
    )
{

#if DBG
    if (m_pIBitmapSourceDBG != pBitmapSource)
    {
        // Current caching prevents the source from changing except for the
        // initial call.  See CHwBitmapCache::ChooseBitmapColorSource's cache
        // destruction for more.  (To disable cache destruction you need to
        // make this whole block work under free, disable this assert, set
        // m_fValidRealization to false, replace m_pIBitmapSourceDBG with
        // m_pIBitmapSource, and remove appropriate m_pIBitmapSource sets.
        Assert(m_pIBitmapSourceDBG == NULL);

        // source should never change if this is associated with a IWGXBitmap
        Assert(!m_pBitmap);

        // If the source is changing we should have been fully invalidated
        Assert(m_rcCachedRealizationBounds.IsEmpty());

        m_pIBitmapSourceDBG = pBitmapSource;
        // No Reference held for m_pIBitmapSourceDBG
        //m_pIBitmapSourceDBG->AddRef();
    }
#endif DBG

    m_pIBitmapSource = pBitmapSource;
    // No Reference held for m_pIBitmapSource
    //m_pIBitmapSource->AddRef();

    Assert(m_fmtTexture == oRealizationParams.fmtTexture);

    m_uPrefilterWidth = oRealizationParams.uWidth;
    m_uPrefilterHeight = oRealizationParams.uHeight;

    m_rcPrefilteredBitmap = oRealizationParams.rcSourceContained;

    AssertMinimalTextureDesc(
        m_pDevice,
        oRealizationParams.dlU.d3dta,
        oRealizationParams.dlV.d3dta,
        &m_d3dsdRequired
        );

    m_tlU = oRealizationParams.dlU.eLayout;
    m_tlV = oRealizationParams.dlV.eLayout;

    SetWrapModes(
        oRealizationParams.dlU.d3dta,
        oRealizationParams.dlV.d3dta
        );

    return;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::GetPointerToValidSourceRects
//
//  Synopsis:
//      Return list of valid source rects which for this color source is always
//      just the require realization bounds rectangle.  List ownership is not
//      given to caller.
//
//-----------------------------------------------------------------------------

HRESULT
CHwBitmapColorSource::GetPointerToValidSourceRects(
    __in_ecount_opt(1) IWGXBitmap *pBitmap,
    __out_ecount(1) UINT &cValidSourceRects,
    __deref_out_ecount_full(cValidSourceRects) CMilRectU const * &rgValidSourceRects
    ) const
{
    UNREFERENCED_PARAMETER(pBitmap);
    cValidSourceRects = 1;
    rgValidSourceRects = &m_rcRequiredRealizationBounds;
    RRETURN(S_OK);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::SetBitmapAndContext
//
//  Synopsis:  Set the current context and bitmap this color source is to
//             realize
//

HRESULT
CHwBitmapColorSource::SetBitmapAndContext(
    __in_ecount(1) IWGXBitmapSource *pBitmapSource,
    __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcRealizationBounds,
    __in_ecount(1) const BitmapToXSpaceTransform *pBitmapToXSpaceTransform,
    __in_ecount(1) const RealizationParameters &oRealizationParams,
    __in_ecount_opt(1) CHwBitmapColorSource *pbcsWithReusableRealizationSource
    )
{
    HRESULT hr = S_OK;

    // Parameter Assertions
    Assert(pBitmapSource);
    Assert(pBitmapToXSpaceTransform);

    SetBitmapAndContextCacheParameters(
        pBitmapSource,
        oRealizationParams
        );

    m_rcRequiredRealizationBounds = m_rcPrefilteredBitmap;

    //
    // When shared surfaces are the source copying the bits can be expensive so
    // compute the minimum realization required.  This is particularly
    // profitable when only some part of a DX window hangs over to the
    // non-native device.
    //
    // When oRealizationParams.fMinimumRealizationRectRequiredComputed is true
    // then oRealizationParams.rcSourceContained will have the minimal bounds. 
    // At this point oRealizationParams.rcSourceContained will have been
    // transferred to m_rcPrefilteredBitmap by
    // SetBitmapAndContextCacheParameters and then to
    // m_rcRequiredRealizationBounds just above.
    //

    if (!oRealizationParams.fMinimumRealizationRectRequiredComputed)
    {
        // ComputeMinimumRealizationBounds expects a rectangle covering full
        // prefiltered expanse. Conveniently m_rcRequiredRealizationBounds has
        // just set been set to m_rcPrefilteredBitmap and since
        // fMinimumRealizationRectRequiredComputed is not set the result must
        // be a full coverage rectangle.
        Assert(m_rcRequiredRealizationBounds.Width<UINT>() == m_uPrefilterWidth);
        Assert(m_rcRequiredRealizationBounds.Height<UINT>() == m_uPrefilterHeight);

        // Check for shared surface source
        if (   m_pBitmap
            && m_pBitmap->SourceState() == IWGXBitmap::SourceState::DeviceBitmap)
        {
            CDeviceBitmap *pBitmap =
                DYNCAST(CDeviceBitmap, m_pBitmap);
            Assert(pBitmap);

            //
            // Before computing minimum realization bounds because of copying
            // through system memory, check that contributions may indeed come
            // from a different adapter.  If all contributions come from this
            // adapter then UpdateFromReusableSource should handle the texture
            // population via StretchRect.  So check that (1) there is a NULL
            // reusable realization source list or (2) source from a different
            // adapter.
            //
            if (   pbcsWithReusableRealizationSource == NULL
                || pBitmap->HasContributorFromDifferentAdapter(
                    m_pDevice->GetD3DAdapterLUID()
                    )
               )
            {
                // Determine the least amount of realization work possible
                ComputeMinimumRealizationBounds(
                    rcRealizationBounds,
                    oRealizationParams,
                    IN OUT m_rcRequiredRealizationBounds
                    );
            }
        }
    }

    //
    // Set realization sources to given reusable realization sources for this
    // context.
    //
    CheckAndSetReusableSources(pbcsWithReusableRealizationSource);

    m_uBitmapWidth = oRealizationParams.uBitmapWidth;
    m_uBitmapHeight = oRealizationParams.uBitmapHeight;

    SetFilterMode(
        oRealizationParams.interpolationMode
        );
    
    IFC(CalcTextureTransform(
        pBitmapToXSpaceTransform
        ));

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::DoPrefilterDimensionsMatch
//
//  Synopsis:
//      Compare prefiltering settings to given dimension and return true if
//      they are compatible.
//
//-----------------------------------------------------------------------------

bool
CHwBitmapColorSource::DoPrefilterDimensionsMatch(
    UINT uWidth,
    UINT uHeight
    ) const
{
    return (   (uWidth == m_uPrefilterWidth)
            && (uHeight == m_uPrefilterHeight));
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::CheckAndSetReusableSource
//
//  Synopsis:
//      Check if reusable source is may actually be reused.  There are a couple
//      requirements:
//          1) StretchRect will be possible 9.0 driver support or reusable
//             source created as render target texture.
//          2) There is overlap in the realization areas of each color source.
//          3) There is some difference caching/validity from this color
//             source.
//          4) There is not a IWGXBitmap or the reusable source is not completely
//             dirty.
//

void CHwBitmapColorSource::CheckAndSetReusableSource(
    __inout_ecount(1) CHwBitmapColorSource *pbcsWithReusableRealizationSource
    )
{
    Assert(pbcsWithReusableRealizationSource->m_pbcsRealizationSources == NULL);

    // Two color sources' notion of IWGXBitmap should be the same or the
    // reusable source's notion should be NULL.  A difference, with
    // reusable's being NULL, indicates reusable is a read-only (shared)
    // surface.
    Assert(   (m_pBitmap == pbcsWithReusableRealizationSource->m_pBitmap)
           || (pbcsWithReusableRealizationSource->m_pBitmap == NULL));

    bool fReuseSource = false;

    if (   // Check if there a potentially reusable source
           pbcsWithReusableRealizationSource->IsValid()
           // Check if StretchRect is even possible
        && (IsARenderTarget())
        && (   m_pDevice->CanStretchRectFromTextures()
            || pbcsWithReusableRealizationSource->IsARenderTarget()
           )
           // Check if prefilter settings are compatible
        && pbcsWithReusableRealizationSource->DoPrefilterDimensionsMatch(
            m_uPrefilterWidth, m_uPrefilterHeight
            )
           // Check if realization areas overlap - useless otherwise
        && m_rcRequiredRealizationBounds.DoesIntersect(
            pbcsWithReusableRealizationSource->m_rcRequiredRealizationBounds
            )
       )
    {
        // Check if there is a CBitmap for reusable source.  No CBitmap
        // indicates that its bits may not be updated from source.
        if (pbcsWithReusableRealizationSource->m_pBitmap == NULL)
        {
            fReuseSource = true;
        }
        // If cached uniqueness tokens are the same then don't bother 
        else if (m_uCachedUniquenessToken == pbcsWithReusableRealizationSource->m_uCachedUniquenessToken)
        {
            Assert(pbcsWithReusableRealizationSource->m_pBitmap == m_pBitmap);

            //
            // Behavior/Performance Note:
            //
            //  Reaching this case means that this color source will be updated
            //  without the reusable source being updated.  However the next
            //  time through this check it is likely the uniquenesses will be
            //  different and the following block (labeled "Different
            //  Uniqueness") will be used.  Then there will be two possible
            //  cases.  1) bitmap hasn't changed, which should result in this
            //  color source not needing re-realized.  Or 2) [the more
            //  interesting case] the bitmap has changed.  In this case, the
            //  bitmap's dirty rect tracking should have advanced such that the
            //  GetDirtyRects call should return false indicating that the
            //  reusable source is still not reusable.
            //
            //  Additionally CHwBitmapCache's logic to return a reusable source
            //  is only expected to return reusable sources created earlier
            //  than the required (this) source; so, if small updates are made
            //  to a large bitmap while this source is being used, but the
            //  reusable is not then the whole bitmap will have to be uploaded
            //  when the reusable source is needed again.
            //
            //  If this is a problem it may be solved in two known ways.  The
            //  first is to remove the above check.  That will effectively
            //  always update the reusable source and then use a vid mem to vid
            //  mem transfer to update this source.  The cost is extra video
            //  memory bandwidth and working set.  The other solution is to
            //  change the cache logic to walk past finding the required source
            //  to look for a potential reusable source.  The problem here is
            //  that a circular realization reference can be introduced so the
            //  appropriate protections would have to be introduced for that.
            //  There is an assert for circular reuse references at the
            //  beginning of this method.  Currently Realize will clear
            //  m_pbcsRealizationSource, but there is no guarantee that Realize
            //  will be called after this.  Consider this scenario:
            //      1. HwBCS A is saved as last used and has realization source
            //         B.
            //      2. Realize is not called for A, because of some other
            //         failure.
            //      3. DeriveFromBrushAndContext is called, but last used fails
            //         to return A.
            //      4. ChooseBitmapColorSource selects B as the source and A as
            //         reusable.
            //      5. Now B could reference A and B could reference A.
            //
        }
        // Different uniqueness
        else
        {
            Assert(pbcsWithReusableRealizationSource->m_pBitmap == m_pBitmap);

            //
            // Check reusable's update status
            //

            const MilRectU *rgDirtyRects;
            UINT cDirtyRects = 0;
            UINT uUniqueness;

            // Check for valid dirty rect information, which means somewhat
            // invalid or completely valid.
            if (pbcsWithReusableRealizationSource->GetDirtyRects(
                   OUT &rgDirtyRects,
                IN OUT &cDirtyRects,
                   OUT &uUniqueness
               ))
            {
                if (   cDirtyRects != 1
                    // 1 dirty: rough check for NOT completely invalid
                    || rgDirtyRects[0].left > 0
                    || rgDirtyRects[0].top > 0
                    || rgDirtyRects[0].right  < m_uBitmapWidth
                    || rgDirtyRects[0].bottom < m_uBitmapHeight
                   )
                {
                    fReuseSource = true;
                }
            }
        }
    }

    if (fReuseSource)
    {
        Assert(pbcsWithReusableRealizationSource->m_pbcsRealizationSources == NULL);
        pbcsWithReusableRealizationSource->m_pbcsRealizationSources =
            m_pbcsRealizationSources;   // Transfer reference, if any
        m_pbcsRealizationSources = pbcsWithReusableRealizationSource;
        m_pbcsRealizationSources->AddRef();
    }
    else
    {
        // Check to see if there is a reusable system memory surface that
        // may be shared
        if (   !m_pD3DSysMemRefSurface
            && pbcsWithReusableRealizationSource->m_pD3DSysMemRefSurface)
        {
            m_pvReferencedSystemBits = pbcsWithReusableRealizationSource->m_pvReferencedSystemBits;
            m_pD3DSysMemRefSurface = pbcsWithReusableRealizationSource->m_pD3DSysMemRefSurface;
            m_pD3DSysMemRefSurface->AddRef();
        }
    }

    return;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::CheckAndSetReusableSources
//
//  Synopsis:
//      Process a list of potentially reusable sources.  See
//      CHwBitmapColorSource::CheckAndSetReusableSource
//
//-----------------------------------------------------------------------------

void
CHwBitmapColorSource::CheckAndSetReusableSources(
    __inout_ecount_opt(1) CHwBitmapColorSource *pbcsWithReusableRealizationSources
    )
{
    if (pbcsWithReusableRealizationSources)
    {
        pbcsWithReusableRealizationSources->AddRef();
    }

    // Clear reusable source list - should already be clear; so this is just
    // in case something prevented clean up of a prior list.
    ReleaseInterface(m_pbcsRealizationSources);

    while (pbcsWithReusableRealizationSources)
    {
        // Remove next item from the list, but remember it and steal reference.
        CHwBitmapColorSource *pbcsNext =
            pbcsWithReusableRealizationSources->m_pbcsRealizationSources;
        pbcsWithReusableRealizationSources->m_pbcsRealizationSources = NULL;

        // Check if current item is reusable
        CheckAndSetReusableSource(pbcsWithReusableRealizationSources);

        // Advance to next item.
        pbcsWithReusableRealizationSources->Release();
        pbcsWithReusableRealizationSources = pbcsNext;
    }

    return;
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::ReleaseRealizationSources
//
//  Synopsis:
//      Release list of realization sources.
//
//-----------------------------------------------------------------------------

void
CHwBitmapColorSource::ReleaseRealizationSources(
    )
{
    // Transfer reference
    CHwBitmapColorSource *pbcs = m_pbcsRealizationSources;
    m_pbcsRealizationSources = NULL;

    while (pbcs)
    {
        // Transfer reference of next from current item to local next pointer
        CHwBitmapColorSource *pbcsNext = pbcs->m_pbcsRealizationSources;
        pbcs->m_pbcsRealizationSources = NULL;

        // Release current item
        pbcs->Release();

        // Advance to next item; transfer reference
        pbcs = pbcsNext;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::CreateTexture
//
//  Synopsis:  Creates the lockable texture to be used with HW
//

HRESULT
CHwBitmapColorSource::CreateTexture(
    bool fIsEvictable,
    __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
    )
{
    HRESULT hr = S_OK;

    Assert(m_pVidMemOnlyTexture == NULL);

    Assert(m_d3dsdRequired.Format != D3DFMT_UNKNOWN);
    Assert(m_d3dsdRequired.Type == D3DRTYPE_TEXTURE);
    // 4 usages are allowed - any combination of autogen and RT
    Assert((m_d3dsdRequired.Usage & ~(D3DUSAGE_AUTOGENMIPMAP | D3DUSAGE_RENDERTARGET)) == 0);
    Assert(m_d3dsdRequired.Pool == D3DPOOL_DEFAULT);
    Assert(m_d3dsdRequired.MultiSampleType == D3DMULTISAMPLE_NONE);
    Assert(m_d3dsdRequired.MultiSampleQuality == 0);
    Assert(m_d3dsdRequired.Width);
    Assert(m_d3dsdRequired.Height);

    IFC(CD3DVidMemOnlyTexture::Create(
        &m_d3dsdRequired,
        m_uLevels,
        fIsEvictable,
        m_pDevice,
        &m_pVidMemOnlyTexture,
        pSharedHandle
        ));

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:  ScaleIntervalToPrefiltered
//
//  Synopsis:  Adjust interval for a uOriginalSize sized domain to a
//             prefiltered, uPrefilterSize sized domain
//
//             Rounding always expands the interval to include more.
//

VOID
ScaleIntervalToPrefiltered(
    __inout_ecount(1) UINT &uStart,
    __inout_ecount(1) UINT &uEnd,
    UINT uOriginalSize,
    UINT uPrefilterSize
    )
{
    Assert(uStart <= INT_MAX);
    Assert(uStart < uEnd);
    Assert(uEnd <= uOriginalSize);
    Assert(uOriginalSize <= INT_MAX);    // Not required here, but unexpected.

    //
    // Compute start making sure to round down
    //

    ULONGLONG ullStart = uStart;
    ullStart = (ullStart * uPrefilterSize) / uOriginalSize;
    if (uPrefilterSize > uOriginalSize)
    {
        // Scale up case - for 3D !fallback case
        Assert(ullStart >= uStart);
    }
    else
    {
        // Scale down case - regular prefiltering
        Assert(ullStart <= uStart);
    }
    uStart = static_cast<UINT>(ullStart);

    //
    // Compute end making sure to round up
    //

    ULONGLONG ullEnd = uEnd;
    ullEnd = (ullEnd * uPrefilterSize + uOriginalSize - 1) / uOriginalSize;
    if (uPrefilterSize > uOriginalSize)
    {
        // Scale up case - for 3D !fallback case
        Assert(ullEnd >= uEnd);
        Assert(ullEnd <= uPrefilterSize);
    }
    else
    {
        // Scale down case - regular prefiltering
        Assert(ullEnd <= uEnd);
    }
    uEnd = static_cast<UINT>(ullEnd);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::FillTexture
//
//  Synopsis:  Copies the bitmap samples over to the texture
//

HRESULT
CHwBitmapColorSource::FillTexture(
    )
{
    HRESULT hr = S_OK;

    IWICBitmapScaler *pIWICScaler = NULL;
    IWICFormatConverter *pConverter = NULL;
    IWICImagingFactory *pIWICFactory = NULL;
    IWICBitmapSource *pIWICBitmapSourceNoRef = NULL;
    IWICBitmapSource *pWGXWrapperBitmapSource = NULL;
    IWGXBitmapSource *pWICWrapperBitmapSource = NULL;

    IFC(WrapInClosestBitmapInterface(m_pIBitmapSource, &pWGXWrapperBitmapSource));
    pIWICBitmapSourceNoRef = pWGXWrapperBitmapSource; // No ref changes

    // This variable should be true iff m_pBitmap refers to the same bitmap
    // as pIWICBitmapSourceNoRef
    BOOL fBitmapSourceIsBitmap = (m_pBitmap != NULL);

    //
    // Disable CDeviceBitmap as a IWGXBitmap even when transforms aren't
    // used because using Lock will request that the entire collection of
    // shared surfaces get pulled to system memory.  Falling back to Copy lets
    // just the required dirty rectangles get pulled down, though this does
    // mean an extra sys-mem to sys-mem copy.
    //
    // Future Consideration:   Enable CDeviceBitmap as IWGXBitmap with specialized "full" Lock
    //
    fBitmapSourceIsBitmap =
        fBitmapSourceIsBitmap && (m_pBitmap->SourceState() != IWGXBitmap::SourceState::DeviceBitmap);

    //
    // Add a bitmap scaler, if needed.
    //

    #if DBG
    {
        UINT uWidth, uHeight;
        Assert(SUCCEEDED(pIWICBitmapSourceNoRef->GetSize(&uWidth, &uHeight)));
        Assert(m_uBitmapWidth  == uWidth);
        Assert(m_uBitmapHeight == uHeight);
    }
    #endif

    Assert(m_uBitmapWidth <= INT_MAX);
    Assert(m_uBitmapHeight <= INT_MAX);
    Assert(m_uPrefilterWidth <= INT_MAX);
    Assert(m_uPrefilterHeight <= INT_MAX);

    if (m_uBitmapWidth != m_uPrefilterWidth || m_uBitmapHeight != m_uPrefilterHeight)
    {
        //
        // We can scale up for filtering to fill textures to a power of 2 size.
        // Currently we do this for 3D only.
        //

        IFC(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));
        IFC(pIWICFactory->CreateBitmapScaler(&pIWICScaler));

        IFC(pIWICScaler->Initialize(pIWICBitmapSourceNoRef,
                                    m_uPrefilterWidth,
                                    m_uPrefilterHeight,
                                    WICBitmapInterpolationModeFant));

        pIWICBitmapSourceNoRef = pIWICScaler;  // No ref changes
        fBitmapSourceIsBitmap = FALSE;
    }

    //
    // Get and validate format
    //

    WICPixelFormatGUID fmtWIC;
    MilPixelFormat::Enum fmtMIL;

    IFC(pIWICBitmapSourceNoRef->GetPixelFormat(&fmtWIC));
    IFC(WicPfToMil(fmtWIC, &fmtMIL));

    if (m_d3dsdRequired.Format != PixelFormatToD3DFormat(m_fmtTexture))
    {
        RIP("Source bitmap has unrecognized format.");
        IFC(WGXERR_INTERNALERROR);
    }

    if (m_fmtTexture != fmtMIL)
    {
        //
        // Convert all other pixel formats to a format appropriate for hardware
        // acceleration using the SW format converter.
        //
        // Any unsupported pixel formats will be bounced by the
        // CFormatConverter object.
        //

        // Note: IWICFormatConverter will simply AddRef the source
        // image and return it if the source and destination formats are
        // the same.

        if (!pIWICFactory)
        {
            IFC(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));
        }

        IFC(pIWICFactory->CreateFormatConverter(&pConverter));
        IFC(pConverter->Initialize(
            pIWICBitmapSourceNoRef,
            MilPfToWic(m_fmtTexture),
            WICBitmapDitherTypeNone,
            NULL,
            0.0f,
            WICBitmapPaletteTypeCustom
            ));

        pIWICBitmapSourceNoRef = pConverter;  // No ref changes
        fBitmapSourceIsBitmap = FALSE;
    }

    //
    // Validate size
    //

    #if DBG
    {
        UINT uWidth, uHeight;
        Assert(SUCCEEDED(pIWICBitmapSourceNoRef->GetSize(&uWidth, &uHeight)));
        Assert(m_uPrefilterWidth == uWidth);
        Assert(m_uPrefilterHeight == uHeight);
    }
    #endif

    if (   (m_d3dsdRequired.Width  < (m_rcPrefilteredBitmap.Width<UINT>()))
        || (m_d3dsdRequired.Height < (m_rcPrefilteredBitmap.Height<UINT>())))
    {
        RIP("Source bitmap rect is larger than destination.");
        IFC(WGXERR_INTERNALERROR);
    }

    IFC(WrapInClosestBitmapInterface(pIWICBitmapSourceNoRef, &pWICWrapperBitmapSource));

    IFC(FillTextureWithTransformedSource(
        pWICWrapperBitmapSource,
        fBitmapSourceIsBitmap
        ));

    Assert(IsRealizationValid());

Cleanup:
    ReleaseInterfaceNoNULL(pWGXWrapperBitmapSource);
    ReleaseInterfaceNoNULL(pIWICFactory);
    ReleaseInterfaceNoNULL(pIWICScaler);
    ReleaseInterfaceNoNULL(pConverter);
    ReleaseInterfaceNoNULL(pWICWrapperBitmapSource);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::FillTextureWithTransformedSource
//
//  Synopsis:  Copies the bitmap samples over to the texture
//             The incoming source must be in the format of the texture and it
//             should already have a prefilter transformation applied if necessary.
//
//-----------------------------------------------------------------------------
HRESULT
CHwBitmapColorSource::FillTextureWithTransformedSource(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    BOOL fBitmapSourceIsCBitmap
    )
{
    HRESULT hr = S_OK;

   UINT uNewestUniquenessToken;

    // System memory surface to use to copy the bits from system
    // memory to video memory.
    // This surface is sometimes just a lightweight wrapper around
    // the bits in the IWGXBitmap.  Other times it holds a copy of the
    // bitmap.
    IDirect3DSurface9* pD3DSysMemSurface = NULL;

    // Lock for bitmap- needed if we want to copy to or reference in
    // the system surface
    IWGXBitmapLock *pILock = NULL;

    //
    // Texture is about to be updated, but only within
    // m_rcRequiredRealizationBounds.  Trim cached area to the area required so
    // that dirty rects are limited to the area for which they have meaning.
    // Later upon successful realization we expect that
    // m_rcCachedRealizationBounds will be set to the full required area and
    // the cached uniqueness value updated.
    //
    // Conveniently at the same time we can check if required area is
    // completely invalid.
    //
    // But before trimming cached area check if the current contents are
    // current (which means the only reason we are here is because required
    // area has changed.)  In that case, try to keep as much cached area as
    // possible by extending required area to include current cached area.  But
    // only extend such that extension places no additional realization burdens
    // now.
    //

    if (IsRealizationCurrent())
    {
        ExtendBaseByAdjacentSectionsOfRect(
            IN m_rcRequiredRealizationBounds,   /* rcBase */
            IN m_rcCachedRealizationBounds,     /* rcPossbileExtension */
            OUT m_rcRequiredRealizationBounds
            );
    }

    bool fCompletelyInvalid =
        !m_rcCachedRealizationBounds.Intersect(m_rcRequiredRealizationBounds);

    //
    // Get the list of dirty rects
    //
    // This is done without regard to being completely invalid because it also
    // prompts source to reset its dirty list on the next dirty add.
    //

    const MilRectU *rgDirtyRects = NULL;
    UINT cDirtyRects = 0;

    if (!GetDirtyRects(
            OUT &rgDirtyRects,
         IN OUT &cDirtyRects,
            OUT &uNewestUniquenessToken
            ))
    {
        fCompletelyInvalid = true;
    }
    else
    {
        // GetDirtyRects has returned true indicating a valid dirty rect list.
        // This means cached uniqueness matched either uniquess that will yield
        // a non-zero dirty rect list or the current uniqueness that yields a
        // zero length list.  But if it is the latter case it must be purely a
        // coincidence with in inaccurate m_uCachedUniqueness value, because
        // this realization code should only be reached when uniqueness is
        // different or some area is not yet realized.  Some area not yet
        // realized can be checked with m_rcCachedRealizationBounds not
        // containing m_rcRequiredRealizationBounds which has just been set.
        Assert(   (cDirtyRects > 0)
               || !m_rcCachedRealizationBounds.DoesContain(m_rcRequiredRealizationBounds));
    }

    Assert(cDirtyRects <= IWGXBitmap::c_maxBitmapDirtyListSize);

    // rgDestDirtyRects should be large enough to hold all possible dirty
    // rects.  Dirty rects come from IWGXBitmap::GetDirtyRects and required area
    // not covered by cached area.  GetDirtyRects may return up to
    // IWGXBitmap::c_maxBitmapDirtyListSize rectangles and subtracting cache from
    // required may generate up to 4 rectangles.
    CMilRectU rgDestDirtyRects[IWGXBitmap::c_maxBitmapDirtyListSize + 4];
    UINT cPrefilteredDirtyRects = 0;

    if (!fCompletelyInvalid)
    {
        C_ASSERT(IWGXBitmap::c_maxBitmapDirtyListSize < ARRAYSIZE(rgDestDirtyRects));

        cPrefilteredDirtyRects = ComputePrefilteredDirtyRects(
            rgDirtyRects,
            cDirtyRects,
            rgDestDirtyRects
            );

        if (   (cPrefilteredDirtyRects > 0)
            && rgDestDirtyRects[0].DoesContain(m_rcCachedRealizationBounds))
        {
            fCompletelyInvalid = true;
        }
    }

    if (fCompletelyInvalid)
    {
        rgDestDirtyRects[0] = m_rcRequiredRealizationBounds;
        cPrefilteredDirtyRects = 1;
    }
    else
    {
        // There should be at lease 4 rects left for
        // CalculateSubtractionRectangles to fill in. 
        Assert(cPrefilteredDirtyRects <= ARRAYSIZE(rgDestDirtyRects) - 4);

        cPrefilteredDirtyRects +=
            m_rcRequiredRealizationBounds.CalculateSubtractionRectangles(
                m_rcCachedRealizationBounds,
                OUT &rgDestDirtyRects[cPrefilteredDirtyRects],
                4
                );

    }

    if (cPrefilteredDirtyRects > 0)
    {
        CMilRectU *rgUpdateFromBitmapRects = rgDestDirtyRects;

        DynArrayIA<CMilRectU,ARRAYSIZE(rgDestDirtyRects)>
            rgDestDirtyRectsRemaining[2];


        if (m_pbcsRealizationSources)
        {
            PDynCMilRectUArray const rgrgRemainingRects[] =
            {
                &rgDestDirtyRectsRemaining[0],
                &rgDestDirtyRectsRemaining[1]
            };

            CHwBitmapColorSource *pbcsRealizationSource =
                m_pbcsRealizationSources;

            UINT uActiveOutputArrayIndex = 0;

            while (pbcsRealizationSource)
            {
                // Intersection is expected.  Without it why is there a reusable
                // source?
                Assert(m_rcRequiredRealizationBounds.DoesIntersect(
                    pbcsRealizationSource->m_rcRequiredRealizationBounds
                    ));

                IFC(UpdateFromReusableSource(
                    pIBitmapSource,
                    fBitmapSourceIsCBitmap,
                    pbcsRealizationSource,
                    cPrefilteredDirtyRects,
                    rgUpdateFromBitmapRects,
                    OUT &cPrefilteredDirtyRects,
                    OUT &rgUpdateFromBitmapRects,
                    ARRAYSIZE(rgrgRemainingRects),
                    rgrgRemainingRects,
                    IN OUT &uActiveOutputArrayIndex
                    ));

                // Advance to next realization source, if any.
                pbcsRealizationSource =
                    pbcsRealizationSource->m_pbcsRealizationSources;
            }
        }

        //
        // Process any updates needed from system memory bitmap
        //
        if (cPrefilteredDirtyRects > 0)
        {
            //
            // Determine which case we are in and prepare to push the source
            // bits to video memory in the appropriate way.
            //
            // 1. Create/createref a system memory texture if needed
            //
            bool fCopySourceToSysMemSurface = true;

            IFC(PrepareToPushSourceBitsToVidMem(
                fBitmapSourceIsCBitmap,
                &pILock,
                &fCopySourceToSysMemSurface,
                &pD3DSysMemSurface
                DBG_COMMA_PARAM(pIBitmapSource)
                ));

            // [from the synopsis of PushTheSourceBitsToVideoMemory()]
            // 2. optional- Copy dirty region from source to system memory source
            // 3. updates the video memory
            IFC(PushTheSourceBitsToVideoMemory(
                pIBitmapSource,
                cPrefilteredDirtyRects,
                rgUpdateFromBitmapRects,
                pD3DSysMemSurface,
                fCopySourceToSysMemSurface        // true will cause #2
                ));
        }

        // We've dirtied the 0 level and on some cards we need to update the other
        // levels of the mipmaps.  On other cards or if we don't have mipmaps
        // this is a no-op.
        IFC(m_pVidMemOnlyTexture->UpdateMipmapLevels());
    }

    // Update cached uniqueness and cached area now that texture update is
    // complete.
    m_uCachedUniquenessToken = uNewestUniquenessToken;
    m_rcCachedRealizationBounds = m_rcRequiredRealizationBounds;

    Assert(IsRealizationValid());

Cleanup:

    // The system memory surface should be released before
    // the bitmap is unlocked because the surface references
    // the bitmap's bits through the lock.

    // Note: this does not fully release the system memory surface since
    // we have another reference in a member variable. However, we
    // are okay because the location of the bitmap's bits do (should)
    // not change

    // Future Consideration:  Allow multiple locks on IWGXBitmap and
    // hold on to a lock here.
    ReleaseInterfaceNoNULL(pD3DSysMemSurface);

    if (pILock)
    {
        // Assert that we had a pD3DSysMemSurface (though it's half released at
        // this point), assuming success.
        Assert(pD3DSysMemSurface || FAILED(hr));

        // Release the lock.
        ReleaseInterfaceNoNULL(pILock);
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwBitmapColorSource::GetDirtyRects
//
//  Synopsis:  Gets an array of dirty rects from the bitmap.
//
//-------------------------------------------------------------------------
__success(true) bool
CHwBitmapColorSource::GetDirtyRects(
    __deref_out_ecount(*pcDirtyRects) MilRectU const ** const prgDirtyRects,
    __deref_in_range(0,0) __deref_out_range(0,5) UINT * const pcDirtyRects,
    __out_ecount(1) UINT * const puNewestUniquenessToken
    ) const
{
    // Always set uniqueness.  In absence of IWGXBitmap, uniqueness is whatever is
    // already cached.
    *puNewestUniquenessToken = m_uCachedUniquenessToken;

    //
    // Check for dirty rects if we are associated with a IWGXBitmap
    //

    return (m_pBitmap != NULL)
        && m_pBitmap->GetDirtyRects(
            OUT prgDirtyRects,
            OUT pcDirtyRects,
            IN OUT puNewestUniquenessToken
            );
}


//+------------------------------------------------------------------------
//
//  Function:  CHwBitmapColorSource::PrepareToPushSourceBitsToVidMem
//
//  Synopsis:  Determine which case we are in and prepare to push the source
//             bits to video memory in the appropriate way.                 
//
//             Create/createref a system memory texture if needed
//
//-------------------------------------------------------------------------
HRESULT
CHwBitmapColorSource::PrepareToPushSourceBitsToVidMem(
    BOOL fBitmapSourceIsCBitmap,
    __deref_out_ecount_opt(1) IWGXBitmapLock **ppILock,
        // non-null if we locked the IWGXBitmap- caller is responsible for unlocking, as
        // the bitmap should remain locked until after we PushTheBitsToVideoMemory()
    __out_ecount(1) bool *pfShouldCopySourceToSysMemSurface,
    __deref_out_ecount(1) IDirect3DSurface9** ppD3DSysMemSurface
    DBG_COMMA_PARAM(__in_ecount(1) IWGXBitmapSource *pIDBGBitmapSource)
    )
{
    HRESULT hr = S_OK;

    IWGXBitmapLock *pILock = NULL;
    void *pvBits = NULL;
    UINT uWidth = 0;
    UINT uHeight = 0;
    bool fCanShareBitsWithD3D = false;
    IMILDynamicResource *pDynamicResource = NULL;
    bool fShouldLockBitmap = false;

    *pfShouldCopySourceToSysMemSurface = true;
    *ppILock = NULL;

    if (   fBitmapSourceIsCBitmap
        && ((m_tlU == NaturalTexelLayout) || (m_tlU == FirstOnlyTexelLayout))
        && ((m_tlV == NaturalTexelLayout) || (m_tlV == FirstOnlyTexelLayout))
       )
    {
        //
        // Bits are ready and we can share them with D3D (LDDM)
        //
        // Reference the bitmap bits in a system memory surface and set up
        // the code to copy directly from this surface to the video memory
        // texture's level 0 surface.
        //

        Assert(m_pBitmap != NULL);

        //
        // We need to lock the bitmap in 2 cases
        //    1) We're on a LDDM device - so we can share the bits with D3D
        //    2) The bitmap is a dynamic resource - so we can cache the sys-mem texture
        if (m_pDevice->IsLDDMDevice())
        {
            fShouldLockBitmap = true;
        }
        else
        {
            //
            // Is this a dynamic resource?
            //
            if(SUCCEEDED(m_pBitmap->QueryInterface(
                IID_IMILDynamicResource,
                reinterpret_cast<void **>(&pDynamicResource)
                )))
            {
                IFC(pDynamicResource->IsDynamicResource(&fShouldLockBitmap));
            }
        }


        //
        // On pre-LDDM devices, we can't share the bitmaps bits with D3D, however
        // if it's a dynamic bitmap we still need to get a pointer of the bits
        // so GetSysMemUpdateSurfaceSource can potentially use a cached system
        // memory texture.
        //
        if (fShouldLockBitmap)
        {
            //
            // When realizing a sub-portion of a large texture the width and height
            // of the source may be different than the texture.
            //
            // We don't however expect prefiltering in this case.  If there was
            // prefiltering, or color conversion, we expect a different path to be
            // taken since we don't cache those results (don't have a IWGXBitmap
            // source).
            //

            Assert(m_uBitmapWidth  == m_uPrefilterWidth);
            Assert(m_uBitmapHeight == m_uPrefilterHeight);

            {
                WICRect rcLock = {0, 0, m_uBitmapWidth, m_uBitmapHeight};
                IFC(m_pBitmap->Lock(
                    &rcLock,
                    MilBitmapLock::Read,
                    &pILock
                    ));
            }

            // Get the bits from the bitmap
            UINT cbBufferSize;
            IFC(pILock->GetDataPointer(
                &cbBufferSize,
                reinterpret_cast<BYTE**>(&pvBits)
                ));

            UINT uSourceStride;
            IFC(pILock->GetStride(&uSourceStride));

            MilPixelFormat::Enum bitmapFormat;
            IFC(pILock->GetPixelFormat(&bitmapFormat));

            BYTE bPixelWidth = GetPixelFormatSize(bitmapFormat);

            AssertConstMsgW((bPixelWidth % 8) == 0, 
                            L"CHwBitmapColorSource::PrepareToPushSourceBitsToVidMem:\n"
                            L" Only support pixel formats with pixel sizes in multiples of 8bits");

            bPixelWidth /= 8;

            AssertConstMsgW(bPixelWidth >= 1, 
                            L"CHwBitmapColorSource::PrepareToPushSourceBitsToVidMem:\n"
                            L" Only support pixel formats with pixel size >= 8bits");

            // Assert that a format converter was not needed
            Assert(m_d3dsdRequired.Format == PixelFormatToD3DFormat(bitmapFormat));

            //
            // We can only share bits with D3D on an LDDM device where the pixel size
            // is an even multiple of the stride.
            //
            if (m_pDevice->IsLDDMDevice())
            {
                if ((uSourceStride % bPixelWidth) == 0)
                {
                    uWidth = uSourceStride / bPixelWidth;
                    uHeight = m_uBitmapHeight;
                    fCanShareBitsWithD3D = true;
                }
                else
                {
                    TraceTag((tagMILWarning,
                              "CHwBitmapColorSource::PrepareToPushSourceBitsToVidMem:\n"
                              "D3D only supports sharing bitmaps with strides that\n"
                              "                  are multiples of the pixel format size in bytes"));

                    ReleaseInterface(pILock);
                    pvBits = NULL;
                }
            }
        }
    }

    if (!fCanShareBitsWithD3D)
    {
        //
        // Can't share bits with D3D (or don't want to because of irregular
        // layout like border)
        //
        // Get a system surface to store a copy of the transformed bitmap, and
        // set up the code to copy the dirty source bits to this surface,
        // applying any format converters, etc. Later code will then copy the
        // bits from the system memory surface to the video memory texture's
        // level 0 surface.
        //

        uWidth = m_d3dsdRequired.Width;
        uHeight = m_d3dsdRequired.Height;
    } 

    //
    // Get the surface
    //

    IFC(GetSysMemUpdateSurfaceSource(
        pvBits,
        uWidth,
        uHeight,
        fCanShareBitsWithD3D,
        ppD3DSysMemSurface
        ));

    Assert(*ppD3DSysMemSurface);

    if (pILock)
    {
        *ppILock = pILock;  // Transfer reference
        pILock = NULL;
    }

    //
    // If we didn't share bits with D3D then we're going to need to copy
    // the bits to the system memory texture.
    //
    *pfShouldCopySourceToSysMemSurface = !fCanShareBitsWithD3D;

Cleanup:

    ReleaseInterfaceNoNULL(pILock);
    ReleaseInterface(pDynamicResource);

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::ComputePrefilteredDirtyRects
//
//  Synopsis:
//      Convert original source relative dirty rects to prefiltered source
//      relative dirty rects.
//

__range(0, cDirtyRects) UINT
CHwBitmapColorSource::ComputePrefilteredDirtyRects(
    __in_ecount(cDirtyRects) const MilRectU *rgDirtyRects,
    __in_range(0,5) UINT cDirtyRects,
    __out_ecount_part(cDirtyRects,return) CMilRectU *rgPrefilteredDirtyRects
    )
{
    UINT cDestDirtyRectCount = 0;

    Assert(!m_rcCachedRealizationBounds.IsEmpty());

    //
    // Iterate through rectangles that need set/updated
    //
    for (UINT i = 0; i < cDirtyRects; i++)
    {
        CMilRectU &rc = rgPrefilteredDirtyRects[cDestDirtyRectCount];

        rc = rgDirtyRects[i];

        Assert(rc.right <= m_uBitmapWidth);
        Assert(rc.bottom <= m_uBitmapHeight);

        //
        // Adjust rect as needed if there is prefiltering
        // We must do this adjustment at the time the rects are used
        // rather than with the rest of the prefilter logic because the 
        // dirty rect list cannot be changed or copied without making
        // another allocation.
        //

        if (m_uBitmapWidth != m_uPrefilterWidth)
        {
            ScaleIntervalToPrefiltered(
                IN OUT rc.left,
                IN OUT rc.right,
                m_uBitmapWidth,
                m_uPrefilterWidth
                );
        }

        if (m_uBitmapHeight != m_uPrefilterHeight)
        {
            ScaleIntervalToPrefiltered(
                IN OUT rc.top,
                IN OUT rc.bottom,
                m_uBitmapHeight,
                m_uPrefilterHeight
                );
        }


        Assert(rc.right <= m_uPrefilterWidth);
        Assert(rc.bottom <= m_uPrefilterHeight);

        EventWriteBitmapCopyInfo(rc.right-rc.left, rc.bottom - rc.top);

        //
        // Clip to portion of interest
        //

        if (rc.Intersect(m_rcCachedRealizationBounds))
        {
            // Keep this non-empty dirty rect
            cDestDirtyRectCount++;
        }
        // continue with next dirty rect
    }

    return cDestDirtyRectCount;
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::UpdateFromReusableSource
//
//  Synopsis:
//      Update this color source with bits from the given reusable color
//      source.  The list of rectangles may or may not intersect with valid
//      parts of source.
//
//      Upon completion a new list is generated containing all dirty areas that
//      still need pulled from the bitmap source.
//

HRESULT
CHwBitmapColorSource::UpdateFromReusableSource(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    BOOL fBitmapSourceIsCBitmap,
    __in_ecount(1) CHwBitmapColorSource *pbcsSource,
    __in_range(>=,1) UINT cDirtyRects,
    __in_ecount(cDirtyRects) CMilRectU *rgDirtyRects,
    __out_ecount(1) UINT *pcRemaining,
    __deref_out_ecount_full(*pcRemaining) CMilRectU **prgRemainingRects,
    UINT const cDirtyRectRemainingBuffers,
    __in_ecount(cDirtyRectRemainingBuffers) PDynCMilRectUArray const *rgprgDirtyRectsRemaining,
    __deref_in_range(0,cDirtyRectRemainingBuffers-1) __deref_out_range(0,cDirtyRectRemainingBuffers-1) UINT *puActiveOutputArrayIndex
    )
{
    HRESULT hr = S_OK;

    CD3DSurface *pVidMemSourceSurface = NULL;
    CD3DSurface *pVidMemDestSurface = NULL;

    // There shouldn't be a border with a reusable source.
    Assert(m_tlU != EdgeWrappedTexelLayout);
    Assert(m_tlU != EdgeMirroredTexelLayout);
    Assert(m_tlV != EdgeWrappedTexelLayout);
    Assert(m_tlV != EdgeMirroredTexelLayout);
    Assert(pbcsSource->m_tlU != EdgeWrappedTexelLayout);
    Assert(pbcsSource->m_tlU != EdgeMirroredTexelLayout);
    Assert(pbcsSource->m_tlV != EdgeWrappedTexelLayout);
    Assert(pbcsSource->m_tlV != EdgeMirroredTexelLayout);

    //
    // Update the surface by walking all valid portions of reusable source and
    // looking for overlap with dirty rects.
    //

    UINT uActiveOutputArrayIndex = *puActiveOutputArrayIndex;

    CMilRectU const *pValidSourceRect;
    UINT cValidSourceRects;

    IFC(pbcsSource->GetPointerToValidSourceRects(
        m_pBitmap,
        OUT cValidSourceRects,
        OUT pValidSourceRect
        ));

    while (cValidSourceRects-- > 0)
    {
        DynArray<CMilRectU> &rgDirtyRectsRemaining =
            *rgprgDirtyRectsRemaining[uActiveOutputArrayIndex];

        UINT cRemaining = 0;
        rgDirtyRectsRemaining.Reset(FALSE);

        for (UINT i = 0; i < cDirtyRects; ++i)
        {
            CMilRectU rcDirtySource = rgDirtyRects[i];

            if (!rcDirtySource.Intersect(*pValidSourceRect))
            {
                //
                // Collect entire dirty rectangle as a remaining dirty rect since
                // it is not covered by reusable source.
                //
                IFC(rgDirtyRectsRemaining.Add(rgDirtyRects[i]));
                cRemaining++;
            }
            else
            {
                //
                // Collect areas of dirty rectangle not covered by reusable source.
                //
                CMilRectU *rgRemainingRectSink;

                IFC(rgDirtyRectsRemaining.AddMultiple(4, &rgRemainingRectSink));

                cRemaining += rgDirtyRects[i].CalculateSubtractionRectangles(
                    rcDirtySource,
                    OUT rgRemainingRectSink,
                    4
                    );

                // SetCount to actual number used, which may be less than 4
                // added to count by AddMultiple above.
                rgDirtyRectsRemaining.SetCount(cRemaining);


                CMilRectU rcDirtyDest;

                // Now that some overlap is found check that source and destination
                // surfaces are prepared.  This is done once for the loop.
                if (!pVidMemDestSurface)
                {
                    //
                    // Make sure realization source is realized
                    //

                    if (!pbcsSource->IsRealizationValid())
                    {
                        IFC(pbcsSource->FillTextureWithTransformedSource(
                            pIBitmapSource,
                            fBitmapSourceIsCBitmap
                            ));
                    }

                    IFC(pbcsSource->m_pVidMemOnlyTexture->GetD3DSurfaceLevel(
                        0,
                        &pVidMemSourceSurface
                        ));

                    IFC(m_pVidMemOnlyTexture->GetD3DSurfaceLevel(0, &pVidMemDestSurface));

                    Assert(pVidMemDestSurface);
                }

                rcDirtyDest = rcDirtySource;

                //
                // We are reusing a existing realization that has its own place
                // where it keeps it realization.  So we need to offset
                // rgDirtySource before StretchRect.
                //
                // Note that this assumes source has same sense of layout as
                // destination as far as border or no border is concerned.  See
                // layout asserts above.
                //
                rcDirtySource.Offset(
                    -static_cast<int>(pbcsSource->m_rcPrefilteredBitmap.left),
                    -static_cast<int>(pbcsSource->m_rcPrefilteredBitmap.top)
                    );

                //
                // Offset rgDirtyDest for destination storage location.
                //
                rcDirtyDest.Offset(
                    -static_cast<int>(m_rcPrefilteredBitmap.left),
                    -static_cast<int>(m_rcPrefilteredBitmap.top)
                    );

                //
                // By this point the dirty rects have been processed into
                // (0,0)-(INT_MAX,INT_MAX) bound rectangles and should not be
                // empty. This means the source and dest rectangles may be directly
                // cast to an integer based rectangle.
                //
                Assert(!rcDirtySource.IsEmpty());
                Assert(rcDirtySource.right <= INT_MAX);
                Assert(rcDirtySource.bottom <= INT_MAX);
                Assert(!rcDirtyDest.IsEmpty());
                Assert(rcDirtyDest.right <= INT_MAX);
                Assert(rcDirtyDest.bottom <= INT_MAX);

                IFC(m_pDevice->StretchRect(
                        pVidMemSourceSurface,
                        reinterpret_cast<const RECT *>(&rcDirtySource),
                        pVidMemDestSurface,
                        reinterpret_cast<const RECT *>(&rcDirtyDest),
                        D3DTEXF_NONE  // No stretching, so NONE is fine.  NONE is
                                      // better than POINT only because RefRast
                                      // doesn't expose a cap and this call would
                                      // fail.
                        ));
            }
        }

        Assert(cRemaining == rgDirtyRectsRemaining.GetCount());

        cDirtyRects = cRemaining;
        rgDirtyRects = rgDirtyRectsRemaining.GetDataBuffer();

        uActiveOutputArrayIndex =
            (uActiveOutputArrayIndex + 1) % cDirtyRectRemainingBuffers;

        if (cRemaining == 0)
        {
            break;
        }

        //
        // Setup next iteration - next valid source rect
        //
        pValidSourceRect++;
    }

    #if DBG_ANAYLSIS
    if (rgDirtyRects == rgprgDirtyRectsRemaining[uActiveOutputArrayIndex]->GetDataBuffer())
    {
        Assert(cDirtyRects == rgprgDirtyRectsRemaining[uActiveOutputArrayIndex]->GetCount());
    }
    #endif

    *pcRemaining = cDirtyRects;
    *prgRemainingRects = rgDirtyRects;
    *puActiveOutputArrayIndex = uActiveOutputArrayIndex;

Cleanup:
    ReleaseInterfaceNoNULL(pVidMemSourceSurface);
    ReleaseInterfaceNoNULL(pVidMemDestSurface);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:  CHwBitmapColorSource::PushTheBitsToVideoMemory
//
//  Synopsis:  Pushes the bits from the source bitmap to its final destination
//             however that is necessary.
//
//             This handles steps 2 to 3 of the algorithm (comment duplicated above)
//             2. optional- Copy dirty region from source to system memory
//             3. updates the video memory
//
//-------------------------------------------------------------------------
HRESULT
CHwBitmapColorSource::PushTheSourceBitsToVideoMemory(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    __in_range(>=,1) UINT cDirtyRects,
    __inout_ecount(cDirtyRects) CMilRectU *rgDirtyRects,
    __inout_ecount(1) IDirect3DSurface9 *pD3DSysMemSurface,
    bool fCopySourceToSysMemSurface
            // true will cause #2
    )
{
    HRESULT hr = S_OK;

    IDirect3DSurface9 *pD3DDestSurface = NULL;

    bool fLockedSurface = false;

    // If one direction used an Edge layout then the other one is also expected
    // to have an Edge layout.  See ReconcileLayouts for details.
    Assert((m_tlU == EdgeWrappedTexelLayout || m_tlU == EdgeMirroredTexelLayout) ==
           (m_tlV == EdgeWrappedTexelLayout || m_tlV == EdgeMirroredTexelLayout));

    // Size of border around destination
    UINT const uBorderSize =
        (m_tlV == EdgeWrappedTexelLayout || m_tlV == EdgeMirroredTexelLayout) ? 1 : 0;

    UINT const uPixelSize = D3DFormatSize(m_d3dsdRequired.Format);
    D3DLOCKED_RECT d3dlrBitmapCopyDestination = { 0, NULL };
    UINT cbLockedBufferSize = 0;

    bool fUpdateBorder = false;

    //
    // Lock the surface we are copying the bits to (if necessary)
    //

    if (fCopySourceToSysMemSurface)
    {
        RECT rcTextureLock;

        rcTextureLock.left = 0;
        rcTextureLock.top = 0;
        rcTextureLock.right = static_cast<LONG>(m_d3dsdRequired.Width);
        rcTextureLock.bottom = static_cast<LONG>(m_d3dsdRequired.Height);

        Assert(!fLockedSurface);
        IFC(pD3DSysMemSurface->LockRect(
            &d3dlrBitmapCopyDestination,
            &rcTextureLock,
            D3DLOCK_NO_DIRTY_UPDATE
            ));

        WICRect rcMilTextureLock;
        rcMilTextureLock.X = 0;
        rcMilTextureLock.Y = 0;
        rcMilTextureLock.Width = static_cast<INT>(m_d3dsdRequired.Width);
        rcMilTextureLock.Height = static_cast<INT>(m_d3dsdRequired.Height);
        cbLockedBufferSize = GetRequiredBufferSize(m_fmtTexture, d3dlrBitmapCopyDestination.Pitch, &rcMilTextureLock);
        fLockedSurface = true;
    }

    IFC(m_pVidMemOnlyTexture->GetID3DSurfaceLevel(0, &pD3DDestSurface));

    //
    // If there is a border and any dirty rect touches an edge then the border
    // needs update.  To simplify logic here we mark the entire surface as
    // dirty.
    //

    if (uBorderSize)
    {
        Assert(uBorderSize == 1);

        CMilRectU rcUnionSrc = rgDirtyRects[0];

        for (UINT i = 1; i < cDirtyRects; i++)
        {
            rcUnionSrc.Union(rgDirtyRects[i]);
        }

        if (rcUnionSrc.left == 0                          ||
            rcUnionSrc.top == 0                           ||
            rcUnionSrc.right == m_d3dsdRequired.Width-2   ||
            rcUnionSrc.bottom == m_d3dsdRequired.Height-2 )
        {
            // Make sure to copy the entire source.
            rgDirtyRects[0] = CMilRectU(0, 0, m_d3dsdRequired.Width-2, m_d3dsdRequired.Height-2, XYWH_Parameters);
            cDirtyRects = 1;
            fUpdateBorder = true;
        }
    }

    // Border requires system memory surface:
    Assert(fCopySourceToSysMemSurface || uBorderSize == 0);
    Assert(fCopySourceToSysMemSurface || !fUpdateBorder);
    
    //
    // If we need to copy to system memory surface then make the update(s).
    //

    if (fCopySourceToSysMemSurface)
    {
        for (UINT i = 0; i < cDirtyRects; ++i)
        {
            CMilRectU const &rcDirty = rgDirtyRects[i];

            POINT ptDest = {
                rcDirty.left - m_rcPrefilteredBitmap.left,
                rcDirty.top - m_rcPrefilteredBitmap.top
            };

            WICRect rcCopy = {
                rcDirty.left, rcDirty.top, rcDirty.right - rcDirty.left, rcDirty.bottom - rcDirty.top
            };

            BYTE *pvDestPixels = reinterpret_cast<BYTE*>(d3dlrBitmapCopyDestination.pBits)
                                 + uPixelSize * static_cast<UINT>(ptDest.x)
                                 + d3dlrBitmapCopyDestination.Pitch * ptDest.y +
                                 // Offset according to border.
                                 uBorderSize * (uPixelSize + d3dlrBitmapCopyDestination.Pitch);

            IFC(pIBitmapSource->CopyPixels(
                &rcCopy,
                d3dlrBitmapCopyDestination.Pitch,
                cbLockedBufferSize,
                pvDestPixels
                ));
        }

        if (uBorderSize)
        {
            // Offset destination rectangles according to destination border
            for (UINT i = 0; i < cDirtyRects; ++i)
            {
                rgDirtyRects[i].Offset(uBorderSize, uBorderSize);
            }

            if (fUpdateBorder)
            {
                Assert(cDirtyRects == 1);

                // Here we pass the entire source rect offset into destination.
                //
                // Note that if we ever want to do a partial update with borders
                // then the resulting rectangle list may grow in the case of
                // tiling.
                UpdateBorders(
                    &rgDirtyRects[0],   // Entire source offset
                    uPixelSize,
                    d3dlrBitmapCopyDestination.Pitch,
                    cbLockedBufferSize,
                    reinterpret_cast<BYTE*>(d3dlrBitmapCopyDestination.pBits)
                    );

                // Make sure to dirty the entire destination including borders.
                // This is essentially an inflate of uBorderSize x uBorderSize
                // since the rectangle is already the entire source.
                rgDirtyRects[0] = CMilRectU(0, 0, m_d3dsdRequired.Width, m_d3dsdRequired.Height, XYWH_Parameters);
            }
        }
    }

    //
    // The texture should be unlocked before we call UpdateSurface on 
    // one of its surfaces
    //
    if (fLockedSurface)
    {
        IFC(pD3DSysMemSurface->UnlockRect());
        fLockedSurface = false;
    }

    //
    // Update the surface
    //

    Assert(!fLockedSurface);
    Assert(pD3DDestSurface);

    for (UINT i = 0; i < cDirtyRects; ++i)
    {
        CMilRectU &rcDirty = rgDirtyRects[i];

        POINT ptDest = {
            rcDirty.left - m_rcPrefilteredBitmap.left,
            rcDirty.top  - m_rcPrefilteredBitmap.top
        };

        CMilRectU &rcDirtySource = rcDirty;

        if (fCopySourceToSysMemSurface)
        {
            // We are not using a shared surface so the sys mem surface
            // looks like the video memory surface NOT the bitmap source and so
            // we need to offset rgDirtyRects before UpdateSurface.
            //
            // Note: this changes the rectangle in the array - that is fine.
            rcDirtySource.Offset(-static_cast<int>(m_rcPrefilteredBitmap.left),
                                 -static_cast<int>(m_rcPrefilteredBitmap.top));
        }

#if DBG
        // Turn on dirty tint for non-reference case
        if (fCopySourceToSysMemSurface)
        {
            DbgTintDirtyRectangle(
                d3dlrBitmapCopyDestination.pBits,
                d3dlrBitmapCopyDestination.Pitch,
                m_d3dsdRequired.Format,
                &rcDirtySource
                );
        }
#endif

        //
        // By this point the dirty list has been processed into a
        // (0,0)-(INT_MAX,INT_MAX) bound rectangle and should not be empty.
        // This means it may be directly cast to an integer based rectangle.
        //
        Assert(!rcDirtySource.IsEmpty());
        Assert(rcDirtySource.right <= INT_MAX);
        Assert(rcDirtySource.bottom <= INT_MAX);

        //
        // Use UpdateSurface to update the destination if the source is
        // a system memory surface.  If it isn't, then UpdateSurface will fail
        // because it can only take sources that are sysmem.  In the case that
        // the source is not sysmem we use StretchRect with the same source
        // and destination rect, so no stretching is actually done.
        //
        // Note that in order to StretchRect the destination has to be a
        // rendertarget texture, so we had to make sure to create the destination
        // with D3DUSAGE_RENDERTARGET.
        //

        IFC(m_pDevice->UpdateSurface(
                pD3DSysMemSurface,
                reinterpret_cast<const RECT *>(&rcDirtySource),
                pD3DDestSurface,
                &ptDest
                ));
    }

    //
    // Check for presence of composition debug utility
    //

    if (g_pMediaControl)
    {
        //
        // Update texture update stats
        //
        ULONG uUpdatedPixelsAcrossDirtyRects = 0;
        for (UINT i = 0; i < cDirtyRects; i++)
        {
            CMilRectU const &rcDirty = rgDirtyRects[i];
            ULONG uUpdatedPixels = rcDirty.Width<UINT>() * rcDirty.Height<UINT>() * uPixelSize;
            uUpdatedPixelsAcrossDirtyRects += uUpdatedPixels;
        }
        InterlockedExchangeAdd(reinterpret_cast<volatile LONG *>(&g_dwTextureUpdatesPerFrame), uUpdatedPixelsAcrossDirtyRects);
    }

Cleanup:
    if (fLockedSurface)
    {
        Assert(FAILED(hr));
        IGNORE_HR(pD3DSysMemSurface->UnlockRect());
    }

    ReleaseInterfaceNoNULL(pD3DDestSurface);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwBitmapColorSource::GetSysMemUpdateSurfaceSource
//
//  Synopsis:  Gets a system memory surface that references
//             the bitmap's bits- reusing the last one if possible.
//
//-------------------------------------------------------------------------
HRESULT CHwBitmapColorSource::GetSysMemUpdateSurfaceSource(
    __in_opt void *pvCurrentBits,
    UINT uWidth,
    UINT uHeight,
    bool fCanCreateFromBits,
    __deref_out_ecount(1) IDirect3DSurface9** ppD3DSysMemSurface
    )
{
    HRESULT hr = S_OK;

    if (pvCurrentBits)
    {
        if (pvCurrentBits == m_pvReferencedSystemBits)
        {
            Assert(m_pvReferencedSystemBits);
            Assert(m_pD3DSysMemRefSurface);

            Assert(m_uPrefilterWidth == m_uBitmapWidth);
            Assert(m_uPrefilterHeight == m_uBitmapHeight);

            // Width might be greater if the stride is larger
            Assert(uWidth >= m_uBitmapWidth);
            Assert(uHeight == m_uBitmapHeight);

            AssertSysMemSurfaceDescriptionNotChanged(
                m_pD3DSysMemRefSurface,
                uWidth,
                uHeight
                );

            *ppD3DSysMemSurface = m_pD3DSysMemRefSurface;
            m_pD3DSysMemRefSurface->AddRef();
            goto Cleanup;
        }
    }

    FreAssertConstMsgA((m_pvReferencedSystemBits == NULL),
                       "The bitmap bits moved. We cannot handle this well because we reference them"
                       );

    //
    // Create the surface
    //
    IFC(m_pDevice->CreateSysMemUpdateSurface(
        uWidth,
        uHeight,
        m_d3dsdRequired.Format,
        fCanCreateFromBits ? pvCurrentBits : NULL,
        ppD3DSysMemSurface
        ));


    //
    // Cache the sys-mem texture only when pvCurrentBits isn't NULL.
    //
    if (pvCurrentBits)
    {
        ReplaceInterface(m_pD3DSysMemRefSurface, *ppD3DSysMemSurface);

        m_pvReferencedSystemBits = pvCurrentBits;
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::IsValid
//
//  Synopsis:  Determine if this is valid; simply check if HW resource is valid
//

bool
CHwBitmapColorSource::IsValid() const
{
    return (m_pVidMemOnlyTexture && m_pVidMemOnlyTexture->IsValid());
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::IsOpaque
//
//  Synopsis:  Does the source contain alpha?  This method tells you.
//
//-----------------------------------------------------------------------------

bool
CHwBitmapColorSource::IsOpaque(
    ) const
{
    return !HasAlphaChannel(m_fmtTexture);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::Realize
//
//  Synopsis:  Create or get a realization of the current device independent
//             bitmap.  If already in the cache, just make sure the current
//             realization still works in this context.
//

HRESULT
CHwBitmapColorSource::Realize(
    )
{
    HRESULT hr = S_OK;

    Assert(m_pIBitmapSource);

    if (   m_pVidMemOnlyTexture
        && !m_pVidMemOnlyTexture->IsValid())
    {
        ReleaseInterface(m_pVidMemOnlyTexture);
    }

#if DBG
    if (m_pVidMemOnlyTexture)
    {
        //
        // Check if existing texture has enough texels
        // required for handling realization
        //

        UINT uWidth, uHeight;

        m_pVidMemOnlyTexture->GetTextureSize(&uWidth, &uHeight);

        Assert(m_d3dsdRequired.Width == uWidth);
        Assert(m_d3dsdRequired.Height == uHeight);
    }
#endif

    bool fValidRealization;

    if (!m_pVidMemOnlyTexture)
    {
        //
        // Create a new texture
        //

        IFC(CreateTexture(/* fIsEvictable = */ true, NULL));

        // Anytime a new texture is allocated, a full realization is needed.
        m_rcCachedRealizationBounds.SetEmpty();
        fValidRealization = false;
    }
    else
    {
        fValidRealization = IsRealizationValid();
    }

    if (!fValidRealization)
    {
        if (   !m_pBitmap
            || m_pBitmap->SourceState() != IWGXBitmap::SourceState::NoSource
           )
        {
            //
            // Populate the texture
            //

            IFC(FillTexture());
        }
        else
        {
            // Successful population (including population with nothing) means
            // there is a valid realization.  Update uniqueness if there is a
            // valid uniqueness to compare against.
            m_rcCachedRealizationBounds = m_rcRequiredRealizationBounds;
            if (m_pBitmap)
            {
                m_pBitmap->GetUniquenessToken(&m_uCachedUniquenessToken);
            }
        }
    }

    // Successful realization; so realization should be valid.
    Assert(IsRealizationValid());

Cleanup:
    // Release the possible realization sources that are only truly good for
    // this realization pass.
    ReleaseRealizationSources();

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::SendDeviceStates
//
//  Synopsis:  Send related texture states to the device
//

HRESULT
CHwBitmapColorSource::SendDeviceStates(
    DWORD dwStage,
    DWORD dwSampler
    )
{
    HRESULT hr = S_OK;

    Assert(IsRealizationValid());

    IFC(CHwTexturedColorSource::SendDeviceStates(
        dwStage,
        dwSampler
        ));

    IFC(m_pDevice->SetTexture(dwSampler, m_pVidMemOnlyTexture));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::SendVertexMapping
//
//  Synopsis:  Send the vertex mapping for this textured source to the vertex
//             builder.
//
//-----------------------------------------------------------------------------
HRESULT
CHwBitmapColorSource::SendVertexMapping(
    __inout_ecount_opt(1) CHwVertexBuffer::Builder *pVertexBuilder,
    MilVertexFormatAttribute mvfaLocation
    )
{
    HRESULT hr = S_OK;

    // Base call
    IFC(CHwTexturedColorSource::SendVertexMapping(pVertexBuilder,mvfaLocation));

    // If one direction used an Edge layout then the other one is also expected
    // to have an Edge layout.  See ReconcileLayouts for details.
    Assert((m_tlU == EdgeWrappedTexelLayout || m_tlU == EdgeMirroredTexelLayout) ==
           (m_tlV == EdgeWrappedTexelLayout || m_tlV == EdgeMirroredTexelLayout));

    if (   m_tlU != NaturalTexelLayout
           // Don't waffle for FirstOnly layout since we know that means
           // special mip-map case for 3D and we don't support waffling for
           // meshes.  Technically we would like to waffle, but it is assumed
           // that all samples stay with in natural texture range (excluding
           // "bleed-in" of garbage from unfilled texture with sub-levels) and
           // thus no waffling is needed.
        && m_tlU != FirstOnlyTexelLayout)
    {
        // Send information to vertex buffer about necessary waffling and base-tile
        // padding
    
        //
        // Decode the coordinate index from the mvfaLocation
        //

        DWORD dwCoordIndex;

        IFC(MVFAttrToCoordIndex(mvfaLocation, &dwCoordIndex));

        //
        // Send the mapping
        //
    
        // Bounding rectangle of real source base tile in the d3d texture in normalized (unit sq)
        // coordinates.  This is the rectangle inset by one texel from the actual texture.

        //  Future Consideration:  For what it's worth if this rectangle is set to be LARGER than the actual texture
        // we can get the effect of tiling a texture map with gaps between the tiles without actually
        // creating larger textures with gaps in them.

        float w = static_cast<float>(m_d3dsdRequired.Width);
        float h = static_cast<float>(m_d3dsdRequired.Height);
        
        CMilPointAndSizeF rect(1/w,1/h,(w-2)/w,(h-2)/h);

        WaffleModeFlags waffleMode = WaffleModeEnabled;
        if (m_tlU == EdgeMirroredTexelLayout)
            waffleMode = (WaffleModeFlags) (waffleMode | WaffleModeFlipX);
        
        if (m_tlV == EdgeMirroredTexelLayout)
            waffleMode = (WaffleModeFlags) (waffleMode | WaffleModeFlipY);
        
        IFC(pVertexBuilder->SetWaffling(dwCoordIndex, &rect, waffleMode));
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapColorSource::AddToReusableRealizationSourceList
//
//  Synopsis:
//      Set this bitmap color source as a reusable realization source
//      in the given list.
//
//-----------------------------------------------------------------------------

void
CHwBitmapColorSource::AddToReusableRealizationSourceList(
    __deref_inout_ecount_inopt(1) CHwBitmapColorSource * &pbcsReusableList
    )
{
    ReleaseInterfaceNoNULL(m_pbcsRealizationSources);
    m_pbcsRealizationSources = pbcsReusableList;
    pbcsReusableList = this;
    AddRef();
}

#if DBG
//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::AssertMinimalTextureDesc
//
//  Synopsis:  Asserts that the device can handle the surface description
//
//-----------------------------------------------------------------------------
VOID
CHwBitmapColorSource::AssertMinimalTextureDesc(
    __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
    D3DTEXTUREADDRESS taU,
    D3DTEXTUREADDRESS taV,
    __in_ecount(1) const D3DSURFACE_DESC *pd3dsdRequired
    )
{
    D3DSURFACE_DESC d3dsdRequired;
    
    RtlCopyMemory(
        OUT &d3dsdRequired,
        IN pd3dsdRequired,
        sizeof(d3dsdRequired)
        );

    Assert(pDevice->GetMinimalTextureDesc(
        &d3dsdRequired,
        TRUE,
        (GMTD_CHECK_ALL |
         (TextureAddressingAllowsConditionalNonPower2Usage(taU,taV) ?
          GMTD_NONPOW2CONDITIONAL_OK : 0)
        )
        ) == S_OK);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::AssertSysMemSurfaceDescriptionNotChanged
//
//  Synopsis:  Asserts that the surface description has not changed from what
//             we expect.
//
//-----------------------------------------------------------------------------
void CHwBitmapColorSource::AssertSysMemSurfaceDescriptionNotChanged(
    __in_ecount(1) IDirect3DSurface9 *pD3DSysMemSurface,
    UINT Width,
    UINT Height
    )
{
    D3DSURFACE_DESC desc;

    Verify(SUCCEEDED(pD3DSysMemSurface->GetDesc(&desc)));

    Assert(desc.Format ==               m_d3dsdRequired.Format);
    Assert(desc.Usage ==                (m_d3dsdRequired.Usage & desc.Usage));
    Assert(desc.Pool ==                 D3DPOOL_SYSTEMMEM);
    Assert(desc.MultiSampleType ==      m_d3dsdRequired.MultiSampleType);
    Assert(desc.MultiSampleQuality ==   m_d3dsdRequired.MultiSampleQuality);
    Assert(desc.Width ==                Width);
    Assert(desc.Height ==               Height);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::AssertSysMemTextureDescriptionNotChanged
//
//  Synopsis:  Asserts that the texture description has not changed from what
//             we expect.
//
//-----------------------------------------------------------------------------
void CHwBitmapColorSource::AssertSysMemTextureDescriptionNotChanged(
    __in_ecount(1) IDirect3DTexture9 *pD3DSysMemTexture
    )
{
    IDirect3DSurface9 *pD3DSysMemSurface = NULL;

    Verify(SUCCEEDED(pD3DSysMemTexture->GetSurfaceLevel(
        0,
        &pD3DSysMemSurface
        )));

    if (pD3DSysMemSurface)
    {
        AssertSysMemSurfaceDescriptionNotChanged(
            pD3DSysMemSurface,
            m_d3dsdRequired.Width,
            m_d3dsdRequired.Height
            );

        pD3DSysMemSurface->Release();
    }
}
#endif

#if DBG
//+----------------------------------------------------------------------------
//
//  Member:    DbgTintDirtyRectangle
//
//  Synopsis:  Tint bitmap dirty rectangles in debug to show update regions
//

//
// Bitmap dirty rectangle tint colors
//

extern const MilColorB c_rgDirtyRectangleTint[];
extern const int c_iNumDirtyRectangleTints;

const MilColorB c_rgDirtyRectangleTint[] =
{
    0xffa0ffff,
    0xffffa0ff,
    0xffffffa0,
};

const int c_iNumDirtyRectangleTints =  ARRAY_SIZE(c_rgDirtyRectangleTint);

void
DbgTintDirtyRectangle(
    __inout_xcount(
        iPitch * (prcDirty->bottom - 1) +
        sizeof(GpCC) * (prcDirty->right)
        ) void *pvDbgTintBits,
    INT iPitch,
    D3DFORMAT d3dFmt,
    const CMilRectU *prcDirty
    )
{
    if (IsTagEnabled(tagShowBitmapDirtyRectangles))
    {
        static UINT uDbgTintColor = 0;

        GpCC cDbgTint;
        INT iDbgIntensity;

        // This debug code will not work for other formats
        // unless we add special code.
        if (D3DFormatSize(d3dFmt) != sizeof(GpCC))
        {
            TraceTag((tagMILWarning,
                      "CHwBitmapColorSource::DbgTintDirtyRectangle "
                      "does not support the current pixel format. "
                      "Drawing without tinting."));
            goto Cleanup;
        }

        pvDbgTintBits =
            reinterpret_cast<BYTE*>(pvDbgTintBits)
            + sizeof(GpCC) * prcDirty->left
            + prcDirty->top * iPitch;

        Assert(uDbgTintColor < ARRAY_SIZE(c_rgDirtyRectangleTint));
        cDbgTint.argb = c_rgDirtyRectangleTint[uDbgTintColor];

        //
        // Debug stuff that tints software primitives purple
        //

        for (unsigned j = prcDirty->top; j < prcDirty->bottom; j++)
        {
            GpCC *pDbgColorData = reinterpret_cast<GpCC *>(pvDbgTintBits);

            for (unsigned i = prcDirty->left; i < prcDirty->right; i++)
            {
                GpCC &c = (*pDbgColorData);

                // Not a real intensity, but good enough to show dirty rectangles
                iDbgIntensity = (c.r + c.g + c.b)/3;

                c.r = static_cast<BYTE>(iDbgIntensity*cDbgTint.r / 255);
                c.g = static_cast<BYTE>(iDbgIntensity*cDbgTint.g / 255);
                c.b = static_cast<BYTE>(iDbgIntensity*cDbgTint.b / 255);

                pDbgColorData++;
            }

            pvDbgTintBits = reinterpret_cast<BYTE*>(pvDbgTintBits) + iPitch;
        }

        uDbgTintColor = (uDbgTintColor + 1) % c_iNumDirtyRectangleTints;
    }

Cleanup:
    return;
}

#endif DBG


//+----------------------------------------------------------------------------
//
//  Function:  SelfCopyPixels
//
//  Synopsis:  Copy source rectangle to new location (non-overlapping)
//             in image.  Does not check memory!
//
//-----------------------------------------------------------------------------
void
SelfCopyPixels(
    __in_ecount(1) const CMilRectU &rc,       // Source rectangle
    UINT x,                                      // Destination origin x
    UINT y,                                      // Destination origin y
    UINT cbStep,                                 // Distance twixt successive
                                                 // pixels, which is also size
                                                 // of a pixel
    UINT cbStride,                               // Distance between successive
                                                 // rows
    UINT cbBufferSize,                           // Size of buffer
    __inout_bcount(cbBufferSize) BYTE *pvPixels  // Pointer to start of output
    )
{
    #pragma prefast(suppress: 22013, "Offset calculations may not overflow")
    UINT offReadEnd = cbStep * rc.right + cbStride * (rc.bottom-1);
    UINT offWriteEnd = cbStep * (x+rc.Width()) + cbStride * (y+rc.Height()-1);

    if (cbBufferSize < offReadEnd)
    {
        RIP("Buffer size too small for source rectangle");
    }
    else if (cbBufferSize < offWriteEnd)
    {
        RIP("Buffer size too small for destination rectangle");
    }
    else
    {
        for (UINT i = rc.left; i < rc.right; ++i)
        {
            for (UINT j = rc.top; j < rc.bottom; ++j)
            {
                BYTE *pvSrc = pvPixels + j * cbStride + i * cbStep;
                BYTE *pvDst = pvPixels + ((j-rc.top)+y) * cbStride + ((i-rc.left)+x) * cbStep;
                Assert(pvDst + cbStep <= pvPixels + cbBufferSize);
                Assert(pvSrc + cbStep <= pvPixels + cbBufferSize);

                RtlCopyMemory(pvDst, pvSrc, cbStep);
            }
        }
    }
    
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapColorSource::UpdateBorders
//
//  Synopsis:  Given a dirty rectangle and a pointer (and description) of the
//             an image shaped like the D3D surface for this color source
//             this routine updates the borders if necessary.
//             Should only be called with non-NaturalTexelLayout.
//
//-----------------------------------------------------------------------------
void
CHwBitmapColorSource::UpdateBorders(
    __in_ecount(1)
    const CMilRectU *prc,    // Dirty rectangle in image - currently dbg
                                // only since it is always the entire source
                                // rectangle (offset).

    UINT cbStep,                // Number of bytes between successive pixels and pixel size
    UINT cbStride,              // Number of bytes between successive rows
    UINT cbBufferSize,          // Size of pvPixels buffer

    __inout_bcount(cbBufferSize)
    BYTE *pvPixels              // Pointer to start of output
    )
{
    Assert(m_tlU == EdgeMirroredTexelLayout || m_tlU == EdgeWrappedTexelLayout);
    Assert(m_tlV == EdgeMirroredTexelLayout || m_tlV == EdgeWrappedTexelLayout);

    // Get the width and height of the destination from the D3DSURFACE_DESC
    UINT uWidth = m_d3dsdRequired.Width;
    UINT uHeight = m_d3dsdRequired.Height;

    // The columns to use as the DESTINATION border from the left and
    // right columns in the source image, or -1 if the left and right
    // columns of the image weren't in the source rect.
    INT L = -1, R = -1;

    // Same thing for the rows
    INT T = -1, B = -1;

    Assert(prc->left == 1);
    {
        switch (m_tlU)
        {
        case EdgeMirroredTexelLayout:
            L = 0;
            break;
        case EdgeWrappedTexelLayout:
            L = uWidth-1;
            break;
        }
    }

    Assert(prc->right == uWidth-1);
    {
        switch (m_tlU)
        {
        case EdgeMirroredTexelLayout:
            R = uWidth-1;
            break;
        case EdgeWrappedTexelLayout:
            R = 0;
            break;
        }
    }

    Assert(prc->top == 1);
    {
        switch (m_tlV)
        {
        case EdgeMirroredTexelLayout:
            T = 0;
            break;
        case EdgeWrappedTexelLayout:
            T = uHeight-1;
            break;
        }
    }

    Assert(prc->bottom == uHeight-1);
    {
        switch (m_tlV)
        {
        case EdgeMirroredTexelLayout:
            B = uHeight-1;
            break;
        case EdgeWrappedTexelLayout:
            B = 0;
            break;
        }
    }

    //
    // Fix borders
    //

    //
    // Left and right borders
    //

    if (L != -1)
    {
        // This rectangle is where the left side ended up in the destination bitmap, so
        // shifted 1,1.
        CMilRectU leftSide(1, 1, 1, uHeight-2, XYWH_Parameters);
        // Copy left side to appropriate left or right border
        SelfCopyPixels( leftSide, L, 1,
                        cbStep, cbStride, cbBufferSize, pvPixels );
    }

    if (R != -1)
    {
        CMilRectU rightSide(uWidth-2, 1, 1, uHeight-2, XYWH_Parameters);
        // Copy right side to appropriate left or right border
        SelfCopyPixels( rightSide, R, 1,
                        cbStep, cbStride, cbBufferSize, pvPixels );
    }

    //
    // Top and bottom borders including corners
    //
    // Note: inclusion of corners here is possible because the earlier left and
    //       right border updates ensure we can treat the left and right
    //       border's top and bottom just like the interior top and bottoms.
    //

    if (T != -1)
    {
        CMilRectU topSide(0, 1, uWidth, 1, XYWH_Parameters);
        // Copy top side to appropriate top or bottom border
        SelfCopyPixels( topSide, 0, T,
                        cbStep, cbStride, cbBufferSize, pvPixels );
    }

    if (B != -1)
    {
        CMilRectU bottomSide(0, uHeight-2, uWidth, 1, XYWH_Parameters);
        // Copy bottom side to appropriate top or bottom border
        SelfCopyPixels( bottomSide, 0, B,
                        cbStep, cbStride, cbBufferSize, pvPixels );
    }

}




