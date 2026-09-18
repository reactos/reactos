// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        
//      CPP file for CViewportAlignedIntermediateRealizer
//

#include "precomp.hpp"

//
// "Spaces and Transforms"
// See BrushIntermediateRealizer.cpp
//


//+----------------------------------------------------------------------------
//
//  Member: 
//      CViewportAlignedIntermediateRealizer::CViewportAlignedIntermediateRealizer
//
//  Synopsis:  
//      ctor
//

CViewportAlignedIntermediateRealizer::CViewportAlignedIntermediateRealizer(
    __in_ecount(1) const BrushContext *pBrushContext,
        // Brush context information
    __in_ecount(1)  const CMILMatrix *pmatContentToViewport,
        // Transform inferred from the Viewbox, Viewport, Stretch, & Alignment properties
    __in_ecount(1)  const CMILMatrix *pmatViewportToWorld,
        // User-specified Viewport->World transform
    __in_ecount(1)  const MilPointAndSizeD *prcdViewport,
        // User-specified viewport in world coordinates
    __in_ecount_opt(1) const BrushCachingParameters *pCachingParams,
        // Optional brush-caching parameters.  If non-null, the realizer will determine
        // whether or not the previous realization can be re-used.
    MilTileMode::Enum tileMode
        // User-specified tile mode
    ) : CBrushIntermediateRealizer(
            pBrushContext,
            pmatContentToViewport,
            pmatViewportToWorld,
            prcdViewport,
            pCachingParams
            )
{
    m_tileMode = tileMode;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CViewportAlignedIntermediateRealizer::Realize
//
//  Synopsis:  
//      Realizes brush to intermediate surface.
//

HRESULT
CViewportAlignedIntermediateRealizer::Realize(
    __deref_out_ecount_opt(1) IWGXBitmapSource **ppCachedSurface,
        // Cached intermediate surface to use.  If non-NULL, ppIRenderTarget & ppDrawingContext will be NULL.
    __deref_out_ecount_opt(1) IMILRenderTargetBitmap **ppIRenderTarget,
        // Intermediate render target for this tile brush.  If non-NULL, ppCachedSurface will be NULL.
    __deref_out_ecount_opt(1) CDrawingContext **ppDrawingContext,
        // Drawing context to use on intermediate render target.  If non-NULL, ppCachedSurface will be NULL.
    __out_ecount(1) CMILMatrix* pmatSurfaceToWorldSpace,
        // Transforms from the ouput intermediate surface to world coordinates
    __out_ecount(1) BOOL *pfBrushIsEmpty,
        // Does this brush render nothing?
    __out_ecount_opt(1) CParallelogram *pSurfaceBoundsWorldSpace
        // The surface bounds in world space
    )
{
    HRESULT hr = S_OK;

    MilRectF rcBaseTile_SampleScaledViewportSpace;
    MilRectF rcRenderBounds_SampleScaledViewportSpace;

    UINT uSurfaceWidth, uSurfaceHeight;

    CMILMatrix matScaleOfViewportToWorld, matNonScaleOfViewportToWorld;
    CMILMatrix matScaleOfWorldToSampleSpace, matNonScaleOfWorldToSampleSpace;

    CMILMatrix matBaseTile_SampleScaledViewportSpaceToBaseTile_SurfaceSpace;
    CMILMatrix matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace;
    CMilPoint2F vecTranslationFromRenderedTileToBaseTile;

    CMilRectF rcIntermediateBounds_SurfaceSpace;

    CachedBrushRealizationState cachedRealizationState;
    IWGXBitmapSource *pCachableBitmap = NULL;

    BOOL fCanDecompose;    

    UINT uAdapterIndex = m_pBrushContext->uAdapterIndex;

    // If the adapter index is InvalidToken, then we can't support any HW-specific 
    // intermediates.  However, we can still look for intermediates that aren't 
    // specific to the HW device.
    if (uAdapterIndex == CMILResourceCache::InvalidToken)
    {
        uAdapterIndex = CMILResourceCache::SwRealizationCacheIndex;
    }                   

    // Initialize out-params
    *pfBrushIsEmpty = FALSE;
    *ppIRenderTarget = NULL;
    *ppDrawingContext = NULL;
    if (ppCachedSurface)
    {
        *ppCachedSurface = NULL;
    }

    //
    // Decompose WorldToSampleSpace Transform
    //

    m_pBrushContext->matWorldToSampleSpace.DecomposeMatrixIntoScaleAndRest(
        &matScaleOfWorldToSampleSpace,
        &matNonScaleOfWorldToSampleSpace,
        &fCanDecompose);

    if (!fCanDecompose)
    {
        // World matrix scales to 0 in the X or Y dimensions.
        *pfBrushIsEmpty = TRUE;
        goto Cleanup;
    }

    //
    // Decompose ViewportToWorld Transform
    //
    if (NULL != m_pmatViewportToWorld)
    {
        m_pmatViewportToWorld->DecomposeMatrixIntoScaleAndRest(
            &matScaleOfViewportToWorld,
            &matNonScaleOfViewportToWorld,
            &fCanDecompose);

        if (!fCanDecompose)
        {
            // ViewportToWorld matrix scales to 0 in the X or Y dimensions.
            *pfBrushIsEmpty = TRUE;
            goto Cleanup;
        }
    }

    //
    // Calculate the ideal base tile in sample-scaled viewport space
    //
    // The ideal base tile is what the intermediate surface would be if we
    // could have such a surface in floating point coordinates, and without
    // any maximum size constraints.
    //
    // For a description of how the spaces and transforms used here
    // are related see "Spaces and Transforms" at the top of this file.
    //
    CalculateIdealSurfaceSpaceBaseTile(
        m_pmatViewportToWorld ? &matScaleOfViewportToWorld : NULL,
        m_pmatViewportToWorld ? &matNonScaleOfViewportToWorld : NULL,
        &matScaleOfWorldToSampleSpace,
        &(m_pBrushContext->matWorldToSampleSpace),
        pfBrushIsEmpty,
        &rcBaseTile_SampleScaledViewportSpace,
        &rcRenderBounds_SampleScaledViewportSpace
        );

    // Avoid creating an intermediate surface if the brush is empty due
    // to a degenerate matrix
    if (*pfBrushIsEmpty)
    {
        goto Cleanup;
    }

    //
    // Calculate the integer size of the intermediate surface and a transform between
    // the ideal base tile and the actual intermediate surface.
    //
    CalculateSurfaceSizeAndMapping(
        &rcBaseTile_SampleScaledViewportSpace,
        &rcRenderBounds_SampleScaledViewportSpace,
        pfBrushIsEmpty,
        &uSurfaceWidth,
        &uSurfaceHeight,
        &matBaseTile_SampleScaledViewportSpaceToBaseTile_SurfaceSpace,
        &vecTranslationFromRenderedTileToBaseTile
        );

    // Avoid creating an intermediate surface if the brush is empty due
    // to being outside the viewable region
    if (*pfBrushIsEmpty)
    {
        goto Cleanup;
    }                               

    {
        //
        // Calculate a transform from the rendered tile in SurfaceSpace to the 
        // base tile in SampleScaledViewportSpace
        //
        
        matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace.SetToInverseOfTranslateOrScale(
            matBaseTile_SampleScaledViewportSpaceToBaseTile_SurfaceSpace
            );

        MatrixPrependTranslate2D(
            &matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace, 
            -vecTranslationFromRenderedTileToBaseTile.X, 
            -vecTranslationFromRenderedTileToBaseTile.Y
            );
    }

    //
    // Calculate matrix to associate with the rasterized base tile that transforms
    // from the surface to world space.
    //
    CalculateSurfaceToWorldMapping(
        &matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace,
        m_pmatViewportToWorld ? &matNonScaleOfViewportToWorld : NULL,
        &matScaleOfWorldToSampleSpace,
        pmatSurfaceToWorldSpace
        );        

    //
    // Calculating caching parameters if m_pCachingParams was passed in
    //    
    rcIntermediateBounds_SurfaceSpace.left = 0.0f;
    rcIntermediateBounds_SurfaceSpace.top = 0.0f;
    rcIntermediateBounds_SurfaceSpace.right = static_cast<float>(uSurfaceWidth);
    rcIntermediateBounds_SurfaceSpace.bottom = static_cast<float>(uSurfaceHeight); 

    // Determine whether or not a intermediate surface already exists, if 
    // caching is enabled
    if (NULL != m_pCachingParams)
    {
        // ppCachedSurface must be non-NULL when m_pCachingParams is non-NULL
        Assert(ppCachedSurface);
        
        m_pCachingParams->pIntermediateCache->FindIntermediate(
            IN uAdapterIndex,
            IN m_pCachingParams,
            IN (m_pmatViewportToWorld != NULL) ? &matScaleOfViewportToWorld : NULL,
            IN matScaleOfWorldToSampleSpace, 
            IN matRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace, 
            IN rcIntermediateBounds_SurfaceSpace,
            OUT ppCachedSurface,
            OUT &cachedRealizationState
            );
    }

    //
    // Create the intermediate surface for the base tile and the render context, 
    // if a cached surface wasn't found
    //
    if (ppCachedSurface == NULL ||
        *ppCachedSurface == NULL)
    {
        IFC(CreateSurfaceAndContext(
            m_pmatContentToViewport,
            m_pmatViewportToWorld ? &matScaleOfViewportToWorld : NULL,
            &matScaleOfWorldToSampleSpace,
            &matBaseTile_SampleScaledViewportSpaceToBaseTile_SurfaceSpace,
            uSurfaceWidth,
            uSurfaceHeight,
            ppIRenderTarget,
            ppDrawingContext
            ));

        // Store this intermediate if caching is enabled
        if (NULL != m_pCachingParams)
        {
            if (!m_pBrushContext->pRenderTargetCreator->WasUsedToCreateHardwareRT())
            {
                // uAdapterIndex is not related to the render target that asked 
                // for the realization to be created.
                uAdapterIndex = CMILResourceCache::SwRealizationCacheIndex;
            }                    

            IFC((*ppIRenderTarget)->GetCacheableBitmapSource(&pCachableBitmap));

            m_pCachingParams->pIntermediateCache->StoreIntermediate(
                IN pCachableBitmap,
                IN uAdapterIndex,
                IN cachedRealizationState
                );      
        }        
    }
    else        
    {
        CMILMatrix matCachedSurfaceToExpectedSurface;

        // When re-using the intermediate, the bounds of the intermediate we re-use
        // may be different from the calculated bounds due to the cache invalidation threshold.
        // To account for that, we need to prepend a transform which maps from actual bounds
        // to the calculated bounds in the Surface->World transform.  
        //
        // This is easier to conceptualize if you think about inverting the Surface->World 
        // transform.  Without this fixup, that transform will be from World -> 
        // Calculated Surface.  To get it to the actual surface, we need to add a Calculated->
        // Actual Surface transform so that the inverted transform will be 
        // World->Calculated Surface->Actual Surface.
        //
        // Invert that back, and you get Actual Surface -> Calculated Surface -> World.
        // Thus, we need to prepend a Actual Surface -> Calculated Surface transform to the
        // current Surface -> World to make this mapping correct.

        matCachedSurfaceToExpectedSurface.InferAffineMatrix(
            /* dest */ rcIntermediateBounds_SurfaceSpace,
            /* src */ cachedRealizationState.rcIntermediateBounds_SurfaceSpace
            );
        
        pmatSurfaceToWorldSpace->PreMultiply(
            matCachedSurfaceToExpectedSurface
            );

        // Change the intermediate bounds used to calculate pSurfaceBoundsWorldSpace during
        // the next block to the cached intermediate bounds.
        rcIntermediateBounds_SurfaceSpace = cachedRealizationState.rcIntermediateBounds_SurfaceSpace;
        
        *ppIRenderTarget = NULL;
        *ppDrawingContext = NULL;
    }

    //
    // Calculate the surface bounds in world space
    //
    if (pSurfaceBoundsWorldSpace)
    {    
        pSurfaceBoundsWorldSpace->Set(rcIntermediateBounds_SurfaceSpace);
        pSurfaceBoundsWorldSpace->Transform(pmatSurfaceToWorldSpace);
    }

Cleanup:

    // Release render target & context upon failure
    if (FAILED(hr))
    {
        ReleaseInterface(*ppIRenderTarget);
        ReleaseInterface(*ppDrawingContext);

        if (ppCachedSurface)
        {
            ReleaseInterface(*ppCachedSurface);
        }
    }

    ReleaseInterfaceNoNULL(pCachableBitmap);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     
//      CViewportAlignedIntermediateRealizer::CalculateIdealSurfaceSpaceBaseTile
//
//  Synopsis:   
//      Determines the size and position of the ideal base tile for the tile
//      brush in sample-scaled viewport space. The ideal base tile is what the 
//      intermediate surface would be if we could have such a surface in 
//      floating point coordinates, and without any maximum size constraints.
//
//      To determine the ideal base tile, the scale factors present in the
//      Viewport->World and World->SampleSpace transforms are applied to the
//      user-specified viewport.  The bounds of a rectangle to which this 
//      rectangle may be clipped to is also determined.
//
//      For a description of how the spaces and transforms used here
//      are related see "Spaces and Transforms" in BrushIntermediateRealizer.cpp.
//
//-----------------------------------------------------------------------------
void
CViewportAlignedIntermediateRealizer::CalculateIdealSurfaceSpaceBaseTile(
    __in_ecount_opt(1) const CMILMatrix *pmatScaleOfViewportToWorld,  
        // Scale portion of user-specified Viewport->World transform
    __in_ecount_opt(1) const CMILMatrix *pmatNonScaleOfViewportToWorld,  
        // Non-scale portion of the user-specified Viewport->World transform
    __in_ecount(1) const CMILMatrix *pmatScaleOfWorldToSampleSpace,      
        // Scale portion of the world->sample space transform
    __in_ecount(1) const CMILMatrix *pmatWorldToSampleSpace,
        // The world->sample space transform
    __inout_ecount(1) BOOL *pfBrushIsEmpty,             
        // Does this brush render nothing?
    __out_ecount(1) MilRectF *prcBaseTile_SampleScaledViewportSpace,
        // Resultant size and position of the ideal tile in scaled world space
    __out_ecount(1) MilRectF *prcRenderBounds_SampleScaledViewportSpace
        // Bounds of shape filled by base tile in scaled world spaces
    )
{
    // Initialize the viewable world extent to the world bounds, then intersect
    // this with the sample space clip later
    CMilRectF rcRenderBounds = m_pBrushContext->rcWorldSpaceBounds;
    Assert(rcRenderBounds.IsWellOrdered());

    //
    // If a clip exists, intersect the bounding box with the clip
    //
    if (!m_pBrushContext->rcSampleSpaceClip.IsInfinite())
    {
        CMilRectF rcWorldSpaceClip;
        CMILMatrix matSampleSpaceToWorld;

        // The clip rect is in sample space coordinates.  Use the inverse
        // worldToSampleSpace transform to bring it into world space, the same
        // coordinate space as the bounding box.
        if (!matSampleSpaceToWorld.Invert(*pmatWorldToSampleSpace))
        {
            *pfBrushIsEmpty = TRUE;
            goto Cleanup;
        }

        matSampleSpaceToWorld.Transform2DBounds(
            m_pBrushContext->rcSampleSpaceClip,
            OUT rcWorldSpaceClip
            );

        // Compute the clipped bounding box
        if (!rcRenderBounds.Intersect(rcWorldSpaceClip))
        {
            // If the clip or bounding box are empty then this brush will render nothing.
            *pfBrushIsEmpty = TRUE;
            goto Cleanup;
        }
    }

    //
    // Now that we have the viewable extents in world space, we must transform
    // them into WorldScaledViewport space so that we can use them to trim down
    // the size of the viewport in the calculation of the ideal base tile.
    // (The viewport will be transformed into WorldScaledViewport space shortly too).
    //
    // For a description of how the spaces and transforms used here are related
    // see "Spaces and Transforms" at the top of this file.
    //
    // WorldScaledViewport space = Viewport Space * ScaleOfViewportToWOrld
    // World space = Viewport Space * ScaleOfViewportToWorld * NonScaleOfViewportToWorld
    //
    // Therefore, WorldScaledViewport space = World space * (NonScaleOfViewportToWorld)^-1
    //
    if (pmatNonScaleOfViewportToWorld)
    {
        CMILMatrix matWorldToViewportWithoutScale;

        if (!matWorldToViewportWithoutScale.Invert(*pmatNonScaleOfViewportToWorld))
        {
            *pfBrushIsEmpty = TRUE;
            goto Cleanup;
        }

        matWorldToViewportWithoutScale.Transform2DBounds(
            rcRenderBounds,
            OUT rcRenderBounds
            );
    }

    {
        //
        // Scale the Viewport by the scale factor present in the ViewportToWorld matrix.
        // We call the result the "WorldScaled" viewport
        //
    
        CMilRectF rcWorldScaledViewport;
    
        if (pmatScaleOfViewportToWorld)
        {
            pmatScaleOfViewportToWorld->Transform2DBounds(m_rcViewport, OUT rcWorldScaledViewport);
        }
        else
        {
            rcWorldScaledViewport = m_rcViewport;
        }
        
        // Apply the worldToSampleSpace scale factor to the ideal base tile
        //
        // Once we've determined the bounds of the surface in worldScaledViewport
        // space, apply the scale factor of World->SampleSpace to avoid unnecessary
        // scaling of the rasterized tile.
        //
        // For a description of how the spaces and transforms used here
        // are related see "Spaces and Transforms" at the top of this file.
        //
        pmatScaleOfWorldToSampleSpace->Transform2DBounds(rcWorldScaledViewport, OUT *prcBaseTile_SampleScaledViewportSpace);
        pmatScaleOfWorldToSampleSpace->Transform2DBounds(rcRenderBounds, OUT *prcRenderBounds_SampleScaledViewportSpace);
    }

    // Avoid creating an intermediate surface if the brush is so small that it doesn't
    // affect a noticable portion of a single pixel.
    //
    // Even though this check is sufficient to avoid introducing a non-invertible
    // scaled world -> surface transform, we should use a tolerance that is
    // less arbitrary.
    // This check wrongly bails out on drawing tile brushes with very small viewports
    // instead of rendering with a nearly solid color.  However, CalculateSurfaceSizeAndMapping1D
    // relies on it so fix them both at the same time.
    if (IsCloseReal(prcBaseTile_SampleScaledViewportSpace->right, prcBaseTile_SampleScaledViewportSpace->left) ||
        IsCloseReal(prcBaseTile_SampleScaledViewportSpace->bottom, prcBaseTile_SampleScaledViewportSpace->top))
    {
        *pfBrushIsEmpty = TRUE;
        goto Cleanup;
    }

Cleanup:

    ;
}


//+--------------------------------------------------------------------------------------
//
//  Member:     CViewportAlignedIntermediateRealizer::CalculateSurfaceSizeAndMapping
//
//  Synopsis:   Determines the actual size of the intermediate surface we are going to
//              render the ideal base tile into.  This method also calculates the mapping
//              from the ideal tile in 'scaled world space' to the intermediate surface.
//
//---------------------------------------------------------------------------------------
void
CViewportAlignedIntermediateRealizer::CalculateSurfaceSizeAndMapping(
    __in_ecount(1) MilRectF *prcBaseTile_SampleScaledViewportSpace,        
        // Ideal base tile
    __in_ecount(1) MilRectF *prcRenderBounds_SampleScaledViewportSpace,           
        // Bound of rendered shape
    __inout_ecount(1) BOOL *pfBrushIsEmpty,             
        // Does this brush render nothing?
    __out_ecount(1) UINT *puSurfaceWidth,                             
        // Output width of intermediate surface
    __out_ecount(1) UINT *puSurfaceHeight,                            
        // Output height of intermediate surface
    __out_ecount(1) CMILMatrix *pmatSampleScaledViewportSpaceToSurfaceSpace,
        // Mapping from ideal tile to intermediate surface,
     __out_ecount(1) CMilPoint2F *pvecTranslationFromRenderedTileToBaseTile
        // Translation vector from the tile we are trying to render to the base tile
        // (components will only be non-0 if we are clipping to a non-base tile)
    )
{
    pmatSampleScaledViewportSpaceToSurfaceSpace->SetToIdentity();

    {
        TileMode1D::Enum tileModeX;

        switch (m_tileMode)
        {
        case MilTileMode::None:
        case MilTileMode::Extend:
            tileModeX = TileMode1D::None;
            break;

        case MilTileMode::FlipX:
        case MilTileMode::FlipXY:
            tileModeX = TileMode1D::Flip;
            break;

        case MilTileMode::FlipY:
        case MilTileMode::Tile:
            tileModeX = TileMode1D::Tile;
            break;

        default:
            RIPW(L"Unexpected tileMode encountered during TileBrush realization");

            //
            // Enum values coming
            // from bad packets are not yet validated by the UCE. We use a
            // default value here because of this.
            //
            tileModeX = TileMode1D::None;
            break;
        }

        CalculateSurfaceSizeAndMapping1D(
            tileModeX,
            prcBaseTile_SampleScaledViewportSpace->left,
            prcBaseTile_SampleScaledViewportSpace->right,
            prcRenderBounds_SampleScaledViewportSpace->left,
            prcRenderBounds_SampleScaledViewportSpace->right,
            pfBrushIsEmpty,            
            puSurfaceWidth,
            &pmatSampleScaledViewportSpaceToSurfaceSpace->_11,
            &pmatSampleScaledViewportSpaceToSurfaceSpace->_41,
            &pvecTranslationFromRenderedTileToBaseTile->X
            );
    
        if (*pfBrushIsEmpty)
        {
            goto Cleanup;
        }
    }

    {
        TileMode1D::Enum tileModeY;

        switch (m_tileMode)
        {
        case MilTileMode::None:
        case MilTileMode::Extend:
            tileModeY = TileMode1D::None;
            break;

        case MilTileMode::FlipY:
        case MilTileMode::FlipXY:
            tileModeY = TileMode1D::Flip;
            break;

        case MilTileMode::FlipX:
        case MilTileMode::Tile:
            tileModeY = TileMode1D::Tile;
            break;

        default:
            RIPW(L"Unexpected tileMode encountered during TileBrush realization");

            //
            // Enum values coming
            // from bad packets are not yet validated by the UCE. We use a
            // default value here because of this.
            //
            tileModeY = TileMode1D::None;
            break;
        }

        CalculateSurfaceSizeAndMapping1D(
            tileModeY,
            prcBaseTile_SampleScaledViewportSpace->top,
            prcBaseTile_SampleScaledViewportSpace->bottom,
            prcRenderBounds_SampleScaledViewportSpace->top,
            prcRenderBounds_SampleScaledViewportSpace->bottom,
            pfBrushIsEmpty,
            puSurfaceHeight,
            &pmatSampleScaledViewportSpaceToSurfaceSpace->_22,
            &pmatSampleScaledViewportSpaceToSurfaceSpace->_42,
            &pvecTranslationFromRenderedTileToBaseTile->Y
            );
    
        if (*pfBrushIsEmpty)
        {
            goto Cleanup;
        }
    }

    AdjustSurfaceSizeAndMapping1D(
        *puSurfaceHeight,
        puSurfaceWidth,
        &pmatSampleScaledViewportSpaceToSurfaceSpace->_11,
        &pmatSampleScaledViewportSpaceToSurfaceSpace->_41
        );

    AdjustSurfaceSizeAndMapping1D(
        *puSurfaceWidth,
        puSurfaceHeight,
        &pmatSampleScaledViewportSpaceToSurfaceSpace->_22,
        &pmatSampleScaledViewportSpaceToSurfaceSpace->_42
        );

Cleanup:
    ;
}


//+----------------------------------------------------------------------------
//
//  Member:     CViewportAlignedIntermediateRealizer::CalculateSurfaceSizeAndMapping1D
//
//  Synopsis:   For one dimension this helper determines the actual size
//              (pre-max-texture-cap) of the intermediate surface and the
//              mapping from the base tile in 'sample-scaled viewport space' 
//              to the intermediate surface.
//
//-----------------------------------------------------------------------------
void
CViewportAlignedIntermediateRealizer::CalculateSurfaceSizeAndMapping1D(
    TileMode1D::Enum tileMode1D,
    float rBaseTileMin_SampledScaledViewport,
    float rBaseTileMax_SampledScaledViewport,
    float rRenderBoundsMin,
    float rRenderBoundsMax,
    __inout_ecount(1) BOOL *pfBrushIsEmpty,
    __out_ecount(1) __deref_out_range(1,INT_MAX) UINT *puSize,
    __out_ecount(1) float *prSampleScaledViewportToSurfaceScale,
    __out_ecount(1) float *prSampleScaledViewportToSurfaceOffset,
    __out_ecount(1) float *pflTranslationFromRenderedTileToBaseTile
    )
{
    // Compute integral size of base tile and scale and offset from scaled world space to
    // intermediate surface assuming we're going to render the whole tile.
    float rBaseTileWidth_SampleScaledViewport = rBaseTileMax_SampledScaledViewport-rBaseTileMin_SampledScaledViewport;

    // This is ensured by the IsCloseReal check in CalculateIdealSurfaceSpaceBaseTile
    // which is unfortunately not correct.
    Assert(!(rBaseTileWidth_SampleScaledViewport <= 0));
    
    // Replace NaN w/0 & makes rBaseTileWidth_SampleScaledViewport non-negative
    rBaseTileWidth_SampleScaledViewport = ClampMinFloat(rBaseTileWidth_SampleScaledViewport, 0.f);
    
    INT iSizeUnadjusted = max(GpCeilingSat(rBaseTileWidth_SampleScaledViewport), static_cast<INT>(1));
    *puSize = static_cast<UINT>(iSizeUnadjusted);

    Assert(*puSize > 0 && *puSize <= INT_MAX);

    *prSampleScaledViewportToSurfaceScale = static_cast<float>(iSizeUnadjusted)/rBaseTileWidth_SampleScaledViewport;
    *prSampleScaledViewportToSurfaceOffset = -rBaseTileMin_SampledScaledViewport * *prSampleScaledViewportToSurfaceScale;
    *pflTranslationFromRenderedTileToBaseTile = 0.0f;
    
    // We can try to reduce the size of the intermediate if the shape we're
    // filling only uses a portion of the tile.  We only try to find the case
    // where the shape lies entirely within the bounds of the primary tile.
    // NOTE that we need to consider actual pixels not just floating point
    // bounds to make sure we have pixels on either side of our start point and
    // our end point.

    // Calculate the indices of the pixels touched by the shape bounds by
    // transforming the bounds into intermediate surface coordinates using our
    // tentative transform and then comparing them to the intermediate surface
    // size.

    // This conditional clipping code
    // handles all tiled modes the same.  We could do more accurate calculations
    // for tile and flip modes.
    rRenderBoundsMin *= *prSampleScaledViewportToSurfaceScale;
    rRenderBoundsMax *= *prSampleScaledViewportToSurfaceScale;
    rRenderBoundsMin += *prSampleScaledViewportToSurfaceOffset;
    rRenderBoundsMax += *prSampleScaledViewportToSurfaceOffset;

    // This computation gives INCLUSIVE start and stop pixels.  Hence the +/- 1
    // in the size computations.
    INT iBoundMinPixel = GpFloorSat(rRenderBoundsMin-0.5f);
    INT iBoundMaxPixel = GpFloorSat(rRenderBoundsMax+0.5f);

    //
    // Avoid computations if we are outside the range of integers this routine
    // can handle. Note: abs() not used in this computation because 
    // -INT_MIN == INT_MIN in signed integer space due to overflow.
    //
    if (   iBoundMinPixel >= (INT_MAX - iSizeUnadjusted)
        || iBoundMinPixel <= -(INT_MAX - iSizeUnadjusted)
        || iBoundMaxPixel >= (INT_MAX - iSizeUnadjusted)
        || iBoundMaxPixel <= -(INT_MAX - iSizeUnadjusted)
       )
    {
        goto Cleanup;
    }


    if (tileMode1D == TileMode1D::None)
    {
        //
        // We can simply intersect the integer render bounds with the
        // intermediate bounds. The floating point comparisons protect
        // us from the non-NaN safe floor operations.
        //

        if (   !(rRenderBoundsMin >= 0.0f) // structured for NaN
            || iBoundMinPixel < 0
            )
        {
            iBoundMinPixel = 0;
        }
        

        if (   !(rRenderBoundsMax < static_cast<FLOAT>(iSizeUnadjusted)) // structured for NaN
            || iBoundMaxPixel >= iSizeUnadjusted
           )
        {
            iBoundMaxPixel = iSizeUnadjusted - 1;
        }

        //
        // If the intersection is empty, avoid creating the intermediate
        //

        if (   iBoundMinPixel >= iSizeUnadjusted
            || iBoundMaxPixel < 0
            )
        {
            *pfBrushIsEmpty = TRUE;
            goto Cleanup;
        }

        *puSize = iBoundMaxPixel-iBoundMinPixel+1;
        *prSampleScaledViewportToSurfaceOffset -= iBoundMinPixel;
    }
    else
    {
        // The interval consistency could fail if we overflowed and got NaN or if we
        // started with NaN.
        if (iBoundMinPixel <= iBoundMaxPixel)
        {
            INT iTileShiftForMinPixel = 0;
            INT iTileShiftForMaxPixel = 0;

            CalculateTileShiftForPixel(
                iBoundMinPixel,
                iSizeUnadjusted,
                &iTileShiftForMinPixel
                );

            CalculateTileShiftForPixel(
                iBoundMaxPixel,
                iSizeUnadjusted,
                &iTileShiftForMaxPixel
                );

            //
            // If the rendering occurs outside the base tile, see if there is
            // another tile that we can use instead tile of which we can render
            //

            if (iTileShiftForMinPixel == iTileShiftForMaxPixel)
            {
                //
                // Both min and max are in the same tile
                //

                INT iShift = iTileShiftForMaxPixel * iSizeUnadjusted;
                *pflTranslationFromRenderedTileToBaseTile = static_cast<FLOAT>(iShift);

                iBoundMinPixel += iShift;
                iBoundMaxPixel += iShift;
                
                // If this tile was a flipped tile, we will take care of that later.

                //
                // Future Consideration:   "Future comment 1A" We could use
                // clamp in this dimension because we do not need to rely on
                // brush wrapping. This could give us more perf, but we'd have
                // to be capable of dealing with two TileMode1Ds instead of one
                // MilTileMode::Enum all the way through the stack. Note that we'd need
                // to take care of "Future comment 1B" as well
                //
            }
            else if (   tileMode1D == TileMode1D::Flip
                     && iTileShiftForMaxPixel - iTileShiftForMinPixel == -1)
            {
                //
                // Flip modes are optimized here for the case where the render
                // bounds span two tiles.
                //
                
                // Use tile in which max pixel exists as a base. If this tile
                // was a flipped tile, we will take care of that later.

                INT iShift = iTileShiftForMaxPixel * iSizeUnadjusted;

                iBoundMinPixel += iShift;
                iBoundMaxPixel += iShift;

                // We only need space from 0 to max(abs(iBoundMinPixel), abs(iBoundMaxPixel))

                Assert(iBoundMinPixel < 0);
                Assert(iBoundMaxPixel >= 0);
                
                if (iBoundMaxPixel > -iBoundMinPixel)
                {
                    iBoundMinPixel = 0;
                }
                else
                {
                    iBoundMaxPixel = -iBoundMinPixel;
                    iBoundMinPixel = 0;
                }

                if (iBoundMaxPixel >= iSizeUnadjusted)
                {
                    Assert(iBoundMaxPixel == iSizeUnadjusted);

                    //
                    // This is the case where the bounds we need to render
                    // cover an entire tile. There is nothing we can clip here
                    // so we back out of the operation.
                    //
                    goto Cleanup;
                }

                *pflTranslationFromRenderedTileToBaseTile = static_cast<FLOAT>(iShift);
            }
            else
            {
                // abort- there is nothing we can do

                //
                // Add logic to
                // handle the case where we span two tiles and are using
                // TileMode1D::Tile
                //
                goto Cleanup;
            }

            Assert(iBoundMinPixel >= 0);
            Assert(iBoundMinPixel < iSizeUnadjusted);

            Assert(iBoundMaxPixel >= 0);
            Assert(iBoundMaxPixel < iSizeUnadjusted);

            Assert(iBoundMaxPixel >= iBoundMinPixel);

            //
            // If the only contents needed are the contents of a flipped tile,
            // we must take care to flip the (now adjusted) min & max bounds
            // across the middle of the tile so that the right area of the
            // brush is drawn. Additionally, we must ensure that we flip the
            // contents when we are rendering with the intermediate.
            //
            if (   tileMode1D == TileMode1D::Flip
                && (iTileShiftForMaxPixel & 1) == 1)
            {
                // Reverse the min and the max, shifting over one tile.
                // This causes us to render the clipped region the wrong
                // way, but at least now it is the right portion.
                {
                    INT iBoundMinPixelOrig = iBoundMinPixel;
                    iBoundMinPixel = -iBoundMaxPixel + (iSizeUnadjusted - 1);
                    iBoundMaxPixel = -iBoundMinPixelOrig + (iSizeUnadjusted - 1);
                }

                //
                // Mucking with the translation vector here causes us to use
                // the rasterizer/waffler to flip across the border of the
                // clipped intermediate. Note that we could have achieved the
                // same effect with a flip transformation about the middle of
                // the visible region.
                //
                // Future Consideration:  "Future comment 1B" We could use
                // flip transformation (-1 scale) instead of relying on brush
                // wrapping logic to do the flip. This would involve adding a
                // second scale factor to the mix. This alternate algorithm is
                // needed to avoid using brush wrapping logic unecessarily. See
                // "Future comment 1A".
                //
                *pflTranslationFromRenderedTileToBaseTile += 
                    static_cast<FLOAT>(2 * (iBoundMaxPixel + 1) - iSizeUnadjusted);
            }

            Assert(iBoundMinPixel >= 0);
            Assert(iBoundMinPixel < iSizeUnadjusted);

            Assert(iBoundMaxPixel >= 0);
            Assert(iBoundMaxPixel < iSizeUnadjusted);

            Assert(iBoundMaxPixel >= iBoundMinPixel);

            //
            // Shrink the intermediate to the pixels used in rendering.
            //
            *puSize = iBoundMaxPixel-iBoundMinPixel+1;
            *prSampleScaledViewportToSurfaceOffset -= iBoundMinPixel;
        }
    }

Cleanup:
    ;
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CViewportAlignedIntermediateRealizer::CalculateTileShiftForPixel
//
//  Synopsis:  
//      Calculates the integer iTileShift such that
//          iPixel + (iTileShift * iTileSize)
//      is in the range of [0, iTileSize)
//

void
CViewportAlignedIntermediateRealizer::CalculateTileShiftForPixel(
    INT iPixel,
    INT iTileSize,
    __out_ecount(1) INT *piTileShift
    )
{
    if (iPixel < 0)
    {
        *piTileShift = ( -iPixel + iTileSize-1 ) / iTileSize;
    }
    else
    {
        *piTileShift = -iPixel / iTileSize;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CViewportAlignedIntermediateRealizer::AdjustSurfaceSizeAndMapping1D
//
//  Synopsis:
//      For one dimension this helper determines the actual size
//      (post-max-texture-cap and pow-2 constraints) of the intermediate
//      surface. It also determines the mapping from the ideal tile in 'scaled
//      world space' to the intermediate surface. Note that this function
//      depends on the results CalculateSurfaceSizeAndMapping1D for both
//      dimensions.
//
//-----------------------------------------------------------------------------
void
CViewportAlignedIntermediateRealizer::AdjustSurfaceSizeAndMapping1D(
    UINT uSizeInOtherDimension,
    __inout_ecount(1) __deref_out_range(1,MAX_TILEBRUSH_INTERMEDIATE_SIZE) UINT *puSize,
    __inout_ecount(1) float *prSampleScaledViewportToSurfaceScale,
    __inout_ecount(1) float *prSampleScaledViewportToSurfaceOffset
    )
{
    Assert(*puSize > 0);

    C_ASSERT(IS_POWER_OF_2(MAX_TILEBRUSH_INTERMEDIATE_SIZE));

    UINT uSizeCap = 0;

    if (   m_pBrushContext->fBrushIsUsedFor3D
        && m_tileMode != MilTileMode::None
        && m_tileMode != MilTileMode::Extend
        )
    {
        //
        // Scale up to the next power of 2. 3D requires bitmaps to be scaled to
        // the next power of 2 before rendering such that they can be tiled.
        // This logic will keep us from invoking the Fant scaler.
        //
        // Future Consideration:   If we ever implement mipmapping support
        // for intermediates we should take out the m_tileMode != MilTileMode::None
        // check. Mipmapping also requires power of 2 dimensions.
        //

        uSizeCap = RoundToPow2(*puSize);

        //
        // Cap the size at the MAX_TILEBRUSH_INTERMEDIATE_SIZE to avoid creating intermediates
        // with near infinite dimensions.
        //
        if (uSizeCap > MAX_TILEBRUSH_INTERMEDIATE_SIZE)
        {
            uSizeCap = MAX_TILEBRUSH_INTERMEDIATE_SIZE;
        }
    }
    else if (*puSize > MAX_TILEBRUSH_INTERMEDIATE_SIZE - 2)
    {
        //
        // Cap the size at the MAX_TILEBRUSH_INTERMEDIATE_SIZE to avoid creating intermediates
        // with near infinite dimensions.
        //
        if (uSizeInOtherDimension > MAX_TILEBRUSH_INTERMEDIATE_SIZE - 2)
        {
            //
            // The size in the other dimension will be increased to
            // MAX_TILEBRUSH_INTERMEDIATE_SIZE. Therefore, this dimension may
            // be at MAX_TILEBRUSH_INTERMEDIATE_SIZE too and we can avoid
            // waffling since this is a power of 2.
            //
            uSizeCap = MAX_TILEBRUSH_INTERMEDIATE_SIZE;
        }
        else
        {
            //
            // It's likely that the other dimension is not a power of 2. We
            // leave room for two more texels so that the conditional
            // non-power-of two tiling support has room to maneuver.

            // Lest someone think it is a good idea to check for powers of two
            // here... If we check for power of two then animation will look
            // funny. The width will change as you animate the height. This
            // still happens now, but only in one isolated case, and then only
            // by 1%.

            // Future Consideration:  One improvement that could be made is
            // to check the device caps to see if the power of two thing is an
            // issue.
            //
            uSizeCap = MAX_TILEBRUSH_INTERMEDIATE_SIZE - 2;
        }
    }

    if (uSizeCap != 0)
    {
        float rSizeCapScale = static_cast<float>(uSizeCap)/static_cast<float>(*puSize);
        *prSampleScaledViewportToSurfaceScale = *prSampleScaledViewportToSurfaceScale * rSizeCapScale;
        *prSampleScaledViewportToSurfaceOffset = *prSampleScaledViewportToSurfaceOffset * rSizeCapScale;
        *puSize = uSizeCap;
    }
}

//+--------------------------------------------------------------------------------------
//
//  Member:     CViewportAlignedIntermediateRealizer::CreateSurfaceAndContext
//
//  Synopsis:   Creates the intermediate surface and render context using the
//              predetermined surface size.  It then places a transform on the
//              render context to map from the Viewbox (which the content
//              we render is defined in) to the intermediate surface.
//
//---------------------------------------------------------------------------------------
HRESULT
CViewportAlignedIntermediateRealizer::CreateSurfaceAndContext(
        // Brush context information
    __in_ecount(1) const CMILMatrix *pmatContentToViewport,
        // Viewbox -> Viewport transform
    __in_ecount(1) const CMILMatrix *pmatScaleOfViewportToWorld,
        // Scale portion of user-specified Viewport->World transform
    __in_ecount(1) const CMILMatrix *pmatScaleOfWorldToSampleSpace,
        // Non-Scale portion of user-specified Viewport->World transform
    __in_ecount(1) const CMILMatrix *pmatSampleScaledViewportSpaceToSurfaceSpace,
        // Scaled world to surface mapping
    UINT uSurfaceWidth,
        // Width of render target to create
    UINT uSurfaceHeight,
        // Height of render target to create
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTarget,
        // Output render target
    __deref_out_ecount(1) CDrawingContext **ppDrawingContext          
        // Output drawing context
    )
{
    // Assert that required parameters are non-NULL
    Assert( pmatContentToViewport &&
            pmatScaleOfWorldToSampleSpace &&
            pmatSampleScaledViewportSpaceToSurfaceSpace &&
            ppIRenderTarget &&
            ppDrawingContext);

    HRESULT hr = S_OK;
    
    CMILMatrix matViewboxToSurface;

    IFC(CBrushIntermediateRealizer::CreateSurfaceAndContext(
        uSurfaceWidth,
        uSurfaceHeight,
        m_tileMode,
        ppIRenderTarget,
        ppDrawingContext
        ));

    //
    // Calculate Viewbox -> Surface Transform
    //
    // Apply a transform to the render context which maps instructions specified within
    // the Viewbox to the intermediate surface.  This transform includes the
    // Viewbox->Viewport transform, the scale transforms that are applied
    // to avoid scaling, and a translation that places the top-left of the surface
    // at the origin.
    //

    // First, apply the Viewbox to Viewport transform
    matViewboxToSurface = *pmatContentToViewport;

    // Apply the user-specified scale transform, if one exists
    if (NULL != pmatScaleOfViewportToWorld)
    {
        matViewboxToSurface.Multiply(*pmatScaleOfViewportToWorld);
    }

    // Then apply the world scale transform
    matViewboxToSurface.Multiply(*pmatScaleOfWorldToSampleSpace);

    // Finally, transform from scaled world space to the intermediate surface
    matViewboxToSurface.Multiply(*pmatSampleScaledViewportSpaceToSurfaceSpace);

    //
    // Push viewbox -> surface transform on render context
    //

    IFC((*ppDrawingContext)->PushTransform(&matViewboxToSurface));

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     CViewportAlignedIntermediateRealizer::CalculateSurfaceToWorldMapping
//
//  Synopsis:   Calculate a transform that maps the rasterized tile in the
//              intermediate surface to world space. This matrix is eventually
//              placed directly on the texture brush.
//
//-----------------------------------------------------------------------------
void
CViewportAlignedIntermediateRealizer::CalculateSurfaceToWorldMapping(
    __in_ecount(1) const CMILMatrix *pmatRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace,
        // Surface to scaled world mapping
    __in_ecount(1) const CMILMatrix *pmatNonScaleOfViewportToWorld,
        // Non-scale portion of the user-specified Viewport->World transform
    __in_ecount(1) const CMILMatrix *pmatScaleOfWorldToSampleSpace,
        // Scale portion of the user-specifiedViewport->World transform.
    __out_ecount(1) CMILMatrix *pmatSurfaceToWorldSpace
        // Output surface to world transform
)
{
    CMILMatrix matScaleOfSampleSpaceToWorld;

    // Initialize pmatSurfaceToWorld to pmatRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace

    *pmatSurfaceToWorldSpace = *pmatRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace;

    //
    // Invert the scale component of the worldToSampleSpace transform
    //
    // If the world scale in not invertible, this will already have been detected by
    // DecomposeMatrixIntoScaleAndRest.  Thus, we don't to check the return value of
    // Invert here again.
    //
    Verify(matScaleOfSampleSpaceToWorld.Invert(*pmatScaleOfWorldToSampleSpace));

    //
    // Apply the inverse scale transform to such that instead of going to 
    // SampleScaledViewportSpace, we go to WorldScaledViewportSpace.
    //
    //
    // For a description of how the spaces and transforms used here
    // are related see "Spaces and Transforms" at the top of this file.
    //
    // IdealSurfaceSpace = WorldScaledViewportSpace * ScaleOfWorldToSampleSpace
    // Therefore, IdealSurfaceSpace->WorldScaledViewport = (ScaleOfWorldToSampleSpace)^-1
    //
    pmatSurfaceToWorldSpace->Multiply(matScaleOfSampleSpaceToWorld);

    //
    // Apply the non-scale portion of the ViewportToWorld transform, such that instead of going
    // to WorldScaledViewportSpace, we go to WorldSpace.
    //
    // For a description of how the spaces and transforms used here
    // are related see "Spaces and Transforms" at the top of this file.
    //
    // WorldSpace = WorldScaledViewport space * NonScaleOfViewportToWorld
    //
    if (NULL != pmatNonScaleOfViewportToWorld)
    {
        pmatSurfaceToWorldSpace->Multiply(*pmatNonScaleOfViewportToWorld);
    }
}



